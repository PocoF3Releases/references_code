/*
 * Copyright (c) 2015,2019 Qualcomm Technologies, Inc.
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

#ifndef _EXTENDED_HTTPLIVE_SOURCE_H_
#define _EXTENDED_HTTPLIVE_SOURCE_H_

#include "nuplayer/include/nuplayer/NuPlayer.h"
#include "nuplayer/include/nuplayer/HTTPLiveSource.h"

namespace android {

struct IMediaHTTPService;

struct ExtendedHTTPLiveSource : public NuPlayer::HTTPLiveSource {
    ExtendedHTTPLiveSource(
            const sp<AMessage> &notify,
            const sp<IMediaHTTPService> &httpService,
            const char *url,
            const KeyedVector<String8, String8> *headers);

    virtual void prepareAsync();

protected:
    virtual ~ExtendedHTTPLiveSource();

private:
    uint32_t getSupportedFeature();

    DISALLOW_EVIL_CONSTRUCTORS(ExtendedHTTPLiveSource);
};

} // namespace android

#endif // _EXTENDED_HTTPLIVE_SOURCE_H_
