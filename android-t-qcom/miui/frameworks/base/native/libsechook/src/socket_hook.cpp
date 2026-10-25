#define ALOGD(...)
#define LOG_TAG "LIBSEC"
#include <utils/Log.h>
#include <cutils/qtaguid.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <private/android_filesystem_config.h>

#ifdef PLATFORM_VERSION_GREATER_THAN_25
#include <bits/glibc-syscalls.h>
#else
#include <sys/glibc-syscalls.h>
#endif
extern "C" int __set_errno(int);

#ifndef __set_errno
#define __set_errno(val) errno = (val)
#endif

#include "include/socket_hook.h"
#include "include/hookutils.h"

#define FIREWALL_SERVER_NAME "mfwd%d"

#define FIREWALL_ACTION_BLOCK 1
#define FIREWALL_ACTION_ALLOW 0
#define FIREWALL_ACTION_INIT -1

#define SOCKET_QUERY_INTERVAL 2
#define SOCKET_QUERY_TIMEOUT 3

#define TAG_SERVER_NAME "mtagd%d"

#define TAG_VALUE_NONE 0
#define TAG_VALUE_INIT -1

#define SERVER_NAME_BUFFER_LENGTH 10

#define USER_OWNER 0
#define USER_XSPACE 999


/* Block by self feature */
static int block_by_self = 0;

void block_self_network(int block) {
    block_by_self = block;
}

int is_block_self_network() {
    return block_by_self;
}


/* Hooked shandow feature */
int (*shandow_socket_ptr)(int, int, int) = (int (*)(int, int, int))0;

void set_shandow_socket(int (*shandow_socket)(int, int, int)) {
    shandow_socket_ptr = shandow_socket;
}

static struct sockaddr_un s_mfwd_addr;
static socklen_t s_mfwd_len;
static struct sockaddr_un s_mtagd_addr;
static socklen_t s_mtagd_len;


#ifdef PLATFORM_VERSION_GREATER_THAN_23
void* netdClientHandle = dlopen("libnetd_client.so", RTLD_NOW);
ptr_t (*get_bionic_socket)() = (ptr_t (*)()) dlsym(netdClientHandle, "getBionicSocket");
int (*real_socket_ptr)(int, int, int) = (int (*)(int, int, int))get_bionic_socket();
#else //PLATFORM_VERSION_GREATER_THAN_23
int (*real_socket_ptr)(int, int, int) = (int (*)(int, int, int))get_fun_ptr_dlsym("socket");
#endif //PLATFORM_VERSION_GREATER_THAN_23


static unsigned long pre_time = 0;

static int m_fw_result = FIREWALL_ACTION_INIT;

static int m_tag_value = TAG_VALUE_INIT;

pthread_once_t pthread_once_init_socket_addr = PTHREAD_ONCE_INIT;
void static init_socket_addr() {
    int my_userid = getuid() / AID_USER;

    // XSpace user run same rules with owner user
    if (my_userid == USER_XSPACE) {
        my_userid = USER_OWNER;
    }

    s_mfwd_addr.sun_family = AF_LOCAL;
    s_mfwd_addr.sun_path[0] = '\0';
    sprintf(&s_mfwd_addr.sun_path[1], FIREWALL_SERVER_NAME, my_userid);
    s_mfwd_len = offsetof(struct sockaddr_un, sun_path) + 1
            + strlen(&s_mfwd_addr.sun_path[1]);

    s_mtagd_addr.sun_family = AF_LOCAL;
    s_mtagd_addr.sun_path[0] = '\0';
    sprintf(&s_mtagd_addr.sun_path[1], TAG_SERVER_NAME, my_userid);
    s_mtagd_len = offsetof(struct sockaddr_un, sun_path) + 1
            + strlen(&s_mtagd_addr.sun_path[1]);
}

int static tag_socket(int fd, int tag) {
    return qtaguid_tagSocket(fd, tag, getuid());
}

void static query_socket_tag_value() {
    int sk_fd;
    sk_fd = real_socket_ptr(AF_LOCAL, SOCK_STREAM, 0);
    if (sk_fd < 0) {
        return;
    }

    pthread_once(&pthread_once_init_socket_addr, init_socket_addr);

    if (connect(sk_fd, (struct sockaddr *) &s_mtagd_addr, s_mtagd_len) < 0) {
        close(sk_fd);
        return;
    }

    struct timeval tv;
    tv.tv_sec = SOCKET_QUERY_TIMEOUT;
    tv.tv_usec = 0;
    if(setsockopt(sk_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))<0){
        ALOGE("tag_socket socket option SO_RCVTIMEO does not support\n");
    }

    int tag_value_res = -1;
    if (read(sk_fd, &tag_value_res, sizeof(tag_value_res)) < 0) {
        ALOGE("tag_socket read failed or timeout\n");
        close(sk_fd);
        return;
    }

    if(tag_value_res != -1) {
        m_tag_value = ntohl(tag_value_res);
    }

    close(sk_fd);
}

void static tag_socket_if_need(int fd) {
    if(m_tag_value == TAG_VALUE_INIT) {
        ALOGD("query_socket_tag_value");
        query_socket_tag_value();
    }

    ALOGD("tag_socket pid:%d tag:0x%x\n", getpid(), m_tag_value);
    if(m_tag_value != TAG_VALUE_NONE && m_tag_value != TAG_VALUE_INIT) {
        tag_socket(fd, m_tag_value);
    }
}

int ___socket(int domain, int type, int protocol) {

    ALOGD("___socket: domain%d, type:%d, protocol:%d\n", domain, type, protocol);

    if (domain != AF_INET && domain != AF_INET6) {
        ALOGD("___socket not network socket");
        return real_socket_ptr(domain, type, protocol);
    }

    if (block_by_self) {
        ALOGD("network restricted by self");
        m_fw_result = FIREWALL_ACTION_BLOCK;
        goto checkresult;
    }

    ALOGD("firewall hooked socket AF_INET or AF_INET6");

    unsigned long cur_time;
    cur_time = time(NULL);

    ALOGD("firewall now:%lu pre:%lu", cur_time, pre_time);

    if (cur_time - pre_time >= SOCKET_QUERY_INTERVAL) {
        pre_time = cur_time;
    } else {
        goto checkresult;
    }

    int sk_fd;
    sk_fd = real_socket_ptr(AF_LOCAL, SOCK_STREAM, 0);
    if (sk_fd < 0) {
        goto end;
    }

    ALOGD("firewall hooked local socket ok ");

    pthread_once(&pthread_once_init_socket_addr, init_socket_addr);

    if (connect(sk_fd, (struct sockaddr *) &s_mfwd_addr, s_mfwd_len) < 0) {
        close(sk_fd);
        goto end;
    }

    struct timeval tv;
    tv.tv_sec = SOCKET_QUERY_TIMEOUT;
    tv.tv_usec = 0;
    if(setsockopt(sk_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))<0){
        ALOGE("firewall socket option SO_RCVTIMEO does not support\n");
    }

    if (read(sk_fd, &m_fw_result, sizeof(m_fw_result)) < 0) {
        ALOGE("firewall read failed or timeout\n");
        close(sk_fd);
        goto end;
    }

    m_fw_result = ntohl(m_fw_result);

    close(sk_fd);

checkresult:

    ALOGD("firewall result:%d", m_fw_result);

    if (m_fw_result == FIREWALL_ACTION_BLOCK) {
        __set_errno(EACCES);
        return -1;
    }

end:
    int fd = -1;
    if (shandow_socket_ptr) {
        fd = shandow_socket_ptr(domain, type, protocol);
    } else {
        fd = real_socket_ptr(domain, type, protocol);
    }
    //if(m_tag_value != TAG_VALUE_NONE) {
    //    tag_socket_if_need(fd);
    //}
    return fd;
}
