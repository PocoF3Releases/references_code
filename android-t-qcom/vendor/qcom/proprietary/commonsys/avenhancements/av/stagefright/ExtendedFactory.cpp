/*
 * Copyright (c) 2015-2021, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a contribution.
 *
 * Copyright (C) 2009, 2010 The Android Open Source Project
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
#define LOG_TAG "ExtendedFactory"
#include <common/AVLog.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MetaData.h>
#include <include/media/stagefright/MediaExtractor.h>

#include <stagefright/AVExtensions.h>
#include "stagefright/ExtendedFactory.h"
#include "stagefright/ExtendedACodec.h"
#include "stagefright/ExtendedAudioSource.h"
#include "stagefright/ExtendedCameraSource.h"
#include "stagefright/ExtendedCameraSourceTimeLapse.h"

namespace android {

AVFactory *createExtendedFactory() {
    return new ExtendedFactory;
}

sp<ACodec> ExtendedFactory::createACodec() {
    return new ExtendedACodec;
}

AudioSource* ExtendedFactory::createAudioSource(
            const audio_attributes_t *attr,
            const AttributionSourceState& attributionSource,
            uint32_t sampleRate,
            uint32_t channels,
            uint32_t outSampleRate,
            audio_port_handle_t selectedDeviceId,
            audio_microphone_direction_t selectedMicDirection,
            float selectedMicFieldDimension) {
    // TODO(b/129493645): use new selectedMicDirection and selectedMicFieldDimension params
    return new ExtendedAudioSource(attr, attributionSource, sampleRate,
                            channels, outSampleRate, selectedDeviceId,
                            selectedMicDirection, selectedMicFieldDimension);
}

//#ifndef BRINGUP_WIP
CameraSource* ExtendedFactory::CreateCameraSourceFromCamera(
            const sp<hardware::ICamera> &camera,
            const sp<ICameraRecordingProxy> &proxy,
            int32_t cameraId,
            const String16& clientName,
            uid_t clientUid,
            pid_t clientPid,
            Size videoSize,
            int32_t frameRate,
            const sp<IGraphicBufferProducer>& surface,
            bool storeMetaDataInVideoBuffers) {
    return new ExtendedCameraSource(camera, proxy, cameraId,
            clientName, clientUid, clientPid, videoSize, frameRate, surface,
            storeMetaDataInVideoBuffers);
}
//#endif
CameraSourceTimeLapse *ExtendedFactory::CreateCameraSourceTimeLapseFromCamera(
        const sp<hardware::ICamera> &camera,
        const sp<ICameraRecordingProxy> &proxy,
        int32_t cameraId,
        const String16& clientName,
        uid_t clientUid,
        pid_t clientPid,
        Size videoSize,
        int32_t videoFrameRate,
        const sp<IGraphicBufferProducer>& surface,
        int64_t timeBetweenFrameCaptureUs,
        bool storeMetaDataInVideoBuffers) {
    return new ExtendedCameraSourceTimeLapse(camera, proxy, cameraId, clientName,
            clientUid, clientPid, videoSize, videoFrameRate, surface,
            timeBetweenFrameCaptureUs, storeMetaDataInVideoBuffers);
}

ExtendedFactory::ExtendedFactory() {
    updateLogLevel();
    AVLOGV("ExtendedFactory()");
}

ExtendedFactory::~ExtendedFactory() {
    AVLOGV("~ExtendedFactory()");
}

}
