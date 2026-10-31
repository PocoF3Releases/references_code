/*
 * Copyright (c) 2015-2016, 2018 Qualcomm Technologies, Inc.
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

#define LOG_TAG "ExtendedCameraSourceTimeLapse"

#include <common/AVLog.h>
#include <inttypes.h>
#include <binder/IPCThreadState.h>
#include <utils/Log.h>
#include <stagefright/ExtendedCameraSourceTimeLapse.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AUtils.h>
#include <media/hardware/HardwareAPI.h>
#include <OMX_QCOMExtns.h>

namespace android {

ExtendedCameraSourceTimeLapse::ExtendedCameraSourceTimeLapse(
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
    //TODO: Remove this parameter in all header and fractory class
    bool /*storeMetaDataInVideoBuffers*/) : CameraSourceTimeLapse(camera, proxy,
        cameraId, clientName, clientUid, clientPid, videoSize, videoFrameRate, surface,
        timeBetweenTimeLapseFrameCaptureUs),
    mBatchSize(1),
    mRecPause(false),
    mPauseAdjTimeUs(0),
    mPauseStartTimeUs(0),
    mPauseEndTimeUs(0),
    mLastFrameOriginalTimestampUs(0),
    mBatchLastFrameTimestampOffsetUs(0) {

    if (mInitCheck == OK) {
        // Can't query params until we hijack Camera's ownership
        int64_t token = IPCThreadState::self()->clearCallingIdentity();

        mBatchSize = max(1, CameraParameters(camera->getParameters())
                        .getInt("video-batch-size"));

        IPCThreadState::self()->restoreCallingIdentity(token);

        if (mBatchSize > 1) {
            mBatchLastFrameTimestampOffsetUs = mTimeBetweenTimeLapseVideoFramesUs * (mBatchSize - 1);
            mTimeBetweenTimeLapseVideoFramesUs *= mBatchSize;
        }
    }
}

status_t ExtendedCameraSourceTimeLapse::start(MetaData *params) {
    if(mRecPause) {
        mRecPause = false;
        mPauseAdjTimeUs += mPauseEndTimeUs - mPauseStartTimeUs;
        AVLOGI("resume : mPause Adj / End / Start : %" PRId64 " / %" PRId64 " / %" PRId64 " us",
                mPauseAdjTimeUs, mPauseEndTimeUs, mPauseStartTimeUs);
        return OK;
    }
    mRecPause = false;
    mPauseAdjTimeUs = 0;
    mPauseStartTimeUs = 0;
    mPauseEndTimeUs = 0;
    mLastFrameOriginalTimestampUs = 0;
    return CameraSource::start(params);
}

#ifndef BRINGUP_WIP
status_t ExtendedCameraSourceTimeLapse::pause() {
    mRecPause = true;
    mPauseStartTimeUs = mLastFrameOriginalTimestampUs;
    AVLOGI("pause : mPauseStart %" PRId64 " us, #Queued Frames : %zu",
            mPauseStartTimeUs, mFramesReceived.size());
    return OK;
}
#endif
bool ExtendedCameraSourceTimeLapse::skipFrameAndModifyTimeStamp(int64_t *timestampUs,
        const sp<IMemory> &frame) {
    bool skip = CameraSourceTimeLapse::skipFrameAndModifyTimeStamp(timestampUs);

    // Adjust timestamps inside the meta buffers for batch payload
    if (metaDataStoredInVideoBuffers() != kMetadataBufferTypeInvalid) {
        VideoNativeHandleMetadata *meta = static_cast<VideoNativeHandleMetadata *>(frame->unsecurePointer());
        if (!meta || meta->eType != kMetadataBufferTypeNativeHandleSource) {
            AVLOGV("Invalid meta-buffer");
            return skip;
        }
        native_handle_t *hnd = (native_handle_t *)meta->pHandle;
        int batchSize = MetaBufferUtil::getBatchSize(hnd);
        if (batchSize < 2) {
            return skip;
        }

        int64_t timeBetweenBatchedFramesNs = mTimeBetweenTimeLapseVideoFramesUs * 1E3 / batchSize;
        for (int i = 0; i < MetaBufferUtil::getBatchSize(hnd); ++i) {
            MetaBufferUtil::setIntAt(hnd, i, MetaBufferUtil::INT_TIMESTAMP, i * timeBetweenBatchedFramesNs);
        }
    }

    return skip;
}
#ifndef BRINGUP_WIP
void ExtendedCameraSourceTimeLapse::dataCallbackTimestamp(int64_t timestampUs,
        int32_t msgType, const sp<IMemory> &data)
{
    int64_t inputTimestamp = timestampUs + mBatchLastFrameTimestampOffsetUs;
    timestampUs -= mPauseAdjTimeUs;
    {
        Mutex::Autolock autoLock(mLock);
        if (mRecPause == true) {
            if(!mFramesReceived.empty()) {
                AVLOGV("releaseQueuedFrames - #Queued Frames : %zu",
                        mFramesReceived.size());
                releaseQueuedFrames();
            }
            AVLOGV("release One Video Frame for Pause : %" PRId64 " us",
                    timestampUs);
            releaseOneRecordingFrame(data);
            mPauseEndTimeUs = inputTimestamp;
            return;
        }
    }
    mSkipCurrentFrame = skipFrameAndModifyTimeStamp(&timestampUs, data);
    mLastFrameOriginalTimestampUs = inputTimestamp;
    CameraSource::dataCallbackTimestamp(timestampUs, msgType, data);
}
#endif
}
