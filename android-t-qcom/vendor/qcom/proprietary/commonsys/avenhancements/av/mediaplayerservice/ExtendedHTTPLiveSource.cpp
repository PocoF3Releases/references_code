/*
 * Copyright (c) 2015-2019, Qualcomm Technologies, Inc.
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

#define LOG_TAG "ExtendedHTTPLiveSource"
#include <common/AVLog.h>
#include <cutils/properties.h>

#include "ExtendedHTTPLiveSource.h"
#include "stagefright/ExtendedLiveSession.h"

namespace android {

ExtendedHTTPLiveSource::ExtendedHTTPLiveSource(
        const sp<AMessage> &notify,
        const sp<IMediaHTTPService> &httpService,
        const char *url,
        const KeyedVector<String8, String8> *headers)
    : HTTPLiveSource(notify, httpService, url, headers) {
    AVLOGV("create Extended HTTPLiveSource");
}

ExtendedHTTPLiveSource::~ExtendedHTTPLiveSource() {
    AVLOGV("~ExtendedHTTPLiveSource");
}

void ExtendedHTTPLiveSource::prepareAsync() {
    AVLOGV("prepareAsync");
    if (mLiveLooper == NULL) {
         mLiveLooper = new ALooper;
         mLiveLooper->setName("http live");
         mLiveLooper->start();

         mLiveLooper->registerHandler(this);
    }

    sp<AMessage> notify = new AMessage(kWhatSessionNotify, this);

    mLiveSession = new ExtendedLiveSession(
             notify,
             (mFlags & kFlagIncognito) ? LiveSession::kFlagIncognito : 0,
             mHTTPService,
             getSupportedFeature());

    mLiveLooper->registerHandler(mLiveSession);

    mLiveSession->connectAsync(
            mURL.c_str(), mExtraHeaders.isEmpty() ? NULL : &mExtraHeaders);
}


uint32_t ExtendedHTTPLiveSource::getSupportedFeature() {
    AVLOGV("getSupportedFeature");
    uint32_t flag = ExtendedLiveSession::NONE;
    char value[PROPERTY_VALUE_MAX];
#ifdef HLS_AUDIO_ONLY_IMG_DISPLAY
    if (property_get("persist.vendor.media.hls.img_sup", value, "0")
            && atoi(value)) {
        flag |= ExtendedLiveSession::IMAGE_SUPPORT;
    }
#endif
    if (property_get("persist.vendor.media.hls.act_bw", value, "0")
            && atoi(value)) {
        flag |= ExtendedLiveSession::ACTUAL_BANDWIDTH;
    }
    if (property_get("persist.vendor.media.hls.asym_bw", value, "0")
            && atoi(value)) {
        flag |= ExtendedLiveSession::ASYM_BANDWIDTH;
    }
    if (property_get("persist.vendor.media.hls.cust_bw", value, "0")
            && atoi(value)) {
        flag |= ExtendedLiveSession::CUST_BANDWIDTH;
    }
    return flag;
}

} // namespace android
