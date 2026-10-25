/*
 * Copyright (c) 2015-2016, Qualcomm Technologies, Inc.
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

#ifndef _EXTENDED_CAMERA_TIME_LAPSE_H_
#define _EXTENDED_CAMERA_TIME_LAPSE_H_

#include <media/stagefright/CameraSourceTimeLapse.h>

namespace android {

class ExtendedCameraSourceTimeLapse : public CameraSourceTimeLapse {
public:
    ExtendedCameraSourceTimeLapse(
        const sp<hardware::ICamera> &camera,
        const sp<ICameraRecordingProxy> &proxy,
        int32_t cameraId,
        const String16& clientName,
        uid_t clientUid,
        pid_t clientPid,
        Size videoSize,
        int32_t videoFrameRate,
        const sp<IGraphicBufferProducer>& surface,
        int64_t timeBetweenTimeLapseFrameCaptureUs,
        bool storeMetaDataInVideoBuffers = true);
    virtual status_t start(MetaData *params = NULL);
#ifndef BRINGUP_WIP
    virtual status_t pause();
#endif

private:
    int mBatchSize;
    bool mRecPause;
    int64_t mPauseAdjTimeUs;
    int64_t mPauseStartTimeUs;
    int64_t mPauseEndTimeUs;
    int64_t mLastFrameOriginalTimestampUs;
    int64_t mBatchLastFrameTimestampOffsetUs;

    // NOTE: While this is exactly the same as the base-class function, it
    // allows us to circumvent a CLANG error for overloading an inherited
    // virtual method.
    virtual bool skipFrameAndModifyTimeStamp(int64_t *timestampUs) {
        return CameraSourceTimeLapse::skipFrameAndModifyTimeStamp(timestampUs);
    }
    bool skipFrameAndModifyTimeStamp(int64_t *, const sp<IMemory> &);
#ifndef BRINGUP_WIP
    void dataCallbackTimestamp(int64_t, int32_t, const sp<IMemory> &);
#endif
};

}
#endif
