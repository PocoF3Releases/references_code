/*
 * Copyright (c) 2015-2017, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 * Apache license notifications and license are retained
 * for attribution purposes only.
 */
/*
 * Copyright (c) 2013 - 2015, The Linux Foundation. All rights reserved.
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
#define LOG_TAG "ExtendedCameraSource"
#include <common/AVLog.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <gui/Surface.h>
#include <utils/String8.h>
#include <cutils/properties.h>
#include "ExtendedCameraSource.h"
#include <OMX_QCOMExtns.h>

namespace android {

ExtendedCameraSource::ExtendedCameraSource(
             const sp<hardware::ICamera>& camera,
             const sp<ICameraRecordingProxy>& proxy,
             int32_t cameraId,
             const String16& clientName,
             uid_t clientUid,
             pid_t clientPid,
             Size videoSize,
             int32_t frameRate,
             const sp<IGraphicBufferProducer>& surface,
             bool /*storeMetaDataInVideoBuffers*/)
  : CameraSource(camera, proxy, cameraId, clientName, clientUid, clientPid, videoSize,
                 frameRate, surface),
    mNotifyPerfPending(false) {
    ALOGI("Careating : ExtendedCameraSource");
}

//#ifndef BRINGUP_WIP
void ExtendedCameraSource::releaseRecordingFrameHandle(native_handle_t* handle) {
    if (mNotifyPerfPending && handle != nullptr) {
        mNotifyPerfPending = false;
        AVLOGI("Notifying perf-event to Camera");
        MetaBufferUtil::setIntAt(handle, 0,
                MetaBufferUtil::INT_BUFEVENT, CAM_META_BUFFER_EVENT_PERF);
    }
    //TODO: Consider removing ExtendedCameraSource class as it is no longer useful
    //CameraSource::releaseRecordingFrameHandle(handle);
}
//#endif // BRINGUP_WIP

}
