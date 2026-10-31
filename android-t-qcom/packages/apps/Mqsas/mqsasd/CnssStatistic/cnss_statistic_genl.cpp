#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

#include <netlink/genl/genl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/mngt.h>
#include <netlink/netlink.h>
#include <cutils/log.h>
#include "cnss_statistic_genl_msg.h"
#include "cnss_statistic_genl.h"
#include "CnssStatistic.h"
#include <sys/wait.h>

using namespace std;
using namespace android;

static struct nl_sock *nl_sock;
static int family_id;
char uidbuf[2048];

int getuid(char *fname, string dstport) {
    int lport, rport, uid = -1;
    if (dstport.c_str() == NULL || fname == NULL)
        return 0;
    //char *buf[12], *s, *label = strrchr(fname, '/')+1;
    string ss_state = "UNKNOWN";
    string state_label[] = {"", "ESTABLISHED", "SYN_SENT", "SYN_RECV", "FIN_WAIT1",
                            "FIN_WAIT2", "TIME_WAIT", "CLOSE", "CLOSE_WAIT",
                            "LAST_ACK", "LISTEN", "CLOSING", "UNKNOWN"};

    FILE *fp = fopen(fname, "r");
    if (!fp) {
        ALOGE("show_ip failed open, %s", fname);
        return 0;
    }

    if (!fgets(uidbuf, sizeof(uidbuf), fp)) {
        ALOGE("failed to get uidbuf, %s", fname);
        fclose(fp);
        return 0; //skip header.
    }

    while (fgets(uidbuf, sizeof(uidbuf), fp)) {

        if (strstr(uidbuf, dstport.c_str()) == NULL) {
            continue;
        }
        union {
            struct {
                unsigned u;
                unsigned char b[4];
            } i4;
            struct {
                struct {
                    unsigned a, b, c, d;
                } u;
                unsigned char b[16];
            } i6;
        } laddr, raddr;
        unsigned state, txq, rxq, num, nitems;
        unsigned long inode;

        // Try ipv6, then try ipv4
        nitems = sscanf(uidbuf,
                        " %d: %8x%8x%8x%8x:%x %8x%8x%8x%8x:%x %x %x:%x %*X:%*X %*X %d %*d %ld",
                        &num, &laddr.i6.u.a, &laddr.i6.u.b, &laddr.i6.u.c,
                        &laddr.i6.u.d, &lport, &raddr.i6.u.a, &raddr.i6.u.b,
                        &raddr.i6.u.c, &raddr.i6.u.d, &rport, &state, &txq, &rxq,
                        &uid, &inode);

        if (nitems != 16) {
            nitems = sscanf(uidbuf,
                            " %d: %x:%x %x:%x %x %x:%x %*X:%*X %*X %d %*d %ld",
                            &num, &laddr.i4.u, &lport, &raddr.i4.u, &rport, &state, &txq,
                            &rxq, &uid, &inode);
            if (nitems != 10)
                continue;
            nitems = AF_INET;
        }
        else
            nitems = AF_INET6;
    }
    fclose(fp);
    return uid;
}

static int cnss_statistic_genl_recv_msg(struct nl_msg *nl_msg, void *data) {
    struct nlmsghdr *nlh = (nlmsghdr *)nlmsg_hdr(nl_msg);
    struct genlmsghdr *gnlh = (genlmsghdr *)nlmsg_data(nlh);
    struct nlattr *attrs[CNSS_STATISTIC_GENL_ATTR_MAX + 1];
    int ret = 0;
    int uid = -1;
    unsigned int proto_subtype = 0;
    unsigned int src_port = 0;
    unsigned int dst_port = 0;

    (void)data;
    if (gnlh->cmd != CNSS_STATISTIC_GENL_EVENT_WOW_WAKEUP)
        return NL_SKIP;

    ret = genlmsg_parse(nlh, 0, attrs, CNSS_STATISTIC_GENL_ATTR_MAX, NULL);
    if (ret < 0) {
        ALOGE("RX NLMSG: Parse fail %d", ret);
        return 0;
    }

    proto_subtype = nla_get_u16(attrs[CNSS_STATISTIC_GENL_ATTR_PROTO_SUBTYPE]);
    src_port = nla_get_u16(attrs[CNSS_STATISTIC_GENL_ATTR_SRC_PORT]);
    dst_port = nla_get_u16(attrs[CNSS_STATISTIC_GENL_ATTR_DST_PORT]);

    if (dst_port == 0)
        return 0;
    string buf;
    sprintf((char *)buf.data(), "%X", dst_port);
    string tcp6 = "/proc/net/tcp6";
    string tcp = "/proc/net/tcp";
    string udp6 = "/proc/net/udp6";
    string udp = "/proc/net/udp";

    uid = getuid((char *)tcp6.data(), buf);
    if (uid >= 0) goto found;
    uid = getuid((char *)tcp.data(), buf);
    if (uid >= 0) goto found;
    uid = getuid((char *)udp6.data(), buf);
    if (uid >= 0) goto found;
    uid = getuid((char *)udp.data(), buf);
found:
    ALOGI("Wow from suspend: uid %d, subtype %d, src %d, dst %d",
           uid, proto_subtype, src_port, dst_port);
    CnssStatistic::onWakeup(uid, proto_subtype, src_port, dst_port);
    return 0;
}

int cnss_statistic_genl_recvmsgs(void) {
    int ret = 0;
    if (!nl_sock)
        return -EINVAL;
    ret = nl_recvmsgs_default(nl_sock);
    if (ret < 0)
        ALOGE("NL msg recv error %d", ret);
    return ret;
}

int cnss_statistic_genl_init(void) {
    struct nl_sock *sock;
    int ret = 0;
    int mcgrp_id = 1;

    sock = nl_socket_alloc();
    if (!sock) {
        ALOGE("NL socket alloc fail");
        return -EINVAL;
    }

    ret = genl_connect(sock);
    if (ret < 0) {
        ALOGE("Cnss:GENL socket connect fail:%d",ret);
        goto free_socket;
    }

    ret = nl_socket_set_buffer_size(sock, 8192, 0);
    if (ret < 0) {
            ALOGE("Could not set NL RX buffer size %d", ret);
    }

    family_id = genl_ctrl_resolve(sock, CNSS_STATISTIC_GENL_NAME);
    if (family_id < 0) {
        ret = family_id;
        ALOGE("Couldn't resolve family id");
        goto close_socket;
    }

    mcgrp_id = genl_ctrl_resolve_grp(sock, CNSS_STATISTIC_GENL_NAME,
                                     CNSS_STATISTIC_MULTICAST_GROUP_EVENT);

    nl_socket_disable_seq_check(sock);
    ret = nl_socket_modify_cb(sock, NL_CB_MSG_IN,
                              NL_CB_CUSTOM, cnss_statistic_genl_recv_msg, NULL);
    if (ret < 0) {
        ALOGE("Couldn't modify NL cb, ret %d", ret);
        goto close_socket;
    }

    ret = nl_socket_add_membership(sock, mcgrp_id);
    if (ret < 0) {
        ALOGE("Couldn't add membership to group %d, ret %d", mcgrp_id, ret);
        goto close_socket;
    }
    nl_sock = sock;

    if (NULL == nl_sock) {
        ALOGE("mqsasd :its not created\n");
    }
    return ret;

close_socket:
    nl_close(sock);
        ALOGE("mqsasd : close_socket!\n");
free_socket:
    nl_socket_free(sock);
    return ret;
}

int cnss_statistic_send_enable_command(void) {
    struct nl_msg *nlmsg;
    void *msg_head = NULL;

    ALOGD("cnss_statistic_send_control_command：enable");
    nlmsg = nlmsg_alloc();
    if (NULL == nlmsg) {
        ALOGE("nlmsg alloc failed\n");
        return -ENOMEM;
    }
    msg_head = genlmsg_put(nlmsg, 0, 0, family_id, 0, 0, CNSS_STATISTIC_GENL_CMD_ENABLE, 0);
    if (NULL == msg_head) {
        ALOGE("genlmsg put failed\n");
        nlmsg_free(nlmsg);
        return -EINVAL;
    }

    if (NULL == nl_sock) {
        ALOGE("nl command socket is not created\n");
        nlmsg_free(nlmsg);
        return -EINVAL;
    }
    int err = nl_send_auto_complete(nl_sock, nlmsg);
    if (err < 0) {
        ALOGE("nl auto complete err: %d\n", err);
        nlmsg_free(nlmsg);
        return -EINVAL;
    }
    nlmsg_free(nlmsg);
    return 0;
}

int cnss_statistic_send_disable_command(void) {
    struct nl_msg *nlmsg;
    void *msg_head = NULL;

    ALOGD("cnss_statistic_send_control_command:disable");
    nlmsg = nlmsg_alloc();
    if (NULL == nlmsg) {
        ALOGE("nlmsg alloc failed\n");
        return -ENOMEM;
    }
    msg_head = genlmsg_put(nlmsg, 0, 0, family_id, 0, 0, CNSS_STATISTIC_GENL_CMD_DISABLE, 0);
    if (NULL == msg_head) {
        ALOGE("genlmsg put failed\n");
        nlmsg_free(nlmsg);
        return -EINVAL;
    }

    if (NULL == nl_sock) {
        ALOGE("nl command socket is not created\n");
        nlmsg_free(nlmsg);
        return -EINVAL;
    }
    int err = nl_send_auto_complete(nl_sock, nlmsg);
    if (err < 0) {
        ALOGE("nl auto complete err: %d\n", err);
        nlmsg_free(nlmsg);
        return -EINVAL;
    }
    nlmsg_free(nlmsg);
    return 0;
}

int cnss_statistic_genl_get_fd(void) {
    if (!nl_sock)
        return -1;
    else
        return nl_socket_get_fd(nl_sock);
}

void cnss_statistic_genl_exit(void) {
    if (nl_sock) {
        nl_close(nl_sock);
        nl_socket_free(nl_sock);
        nl_sock = NULL;
    }
}

