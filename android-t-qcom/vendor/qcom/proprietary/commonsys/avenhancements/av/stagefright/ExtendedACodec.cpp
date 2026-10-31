/*
 * Copyright (c) 2015-2020,2022 Qualcomm Technologies, Inc.
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
#define LOG_TAG "ExtendedACodec"

#ifdef __LP64__
#define OMX_ANDROID_COMPILE_AS_32BIT_ON_64BIT_PLATFORMS
#endif

#include <common/AVLog.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AUtils.h>
#include <media/stagefright/foundation/ABitReader.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaCodecList.h>
#include <gui/Surface.h>
#include <media/OMXBuffer.h>

#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include "include/SharedMemoryBuffer.h"

#include <hidlmemory/mapping.h>
#include <binder/MemoryDealer.h>

#include <media/stagefright/MetaData.h>
#ifndef BRINGUP_WIP
#include <media/stagefright/OMXCodec.h>
#endif

#include <media/hardware/HardwareAPI.h>

#include <QCMetaData.h>

#include <stagefright/AVExtensions.h>
#include <stagefright/ExtendedACodec.h>
#include <stagefright/ExtendedUtils.h>
#include <cutils/properties.h>

#include <OMX_Component.h>
#include <OMX_VideoExt.h>
#include <OMX_IndexExt.h>
#include <QOMX_AudioExtensions.h>
#include <QOMX_AudioIndexExtensions.h>
#include <OMX_QCOMExtns.h>
#include <OMX_Types.h>
#include <cutils/ashmem.h>

namespace android {

#ifndef OMX_QTI_INDEX_PARAM_VIDEO_PREFER_ADAPTIVE_PLAYBACK
#define OMX_QTI_INDEX_PARAM_VIDEO_PREFER_ADAPTIVE_PLAYBACK "OMX.QTI.index.param.video.PreferAdaptivePlayback"
#endif

#define MIN_NATIVE_WINDOW_UNDEQUEUED_BUFFERS 4 // 1 (minUndequeuedBuffers) + 3 (Extra buffers as per
                                               // AOSP to reduce starvation from the consumer)

#define OMX_AUDIO_PCMModeVendorHoa 0x7F000001 // OMX vendor extensions are not available
                                              // in system modules

const int32_t ExtendedACodec::kNumBFramesPerPFrame = 1;

// checks and converts status_t to a non-side-effect status_t
static inline status_t makeNoSideEffectStatus(status_t err) {
    switch (err) {
    // the following errors have side effects and may come
    // from other code modules. Remap for safety reasons.
    case INVALID_OPERATION:
    case DEAD_OBJECT:
        return UNKNOWN_ERROR;
    default:
        return err;
    }
}

template<class T>
static void InitOMXParams(T *params) {
    memset(params, 0, sizeof(T));
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

status_t ExtendedACodec::setMpeghParameters(const sp<AMessage> &msg) {
    status_t err;

    // Binaural
    int32_t binaural;
    if (msg->findInt32("binaural-mode", &binaural)) {
        QOMX_AUDIO_PARAM_BINAURAL_TYPE in_binaural;
        InitOMXParams(&in_binaural);
        in_binaural.nPortIndex = kPortIndexInput;
        in_binaural.eBinauralMode = binaural ? QOMX_BINAURAL_MODE_ON : QOMX_BINAURAL_MODE_OFF;

        err = mOMXNode->setParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioBinauralMode,
                &in_binaural, sizeof(in_binaural));
        if (err != OK) {
            AVLOGE("setMpeghParameters Binaural mode failed");
            return err;
        }
    }

    // Channel Mask
    int32_t channel_mask;
    if (msg->findInt32("channel-mask", &channel_mask)) {
        QOMX_AUDIO_PARAM_CHANNEL_CONFIG_TYPE in_channel;
        InitOMXParams(&in_channel);
        in_channel.nPortIndex = kPortIndexInput;
        in_channel.nChannelMask = channel_mask;

        err = mOMXNode->setParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioChannelMask,
                &in_channel, sizeof(in_channel));
        if (err != OK) {
            AVLOGE("setMpeghParameters Channel Mask failed");
            return err;
        }
    }

    // Rotation
    int32_t qx, qy, qz, qw;
    if (msg->findInt32("rotation-qx", &qx) &&
        msg->findInt32("rotation-qy", &qy) &&
        msg->findInt32("rotation-qz", &qz) &&
        msg->findInt32("rotation-qw", &qw)) {

        QOMX_AUDIO_PARAM_ROTATION_TYPE in_rotation;
        InitOMXParams(&in_rotation);
        in_rotation.nPortIndex = kPortIndexInput;
        in_rotation.nQX = qx;
        in_rotation.nQY = qy;
        in_rotation.nQZ = qz;
        in_rotation.nQW = qw;

        err = mOMXNode->setParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioRotation,
                &in_rotation, sizeof(in_rotation));
        if (err != OK) {
            AVLOGE("setMpeghParameters Rotation failed");
            return err;
        }
    }

    int32_t hoaMode = 0;
    if (msg->findInt32("enable-hoa", (int32_t *)&hoaMode)) {
        AVLOGV("setMpeghParameters hoaMode set to %d", hoaMode);
        if (hoaMode != 0) {
            // profile
            OMX_AUDIO_PARAM_PCMMODETYPE out_profile;
            InitOMXParams(&out_profile);
            out_profile.nPortIndex = kPortIndexOutput;
            int err = mOMXNode->getParameter(OMX_IndexParamAudioPcm,
            &out_profile, sizeof(out_profile));
            if (OK != err) {
                AVLOGE("returning error %d at %d", err, __LINE__);
            }
            out_profile.ePCMMode = (OMX_AUDIO_PCMMODETYPE)OMX_AUDIO_PCMModeVendorHoa;
            err = mOMXNode->setParameter(OMX_IndexParamAudioPcm,
            &out_profile, sizeof(out_profile));
            if (OK != err) {
                AVLOGE("returning error %d at %d", err, __LINE__);
                return err;
            }
        }
    }

    return OK;
}

status_t ExtendedACodec::getMpeghPortFormat(OMX_U32 portIndex, sp<AMessage> &notify) {
    QOMX_AUDIO_PARAM_MPEGH_TYPE params;
    InitOMXParams(&params);
    params.nPortIndex = portIndex;
    CHECK_EQ(mOMXNode->getParameter(
            (OMX_INDEXTYPE)QOMX_IndexParamAudioMpegh,
            &params, sizeof(params)),
            (status_t)OK);
    notify->setString("mime", MEDIA_MIMETYPE_AUDIO_MHAS);
    /* MPEGH supports only 48k sample rate*/
    notify->setInt32("sample-rate", 48000);
    notify->setInt32("channel-count", 4);
    return OK;
}

ExtendedACodec::ExtendedACodec()
    : mComponentAllocByName(false),
      mEncoderComponent(false),
      mVideoDSModeSupport(true),
      mIsOmxVppAlloc(false) {
    updateLogLevel();
    AVLOGV("ExtendedACodec()");

    memset( &mInputExtradataInfo , 0, sizeof(ExtradataInfo));
    memset( &mOutputExtradataInfo , 0, sizeof(ExtradataInfo));
}

ExtendedACodec::~ExtendedACodec() {
    AVLOGV("~ExtendedACodec()");
}

void ExtendedACodec::initiateAllocateComponent(const sp<AMessage> &msg) {
    int32_t nameIsType;
    if (msg->findInt32("nameIsType", &nameIsType) && nameIsType) {
        mComponentAllocByName = false;
    } else {
        mComponentAllocByName = true;
    }
    ACodec::initiateAllocateComponent(msg);
}

status_t ExtendedACodec::configureCodec(const char *mime, const sp<AMessage> &msg) {
    AVLOGV("configureCodec()");

    int32_t encoder = 0;
    if (msg->findInt32("encoder", &encoder) && encoder) {
        mEncoderComponent = encoder;
    }

    setupOmxVpp(msg);

    int32_t preferAdaptive = 0;
    status_t err = OK;

    err = setOutputFrameRateVendorExtension(msg);
    if (err != OK) {
        AVLOGI("Failed to set vendor ext output frame-rate %d", err);
        err = OK;
    }

    if (!strcasecmp(MEDIA_MIMETYPE_VIDEO_TME, mime)) {
        //TME does not need a bitrate, but uses encoder path
        //Set a dummy bitrate so to avoid encoder session rejection
        msg->setInt32("bitrate", 1);
    }

    int32_t priority = 0;
    if (msg->findInt32("priority", &priority)) {
        setPriority(priority);
    }

    if (msg->findInt32("prefer-adaptive-playback", &preferAdaptive)
            && preferAdaptive == 1) {
        AVLOGI("[%s] Adaptive playback preferred", mComponentName.c_str());

        do {
            QOMX_ENABLETYPE enableType;
            OMX_INDEXTYPE indexType;
            InitOMXParams(&enableType);

            err = mOMXNode->getExtensionIndex(
                    OMX_QTI_INDEX_PARAM_VIDEO_PREFER_ADAPTIVE_PLAYBACK,
                    &indexType);
            if (err != OK) {
                AVLOGW("Failed to get extension for adaptive playback");
                break;
            }

            enableType.bEnable = OMX_TRUE;
            err = mOMXNode->setParameter(indexType,
                    (void *)&enableType, sizeof(enableType));
            if (err != OK) {
                AVLOGW("Failed to set adaptive playback");
            }
            break;
        } while(1);
    }

#ifdef OMX_QTI_INDEX_PARAM_NATIVE_RECORDER
    int32_t isNativeRecorder = 0;
    if (mEncoderComponent && msg->findInt32("isNativeRecorder", &isNativeRecorder)
            && isNativeRecorder) {
        do {
            QOMX_ENABLETYPE enableType;
            OMX_INDEXTYPE indexType;
            InitOMXParams(&enableType);

            status_t err = mOMXNode->getExtensionIndex(
                    OMX_QTI_INDEX_PARAM_NATIVE_RECORDER,
                    &indexType);
            if (err != OK) {
                AVLOGW("Failed to get extension for native recorder mode setting");
                break;
            }

            enableType.bEnable = OMX_TRUE;
            err = mOMXNode->setParameter(indexType,
                    (void *)&enableType, sizeof(enableType));
            if (err != OK) {
                AVLOGW("Failed to set native recorder mode");
            }
            break;
        } while(0);
    }
#endif // OMX_QTI_INDEX_PARAM_NATIVE_RECORDER

    bool isDP = isMPEG4DP(msg);
    bool isConfigUnsupported = isCodecConfigUnsupported(msg);

    bool isQCComponent = mComponentName.startsWith("OMX.qcom.") ||
        mComponentName.startsWith("OMX.ittiam.") ||
        mComponentName.startsWith("OMX.qti.");

    uint32_t flags = 0;

    if (!((isDP || isConfigUnsupported) && isQCComponent)) {
        err = ACodec::configureCodec(mime, msg);
    } else {
#ifdef _ANDROID_O_MR1_CHANGES
        if (isDP) {
            flags |= MediaCodecList::kPreferSoftwareCodecs;
        }
#else
        mime = MEDIA_MIMETYPE_VIDEO_MPEG4_DP;
#endif
    }
    bool needRealloc = isQCComponent && ((isDP || isConfigUnsupported) || (err != OK &&
                            !mComponentAllocByName && !mEncoderComponent &&
                            (mComponentName.find("encoder") < 0) &&
                            !strncmp(mime, "video/", strlen("video/"))));
    if (needRealloc) {
        err = reallocateComponent(mime, flags);
        if (err != OK) {
            AVLOGE("Failed to reallocate component");
            signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err));
            return err;
        }

        err = ACodec::configureCodec(mime, msg);
        if (err != OK) {
            AVLOGE("[%s] configureCodec returning error %d",
                  mComponentName.c_str(), err);
            signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err));
            return err;
        }
    }

    if (err != OK) {
        AVLOGE("[%s] configureCodec returning error %d",
                  mComponentName.c_str(), err);
        signalError(OMX_ErrorUndefined, makeNoSideEffectStatus(err));
        return err;
    }

    int32_t featureNalLengthBitstream = 0;
    msg->findInt32("feature-nal-length-bitstream", &featureNalLengthBitstream);

    if (featureNalLengthBitstream && mEncoderComponent) {
        int32_t nalLengthInBytes = 0;
        msg->findInt32("nal-length-in-bytes", &nalLengthInBytes);

        OMX_VIDEO_CONFIG_NALSIZE nalSize;
        InitOMXParams(&nalSize);
        nalSize.nPortIndex = kPortIndexOutput;
        nalSize.nNaluBytes = nalLengthInBytes;

        status_t err = mOMXNode->setConfig(OMX_IndexConfigVideoNalSize,
                (void *)&nalSize, sizeof(nalSize));
        if (err == OK && nalLengthInBytes != 0) {
            mOutputFormat->setInt32("feature-nal-length-bitstream", 1);
        }
    }

#ifdef OMX_QTI_INDEX_PARAM_VIDEO_CLIENT_EXTRADATA
    do {
        QOMX_EXTRADATA_ENABLE enableType;
        OMX_INDEXTYPE indexType;
        InitOMXParams(&enableType);

        status_t err = mOMXNode->getExtensionIndex(OMX_QTI_INDEX_PARAM_VIDEO_CLIENT_EXTRADATA,
                &indexType);
        if (err != OK) {
            AVLOGW("Failed to get extension for extradata parameter");
            break;
        }

        enableType.nPortIndex = kPortIndexOutputExtradata;
        err = mOMXNode->getParameter(indexType,
                        (void *)&enableType, sizeof(enableType));
        if (err != OK) {
            AVLOGW("getParameter for extradata failed");
            break;
        }

        mOutputExtradataInfo.mEnable = (enableType.bEnable == OMX_TRUE);

        err = mOMXNode->setParameter(indexType,
                        (void *)&enableType, sizeof(enableType));
        if (err != OK) {
            AVLOGW("setParameter for extradata failed");
            break;
        }
    } while(0);
#endif

    return err;
}

status_t ExtendedACodec::reallocateComponent(const char *mime, uint32_t flags) {
    AVLOGV("reallocateComponent()");
    Vector<AString> matchingCodecs;
//  Vector<OMXCodec::CodecNameAndQuirks> matchingCodecs;
    AString componentName;
    status_t err = UNKNOWN_ERROR;
    MediaCodecList::findMatchingCodecs(
        mime,
        false, // createEncoder
        flags,     // flags
        &matchingCodecs);

    sp<IOMXNode> altNode;
    for (size_t matchIndex = 0; matchIndex < matchingCodecs.size();
            ++matchIndex) {
        componentName = matchingCodecs[matchIndex];
        if (!strcmp(mComponentName.c_str(), componentName.c_str())) {
            continue;
        }

        sp<IOMXObserver> observer = createObserver();
        err = mOMX->allocateNode(componentName.c_str(), observer, &altNode);

        if (err == OK) {
            break;
        } else {
            AVLOGW("Allocating component '%s' failed, try next one.", componentName.c_str());
        }
    }

    if (altNode == NULL) {
        AVLOGE("Unable to replace %s of type '%s'", mComponentName.c_str(), mime);
    } else {
        status_t err = mOMXNode->freeNode();
        if (err != OK) {
            AVLOGE("Failed to freeNode");
            //mNode is deleted already..no point in returning
            //return err;
        }
        mOMXNode = altNode;
        AVLOGI("Reallocated %s in place of %s", componentName.c_str(), mComponentName.c_str());
        mComponentName = componentName;
        mNodeGeneration++;
    }

    return err;
}
status_t ExtendedACodec::allocateBuffersOnExtradataPort(OMX_U32 portIndex) {
    CHECK(portIndex == kPortIndexInputExtradata || portIndex == kPortIndexOutputExtradata);

     AVLOGV("allocateBuffersOnExtradataPort portIndex - %d", portIndex);

    status_t err = OK;
    ExtradataInfo *extraData = (portIndex == kPortIndexInputExtradata ? &mInputExtradataInfo : &mOutputExtradataInfo);

    do {
        if (extraData->mEnable) {
            OMX_PARAM_PORTDEFINITIONTYPE def;
            InitOMXParams(&def);
            def.nPortIndex = portIndex;

            err = mOMXNode->getParameter(
                    OMX_IndexParamPortDefinition, &def, sizeof(def));

            if (err != OK) {
                AVLOGE("getParameter for extradata port definition is failed");
                break;
            }

            def.nBufferCountActual = def.nBufferCountMin;
            def.eDir = (portIndex == kPortIndexOutputExtradata ? OMX_DirOutput : OMX_DirInput);

            err = mOMXNode->setParameter(
                    OMX_IndexParamPortDefinition, &def, sizeof(def));

            if (err != OK) {
                AVLOGE("setParameter for extradata port definition is failed");
                break;
            }

            extraData->mBufferSize = def.nBufferSize;
            extraData->mNumBuffers = def.nBufferCountActual;


            mAllocator[portIndex] = TAllocator::getService("ashmem");
            if (mAllocator[portIndex] == nullptr) {
                ALOGE("hidl allocator on port %d is null",
                        (int)portIndex);
                err = NO_MEMORY;
                break;
            }

            for( OMX_U32 i = 0; i < def.nBufferCountActual; i++) {
                BufferInfo info;
                info.mStatus = BufferInfo::OWNED_BY_US;
                info.mFenceFd = -1;
                info.mRenderInfo = NULL;
                info.mGraphicBuffer = NULL;
                info.mNewGraphicBuffer = false;

                hidl_memory hidlMemToken;
                sp<TMemory> hidlMem;

                bool success;
                auto transStatus = mAllocator[portIndex]->allocate(
                            def.nBufferSize,
                            [&success, &hidlMemToken](
                                    bool s,
                                    hidl_memory const& m) {
                                success = s;
                                hidlMemToken = m;
                            });

                if (!transStatus.isOk()) {
                    AVLOGE("hidl's AshmemAllocator failed at the "
                            "transport: %s", transStatus.description().c_str());
                    err = NO_MEMORY;
                    break;
                }

                if (!success) {
                    err = NO_MEMORY;
                    break;
                }

                hidlMem = mapMemory(hidlMemToken);

                if (hidlMem == NULL) {
                    err = NO_MEMORY;
                    break;
                }

                err = mOMXNode->useBuffer(
                            portIndex, hidlMemToken, &info.mBufferID);
                if (err != OK) {
                     AVLOGE("Unable to allocate ExtraData Buffer");
                     break;
                }

                info.mCodecData = new SharedMemoryBuffer(
                            NULL, hidlMem);
                info.mCodecRef = hidlMem;

                mBuffers[portIndex].push(info);
            }
        }
    } while(0);

     if (err != OK) {
        extraData->mEnable = false;
    }

    return OK;
}

status_t ExtendedACodec::freeBuffersOnExtradataPort(OMX_U32 portIndex) {
    CHECK(portIndex == kPortIndexInputExtradata || portIndex == kPortIndexOutputExtradata);

    AVLOGV("freeBuffersOnExtradataPort portIndex - %d", portIndex);
    status_t err = OK;
    for (size_t i = mBuffers[portIndex].size(); i > 0;) {
        i--;
        status_t err2 = freeBuffer(portIndex, i);
        if (err == OK) {
            err = err2;
        }
    }

    if (mAllocator[portIndex] != NULL) {
        mAllocator[portIndex].clear();
    }

    return err;
}

status_t ExtendedACodec::allocateBuffersOnPort(OMX_U32 portIndex) {
    if (portIndex == kPortIndexInputExtradata || portIndex == kPortIndexOutputExtradata) {
        return allocateBuffersOnExtradataPort(portIndex);
    } else {
        return ACodec::allocateBuffersOnPort(portIndex);
    }
}

status_t ExtendedACodec::freeBuffersOnPort(OMX_U32 portIndex) {
    if (portIndex == kPortIndexInputExtradata || portIndex == kPortIndexOutputExtradata) {
        return freeBuffersOnExtradataPort(portIndex);
    } else {
        return ACodec::freeBuffersOnPort(portIndex);
    }
}

bool ExtendedACodec::isMPEG4DP(const sp<AMessage> &msg) {
    bool isDP = false;
    sp<ABuffer> csd0;
    if (!strncmp(mComponentName.c_str(), "OMX.qcom.video.decoder.mpeg4",
                 strlen("OMX.qcom.video.decoder.mpeg4"))
                 && msg->findBuffer("csd-0", &csd0)) {
        isDP = checkDPFromCodecSpecificData((const uint8_t*)csd0->data(),
                                                                csd0->size());
    }
    return isDP;
}

bool ExtendedACodec::isCodecConfigUnsupported(const sp<AMessage> &msg) {
    bool isUnsupported = false;
    int32_t pcm_encoding = kAudioEncodingPcm16bit;
    const char * component = "OMX.qti.audio.decoder.flac";
    if (!strncmp(mComponentName.c_str(), component, strlen(component))
                 && msg->findInt32("pcm-encoding", &pcm_encoding)) {
        isUnsupported = (pcm_encoding== kAudioEncodingPcmFloat);
    }
    return isUnsupported;
}

bool ExtendedACodec::checkDPFromCodecSpecificData(const uint8_t *data, size_t size) {
    bool retVal = false;
    size_t offset = 0, startCodeOffset = 0;
    bool isStartCode = false;
    const int kVolStartCode = 0x20;
    const char kStartCode[] = "\x00\x00\x01";
    // must contain at least 4 bytes for video_object_layer_start_code
    const size_t kMinCsdSize = 4;

    if (!data || (size < kMinCsdSize)) {
        AVLOGV("Invalid CSD (expected at least %zu bytes)", kMinCsdSize);
        return retVal;
    }

    while (offset < size - 3) {
        if ((data[offset + 3] & 0xf0) == kVolStartCode) {
            if (!memcmp(&data[offset], kStartCode, 3)) {
                startCodeOffset = offset;
                isStartCode = true;
                break;
            }
        }

        offset++;
    }

    if (isStartCode) {
        retVal = checkDPFromVOLHeader((const uint8_t*) &data[startCodeOffset],
                (size - startCodeOffset));
    }

    return retVal;
}

#define GET_BITS(n, val) \
    do { \
        if (br.numBitsLeft() < n) \
            return false; \
        val = br.getBits(n); \
    } while (0)

#define SKIP_BITS(n) \
    do { \
        if (!br.skipBits(n)) \
            return false; \
    } while (0)

bool ExtendedACodec::checkDPFromVOLHeader(const uint8_t *data, size_t size) {
    bool retVal = false;
    // must contain at least 4 bytes for video_object_layer_start_code + 9 bits of data
    const size_t kMinHeaderSize = 6;
    uint32_t val;

    if (!data || (size < kMinHeaderSize)) {
        AVLOGV("Invalid VOL header (expected at least %zu bytes)", kMinHeaderSize);
        return false;
    }

    AVLOGV("Checking for MPEG4 DP bit");
    ABitReader br(&data[4], (size - 4));
    SKIP_BITS(1); // random_accessible_vol

    unsigned videoObjectTypeIndication;
    GET_BITS(8, videoObjectTypeIndication);
    if (videoObjectTypeIndication == 0x12u) {
        AVLOGW("checkDPFromVOLHeader: videoObjectTypeIndication:%u",
               videoObjectTypeIndication);
        return false;
    }

    unsigned videoObjectLayerVerid = 1;
    GET_BITS(1, val);
    if (val) {
        GET_BITS(4, videoObjectLayerVerid);
        SKIP_BITS(3); // video_object_layer_priority
        AVLOGV("checkDPFromVOLHeader: videoObjectLayerVerid:%u",
               videoObjectLayerVerid);
    }

    GET_BITS(4, val);
    if (val == 0x0f) { // aspect_ratio_info
        AVLOGV("checkDPFromVOLHeader: extended PAR");
        SKIP_BITS(8); // par_width
        SKIP_BITS(8); // par_height
    }

    GET_BITS(1, val);
    if (val) { // vol_control_parameters
        SKIP_BITS(2);  // chroma_format
        SKIP_BITS(1);  // low_delay
        GET_BITS(1, val);
        if (val) { // vbv_parameters
            SKIP_BITS(15); // first_half_bit_rate
            SKIP_BITS(1);  // marker_bit
            SKIP_BITS(15); // latter_half_bit_rate
            SKIP_BITS(1);  // marker_bit
            SKIP_BITS(15); // first_half_vbv_buffer_size
            SKIP_BITS(1);  // marker_bit
            SKIP_BITS(3);  // latter_half_vbv_buffer_size
            SKIP_BITS(11); // first_half_vbv_occupancy
            SKIP_BITS(1);  // marker_bit
            SKIP_BITS(15); // latter_half_vbv_occupancy
            SKIP_BITS(1);  // marker_bit
        }
    }

    unsigned videoObjectLayerShape;
    GET_BITS(2, videoObjectLayerShape);
    if (videoObjectLayerShape != 0x00u /* rectangular */) {
        AVLOGV("checkDPFromVOLHeader: videoObjectLayerShape:%x",
               videoObjectLayerShape);
        return false;
    }

    SKIP_BITS(1); // marker_bit
    unsigned vopTimeIncrementResolution;
    GET_BITS(16, vopTimeIncrementResolution);
    SKIP_BITS(1); // marker_bit
    GET_BITS(1, val);
    if (val) {  // fixed_vop_rate
        // range [0..vopTimeIncrementResolution)

        // vopTimeIncrementResolution
        // 2 => 0..1, 1 bit
        // 3 => 0..2, 2 bits
        // 4 => 0..3, 2 bits
        // 5 => 0..4, 3 bits
        // ...

        if (vopTimeIncrementResolution <= 0u) {
            return BAD_VALUE;
        }

        if (vopTimeIncrementResolution != 1) {
            --vopTimeIncrementResolution;
        }

        unsigned numBits = 0;
        while (vopTimeIncrementResolution > 0) {
            ++numBits;
            vopTimeIncrementResolution >>= 1;
        }

        SKIP_BITS(numBits);  // fixed_vop_time_increment
    }

    SKIP_BITS(1);  // marker_bit
    SKIP_BITS(13); // video_object_layer_width
    SKIP_BITS(1);  // marker_bit
    SKIP_BITS(13); // video_object_layer_height
    SKIP_BITS(1);  // marker_bit
    SKIP_BITS(1);  // interlaced
    SKIP_BITS(1);  // obmc_disable
    unsigned spriteEnable = 0;
    if (videoObjectLayerVerid == 1) {
        GET_BITS(1, spriteEnable);
    } else {
        GET_BITS(2, spriteEnable);
    }

    if (spriteEnable == 0x1) { // static
        int spriteWidth;
        GET_BITS(13, spriteWidth);
        AVLOGV("checkDPFromVOLHeader: spriteWidth:%d", spriteWidth);
        SKIP_BITS(1) ; // marker_bit
        SKIP_BITS(13); // sprite_height
        SKIP_BITS(1);  // marker_bit
        SKIP_BITS(13); // sprite_left_coordinate
        SKIP_BITS(1);  // marker_bit
        SKIP_BITS(13); // sprite_top_coordinate
        SKIP_BITS(1);  // marker_bit
        SKIP_BITS(6);  // no_of_sprite_warping_points
        SKIP_BITS(2);  // sprite_warping_accuracy
        SKIP_BITS(1);  // sprite_brightness_change
        SKIP_BITS(1);  // low_latency_sprite_enable
    } else if (spriteEnable == 0x2) { // GMC
        SKIP_BITS(6); // no_of_sprite_warping_points
        SKIP_BITS(2); // sprite_warping_accuracy
        SKIP_BITS(1); // sprite_brightness_change
    }

    if (videoObjectLayerVerid != 1
            && videoObjectLayerShape != 0x0u) {
        SKIP_BITS(1);
    }

    GET_BITS(1, val);
    if (val) { // not_8_bit
        SKIP_BITS(4);  // quant_precision
        SKIP_BITS(4);  // bits_per_pixel
    }

    if (videoObjectLayerShape == 0x3) {
        SKIP_BITS(1);
        SKIP_BITS(1);
        SKIP_BITS(1);
    }

    GET_BITS(1, val);
    if (val) { // quant_type
        GET_BITS(1, val);
        if (val) { // load_intra_quant_mat
            unsigned IntraQuantMat = 1;
            for (int i = 0; i < 64 && IntraQuantMat; i++) {
                GET_BITS(8, IntraQuantMat);
            }
        }

        GET_BITS(1, val);
        if (val) { // load_non_intra_quant_matrix
            unsigned NonIntraQuantMat = 1;
            for (int i = 0; i < 64 && NonIntraQuantMat; i++) {
                GET_BITS(8, NonIntraQuantMat);
            }
        }
    } /* quantType */

    if (videoObjectLayerVerid != 1) {
        unsigned quarterSample;
        GET_BITS(1, quarterSample);
        AVLOGV("checkDPFromVOLHeader: quarterSample:%u",
                quarterSample);
    }

    SKIP_BITS(1); // complexity_estimation_disable
    SKIP_BITS(1); // resync_marker_disable
    unsigned dataPartitioned;
    GET_BITS(1, dataPartitioned);
    if (dataPartitioned) {
        retVal = true;
    }

    AVLOGD("checkDPFromVOLHeader: DP:%u", dataPartitioned);
    return retVal;
}
#undef GET_BITS
#undef SKIP_BITS

status_t ExtendedACodec::setupVideoDecoder(
        const char *mime, const sp<AMessage> &msg, bool usingNativeBuffers,
        bool usingSwRenderer, sp<AMessage> &outputFormat) {
    AVLOGI("setupVideoDecoder()");
    status_t err = OK;
    bool isQCComponent = mComponentName.startsWith("OMX.qcom.") ||
            mComponentName.startsWith("OMX.ittiam.") ||
            mComponentName.startsWith("OMX.qti.");
    int32_t frameRateInt;
    float frameRateFloat;
    if (!msg->findFloat("frame-rate", &frameRateFloat)) {
        if (!msg->findInt32("frame-rate", &frameRateInt)) {
            frameRateInt = -1;
        }
        frameRateFloat = (float)frameRateInt;
    }
    if (frameRateFloat != -1) {
        OMX_VENDOR_VIDEOFRAMERATE data;
        InitOMXParams(&data);
        data.nPortIndex = kPortIndexInput;
        data.nFps = (OMX_U32)(frameRateFloat * 65536.0f);
        data.bEnabled = OMX_TRUE;

        err = mOMXNode->setConfig((OMX_INDEXTYPE)OMX_IndexVendorVideoFrameRate,
                            (void*)&data, sizeof(OMX_VENDOR_VIDEOFRAMERATE));
    }

    // Enable Sync-frame decode mode for thumbnails
    int32_t thumbnailMode = 0;
    if (msg->findInt32("thumbnail-mode", &thumbnailMode) &&
        thumbnailMode) {
        AVLOGD("Enabling thumbnail mode.");
        QOMX_ENABLETYPE enableType;
        OMX_INDEXTYPE indexType;
        InitOMXParams(&enableType);

        status_t err = mOMXNode->getExtensionIndex(
                OMX_QCOM_INDEX_PARAM_VIDEO_SYNCFRAMEDECODINGMODE,
                &indexType);
        if (err != OK) {
            AVLOGW("Failed to get extension for SYNCFRAMEDECODINGMODE");
            return err;
        }

        enableType.bEnable = OMX_TRUE;
        err = mOMXNode->setParameter(indexType,
                   (void *)&enableType, sizeof(enableType));
        if (err != OK) {
            AVLOGW("Failed to set extension for SYNCFRAMEDECODINGMODE");
            return err;
        }
        AVLOGI("Thumbnail mode enabled.");
    }

    err = ACodec::setupVideoDecoder(mime, msg, usingNativeBuffers,
        usingSwRenderer, outputFormat);

    if (err != OK || !isQCComponent) {
        return err;
    }

    configureFramePackingFormat(msg);
    setDIVXFormat(msg, mime);

    AString fileFormat;
    const char *fileFormatCStr = NULL;
    bool success = msg->findString(ExtendedUtils::getMsgFromKey(kKeyFileFormat), &fileFormat);
    if (success) {
        fileFormatCStr = fileFormat.c_str();
    }

    // Enable timestamp reordering for mpeg4 and vc1 codec types, the AVI file
    // type, and hevc content in the ts container
    bool tsReorder = false;
    const char* roleVC1 = "OMX.qcom.video.decoder.vc1";
    const char* roleMPEG4 = "OMX.qcom.video.decoder.mpeg4";
    const char* roleHEVC = "OMX.qcom.video.decoder.hevc";
    if (!strncmp(mComponentName.c_str(), roleVC1, strlen(roleVC1)) ||
            !strncmp(mComponentName.c_str(), roleMPEG4, strlen(roleMPEG4))) {
        // The codec requires timestamp reordering
        tsReorder = true;
    } else if (fileFormatCStr!= NULL) {
        // Check for containers that support timestamp reordering
        AVLOGD("Container format = %s", fileFormatCStr);
        if (!strncmp(fileFormatCStr, "video/avi", 9)) {
            // The container requires timestamp reordering
            tsReorder = true;
        } else if (!strncmp(fileFormatCStr, MEDIA_MIMETYPE_CONTAINER_MPEG2TS,
                strlen(MEDIA_MIMETYPE_CONTAINER_MPEG2TS)) &&
                !strncmp(mComponentName.c_str(), roleHEVC, strlen(roleHEVC))) {
            // HEVC content in the TS container requires timestamp reordering
            tsReorder = true;
        }
    }

    if (tsReorder) {
        AVLOGI("Enabling timestamp reordering");
        QOMX_INDEXTIMESTAMPREORDER reorder;
        InitOMXParams(&reorder);
        reorder.nPortIndex = kPortIndexOutput;
        reorder.bEnable = OMX_TRUE;
        status_t err = mOMXNode->setParameter((OMX_INDEXTYPE)OMX_QcomIndexParamEnableTimeStampReorder,
                       (void *)&reorder, sizeof(reorder));

        if (err != OK) {
            AVLOGW("Failed to enable timestamp reordering");
        }
    }

    // MediaCodec clients can request decoder extradata by setting
    // "enable-extradata-<type>" in MediaFormat.
    // Following <type>s are supported:
    //    "user" => user-extradata
    int extraDataRequested = 0;
    if (msg->findInt32("enable-extradata-user", &extraDataRequested) &&
            extraDataRequested == 1) {
        AVLOGI("[%s] User-extradata requested", mComponentName.c_str());
        QOMX_ENABLETYPE enableType;
        InitOMXParams(&enableType);
        enableType.bEnable = OMX_TRUE;

        status_t err = mOMXNode->setParameter(
                (OMX_INDEXTYPE)OMX_QcomIndexEnableExtnUserData,
                (void *)&enableType, sizeof(enableType));
        if (err != OK) {
            AVLOGW("[%s] Failed to enable user-extradata", mComponentName.c_str());
        }
    }

    err = setDitherControl(msg);
    if (err != OK) {
        AVLOGW("Setting of dither control failed with err:%d",err);
    }
    return OK;
}

status_t ExtendedACodec::setupVideoEncoder(
            const char *mime, const sp<AMessage> &msg,
            sp<AMessage> &outputformat, sp<AMessage> &inputformat) {
    AVLOGI("setupVideoEncoder()");
    status_t err = ACodec::setupVideoEncoder(mime, msg, outputformat, inputformat);

    bool isQCComponent = mComponentName.startsWith("OMX.qcom.");

    if (err != OK || !isQCComponent) {
        return err;
    }

    AVLOGI("[%s] configure, AMessage : %s\n", mComponentName.c_str(), msg->debugString().c_str());

    // Set batch size for batch-mode encoding
    int32_t batchSize;
    if (msg->findInt32("batch-size", &batchSize)) {
        OMX_INDEXTYPE indexType;
        OMX_PARAM_U32TYPE batch;
        InitOMXParams(&batch);
        AVLOGI("Configuring batch-size: %d", batchSize);

        err = mOMXNode->getExtensionIndex(
                "OMX.QCOM.index.param.video.InputBatch", &indexType);
        if (err != OK) {
            AVLOGE("Failed to get extension for OMX.QCOM.index.param.video.InputBatch");
            return err;
        }

        err = mOMXNode->getParameter(indexType, (void *)&batch, sizeof(batch));
        if (err != OK) {
            AVLOGE("Failed to get InputBatch");
            return err;
        }

        batch.nU32 = batchSize;
        err = mOMXNode->setParameter(indexType, (void *)&batch, sizeof(batch));
        if (err != OK) {
            AVLOGE("Failed to set InputBatch");
            return err;
        }
    }

    if (!strncmp(mime, MEDIA_MIMETYPE_VIDEO_TME, strlen(MEDIA_MIMETYPE_VIDEO_TME))) {
        return setupTMEEncoderParameters(msg, outputformat);
    }
    return OK;
}

status_t ExtendedACodec::setupTMEEncoderParameters(const sp<AMessage> &msg __unused,
            sp<AMessage> &outputFormat __unused) {
#ifdef OMX_QTI_INDEX_PARAM_TME
    status_t ret;
    OMX_INDEXTYPE indexType;
    QOMX_VIDEO_PARAM_TMETYPE tme;
    int32_t profile = QOMX_VIDEO_TMEProfile0;
    int32_t level = QOMX_VIDEO_TMELevelInteger;

    if (msg->findInt32("profile", &profile)) {
        if (!msg->findInt32("level", &level)) {
            return INVALID_OPERATION;
        }
    }

    InitOMXParams(&tme);
    tme.eProfile = (QOMX_VIDEO_TMEPROFILETYPE)profile;
    tme.eLevel = (QOMX_VIDEO_TMELEVELTYPE)level;

    ret = mOMXNode->getExtensionIndex(
                OMX_QTI_INDEX_PARAM_TME,
                &indexType);
    if (ret != OK) {
        AVLOGE("Failed to get extension for Low Latency");
        return ret;
    }

    ret = mOMXNode->setParameter(indexType,
        (void *)&tme, sizeof(tme));
    if (ret != OK) {
        AVLOGE("Failed to set tme parameter");
        return ret;
    }

    ret = mOMXNode->getParameter(indexType,
        (void *)&tme, sizeof(tme));
    if (ret != OK) {
        AVLOGE("Failed to get tme parameter");
        return ret;
    }

    outputFormat->setInt32("tme-version", tme.ePayloadVersion);
    return ret;
#else
    return INVALID_OPERATION;
#endif
}

status_t ExtendedACodec::setParameters(const sp<AMessage> &msg) {
    AVLOGV("setParameters()");
    status_t err = ACodec::setParameters(msg);

    bool isQCComponent = mComponentName.startsWith("OMX.qcom") ||
        mComponentName.startsWith("OMX.Qualcomm") || mComponentName.startsWith("OMX.qti");

    //do nothing for non QC component and QC decoders
    if (err != OK || !isQCComponent) {
        return err;
    }

    err = setMpeghParameters(msg);
    if (err != OK) {
        AVLOGE("setMpeghParameters failed");
    }

    return OK;
}

status_t ExtendedACodec::setupCustomCodec(
        status_t inputErr, const char *mime, const sp<AMessage> &msg) {
    status_t err = inputErr;

    AVLOGD("setupCustomCodec for %s", mime);
    if (mIsEncoder) {
        int32_t numChannels, sampleRate;
        if (msg->findInt32("channel-count", &numChannels)
              && msg->findInt32("sample-rate", &sampleRate)) {
            setupRawAudioFormat(kPortIndexInput, sampleRate, numChannels);
        }
    }

    int32_t numChannels, sampleRate;
    CHECK(msg->findInt32("channel-count", &numChannels));
    CHECK(msg->findInt32("sample-rate", &sampleRate));
    if (!strcasecmp(MEDIA_MIMETYPE_AUDIO_EVRC, mime)) {
        err = setEVRCFormat(numChannels, sampleRate);
    } else if (!strcasecmp(MEDIA_MIMETYPE_AUDIO_MHAS, mime)) {
        err = setMPEGHFormat(numChannels, sampleRate, msg);
    } else if (!strcasecmp(MEDIA_MIMETYPE_AUDIO_QCELP, mime)) {
        err = setQCELPFormat(numChannels, sampleRate);
    } else if (!strcasecmp(MEDIA_MIMETYPE_AUDIO_AMR_WB_PLUS, mime) ||
        !strcasecmp(MEDIA_MIMETYPE_AUDIO_QC_AMR_WB_PLUS, mime)) {
        err = setAMRWBPLUSFormat(numChannels, sampleRate);
    } else if (!strcasecmp(MEDIA_MIMETYPE_AUDIO_WMA, mime))  {
        err = setWMAFormat(msg);
    } else if (!strcasecmp(MEDIA_MIMETYPE_AUDIO_FLAC, mime) && !mIsEncoder) {
        int32_t bitsPerSample;
        int32_t minBlkSize, maxBlkSize, minFrmSize, maxFrmSize;
        AVLOGV("ExtendedCodec::setAudioFormat(): FLAC Decoder");
        if (!msg->findInt32(ExtendedUtils::getMsgFromKey(kKeySampleBits), &bitsPerSample)) {
            //if the parser does not set the bitwidth, default it to 16 bit
            AVLOGI("no bitwidth, setting default bitwidth as 16 bits");
            bitsPerSample = 16;
        }
        if (!msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyMinBlkSize), &minBlkSize)) {
            AVLOGI("no min blksize, setting default block size as 16");
            minBlkSize = 16;
        }

        if (!msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyMaxBlkSize), &maxBlkSize)) {
            AVLOGI("no max blksize, setting default block size as 16");
            maxBlkSize = 16;
        }

        if (!msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyMinFrmSize), &minFrmSize)) {
            AVLOGI("no min frame size, setting default frame size as 0");
            minFrmSize = 0;
        }

        if (!msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyMaxFrmSize), &maxFrmSize)) {
            AVLOGI("no max frame size, setting default frame size as 0");
            maxFrmSize = 0;
        }
        err = setFLACDecoderFormat(numChannels, sampleRate, bitsPerSample, minBlkSize, maxBlkSize,
                minFrmSize, maxFrmSize);
    } else if (!strcasecmp(MEDIA_MIMETYPE_AUDIO_ALAC, mime)) {
        int32_t bitsPerSample;
        if (!msg->findInt32(ExtendedUtils::getMsgFromKey(kKeySampleBits), &bitsPerSample)) {
            //if the parser does not set the bitwidth, default it to 16 bit
            AVLOGI("no bitwidth, setting default bitwidth as 16 bits");
            bitsPerSample = 16;
        }
        err = setALACFormat(numChannels, sampleRate, bitsPerSample);
    } else if (!strcasecmp(MEDIA_MIMETYPE_AUDIO_APE, mime)) {
        int32_t bitsPerSample;
        if (!msg->findInt32(ExtendedUtils::getMsgFromKey(kKeySampleBits), &bitsPerSample)) {
            //if the parser does not set the bitwidth, default it to 16 bit
            AVLOGI("no bitwidth, setting default bitwidth as 16 bits");
            bitsPerSample = 16;
        }
        err = setAPEFormat(numChannels, sampleRate, bitsPerSample);
    } else if (!strcasecmp(MEDIA_MIMETYPE_AUDIO_DSD, mime)) {
        int32_t bitsPerSample;
        if (!msg->findInt32(ExtendedUtils::getMsgFromKey(kKeySampleBits), &bitsPerSample)) {
            //if the parser does not set the bitwidth, default it to 16 bit
            AVLOGI("no bitwidth, setting default bitwidth as 16 bits");
            bitsPerSample = 16;
        }
        err = setDSDFormat(numChannels, sampleRate, bitsPerSample);
    } // TODO Audio : handle other Audio formats here

    return err;
}

status_t ExtendedACodec::setupOmxPortConfig(OMX_U32 portIndex, OMX_AUDIO_CODINGTYPE OMX_AUDIO_Coding) {
    status_t err = OMX_ErrorNone;

    OMX_AUDIO_PARAM_PORTFORMATTYPE format;
    InitOMXParams(&format);
    format.nPortIndex = portIndex;
    format.nIndex = OMX_IndexParamAudioPortFormat;
    while (OMX_ErrorNone == err) {
        CHECK_EQ(mOMXNode->getParameter(OMX_IndexParamAudioPortFormat,
                &format, sizeof(format)), (status_t)OK);
        if (format.eEncoding == OMX_AUDIO_Coding) {
            break;
        }
        format.nIndex++;
    }
    if (OK != err) {
        AVLOGV("returning error %d at %d", err, __LINE__);
        return err;
    }
    err = mOMXNode->setParameter(OMX_IndexParamAudioPortFormat,
            &format, sizeof(format));
    if (OK != err) {
        AVLOGV("returning error %d at %d", err, __LINE__);
        return err;
    }

    // port definition
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);
    def.nPortIndex = portIndex;
    def.format.audio.cMIMEType = (OMX_STRING)NULL;
    err = mOMXNode->getParameter(OMX_IndexParamPortDefinition,
            &def, sizeof(def));
    if (OK != err) {
        AVLOGV("returning error %d at %d", err, __LINE__);
        return err;
    }
    def.format.audio.bFlagErrorConcealment = OMX_TRUE;
    def.format.audio.eEncoding = OMX_AUDIO_Coding;
    err = mOMXNode->setParameter(OMX_IndexParamPortDefinition,
            &def, sizeof(def));
    if (OK != err) {
        AVLOGV("returning error %d at %d", err, __LINE__);
        return err;
    }
    return err;
}

status_t ExtendedACodec::setQCELPFormat(
       int32_t numChannels, int32_t /*sampleRate*/) {

   status_t err = OMX_ErrorNone;
   if (mIsEncoder) {
       CHECK(numChannels == 1);
       //////////////// input port ////////////////////
       //handle->setRawAudioFormat(kPortIndexInput, sampleRate, numChannels);
       //////////////// output port ////////////////////
       // format
       OMX_AUDIO_PARAM_PORTFORMATTYPE format;
       InitOMXParams(&format);
       format.nPortIndex = kPortIndexOutput;
       format.nIndex = 0;

       while (OMX_ErrorNone == err) {
            CHECK_EQ(mOMXNode->getParameter(OMX_IndexParamAudioPortFormat,
                    &format, sizeof(format)), (status_t)OK);
            if (format.eEncoding == OMX_AUDIO_CodingQCELP13) {
                break;
            }
            format.nIndex++;
        }
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        err = mOMXNode->setParameter(OMX_IndexParamAudioPortFormat,
                &format, sizeof(format));
        if (err != OK) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        // port definition
        OMX_PARAM_PORTDEFINITIONTYPE def;
        InitOMXParams(&def);
        def.nPortIndex = kPortIndexOutput;
        def.format.audio.cMIMEType = (OMX_STRING)NULL;
        err = mOMXNode->getParameter(OMX_IndexParamPortDefinition,
                &def, sizeof(def));
        if (err != OK) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        def.format.audio.bFlagErrorConcealment = OMX_TRUE;
        def.format.audio.eEncoding = OMX_AUDIO_CodingQCELP13;
        err = mOMXNode->setParameter(OMX_IndexParamPortDefinition,
                &def, sizeof(def));

        // profile
        OMX_AUDIO_PARAM_QCELP13TYPE profile;
        InitOMXParams(&profile);
        profile.nPortIndex = kPortIndexOutput;
        err = mOMXNode->getParameter(OMX_IndexParamAudioQcelp13,
                &profile, sizeof(profile));
        if (err != OK) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        profile.nChannels = numChannels;
        err = mOMXNode->setParameter(OMX_IndexParamAudioQcelp13,
                &profile, sizeof(profile));
    }
    return err;
}

status_t ExtendedACodec::setFLACDecoderFormat(
        int32_t numChannels, int32_t sampleRate, int32_t bitsPerSample,
        int32_t minBlkSize, int32_t maxBlkSize,
        int32_t minFrmSize, int32_t maxFrmSize) {

    QOMX_AUDIO_PARAM_FLAC_DEC_TYPE profileFLACDec;
    OMX_PARAM_PORTDEFINITIONTYPE portParam;
    status_t err;

    AVLOGV("FLACDec setformat sampleRate:%d numChannels:%d, bitsPerSample:%d",
            sampleRate, numChannels, bitsPerSample);

    //for input port
    InitOMXParams(&profileFLACDec);
    profileFLACDec.nPortIndex = kPortIndexInput;
    err = mOMXNode->getParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioFlacDec,
            &profileFLACDec, sizeof(profileFLACDec));
    if (err != OK) {
        AVLOGE("returning error %d for get QOMX_IndexParamAudioFlacDec", err);
        return err;
    }

    profileFLACDec.nSampleRate = sampleRate;
    profileFLACDec.nChannels = numChannels;
    profileFLACDec.nBitsPerSample = bitsPerSample;
    profileFLACDec.nMinBlkSize = minBlkSize;
    profileFLACDec.nMaxBlkSize = maxBlkSize;
    profileFLACDec.nMinFrmSize = minFrmSize;
    profileFLACDec.nMaxFrmSize = maxFrmSize;
    err = mOMXNode->setParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioFlacDec,
            &profileFLACDec, sizeof(profileFLACDec));
    if (err != OK) {
        AVLOGE("returning error %d for set OMX_IndexParamAudioFlacDec", err);
        return err;
    }

    //for output port
    OMX_AUDIO_PARAM_PCMMODETYPE profilePcm;
    InitOMXParams(&profilePcm);
    profilePcm.nPortIndex = kPortIndexOutput;
    err = mOMXNode->getParameter(
            OMX_IndexParamAudioPcm, &profilePcm, sizeof(profilePcm));
    if (err != OK) {
        AVLOGE("returning error %d for OMX_IndexParamAudioPcm", err);
        return err;
    }

    profilePcm.nSamplingRate = sampleRate;
    profilePcm.nChannels = numChannels;
    profilePcm.nBitPerSample = bitsPerSample;
    err = mOMXNode->setParameter(
            OMX_IndexParamAudioPcm, &profilePcm, sizeof(profilePcm));

    return err;
}

status_t ExtendedACodec::setMPEGHFormat(
        int32_t numChannels, int32_t sampleRate, const sp<AMessage> &msg) {
    AVLOGD("setMPEGHFormat");
    status_t err = OMX_ErrorNone;
    if (mIsEncoder) {
        (void)msg;
        if (numChannels != 4) {
            AVLOGE("setMPEGHFormat - Invalid number of channels %d", numChannels);
            return BAD_VALUE;
        }

        if (sampleRate != 48000) {
            AVLOGE("setMPEGHFormat - Invalid sampling rate %d", sampleRate);
            return BAD_VALUE;
        }

        //////////////// input port ////////////////////
        OMX_AUDIO_PARAM_PORTFORMATTYPE in_format;
        InitOMXParams(&in_format);
        in_format.nPortIndex = kPortIndexInput;
        in_format.nIndex = OMX_IndexParamAudioPortFormat;
        while (OMX_ErrorNone == err) {
            err = mOMXNode->getParameter(OMX_IndexParamAudioPortFormat, &in_format, sizeof(in_format));
            if (err != (status_t)OK) {
                AVLOGE("setMPEGHFormat - OMX GetParam error %d for Index %d", err, in_format.nIndex);
                break;
            }
            if (in_format.eEncoding == OMX_AUDIO_CodingPCM) {
                break;
            }
            in_format.nIndex++;
        }
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        err = mOMXNode->setParameter(OMX_IndexParamAudioPortFormat,
                &in_format, sizeof(in_format));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }

        // port definition
        OMX_PARAM_PORTDEFINITIONTYPE in_def;
        InitOMXParams(&in_def);
        in_def.nPortIndex = kPortIndexInput;
        in_def.format.audio.cMIMEType = (OMX_STRING)NULL;
        err = mOMXNode->getParameter(OMX_IndexParamPortDefinition,
                &in_def, sizeof(in_def));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        in_def.format.audio.bFlagErrorConcealment = OMX_TRUE;
        in_def.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
        err = mOMXNode->setParameter(OMX_IndexParamPortDefinition,
                &in_def, sizeof(in_def));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        // profile
        OMX_AUDIO_PARAM_PCMMODETYPE in_profile;
        InitOMXParams(&in_profile);
        in_profile.nPortIndex = kPortIndexInput;
        err = mOMXNode->getParameter(OMX_IndexParamAudioPcm,
                &in_profile, sizeof(in_profile));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        err = mOMXNode->setParameter(OMX_IndexParamAudioPcm,
                &in_profile, sizeof(in_profile));

        //////////////// output port ////////////////////
        // format
        OMX_AUDIO_PARAM_PORTFORMATTYPE format;
        InitOMXParams(&format);
        format.nPortIndex = kPortIndexOutput;
        format.nIndex = OMX_IndexParamAudioPortFormat;
        while (OMX_ErrorNone == err) {
            CHECK_EQ(mOMXNode->getParameter(OMX_IndexParamAudioPortFormat,
                    &format, sizeof(format)), (status_t)OK);
            if (format.eEncoding == (OMX_AUDIO_CODINGTYPE)QOMX_AUDIO_CodingMPEGH) {
                break;
            }
            format.nIndex++;
        }
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        err = mOMXNode->setParameter(OMX_IndexParamAudioPortFormat,
                &format, sizeof(format));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }

        // port definition
        OMX_PARAM_PORTDEFINITIONTYPE def;
        InitOMXParams(&def);
        def.nPortIndex = kPortIndexOutput;
        def.format.audio.cMIMEType = (OMX_STRING)NULL;
        err = mOMXNode->getParameter(OMX_IndexParamPortDefinition,
                &def, sizeof(def));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        def.format.audio.bFlagErrorConcealment = OMX_TRUE;
        def.format.audio.eEncoding = (OMX_AUDIO_CODINGTYPE)QOMX_AUDIO_CodingMPEGH;
        err = mOMXNode->setParameter(OMX_IndexParamPortDefinition,
                &def, sizeof(def));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        // profile
        QOMX_AUDIO_PARAM_MPEGH_TYPE profile;
        InitOMXParams(&profile);
        profile.nPortIndex = kPortIndexOutput;
        err = mOMXNode->getParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioMpegh,
                &profile, sizeof(profile));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }

        int32_t bitRate = 300000;
        if (msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyBitRate), &bitRate)) {
            AVLOGD("setMPEGHEncFormat received bitrate %d", bitRate);
        }

        if (bitRate < 384000) {
            bitRate = 300000;
        } else if (bitRate >= 512000) {
            bitRate = 512000;
        } else {
            bitRate = 384000;
        }

        AVLOGD("setMPEGHEncFormat adapted bitrate %d", bitRate);
        profile.nBitRate = bitRate / 1000;
        err = mOMXNode->setParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioMpegh,
                &profile, sizeof(profile));

    } else {
        void *headerConfig = NULL;
        void *headerSceneInfo = NULL;
        int32_t bitWidth = 16;
        uint32_t channelMask = AUDIO_CHANNEL_OUT_STEREO;
        int32_t binuralMode = 0;
        int32_t hoaMode = 0;

        // Get Configuration Params
        if (msg->findPointer(ExtendedUtils::getMsgFromKey(kKeyMHAConfig), &headerConfig)) {
            AVLOGD("setMPEGHDecFormat received header config");
        }
        if (msg->findPointer(ExtendedUtils::getMsgFromKey(kKeyMHASceneInfo), &headerSceneInfo)) {
            AVLOGD("setMPEGHDecFormat received header Scene Info");
        }
        if (msg->findInt32("bits-per-sample", &bitWidth)) {
            AVLOGD("setMPEGHDecFormat bitWidth set to %d", bitWidth);
        }
        if (msg->findInt32("channel-mask", (int32_t *)&channelMask)) {
            AVLOGD("setMPEGHDecFormat channelMask set to 0x%x", channelMask);
        } else if (numChannels > 2) {
            channelMask = audio_channel_out_mask_from_count(numChannels);
        }
        if (msg->findInt32("binaural-mode", (int32_t *)&binuralMode)) {
            AVLOGD("setMPEGHDecFormat binuralMode set to %d", binuralMode);
        }
        if (msg->findInt32("enable-hoa", (int32_t *)&hoaMode)) {
            AVLOGD("setMPEGHDecFormat hoaMode set to %d", hoaMode);
        }

        //////////////// input port ////////////////////
        err = setupOmxPortConfig(kPortIndexInput, (OMX_AUDIO_CODINGTYPE)QOMX_AUDIO_CodingMPEGH);
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }

        // profile
        QOMX_AUDIO_PARAM_MPEGH_TYPE in_profile;
        InitOMXParams(&in_profile);
        in_profile.nPortIndex = kPortIndexInput;
        err = mOMXNode->getParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioMpegh,
                &in_profile, sizeof(in_profile));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }

        in_profile.nSampleRate = sampleRate;
        err = mOMXNode->setParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioMpegh,
                &in_profile, sizeof(in_profile));

        // MPEG Header
        QOMX_AUDIO_PARAM_MPEGH_HEADER_TYPE in_header;
        InitOMXParams(&in_header);
        in_header.nPortIndex = kPortIndexInput;
        err = mOMXNode->getParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioMpeghHeader,
                &in_header, sizeof(in_header));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }

        in_header.pConfig = (OMX_U64)headerConfig;
        in_header.pSceneInfo = (OMX_U64)headerSceneInfo;
        err = mOMXNode->setParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioMpeghHeader,
                &in_header, sizeof(in_header));

        // Channel Config
        QOMX_AUDIO_PARAM_CHANNEL_CONFIG_TYPE in_channel;
        InitOMXParams(&in_channel);
        in_channel.nPortIndex = kPortIndexInput;
        err = mOMXNode->getParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioChannelMask,
                &in_channel, sizeof(in_channel));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }

        in_channel.nChannelMask = channelMask;
        err = mOMXNode->setParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioChannelMask,
                &in_channel, sizeof(in_channel));

        // Binaural
        QOMX_AUDIO_PARAM_BINAURAL_TYPE in_binaural;
        InitOMXParams(&in_binaural);
        in_binaural.nPortIndex = kPortIndexInput;
        err = mOMXNode->getParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioBinauralMode,
                &in_binaural, sizeof(in_binaural));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }

        in_binaural.eBinauralMode = binuralMode ? QOMX_BINAURAL_MODE_ON : QOMX_BINAURAL_MODE_OFF;
        err = mOMXNode->setParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioBinauralMode,
                &in_binaural, sizeof(in_binaural));

        // Rotation
        QOMX_AUDIO_PARAM_ROTATION_TYPE in_rotation;
        InitOMXParams(&in_rotation);
        in_rotation.nPortIndex = kPortIndexInput;
        err = mOMXNode->getParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioRotation,
                &in_rotation, sizeof(in_rotation));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        err = mOMXNode->setParameter((OMX_INDEXTYPE)QOMX_IndexParamAudioRotation,
                &in_rotation, sizeof(in_rotation));

        //////////////// output port ////////////////////
        err = setupOmxPortConfig(kPortIndexOutput, OMX_AUDIO_CodingPCM);
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }

        // profile
        OMX_AUDIO_PARAM_PCMMODETYPE out_profile;
        InitOMXParams(&out_profile);
        out_profile.nPortIndex = kPortIndexOutput;
        err = mOMXNode->getParameter(OMX_IndexParamAudioPcm,
                &out_profile, sizeof(out_profile));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }

        out_profile.nChannels = numChannels;
        out_profile.nBitPerSample = bitWidth;
        out_profile.nSamplingRate = sampleRate;
        out_profile.bInterleaved = OMX_TRUE;
        if (hoaMode != 0) {
            out_profile.ePCMMode = (OMX_AUDIO_PCMMODETYPE)OMX_AUDIO_PCMModeVendorHoa;
        }

        AVLOGD("setMPEGHDecFormat - output channels %u bits per sample %u",
            out_profile.nChannels,
            out_profile.nBitPerSample);

        AVLOGD("setMPEGHDecFormat - sample rate %u PCM Mode 0x%x",
            out_profile.nSamplingRate,
            out_profile.ePCMMode);

        err = mOMXNode->setParameter(OMX_IndexParamAudioPcm,
                &out_profile, sizeof(out_profile));
    }
    return err;
}

status_t ExtendedACodec::setEVRCFormat(
        int32_t numChannels, int32_t /*sampleRate*/) {
    AVLOGD("setEVRCFormat");
    status_t err = OMX_ErrorNone;
    if (mIsEncoder) {
        CHECK(numChannels == 1);
        //////////////// input port ////////////////////
        //handle->setRawAudioFormat(kPortIndexInput, sampleRate, numChannels);
        //////////////// output port ////////////////////
        // format
        OMX_AUDIO_PARAM_PORTFORMATTYPE format;
        InitOMXParams(&format);
        format.nPortIndex = kPortIndexOutput;
        format.nIndex = 0;
        while (OMX_ErrorNone == err) {
            CHECK_EQ(mOMXNode->getParameter(OMX_IndexParamAudioPortFormat,
                    &format, sizeof(format)), (status_t)OK);
            if (format.eEncoding == OMX_AUDIO_CodingEVRC) {
                break;
            }
            format.nIndex++;
        }
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        err = mOMXNode->setParameter(OMX_IndexParamAudioPortFormat,
                &format, sizeof(format));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }

        // port definition
        OMX_PARAM_PORTDEFINITIONTYPE def;
        InitOMXParams(&def);
        def.nPortIndex = kPortIndexOutput;
        def.format.audio.cMIMEType = (OMX_STRING)NULL;
        err = mOMXNode->getParameter(OMX_IndexParamPortDefinition,
                &def, sizeof(def));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        def.format.audio.bFlagErrorConcealment = OMX_TRUE;
        def.format.audio.eEncoding = OMX_AUDIO_CodingEVRC;
        err = mOMXNode->setParameter(OMX_IndexParamPortDefinition,
                &def, sizeof(def));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        // profile
        OMX_AUDIO_PARAM_EVRCTYPE profile;
        InitOMXParams(&profile);
        profile.nPortIndex = kPortIndexOutput;
        err = mOMXNode->getParameter(OMX_IndexParamAudioEvrc,
                &profile, sizeof(profile));
        if (OK != err) {
            AVLOGV("returning error %d at %d", err, __LINE__);
            return err;
        }
        profile.nChannels = numChannels;
        err = mOMXNode->setParameter(OMX_IndexParamAudioEvrc,
                &profile, sizeof(profile));

    } else {
        AVLOGD("EVRC decoder");
    }
    return err;
}

status_t ExtendedACodec::setAMRWBPLUSFormat(
        int32_t numChannels, int32_t sampleRate) {

    QOMX_AUDIO_PARAM_AMRWBPLUSTYPE profileAMRWBPlus;
    OMX_INDEXTYPE indexTypeAMRWBPlus;
    OMX_PARAM_PORTDEFINITIONTYPE portParam;

    AVLOGV("AMRWB+ setformat sampleRate:%d numChannels:%d",sampleRate,numChannels);

    //configure input port
    InitOMXParams(&portParam);
    portParam.nPortIndex = kPortIndexInput;
    status_t err = mOMXNode->getParameter(
            OMX_IndexParamPortDefinition, &portParam, sizeof(portParam));
    CHECK_EQ(err, (status_t)OK);
    err = mOMXNode->setParameter(
            OMX_IndexParamPortDefinition, &portParam, sizeof(portParam));
    CHECK_EQ(err, (status_t)OK);

    //configure output port
    portParam.nPortIndex = kPortIndexOutput;
    err = mOMXNode->getParameter(
            OMX_IndexParamPortDefinition, &portParam, sizeof(portParam));
    CHECK_EQ(err, (status_t)OK);
    err = mOMXNode->setParameter(
            OMX_IndexParamPortDefinition, &portParam, sizeof(portParam));
    CHECK_EQ(err, (status_t)OK);

    err = mOMXNode->getExtensionIndex(OMX_QCOM_INDEX_PARAM_AMRWBPLUS, &indexTypeAMRWBPlus);
    CHECK_EQ(err, (status_t)OK);

    //for input port
    InitOMXParams(&profileAMRWBPlus);
    profileAMRWBPlus.nPortIndex = kPortIndexInput;
    err = mOMXNode->getParameter(indexTypeAMRWBPlus, &profileAMRWBPlus, sizeof(profileAMRWBPlus));
    CHECK_EQ(err,(status_t)OK);

    profileAMRWBPlus.nSampleRate = sampleRate;
    profileAMRWBPlus.nChannels = numChannels;
    err = mOMXNode->setParameter(indexTypeAMRWBPlus, &profileAMRWBPlus, sizeof(profileAMRWBPlus));
    CHECK_EQ(err,(status_t)OK);

    //for output port
    OMX_AUDIO_PARAM_PCMMODETYPE profilePcm;
    InitOMXParams(&profilePcm);
    profilePcm.nPortIndex = kPortIndexOutput;
    err = mOMXNode->getParameter(
            OMX_IndexParamAudioPcm, &profilePcm, sizeof(profilePcm));
    CHECK_EQ(err, (status_t)OK);

    profilePcm.nSamplingRate = sampleRate;
    profilePcm.nChannels = numChannels;
    err = mOMXNode->setParameter(
            OMX_IndexParamAudioPcm, &profilePcm, sizeof(profilePcm));
    CHECK_EQ(err, (status_t)OK);
    AVLOGE("returning error %d", err);
    return err;
}

status_t ExtendedACodec::setWMAFormat(
        const sp<AMessage> &msg) {
    AVLOGV("setWMAFormat Called");

    if (mIsEncoder) {
        AVLOGE("WMA encoding not supported");
        return OK;
    } else {
        int32_t version;
        OMX_AUDIO_PARAM_WMATYPE paramWMA;
        QOMX_AUDIO_PARAM_WMA10PROTYPE paramWMA10;
        CHECK(msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyWMAVersion), &version));
        int32_t numChannels;
        int32_t bitRate;
        int32_t sampleRate;
        int32_t encodeOptions;
        int32_t blockAlign;
        int32_t bitspersample;
        int32_t formattag;
        int32_t advencopt1;
        int32_t advencopt2;
        int32_t VirtualPktSize;
        if (version==kTypeWMAPro || version==kTypeWMALossLess) {
            CHECK(msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyWMABitspersample), &bitspersample));
            CHECK(msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyWMAFormatTag), &formattag));
            CHECK(msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyWMAAdvEncOpt1), &advencopt1));
            CHECK(msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyWMAAdvEncOpt2), &advencopt2));
            CHECK(msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyWMAVirPktSize), &VirtualPktSize));
        }
        if (version==kTypeWMA) {
            InitOMXParams(&paramWMA);
            paramWMA.nPortIndex = kPortIndexInput;
        } else if (version==kTypeWMAPro || version==kTypeWMALossLess) {
            InitOMXParams(&paramWMA10);
            paramWMA10.nPortIndex = kPortIndexInput;
        }
        CHECK(msg->findInt32("channel-count", &numChannels));
        CHECK(msg->findInt32("sample-rate", &sampleRate));
        CHECK(msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyBitRate), &bitRate));
        CHECK(msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyWMAEncodeOpt), &encodeOptions));
        CHECK(msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyWMABlockAlign), &blockAlign));
        AVLOGV("Channels: %d, SampleRate: %d, BitRate; %d"
                "EncodeOptions: %d, blockAlign: %d", numChannels,
                sampleRate, bitRate, encodeOptions, blockAlign);
        if (sampleRate > 48000 && numChannels > 2) {
            AVLOGE("Unsupported samplerate/channels");
            return ERROR_UNSUPPORTED;
        }
        if (version==kTypeWMAPro || version==kTypeWMALossLess) {
            AVLOGV("Bitspersample: %d, wmaformattag: %d,"
                    "advencopt1: %d, advencopt2: %d VirtualPktSize %d", bitspersample,
                    formattag, advencopt1, advencopt2, VirtualPktSize);
        }
        status_t err = OK;
        OMX_INDEXTYPE index;
        if (version==kTypeWMA) {
            err = mOMXNode->getParameter(
                    OMX_IndexParamAudioWma, &paramWMA, sizeof(paramWMA));
        } else if (version==kTypeWMAPro || version==kTypeWMALossLess) {
            mOMXNode->getExtensionIndex("OMX.Qualcomm.index.audio.wma10Pro",&index);
            err = mOMXNode->getParameter(
                    index, &paramWMA10, sizeof(paramWMA10));
        }
        if (err != OK) {
            return err;
        }
        if (version==kTypeWMA) {
            paramWMA.nChannels = numChannels;
            paramWMA.nSamplingRate = sampleRate;
            paramWMA.nEncodeOptions = encodeOptions;
            paramWMA.nBitRate = bitRate;
            paramWMA.nBlockAlign = blockAlign;
        } else if (version==kTypeWMAPro || version==kTypeWMALossLess) {
            paramWMA10.nChannels = numChannels;
            paramWMA10.nSamplingRate = sampleRate;
            paramWMA10.nEncodeOptions = encodeOptions;
            paramWMA10.nBitRate = bitRate;
            paramWMA10.nBlockAlign = blockAlign;
        }
        if (version==kTypeWMAPro || version==kTypeWMALossLess) {
            paramWMA10.advancedEncodeOpt = advencopt1;
            paramWMA10.advancedEncodeOpt2 = advencopt2;
            paramWMA10.formatTag = formattag;
            paramWMA10.validBitsPerSample = bitspersample;
            paramWMA10.nVirtualPktSize = VirtualPktSize;
        }
        if (version==kTypeWMA) {
            err = mOMXNode->setParameter(
                    OMX_IndexParamAudioWma, &paramWMA, sizeof(paramWMA));
        } else if (version==kTypeWMAPro || version==kTypeWMALossLess) {
            err = mOMXNode->setParameter(
                    index, &paramWMA10, sizeof(paramWMA10));
        }
        return err;
    }
    return OK;
}

status_t ExtendedACodec::setALACFormat(
        int32_t numChannels, int32_t sampleRate, int32_t bitsPerSample) {

    QOMX_AUDIO_PARAM_ALACTYPE paramALAC;
    OMX_PARAM_PORTDEFINITIONTYPE portParam;
    OMX_INDEXTYPE indexTypeALAC;
    status_t err = OK;

    AVLOGV("setALACFormat sampleRate:%d numChannels:%d bitsPerSample: %d",
            sampleRate, numChannels, bitsPerSample);

    err = mOMXNode->getExtensionIndex(OMX_QCOM_INDEX_PARAM_ALAC, &indexTypeALAC);
    if (err != OK) {
        return err;
    }

    //for input port
    InitOMXParams(&paramALAC);
    paramALAC.nPortIndex = kPortIndexInput;
    err = mOMXNode->getParameter(indexTypeALAC, &paramALAC, sizeof(paramALAC));
    if (err != OK) {
        return err;
    }

    paramALAC.nSampleRate = sampleRate;
    paramALAC.nChannels = numChannels;
    paramALAC.nBitDepth = bitsPerSample;
    err = mOMXNode->setParameter(indexTypeALAC, &paramALAC, sizeof(paramALAC));
    if (err != OK) {
        return err;
    }

    //for output port
    OMX_AUDIO_PARAM_PCMMODETYPE profilePcm;
    InitOMXParams(&profilePcm);
    profilePcm.nPortIndex = kPortIndexOutput;
    err = mOMXNode->getParameter(
           OMX_IndexParamAudioPcm, &profilePcm, sizeof(profilePcm));
    if (err != OK) {
        return err;
    }

    profilePcm.nSamplingRate = sampleRate;
    profilePcm.nChannels = numChannels;
    profilePcm.nBitPerSample = bitsPerSample;
    err = mOMXNode->setParameter(
           OMX_IndexParamAudioPcm, &profilePcm, sizeof(profilePcm));

    return err;
}
status_t ExtendedACodec::setAPEFormat(
        int32_t numChannels, int32_t sampleRate, int32_t bitsPerSample) {

    QOMX_AUDIO_PARAM_APETYPE paramAPE;
    OMX_PARAM_PORTDEFINITIONTYPE portParam;
    OMX_INDEXTYPE indexTypeAPE;
    status_t err = OK;

    AVLOGV("setAPEFormat sampleRate:%d numChannels:%d bitsPerSample:%d",
            sampleRate, numChannels, bitsPerSample);

    err = mOMXNode->getExtensionIndex(OMX_QCOM_INDEX_PARAM_APE, &indexTypeAPE);
    if (err != OK) {
        return err;
    }

    //for input port
    InitOMXParams(&paramAPE);
    paramAPE.nPortIndex = kPortIndexInput;
    err = mOMXNode->getParameter(indexTypeAPE, &paramAPE, sizeof(paramAPE));
    if (err != OK) {
        return err;
    }

    paramAPE.nSampleRate = sampleRate;
    paramAPE.nChannels = numChannels;
    paramAPE.nBitsPerSample = bitsPerSample;
    err = mOMXNode->setParameter(indexTypeAPE, &paramAPE, sizeof(paramAPE));
    if (err != OK) {
        return err;
    }

    //for output port
    OMX_AUDIO_PARAM_PCMMODETYPE profilePcm;
    InitOMXParams(&profilePcm);
    profilePcm.nPortIndex = kPortIndexOutput;
    err = mOMXNode->getParameter(
           OMX_IndexParamAudioPcm, &profilePcm, sizeof(profilePcm));
    if (err != OK) {
        return err;
    }

    profilePcm.nSamplingRate = sampleRate;
    profilePcm.nChannels = numChannels;
    profilePcm.nBitPerSample = bitsPerSample;
    err = mOMXNode->setParameter(
           OMX_IndexParamAudioPcm, &profilePcm, sizeof(profilePcm));

    return err;
}

status_t ExtendedACodec::setDSDFormat(
        int32_t numChannels, int32_t sampleRate, int32_t bitsPerSample) {

    QOMX_AUDIO_PARAM_DSD_TYPE paramDSD;
    OMX_PARAM_PORTDEFINITIONTYPE portParam;
    OMX_INDEXTYPE indexTypeDSD;
    status_t err = OK;

    AVLOGV("setDSDFormat sampleRate:%d numChannels:%d bitsPerSample:%d",
            sampleRate, numChannels, bitsPerSample);

    err = mOMXNode->getExtensionIndex(OMX_QCOM_INDEX_PARAM_DSD, &indexTypeDSD);
    if (err != OK) {
        return err;
    }

    //for output port
    OMX_AUDIO_PARAM_PCMMODETYPE profilePcm;
    InitOMXParams(&profilePcm);
    profilePcm.nPortIndex = kPortIndexOutput;
    err = mOMXNode->getParameter(
           OMX_IndexParamAudioPcm, &profilePcm, sizeof(profilePcm));
    if (err != OK) {
        return err;
    }

    profilePcm.nSamplingRate = sampleRate;
    profilePcm.nChannels = numChannels;
    profilePcm.nBitPerSample = 24;
    err = mOMXNode->setParameter(
           OMX_IndexParamAudioPcm, &profilePcm, sizeof(profilePcm));
    if (err != OK) {
        return err;
    }

    //for input port
    InitOMXParams(&paramDSD);
    paramDSD.nPortIndex = kPortIndexInput;
    err = mOMXNode->getParameter(indexTypeDSD, &paramDSD, sizeof(paramDSD));
    if (err != OK) {
        return err;
    }

    paramDSD.nSampleRate = sampleRate;
    paramDSD.nChannels = numChannels;
    paramDSD.nBitsPerSample = bitsPerSample;
    err = mOMXNode->setParameter(indexTypeDSD, &paramDSD, sizeof(paramDSD));
    if (err != OK) {
        return err;
    }
    return err;
}

status_t ExtendedACodec::GetVideoCodingTypeFromMime(
        const char *mime, OMX_VIDEO_CODINGTYPE *codingType) {
    if (ACodec::GetVideoCodingTypeFromMime(mime, codingType) == OK) {
        return OK;
    }

    status_t retVal = OK;
    if (!strcasecmp(MEDIA_MIMETYPE_VIDEO_DIVX, mime)) {
        *codingType = (OMX_VIDEO_CODINGTYPE)QOMX_VIDEO_CodingDivx;
    } else if (!strcasecmp(MEDIA_MIMETYPE_VIDEO_DIVX4, mime)) {
        *codingType = (OMX_VIDEO_CODINGTYPE)QOMX_VIDEO_CodingDivx;
    } else if (!strcasecmp(MEDIA_MIMETYPE_VIDEO_DIVX311, mime)) {
        *codingType = (OMX_VIDEO_CODINGTYPE)QOMX_VIDEO_CodingDivx;
    } else if (!strcasecmp(MEDIA_MIMETYPE_VIDEO_WMV, mime)) {
        *codingType = OMX_VIDEO_CodingWMV;
    } else if (!strcasecmp(MEDIA_MIMETYPE_VIDEO_WMV_VC1, mime)) {
        *codingType = OMX_VIDEO_CodingWMV;
    } else if (!strcasecmp(MEDIA_MIMETYPE_CONTAINER_MPEG2, mime)) {
        *codingType = OMX_VIDEO_CodingMPEG2;
    } else if (!strcasecmp(MEDIA_MIMETYPE_VIDEO_TME, mime)) {
        *codingType = (OMX_VIDEO_CODINGTYPE)QOMX_VIDEO_CodingTME;
    } else {
        retVal = ERROR_UNSUPPORTED;
    }

    return retVal;
}

status_t ExtendedACodec::getPortFormat(OMX_U32 portIndex, sp<AMessage> &notify) {
    AVLOGD("getPortFormat()");
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);
    def.nPortIndex = portIndex;

    status_t err = mOMXNode->getParameter(OMX_IndexParamPortDefinition, &def, sizeof(def));
    if (err != OK) {
        ALOGE("Failed to getParameter for port %d", portIndex);
        return err;
    }

    if (def.eDir != (portIndex == kPortIndexOutput ? OMX_DirOutput : OMX_DirInput)) {
        ALOGE("Bad eDir in getParameter for port %d", portIndex);
        return BAD_VALUE;;
    }

    if (def.eDomain == OMX_PortDomainAudio) {

        OMX_AUDIO_PORTDEFINITIONTYPE *audioDef = &def.format.audio;

        switch ((int)audioDef->eEncoding) {
            case OMX_AUDIO_CodingQCELP13:
            {
                OMX_AUDIO_PARAM_QCELP13TYPE params;
                InitOMXParams(&params);
                params.nPortIndex = portIndex;

                CHECK_EQ(mOMXNode->getParameter(
                        OMX_IndexParamAudioQcelp13,
                        &params, sizeof(params)),
                        (status_t)OK);

                notify->setString("mime", MEDIA_MIMETYPE_AUDIO_QCELP);
                notify->setInt32("channel-count", params.nChannels);
                /* QCELP supports only 8k sample rate*/
                notify->setInt32("sample-rate", 8000);
                return OK;
            }
            case QOMX_AUDIO_CodingMPEGH:
            {
                return getMpeghPortFormat(portIndex, notify);
            }
            case OMX_AUDIO_CodingEVRC:
            {
                OMX_AUDIO_PARAM_EVRCTYPE params;
                InitOMXParams(&params);
                params.nPortIndex = portIndex;
                CHECK_EQ(mOMXNode->getParameter(
                        OMX_IndexParamAudioEvrc,
                        &params, sizeof(params)),
                        (status_t)OK);
                notify->setString("mime", MEDIA_MIMETYPE_AUDIO_EVRC);
                notify->setInt32("channel-count", params.nChannels);
                /* EVRC supports only 8k sample rate*/
                notify->setInt32("sample-rate", 8000);
                return OK;
            }
            case QOMX_IndexParamAudioAmrWbPlus:
            {
                OMX_INDEXTYPE index;
                QOMX_AUDIO_PARAM_AMRWBPLUSTYPE params;
                InitOMXParams(&params);
                AVLOGD("AMRWB format");
                params.nPortIndex = portIndex;
                mOMXNode->getExtensionIndex(OMX_QCOM_INDEX_PARAM_AMRWBPLUS, &index);
                CHECK_EQ(mOMXNode->getParameter(index, &params, sizeof(params)),(status_t)OK);
                notify->setString("mime", MEDIA_MIMETYPE_AUDIO_QC_AMR_WB_PLUS);
                notify->setInt32("channel-count", params.nChannels);
                notify->setInt32("sample-rate",  params.nSampleRate);
                return OK;
            }
          case OMX_AUDIO_CodingWMA:
          {
              status_t err = OK;
              OMX_INDEXTYPE index;
              OMX_AUDIO_PARAM_WMATYPE paramWMA;
              QOMX_AUDIO_PARAM_WMA10PROTYPE paramWMA10;

              InitOMXParams(&paramWMA);
              paramWMA.nPortIndex = portIndex;
              err = mOMXNode->getParameter(
                      OMX_IndexParamAudioWma, &paramWMA, sizeof(paramWMA));
              if(err == OK) {
                  AVLOGV("WMA format");
                  notify->setString("mime",MEDIA_MIMETYPE_AUDIO_WMA);
                  notify->setInt32("channel-count", paramWMA.nChannels);
                  notify->setInt32("sample-rate", paramWMA.nSamplingRate);
              } else {
                  InitOMXParams(&paramWMA10);
                  paramWMA10.nPortIndex = portIndex;
                  mOMXNode->getExtensionIndex("OMX.Qualcomm.index.audio.wma10Pro",&index);
                  CHECK_EQ(mOMXNode->getParameter(index, &paramWMA10, sizeof(paramWMA10)),(status_t)OK);
                  AVLOGV("WMA10 format");
                  notify->setString("mime",MEDIA_MIMETYPE_AUDIO_WMA);
                  notify->setInt32("channel-count", paramWMA10.nChannels);
                  notify->setInt32("sample-rate", paramWMA10.nSamplingRate);
              }
              return OK;
          }
          case QOMX_IndexParamAudioAlac:
          {
              OMX_INDEXTYPE index;
              QOMX_AUDIO_PARAM_ALACTYPE params;

              InitOMXParams(&params);
              params.nPortIndex = portIndex;
              mOMXNode->getExtensionIndex(OMX_QCOM_INDEX_PARAM_ALAC, &index);
              CHECK_EQ(mOMXNode->getParameter(index, &params, sizeof(params)),(status_t)OK);
              notify->setString("mime", MEDIA_MIMETYPE_AUDIO_ALAC);
              notify->setInt32("channel-count", params.nChannels);
              notify->setInt32("sample-rate", params.nSampleRate);
              return OK;
          }

          case QOMX_IndexParamAudioApe:
          {
              OMX_INDEXTYPE index;
              QOMX_AUDIO_PARAM_APETYPE params;
              status_t status = OK;

              InitOMXParams(&params);
              params.nPortIndex = portIndex;
              mOMXNode->getExtensionIndex(OMX_QCOM_INDEX_PARAM_APE, &index);
              status = mOMXNode->getParameter(index, &params, sizeof(params));
              if (status != OK) {
                  return status;
              }
              notify->setString("mime", MEDIA_MIMETYPE_AUDIO_APE);
              notify->setInt32("channel-count", params.nChannels);
              notify->setInt32("sample-rate",  params.nSampleRate);
              return OK;
          }
          case QOMX_IndexParamAudioDsdDec:
          {
              OMX_INDEXTYPE index;
              QOMX_AUDIO_PARAM_DSD_TYPE params;
              status_t status = OK;

              InitOMXParams(&params);
              params.nPortIndex = portIndex;
              mOMXNode->getExtensionIndex(OMX_QCOM_INDEX_PARAM_DSD, &index);
              status = mOMXNode->getParameter(index, &params, sizeof(params));
              if (status != OK) {
                  return status;
              }
              notify->setString("mime", MEDIA_MIMETYPE_AUDIO_DSD);
              notify->setInt32("channel-count", params.nChannels);
              notify->setInt32("sample-rate",  params.nSampleRate);
              return OK;
          }
          default:
              // Let ACodec handle the rest
              break;
        }
    }

    return ACodec::getPortFormat(portIndex, notify);
}

void ExtendedACodec::setBFrames(OMX_VIDEO_PARAM_MPEG4TYPE *mpeg4type) {
    //ignore non QC components
    if (strncmp(mComponentName.c_str(), "OMX.qcom.", 9) || mpeg4type == NULL) {
        return;
    }
    if (mpeg4type->eProfile > OMX_VIDEO_MPEG4ProfileSimple) {
        mpeg4type->nPFrames = (mpeg4type->nPFrames + kNumBFramesPerPFrame) /
                (kNumBFramesPerPFrame + 1);
        mpeg4type->nBFrames = mpeg4type->nPFrames * kNumBFramesPerPFrame;
        if (mpeg4type->nBFrames) {
            mpeg4type->nAllowedPictureTypes |= OMX_VIDEO_PictureTypeB;
        }
    }
    return;
}

status_t ExtendedACodec::setupErrorCorrectionParameters() {
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrectionType;
    InitOMXParams(&errorCorrectionType);
    errorCorrectionType.nPortIndex = kPortIndexOutput;

    status_t err = mOMXNode->getParameter(
            OMX_IndexParamVideoErrorCorrection,
            &errorCorrectionType, sizeof(errorCorrectionType));

    if (err != OK) {
        return OK;  // Optional feature. Ignore this failure
    }

    errorCorrectionType.bEnableHEC = OMX_FALSE;
    errorCorrectionType.bEnableResync = OMX_FALSE;
    errorCorrectionType.nResynchMarkerSpacing = 0;
    errorCorrectionType.bEnableDataPartitioning = OMX_FALSE;
    errorCorrectionType.bEnableRVLC = OMX_FALSE;

    return mOMXNode->setParameter(
            OMX_IndexParamVideoErrorCorrection,
            &errorCorrectionType, sizeof(errorCorrectionType));
}

status_t ExtendedACodec::configureFramePackingFormat(
        const sp<AMessage> &msg) {
    status_t err = OK;
    int32_t mode = 0;
    OMX_QCOM_PARAM_PORTDEFINITIONTYPE portFmt;
    InitOMXParams(&portFmt);
    portFmt.nPortIndex = kPortIndexInput;

    if (msg->findInt32("use-arbitrary-mode", &mode) && mode) {
        AVLOGI("Decoder will be in arbitrary mode");
        portFmt.nFramePackingFormat = OMX_QCOM_FramePacking_Arbitrary;
    } else {
        AVLOGI("Decoder will be in frame by frame mode");
        portFmt.nFramePackingFormat = OMX_QCOM_FramePacking_OnlyOneCompleteFrame;
    }
    err = mOMXNode->setParameter(
            (OMX_INDEXTYPE)OMX_QcomIndexPortDefn,
            (void *)&portFmt, sizeof(portFmt));
    if (err != OK) {
        AVLOGW("Failed to set frame packing format on component");
    }
    return err;
}

status_t ExtendedACodec::setDIVXFormat(
        const sp<AMessage> &msg, const char* mime) {
    status_t err = OK;

    if (!strcasecmp(MEDIA_MIMETYPE_VIDEO_DIVX, mime) ||
            !strcasecmp(MEDIA_MIMETYPE_VIDEO_DIVX4, mime) ||
            !strcasecmp(MEDIA_MIMETYPE_VIDEO_DIVX311, mime)) {
        AVLOGD("Setting QOMX_VIDEO_PARAM_DIVXTYPE params");
        QOMX_VIDEO_PARAM_DIVXTYPE paramDivX;
        InitOMXParams(&paramDivX);
        paramDivX.nPortIndex = kPortIndexOutput;
        int32_t DivxVersion = 0;

        if (!msg->findInt32(ExtendedUtils::getMsgFromKey(kKeyDivXVersion), &DivxVersion)) {
            DivxVersion = kTypeDivXVer_4;
            AVLOGW("Divx version key missing, initializing the version to %d", DivxVersion);
        }

        AVLOGD("Divx Version Type %d", DivxVersion);

        if (DivxVersion == kTypeDivXVer_4) {
            paramDivX.eFormat = QOMX_VIDEO_DIVXFormat4;
        } else if (DivxVersion == kTypeDivXVer_5) {
            paramDivX.eFormat = QOMX_VIDEO_DIVXFormat5;
        } else if (DivxVersion == kTypeDivXVer_6) {
            paramDivX.eFormat = QOMX_VIDEO_DIVXFormat6;
        } else if (DivxVersion == kTypeDivXVer_3_11 ) {
            paramDivX.eFormat = QOMX_VIDEO_DIVXFormat311;
        } else {
            paramDivX.eFormat = QOMX_VIDEO_DIVXFormatUnused;
        }
        paramDivX.eProfile = (QOMX_VIDEO_DIVXPROFILETYPE)0;    //Not used for now.

        err = mOMXNode->setParameter((OMX_INDEXTYPE)OMX_QcomIndexParamVideoDivx,
                &paramDivX, sizeof(paramDivX));
    }

    return err;
}

bool ExtendedACodec::getDSModeHint(
        const sp<AMessage>& msg, int64_t timeUs) {

    if (strncmp(mComponentName.c_str(), "OMX.qcom.video.",
            strlen("OMX.qcom.video.")) || msg == NULL || !mVideoDSModeSupport) {
        return false;
    }

    OMX_INDEXTYPE nIndex;
    QOMX_ENABLETYPE dsMode;
    status_t err = OK;

    InitOMXParams(&dsMode);
    dsMode.bEnable = OMX_FALSE;

    if(mOMXNode->getExtensionIndex("OMX.QTI.index.config.video.getdsmode",
            &nIndex) != OMX_ErrorNone) {
        AVLOGV("getDSModeHint: getdsmode not supported");
        mVideoDSModeSupport = false;
        return false;
    }

    err = mOMXNode->getConfig(nIndex,
            (void* )&dsMode, sizeof(dsMode));

    if (timeUs) {

        if( mOMXNode->getExtensionIndex ("OMX.QTI.index.config.video.settimedata",
                &nIndex) != OMX_ErrorNone) {
            AVLOGV("getDSModeHint: settimedata not supported");
            return false;
        }

        int64_t timestampNs = 0;

        OMX_TIME_CONFIG_TIMESTAMPTYPE timeStampInfo;
        InitOMXParams(&timeStampInfo);

        if (msg->findInt64("timestampNs", &timestampNs)) {
            timeStampInfo.nTimestamp = timestampNs / 1000;
            timeStampInfo.nPortIndex = 0;
            AVLOGV("getDSModeHint: nRealTime = %lld", timeStampInfo.nTimestamp);

            err = mOMXNode->setConfig(nIndex,
                    (void*)&timeStampInfo,
                    sizeof(timeStampInfo));
        }

        timeStampInfo.nTimestamp = timeUs;
        timeStampInfo.nPortIndex = 1;
        AVLOGV("getDSModeHint: nTimeStamp = %lld", timeStampInfo.nTimestamp);

        err = mOMXNode->setConfig(nIndex,
                (void*)&timeStampInfo,
                sizeof(timeStampInfo));

        if (err != OK) {
            AVLOGV("getDSModeHint: Failed sendRenderingTime nTimeStamp = %lld",
                    timeStampInfo.nTimestamp);
        }
    }

    /* Cancel frames for rendering */
    if(dsMode.bEnable == OMX_TRUE) {
        return true;
    } else {
        mVideoDSModeSupport = false;
    }

    return false;
}

status_t ExtendedACodec::setDitherControl(
    const sp<AMessage> &msg) {

    status_t err = OK;

    AVLOGV("Setting of dither control");
    int32_t value = 0;
    bool paramsSet = false;
    paramsSet = msg->findInt32("dither-type", (int32_t *)&value);
    AVLOGV("dither-type search is %d and value is %d", paramsSet, value);

    if (value < QOMX_DITHER_DISABLE || value > QOMX_DITHER_ALL_COLORSPACE) {
        value = QOMX_DITHER_ALL_COLORSPACE;
        AVLOGW("Dither type outside the range, default to QOMX_DITHER_ALL_COLORSPACE");
    }

    if (paramsSet) {
        /*Pass the dither type to the OMX component */
        QOMX_VIDEO_DITHER_CONTROL ditherCtrl;
        InitOMXParams(&ditherCtrl);
        err = mOMXNode->getParameter((OMX_INDEXTYPE)OMX_QTIIndexParamDitherControl,
                                 (void*)&ditherCtrl, sizeof(ditherCtrl));
        if(err != OK) {
            AVLOGW("Failed to get dither type flag");
            return err;
        }
        ditherCtrl.eDitherType = (QOMX_VIDEO_DITHERTYPE)value;
        err = mOMXNode->setParameter((OMX_INDEXTYPE)OMX_QTIIndexParamDitherControl,
                                 (void *)&ditherCtrl, sizeof(ditherCtrl));
        if (err != OK) {
            AVLOGW("Failed to set dither type flag");
            return err;
        }
    }
    return err;
}

status_t ExtendedACodec::submitOutputMetadataBuffer() {
    if (!mIsOmxVppAlloc) {
        return ACodec::submitOutputMetadataBuffer();
    }
    AVLOGV("submitOutputMetadataBuffer");
    uint32_t extraBuffersToSubmit = 2;
    status_t err = OK;
    while (extraBuffersToSubmit > 0 && err == OK) {
       err = ACodec::submitOutputMetadataBuffer();
       extraBuffersToSubmit--;
    }
    return err;
}

status_t ExtendedACodec::setVppVendorExtension(
        const sp<AMessage> &msg) {
    status_t err = OK;
    sp<AMessage> vppMsg = new AMessage;
    for (size_t i = 0; i < msg->countEntries(); i++) {
        AMessage::Type keyType;
        const char* key = msg->getEntryNameAt(i, &keyType);
        if (key != nullptr && !strncmp(key, "vendor.qti-ext-vpp", 18)) {
            AVLOGV("find vpp vendor message: %s", key);
            switch(keyType) {
            case AMessage::kTypeInt32:
            {
                int32_t value;
                if (msg->findInt32(key, &value))
                    vppMsg->setInt32(key, value);
                break;
            }
            case AMessage::kTypeString:
            {
                AString value;
                if (msg->findString(key, &value))
                    vppMsg->setString(key, value);
                break;
            }
            default:
                break;
            }
        }
    }
    AVLOGV("vpp message = %s", vppMsg->debugString().c_str());
    if (vppMsg->countEntries() > 0) {
        AVLOGV("set vpp vendor extension");
        err = setVendorParameters(vppMsg);
    }
    return err;
}


void ExtendedACodec::setupOmxVpp(
        const sp<AMessage> &msg) {
    if (!mComponentName.startsWith("OMX.qcom.video.decoder.")
            && !mComponentName.startsWith("OMX.qti.video.decoder."))
        return;
    // Check whether OmxVpp component is already allocated
    QOMX_VPP_ENABLE omxVppEnable;
    InitOMXParams(&omxVppEnable);
    omxVppEnable.enable_vpp = OMX_FALSE;
    status_t err = mOMXNode->getParameter((OMX_INDEXTYPE)OMX_QcomIndexParamEnableVpp,
            (void *)&omxVppEnable, sizeof(omxVppEnable));
    if (err == OK && omxVppEnable.enable_vpp == OMX_TRUE) {
        AVLOGV("omx vpp is already allocated");
        mIsOmxVppAlloc = true;
    }
    const char* vppVendorName = "vendor.qti-ext-vpp.mode";
    AString mode;
    if (!msg->findString(vppVendorName, &mode))
        return;
    if (!mode.equalsIgnoreCase("HQV_MODE_AUTO")
            && !mode.equalsIgnoreCase("HQV_MODE_MANUAL"))
        return;

    if (!mIsOmxVppAlloc) {
        AVLOGV("reallocate omx vpp component");
        sp<IOMXNode> backupNode = mOMXNode;
        sp<IOMXNode> omxVppNode;
        sp<IOMXObserver> observer = createObserver();
        err = mOMX->allocateNode("OMX.qti.vdec.vpp", observer, &omxVppNode);
        if (err != OK || omxVppNode == NULL) {
            AVLOGW("failed to reallocate omx vpp component: %d", err);
            if(omxVppNode != NULL)
                omxVppNode->freeNode();
            return;
        }
        backupNode->freeNode();
        mOMXNode = omxVppNode;
        msg->setString("vendor.qti-ext-vpp.cmp", mComponentName);
        AVLOGV("reallocate omx vpp component done");
        mIsOmxVppAlloc = true;
        mNodeGeneration++;
    }
    err = setVppVendorExtension(msg);
    AVLOGV("set vpp vendor ext: %d", err);
}

status_t ExtendedACodec::setOutputFrameRateVendorExtension(
        const sp<AMessage> &msg) {
    status_t err = OK;
    sp<AMessage> outputFpsMsg = new AMessage;
    for (size_t i = 0; i < msg->countEntries(); i++) {
        AMessage::Type keyType;
        const char* key = msg->getEntryNameAt(i, &keyType);
        if (key != nullptr && !strncmp(key, "output-frame-rate", 18)) {
            AVLOGV("find vendor message: %s", key);
            int32_t value;
            if (msg->findInt32(key, &value)) {
                outputFpsMsg->setInt32("vendor.qti-ext-dec-output-frame-rate", value);
                break;
            }
        }
    }
    if (outputFpsMsg->countEntries() > 0) {
        AVLOGV("set output-frame-rate vendor extension");
        err = setVendorParameters(outputFpsMsg);
    }
    return err;
}

}  // namespace android
