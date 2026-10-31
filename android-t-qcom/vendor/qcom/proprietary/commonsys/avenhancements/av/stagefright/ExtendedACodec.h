/*
 * Copyright (c) 2015-2019, Qualcomm Technologies, Inc.
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

#ifndef _A_EXT_CODEC_H_
#define _A_EXT_CODEC_H_

#include <stdint.h>
#include <android/native_window.h>
#include <media/IOMX.h>
#include <media/stagefright/AHierarchicalStateMachine.h>
#include <media/stagefright/CodecBase.h>
#include <media/stagefright/SkipCutBuffer.h>
#include <OMX_Audio.h>
#include <media/stagefright/ACodec.h>

namespace android {

struct ExtendedACodec : public ACodec {
    ExtendedACodec();

protected:
    virtual ~ExtendedACodec();

    virtual void initiateAllocateComponent(const sp<AMessage> &msg);

    virtual status_t configureCodec(const char *mime, const sp<AMessage> &msg);

    virtual status_t setupVideoDecoder(
             const char *mime, const sp<AMessage> &msg, bool usingNativeBuffers,
             bool haveSwRenderer, sp<AMessage> &outputformat);

    virtual status_t setupVideoEncoder(
             const char *mime, const sp<AMessage> &msg,
             sp<AMessage> &outputformat, sp<AMessage> &inputformat);

    virtual status_t setParameters(const sp<AMessage> &msg);

    virtual status_t setupCustomCodec(
            status_t inputErr, const char *mime, const sp<AMessage> &msg);

    virtual status_t GetVideoCodingTypeFromMime(
            const char *mime, OMX_VIDEO_CODINGTYPE *codingType);

    virtual status_t getPortFormat(OMX_U32 portIndex, sp<AMessage> &notify);

    virtual void setBFrames(OMX_VIDEO_PARAM_MPEG4TYPE *mpeg4type);

    status_t setupTMEEncoderParameters(const sp<AMessage> &msg,
            sp<AMessage> &outputFormat);

    virtual status_t setupErrorCorrectionParameters();

    virtual bool getDSModeHint(const sp<AMessage>& msg, int64_t timeUs);

    virtual status_t allocateBuffersOnPort(OMX_U32 portIndex);

    virtual status_t freeBuffersOnPort(OMX_U32 portIndex);

    virtual status_t submitOutputMetadataBuffer();
private:
    /* B-Frames are disabled due to power and performance issues.
     * NOTE: This disables B-Frames for all usecases via MediaCodec. */
    static const int32_t kNumBFramesPerPFrame;

    status_t setupOmxPortConfig(OMX_U32 portIndex, OMX_AUDIO_CODINGTYPE OMX_AUDIO_Coding);

    status_t configureFramePackingFormat(const sp<AMessage> &msg);

    status_t setDIVXFormat(const sp<AMessage> &msg, const char* mime);

    status_t setMpeghParameters(const sp<AMessage> &msg);

    status_t getMpeghPortFormat(OMX_U32 portIndex, sp<AMessage> &notify);

    status_t setEVRCFormat(int32_t numChannels, int32_t sampleRate);

    status_t setMPEGHFormat(int32_t numChannels, int32_t sampleRate, const sp<AMessage> &msg);

    status_t setQCELPFormat(int32_t numChannels, int32_t sampleRate);

    status_t setAMRWBPLUSFormat(int32_t numChannels, int32_t sampleRate);

    status_t setWMAFormat(const sp<AMessage> &meta);

    status_t setFLACDecoderFormat(int32_t numChannels,
            int32_t sampleRate, int32_t bitsPerSample,
            int32_t minBlkSize, int32_t maxBlkSize,
            int32_t minFrmSize, int32_t maxFrmSize);

    status_t setALACFormat(int32_t numChannels, int32_t sampleRate, int32_t bitsPerSample);

    status_t setAPEFormat(int32_t numChannels, int32_t sampleRate, int32_t bitsPerSample);

    status_t setDSDFormat(int32_t numChannels, int32_t sampleRate, int32_t bitsPerSample);

    status_t setupVQZIP(const sp<AMessage> &msg);

    status_t reallocateComponent(const char *mime, uint32_t flags=0);
    bool isMPEG4DP(const sp<AMessage> &msg);
    bool isCodecConfigUnsupported(const sp<AMessage> &msg);
    bool checkDPFromCodecSpecificData(const uint8_t *data, size_t size);
    bool checkDPFromVOLHeader(const uint8_t *data, size_t size);

    status_t allocateBuffersOnExtradataPort(OMX_U32 portIndex);

    status_t freeBuffersOnExtradataPort(OMX_U32 portIndex);
private:
    bool mComponentAllocByName;
    bool mEncoderComponent;
    bool mVideoDSModeSupport;

    status_t setDitherControl(const sp<AMessage> &msg);

    typedef struct {
        bool   mEnable;
        size_t mBufferSize;
        size_t mNumBuffers;
    } ExtradataInfo;

    ExtradataInfo mInputExtradataInfo;
    ExtradataInfo mOutputExtradataInfo;
    bool mIsOmxVppAlloc;

    status_t setVppVendorExtension(const sp<AMessage> &msg);
    void setupOmxVpp(const sp<AMessage> &msg);
    status_t setOutputFrameRateVendorExtension(const sp<AMessage> &msg);
};

}

#endif //_A_EXT_CODEC_H_
