#ifndef LOG_TAG
#define LOG_TAG "JNI_GREEZE"
#endif

#include "jni.h"
#include "core_jni_helpers.h"
#include <stdio.h>
#include <string.h>
#include <linux/socket.h>
#include <linux/netlink.h>
#include <netinet/in.h>

#include "utils/Log.h"

namespace android {

#define MILLET_KERNEL_ID 0x12341234
#define MILLET_USER_ID   0xabcddcba
#define MILLET_ERR  -1
#define MILLET_OK  0
#define EXT_LEN			6
enum PKG_CMD {
    CMD_NOP,
    ADD_UID,
    DEL_UID,
    CLEAR_ALL_UID,
    CMD_END,
};

enum SIG_STAT {
    KILLED_BY_PRO,
    KILLED_BY_LMK,
};

enum BINDER_STAT {
    BINDER_IN_IDLE,
    BINDER_IN_BUSY,
    BINDER_THREAD_IN_BUSY,
    BINDER_PROC_IN_BUSY,
    BINDER_IN_TRANSACTION,
    BINDER_ST_NUM,
};

enum MILLET_TYPE {
    O_TYPE,
    SIG_TYPE,
    BINDER_TYPE,
    BINDER_ST_TYPE,
    MEM_TYPE,
    PKG_TYPE,
    HANDSHK_TYPE,
    MILLET_TYPES_NUM,
};

enum MSG_TYPE {
    O_MSG,
    LOOPBACK_MSG,
    MSG_TO_USER,
    MSG_TO_KERN,
    MSG_END,
};

typedef struct millet_sock {
    int sock;
    int in_use;
} millet_sock;

struct time_stamp {
    unsigned long long sec;
    long nsec;
};

struct millet_userconf{
	enum MILLET_TYPE owner;
	enum MSG_TYPE msg_type;
	unsigned long src_port;
	unsigned long dst_port;
	unsigned long pri[EXT_LEN];
	union {
		union {
			unsigned long data;
		} comm;

		union {
			struct pkg_user {
				enum PKG_CMD cmd;
				int uid;
			} pkg;

			struct binder_st_user {
				int uid;
			} binder_st;
		} u_priv;
	} mod;


};
// warning : should sync with
// 1. kernel->millet.h
// 2. monitor->millet.h

static millet_sock m_sock;

static int32_t create_bind_socket() {
    struct sockaddr_nl src_addr;
    //bind socket
    int32_t fd = socket(AF_NETLINK, SOCK_RAW, 29);
    if (fd == -1) {
        ALOGE("create socket error: %s", strerror(errno));
        return -1;
    }
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();
    src_addr.nl_groups = 0;
    int ret = 0;
    ret = bind(fd, (struct sockaddr *) &src_addr, sizeof(src_addr));
    if (ret < 0) {
        ALOGE("bind socket failed: %s", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

static void init_msock() {
    ALOGE("jni init static m_sock");
    m_sock.sock = create_bind_socket();
    m_sock.in_use = 1;
}

static void destroy_socket() {
    if (m_sock.sock != -1) {
        m_sock.in_use = 0;
        close(m_sock.sock);
    }
}

static int send_msg_to_kernel(int32_t fd, enum MILLET_TYPE type, struct millet_userconf *data) {
    int ret = MILLET_OK;
    struct sockaddr_nl dst_addr;
    struct nlmsghdr *nlh = NULL;
    struct iovec iov;
    struct msghdr msg;
    struct millet_userconf *millet_msg;
    int msglen = sizeof(struct millet_userconf);

    if (!data) {
        ALOGE("send NULL msg");
        return MILLET_ERR;
    }

    nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(msglen));
    if (!nlh) {
        ALOGE("MILLET_USER: malloc millet msg failed!\n");
        return MILLET_ERR;
    }

    nlh->nlmsg_len = NLMSG_SPACE(msglen);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;
    millet_msg = (struct millet_userconf *) NLMSG_DATA(nlh);
    memcpy(millet_msg, data, sizeof(struct millet_userconf));

    millet_msg->msg_type = data->msg_type;
    millet_msg->owner = type;
    millet_msg->src_port = MILLET_USER_ID;
    millet_msg->dst_port = MILLET_KERNEL_ID;

    iov.iov_base = (void *) nlh;
    iov.iov_len = NLMSG_SPACE(msglen);

    memset(&dst_addr, 0, sizeof(dst_addr));
    dst_addr.nl_family = AF_NETLINK;
    dst_addr.nl_pid = 0;
    dst_addr.nl_groups = 0;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *) &dst_addr;
    msg.msg_namelen = sizeof(dst_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ret = sendmsg(fd, &msg, 0);
    if (ret == MILLET_ERR) {
        ALOGE("MILLET_USER: sendmsg failed %s\n",
              strerror(errno));
        destroy_socket();
        free(nlh);
        return errno;
    }

    free(nlh);
    return MILLET_OK;
}

static void com_miui_server_greeze_GreezeManagerService_AddConcernedUid
        (JNIEnv *env, jclass, jint uid) {

    if (m_sock.sock == -1 || m_sock.in_use == 0)
        init_msock();

    if (m_sock.sock == -1) {
        ALOGE("create & bind socket fail");
        return;
    }

    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = PKG_TYPE;
    data.mod.u_priv.pkg.cmd = ADD_UID;
    data.mod.u_priv.pkg.uid = uid;
    if (send_msg_to_kernel(m_sock.sock, PKG_TYPE, &data) != MILLET_OK) {
        ALOGE("send msg error");
    }
}

static void com_miui_server_greeze_GreezeManagerService_DelConcernedUid(JNIEnv *env, jclass, jint uid) {

    if (m_sock.sock == -1 || m_sock.in_use == 0)
        init_msock();

    if (m_sock.sock == -1) {
        ALOGE("create & bind socket fail");
        return;
    }

    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = PKG_TYPE;
    data.mod.u_priv.pkg.cmd = DEL_UID;
    data.mod.u_priv.pkg.uid = uid;
    if (send_msg_to_kernel(m_sock.sock, PKG_TYPE, &data) != MILLET_OK) {
        ALOGE("send msg error");
    }
}

static void com_miui_server_greeze_GreezeManagerService_ClearConcernedUid
        (JNIEnv *env, jclass) {
    if (m_sock.sock == -1 || m_sock.in_use == 0)
        init_msock();

    if (m_sock.sock == -1) {
        ALOGE("create & bind socket fail");
        return;
    }

    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = PKG_TYPE;
    data.mod.u_priv.pkg.cmd = CLEAR_ALL_UID;
    if (send_msg_to_kernel(m_sock.sock, PKG_TYPE, &data) != MILLET_OK) {
        ALOGE("send msg error");
    }

}

static void com_miui_server_greeze_GreezeManagerService_QueryBinder
        (JNIEnv *env, jclass, jint uid) {
    if (m_sock.sock == -1 || m_sock.in_use == 0)
        init_msock();

    if (m_sock.sock == -1) {
        ALOGE("create & bind socket fail");
        return;
    }

    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = BINDER_ST_TYPE;
    data.mod.u_priv.binder_st.uid = uid;
    if (send_msg_to_kernel(m_sock.sock, BINDER_ST_TYPE, &data) != MILLET_OK) {
        ALOGE("send msg error");
    }

}

static void com_miui_server_greeze_GreezeManagerService_LoopOnce
        (JNIEnv *env, jclass) {
    if (m_sock.sock == -1 || m_sock.in_use == 0)
        init_msock();

    if (m_sock.sock == -1) {
        ALOGE("create & bind socket fail");
        return;
    }

    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = HANDSHK_TYPE;
    if (send_msg_to_kernel(m_sock.sock, HANDSHK_TYPE, &data) != MILLET_OK) {
        ALOGE("send msg error");
    }

}

static const JNINativeMethod methods[] = {
        {"nAddConcernedUid",   "(I)V", (void *) com_miui_server_greeze_GreezeManagerService_AddConcernedUid},
        {"nDelConcernedUid",   "(I)V", (void *) com_miui_server_greeze_GreezeManagerService_DelConcernedUid},
        {"nClearConcernedUid", "()V",  (void *) com_miui_server_greeze_GreezeManagerService_ClearConcernedUid},
        {"nQueryBinder",       "(I)V", (void *) com_miui_server_greeze_GreezeManagerService_QueryBinder},
        {"nLoopOnce",          "()V", (void *) com_miui_server_greeze_GreezeManagerService_LoopOnce},
};

static const char* const kClassPathName = "com/miui/server/greeze/GreezeManagerService";

jboolean register_com_miui_server_greeze_GreezeManagerService(JNIEnv* env)
{
    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    LOG_FATAL_IF(clazz == NULL, "Unable to find class com.miui.server.greeze.GreezeManagerService");

    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}
}