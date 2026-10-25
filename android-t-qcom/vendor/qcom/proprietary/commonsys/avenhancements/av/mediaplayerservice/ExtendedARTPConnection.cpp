/*
 * Copyright (c) 2015, 2022 Qualcomm Technologies, Inc.
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
#define LOG_TAG "ExtendedARTPConnection"
#include <common/AVLog.h>

#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/rtsp/ARTPConnection.h>

#include <arpa/inet.h>

#include "mediaplayerservice/AVMediaServiceExtensions.h"
#include "mediaplayerservice/ExtendedARTPConnection.h"

namespace android {

ExtendedARTPConnection::ExtendedARTPConnection()
    : mIsIPV6(false) {
    AVLOGV("ExtendedARTPConnection");
}

ExtendedARTPConnection::~ExtendedARTPConnection() {
    AVLOGV("~ExtendedARTPConnection");
}

size_t ExtendedARTPConnection::sockAddrSize() {
    if (mIsIPV6) {
        AVLOGV("IPV6 sock size");
        return sizeof(struct sockaddr_in6);
    } else {
        AVLOGV("IPV4 sock size");
        return sizeof(struct sockaddr_in);
    }
}

void ExtendedARTPConnection::onAddStream(const sp<AMessage> &msg) {
    AVLOGV("onAddStream");
    int32_t isIPV6;
    if (msg->findInt32("isIPV6", &isIPV6)) {
        AVLOGV("isIPV6 is %d", isIPV6);
        mIsIPV6 = isIPV6;
    }
    ARTPConnection::onAddStream(msg);
}
}
