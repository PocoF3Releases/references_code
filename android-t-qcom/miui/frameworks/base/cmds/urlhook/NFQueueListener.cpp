//////////////////////////////////////////////////////////////////////////////
// NFQueue
#ifdef ENABLE_NFQUEUE

#include "UrlHookListener.h"
#include "UrlHookLog.h"
#include "UrlHookMsg.h"
#include <errno.h>
#include <linux/netfilter.h> /* for NF_ACCEPT */
#include <linux/netlink.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * linux kernal support NFQueue uid since 2013, see the changes:
 * https://www.spinics.net/lists/netdev/msg262412.html
 * https://www.spinics.net/lists/netdev/msg262410.html
 *
 * android-m, android-n support uid too, but android-l & android-kk didn't
 * see source kernel/net/netfilter/nfnetlink_queue_core.c
 * see function nfqnl_build_packet_message when call nfqnl_put_sk_uidgid
 */

#define PACKED __attribute__((packed))

#ifndef aligned_u64
#define aligned_u64 unsigned long long __attribute__((aligned(8)))
#endif

enum {
  BUFF_SIZE = 8 * 1024,
  NFNL_MAX_SUBSYS = 16,
  NFNL_SUBSYS_QUEUE = 3,
};

enum {
  NFQNL_MSG_PACKET,
  NFQNL_MSG_VERDICT,
  NFQNL_MSG_CONFIG,

  NFQNL_MSG_MAX
};

enum {
  NFQA_UNSPEC,
  NFQA_PACKET_HDR,
  NFQA_VERDICT_HDR,
  NFQA_MARK,
  NFQA_TIMESTAMP,
  NFQA_IFINDEX_INDEV,
  NFQA_IFINDEX_OUTDEV,
  NFQA_IFINDEX_PHYSINDEV,
  NFQA_IFINDEX_PHYSOUTDEV,
  NFQA_HWADDR,
  NFQA_PAYLOAD,
  NFQA_MAX_
};

const uint32_t NFQA_MAX = NFQA_MAX_;

struct NFQAttr {
  uint16_t len;
  uint16_t type;
};

struct NFQCommonMsg {
  uint8_t family;
  uint8_t version;
  uint16_t res_id;
};

struct NFQMsgPacketHdr {
  uint32_t packet_id;
  uint16_t hw_protocol;
  uint8_t hook;
} PACKED;

struct NFQMsgPacketHW {
  uint16_t hw_addrlen;
  uint16_t _pad;
  uint8_t hw_addr[8];
} PACKED;

struct NFQMsgPacketTimestamp {
  aligned_u64 sec;
  aligned_u64 usec;
} PACKED;

struct NFQMsgVerdictHdr {
  uint32_t verdict;
  uint32_t id;
} PACKED;

struct NFQMsgConfigCmd {
  uint8_t command;
  uint8_t _pad;
  uint16_t pf;
} PACKED;

struct NFQMsgConfigParams {
  uint32_t copy_range;
  uint8_t copy_mode;
} PACKED;

enum {
  NFQA_CFG_UNSPEC,
  NFQA_CFG_CMD,
  NFQA_CFG_PARAMS,
  NFQA_CFG_QUEUE_MAXLEN,
  NFQA_CFG_MAX_
};

enum {
  NFQNL_COPY_NONE,
  NFQNL_COPY_META,
  NFQNL_COPY_PACKET,
};

// Not use
// const uint32_t NFQA_CFG_MAX = NFQA_CFG_MAX_ - 1;

static inline uint8_t nf_msg_type(uint16_t type) { return type & 0x00ff; }
static inline uint8_t nf_subsys_id(uint16_t type) {
  return (type & 0xff00) >> 8;
}

static inline bool nf_msg_is_error(struct nlmsghdr *h) {
  if (h->nlmsg_type == NLMSG_ERROR ||
      (h->nlmsg_type == NLMSG_DONE && h->nlmsg_type == NLM_F_MULTI)) {
    if (h->nlmsg_len < NLMSG_ALIGN(sizeof(struct nlmsgerr))) {
      errno = EBADMSG;
      return true;
    }
    errno = -(*((int *)NLMSG_DATA(h)));
    return true;
  }
  return false;
}

template <typename T> static inline T *nf_msg_tail(struct nlmsghdr *n) {
  return (T *)(((char *)n) + NLMSG_ALIGN(n->nlmsg_len));
}

static inline NFQCommonMsg *nf_common_msg(struct nlmsghdr *nlh) {
  return (NFQCommonMsg *)NLMSG_DATA(nlh);
}

template <typename T> static inline uint32_t nf_msg_align() {
  return NLMSG_ALIGN(sizeof(T));
}

template <typename T> static inline uint32_t nf_msg_len() {
  return NLMSG_LENGTH(nf_msg_align<T>());
}

static inline NFQAttr *nf_attr(struct nlmsghdr *nlh) {
  return (NFQAttr *)(((char *)nf_common_msg(nlh)) +
                     nf_msg_align<NFQCommonMsg>());
}

static inline bool nf_attr_ok(NFQAttr *attr, uint32_t len) {
  return ((int)len > 0 && attr->len > sizeof(NFQAttr));
}
static inline uint16_t nf_attr_type(NFQAttr *attr) {
  return attr->type & 0x7fff;
}

static inline uint32_t nf_attr_align(uint32_t len) { return ((len + 3) & ~3); }

static inline uint32_t nf_attr_len(uint32_t len) {
  return nf_attr_align(sizeof(NFQAttr)) + nf_attr_align(len);
}

static inline NFQAttr *nf_attr_next(NFQAttr *attr, uint32_t &attr_len) {
  if (attr_len > nf_attr_align(attr->len)) {
    attr_len -= nf_attr_align(attr->len);
  } else {
    attr_len = 0;
  }
  return (NFQAttr *)(((char *)attr) + nf_attr_align(attr->len));
}
template <typename T> static inline T *nf_attr_data(NFQAttr *attr) {
  return (T *)(((char *)attr) + nf_attr_align(sizeof(NFQAttr)));
}

const uint32_t NFNL_HEADER_LEN =
    NLMSG_ALIGN(sizeof(struct nlmsghdr)) + NLMSG_ALIGN(sizeof(NFQCommonMsg));

static void dump_packet_attr(NFQAttr *pattr) {
  switch (pattr->type) {
  case NFQA_UNSPEC:
    ALOGI("packet unspect");
    break;
  case NFQA_PACKET_HDR: {
    NFQMsgPacketHdr *hdr = nf_attr_data<NFQMsgPacketHdr>(pattr);
    ALOGI("packet hdr id=%d", hdr->packet_id);
    ALOGI("packet hdr hw_protocol=%d", hdr->hw_protocol);
    ALOGI("packet hdr daemon=%d", hdr->hook);
    break;
  }
  case NFQA_VERDICT_HDR: {
    NFQMsgVerdictHdr *hdr = nf_attr_data<NFQMsgVerdictHdr>(pattr);
    ALOGI("verdict hdr verdict=%d", hdr->verdict);
    ALOGI("verdict hdr id=%d", hdr->id);
    break;
  }
  case NFQA_MARK: {
    uint32_t *pmark = nf_attr_data<uint32_t>(pattr);
    ALOGI("mark=%d", *pmark);
    break;
  }
  case NFQA_TIMESTAMP:
    ALOGI("timestamp");
    break;
  case NFQA_IFINDEX_INDEV:
    ALOGI("IFINDEX_INDEV");
    break;
  case NFQA_IFINDEX_OUTDEV:
    ALOGI("IFINDEX_OUTDEV");
    break;
  case NFQA_IFINDEX_PHYSINDEV:
    ALOGI("IFINDEX_PHYSINDEV");
    break;
  case NFQA_IFINDEX_PHYSOUTDEV:
    ALOGI("IFINDEX_PHYSOUTDEV");
    break;
  case NFQA_HWADDR: {
    NFQMsgPacketHW *hw = nf_attr_data<NFQMsgPacketHW>(pattr);
    ALOGI("packet hw addrlen=%d", hw->hw_addrlen);
    uint8_t *ad = hw->hw_addr;
    ALOGI("packet hw addrs={%d.%d.%d.%d.%d.%d,%d.%d}", ad[0], ad[1], ad[2],
          ad[3], ad[4], ad[5], ad[6], ad[7]);
    break;
  }
  case NFQA_PAYLOAD: {
    int len = pattr->len - nf_attr_align(sizeof(NFQAttr));
    uint8_t *data = nf_attr_data<uint8_t>(pattr);
    logbuff("payload", (const char *)data, len);
    break;
  }
  }
}

static void dump_config_attr(NFQAttr *pattr) {
  switch (pattr->type) {
  case NFQA_CFG_UNSPEC:
    ALOGI("config unspec");
    break;
  case NFQA_CFG_CMD: {
    NFQMsgConfigCmd *cmd = nf_attr_data<NFQMsgConfigCmd>(pattr);
    ALOGI("config command: %d", cmd->command);
    ALOGI("config command pf:%d", cmd->pf);
    break;
  }
  case NFQA_CFG_PARAMS: {
    NFQMsgConfigParams *params = nf_attr_data<NFQMsgConfigParams>(pattr);
    ALOGI("config param: copy_range=%d", params->copy_range);
    ALOGI("config param: copy_mode=%d", params->copy_mode);
    break;
  }
  case NFQA_CFG_QUEUE_MAXLEN:
    ALOGI("config queue maxlen");
    break;
  }
}

static void nf_dump_msg(const char *prefix, struct nlmsghdr *h) {
  uint8_t type = nf_msg_type(h->nlmsg_type);
  uint8_t subsys_id = nf_subsys_id(h->nlmsg_type);
  ALOGI("dump nfqueue %s (%p)===>>>>>>>>>>>>>>>>>======== ", prefix, h);
  ALOGI("nlmsg_len=%d", h->nlmsg_len);
  ALOGI("nlmsg_type=%d", type);
  ALOGI("subsys_id=%d", subsys_id);
  ALOGI("nlmsg_flags=0x%0x", h->nlmsg_flags);
  ALOGI("nlmsg_seq=%d", h->nlmsg_seq);
  ALOGI("nlmsg_pid=%d", h->nlmsg_pid);

  if (nf_msg_is_error(h)) {
    return;
  }

  uint32_t len = h->nlmsg_len;

  uint32_t min_len = nf_msg_len<NFQCommonMsg>();

  ALOGI("len=%d, min_len=%d", len, min_len);

  if (len <= NLMSG_ALIGN(min_len)) {
    return;
  }

  NFQCommonMsg *comm_msg = nf_common_msg(h);
  ALOGI("gen_family=%d", comm_msg->family);
  ALOGI("gen_version=%d", comm_msg->version);
  ALOGI("gen_res_id=%d", comm_msg->res_id);

  len -= NLMSG_ALIGN(min_len);

  NFQAttr *pattr = nf_attr(h);
  while (nf_attr_ok(pattr, len)) {
    ALOGI("attr (%p) type=%d, len=%d", pattr, pattr->type, pattr->len);
    switch (type) {
    case NFQNL_MSG_PACKET:
      dump_packet_attr(pattr);
      break;
    case NFQNL_MSG_VERDICT:
      if (pattr->type == NFQA_VERDICT_HDR) {
        dump_packet_attr(pattr);
      }
      break;
    case NFQNL_MSG_CONFIG:
      dump_config_attr(pattr);
      break;
    }

    pattr = nf_attr_next(pattr, len);
    ALOGI("next attr :%p, len=%d", pattr, len);
  }

  ALOGI("end %s ===<<<<<<<<<<<<<<<<<<<<<<======== ", prefix);
}

struct NFQPackageData {
  char buf[BUFF_SIZE];
  int len;
  NFQCommonMsg *comMsg;
  NFQAttr *attrs[NFQA_MAX];

  bool parse() {
    if (len <= 0) {
      return false;
    }

    char *pbuf = buf;
    while (len >= (int)NLMSG_SPACE(0)) {
      uint32_t rlen;
      struct nlmsghdr *nlh = (struct nlmsghdr *)pbuf;

      if (nlh->nlmsg_len < sizeof(struct nlmsghdr) ||
          len < (int)(nlh->nlmsg_len)) {
        return false;
      }

      rlen = NLMSG_ALIGN(nlh->nlmsg_len);

      if (rlen > (uint32_t)len) {
        rlen = (uint32_t)len;
      }

      if (!handleMsg(nlh, rlen)) {
        return false;
      }

      len -= rlen;
      pbuf += rlen;
    }
    return true;
  }

  bool handleMsg(struct nlmsghdr *nlh, uint32_t rlen) {
    uint8_t type = nf_msg_type(nlh->nlmsg_type);
    uint8_t subsys_id = nf_subsys_id(nlh->nlmsg_type);

    if (subsys_id > NFNL_MAX_SUBSYS) {
      return false;
    }

    if (NFNL_SUBSYS_QUEUE == subsys_id) {
      if (NFQNL_MSG_PACKET == type) {
        return handlePacket(nlh, rlen);
      }
    }
    return false;
  }

  bool handlePacket(struct nlmsghdr *nlh,
                    uint32_t rlen __attribute__((unused))) {

    if (nlh->nlmsg_len < nf_msg_len<NFQCommonMsg>()) {
      return false;
    }

    comMsg = nf_common_msg(nlh);
    memset(attrs, 0, sizeof(attrs));

    uint32_t min_len = NLMSG_SPACE(sizeof(NFQCommonMsg));

    if (nlh->nlmsg_len < min_len) {
      return false;
    }

    NFQAttr *pattr = nf_attr(nlh);
    uint32_t attrlen = nlh->nlmsg_len - NLMSG_ALIGN(min_len);

    while (nf_attr_ok(pattr, attrlen)) {
      uint32_t flavor = nf_attr_type(pattr);
      if (flavor) {
        if (flavor <= NFQA_MAX) {
          attrs[flavor - 1] = pattr;
        }
      }

      pattr = nf_attr_next(pattr, attrlen);
    }

    return true;
  }

  template <typename T> T *getAttrData(int attr_id) {
    return (attrs[attr_id - 1]) ? nf_attr_data<T>(attrs[attr_id - 1]) : NULL;
  }

  NFQMsgPacketHdr *getPacketHdr() {
    return getAttrData<NFQMsgPacketHdr>(NFQA_PACKET_HDR);
  }

  uint32_t getMark() {
    uint32_t *pmark = getAttrData<uint32_t>(NFQA_MARK);
    return ntohl(pmark ? *pmark : 0);
  }

  NFQMsgPacketHW *getPacketHW() {
    return getAttrData<NFQMsgPacketHW>(NFQA_HWADDR);
  }

  uint8_t *getPayload(int &len) {
    uint8_t *data = getAttrData<uint8_t>(NFQA_PAYLOAD);
    len = -1;
    if (data) {
      len = attrs[NFQA_PAYLOAD - 1]->len - nf_attr_align(sizeof(NFQAttr));
    }
    return data;
  }

  uint32_t getPackageId() {
    NFQMsgPacketHdr *hdr = getPacketHdr();
    return ntohl(hdr ? hdr->packet_id : 0);
  }
};

class NFQueue {
  int mFd;
  int mQueueNum;
  char *mStrErrInfo;
  struct sockaddr_nl mLocal;
  struct sockaddr_nl mPeer;

  enum { SUBSYS_ID = NFNL_SUBSYS_QUEUE };

  enum {
    CFG_CMD_NONE,
    CFG_CMD_BIND,
    CFG_CMD_UNBIND,
    CFG_CMD_PF_BIND,
    CFG_CMD_PF_UNBIND
  };

  void setError(const char *errMsg) {
    if (mStrErrInfo) {
      free(mStrErrInfo);
      mStrErrInfo = NULL;
    }

    if (errMsg) {
      mStrErrInfo = strdup(errMsg);
    }
  }

  bool isOpened() { return mFd > 0; }

  bool rebind() {
    if (!isOpened()) {
      setError("open the nfqueue before bind it");
      return false;
    }

    mLocal.nl_groups = 0;
    int err = bind(mFd, (struct sockaddr *)&mLocal, sizeof(mLocal));

    if (err == -1) {
      setError("bind socket failed");
      return false;
    }

    return true;
  }

  bool fillHeader(struct nlmsghdr *nlhdr, uint32_t len, uint8_t family,
                  uint16_t res_id, uint16_t msg_type, uint16_t msg_flags) {
    NFQCommonMsg *com_msg = (NFQCommonMsg *)(((char *)nlhdr) + sizeof(*nlhdr));

    memset(nlhdr, 0, sizeof(*nlhdr));
    nlhdr->nlmsg_len = NLMSG_LENGTH(len + sizeof(NFQCommonMsg));
    nlhdr->nlmsg_type = (SUBSYS_ID << 8) | msg_type;
    nlhdr->nlmsg_flags = msg_flags;
    nlhdr->nlmsg_pid = 0;

    com_msg->family = family;
    com_msg->version = 0;
    com_msg->res_id = htons(res_id);

    return true;
  }

  bool addAttr(struct nlmsghdr *n, uint32_t maxlen, int type, const void *data,
               uint32_t alen) {
    int len = nf_attr_len(alen);
    NFQAttr *nfa;

    if (NLMSG_ALIGN(n->nlmsg_len) + len > maxlen) {
      setError("package is not enough");
      return false;
    }

    nfa = nf_msg_tail<NFQAttr>(n);
    nfa->type = type;
    nfa->len = len;
    memcpy(nf_attr_data<char>(nfa), data, alen);

    n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + len;

    return true;
  }

  bool sendConfigMessage(uint8_t command, uint16_t qnum, uint16_t pf) {
    char buf[NFNL_HEADER_LEN + nf_attr_len(sizeof(NFQMsgConfigCmd))];
    struct nlmsghdr *nlhdr = (struct nlmsghdr *)buf;
    NFQMsgConfigCmd cmd;

    fillHeader(nlhdr, 0, AF_UNSPEC, qnum, NFQNL_MSG_CONFIG,
               NLM_F_REQUEST | NLM_F_ACK);

    cmd.command = command;
    cmd.pf = htons(pf);

    addAttr(nlhdr, sizeof(buf), NFQA_CFG_CMD, &cmd, sizeof(cmd));

    return query(nlhdr);
  }

  bool sendConfigMode(uint8_t mode, uint16_t qnum, uint32_t range) {
    char buf[NFNL_HEADER_LEN + nf_attr_len(sizeof(NFQMsgConfigParams))];

    struct nlmsghdr *nlhdr = (struct nlmsghdr *)buf;
    NFQMsgConfigParams params;

    fillHeader(nlhdr, 0, AF_UNSPEC, qnum, NFQNL_MSG_CONFIG,
               NLM_F_REQUEST | NLM_F_ACK);

    params.copy_range = htonl(range);
    params.copy_mode = mode;

    addAttr(nlhdr, sizeof(buf), NFQA_CFG_PARAMS, &params, sizeof(params));

    return query(nlhdr);
  }

  int send(struct nlmsghdr *n) {
    if (CAN_DEBUG) {
      nf_dump_msg("send to", n);
    }
    return ::sendto(mFd, n, n->nlmsg_len, 0, (struct sockaddr *)&mPeer,
                    sizeof(mPeer));
  }

  int recv(char *buf, size_t len) {
    socklen_t addrlen;
    int status;
    struct sockaddr_nl peer;

    if (len < sizeof(struct nlmsgerr) || len < sizeof(struct nlmsghdr)) {
      return -1;
    }

    addrlen = sizeof(mPeer);
    status = ::recvfrom(mFd, buf, len, 0, (struct sockaddr *)&peer, &addrlen);
    if (status <= 0) {
      setError("recvfrom data failed!");
      return status;
    }

    if (addrlen != sizeof(peer)) {
      setError("recvfrom data failed!");
      return -1;
    }

    if (peer.nl_pid != 0) {
      setError("recvfrom data is not from kernal");
      return -1;
    }

    return status;
  }

  bool query(struct nlmsghdr *nlh) {
    if (send(nlh) == -1) {
      setError("send data failed");
      return false;
    }

    return catchResult();
  }

  bool catchResult() {
    bool ret = false;
    while (1) {
      char buf[BUFF_SIZE];
      int rlen = recv(buf, sizeof(buf));
      if (rlen <= 0) {
        if (errno == EINTR) {
          continue;
        }
        break;
      }

      struct nlmsghdr *nlh = (struct nlmsghdr *)buf;
      if (CAN_DEBUG) {
        nf_dump_msg("catchResult", nlh);
      }

      DEBUG("catchResult recv len=%d", rlen);

      while (rlen > (int)NLMSG_SPACE(0) && NLMSG_OK(nlh, (uint32_t)rlen)) {
        // only get ack
        if (nf_msg_is_error(nlh)) {
          if (errno == 0) { // ACK
            DEBUG("catch ACK");
            ret = true;
          }
          return ret;
        }

        nlh = NLMSG_NEXT(nlh, rlen);
      }
    }
    return false;
  }

  bool setVerdict(uint32_t pack_id, uint32_t verdict) {

    NFQMsgVerdictHdr vh;

    char buf[NFNL_HEADER_LEN + nf_attr_len(sizeof(vh))];

    struct nlmsghdr *nlh = (struct nlmsghdr *)buf;

    vh.verdict = htonl(verdict);
    vh.id = htonl(pack_id);

    fillHeader(nlh, 0, AF_UNSPEC, mQueueNum, NFQNL_MSG_VERDICT, NLM_F_REQUEST);

    addAttr(nlh, sizeof(buf), NFQA_VERDICT_HDR, &vh, sizeof(vh));

    struct iovec iov[1];

    memset(iov, 0, sizeof(iov));

    iov[0].iov_base = buf;
    iov[0].iov_len = nf_msg_tail<char>(nlh) - buf;

    struct msghdr msg;

    msg.msg_name = (struct sockaddr *)&mPeer;
    msg.msg_namelen = sizeof(mPeer);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;

    if (CAN_DEBUG) {
      nf_dump_msg("send verdict", nlh);
    }

    return sendmsg(mFd, &msg, 0) >= 0;
  }

public:
  NFQueue() : mFd(0), mQueueNum(0), mStrErrInfo(NULL) {
    memset(&mLocal, 0, sizeof(mLocal));
    memset(&mPeer, 0, sizeof(mPeer));
  }

  ~NFQueue() {
    setError(NULL);
    close();
  }

  bool open(int num) {
    close();

    mQueueNum = num;

    socklen_t addr_len;
    int err;
    mFd = socket(AF_NETLINK, SOCK_RAW, NETLINK_NETFILTER);

    if (mFd == -1) {
      setError("cannot create the netlink socket");
      return false;
    }

    mLocal.nl_family = AF_NETLINK;
    mPeer.nl_family = AF_NETLINK;

    addr_len = sizeof(mLocal);
    err = getsockname(mFd, (struct sockaddr *)&mLocal, &addr_len);
    if (addr_len != sizeof(mLocal)) {
      setError("cannot get the right sockname");
      return false;
    }

    if (mLocal.nl_family != AF_NETLINK) {
      setError("kernal does not support AF_NETLINK");
      return false;
    }

    if (!rebind()) {
      return false;
    }

    err = getsockname(mFd, (struct sockaddr *)&mLocal, &addr_len);
    if (addr_len != sizeof(mLocal)) {
      setError("cannot get the right sockname after bind");
      return false;
    }

    if (!rebind()) {
      return false;
    }

    if (!sendConfigMessage(CFG_CMD_PF_UNBIND, 0, AF_INET)) {
      setError("Cannot unbind the protcol family");
      return false;
    }

    if (!sendConfigMessage(CFG_CMD_PF_BIND, 0, AF_INET)) {
      setError("Cannot bind the protcol family");
      return false;
    }

    if (!sendConfigMessage(CFG_CMD_BIND, mQueueNum, 0)) {
      setError("Cannot create queue number");
      return false;
    }

    if (!sendConfigMode(NFQNL_COPY_PACKET, mQueueNum, 0xffff)) {
      setError("Cannot config Mode Copy Packet");
      return false;
    }

    return true;
  }

  int getFd() { return mFd; }

  bool readPackage(NFQPackageData &data) {
    data.len = ::recv(mFd, data.buf, sizeof(data.buf), 0);
    if (data.len < 0) {
      setError("Losing packages!");
      return false;
    }

    if (CAN_DEBUG) {
      nf_dump_msg("readPackage", (struct nlmsghdr *)data.buf);
    }

    if (data.parse()) {
      return setVerdict(data.getPackageId(), NF_ACCEPT);
    }
    return false;
  }

  void close() {
    if (mFd <= 0) {
      return;
    }

    if (!sendConfigMessage(CFG_CMD_UNBIND, mQueueNum, 0)) {
      setError("Cannot destroy queue number");
    }

    if (!sendConfigMessage(CFG_CMD_PF_UNBIND, 0, AF_INET)) {
      setError("Cannot unbind the protcol family");
    }

    ::close(mFd);
    mFd = 0;
  }

  const char *getError() { return mStrErrInfo; }
};

class NFQueueListener : public UrlHookListener {
public:
  NFQueueListener(char **args, int count) : mQueueNumber(0), mRunning(false) {
    ArgCfg ac(args, count);
    mQueueNumber = ac.getInt("--queue-num");
  }

  ~NFQueueListener() {}

  bool start() {
    if (!mNFQueue.open(mQueueNumber)) {
      ALOGE("open NFQUEUE failed: %s, %s", strerror(errno),
            mNFQueue.getError());
      return false;
    }
    DEBUG("open NFQUEUE OK fd:%d", mNFQueue.getFd());

    mRunning = true;
    pthread_create(&mThread, NULL, _listen_loop, this);
    DEBUG("Start NFQUEUE thread, with fd=%d", mNFQueue.getFd());

    return true;
  }

  bool stop() {
    if (mRunning) {
      mRunning = false;
      void *pret;
      pthread_join(mThread, &pret);
    }

    mNFQueue.close();

    DEBUG("close NFQUEUE OK");
    return true;
  }

private:
  int mQueueNumber;
  volatile bool mRunning;
  pthread_t mThread;
  NFQueue mNFQueue;

  static void *_listen_loop(void *param) {
    NFQueueListener *thiz = (NFQueueListener *)param;
    thiz->run();
    return NULL;
  }

  void run() {
    DEBUG("NFQueue Start running:%d", mRunning);
    while (mRunning) {
      fd_set readfds;
      struct timeval tv;
      FD_ZERO(&readfds);
      int fd = mNFQueue.getFd();
      FD_SET(fd, &readfds);
      tv.tv_sec = 0;
      tv.tv_usec = 500 * 1000; // 500ms
      NFQPackageData data;
      //DEBUG("NFQueue select fd:%d", fd);
      if (select(fd + 1, &readfds, NULL, NULL, &tv) > 0) {
        DEBUG("NFQueue read data from fd=%d", fd);

        if (mNFQueue.readPackage(data)) {

          DEBUG("NFQueue parseQueneMsg ****************");
          parseQueneMsg(data);
          DEBUG("NFQueue parseQueneMsg ----------------"); 

          continue;
        }

        ALOGE("NFQUEUE recv failed");
        break;
      }
    }
    DEBUG("NFQUEUE thread exits");
  }

  void parseQueneMsg(NFQPackageData &data) {
    NFQMsgPacketHW *hwph;
    u_int32_t mark = 0;
    int ret;
    unsigned char *payload_data;
    uint32_t uid;

    hwph = data.getPacketHW();
    if (hwph && CAN_DEBUG) {
      int i, hlen = ntohs(hwph->hw_addrlen);
      char szbuf[256];
      if (hlen > (int)sizeof(szbuf) / 3)
        hlen = sizeof(szbuf) / 3;
      int idx = 0;
      for (i = 0; i < hlen - 1; i++) {
        idx += sprintf(szbuf + idx, "%02x:", hwph->hw_addr[i]);
      }
      sprintf(szbuf + idx, "%02x", hwph->hw_addr[hlen - 1]);
      DEBUG("NFQUEUE hw_src_addr=%s", szbuf);
    }

    mark = data.getMark();
    if (mark) {
      DEBUG("NFQUEUE mark=%d", mark);
    }

    payload_data = data.getPayload(ret);

    if (ret >= 0) {
      uid = find_uid_by_ip_packet(payload_data, ret);
      //sendData(uid, mark, (const char *)payload_data, ret);
    }
    sendNotify();
  }

  void sendNotify() {
    unsigned char buf[512];
    // TODO(zhangzhao) delete domain
    char domain[] = "v2.thefatherofsalmon.com";
    MessageStream ms(buf);

    ms.put((char)0);
    ms.put((char)TYPE_NOTIFY);
    ms.put(KEY_HOSTNAME, domain, strlen(domain));

    DEBUG("NFQUEUE sendNotify begin");
    sendHookMsg(buf, ms.size());
    DEBUG("NFQUEUE sendNotify end");
  }

  void sendData(uint32_t uid, u_int32_t mark, const char *data, size_t ret) {
    unsigned char buf[1024 * 4];

    MessageStream ms(buf);

    ms.put((char)0);
    ms.put((char)TYPE_NFQUEUE);
    ms.put(KEY_UID, uid);
    ms.put(KEY_MARK, mark);

    int len = sizeof(buf) - ms.size();
    if (len > (int)ret)
      len = ret;
    ms.put(KEY_PAYLOAD, data, len);

    logbuff("NFQUEUE Payload:", (const char *)data, len);
    sendHookMsg(buf, ms.size());
    logbuff("NFQUEUE send:", (const char *)buf, ms.size());
  }
};

UrlHookListener *CreateNFQueueListener(char **args, int count) {
  return new NFQueueListener(args, count);
}

#endif
