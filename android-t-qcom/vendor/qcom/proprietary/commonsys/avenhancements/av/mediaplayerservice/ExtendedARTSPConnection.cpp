/*
 * Copyright (c) 2015, 2019, 2022 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 * Apache license notifications and license are retained
 * for attribution purposes only.
 */
/*
 * Copyright (C) 2010 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "ExtendedARTSPConnection"
#include <common/AVLog.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/rtsp/ARTSPConnection.h>
#include <media/stagefright/rtsp/NetworkUtils.h>

// TO-DO: Check if this is needed
#include <datasource/HTTPBase.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#include "mediaplayerservice/AVMediaServiceExtensions.h"
#include "mediaplayerservice/ExtendedARTSPConnection.h"
namespace android {

static status_t MakeSocketBlocking(int s, bool blocking);

ExtendedARTSPConnection::ExtendedARTSPConnection(bool uidValid, uid_t uid)
    : ARTSPConnection(uidValid, uid),
      mAddrHeader(NULL),
      mIsIPV6(false) {
    AVLOGV("ExtendedARTSPConnection");
}

ExtendedARTSPConnection::~ExtendedARTSPConnection() {
    AVLOGV("~ExtendedARTSPConnection");
    if (mAddrHeader != NULL) {
        freeaddrinfo((struct addrinfo *)mAddrHeader);
        mAddrHeader = NULL;
    }
}

bool ExtendedARTSPConnection::isIPV6() {
    AVLOGV("isIPV6 %d", mIsIPV6);
    return mIsIPV6;
}

void ExtendedARTSPConnection::performConnect(const sp<AMessage> &reply,
            AString host, unsigned port) {
    AVLOGV("performConnect");

    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof (hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host.c_str(), NULL, &hints, (struct addrinfo **)(&mAddrHeader));

    if (err != 0 || mAddrHeader == NULL) {
        AVLOGE("Unknown host, err %d (%s)", err, gai_strerror(err));
        reply->setInt32("result", -ENOENT);
        reply->post();
        mState = DISCONNECTED;

        if (mAddrHeader != NULL) {
            freeaddrinfo((struct addrinfo *)mAddrHeader);
            mAddrHeader = NULL;
        }
        return;
    }
    if (!createSocketAndConnect(mAddrHeader, port, reply)) {
        AVLOGV("Failed to connect to %s", host.c_str());
        reply->setInt32("result", -errno);
        mState = DISCONNECTED;
        mSocket = -1;
        reply->post();
        freeaddrinfo((struct addrinfo *)mAddrHeader);
        mAddrHeader = NULL;
    }
}

void ExtendedARTSPConnection::performCompleteConnection(const sp<AMessage> &msg,
            int err) {
    AVLOGV("performCompleteConnection");

    int32_t port;
    CHECK(msg->findInt32("port", &port));
    void *addr;
    CHECK(msg->findPointer("addrinfo", &addr));
    int32_t ipver;
    CHECK(msg->findInt32("ipversion", &ipver));
    sp<AMessage> reply;
    CHECK(msg->findMessage("reply", &reply));

    if (err != 0) {
        AVLOGE("err = %d (%s)", err, strerror(err));

        if (mUIDValid) {
            NetworkUtils::UnRegisterSocketUserTag(mSocket);
            NetworkUtils::UnRegisterSocketUserMark(mSocket);
        }
        close(mSocket);
        mSocket = -1;
        struct addrinfo *next = ((struct addrinfo *)addr)->ai_next;
        if (next == NULL) {
            freeaddrinfo((struct addrinfo *)mAddrHeader);
            mAddrHeader = NULL;
            mState = DISCONNECTED;
            reply->setInt32("result", -errno);
            reply->post();
        } else {
            AVLOGV("try to connect the next address");
            sp<AMessage> msg = new AMessage(kWhatReconnect, this);
            msg->setInt32("port", port);
            msg->setPointer("addrinfo", next);
            msg->setMessage("reply", reply);
            msg->setInt32("connection-id", mConnectionID);
            msg->post();
        }
    } else {
        AVLOGV("Connected in onCompleteConnection IP version = v%d", ipver);
        mIsIPV6 = (ipver == 6);
        reply->setInt32("result", OK);
        mState = CONNECTED;
        mNextCSeq = 1;
        freeaddrinfo((struct addrinfo *)mAddrHeader);
        mAddrHeader = NULL;
        postReceiveReponseEvent();
        reply->post();
    }
}

void ExtendedARTSPConnection::onMessageReceived(const sp<AMessage> &msg) {
    if (msg->what() == kWhatReconnect) {
        AVLOGV("kWhatReconnect");
        onReconnect(msg);
        return;
    }
    ARTSPConnection::onMessageReceived(msg);
}

bool ExtendedARTSPConnection::createSocketAndConnect(void *res,
        unsigned port, const sp<AMessage> &reply) {
    for (struct addrinfo *result = (struct addrinfo *)res; result; result = result->ai_next) {
        char ipstr[INET6_ADDRSTRLEN];
        int32_t ipver;
        void *sptr;

        switch (result->ai_family) {
        case AF_INET:
            sptr = &((struct sockaddr_in *)result->ai_addr)->sin_addr;
            ((struct sockaddr_in *)result->ai_addr)->sin_port = htons(port);
            reply->setInt32("server-ip", ntohl(((struct in_addr *)sptr)->s_addr));
            ipver = 4;
            break;
        case AF_INET6:
            sptr = &((struct sockaddr_in6 *)result->ai_addr)->sin6_addr;
            ((struct sockaddr_in6 *)result->ai_addr)->sin6_port = htons(port);
            ipver = 6;
            break;
        default:
            AVLOGW("Skipping unknown protocol family %d", result->ai_family);
            continue;
        }

        inet_ntop(result->ai_family, sptr, ipstr, sizeof(ipstr));
        AVLOGV("Connecting to IPv%d: %s", ipver, ipstr);

        mSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

        if (mUIDValid) {
            NetworkUtils::RegisterSocketUserTag(mSocket, mUID,
                                            (uint32_t)*(uint32_t*) "RTSP");
            NetworkUtils::RegisterSocketUserMark(mSocket, mUID);
        }

        MakeSocketBlocking(mSocket, false);

        int err = ::connect(mSocket, result->ai_addr, result->ai_addrlen);
        if (err == 0) {
            AVLOGV("Connected to (%s), IP version = v%d", ipstr, ipver);
            reply->setInt32("result", OK);
            mState = CONNECTED;
            mNextCSeq = 1;
            mIsIPV6 = (ipver == 6);
            postReceiveReponseEvent();
            reply->post();
            freeaddrinfo((struct addrinfo *)mAddrHeader);
            mAddrHeader = NULL;
            return true;
        }

        if (errno == EINPROGRESS) {
            AVLOGV("Connection to %s in progress", ipstr);
            sp<AMessage> msg = new AMessage(kWhatCompleteConnection, this);
            msg->setInt32("ipversion", ipver);
            msg->setInt32("port", port);
            msg->setPointer("addrinfo", result);
            msg->setMessage("reply", reply);
            msg->setInt32("connection-id", mConnectionID);
            msg->post();
            return true;
        }

        if (mUIDValid) {
            NetworkUtils::UnRegisterSocketUserTag(mSocket);
            NetworkUtils::UnRegisterSocketUserMark(mSocket);
        }
        close(mSocket);
        AVLOGV("Connection err %d, (%s)", errno, strerror(errno));
    }
    return false;
}

void ExtendedARTSPConnection::onReconnect(const sp<AMessage> &msg) {
    AVLOGV("onReconnect");
    sp<AMessage> reply;
    CHECK(msg->findMessage("reply", &reply));
    int32_t connectionID;
    CHECK(msg->findInt32("connection-id", &connectionID));
    if ((connectionID != mConnectionID) || mState != CONNECTING) {
        // While we were attempting to connect, the attempt was
        // cancelled.
        reply->setInt32("result", -ECONNABORTED);
        reply->post();
        if (mAddrHeader != NULL) {
            freeaddrinfo((struct addrinfo *)mAddrHeader);
            mAddrHeader = NULL;
        }
        return;
    }
    int32_t port;
    CHECK(msg->findInt32("port", &port));
    void *addr;
    CHECK(msg->findPointer("addrinfo", &addr));
    if (!createSocketAndConnect(addr, port, reply)) {
        AVLOGE("Failed to reconnect");
        reply->setInt32("result", -errno);
        mState = DISCONNECTED;
        mSocket = -1;
        reply->post();
        freeaddrinfo((struct addrinfo *)mAddrHeader);
        mAddrHeader = NULL;
    }
}

static status_t MakeSocketBlocking(int s, bool blocking) {
    int flags = fcntl(s, F_GETFL, 0);
    if (flags == -1) {
        return UNKNOWN_ERROR;
    }

    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }

    flags = fcntl(s, F_SETFL, flags);
    return flags == -1 ? UNKNOWN_ERROR : OK;
}

}
