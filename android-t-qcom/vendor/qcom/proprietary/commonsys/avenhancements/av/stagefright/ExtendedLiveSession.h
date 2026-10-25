/*
 * Copyright (c) 2015, Qualcomm Technologies, Inc.
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

#ifndef _EXTENDED_LIVE_SESSION_H_
#define _EXTENDED_LIVE_SESSION_H_

#include "httplive/LiveSession.h"

namespace android {

struct IMediaHTTPService;

struct ExtendedLiveSession : public LiveSession {

    enum FeatureFlag {
        NONE                          = 0,
        IMAGE_SUPPORT                 = 1,
        ACTUAL_BANDWIDTH              = 1 << 1,
        ASYM_BANDWIDTH                = 1 << 2,
        CUST_BANDWIDTH                = 1 << 3,
    };

    ExtendedLiveSession(
            const sp<AMessage> &notify,
            uint32_t flags,
            const sp<IMediaHTTPService> &httpService,
            uint32_t featureFlag);

    enum {
        kWhatImageDetected = 1000,
    };

protected:
    friend struct ExtendedPlaylistFetcher;
    virtual ~ExtendedLiveSession();
    virtual sp<PlaylistFetcher> addFetcher(const char *uri);
    virtual void onMessageReceived(const sp<AMessage> &msg);
    virtual status_t dequeueAccessUnit(StreamType stream, sp<ABuffer> *accessUnit);
    virtual void postPrepared(status_t err);
    virtual void swapPacketSource(StreamType stream);
    virtual void onMasterPlaylistFetched(const sp<AMessage> &msg);

    //The below function replicates the AOSP code, except that one
    //one additional check is added in the if condition where postPrepared()
    //is sent.
    virtual void onPollBuffering();
    virtual size_t getBandwidthIndex(int32_t bandwidthBps);

private:
    enum {
        kWhatDetectImage                = 'dimg',
        kWhatSwitchConfiguration        = 'swco',
        kWhatSwitchRealBandwidth        = 'swrb',
    };
    uint32_t mFeatureFlag;
    bool mIsImageDisplay;
    bool mDownloadFirstTs;
    bool isAudioOnly();
    void onMetaDataDetected(const sp<ABuffer> &buffer);
    void onDefaultMetaDataDetected();
    sp<ABuffer> decodeJpegImage(FILE* source, int32_t *width, int32_t *height);
    void notifyImage(const sp<ABuffer> &imageBuffer, int32_t width, int32_t height);

    void onSwitchConfiguration(const sp<AMessage> &msg);
    bool onSwitchToRealBandwidth();
    status_t switchToRealBandwidth();

    DISALLOW_EVIL_CONSTRUCTORS(ExtendedLiveSession);
};

} // namespace android

#endif // _EXTENDED_LIVE_SESSION_H_
