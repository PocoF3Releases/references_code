/*
 * Copyright (c) 2015-2019,2021, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef _EXTENDED_FACTORY_H_
#define _EXTENDED_FACTORY_H_

#include "mpeg2ts/ESQueue.h"

namespace android {

using content::AttributionSourceState;

class MediaExtractor;
struct ExtendedFactory : public AVFactory {

    virtual sp<ACodec> createACodec();
#ifdef P_BRINGUP
    virtual MediaExtractor* createExtendedExtractor(
             const sp<DataSource> &source, const char *mime,
             const sp<AMessage> &msg,
             const uint32_t flags);
#endif //P_BRINGUP
#ifndef BRINGUP_WIP
    virtual sp<NuCachedSource2> createCachedSource(
            const sp<DataSource> &source,
            const char *cacheConfig,
            bool disconnectAtHighwatermark);
#endif
    virtual AudioSource* createAudioSource(
            const audio_attributes_t *attr,
            const AttributionSourceState& attributionSource,
            uint32_t sampleRate,
            uint32_t channels,
            uint32_t outSampleRate = 0,
            audio_port_handle_t selectedDeviceId = AUDIO_PORT_HANDLE_NONE,
            audio_microphone_direction_t selectedMicDirection = MIC_DIRECTION_UNSPECIFIED,
            float selectedMicFieldDimension = MIC_FIELD_DIMENSION_NORMAL);
//#ifndef BRINGUP_WIP

    virtual CameraSource *CreateCameraSourceFromCamera(
            const sp<hardware::ICamera> &camera,
            const sp<ICameraRecordingProxy> &proxy,
            int32_t cameraId,
            const String16& clientName,
            uid_t clientUid,
            pid_t clientPid,
            Size videoSize,
            int32_t frameRate,
            const sp<IGraphicBufferProducer>& surface,
            bool storeMetaDataInVideoBuffers = true);
//#endif
    virtual CameraSourceTimeLapse *CreateCameraSourceTimeLapseFromCamera(
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
            bool storeMetaDataInVideoBuffers = true);

protected:
    virtual ~ExtendedFactory();

private:
    ExtendedFactory(const ExtendedFactory &);
    ExtendedFactory &operator=(ExtendedFactory &);

public:
    ExtendedFactory();
};

extern "C" AVFactory *createExtendedFactory();

} //namespace android

#endif // _EXTENDED_FACTORY_H_

