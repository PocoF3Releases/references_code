/*
 * Copyright (c) 2015-2019,2021-2022 Qualcomm Technologies, Inc.
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
#define LOG_TAG "ExtendedSFRecorder"
#include <inttypes.h>
#include <common/AVLog.h>
#include <utils/Errors.h>
#include <cutils/properties.h>

#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/AudioSource.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/AidlConversion.h>
#include <media/MediaProfiles.h>

#include <StagefrightRecorder.h>

#include "mediaplayerservice/AVMediaServiceExtensions.h"
#include "stagefright/ExtendedAudioSource.h"
#include "stagefright/CompressAACAudioSource.h"
#include "mediaplayerservice/ExtendedSFRecorder.h"
#include "mediaplayerservice/ExtendedWriter.h"
#include "mediaplayerservice/WAVEWriter.h"

//TODO: don't need this dependency after MetaData keys are moved to separate file
#include <stagefright/AVExtensions.h>
#include <stagefright/ExtendedUtils.h>

#include <OMX_Video.h>

namespace android {
ExtendedSFRecorder::ExtendedSFRecorder(const AttributionSourceState& attributionSource)
  : StagefrightRecorder(attributionSource),
    mRecPaused(false) {
    mAttributionSource = attributionSource;
    updateLogLevel();
    pConfigsIns = AVConfigHelper::getInstance();
    mAppOpsManager = std::make_shared<AppOpsManager>();
    AVLOGV("ExtendedSFRecorder()");
}

ExtendedSFRecorder::~ExtendedSFRecorder() {
    AVLOGV("~ExtendedSFRecorder()");
}

status_t ExtendedSFRecorder::setAudioSource(audio_source_t as) {
    if (!isAudioDisabled()) {
        return StagefrightRecorder::setAudioSource(as);
    }
    return OK;
}

status_t ExtendedSFRecorder::setAudioEncoder(audio_encoder ae) {
    if (!isAudioDisabled()) {
        // Do more QC stuff here if required
        return StagefrightRecorder::setAudioEncoder(ae);
    }
    return OK;
}

void ExtendedSFRecorder::setupCustomVideoEncoderParams(sp<MediaSource> cameraSource,
        sp<AMessage> &format) {
    AVLOGV("setupCustomVideoEncoderParams");

    if (cameraSource != NULL) {
        sp<MetaData> meta = cameraSource->getFormat();
        int32_t batchSize;
        if (meta->findInt32(kKeyLocalBatchSize, &batchSize)) {
            AVLOGV("Setting batch size = %d", batchSize);
            format->setInt32("batch-size", batchSize);
        }
    }
    setEncoderProfile();
}

bool ExtendedSFRecorder::isAudioDisabled() {
    bool bAudioDisabled = false;

    bAudioDisabled = pConfigsIns->isSFRecorderDisabled();
    AVLOGD("Audio disabled %d", bAudioDisabled);

    return bAudioDisabled;
}

void ExtendedSFRecorder::setEncoderProfile() {

    char value[PROPERTY_VALUE_MAX];
    if (property_get("vendor.encoder.video.profile", value, NULL) <= 0) {
        return;
    }

    AVLOGI("Setting encoder profile : %s", value);

    int32_t profile = mVideoEncoderProfile;
    int32_t level = mVideoEncoderLevel;

    switch (mVideoEncoder) {
        case VIDEO_ENCODER_H264:
            // Set the minimum valid level if the level was undefined;
            // encoder will choose the right level anyways
            level = (level < 0) ? OMX_VIDEO_AVCLevel1 : level;
            if (strncmp("base", value, 4) == 0) {
                profile = OMX_VIDEO_AVCProfileBaseline;
                AVLOGI("H264 Baseline Profile");
            } else if (strncmp("main", value, 4) == 0) {
                profile = OMX_VIDEO_AVCProfileMain;
                AVLOGI("H264 Main Profile");
            } else if (strncmp("high", value, 4) == 0) {
                profile = OMX_VIDEO_AVCProfileHigh;
                AVLOGI("H264 High Profile");
            } else {
                AVLOGW("Unsupported H264 Profile");
            }
            break;
        case VIDEO_ENCODER_MPEG_4_SP:
            level = (level < 0) ? OMX_VIDEO_MPEG4Level0 : level;
            if (strncmp("simple", value, 5) == 0 ) {
                profile = OMX_VIDEO_MPEG4ProfileSimple;
                AVLOGI("MPEG4 Simple profile");
            } else if (strncmp("asp", value, 3) == 0 ) {
                profile = OMX_VIDEO_MPEG4ProfileAdvancedSimple;
                AVLOGI("MPEG4 Advanced Simple Profile");
            } else {
                AVLOGW("Unsupported MPEG4 Profile");
            }
            break;
        default:
            AVLOGW("No custom profile support for other codecs");
            break;
    }
    // Override _both_ profile and level, only if they are valid
    if (profile && level) {
        mVideoEncoderProfile = profile;
        mVideoEncoderLevel = level;
    }
}

status_t ExtendedSFRecorder::handleCustomOutputFormats() {
    status_t status = OK;
    switch (mOutputFormat) {
        case OUTPUT_FORMAT_QCP:
        case OUTPUT_FORMAT_WAVE:
          status = mWriter->start();
          break;

        default:
           status = UNKNOWN_ERROR;
           break;
    }
    return status;
}

status_t ExtendedSFRecorder::handleCustomRecording() {
    status_t status = OK;
    switch (mOutputFormat) {
        case OUTPUT_FORMAT_QCP:
            status = setupExtendedRecording();
            break;
        case OUTPUT_FORMAT_WAVE:
            status = setupWAVERecording();
            break;

        default:
            status = UNKNOWN_ERROR;
            break;
    }
    return status;
}

status_t ExtendedSFRecorder::handleCustomAudioSource(sp<AMessage> format) {
    status_t status = OK;
    switch (mAudioEncoder) {
        case AUDIO_ENCODER_LPCM:
            format->setString("mime", MEDIA_MIMETYPE_AUDIO_RAW);
            break;
        case AUDIO_ENCODER_EVRC:
            format->setString("mime", MEDIA_MIMETYPE_AUDIO_EVRC);
            break;
        case AUDIO_ENCODER_QCELP:
            format->setString("mime", MEDIA_MIMETYPE_AUDIO_QCELP);
            break;
        case AUDIO_ENCODER_MPEGH:
            format->setString("mime", MEDIA_MIMETYPE_AUDIO_MHAS);
            break;
        default:
            status = UNKNOWN_ERROR;
            break;
    }
    return status;
}

status_t ExtendedSFRecorder::handleCustomAudioEncoder() {
    status_t status = OK;
    switch (mAudioEncoder) {
        case AUDIO_ENCODER_LPCM:
        case AUDIO_ENCODER_EVRC:
        case AUDIO_ENCODER_QCELP:
        case AUDIO_ENCODER_MPEGH:
            break;

        default:
            status = UNKNOWN_ERROR;
            break;
    }
    return status;
}

status_t ExtendedSFRecorder::setupWAVERecording() {
    CHECK(mOutputFormat == OUTPUT_FORMAT_WAVE);
    CHECK(mAudioEncoder == AUDIO_ENCODER_LPCM);
    CHECK(mAudioSource != AUDIO_SOURCE_CNT);

    mWriter = new WAVEWriter(mOutputFd);
    return setupRawAudioRecording();
}

status_t ExtendedSFRecorder::setupExtendedRecording() {
    CHECK(mOutputFormat == OUTPUT_FORMAT_QCP);

    if (mSampleRate != 8000) {
        AVLOGE("Invalid sampling rate %d used for recording",
             mSampleRate);
        return BAD_VALUE;
    }
    if (mAudioChannels != 1) {
        AVLOGE("Invalid number of audio channels %d used for recording",
                mAudioChannels);
        return BAD_VALUE;
    }

    if (mAudioSource >= AUDIO_SOURCE_CNT) {
        AVLOGE("Invalid audio source: %d", mAudioSource);
        return BAD_VALUE;
    }

    mWriter = new ExtendedWriter(mOutputFd);
    return setupRawAudioRecording();
}

status_t ExtendedSFRecorder::pause() {
    if (mOutputFormat == OUTPUT_FORMAT_WAVE) {
        if (mWriter == NULL) {
            return UNKNOWN_ERROR;
        }
        status_t err = mWriter->pause();
        if (err != OK) {
            AVLOGE("Writer pause in StagefrightRecorder failed");
            return err;
        }
    }

    return StagefrightRecorder::pause();
}

status_t ExtendedSFRecorder::resume() {
    if (mOutputFormat == OUTPUT_FORMAT_WAVE) {
        if (mWriter == NULL) {
            return UNKNOWN_ERROR;
        }
        status_t err = mWriter->start();
        if (err != OK) {
            AVLOGE("Writer start in StagefrightRecorder failed");
            return err;
        }
    }

    return StagefrightRecorder::resume();
}

sp<MediaSource> ExtendedSFRecorder::setPCMRecording() {
    audio_attributes_t attr = AUDIO_ATTRIBUTES_INITIALIZER;
    attr.source = mAudioSource;

    sp<AudioSource> audioSource =
        new ExtendedAudioSource(
                &attr,
                mAttributionSource,
                mSampleRate,
                mAudioChannels,
                mSampleRate);

    status_t err = audioSource->initCheck();

    if (err != OK) {
        AVLOGE("audio source is not initialized");
        return NULL;
    }
    if (mAudioEncoder == AUDIO_ENCODER_LPCM) {
        AVLOGI("No encoder is needed for linear PCM format");
        return audioSource;

    }
    return NULL;
}

bool ExtendedSFRecorder::isCompressAudioRecordingEligible() {
    bool isCompressCaptureEligible = false;

    audio_mode_t phoneState = AudioSystem::getPhoneState();
    if (phoneState > AUDIO_MODE_NORMAL) {
        AVLOGI("%s: audio mode is :%d; no compress audio recording", __func__,
               phoneState);
        return isCompressCaptureEligible;
    }
    if (mAudioSource == AUDIO_SOURCE_UNPROCESSED) {
        AVLOGI(
            "%s: audio source is AUDIO_SOURCE_UNPROCESSED; no compress audio "
            "recording",
            __func__);
        return isCompressCaptureEligible;
    }

    /**
     * APS will slience the audio record track, if there is
     * no RECORD AUDIO permission. Compress Audio Source
     * can't handle the slience data. Hence the check.
     **/
    const int32_t mode = mAppOpsManager->checkOp(
        AppOpsManager::OP_RECORD_AUDIO, mAttributionSource.uid,
        VALUE_OR_FATAL(aidl2legacy_string_view_String16(
            mAttributionSource.packageName.value_or(""))));
    if (mode == AppOpsManager::MODE_ALLOWED) {
        isCompressCaptureEligible = true;
        AVLOGV(
            "%s: application allowed (uid: %d; package name: %s) for "
            "OP_RECORD_AUDIO (%d)",
            __func__, mAttributionSource.uid,
            aidl2legacy_string_view_String16(
                mAttributionSource.packageName.value_or(""))
                ->string(),
            AppOpsManager::OP_RECORD_AUDIO);
    } else {
        AVLOGI(
            "%s: seems requesting application "
            "(uid: %d; package name: %s) is not allowed "
            "Missing OP_RECORD_AUDIO (%d); Hence no "
            "compress audio recording",
            __func__, mAttributionSource.uid,
            aidl2legacy_string_view_String16(
                mAttributionSource.packageName.value_or(""))
                ->string(),
            AppOpsManager::OP_RECORD_AUDIO);
        return isCompressCaptureEligible;
    }

    return isCompressCaptureEligible;
}

bool ExtendedSFRecorder::isCompressAudioRecordingSupported() {
    status_t status = OK;
    uint32_t numDeviceSourcePorts = 0;
    uint32_t generation = 0;
    bool isCompressCaptureSupported = false;
    audio_format_t audioFormat;

    // choose the right audio format based on encoder
    if (mAudioEncoder == AUDIO_ENCODER_AAC) {
        audioFormat = AUDIO_FORMAT_AAC_LC;
    } else {
        return isCompressCaptureSupported;
    }

    // check whether compress capture is supported or not
    status = AudioSystem::listAudioPorts(
        AUDIO_PORT_ROLE_SOURCE, AUDIO_PORT_TYPE_DEVICE, &numDeviceSourcePorts,
        nullptr, &generation);
    if (status != OK || numDeviceSourcePorts <= 0) {
        AVLOGE(
            "%s: listports of role %zu and type %zu failed or 0 device source "
            "ports available",
            __func__, AUDIO_PORT_ROLE_SOURCE, AUDIO_PORT_TYPE_DEVICE);
        return isCompressCaptureSupported;
    }

    auto deviceSourcePorts =
        std::make_unique<struct audio_port_v7[]>(numDeviceSourcePorts);
    status = AudioSystem::listAudioPorts(
        AUDIO_PORT_ROLE_SOURCE, AUDIO_PORT_TYPE_DEVICE, &numDeviceSourcePorts,
        deviceSourcePorts.get(), &generation);
    if (status != OK || numDeviceSourcePorts <= 0) {
        AVLOGE("%s: listports of role %zu and type %zu is failed", __func__,
               AUDIO_PORT_ROLE_SOURCE, AUDIO_PORT_TYPE_DEVICE);
        return isCompressCaptureSupported;
    }
    AVLOGV("%s: Number of Device Source Ports: %zu", __func__,
           numDeviceSourcePorts);

    // does the built-in mic have support
    auto isBuiltInMicDevicePort = [](struct audio_port_v7 &ap) {
        return ap.ext.device.type == AUDIO_DEVICE_IN_BUILTIN_MIC;
    };
    auto builtInMicDevicePort = std::find_if(
        deviceSourcePorts.get(), deviceSourcePorts.get() + numDeviceSourcePorts,
        isBuiltInMicDevicePort);

    if (builtInMicDevicePort) {
        AVLOGV("%s: got Port with device:%s id:%zu type:%zu role:%zu", __func__,
               audio_device_to_string(builtInMicDevicePort->ext.device.type),
               builtInMicDevicePort->id, builtInMicDevicePort->type,
               builtInMicDevicePort->role);

        auto hasAudioFormat = [&](struct audio_profile &profile) {
            return profile.format == audioFormat;
        };
        auto compressProfile =
            std::find_if(builtInMicDevicePort->audio_profiles,
                         (builtInMicDevicePort->audio_profiles) +
                             (builtInMicDevicePort->num_audio_profiles),
                         hasAudioFormat);
        if (compressProfile && compressProfile->format == audioFormat) {
            AVLOGV("%s: got %s Profile", __func__,
                   audio_format_to_string(compressProfile->format));
            auto sampleRates = compressProfile->sample_rates;
            auto numSampleRates = compressProfile->num_sample_rates;
            auto channelMasks = compressProfile->channel_masks;
            auto numChannelMasks = compressProfile->num_channel_masks;
            if (std::find(sampleRates, sampleRates + numSampleRates,
                          mSampleRate) &&
                std::find(channelMasks, channelMasks + numChannelMasks,
                          audio_channel_in_mask_from_count(mAudioChannels))) {
                AVLOGI(
                    "%s: found source device port, name: %s with %s"
                    " with sample rate %zu and channel count %zu",
                    __func__, builtInMicDevicePort->name,
                    audio_format_to_string(audioFormat), mSampleRate,
                    mAudioChannels);

                // check whether compress capture is allowed or not
                if (property_get_bool("vendor.audio.compress_capture.enabled",
                                      false)) {
                    if ((audioFormat == AUDIO_FORMAT_AAC_LC ||
                         audioFormat == AUDIO_FORMAT_AAC_ADTS_LC ||
                         audioFormat == AUDIO_FORMAT_AAC_ADTS_HE_V1 ||
                         audioFormat == AUDIO_FORMAT_AAC_ADTS_HE_V2) &&
                        property_get_bool("vendor.audio.compress_capture.aac",
                                          false)) {
                        isCompressCaptureSupported =
                            isCompressAudioRecordingEligible();
                    } else {
                        AVLOGI(
                            "%s: compress capture disabled or unavailable for "
                            "%s",
                            __func__, audio_format_to_string(audioFormat));
                    }
                } else {
                    AVLOGI("%s: compress capture feature disabled", __func__);
                }
            }
        }
    }
    return isCompressCaptureSupported;
}

sp<AudioSource> ExtendedSFRecorder::setCompressAudioRecording() {
    audio_attributes_t attr = AUDIO_ATTRIBUTES_INITIALIZER;
    attr.source = mAudioSource;
    audio_format_t audioFormat;

    if (mAudioEncoder == AUDIO_ENCODER_AAC) {
        audioFormat = AUDIO_FORMAT_AAC_LC;
    } else {
        return nullptr;
    }

    sp<CompressAACAudioSource> cAACSource = new CompressAACAudioSource(
        audioFormat, mAudioBitRate, &attr, mAttributionSource, mSampleRate,
        mAudioChannels, mSampleRate);

    status_t err = cAACSource->initCheck();

    if (err != OK) {
        AVLOGE("compress aac audio source init failed");
        cAACSource.clear();
        return nullptr;
    }

    mAudioBitRate = cAACSource->getBitRate();
    mTotalBitRate += mAudioBitRate;
    mEnabledCompressAudioRecording = true;

    return cAACSource;
}
} // namespace android
