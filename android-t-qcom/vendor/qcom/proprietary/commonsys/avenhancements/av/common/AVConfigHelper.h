/*
 * Copyright (c) 2019, 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef _AV_CONFIG_HELPER_H_
#define _AV_CONFIG_HELPER_H_

#include <string>
#include <media/stagefright/foundation/ABase.h>

namespace android {

using namespace std;

struct AVValues {
    bool     audio_use_sw_alac_decoder;
    bool     audio_use_sw_ape_decoder;
    bool     audio_use_sw_mpegh_decoder;
    bool     audio_flac_sw_decoder_24bit;
    bool     audio_hw_aac_encoder;
    string   audio_debug_sf_noaudio;
    uint32_t audio_record_aggregate_buffer_duration;
    bool     aac_adts_offload_enabled;
    bool     alac_offload_enabled;
    bool     ape_offload_enabled;
    bool     flac_offload_enabled;
    bool     pcm_offload_enabled_16;
    bool     pcm_offload_enabled_24;
    bool     qti_flac_decoder;
    bool     vorbis_offload_enabled;
    bool     wma_offload_enabled;
};

class AVConfigHelper {
public:
    AVConfigHelper();
    virtual ~AVConfigHelper() {};

    static AVConfigHelper *getInstance();

    /* member functions to query settings */
    bool useSwAlacDecoder();
    bool useSwApeDecoder();
    bool useSwMpeghDecoder();
    bool flacSwSupports24Bit();
    bool useHwAACEncoder();

    bool isSFRecorderDisabled();
    bool isSFPlayerDisabled();
    uint32_t getRecordAggrBufDuration();

    bool isAACADTSOffloadEnabled();
    bool isAlacOffloadEnabled();
    bool isApeOffloadEnabled();
    bool isFlacOffloadEnabled();
    bool isPCMOffload16BitEnabled();
    bool isPCMOffload24BitEnabled();
    bool useQTIFlacDecoder();
    bool isVorbisOffloadEnabled();
    bool isWmaOffloadEnabled();

private:
    AVValues mConfigs;

    DISALLOW_EVIL_CONSTRUCTORS(AVConfigHelper);
};

}

#endif /* _AV_CONFIG_HELPER_H_ */
