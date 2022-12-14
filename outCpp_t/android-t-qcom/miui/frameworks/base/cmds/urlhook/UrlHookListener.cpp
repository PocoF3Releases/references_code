
#include <errno.h>
#include <linux/netfilter.h> /* for NF_ACCEPT */
#include <linux/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "UrlHookLog.h"
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "UrlHookListener.h"
#include "UrlHookMsg.h"

/////////////////////////////////////////////////////////////////////////////
// find uid by ip packet
struct ip_t {
  uint32_t ip;
  uint16_t port;
};

struct ip_hdr_t {
  uint8_t hdr_len : 4;
  uint8_t v4 : 4;
  uint8_t tos;
  uint16_t pkt_len;
  uint16_t id;
  uint16_t offset;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t hdr_checksum;
  uint32_t src_ip;
  uint32_t dst_ip;
  /* extra */
};

struct tcp_hdr_t {
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t squ_num;
  uint32_t ack_squ;
  uint16_t flags;
  uint16_t window;
  uint32_t checksum;
  uint32_t imm_ptr;
};

struct tcp_net_stat_t {
  uint32_t local_ip;
  uint32_t remote_ip;
  uint16_t local_port;
  uint16_t remote_port;
  uint32_t uid;
  uint8_t state;
};

struct tcp_net_stat_infos_t {
  uint32_t count;
  tcp_net_stat_t stats[1];
};

#define tcp_net_state_infos_size(count)                                        \
  (sizeof(tcp_net_stat_infos_t) + ((count)-1) * sizeof(tcp_net_stat_t))

enum {
  STAT_ERROR = 0,
  STAT_TCP_ESTABLISHED,
  STAT_TCP_SYN_SEND,
  STAT_TCP_SYN_RECV,
  STAT_TCP_FIN_WAIT1,
  STAT_TCP_FIN_WAIT2,
  STAT_TCP_TIME_WAIT,
  STAT_TCP_CLOSE,
  STAT_TCP_CLOSE_WAIT,
  STAT_TCP_LAST_ACK,
  STAT_TCP_LISTEN,
  STAT_TCP_CLOSING,
  STAT_TCP_MAX
};

typedef union iaddr iaddr;
union iaddr {
  unsigned u;
  unsigned char b[4];
};

#define NET_TCP "/proc/net/tcp"
#define STAT_BUF_SIZE 100

static const char *state_names[] = {"Error",     "Established", "Syn Send",
                                    "Syn Recv",  "Fin Wait1",   "Fin Wait2",
                                    "Time Wait", "Close",       "Close Wait",
                                    "Last ACK",  "Listen",      "Closing"};

static const char *state_name(int stat) {
  if (stat < 0 || stat >= STAT_TCP_MAX)
    return "Unkown";
  return state_names[stat];
}

static tcp_net_stat_infos_t *alloc_net_state_infos(tcp_net_stat_infos_t *pinfos,
                                                   int count,
                                                   tcp_net_stat_t *states) {
  if (count <= 0)
    return pinfos;

  int add_start = 0;

  if (pinfos == NULL) {
    pinfos = (tcp_net_stat_infos_t *)malloc(tcp_net_state_infos_size(count));
  } else {
    add_start = pinfos->count;
    pinfos = (tcp_net_stat_infos_t *)realloc(
        pinfos, tcp_net_state_infos_size(add_start + count));
  }

  pinfos->count = add_start + count;
  memcpy(&(pinfos->stats[add_start]), states, count * sizeof(tcp_net_stat_t));

  return pinfos;
}

static tcp_net_stat_infos_t *read_tcp_net_infos() {
  FILE *fp = fopen(NET_TCP, "r");

  if (fp == NULL) {
    ALOGE("Cannot open %s error:%s", NET_TCP, strerror(errno));
    return NULL;
  }

  Buffer<char, BUFSIZ> buf;
  tcp_net_stat_t stats[STAT_BUF_SIZE];
  tcp_net_stat_infos_t *pinfos = NULL;

  // skip the header line
  fgets(buf.Data(), buf.Size(), fp);
  int idx = 0;
  while (fgets(buf.Data(), buf.Size(), fp)) {
    // sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt
    // uid  timeout inode
    iaddr laddr, raddr;
    unsigned int lport, rport, state, txq, rxq, num, tr, tr_when, retrnsmt, uid;
    int n = sscanf(buf.Data(), " %d: %x:%x %x:%x %x %x:%x %x:%x %x %d", &num,
                   &laddr.u, &lport, &raddr.u, &rport, &state, &txq, &rxq, &tr,
                   &tr_when, &retrnsmt, &uid);
    if (n != 12) {
      continue;
    }

    if (state == STAT_TCP_ESTABLISHED || state == STAT_TCP_SYN_SEND ||
        state == STAT_TCP_SYN_RECV || state == STAT_TCP_FIN_WAIT1 ||
        state == STAT_TCP_FIN_WAIT2 || state == STAT_TCP_TIME_WAIT ||
        state == STAT_TCP_LISTEN) {

      tcp_net_stat_t *pns = &stats[idx++];
      pns->local_ip = laddr.u;
      pns->remote_ip = raddr.u;
      pns->local_port = (uint16_t)lport;
      pns->remote_port = (uint16_t)rport;
      pns->uid = uid;
      pns->state = state;

      if (idx >= STAT_BUF_SIZE) {
        pinfos = alloc_net_state_infos(pinfos, idx, stats);
        idx = 0;
      }
    }
  }

  if (idx > 0) {
    pinfos = alloc_net_state_infos(pinfos, idx, stats);
  }

  fclose(fp);

  return pinfos;
}

static int read_src_dst_ips(void *pkt, ip_t *psrc, ip_t *pdst, int len) {
  ip_hdr_t *pip_hdr = (ip_hdr_t *)pkt;

  int ip_hdr_len = pip_hdr->hdr_len * 4;
  if (ip_hdr_len >= len) {
    return 0;
  }

  psrc->ip = pip_hdr->src_ip;
  pdst->ip = pip_hdr->dst_ip;

  /*get tcp header*/
  tcp_hdr_t *ptcp_hdr = (tcp_hdr_t *)((char *)pkt + ip_hdr_len);

  psrc->port = ntohs(ptcp_hdr->src_port);
  pdst->port = ntohs(ptcp_hdr->dst_port);

  return 0;
}

static void dump_tcp_net_stat_infos(tcp_net_stat_infos_t *pinfos) {
  if (CAN_DEBUG) {
    size_t i;
    for (i = 0; i < pinfos->count; i++) {
      tcp_net_stat_t *pstat = &(pinfos->stats[i]);
      iaddr laddr, raddr;
      laddr.u = pstat->local_ip;
      raddr.u = pstat->remote_ip;
      ALOGI("tcp %d.%d.%d.%d:%d -> %d.%d.%d.%d:%d %d %s", laddr.b[0],
            laddr.b[1], laddr.b[2], laddr.b[3], pstat->local_port, raddr.b[0],
            raddr.b[1], raddr.b[2], raddr.b[3], pstat->remote_port, pstat->uid,
            state_name(pstat->state));
    }
  }
}

static bool test_ip_port(tcp_net_stat_t *pstat, ip_t *p1, ip_t *p2) {
  ip_t *test_ips[2] = {0, 0};
  int test_ip_count = 0;
  if (p1->port >= 1024)
    test_ips[test_ip_count++] = p1;
  if (p2->port >= 1024)
    test_ips[test_ip_count++] = p2;

  if (test_ip_count == 0)
    return false;

  for (int i = 0; i < test_ip_count; i++) {
    if (pstat->local_port >= 1024 && pstat->local_ip == test_ips[i]->ip &&
        pstat->local_port == test_ips[i]->port) {
      return true;
    }
    if (pstat->remote_port >= 1024 && pstat->remote_ip == test_ips[i]->ip &&
        pstat->remote_port == test_ips[i]->port) {
      return true;
    }
  }
  return false;
}

static uint32_t find_uid(tcp_net_stat_infos_t *pinfos, ip_t *psrc, ip_t *pdst) {
  size_t i;

  for (i = 0; i < pinfos->count; i++) {
    tcp_net_stat_t *pstat = &(pinfos->stats[i]);

    if (test_ip_port(pstat, psrc, pdst))
      return pstat->uid;
  }
  return 0;
}

uint32_t find_uid_by_ip_packet(void *pkt, int len) {
  // read src:port, dst:port
  ip_t src, dst;

  tcp_net_stat_infos_t *pinfos;

  if (read_src_dst_ips(pkt, &src, &dst, len) < 0) {
    return 0;
  }

  pinfos = read_tcp_net_infos();

  if (pinfos == NULL) {
    ALOGE("read net tcp infos failed!");
    return 0;
  }

  uint32_t uid = find_uid(pinfos, &src, &dst);

  if (CAN_DEBUG) {
    iaddr laddr, raddr;
    laddr.u = src.ip;
    raddr.u = dst.ip;
    ALOGI("IP Package: %d.%d.%d.%d:%d => %d.%d.%d.%d:%d  uid=%d", laddr.b[0],
          laddr.b[1], laddr.b[2], laddr.b[3], src.port, raddr.b[0], raddr.b[1],
          raddr.b[2], raddr.b[3], dst.port, uid);
    dump_tcp_net_stat_infos(pinfos);
  }

  free(pinfos);

  return uid;
}

////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_NFLOG

#include <cutils/uevent.h>
#include <netlink/attr.h>
#include <netlink/genl/genl.h>
#include <netlink/handlers.h>
#include <netlink/msg.h>
#include <sysutils/SocketListener.h>

//#include <linux/netfilter/nfnetlink.h>
#include <linux/netfilter/nfnetlink_compat.h>
#include <linux/netfilter/nfnetlink_log.h>

#if PLATFORM_SDK_VERSION >= 23
#include "pcap-netfilter-linux-android.h"
#endif

#define IS_IN_URLHOOK_LISTENER_CPP
#include "nflog-help.h"

#define LOCAL_NFLOG_PACKET ((NFNL_SUBSYS_ULOG << 8) | NFULNL_MSG_PACKET)

class NFLogListener : public UrlHookListener {

  struct InnerListener : public SocketListener {
    NFLogListener *mOut;
    InnerListener(NFLogListener *out, int socket)
        : SocketListener(socket, false), mOut(out) {}

    bool onDataAvailable(SocketClient *cli) {
      return mOut->onDataAvailable(cli);
    }
  };

  InnerListener *mInner;

public:
  enum { GROUP_SIZE = 8 * 1024 };
  NFLogListener(char **arg, int count) : mInner(NULL), mGroup(0), mNFLogFd(0) {
    ArgCfg ac(arg, count);
    mGroup = ac.getInt("--group");
  }

  ~NFLogListener() { stop(); }

  bool onDataAvailable(SocketClient *cli) {
    int socket = cli->getSocket();
    ssize_t count;
    uid_t uid = -1;

    bool require_group = false;

    count = TEMP_FAILURE_RETRY(
        uevent_kernel_recv(socket, mBuff, sizeof(mBuff), require_group, &uid));

    if (count < 0) {
      ALOGE("NFLogListener recv failed(%s)", strerror(errno));
    }

    dump(mBuff, count, uid);

    // parse data
    parseMessage(count);

    return true;
  }

private:
  int mGroup;
  int mNFLogFd;
  char mBuff[GROUP_SIZE];

  void dump(const char *buff, ssize_t count, uid_t uid) {
    DEBUG("recv %d, uid=%d", (int)count, uid);
    logbuff("recv", buff, count);
  }

  void parseMessage(ssize_t size) {
    struct nlmsghdr *nh;

    char *buffer = mBuff;
    for (nh = (struct nlmsghdr *)buffer;
         NLMSG_OK(nh, (unsigned)size) && (nh->nlmsg_type != NLMSG_DONE);
         nh = NLMSG_NEXT(nh, size)) {
      DEBUG("message type:0x%x", nh->nlmsg_type);
      if (nh->nlmsg_type != LOCAL_NFLOG_PACKET) {
        ALOGE("Is not nflog packet!");
        return;
      }

      parseNfPacketMessage(nh);
    }
  }

  void parseNfPacketMessage(struct nlmsghdr *nh) {
    uint32_t uid = 0;
    int len = 0;
    char *raw = NULL;
    char prefix[32] = "\0";

    struct nlattr *uid_attr =
        nlmsg_find_attr(nh, sizeof(struct genlmsghdr), NFULA_UID);
    if (uid_attr) {
      uid = ntohl(nla_get_u32(uid_attr));
      DEBUG("get uid=%d", uid);
    }

    struct nlattr *prefix_attr =
        nlmsg_find_attr(nh, sizeof(struct genlmsghdr), NFULA_PREFIX);
    if (prefix_attr) {
      nla_strlcpy(prefix, prefix_attr, sizeof(prefix));
      DEBUG("get prefix=%s", prefix);
    }

    struct nlattr *payload =
        nlmsg_find_attr(nh, sizeof(struct genlmsghdr), NFULA_PAYLOAD);
    if (payload) {
      len = nla_len(payload);
      raw = (char *)nla_data(payload);

      if (uid == 0)
        uid = find_uid_by_ip_packet(raw, len);

      dump(raw, len, uid);
    }

    sendNfMessage(uid, prefix, raw, len);
  }

  void sendNfMessage(uid_t uid, const char *prefix, const char *raw, int len) {

    Buffer<unsigned char, 1024 * 4> buf;

    MessageStream ms(buf.Data());
    ms.put((char)0);
    ms.put((char)TYPE_NFLOG);
    ms.put(KEY_UID, uid);
    ms.put(KEY_PREFIX, prefix);
    if (len > (int)buf.Size() - 4 - (int)ms.size()) {
      len = (int)buf.Size() - 4 - (int)ms.size();
    }
    ms.put(KEY_PAYLOAD, raw, len);

    logbuff("sendNfMessage:", (const char *)buf.Data(), ms.size());

    sendHookMsg(buf.Data(), ms.size());
  }

  bool setupSocket(int groups, int family, bool confg_as_group) {

    struct sockaddr_nl nladdr;
    int sz = GROUP_SIZE;

    int on = 1;

    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    nladdr.nl_pid = getpid() | 123; // Imported diffrent from NetlinkManager
    nladdr.nl_groups = confg_as_group ? 0 : (1 << (groups & 31));

    int sock;

    if ((sock = socket(PF_NETLINK, SOCK_DGRAM, family)) < 0) {
      ALOGE("Unable to create netlink socket :%s", strerror(errno));
      return false;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz)) < 0) {
      ALOGE("Unable to set uevent socket SO_RCVBUFFORCE option:%s",
            strerror(errno));
      close(sock);
      return false;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on)) < 0) {
      ALOGE("Unable to set uevent socket SO_PASSCRED option:%s",
            strerror(errno));
      close(sock);
      return false;
    }

    if (bind(sock, (struct sockaddr *)&nladdr, sizeof(nladdr)) < 0) {
      ALOGE("Unable to bind netlink socket:%s", strerror(errno));
      close(sock);
      return false;
    }

    // conifg
    if (confg_as_group) {
      /*if (nflog_bind_group(sock, groups) < 0) {
          ALOGE("nflog bind group %d failed", groups);
          return false;
      }*/
      if (android_nflog_send_config_cmd(sock, groups, NFULNL_CFG_CMD_PF_UNBIND,
                                        AF_INET) < 0) {
        ALOGE("Failed NFULNL_CFG_CMD_PF_UNBIND (sock:%d) %s", sock,
              strerror(errno));
        return -1;
      }
      if (android_nflog_send_config_cmd(sock, groups, NFULNL_CFG_CMD_PF_BIND,
                                        AF_INET) < 0) {
        ALOGE("Failed NFULNL_CFG_CMD_PF_BIND (sock:%d) %s", sock,
              strerror(errno));
        return -1;
      }
      if (android_nflog_send_config_cmd(sock, groups, NFULNL_CFG_CMD_BIND,
                                        AF_UNSPEC) < 0) {
        ALOGE("Failed NFULNL_CFG_CMD_BIND (sock%d) %s", sock, strerror(errno));
        return -1;
      }
    }

    mInner = new InnerListener(this, sock);
    mNFLogFd = sock;

    if (mInner->startListener()) {
      ALOGE("Unable to start UrlHookListener: %s", strerror(errno));
      return false;
    }

    DEBUG("Start UrlHookListener with %s group=%d OK",
          confg_as_group ? "NFLog" : "ULog", groups);
    return true;
  }

public:
  bool start() {
    /*if (use_ulog) {
        return setupSocket(group, NETLINK_NFLOG, false);
    } else { //use nflog */
    return setupSocket(mGroup, NETLINK_NETFILTER, true);
    //}
  }

  bool stop() {
    if (mInner) {
      if (mInner->stopListener()) {
        ALOGE("Unable to stop the NFLogListener");
      }
      delete mInner;
      close(mNFLogFd);
      mInner = NULL;
      mNFLogFd = 0;
      DEBUG("Stop the NFLog Listener");
    }
    return true;
  }
};
#endif

#ifdef ENABLE_NFQUEUE
UrlHookListener *CreateNFQueueListener(char **arts, int count);
#endif

//////////////////////////////////////////////////////////
UrlHookListener *UrlHookListener::createFromArgs(char **args, int count,
                                                 UrlHookListenerCallback *cb) {
  UrlHookListener *listener = NULL;
  ArgCfg argcfg(args, count);
  const char *use = argcfg.getString("--use");

#ifdef ENABLE_NFQUEUE
  if (use && strcmp(use, "nfqueue") == 0) {
    listener = CreateNFQueueListener(args, count);
  } else
#endif
#ifdef ENABLE_NFLOG
      if (use == NULL || strcmp(use, "nflog") == 0) {
    listener = new NFLogListener(args, count);
  }
#endif
  if (listener == NULL) {
    ALOGE("Cannot create UrlHookListener by use:%s", use);
    return NULL;
  }

  listener->mCallback = cb;
  return listener;
}
