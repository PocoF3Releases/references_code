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

#ifndef _SOCK_DIAG_H
#define _SOCK_DIAG_H

#include <unistd.h>
#include <sys/socket.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include "Fwmark.h"
#include <linux/sock_diag.h>
#include <linux/inet_diag.h>

#include <functional>
#include <set>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdarg.h>

const int kNetlinkDumpBufferSize = 8192;

struct inet_diag_msg;
struct tcp_info;

namespace android {

typedef std::function<void(nlmsghdr *, uint32_t count)> NetlinkDumpCallback;

class SockDiag {

  public:
    static const int kBufferSize = 4096;


    typedef std::function<void(Fwmark mark, const struct inet_diag_msg *, const struct tcp_info *,
                                uint32_t tcp_info_length, uint32_t count)> TcpInfoReader;

    SockDiag() : mSock(-1), mWriteSock(-1) {}
    bool open();
    virtual ~SockDiag() { closeSocks(); }

    int sendDumpRequest(uint8_t proto, uint8_t family, uint32_t states);
    int readDiagMsgWithTcpInfo(const TcpInfoReader& callback, uint32_t count);
    int getLiveTcpInfos(const TcpInfoReader& sockInfoReader, uint32_t count);


  private:
    int mSock;
    int mWriteSock;
    bool hasSocks() { return mSock != -1 && mWriteSock != -1; }
    void closeSocks() { close(mSock); close(mWriteSock); mSock = mWriteSock = -1; }
    int sendDumpRequest(uint8_t proto, uint8_t family, uint8_t extensions, uint32_t states,
                        iovec *iov, int iovcnt);
};

}  // namespace android

#endif  // _SOCK_DIAG_H
