/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <linux/netlink.h>
#include <linux/sock_diag.h>
#include <linux/inet_diag.h>

#define LOG_TAG "SockDiag"

#include <android-base/strings.h>
#include <android/log.h>
#include "log/log.h"


#include "SockDiag.h"
#include <chrono>

#ifndef SOCK_DESTROY
#define SOCK_DESTROY 21
#endif

#define ARRAY_SIZE(x) (sizeof((x)) / (sizeof(((x)[0]))))
#define INET_DIAG_BC_MARK_COND 10

namespace android {

namespace {

int processNetlinkDump(int sock, const NetlinkDumpCallback& callback, uint32_t count) {
    char buf[kNetlinkDumpBufferSize];
    ssize_t bytesread;
    do {
        bytesread = read(sock, buf, sizeof(buf));

        if (bytesread < 0) {
            return -errno;
        }
        uint32_t len = bytesread;
        for (nlmsghdr *nlh = reinterpret_cast<nlmsghdr *>(buf);
             NLMSG_OK(nlh, len);
             nlh = NLMSG_NEXT(nlh, len)) {
            switch (nlh->nlmsg_type) {
              case NLMSG_DONE:
                return 0;
              case NLMSG_ERROR: {
                nlmsgerr *err = reinterpret_cast<nlmsgerr *>(NLMSG_DATA(nlh));
                return err->error;
              }
              default:
                callback(nlh, count);
            }
        }
    } while (bytesread > 0);
    return 0;
}

int checkError(int fd) {
    struct {
        nlmsghdr h;
        nlmsgerr err;
    } __attribute__((__packed__)) ack;
    ssize_t bytesread = recv(fd, &ack, sizeof(ack), MSG_DONTWAIT | MSG_PEEK);
    if (bytesread == -1) {
       // Read failed (error), or nothing to read (good).
       return (errno == EAGAIN) ? 0 : -errno;
    } else if (bytesread == (ssize_t) sizeof(ack) && ack.h.nlmsg_type == NLMSG_ERROR) {
        // We got an error. Consume it.
        recv(fd, &ack, sizeof(ack), 0);
        return ack.err.error;
    } else {
        // The kernel replied with something. Leave it to the caller.
        return 0;
    }
}

}  // namespace

int SockDiag::getLiveTcpInfos(const TcpInfoReader& tcpInfoReader, uint32_t count)
{
    const int proto = IPPROTO_TCP;
    const uint32_t states = (1 << TCP_ESTABLISHED) | (1 << TCP_SYN_SENT) | (1 << TCP_SYN_RECV);
    const uint8_t extensions = (1 << INET_DIAG_MEMINFO); // flag for dumping struct tcp_info.
    iovec iov[] = {
        { nullptr, 0 },
    };

    for (const int family : {AF_INET, AF_INET6}) {
        const char *familyName = (family == AF_INET) ? "IPv4" : "IPv6";
        if (int ret = sendDumpRequest(proto, family, extensions, states, iov, ARRAY_SIZE(iov))) {
            ALOGE("Failed to dump %s sockets struct tcp_info: %s", familyName, strerror(-ret));
            return ret;
        }
        if (int ret = readDiagMsgWithTcpInfo(tcpInfoReader, count)) {
            ALOGE("Failed to read %s sockets struct tcp_info: %s", familyName, strerror(-ret));
            return ret;
        }
    }
    return 0;
}


bool SockDiag::open() {
    if (hasSocks()) {
        return false;
    }
    mSock = socket(PF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_INET_DIAG);
    mWriteSock = socket(PF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_INET_DIAG);
    if (!hasSocks()) {
        closeSocks();
        return false;
    }
    sockaddr_nl nl = { .nl_family = AF_NETLINK };
    if ((connect(mSock, reinterpret_cast<sockaddr *>(&nl), sizeof(nl)) == -1) ||
        (connect(mWriteSock, reinterpret_cast<sockaddr *>(&nl), sizeof(nl)) == -1)) {
        closeSocks();
        return false;
    }
    return true;
}

int SockDiag::sendDumpRequest(uint8_t proto, uint8_t family, uint8_t extensions, uint32_t states,
                              iovec *iov, int iovcnt) {
    struct {
        nlmsghdr nlh;
        inet_diag_req_v2 req;
    } __attribute__((__packed__)) request = {
        .nlh = {
            .nlmsg_type = SOCK_DIAG_BY_FAMILY,
            .nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP,
        },
        .req = {
            .sdiag_family = family,
            .sdiag_protocol = proto,
            .idiag_ext = extensions,
            .idiag_states = states,
        },
    };

    size_t len = 0;
    iov[0].iov_base = &request;
    iov[0].iov_len = sizeof(request);
    for (int i = 0; i < iovcnt; i++) {
        len += iov[i].iov_len;
    }
    request.nlh.nlmsg_len = len;
    if (writev(mSock, iov, iovcnt) != (ssize_t) len) {
        return -errno;
    }
    return checkError(mSock);
}

int SockDiag::sendDumpRequest(uint8_t proto, uint8_t family, uint32_t states) {
    iovec iov[] = {
        { nullptr, 0 },
    };
    return sendDumpRequest(proto, family, 0, states, iov, ARRAY_SIZE(iov));
}

int SockDiag::readDiagMsgWithTcpInfo(const TcpInfoReader& tcpInfoReader, uint32_t count) {
     NetlinkDumpCallback callback = [tcpInfoReader] (nlmsghdr *nlh, uint32_t count) {
         if (nlh->nlmsg_type != SOCK_DIAG_BY_FAMILY) {
             ALOGE("expected nlmsg_type=SOCK_DIAG_BY_FAMILY, got nlmsg_type=%d", nlh->nlmsg_type);
             return;
         }
         Fwmark mark;
         struct tcp_info *tcpinfo = nullptr;
         uint32_t tcpinfoLength = 0;
         inet_diag_msg *msg = reinterpret_cast<inet_diag_msg *>(NLMSG_DATA(nlh));
         uint32_t attr_len = nlh->nlmsg_len - NLMSG_LENGTH(sizeof(*msg));
         struct rtattr *attr = reinterpret_cast<struct rtattr*>(msg+1);
         while (RTA_OK(attr, attr_len)) {
             if (attr->rta_type == INET_DIAG_INFO) {
                 tcpinfo = reinterpret_cast<struct tcp_info*>(RTA_DATA(attr));
                 tcpinfoLength = RTA_PAYLOAD(attr);
             }
             if (attr->rta_type == INET_DIAG_MARK) {
                 mark.intValue = *reinterpret_cast<uint32_t*>(RTA_DATA(attr));
             }
             attr = RTA_NEXT(attr, attr_len);
         }
         tcpInfoReader(mark, msg, tcpinfo, tcpinfoLength, count);
     };
     return processNetlinkDump(mSock, callback, count);
}

}  // namespace android
