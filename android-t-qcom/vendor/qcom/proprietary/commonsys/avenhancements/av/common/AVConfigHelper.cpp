/*
 * Copyright (c) 2019, 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "AVConfigHelper"

#include <common/AVConfigHelper.h>
#include <common/AVLog.h>
#include <cutils/properties.h>
#include <stdlib.h>

namespace android {

AVConfigHelper *AVConfigHelper::getInstance()
{
    static AVConfigHelper instance;
    return &instance;
}

AVConfigHelper::AVConfigHelper()
{
    char audio_debug_sf_noaudio[PROPERTY_VALUE_MAX] = "\0";
    uint32_t audio_record_aggregate_buffer_duration = 0;

    audio_record_aggregate_buffer_duration =
        (uint32_t)property_get_int32("vendor.audio.record.aggregate.buffer.duration", 0);
    property_get("persist.vendor.audio.debug.sf.noaudio", audio_debug_sf_noaudio, NULL);

    mConfigs = {
        (bool) property_get_bool("vendor.audio.use.sw.alac.decoder", true),
        (bool) property_get_bool("vendor.audio.use.sw.ape.decoder", true),
        (bool) property_get_bool("vendor.audio.use.sw.mpegh.decoder", false),
        (bool) property_get_bool("vendor.audio.flac.sw.decoder.24bit", true),
        (bool) property_get_bool("vendor.audio.hw.aac.encoder", true),
        audio_debug_sf_noaudio,
        audio_record_aggregate_buffer_duration,
#ifdef AAC_ADTS_OFFLOAD_ENABLED
        true,
#else
        false,
#endif
#ifdef ALAC_OFFLOAD_ENABLED
        true,
#else
        false,
#endif
#ifdef APE_OFFLOAD_ENABLED
        true,
#else
        false,
#endif
#ifdef FLAC_OFFLOAD_ENABLED
        true,
#else
        false,
#endif
#ifdef PCM_OFFLOAD_ENABLED_16
        true,
#else
        false,
#endif
#ifdef PCM_OFFLOAD_ENABLED_24
        true,
#else
        false,
#endif
#ifdef QTI_FLAC_DECODER
        true,
#else
        false,
#endif
#ifdef VORBIS_OFFLOAD_ENABLED
        true,
#else
        false,
#endif
#ifdef WMA_OFFLOAD_ENABLED
        true,
#else
        false,
#endif
    };
}

bool AVConfigHelper::useSwAlacDecoder()
{
    AVLOGV("%s, %d", __func__, mConfigs.audio_use_sw_alac_decoder);
    return mConfigs.audio_use_sw_alac_decoder;
}

bool AVConfigHelper::useSwApeDecoder()
{
    AVLOGV("%s, %d", __func__, mConfigs.audio_use_sw_ape_decoder);
    return mConfigs.audio_use_sw_ape_decoder;
}

bool AVConfigHelper::useSwMpeghDecoder()
{
    AVLOGV("%s, %d", __func__, mConfigs.audio_use_sw_mpegh_decoder);
    return mConfigs.audio_use_sw_mpegh_decoder;
}

bool AVConfigHelper::flacSwSupports24Bit()
{
    AVLOGV("%s, %d", __func__, mConfigs.audio_flac_sw_decoder_24bit);
    return mConfigs.audio_flac_sw_decoder_24bit;
}

bool AVConfigHelper::useHwAACEncoder()
{
    AVLOGV("%s, %d", __func__, mConfigs.audio_hw_aac_encoder);
    return mConfigs.audio_hw_aac_encoder;
}

bool AVConfigHelper::isSFRecorderDisabled()
{
    bool ret = false;
    const char *prop = mConfigs.audio_debug_sf_noaudio.c_str();
    if ((strlen(prop) > 0) && (atoi(prop) & 0x02))
        ret = true;

    AVLOGV("%s, %d", __func__, ret);
    return ret;
}

bool AVConfigHelper::isSFPlayerDisabled()
{
    bool ret = false;
    const char *prop = mConfigs.audio_debug_sf_noaudio.c_str();
    if ((strlen(prop) > 0) && (atoi(prop) & 0x01))
        ret = true;

    AVLOGV("%s, %d", __func__, ret);
    return ret;
}

uint32_t AVConfigHelper::getRecordAggrBufDuration()
{
    AVLOGV("%s, %d", __func__, mConfigs.audio_record_aggregate_buffer_duration);
    return mConfigs.audio_record_aggregate_buffer_duration;
}

bool AVConfigHelper::isAACADTSOffloadEnabled()
{
    AVLOGV("%s, %d", __func__, mConfigs.aac_adts_offload_enabled);
    return mConfigs.aac_adts_offload_enabled;
}

bool AVConfigHelper::isAlacOffloadEnabled()
{
    AVLOGV("%s, %d", __func__, mConfigs.alac_offload_enabled);
    return mConfigs.alac_offload_enabled;
}

bool AVConfigHelper::isApeOffloadEnabled()
{
    AVLOGV("%s, %d", __func__, mConfigs.ape_offload_enabled);
    return mConfigs.ape_offload_enabled;
}

bool AVConfigHelper::isFlacOffloadEnabled()
{
    AVLOGV("%s, %d", __func__, mConfigs.flac_offload_enabled);
    return mConfigs.flac_offload_enabled;
}

bool AVConfigHelper::isPCMOffload16BitEnabled()
{
    AVLOGV("%s, %d", __func__, mConfigs.pcm_offload_enabled_16);
    return mConfigs.pcm_offload_enabled_16;
}

bool AVConfigHelper::isPCMOffload24BitEnabled()
{
    AVLOGV("%s, %d", __func__, mConfigs.pcm_offload_enabled_24);
    return mConfigs.pcm_offload_enabled_24;
}

bool AVConfigHelper::useQTIFlacDecoder()
{
    AVLOGV("%s, %d", __func__, mConfigs.qti_flac_decoder);
    return mConfigs.qti_flac_decoder;
}

bool AVConfigHelper::isVorbisOffloadEnabled()
{
    AVLOGV("%s, %d", __func__, mConfigs.vorbis_offload_enabled);
    return mConfigs.vorbis_offload_enabled;
}

bool AVConfigHelper::isWmaOffloadEnabled()
{
    AVLOGV("%s, %d", __func__, mConfigs.wma_offload_enabled);
    return mConfigs.wma_offload_enabled;
}
}
