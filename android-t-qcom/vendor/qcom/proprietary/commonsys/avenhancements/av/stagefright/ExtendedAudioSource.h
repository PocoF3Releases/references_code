/*
 * Copyright (c) 2015, 2019-2021 Qualcomm Technologies, Inc.
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

#ifndef _EXT_AUDIO_SOURCE_H
#define _EXT_AUDIO_SOURCE_H

#include <stdint.h>
#include <media/stagefright/AudioSource.h>
#include <media/stagefright/MediaBuffer.h>

#include <common/AVConfigHelper.h>

#include <vector>

namespace android {

using content::AttributionSourceState;

struct AudioSource;

typedef void* (*hdr_get_obj_fcn)();

typedef void (*hdr_delete_obj_fcn)(void*);

typedef bool (*hdr_configure_fcn)(
    void*,
    int,
    int);

typedef bool (*hdr_set_param_fcn)(
    void*,
    int,
    int,
    const char*);

typedef void (*hdr_setup_ramp_fcn)(void*);

typedef bool (*hdr_enable_fake_hdr_fcn)(void*);

typedef bool (*hdr_setup_fcn)(void*);

typedef bool (*hdr_process_fcn)(
    void*,
    int8_t*,
    int8_t*);

struct ExtendedAudioSource : public AudioSource {
    ExtendedAudioSource(
            const audio_attributes_t *attr,
            const AttributionSourceState& attributionSource,
            uint32_t sampleRate,
            uint32_t channels,
            uint32_t outSampleRate = 0,
            audio_port_handle_t selectedDeviceId = AUDIO_PORT_HANDLE_NONE,
            audio_microphone_direction_t selectedMicDirection = MIC_DIRECTION_UNSPECIFIED,
            float selectedMicFieldDimension = MIC_FIELD_DIMENSION_NORMAL);
    virtual status_t reset();
    virtual status_t read(
            MediaBufferBase **buffer, const ReadOptions *options = NULL);
    virtual sp<MetaData> getFormat();
    virtual void signalBufferReturned(MediaBufferBase *buffer);
    void onEvent(int event, void* info);

protected:
    virtual ~ExtendedAudioSource();

private:
    bool hdrSymbolLoader_l(void* library_ptr);
    bool setupHdr_l();
    void getConfigParams_l();

    uint32_t mPrevPosition;
    uint32_t mAllocBytes;
    audio_session_t mAudioSessionId;
    AudioRecord::transfer_type mTransferMode;
    AudioRecord::Buffer mTempBuf;
    AVConfigHelper *pConfigsIns;

    bool mIsHdrSrc;
    bool mEnableHdr;
    bool mHdrEnabled;
    bool mEnableWnr;
    bool mEnableAns;
    bool mIsLandscape;
    bool mIsInverted;
    int mFacing; // 0 : None; 1 : Main; 2 : Selfie;

    int mAudioChannelCount;
    int mOutputChannelCount;
    int mAudioSamplingRate;

    std::vector<uint8_t> mStagingBuf;
    size_t mProcessSize;

    void* mHdrLibHandle = nullptr;
    void* mHdrHandle = nullptr;
    hdr_get_obj_fcn mHdrGetObj = nullptr;
    hdr_delete_obj_fcn mHdrDeleteObj = nullptr;
    hdr_configure_fcn mHdrConfig = nullptr;
    hdr_set_param_fcn mHdrSetParam = nullptr;
    hdr_setup_ramp_fcn mHdrSetupRamp = nullptr;
    hdr_enable_fake_hdr_fcn mHdrEnable2ChHdr = nullptr;
    hdr_setup_fcn mHdrSetup = nullptr;
    hdr_process_fcn mHdrProcess = nullptr;
};

}

#endif //_EXT_AUDIO_SOURCE_H
