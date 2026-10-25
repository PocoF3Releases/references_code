/*
 * Copyright (c) 2015-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
/*
 * Copyright (c) 2013 - 2015, The Linux Foundation. All rights reserved.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "ExtendedNuUtils"
#include <common/AVLog.h>
#include <cutils/properties.h>
#include <system/audio.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <QCMetaData.h>
#include <common/AVMetaData.h>
#include <audio_utils/format.h>
#include <nuplayer/include/nuplayer/NuPlayer.h>
#include <nuplayer/include/nuplayer/NuPlayerDecoderBase.h>
#include <stagefright/AVExtensions.h>
#include <mediaplayerservice/AVNuExtensions.h>
#include "mediaplayerservice/ExtendedNuUtils.h"

namespace android {

AVNuUtils *createExtendedNuUtils() {
    return new ExtendedNuUtils;
}

ExtendedNuUtils::ExtendedNuUtils() {
    AVLOGV("ExtendedNuUtils()");
    mBinauralMode = 0;
    mChannelMask = AUDIO_CHANNEL_OUT_STEREO;
}

ExtendedNuUtils::~ExtendedNuUtils() {
    AVLOGV("~ExtendedNuUtils()");
}

bool ExtendedNuUtils::pcmOffloadException(const sp<AMessage> &format) {
    AString mime;
    if (format == NULL) {
        return true;
    }
    CHECK(format->findString("mime", &mime));

    bool decision = false;
    if (mime.empty()) {
        AVLOGV("%s: no audio mime present, ignoring pcm offload", __func__);
        return true;
    }

    pConfigsIns = AVConfigHelper::getInstance();
    if (pConfigsIns->isPCMOffload16BitEnabled() ||
            pConfigsIns->isPCMOffload24BitEnabled()) {
        const char * const ExceptionTable[] = {
            MEDIA_MIMETYPE_AUDIO_AMR_NB,
            MEDIA_MIMETYPE_AUDIO_AMR_WB,
            MEDIA_MIMETYPE_AUDIO_QCELP,
            MEDIA_MIMETYPE_AUDIO_G711_ALAW,
            MEDIA_MIMETYPE_AUDIO_G711_MLAW,
            MEDIA_MIMETYPE_AUDIO_EVRC
        };
        int countException = (sizeof(ExceptionTable) / sizeof(ExceptionTable[0]));
        for(int i = 0; i < countException; i++) {
            if (!strcasecmp(mime.c_str(), ExceptionTable[i])) {
                decision = true;
                break;
            }
        }
        AVLOGV("decision %d mime %s", decision, mime.c_str());
        return decision;
    } else {
        //if PCM offload flag is disabled, do not offload any sessions
        //using pcm offload
        decision = true;
        AVLOGV("decision %d mime %s", decision, mime.c_str());
        return decision;
    }
}

bool ExtendedNuUtils::isVorbisFormat(const sp<MetaData> &meta) {
    const char *mime = {0};

    if ((meta == NULL) || !(meta->findCString(kKeyMIMEType, &mime))) {
        return false;
    }

    return (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_VORBIS)) ? true : false;
}

inline bool ExtendedNuUtils::flacSwSupports24Bit() {
    pConfigsIns = AVConfigHelper::getInstance();
    return pConfigsIns->flacSwSupports24Bit();
}

bool ExtendedNuUtils::avOffloadEnabled() {
    return property_get_bool("audio.offload.video", false /*default value*/);
}

audio_format_t ExtendedNuUtils::getPCMFormat(const sp<AMessage> &format) {
    audio_format_t pcmFormat = AUDIO_FORMAT_PCM_16_BIT;
    if (format != NULL) {
        AudioEncoding e = kAudioEncodingPcm16bit;
        format->findInt32("pcm-encoding", (int32_t *)&e);
        pcmFormat = getAudioFormat(e);
    }
    return pcmFormat;
}

void ExtendedNuUtils::setMPEGHOutputFormat(int32_t binaural, int32_t channelMask) {
    mBinauralMode = binaural;
    mChannelMask = channelMask;
}

void ExtendedNuUtils::setCodecOutputFormat(const sp<AMessage> &format) {
    int32_t pcmEncoding;
    AString mime;
    bool success = format->findString("mime", &mime);
    CHECK(success);

    if (!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_MHAS)) {
        format->setInt32("channel-mask", mChannelMask);
        format->setInt32("channel-count", audio_channel_count_from_out_mask(mChannelMask));
        int32_t rate = 0;
        if (!format->findInt32("sample-rate", &rate) || rate == 0) {
            AVLOGE("Sample rate for MHAS bitstream not provided. Defaulting to 48KHz");
            format->setInt32("sample-rate", 48000);
        }
        format->setInt32("bits-per-sample", 32);
        format->setInt32("binaural-mode", mBinauralMode);
    }

    if (!format->findInt32("pcm-encoding", &pcmEncoding)) {
        //For FLAC, check if the decoder supports 24 bit output
        //and set encoding to 16 bit only if it does not
        if ((!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_FLAC) && !flacSwSupports24Bit()) ||
                (!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_WMA)) ||
                (!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_WMA_PRO)) ||
                (!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_WMA_LOSSLESS))) {
            pcmEncoding = kAudioEncodingPcm16bit;
            AVLOGD("Overriding default codec output format to kAudioEncodingPcm16bit");
        } else if(!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_DSD)) {
            pcmEncoding = kAudioEncodingPcm24bitPacked;
            AVLOGD("Overriding default codec output format to kAudioEncodingPcm24bitPacked");
        } else {
            pcmEncoding = getPcmEncoding(AVUtils::get()->getAudioSampleBits(format));
        }
        format->setInt32("pcm-encoding", pcmEncoding);
    }
    AVLOGD("codec output format is %d for mime %s",
            pcmEncoding, mime.c_str());
}

bool ExtendedNuUtils::isByteStreamModeEnabled(const sp<MetaData> &meta) {
    int32_t isByteMode = 0;
    if (meta != NULL) {
        meta->findInt32(kKeyIsByteMode, &isByteMode);
    }
    return isByteMode ? true : false;
}

void ExtendedNuUtils::printFileName(int fd) {
    if (fd) {
        char symName[40] = {0};
        char fileName[256] = {0};
        snprintf(symName, sizeof(symName), "/proc/%d/fd/%d", getpid(), fd);

        if (readlink(symName, fileName, (sizeof(fileName) - 1)) != -1 ) {
            AVLOGI("printFileName fd(%d) -> %s", fd, fileName);
        }
    }
}

uint32_t ExtendedNuUtils::getFlags() {
    uint32_t  flags = 0;
    flags |= ENABLE_AUDIO_BIG_BUFFERS;
    return flags;
}

bool ExtendedNuUtils::canUseSetBuffers(const sp<MetaData> &meta) {
    if (meta == NULL) {
        return false;
    }

    int32_t useSetBuffers = 0;
    if (meta->findInt32(kKeyEnableSetBuffers, &useSetBuffers) && useSetBuffers) {
        AVLOGI("Parser supports set buffers");
        return true;
    }
    return false;
}

bool ExtendedNuUtils::dropCorruptFrame() {
    char dropCorruptFrame[PROPERTY_VALUE_MAX] = { 0 };
    property_get("persist.vendor.debug.en.drpcrpt", dropCorruptFrame, "1");
    if (!strncmp(dropCorruptFrame, "true", 4) || atoi(dropCorruptFrame)) {
        mDropCorruptFrame = true;
    }
    else {
        mDropCorruptFrame = false;
    }
    AVLOGV("DropCorruptFrame : %s", mDropCorruptFrame ? "Enabled" : "Disabled");
    return mDropCorruptFrame;
}

bool ExtendedNuUtils::findAndReplaceKeys(
        const char *key, sp <AMessage> &dst, const sp <AMessage> &src) {

     int dst_val = 0;
     int src_val = 0;
     int64_t src64_val = 0;
     int64_t dst64_val = 0;
     bool dst_status = false;
     bool src_status = false;
     bool src64_status = false;

     dst_status = dst->findInt32(key, &dst_val);
     src_status = src->findInt32(key, &src_val);
     //Needed to make sure channel mask is updated from channel count.
     if (!strncmp(key, "channel-mask", 12) && (!dst_val && !src_val)) {
         dst_status = false;
         src_status = false;
     }
     //if bit-width, check for bits-per-sample too in src
     if (!src_status && !strncmp(key, "bit-width", 9)) {
         src_status = src->findInt32("bits-per-sample", &src_val);
     }

     //if the key is duration, check for 64 bit duration too
     if (!src_status && !strncmp(key, "durationUs", 10)) {
         src64_status = src->findInt64(key, &src64_val);
         dst_status = dst->findInt64(key, &dst64_val);
     }

     AVLOGV("key :%s before dst: %d, 32 bit src: %d", key, dst_val, src_val);
     //value not present in destination, override using src if available
     if (!dst_status) {
        if (src_status) {
            AVLOGI("Overriding %s: with src value %d", key, src_val);
            dst->setInt32(key, src_val);
        } else if (src64_status) {
            //for 64 bit fields
            AVLOGI("Overriding %s: with 64 bit src value %" PRId64 "",
                key, src64_val);
            dst->setInt64(key, src64_val);
        } else {
            AVLOGI("Both src and dst missing for %s", key);
        }
     }

     return (dst_status | src_status);
}

void ExtendedNuUtils::overWriteAudioOutputFormat(
        sp <AMessage> &dst, const sp <AMessage> &src) {

    //src is audio format from file header
    //dst is audio format from decoders output

    AVLOGV("dump of data from file header meta data %s", src->debugString().c_str());
    AVLOGV("dump of data from decoder meta data %s", dst->debugString().c_str());
    bool channel_status = false;
    bool mask_status = false;

    //it is OK to ignore the status for these two fields
    findAndReplaceKeys("sample-rate", dst, src);
    findAndReplaceKeys("bit-width", dst, src);
    findAndReplaceKeys("durationUs", dst, src);

    mask_status = findAndReplaceKeys("channel-mask", dst, src);
    channel_status = findAndReplaceKeys("channel-count", dst, src);
    if (!mask_status && channel_status) {
       //channel mask was not set, channel count is present
       //derive mask from channel count
       int dchannel = 0;
       dst->findInt32("channel-count", &dchannel);
       int mask = audio_channel_out_mask_from_count(dchannel);
       dst->setInt32("channel-mask", mask);
       AVLOGV("channel mask derived from count:%d", mask);
    }

    if (!channel_status && mask_status) {
        //no channel count set,
        //channel mask is set, use that to generate mask
        int dmask = 0;
        dst->findInt32("channel-mask", &dmask);
        int channels = audio_channel_count_from_out_mask(dmask);
        AVLOGV("channel count derived from mask :%d", channels);
    }

    if (!channel_status && !mask_status) {
        //no channel count or mask, print warning
        AVLOGE("no channel mask or channel count in src or dst");
    }

    AVLOGV("dump of meta data structure after merging %s",
        dst->debugString().c_str());
}

} //namespace android

