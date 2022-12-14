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
#ifndef _EXT_CAMERA_SOURCE_H
#define _EXT_CAMERA_SOURCE_H

#include <media/stagefright/CameraSource.h>

namespace android {

struct ExtendedCameraSource : public CameraSource {
    ExtendedCameraSource(const sp<hardware::ICamera>& camera, const sp<ICameraRecordingProxy>& proxy,
                 int32_t cameraId, const String16& clientName, uid_t clientUid,
                 pid_t clientPid, Size videoSize, int32_t frameRate,
                 const sp<IGraphicBufferProducer>& surface,
                 bool storeMetaDataInVideoBuffers);

//#ifndef BRINGUP_WIP
    virtual void notifyPerformanceMode() {mNotifyPerfPending = true;}
    virtual void releaseRecordingFrameHandle(native_handle_t* handle);
//#endif // BRINGUP_WIP

private:
    bool mNotifyPerfPending;
};

}
#endif //_EXT_CAMERA_SOURCE_H
