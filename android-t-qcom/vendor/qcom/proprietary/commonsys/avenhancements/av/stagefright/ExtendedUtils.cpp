/*
 * Copyright (c) 2015-2022 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
/*
 * Copyright (c) 2013 - 2015, The Linux Foundation. All rights reserved.
 */
/* Not a Contribution.

 * Copyright (C) 2009 The Android Open Source Project
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
#define LOG_TAG "ExtendedUtils"
#include <common/AVLog.h>
#include <cutils/properties.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MetaData.h>
#include <HevcUtils.h>
#include <media/openmax/OMX_Audio.h>
#include <media/AudioSystem.h>
#include <media/mediarecorder.h> //for enum audio_encoder
#include <QCMetaData.h>
#include <media/hardware/HardwareAPI.h>
#include <media/stagefright/MPEG4Writer.h>
#include <media/AudioParameter.h>
#include <QCMetaData.h>
#include <OMX_QCOMExtns.h>
#include <OMX_Component.h>
#include <OMX_VideoExt.h>
#include <OMX_IndexExt.h>
#include <OMX_Index.h>

#include <QOMX_AudioExtensions.h>
#include <QOMX_AudioIndexExtensions.h>
#include <media/stagefright/foundation/avc_utils.h>
#include <stagefright/AVExtensions.h>
#include "stagefright/ExtendedUtils.h"
#include <QOMX_AudioExtensions.h>
#include <camera/CameraParameters.h>

#include <camera/CameraParameters.h>
#include <binder/IPCThreadState.h>

#define  AUDIO_PARAMETER_IS_HW_DECODER_SESSION_AVAILABLE  "is_hw_dec_session_available"

static const uint8_t kHEVCNalUnitTypeIDR         = 0x13;
static const uint8_t kHEVCNalUnitTypeIDRNoLP     = 0x14;
static const uint8_t kHEVCNalUnitTypeCRA         = 0x15;
static const uint8_t kHEVCNalUnitTypeVidParamSet = 0x20;
static const uint8_t kHEVCNalUnitTypeSeqParamSet = 0x21;
static const uint8_t kHEVCNalUnitTypePicParamSet = 0x22;
static const uint32_t kBufferTimestampOffset     = 33333; //33ms
static const int32_t MAX_BITRATE_VORBIS          = 500000;

namespace android {

AVUtils *createExtendedUtils() {
    return new ExtendedUtils;
}

ExtendedUtils::ExtendedUtils() {
    updateLogLevel();
    AVLOGD("ExtendedUtils()");
}

ExtendedUtils::~ExtendedUtils() {
    AVLOGD("~ExtendedUtils()");
}

struct mime_conv_t {
    const char* mime;
    audio_format_t format;
};

static const struct mime_conv_t mimeLookup[] = {
    { MEDIA_MIMETYPE_AUDIO_FLAC,        AUDIO_FORMAT_FLAC },
    { MEDIA_MIMETYPE_CONTAINER_QTIFLAC, AUDIO_FORMAT_FLAC },
    { MEDIA_MIMETYPE_AUDIO_ALAC,        AUDIO_FORMAT_ALAC },
    { MEDIA_MIMETYPE_AUDIO_WMA,         AUDIO_FORMAT_WMA },
    { MEDIA_MIMETYPE_AUDIO_APE,         AUDIO_FORMAT_APE  },
    { MEDIA_MIMETYPE_AUDIO_EVRC,        AUDIO_FORMAT_EVRC },
    { MEDIA_MIMETYPE_AUDIO_QCELP,       AUDIO_FORMAT_QCELP },
    { MEDIA_MIMETYPE_AUDIO_AMR_WB_PLUS, AUDIO_FORMAT_AMR_WB_PLUS },
    { MEDIA_MIMETYPE_AUDIO_QC_AMR_WB_PLUS, AUDIO_FORMAT_AMR_WB_PLUS },
    { MEDIA_MIMETYPE_AUDIO_DSD,         AUDIO_FORMAT_DSD },
    { MEDIA_MIMETYPE_CONTAINER_DSF,     AUDIO_FORMAT_DSD },
    { MEDIA_MIMETYPE_CONTAINER_DFF,     AUDIO_FORMAT_DSD },
    { MEDIA_MIMETYPE_AUDIO_EAC3,        AUDIO_FORMAT_E_AC3},
    { MEDIA_MIMETYPE_AUDIO_MPEG_LAYER_II,        AUDIO_FORMAT_MP2},
    { 0, AUDIO_FORMAT_INVALID }
};
enum MetaKeyType{
    INT32, INT64, STRING, DATA, CSD
};

struct MetaKeyEntry{
    int MetaKey;
    const char* MsgKey;
    MetaKeyType KeyType;
};

static const MetaKeyEntry MetaKeyTable[] {
   {kKeyBitRate              , "bitrate"                , INT32},
   {kKeyAacCodecSpecificData , "aac-codec-specific-data", CSD},
   {kKeyRawCodecSpecificData , "raw-codec-specific-data", CSD},
   {kKeyVorbisData           , "vorbis-data"            , DATA},
   {kKeyDivXVersion          , "divx-version"           , INT32},  // int32_t
   {kKeyDivXDrm              , "divx-drm"               , DATA},  // void *
   {kKeyWMAEncodeOpt         , "wma-encode-opt"         , INT32},  // int32_t
   {kKeyWMAEncodeOptC2       , "vendor.qti-wma-config.encOptions",    INT32},  // int32_t
   {kKeyWMABlockAlign        , "wma-block-align"        , INT32},  // int32_t
   {kKeyWMABlockAlignC2      , "vendor.qti-wma-config.blkAlign",    INT32},  // int32_t
   {kKeyWMAVersion           , "wma-version"            , INT32},  // int32_t
   {kKeyWMAVersionC2         , "vendor.qti-wma-version.value",     INT32},  // int32_t
   {kKeyWMAAdvEncOpt1        , "wma-adv-enc-opt1"       , INT32},  // int16_t
   {kKeyWMAAdvEncOpt1C2      , "vendor.qti-wma-config.advEncOption",    INT32},  // int16_t
   {kKeyWMAAdvEncOpt2        , "wma-adv-enc-opt2"       , INT32},  // int32_t
   {kKeyWMAAdvEncOpt2C2      , "vendor.qti-wma-config.advEncOptions2",    INT32},  // int32_t
   {kKeyWMAFormatTag         , "wma-format-tag"         , INT32},  // int32_t
   {kKeyWMAFormatTagC2       , "vendor.qti-wma-config.formatTag",    INT32},  // int32_t
   {kKeyWMABitspersample     , "wma-bits-per-sample"    , INT32},  // int32_t
   {kKeyWMABitspersampleC2   , "vendor.qti-wma-config.bitsPerSample",    INT32},  // int32_t
   {kKeyWMAVirPktSize        , "wma-vir-pkt-size"       , INT32},  // int32_t
   {kKeyWMAChannelMask       , "wma-channel-mask"       , INT32},  // int32_t
   {kKeyWMAChannelMaskC2     , "vendor.qti-wma-config.channelMask",    INT32},  // int32_t
   // Change to CSD after convertMetaDataToFormat() has generic implementation for CSD
   {kKeyMHAConfig            , "mhaconfig-codec-specific-data",    DATA},
   // Change to CSD after convertMetaDataToFormat() has generic implementation for CSD
   {kKeyMHASceneInfo         , "mhasceneinfo-codec-specific-data", DATA},

   {kKeyFileFormat           , "file-format"            , STRING},  // cstring

   {kkeyAacFormatAdif        , "aac-format-adif"        , INT32},  // bool (int32_t)
   {kkeyAacFormatLtp         , "aac-format-ltp"         , INT32},

   //DTS subtype
   {kKeyDTSSubtype           , "dts-subtype"            , INT32},  //int32_t

   //Extractor sets this
   {kKeyUseArbitraryMode     , "use-arbitrary-mode"     , INT32},  //bool (int32_t)
   {kKeySmoothStreaming      , "smooth-streaming"       , INT32},  //bool (int32_t)
   {kKeyHFR                  , "hfr"                    , INT32},  // int32_t
   {kKeyTimestampReorder     , "vendor.qti-ext-dec-timestamp-reorder.value", INT32},

   {kKeySampleRate           , "sample-rate"            , INT32},
   {kKeyChannelCount         , "channel-count"          , INT32},
   {kKeyMinBlkSize           , "min-block-size"         , INT32},
   {kKeyMaxBlkSize           , "max-block-size"         , INT32},
   {kKeyMinFrmSize           , "min-frame-size"         , INT32},
   {kKeyMaxFrmSize           , "max-frame-size"         , INT32},
   {kKeySampleBits           , "bits-per-sample"        , INT32},
   {kKeyIsByteMode           , "is-byte-stream-mode"    , INT32},
   {kKeyFeatureNalLengthBitstream  , "feature-nal-length-bitstream" , INT32},
   {kKeyAACAOT               , "aac-profile"            , INT32},
};

// static
static inline sp<ABuffer> copyExtradata(void *src, size_t src_size) {
    sp<ABuffer> dst = new ABuffer(src_size);

    if (dst != NULL) {
        memcpy(dst->base(), src, src_size);
        dst->setRange(0, src_size);
    }

    return dst;
}

// static
const char* ExtendedUtils::getMsgFromKey(int key) {
    static const size_t numMetaKeys =
                     sizeof(MetaKeyTable) / sizeof(MetaKeyTable[0]);
    size_t i;
    for (i = 0; i < numMetaKeys; ++i) {
        if (key == MetaKeyTable[i].MetaKey) {
            return MetaKeyTable[i].MsgKey;
        }
    }
    return "unknown";
}

static size_t reassembleNalLengthBitstreamAVCC(const sp<ABuffer> &csd0, const sp<ABuffer> &csd1, char *avcc) {
    avcc[0] = 1;        // version
    avcc[1] = 0x64;     // profile (default to high)
    avcc[2] = 0;        // constraints (default to none)
    avcc[3] = 0xd;      // level (default to 1.3)
    avcc[4] = 0xff;     // reserved+size

    size_t i = 0;
    int numparams = 0;
    int lastparamoffset = 0;
    int avccidx = 6;
    uint8_t *data = csd0->data();
    do {
        int32_t nalSize = 0;
        std::copy(data, data+4, reinterpret_cast<uint8_t *>(&nalSize));
        nalSize = ntohl(nalSize);
        const uint8_t *lastparam = data + 4;
        if (nalSize > 3) {
            if (numparams && memcmp(avcc + 1, lastparam + 1, 3)) {
                    ALOGW("Inconsisted profile/level found in SPS: %x,%x,%x vs %x,%x,%x",
                            avcc[1], avcc[2], avcc[3], lastparam[1], lastparam[2], lastparam[3]);
            } else if (!numparams) {
                // fill in profile, constraints and level
                memcpy(avcc + 1, lastparam + 1, 3);
            }
        }
        avcc[avccidx++] = nalSize >> 8;
        avcc[avccidx++] = nalSize & 0xff;
        memcpy(avcc+avccidx, lastparam, nalSize);
        avccidx += nalSize;
        numparams++;

        i += nalSize + 4;
        data += i;
    } while(i < csd0->size());
    ALOGV("csd0 contains %d params", numparams);

    avcc[5] = 0xe0 | numparams;
    //and now csd-1
    i = 0;
    numparams = 0;
    lastparamoffset = 0;
    int numpicparamsoffset = avccidx;
    avccidx++;
    data = csd1->data();
    do {
        int32_t nalSize = 0;
        std::copy(data, data+4, reinterpret_cast<uint8_t *>(&nalSize));
        nalSize = ntohl(nalSize);
        avcc[avccidx++] = nalSize >> 8;
        avcc[avccidx++] = nalSize & 0xff;
        memcpy(avcc+avccidx, data + 4, nalSize);
        avccidx += nalSize;
        numparams++;

        i += nalSize + 4;
        data += i;
    } while(i < csd1->size());
    avcc[numpicparamsoffset] = numparams;
    return avccidx;
}

static size_t reassembleNalLengthBitstreamHVCC(const sp<ABuffer> &csd0, uint8_t *hvcc,
        size_t hvccSize, size_t nalSizeLength) {
    HevcParameterSets paramSets;
    uint8_t* data = csd0->data();
    if (csd0->size() < 4) {
        ALOGE("csd0 too small");
        return 0;
    }
    if (!memcmp(data, "\x00\x00\x00\x01", 4)) {
        ALOGE("csd0 doesn't start with a nal length");
        return 0;
    }

    status_t err = OK;
    uint32_t bytesLeft = csd0->size();
    while  (bytesLeft > 4) {
        uint32_t nalSize = 0;
        std::copy(data, data+4, reinterpret_cast<uint8_t *>(&nalSize));
        nalSize = ntohl(nalSize);

        err = paramSets.addNalUnit(data + 4, nalSize);
        if (err != OK) {
            return ERROR_MALFORMED;
        }

        bytesLeft -= (nalSize + 4);
        data += nalSize + 4;
    }

    size_t size = hvccSize;
    err = paramSets.makeHvcc(hvcc, &size, nalSizeLength);
    if (err != OK) {
        return 0;
    }
    return size;
}

status_t ExtendedUtils::convertMetaDataToMessage(
        const MetaDataBase *meta, sp<AMessage> *format) {
    CHECK (format != NULL);

    const char *str_val;
    int32_t int32_val;
    int64_t int64_val;
    uint32_t data_type;
    const void *data;
    size_t size;
    static const size_t numMetaKeys =
                     sizeof(MetaKeyTable) / sizeof(MetaKeyTable[0]);

    size_t i;
    for (i = 0; i < numMetaKeys; ++i) {
        if (MetaKeyTable[i].KeyType == INT32 &&
            meta->findInt32(MetaKeyTable[i].MetaKey, &int32_val)) {
            AVLOGV("convert: int32 : key=%s value=%d", MetaKeyTable[i].MsgKey, int32_val);
            format->get()->setInt32(MetaKeyTable[i].MsgKey, int32_val);
        } else if (MetaKeyTable[i].KeyType == INT64 &&
                 meta->findInt64(MetaKeyTable[i].MetaKey, &int64_val)) {
            AVLOGV("convert int64 : key=%s value=%" PRId64 "", MetaKeyTable[i].MsgKey, int64_val);
            format->get()->setInt64(MetaKeyTable[i].MsgKey, int64_val);
        } else if (MetaKeyTable[i].KeyType == STRING &&
                 meta->findCString(MetaKeyTable[i].MetaKey, &str_val)) {
            AVLOGV("convert: string : key=%s value=%s", MetaKeyTable[i].MsgKey, str_val);
            format->get()->setString(MetaKeyTable[i].MsgKey, str_val);
        } else if ( (MetaKeyTable[i].KeyType == DATA ||
                   MetaKeyTable[i].KeyType == CSD) &&
                   meta->findData(MetaKeyTable[i].MetaKey, &data_type, &data, &size)) {
            AVLOGV("convert: data : key=%s size=%zu", MetaKeyTable[i].MsgKey, size);
            if (MetaKeyTable[i].KeyType == CSD) {
                const char *mime;
                CHECK(meta->findCString(kKeyMIMEType, &mime));
                if (strcasecmp(mime, MEDIA_MIMETYPE_VIDEO_AVC)) {
                    sp<ABuffer> buffer = new ABuffer(size);
                    memcpy(buffer->data(), data, size);
                    buffer->meta()->setInt32("csd", true);
                    buffer->meta()->setInt64("timeUs", 0);
                    format->get()->setBuffer("csd-0", buffer);
                } else {
                    const uint8_t *ptr = (const uint8_t *)data;
                    CHECK(size >= 8);
                    int seqLength = 0, picLength = 0;
                    for (size_t i = 4; i < (size - 4); i++)
                    {
                        if ((*(ptr + i) == 0) && (*(ptr + i + 1) == 0) &&
                           (*(ptr + i + 2) == 0) && (*(ptr + i + 3) == 1))
                            seqLength = i;
                    }
                    sp<ABuffer> buffer = new ABuffer(seqLength);
                    memcpy(buffer->data(), data, seqLength);
                    buffer->meta()->setInt32("csd", true);
                    buffer->meta()->setInt64("timeUs", 0);
                    format->get()->setBuffer("csd-0", buffer);
                    picLength=size-seqLength;
                    sp<ABuffer> buffer1 = new ABuffer(picLength);
                    memcpy(buffer1->data(), (const uint8_t *)data + seqLength, picLength);
                    buffer1->meta()->setInt32("csd", true);
                    buffer1->meta()->setInt64("timeUs", 0);
                    format->get()->setBuffer("csd-1", buffer1);
                }
            } else {
                sp<ABuffer> buffer = new ABuffer(size);
                memcpy(buffer->data(), data, size);
                format->get()->setBuffer(MetaKeyTable[i].MsgKey, buffer);
            }
        }
    }
    return OK;
}

status_t ExtendedUtils::convertMetaDataToMessage(
        const sp<MetaData> &meta, sp<AMessage> *format) {
    return convertMetaDataToMessage (meta.get(), format);
}

status_t ExtendedUtils::convertMessageToMetaData(
        const sp<AMessage> &msg, sp<MetaData> &meta) {
    CHECK ((msg != NULL) && (meta != NULL));

    AString str_val;
    int32_t int32_val;
    int64_t int64_val;
    sp<ABuffer> buf_val = NULL;
    static const size_t numMetaKeys =
        sizeof(MetaKeyTable) / sizeof(MetaKeyTable[0]);

    for (size_t i = 0; i < numMetaKeys; ++i) {
        if (MetaKeyTable[i].KeyType == INT32 &&
                msg->findInt32(MetaKeyTable[i].MsgKey, &int32_val)) {
            meta->setInt32(MetaKeyTable[i].MetaKey, int32_val);
        } else if (MetaKeyTable[i].KeyType == INT64 &&
                msg->findInt64(MetaKeyTable[i].MsgKey, &int64_val)) {
            meta->setInt64(MetaKeyTable[i].MetaKey, int64_val);
        } else if (MetaKeyTable[i].KeyType == STRING &&
                msg->findString(MetaKeyTable[i].MsgKey, &str_val)) {
            meta->setCString(MetaKeyTable[i].MetaKey, str_val.c_str());
        } else if (MetaKeyTable[i].KeyType == CSD &&
                msg->findBuffer(MetaKeyTable[i].MsgKey, &buf_val)) {
            if (buf_val == NULL)
                ALOGE("buffer not found");
            else
                meta->setData(MetaKeyTable[i].MetaKey, 'none', buf_val->data(), buf_val->size());
        }
    }

    AString mime;
    if (msg->findString("mime", &mime)) {
        meta->setCString(kKeyMIMEType, mime.c_str());
    } else {
        ALOGW("did not find mime type");
    }

    int32_t nalLengthBitstream = 0;
    msg->findInt32("feature-nal-length-bitstream", &nalLengthBitstream);
    sp<ABuffer> csd0, csd1, csd2;
    if (msg->findBuffer("csd-0", &csd0)) {
        uint8_t* data = csd0->data();
        if (csd0->size() < 4) {
            ALOGE("csd0 too small");
            nalLengthBitstream = 0;
        }
        if (nalLengthBitstream && !memcmp(data, "\x00\x00\x00\x01", 4)) {
            nalLengthBitstream = 0;
        }
    }

    if (msg->findBuffer("csd-0", &csd0) && nalLengthBitstream) {
        int csd0size = csd0->size();
        if (mime == MEDIA_MIMETYPE_VIDEO_AVC) {
            sp<ABuffer> csd1;
            if (msg->findBuffer("csd-1", &csd1)) {
                std::vector<char> avcc(csd0size + csd1->size() + 1024);
                size_t outsize = reassembleNalLengthBitstreamAVCC(csd0, csd1, avcc.data());
                meta->setData(kKeyAVCC, kKeyAVCC, avcc.data(), outsize);
            }
        } else if (mime == MEDIA_MIMETYPE_VIDEO_HEVC ||
                   mime == MEDIA_MIMETYPE_IMAGE_ANDROID_HEIC) {
            std::vector<uint8_t> hvcc(csd0size + 1024);
            size_t outsize = reassembleNalLengthBitstreamHVCC(csd0, hvcc.data(), hvcc.size(), 4);
            meta->setData(kKeyHVCC, kKeyHVCC, hvcc.data(), outsize);
        }
    }

    return OK;
}

bool ExtendedUtils::isHwAudioDecoderSessionAllowed(const sp<AMessage> &format) {

    if (format != NULL) {
        int isallowed = 0;
        String8  hal_result,hal_query;
        AString mime;

        hal_query = AUDIO_PARAMETER_IS_HW_DECODER_SESSION_AVAILABLE;
        CHECK(format->findString("mime", &mime));

        if (!mime.c_str()) {
            AVLOGE("voice_conc: null mime type string");
            return true;
        }

        //Append mime type to query
        hal_query += "=";
        hal_query +=  mime.c_str();

        //Check with HAL whether new session can be created for DSP only decoding formats
        hal_result =
                AudioSystem::getParameters((audio_io_handle_t)0, hal_query);
        AudioParameter result = AudioParameter(hal_result);
        if (result.getInt(
                    String8(AUDIO_PARAMETER_IS_HW_DECODER_SESSION_AVAILABLE),
                    isallowed) == NO_ERROR) {
            if (!isallowed) {
                AVLOGD("voice_conc:ExtendedUtils : Rejecting audio decoder"
                        " session request for %s ", mime.c_str());
                return false;
            }
        }
    }
    return true;
}


sp<MediaCodec> ExtendedUtils::createCustomComponentByName(const sp<ALooper> &looper,
        const char* mime, bool encoder, const sp<AMessage> &msg) {
    sp<MediaCodec> codec = NULL;
    AString mimeType;

    if (!mime) {
        AVLOGE("mime type is not set");
        return NULL;
    }

    bool preferC2Codecs = property_get_bool("vendor.audio.c2.preferred", false);

    if (preferC2Codecs) {
        AVLOGV("CCodec preferred, skip creation of custom OMX audio components");
        return NULL;
    }

    mimeType.setTo(mime);
    if ( msg != NULL && !overrideMimeType(msg, &mimeType)) {
        if (mime != NULL) {
            pConfigsIns = AVConfigHelper::getInstance();
            AVLOGV("createByComponentName for clip of mimetype %s", mime);
            if ( (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_WB_PLUS) ||
                !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_QC_AMR_WB_PLUS) ) && !encoder) {
                codec = MediaCodec::CreateByComponentName(looper, "OMX.qcom.audio.decoder.amrwbplus");
            } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_WMA) && !encoder) {
                codec = MediaCodec::CreateByComponentName(looper, "OMX.qcom.audio.decoder.wma");
            } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_ALAC) && !encoder) {
                if (pConfigsIns->useSwAlacDecoder()) {
                    codec = MediaCodec::CreateByComponentName(looper, "OMX.qti.audio.decoder.alac.sw");
                }
                else {
                    codec = MediaCodec::CreateByComponentName(looper, "OMX.qcom.audio.decoder.alac");
                }
            } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_APE) && !encoder) {
                if (pConfigsIns->useSwApeDecoder()) {
                    codec = MediaCodec::CreateByComponentName(looper, "OMX.qti.audio.decoder.ape.sw");
                }
                else {
                    codec = MediaCodec::CreateByComponentName(looper, "OMX.qcom.audio.decoder.ape");
                }
            } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_QCELP) && !encoder) {
                codec = MediaCodec::CreateByComponentName(looper, "OMX.qcom.audio.decoder.Qcelp13");
            } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_EVRC) && !encoder) {
                codec = MediaCodec::CreateByComponentName(looper, "OMX.qcom.audio.decoder.evrc");
            } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_DSD) && !encoder) {
                codec = MediaCodec::CreateByComponentName(looper, "OMX.qti.audio.decoder.dsd");
            } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MHAS) && !encoder) {
                if (pConfigsIns->useSwMpeghDecoder()) {
                    codec = MediaCodec::CreateByComponentName(looper, "OMX.qti.audio.decoder.mpegh");
                } else {
                    AVLOGE("fail to create mpegh decoder as disabled by property");
                }
            } else {
                  AVLOGV("Could not create by component name");
            }
        }
    } else {
           if (!strcasecmp(mimeType.c_str(), MEDIA_MIMETYPE_AUDIO_WMA_PRO) && !encoder) {
                codec = MediaCodec::CreateByComponentName(looper, "OMX.qcom.audio.decoder.wma10Pro");
            } else if (!strcasecmp(mimeType.c_str(), MEDIA_MIMETYPE_AUDIO_WMA_LOSSLESS) && !encoder) {
                codec = MediaCodec::CreateByComponentName(looper, "OMX.qcom.audio.decoder.wmaLossLess");
            }
    }


    return codec;
}

/*
QCOM HW AAC encoder allowed bitrates
------------------------------------------------------------------------------------------------------------------
Bitrate limit |AAC-LC(Mono)           | AAC-LC(Stereo)        |AAC+(Mono)            | AAC+(Stereo)            | eAAC+                      |
Minimum     |Min(24000,0.5 * f_s)   |Min(24000,f_s)           | 24000                      |24000                        |  24000                       |
Maximum    |Min(192000,6 * f_s)    |Min(192000,12 * f_s)  | Min(192000,6 * f_s)  | Min(192000,12 * f_s)  |  Min(192000,12 * f_s) |
------------------------------------------------------------------------------------------------------------------
*/
bool ExtendedUtils::useQCHWEncoder(const sp<AMessage> &outputFormat, Vector<AString> *role)
{
    int minBitRate = -1;
    int maxBitRate = -1;
    bool currentState;
    int32_t bitRate = 0;
    int32_t sampleRate = 0;
    int32_t numChannels = 0;
    int32_t aacProfile = 0;
    audio_encoder aacEncoder = AUDIO_ENCODER_DEFAULT;
    AString mime;
    bool isQCHWEncoder = false;
    CHECK(outputFormat->findString("mime", &mime));

    bool preferC2Codecs = property_get_bool("vendor.audio.c2.preferred", false);

    if (preferC2Codecs) {
        AVLOGV("CCodec preferred, skip creation of custom OMX audio encoders");
        return false;
    }

    pConfigsIns = AVConfigHelper::getInstance();
    if (!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_AAC) &&
            pConfigsIns->useHwAACEncoder()) {
        //check for QCOM's HW AAC encoder only when qcom.aac.encoder =  true;
        AVLOGV("qcom.aac.encoder enabled, check AAC encoder allowed bitrates");

        outputFormat->findInt32("bitrate", &bitRate);
        outputFormat->findInt32("channel-count", &numChannels);
        outputFormat->findInt32("sample-rate", &sampleRate);
        outputFormat->findInt32("aac-profile", &aacProfile);

        AVLOGD("bitrate:%d, samplerate:%d, channels:%d",
                        bitRate, sampleRate, numChannels);

        switch(aacProfile) {
            case OMX_AUDIO_AACObjectLC:
                aacEncoder = AUDIO_ENCODER_AAC;
                AVLOGV("AUDIO_ENCODER_AAC");
                break;
            case OMX_AUDIO_AACObjectHE:
                aacEncoder = AUDIO_ENCODER_HE_AAC;
                AVLOGV("AUDIO_ENCODER_HE_AAC");
                break;
            case OMX_AUDIO_AACObjectELD:
                aacEncoder = AUDIO_ENCODER_AAC_ELD;
                AVLOGV("AUDIO_ENCODER_AAC_ELD");
                break;
            default:
                aacEncoder = AUDIO_ENCODER_DEFAULT;
                AVLOGE("Wrong encoder profile");
                break;
        }

        switch (aacEncoder) {
            case AUDIO_ENCODER_AAC:// for AAC-LC format
                if (numChannels == 1) {//mono
                    minBitRate = MIN_BITRATE_AAC < (sampleRate / 2) ? MIN_BITRATE_AAC: (sampleRate / 2);
                    maxBitRate = MAX_BITRATE_AAC < (sampleRate*6) ? MAX_BITRATE_AAC: (sampleRate * 6);
                } else if (numChannels == 2) {//stereo
                    minBitRate = MIN_BITRATE_AAC < sampleRate ? MIN_BITRATE_AAC: sampleRate;
                    maxBitRate = MAX_BITRATE_AAC < (sampleRate*12) ? MAX_BITRATE_AAC: (sampleRate * 12);
                }
                break;
            case AUDIO_ENCODER_HE_AAC:// for AAC+ format
                // Do not use HW AAC encoder for HE AAC(AAC+) formats.
                isQCHWEncoder = false;
                break;
            default:
                AVLOGV("encoder:%d not supported by QCOM HW encoder", aacEncoder);
        }

        //return true only when 1. minBitRate and maxBitRate are updated(not -1) 2. minBitRate <= SampleRate <= maxBitRate
        if (bitRate >= minBitRate && bitRate <= maxBitRate) {
           isQCHWEncoder = true;
        }

        if (isQCHWEncoder) {
            role->push("OMX.qcom.audio.encoder.aac");
            AVLOGD("Using encoder OMX.qcom.audio.encoder.aac");
        } else {
           role->push("");
        }
    } else  if (!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_QCELP)) {
        role->push("OMX.qcom.audio.encoder.qcelp13");
        isQCHWEncoder = true;
        AVLOGD("Using encoder OMX.qcom.audio.encoder.qcelp13");
    } else if(!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_EVRC)) {
        role->push("OMX.qcom.audio.encoder.evrc");
        isQCHWEncoder = true;
        AVLOGD("Using encoder OMX.qcom.audio.encoder.evrc");
    } else if(!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_MHAS)) {
        role->push("OMX.qcom.audio.encoder.mpegh");
        isQCHWEncoder = true;
        AVLOGD("Using encoder OMX.qcom.audio.encoder.mpegh");
    } else {
        role->push("");
    }
    return isQCHWEncoder;
}

bool ExtendedUtils::overrideMimeType(
        const sp<AMessage> &msg, AString* mime) {
    status_t ret = false;

    if (!mime) {
        return false;
    }

    if (!strncmp(mime->c_str(), MEDIA_MIMETYPE_AUDIO_WMA,
         strlen(MEDIA_MIMETYPE_AUDIO_WMA))) {
        int32_t WMAVersion = 0;
        if ((msg->findInt32(getMsgFromKey(kKeyWMAVersion), &WMAVersion))) {
            if (WMAVersion==kTypeWMA) {
                //no need to update mime type
            } else if (WMAVersion==kTypeWMAPro) {
                mime->setTo(MEDIA_MIMETYPE_AUDIO_WMA_PRO);
                ret = true;
            } else if (WMAVersion==kTypeWMALossLess) {
                mime->setTo(MEDIA_MIMETYPE_AUDIO_WMA_LOSSLESS);
                ret = true;
            } else {
                AVLOGE("could not set valid wma mime type");
            }
        }
    }
    return ret;
}

status_t ExtendedUtils::mapMimeToAudioFormat( audio_format_t& format, const char* mime )
{

    for (uint32_t i = 0; i < sizeof(mimeLookup)/sizeof(mimeLookup[0]); i++) {
        if (mimeLookup[i].mime != NULL && mime != NULL) {
            if (0 == strncasecmp(mime, mimeLookup[i].mime, strlen(mimeLookup[i].mime))) {
                format = (audio_format_t) mimeLookup[i].format;
                return OK;
            }
       }
    }

    return BAD_VALUE;
}

status_t ExtendedUtils::parseFlacMetaData(const sp<MetaData>& meta, AudioParameter *param) {
    int32_t minBlockSize = 16, maxBlockSize = 16, minFrameSize = 0, maxFrameSize = 0;
    const void* csdBuffer;
    size_t csdSize = 0;
    uint32_t type = 0;

    if (meta->findData(kKeyOpaqueCSD0, &type, &csdBuffer, &csdSize)) {
        // check if it is valid flac CSD
        if (csdSize < FLAC_SIGN_SIZE || memcmp(csdBuffer, FLAC_SIGN_BYTES, FLAC_SIGN_SIZE)) {
            ALOGE("%s invalid CSD", __func__);
            return BAD_VALUE;
        }
        int offset = FLAC_SIGN_SIZE;
        uint8_t* buffer = (uint8_t*) csdBuffer;

        while (offset <= csdSize - FLAC_SIGN_SIZE) {
            uint8_t blockType = buffer[offset] & 0x7F;
            uint8_t lastBlock = (buffer[offset] & 0x80) >> 7;
            uint32_t blockSize = ((buffer[offset + 1]) << 16) |
                                 ((buffer[offset + 2]) << 8) |
                                 ((buffer[offset + 3]));
            uint32_t minStreamInfoBytes = 10; //need to read 10 bytes for block, frame size fields.
            offset += FLAC_SIGN_SIZE;

            if (BLOCK_FLAC_STREAMINFO == blockType && (offset + minStreamInfoBytes) <= csdSize) {
                minBlockSize = (buffer[offset] << 8) | (buffer[offset + 1]);
                offset += 2;
                maxBlockSize = (buffer[offset] << 8) | (buffer[offset + 1]);
                offset += 2;
                minFrameSize = (buffer[offset] << 16) | (buffer[offset + 1] << 8) |
                               (buffer[offset + 2]);
                offset += 3;
                maxFrameSize = (buffer[offset] << 16) | (buffer[offset + 1] << 8) |
                               (buffer[offset + 2]);
                offset += 3;
                break;
            }

            if (lastBlock) break;

            offset += blockSize;
        }
    }
    ALOGV("%s minBlockSize %d, maxBlockSize %d, minFrameSize %d, maxFrameSize %d",
                __func__, minBlockSize, maxBlockSize, minFrameSize, maxFrameSize);
    param->addInt(String8(AUDIO_OFFLOAD_CODEC_FLAC_MIN_BLK_SIZE), minBlockSize);
    param->addInt(String8(AUDIO_OFFLOAD_CODEC_FLAC_MAX_BLK_SIZE), maxBlockSize);
    param->addInt(String8(AUDIO_OFFLOAD_CODEC_FLAC_MIN_FRAME_SIZE), minFrameSize);
    param->addInt(String8(AUDIO_OFFLOAD_CODEC_FLAC_MAX_FRAME_SIZE), maxFrameSize);
    return OK;
}

status_t ExtendedUtils::sendMetaDataToHal(const sp<MetaData>& meta, AudioParameter *param) {

    (void)param;
    const char *mime;
    bool success = meta->findCString(kKeyMIMEType, &mime);
    CHECK(success);

    pConfigsIns = AVConfigHelper::getInstance();

    if(!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_FLAC) && pConfigsIns->isFlacOffloadEnabled()) {
        return parseFlacMetaData(meta, param);
    }

    int32_t wmaFormatTag = 0, wmaBlockAlign = 0, wmaChannelMask = 0;
    int32_t wmaBitsPerSample = 0, wmaEncodeOpt = 0, wmaEncodeOpt1 = 0;
    int32_t wmaEncodeOpt2 = 0;

    if(!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_WMA) && pConfigsIns->isWmaOffloadEnabled()) {
        if (meta->findInt32(kKeyWMAFormatTag, &wmaFormatTag)) {
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_WMA_FORMAT_TAG), wmaFormatTag);
        }
        if (meta->findInt32(kKeyWMABlockAlign, &wmaBlockAlign)) {
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_WMA_BLOCK_ALIGN), wmaBlockAlign);
        }
        if (meta->findInt32(kKeyWMABitspersample, &wmaBitsPerSample)) {
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_WMA_BIT_PER_SAMPLE), wmaBitsPerSample);
        }
        if (meta->findInt32(kKeyWMAChannelMask, &wmaChannelMask)) {
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_WMA_CHANNEL_MASK), wmaChannelMask);
        }
        if (meta->findInt32(kKeyWMAEncodeOpt, &wmaEncodeOpt)) {
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_WMA_ENCODE_OPTION), wmaEncodeOpt);
        }
        if (meta->findInt32(kKeyWMAAdvEncOpt1, &wmaEncodeOpt1)) {
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_WMA_ENCODE_OPTION1), wmaEncodeOpt1);
        }
        if (meta->findInt32(kKeyWMAAdvEncOpt2, &wmaEncodeOpt2)) {
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_WMA_ENCODE_OPTION2), wmaEncodeOpt2);
        }
        AVLOGV("WMA specific meta: fmt_tag 0x%x, blk_align %d, bits_per_sample %d, "
                "enc_options 0x%x", wmaFormatTag, wmaBlockAlign,
                wmaBitsPerSample, wmaEncodeOpt);
        return OK;
    }

    int32_t bitstream_fmt = 1;
    if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_VORBIS) && pConfigsIns->isVorbisOffloadEnabled()) {
        param->addInt(String8(AUDIO_OFFLOAD_CODEC_VORBIS_BITSTREAM_FMT), bitstream_fmt);
        AVLOGV("Vorbis metadata: bitstream_fmt %d", bitstream_fmt);
        return OK;
    }

    const void *data;
    size_t size;
    uint32_t type = 0;
    if (meta->findData(kKeyRawCodecSpecificData, &type, &data, &size)) {
        CHECK(data && (size == ALAC_CSD_SIZE || size == APE_CSD_SIZE));
        AVLOGV("Found kKeyRawCodecSpecificData of size %zu", size);
        const uint8_t *ptr = (uint8_t *) data;

        if (!strncasecmp(mime, MEDIA_MIMETYPE_AUDIO_ALAC,
                    strlen(MEDIA_MIMETYPE_AUDIO_ALAC)) &&
                pConfigsIns->isAlacOffloadEnabled()) {
            uint32_t frameLength = 0, maxFrameBytes = 0, avgBitRate = 0;
            uint32_t samplingRate = 0, channelLayoutTag = 0;
            uint8_t compatibleVersion = 0, pb = 0, mb = 0, kb = 0, numChannels = 0, bitDepth = 0;
            uint16_t maxRun = 0;

            memcpy(&frameLength, ptr + kKeyIndexAlacFrameLength, sizeof(frameLength));
            memcpy(&compatibleVersion, ptr + kKeyIndexAlacCompatibleVersion, sizeof(compatibleVersion));
            memcpy(&bitDepth, ptr + kKeyIndexAlacBitDepth, sizeof(bitDepth));
            memcpy(&pb, ptr + kKeyIndexAlacPb, sizeof(pb));
            memcpy(&mb, ptr + kKeyIndexAlacMb, sizeof(mb));
            memcpy(&kb, ptr + kKeyIndexAlacKb, sizeof(kb));
            memcpy(&numChannels, ptr + kKeyIndexAlacNumChannels, sizeof(numChannels));
            memcpy(&maxRun, ptr + kKeyIndexAlacMaxRun, sizeof(maxRun));
            memcpy(&maxFrameBytes, ptr + kKeyIndexAlacMaxFrameBytes, sizeof(maxFrameBytes));
            memcpy(&avgBitRate, ptr + kKeyIndexAlacAvgBitRate, sizeof(avgBitRate));
            memcpy(&samplingRate, ptr + kKeyIndexAlacSamplingRate, sizeof(samplingRate));

            AVLOGV("ALAC CSD values: frameLength %d bitDepth %d numChannels %d"
                    " maxFrameBytes %d avgBitRate %d samplingRate %d",
                    frameLength, bitDepth, numChannels, maxFrameBytes, avgBitRate, samplingRate);

            param->addInt(String8(AUDIO_OFFLOAD_CODEC_ALAC_FRAME_LENGTH), frameLength);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_ALAC_COMPATIBLE_VERSION), compatibleVersion);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_ALAC_BIT_DEPTH), bitDepth);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_ALAC_PB), pb);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_ALAC_MB), mb);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_ALAC_KB), kb);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_ALAC_NUM_CHANNELS), numChannels);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_ALAC_MAX_RUN), maxRun);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_ALAC_MAX_FRAME_BYTES), maxFrameBytes);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_ALAC_AVG_BIT_RATE), avgBitRate);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_ALAC_SAMPLING_RATE), samplingRate);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_ALAC_CHANNEL_LAYOUT_TAG), channelLayoutTag);
            return OK;
        }

        if (!strncasecmp(mime, MEDIA_MIMETYPE_AUDIO_APE,
                    strlen(MEDIA_MIMETYPE_AUDIO_APE)) &&
                pConfigsIns->isApeOffloadEnabled()) {
            uint16_t compatibleVersion = 0, compressionLevel = 0;
            uint16_t bitsPerSample = 0, numChannels = 0;
            uint32_t formatFlags = 0, blocksPerFrame = 0, finalFrameBlocks = 0;
            uint32_t totalFrames = 0, sampleRate = 0, seekTablePresent = 0;

            memcpy(&compatibleVersion, ptr + kKeyIndexApeCompatibleVersion, sizeof(compatibleVersion));
            memcpy(&compressionLevel, ptr + kKeyIndexApeCompressionLevel, sizeof(compressionLevel));
            memcpy(&formatFlags, ptr + kKeyIndexApeFormatFlags, sizeof(formatFlags));
            memcpy(&blocksPerFrame, ptr + kKeyIndexApeBlocksPerFrame, sizeof(blocksPerFrame));
            memcpy(&finalFrameBlocks, ptr + kKeyIndexApeFinalFrameBlocks, sizeof(finalFrameBlocks));
            memcpy(&totalFrames, ptr + kKeyIndexApeTotalFrames, sizeof(totalFrames));
            memcpy(&bitsPerSample, ptr + kKeyIndexApeBitsPerSample, sizeof(bitsPerSample));
            memcpy(&numChannels, ptr + kKeyIndexApeNumChannels, sizeof(numChannels));
            memcpy(&sampleRate, ptr + kKeyIndexApeSampleRate, sizeof(sampleRate));
            memcpy(&seekTablePresent, ptr + kKeyIndexApeSeekTablePresent, sizeof(seekTablePresent));

            AVLOGV("APE CSD values: compatibleVersion %d compressionLevel %d formatFlags %d"
                    " blocksPerFrame %d finalFrameBlocks %d totalFrames %d bitsPerSample %d"
                    " numChannels %d sampleRate %d seekTablePresent %d",
                    compatibleVersion, compressionLevel, formatFlags, blocksPerFrame, finalFrameBlocks, totalFrames,
                    bitsPerSample, numChannels, sampleRate, seekTablePresent);

            param->addInt(String8(AUDIO_OFFLOAD_CODEC_APE_COMPATIBLE_VERSION), compatibleVersion);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_APE_COMPRESSION_LEVEL), compressionLevel);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_APE_FORMAT_FLAGS), formatFlags);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_APE_BLOCKS_PER_FRAME), blocksPerFrame);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_APE_FINAL_FRAME_BLOCKS), finalFrameBlocks);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_APE_TOTAL_FRAMES), totalFrames);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_APE_BITS_PER_SAMPLE), bitsPerSample);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_APE_NUM_CHANNELS), numChannels);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_APE_SAMPLE_RATE), sampleRate);
            param->addInt(String8(AUDIO_OFFLOAD_CODEC_APE_SEEK_TABLE_PRESENT), seekTablePresent);
            return OK;
        }
    }
    return OK;
}

bool ExtendedUtils::hasAudioSampleBits(const sp<MetaData> &meta) {
    bool status = false;
    const char* mime;
    if (meta != NULL) {
        if (meta->findCString(kKeyMIMEType, &mime)) {
            if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_WMA)) {
                status = meta->hasData(kKeyWMABitspersample);
            } else {
                status = meta->hasData(kKeySampleBits);
            }
        }
    }
    return status;
}

bool ExtendedUtils::hasAudioSampleBits(const sp<AMessage> &format) {
    bool status = false;
    AString mime;
    if (format != NULL) {
        if (format->findString("mime", &mime)) {
            if (!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_WMA)) {
                status = format->contains("wma-bits-per-sample");
            } else {
                status = format->contains("bits-per-sample");
            }
        }
    }
    return status;
}

int32_t ExtendedUtils::getAudioSampleBits(const sp<MetaData> &meta) {
    int32_t bitWidth = 16;
    const char* mime;
    if (meta != NULL) {
        if (meta->findCString(kKeyMIMEType, &mime)) {
            if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_WMA)) {
                meta->findInt32(kKeyWMABitspersample, &bitWidth);
            }
            else {
                meta->findInt32(kKeySampleBits, &bitWidth);
            }
        }
    }
    return bitWidth;
}

int32_t ExtendedUtils::getAudioSampleBits(const sp<AMessage> &format) {
    int32_t bitWidth = 16;
    AString mime;
    if (format != NULL) {
        if (format->findString("mime", &mime)) {
            if (!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_WMA)) {
                format->findInt32("wma-bits-per-sample", &bitWidth);
            } else {
                format->findInt32("bits-per-sample", &bitWidth);
            }
        }
    }
    return bitWidth;
}

bool ExtendedUtils::isAudioWMAPro(const sp<MetaData> &meta) {
    const char *mime = NULL;
    int32_t wmaVersion = kTypeWMA;
    if ((meta == NULL) || !(meta->findCString(kKeyMIMEType, &mime))) {
        return false;
    }

    if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_WMA)) {
       if (meta->findInt32(kKeyWMAVersion, &wmaVersion)) {
          if (wmaVersion == kTypeWMAPro || wmaVersion == kTypeWMALossLess) {
              AVLOGV("wma version is PRO");
              return true;
          }
       }
   }

   return false;
}

bool ExtendedUtils::isAudioWMAPro(const sp<AMessage> &format) {
    AString mime;
    int32_t wmaVersion = kTypeWMA;
    if ((format == NULL) || !(format->findString("mime", &mime))) {
        return false;
    }

    if (!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_WMA)) {
       if (format->findInt32("wma-version", &wmaVersion)) {
          if (wmaVersion == kTypeWMAPro || wmaVersion == kTypeWMALossLess) {
              AVLOGV("wma version is PRO");
              return true;
          }
       }
   }

   return false;
}

audio_format_t ExtendedUtils::updateAudioFormat(audio_format_t audioFormat,
                                                const sp<AMessage> &format) {
    if (audioFormat == AUDIO_FORMAT_WMA) {
        return isAudioWMAPro(format)? AUDIO_FORMAT_WMA_PRO : AUDIO_FORMAT_WMA;
    }
    return audioFormat;
}

audio_format_t ExtendedUtils::updateAudioFormat(audio_format_t audioFormat,
                                                const sp<MetaData> &meta) {
    if (audioFormat == AUDIO_FORMAT_WMA) {
        return isAudioWMAPro(meta)? AUDIO_FORMAT_WMA_PRO : AUDIO_FORMAT_WMA;
    }
    return audioFormat;
}

bool ExtendedUtils::canOffloadStream(const sp<MetaData> &meta)
{
    const char *mime;
    if(meta == NULL || !(meta->findCString(kKeyMIMEType, &mime))) {
        AVLOGD("canOffloadStream: params are wrong");
        return true; // return true as, cannot make decision
    }
    if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_APE)) {
        const void *data;
        size_t size;
        uint32_t type = 0;
        uint16_t compressionLevel = 0;
        if (meta->findData(kKeyRawCodecSpecificData, &type, &data, &size)) {
            if (data == NULL || size != APE_CSD_SIZE) {
                AVLOGD("Disallow offload for APE clips with improper size and data");
                return false;
            }
            memcpy(&compressionLevel, (uint8_t *) data + kKeyIndexApeCompressionLevel, sizeof(compressionLevel));
        }
        if (compressionLevel != APE_COMPRESSION_LEVEL_FAST &&
            compressionLevel != APE_COMPRESSION_LEVEL_NORMAL &&
            compressionLevel != APE_COMPRESSION_LEVEL_HIGH) {
            AVLOGD("Disallow offload for APE clip with compressionLevel %d", compressionLevel);
            return false;
        }
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_VORBIS)) {
        int32_t brate = -1;
        int32_t maxbrate = -1;
        meta->findInt32(kKeyMaxBitRate, &maxbrate);
        meta->findInt32(kKeyBitRate, &brate);
        if ((maxbrate > MAX_BITRATE_VORBIS) || (brate > MAX_BITRATE_VORBIS)) {
            AVLOGD("Disallow vorbis offload due to unsupported avgBitrate(%d), maxBitrate(%d)",
                    brate, maxbrate);
            return false;
        }
    }
    return true;
}

int32_t ExtendedUtils::getAudioMaxInputBufferSize(audio_format_t audioFormat, const sp<AMessage> &format) {
    int32_t audioBufSize = 0;
    int32_t byteStreamMode = 0;
    if (format != NULL) {
        if (format->findInt32("is-byte-stream-mode", &byteStreamMode)) {
            if (byteStreamMode) {
                format->findInt32("max-input-size", &audioBufSize);
                AVLOGD("max-inp-size %d audio format %x", audioBufSize, audioFormat);
            }
        }
    }
    return audioBufSize;
}

struct aac_adts_format_conv_t {
        OMX_AUDIO_AACPROFILETYPE eAacProfileType;
        audio_format_t format;
};

static const struct aac_adts_format_conv_t profileLookup[] = {
    { OMX_AUDIO_AACObjectMain,        AUDIO_FORMAT_AAC_MAIN},
    { OMX_AUDIO_AACObjectLC,          AUDIO_FORMAT_AAC_ADTS_LC},
    { OMX_AUDIO_AACObjectSSR,         AUDIO_FORMAT_AAC_ADTS_SSR},
    { OMX_AUDIO_AACObjectLTP,         AUDIO_FORMAT_AAC_ADTS_LTP},
    { OMX_AUDIO_AACObjectHE,          AUDIO_FORMAT_AAC_ADTS_HE_V1},
    { OMX_AUDIO_AACObjectScalable,    AUDIO_FORMAT_AAC_ADTS_SCALABLE},
    { OMX_AUDIO_AACObjectERLC,        AUDIO_FORMAT_AAC_ADTS_ERLC},
    { OMX_AUDIO_AACObjectLD,          AUDIO_FORMAT_AAC_ADTS_LD},
    { OMX_AUDIO_AACObjectHE_PS,       AUDIO_FORMAT_AAC_ADTS_HE_V2},
    { OMX_AUDIO_AACObjectELD,         AUDIO_FORMAT_AAC_ADTS_ELD},
    { OMX_AUDIO_AACObjectNull,        AUDIO_FORMAT_AAC_ADTS},
};

bool ExtendedUtils::mapAACProfileToAudioFormat(const sp<MetaData> &meta,
                          audio_format_t& audioFormat,
                          uint64_t eAacProfile) {
    int32_t isADTS;
    pConfigsIns = AVConfigHelper::getInstance();
    if (!pConfigsIns->isAACADTSOffloadEnabled())
        return false;

    if (meta->findInt32(kKeyIsADTS, &isADTS)) {
        const struct aac_adts_format_conv_t* p = &profileLookup[0];
        while (p->eAacProfileType != OMX_AUDIO_AACObjectNull) {
            if (eAacProfile == p->eAacProfileType) {
                audioFormat = p->format;
                return true;
            }
            ++p;
        }
        audioFormat = AUDIO_FORMAT_AAC_ADTS;
        return true;
    }
    return false;
}

bool ExtendedUtils::mapAACProfileToAudioFormat(const sp<AMessage> &format,
                          audio_format_t& audioFormat,
                          uint64_t eAacProfile) {
    int32_t isADTS;
    pConfigsIns = AVConfigHelper::getInstance();
    if (!pConfigsIns->isAACADTSOffloadEnabled())
        return false;

    if (format->findInt32("is-adts", &isADTS)) {
        const struct aac_adts_format_conv_t* p = &profileLookup[0];
        while (p->eAacProfileType != OMX_AUDIO_AACObjectNull) {
            if (eAacProfile == p->eAacProfileType) {
                audioFormat = p->format;
                return true;
            }
            ++p;
        }
        audioFormat = AUDIO_FORMAT_AAC_ADTS;
        return true;
    }
    return false;
}

bool ExtendedUtils::isEnhancedExtension(const char *extension) {
    const char *kEnhancedExtensions[] = {
        ".qcp", ".ac3", ".dts", ".wmv", ".ec3", ".mov", ".flv",
        ".ape", ".oga", ".aif", ".aiff", ".wma", ".dsf", ".dff",
        ".dsd", ".mhas", ".divx"
    };
    size_t kNumEnhancedExtensions =
        sizeof(kEnhancedExtensions) / sizeof(kEnhancedExtensions[0]);

    for (size_t i = 0; i < kNumEnhancedExtensions; ++i) {
        if (!strcasecmp(extension, kEnhancedExtensions[i])) {
            return true;
        }
    }

    return false;
}

bool ExtendedUtils::isAudioSourceAggregate(
    const audio_attributes_t *attr,
    uint32_t channelCount) {
    String8 keys = String8(
        "hdr_record_on;"
        "hdr_audio_channel_count;hdr_audio_sampling_rate;"
    );
    String8 params = AudioSystem::getParameters(keys);
    AVLOGV("AudioSystem::getParameters - %s", params.c_str());
    AudioParameter kvpair(params);

    String8 hdrKey = String8("hdr_record_on");
    String8 hdrAudioChannelCountKey =
        String8("hdr_audio_channel_count");
    String8 hdrAudioSamplingRateKey =
        String8("hdr_audio_sampling_rate");

    String8 hdrVal;
    String8 hdrChannelCountVal, hdrSamplingRateVal;

    bool hdr_set = false;
    int hdr_audio_channels = 0, hdr_audio_sample_rate = 0;

    if (kvpair.get(hdrKey, hdrVal) == NO_ERROR) {
        AVLOGV("HDR val: %s", hdrVal.c_str());
        if (!strcmp(hdrVal.c_str(), "true"))
            hdr_set = true;
    } else {
        AVLOGE("HDR key not found");
    }
    if (kvpair.get(hdrAudioChannelCountKey, hdrChannelCountVal)
        == NO_ERROR) {
        AVLOGV("HDR channel count val: %s",
            hdrChannelCountVal.c_str());
        if (!strcmp(hdrChannelCountVal.c_str(), "4"))
            hdr_audio_channels = 4;
    } else {
        AVLOGE("HDR Channel count not found");
    }
    if (kvpair.get(hdrAudioSamplingRateKey, hdrSamplingRateVal)
        == NO_ERROR) {
        AVLOGV("HDR sampling rate val: %s",
            hdrSamplingRateVal.c_str());
        if (!strcmp(hdrSamplingRateVal.c_str(), "48000"))
            hdr_audio_sample_rate = 48000;
    } else {
        AVLOGE("HDR Sampling rate not found");
    }

    bool isAggregateSource =
        ((channelCount == 1) || (channelCount == 2)) &&
        (attr != NULL) &&
        (attr->source == AUDIO_SOURCE_CAMCORDER);
    bool isHdrSource =
        (hdr_audio_channels == 4) &&
        (hdr_audio_sample_rate == 48000) &&
        (attr != NULL) && (attr->source == AUDIO_SOURCE_UNPROCESSED) &&
        property_get_bool("vendor.audio.hdr.record.enable", false) &&
        hdr_set;

    return isAggregateSource || isHdrSource;
}

void ExtendedUtils::extractCustomCameraKeys(
    const CameraParameters& params, sp<MetaData> &meta) {
    AVLOGV("extractCustomCameraKeys");

    // Camera pre-rotates video buffers. Width and Height of
    // of the image will be flipped if rotation is 90 or 270.
    // Encoder must be made aware of the flip in this case.
    const char *pRotation = params.get("video-rotation");
    int32_t preRotation = pRotation ? atoi(pRotation) : 0;
    bool flip = preRotation % 180;

    if (flip) {
        int32_t width = 0;
        int32_t height = 0;
        meta->findInt32(kKeyWidth, &width);
        meta->findInt32(kKeyHeight, &height);

        // width assigned to height is intentional
        meta->setInt32(kKeyWidth, height);
        meta->setInt32(kKeyStride, height);
        meta->setInt32(kKeyHeight, width);
        meta->setInt32(kKeySliceHeight, width);
    }

    int32_t batchSize = params.getInt("video-batch-size");
    if (batchSize > 0) {
        meta->setInt32(kKeyLocalBatchSize, batchSize);
    }
}

void ExtendedUtils::printFileName(int fd) {
    if (fd) {
        char symName[40] = {0};
        char fileName[256] = {0};
        snprintf(symName, sizeof(symName), "/proc/%d/fd/%d", getpid(), fd);

        if (readlink(symName, fileName, (sizeof(fileName) - 1)) != -1 ) {
            AVLOGI("printFileName fd(%d) -> %s", fd, fileName);
        }
    }
}

void ExtendedUtils::addDecodingTimesFromBatch(MediaBufferBase *buf,
         List<int64_t>& decodeTimeQueue) {
    AVLOGV("addDecodingTimesFromBatch");

    if (buf == NULL) {
        return;
    }

    VideoNativeHandleMetadata *meta = static_cast<VideoNativeHandleMetadata *>(buf->data());
    if (!meta || meta->eType != kMetadataBufferTypeNativeHandleSource) {
        AVLOGV("Invalid meta-buffer");
        return;
    }
    native_handle_t *hnd = (native_handle_t *)meta->pHandle;
    int batchSize = MetaBufferUtil::getBatchSize(hnd);
    if (batchSize < 2) {
        return;
    }

    int64_t baseTimeUs = 0ll;
    buf->meta_data().findInt64(kKeyTime, &baseTimeUs);

    // The first buffer's timestamp is inserted in MediaCodecSource, insert for the rest
    for (int i = 1 /*skip first*/; i < batchSize; ++i) {

        // timestamp differences from Camera are in nano-seconds
        int64_t incrementalTimeUs = MetaBufferUtil::getIntAt(hnd, i, MetaBufferUtil::INT_TIMESTAMP) / 1E3;
        int64_t timeUs = baseTimeUs + incrementalTimeUs;

        AVLOGV("base timestamp %" PRId64 ", computed timestamp %" PRId64,
                baseTimeUs, timeUs);
        decodeTimeQueue.push_back(timeUs);
    }
}

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

bool ExtendedUtils::isAudioMuxFormatSupported(const char *mime) {
    if (mime == NULL) {
        AVLOGE("NULL audio mime type");
        return false;
    }

    if (!strcasecmp(MEDIA_MIMETYPE_AUDIO_AMR_NB, mime) ||
        !strcasecmp(MEDIA_MIMETYPE_AUDIO_AMR_WB, mime) ||
        !strcasecmp(MEDIA_MIMETYPE_AUDIO_MHAS, mime) ||
        !strcasecmp(MEDIA_MIMETYPE_AUDIO_AAC, mime)) {
        return true;
    }
    return false;
}

static void getHEVCNalUnitType(uint8_t byte, uint8_t* type) {
    AVLOGV("getNalUnitType: %d", (int)byte);
    // nal_unit_type: 6-bit unsigned integer
    *type = (byte & 0x7E) >> 1;
}

void ExtendedUtils::cacheCaptureBuffers(sp<hardware::ICamera> camera, video_encoder encoder) {
    if (camera != NULL) {
        const char *value = "disable";
            if (encoder == VIDEO_ENCODER_H263 ||
                encoder == VIDEO_ENCODER_MPEG_4_SP) {
                value = "enable";
            }

        int64_t token = IPCThreadState::self()->clearCallingIdentity();
        String8 s = camera->getParameters();
        CameraParameters params(s);
        AVLOGV("%s: caching %s", __func__, value);
        params.set("cache-video-buffers", value);
        if (camera->setParameters(params.flatten()) != OK) {
            AVLOGE("Failed to set parameter - cache-video-buffers");
        }
        IPCThreadState::self()->restoreCallingIdentity(token);
    }
}

void ExtendedUtils::getHFRParams(bool* isHFR, int32_t* batch_size, sp<AMessage> format) {
    int32_t batchSize;
    format->findInt32("batch-size", &batchSize);
    int32_t highFrameRate = 0;
    *batch_size = 0;
    if (format->findInt32("high-frame-rate", &highFrameRate) && highFrameRate == 1) {
        *isHFR = true;
        if (batchSize > 0) {
             format->findInt32("batch-size", batch_size);
        }
    }
    return;
}

int64_t ExtendedUtils::overwriteTimeOffset(bool isHFR,int64_t inputBufferTimeOffsetUs,
    int64_t *prevBufferTimestampUs,int64_t timeUs, int32_t batchSize) {
    int64_t timeOffset = 0;
    if (isHFR) {
        if (batchSize == 0) {
            timeOffset = -(timeUs - *prevBufferTimestampUs - kBufferTimestampOffset);
        } else {
            timeOffset = -(timeUs - *prevBufferTimestampUs - kBufferTimestampOffset * batchSize);
        }
        *prevBufferTimestampUs = timeUs + timeOffset;
    } else {
        timeOffset = inputBufferTimeOffsetUs;
        *prevBufferTimestampUs = timeUs;
    }
    return timeOffset;
}

bool ExtendedUtils::IsHevcIDR(const sp<ABuffer>& buffer) {
    const uint8_t *data = buffer->data();
    size_t size = buffer->size();

    bool foundRef = false;
    const uint8_t *nalStart;
    size_t nalSize;
    while (!foundRef && getNextNALUnit(&data, &size, &nalStart, &nalSize, true) == OK) {
        if (nalSize == 0) {
            AVLOGW("Encountered zero-length HEVC NAL");
            return false;
        }

        uint8_t nalType;
        getHEVCNalUnitType(nalStart[0], &nalType);

        switch(nalType) {
        case kHEVCNalUnitTypeIDR:
        case kHEVCNalUnitTypeIDRNoLP:
        case kHEVCNalUnitTypeCRA:
            foundRef = true;
            break;
        }
    }

    return foundRef;
}

const char *ExtendedUtils::getComponentRole(
        bool isEncoder, const char *mime) {
    AVLOGD("getComponentRole");

    const char *role = AVUtils::getComponentRole(isEncoder, mime);
    if ( role != NULL) {
         return role;
    }
    AVLOGD("getComponentRole for %s", mime);
    struct MimeToRole {
        const char *mime;
        const char *decoderRole;
        const char *encoderRole;
    };

    static const MimeToRole kMimeToRole[] = {
        { MEDIA_MIMETYPE_AUDIO_EVRC,
          "audio_decoder.evrchw", "audio_encoder.evrc" },
        { MEDIA_MIMETYPE_AUDIO_MHAS,
          "audio_decoder.mpegh", "audio_encoder.mpegh" },
        { MEDIA_MIMETYPE_AUDIO_QCELP,
          "audio_decoder.qcelp13Hw", "audio_encoder.qcelp13" },
        { MEDIA_MIMETYPE_VIDEO_DIVX,
          "video_decoder.divx", NULL },
        { MEDIA_MIMETYPE_VIDEO_DIVX4,
          "video_decoder.divx4", NULL },
        { MEDIA_MIMETYPE_VIDEO_DIVX311,
          "video_decoder.divx311", NULL },
        { MEDIA_MIMETYPE_VIDEO_WMV,
          "video_decoder.vc1",  NULL },
        { MEDIA_MIMETYPE_VIDEO_WMV_VC1,
          "video_decoder.vc1",  NULL },
        { MEDIA_MIMETYPE_AUDIO_AC3,
          "audio_decoder.ac3", NULL },
        { MEDIA_MIMETYPE_AUDIO_WMA,
          "audio_decoder.wma", NULL },
        { MEDIA_MIMETYPE_AUDIO_ALAC,
          "audio_decoder.alac", NULL },
        { MEDIA_MIMETYPE_AUDIO_APE,
          "audio_decoder.ape", NULL },
        { MEDIA_MIMETYPE_VIDEO_HEVC,
          "video_decoder.hevc", "video_encoder.hevc" },
        { MEDIA_MIMETYPE_AUDIO_AMR_WB_PLUS,
            "audio_decoder.amrwbplus", "audio_encoder.amrwbplus" },
        { MEDIA_MIMETYPE_AUDIO_QC_AMR_WB_PLUS,
            "audio_decoder.amrwbplus", "audio_encoder.amrwbplus" },
        { MEDIA_MIMETYPE_AUDIO_EVRC,
            "audio_decoder.evrchw", "audio_encoder.evrc" },
        { MEDIA_MIMETYPE_AUDIO_QCELP,
            "audio_decoder.qcelp13Hw", "audio_encoder.qcelp13" },
        { MEDIA_MIMETYPE_VIDEO_MPEG4_DP,
            "video_decoder.mpeg4", NULL },
        { MEDIA_MIMETYPE_VIDEO_TME,
          NULL, "video_encoder.tme" },
        { MEDIA_MIMETYPE_AUDIO_FLAC,
            "audio_decoder.flac", "audio_encoder.flac" },
        { MEDIA_MIMETYPE_AUDIO_DSD,
            "audio_decoder.dsd", NULL },
    };

    static const size_t kNumMimeToRole =
        sizeof(kMimeToRole) / sizeof(kMimeToRole[0]);

    size_t i;
    for (i = 0; i < kNumMimeToRole; ++i) {
        if (!strcasecmp(mime, kMimeToRole[i].mime)) {
            break;
        }
    }

    if (i == kNumMimeToRole) {
        AVLOGE("Unsupported mime %s for %s", mime, isEncoder ?
                "encoder" : "decoder");
        return NULL;
    }

    pConfigsIns = AVConfigHelper::getInstance();
    if (pConfigsIns->useQTIFlacDecoder() &&
            !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_FLAC)) {
        return isEncoder ? NULL : kMimeToRole[i].decoderRole;
    }

    return isEncoder ? kMimeToRole[i].encoderRole
                  : kMimeToRole[i].decoderRole;
}

sp<AMessage> ExtendedUtils::fillExtradata(sp<MediaCodecBuffer> &buffer, sp<AMessage> &format) {
    if (buffer == NULL) {
        return format;
    }

    bool hasExtradata = false;
    sp<AMessage> updatedFormat = format->dup();

    OMX_OTHER_EXTRADATATYPE *p_extra = NULL;
    p_extra = (OMX_OTHER_EXTRADATATYPE *)buffer->base();

#define OMX_QTI_ExtraData_LTRInfo                 0x7F100004
#define OMX_QTI_ExtraData_Index                   0x7F100002
#define OMX_QTI_ExtraData_OutputCropInfo          0x0700000F
#define OMX_QTI_ExtraData_NumConcealedMB          0x7F100001
    while (((uint8_t *)p_extra < buffer->base() + buffer->size()) && p_extra->eType != OMX_ExtraDataNone) {
        switch ((int)p_extra->eType) {
            case OMX_QTI_ExtraData_LTRInfo:
            case OMX_ExtraDataVideoLTRInfo:
            {
                hasExtradata = true;
                sp<ABuffer> ltrInfo = copyExtradata(p_extra->data, p_extra->nDataSize);
                const char *temp = getStringForExtradataType(p_extra->eType);
                if (ltrInfo != NULL && temp != NULL) {
                    updatedFormat->setBuffer(temp, ltrInfo);
                }
                break;
            }
            case OMX_QTI_ExtraData_Index:
            case OMX_ExtraDataOutputCropInfo:
            {
                if (p_extra->eType == OMX_QTI_ExtraData_Index) {
                    uint32_t *etype = (uint32_t *)p_extra->data;
                    if (etype && *etype == OMX_QTI_ExtraData_OutputCropInfo) {
                        hasExtradata = true;
                        sp<ABuffer> outputCropInfo = copyExtradata(++etype, p_extra->nDataSize - 4);
                        const char *temp = getStringForExtradataType(OMX_QTI_ExtraData_OutputCropInfo);
                        if (outputCropInfo != NULL &&  temp != NULL) {
                            updatedFormat->setBuffer(temp, outputCropInfo);
                        }
                    }
                } else if ((int)p_extra->eType == OMX_ExtraDataOutputCropInfo) {
                    hasExtradata = true;
                    sp<ABuffer> outputCropInfo = copyExtradata(p_extra->data, p_extra->nDataSize);
                    const char *temp = getStringForExtradataType(OMX_ExtraDataOutputCropInfo);
                    if (outputCropInfo != NULL &&  temp != NULL) {
                        updatedFormat->setBuffer(temp, outputCropInfo);
                    }
                }
                break;
            }
            case OMX_QTI_ExtraData_NumConcealedMB:
            {
                hasExtradata = true;
                sp<ABuffer> concealMB = copyExtradata(p_extra->data, p_extra->nDataSize);
                const char *temp = getStringForExtradataType(OMX_QTI_ExtraData_NumConcealedMB);
                if (concealMB != NULL && temp != NULL) {
                    updatedFormat->setBuffer(temp, concealMB);
                }
                break;
            }
            default:
                /* No idea what this stuff is, just skip over it */
                ALOGV("Found an unrecognised or unused extradata (%x) ignoring it",
                        p_extra->eType);
                break;
        }

        p_extra = (OMX_OTHER_EXTRADATATYPE *)(((char *)p_extra) + p_extra->nSize);
    }
#undef OMX_QTI_ExtraData_LTRInfo
#undef OMX_QTI_ExtraData_Index
#undef OMX_QTI_ExtraData_OutputCropInfo
#undef OMX_QTI_ExtraData_NumConcealedMB

    return hasExtradata ? updatedFormat : format;
}

} //namespace android
