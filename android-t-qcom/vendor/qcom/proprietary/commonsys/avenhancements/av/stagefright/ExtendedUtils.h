/*
 * Copyright (c) 2015-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
/*
 * Copyright (c) 2013 - 2015, The Linux Foundation. All rights reserved.
 */

#ifndef _EXTENDED_UTILS_H_
#define _EXTENDED_UTILS_H_

#include <common/AVConfigHelper.h>
#include <system/audio.h> //for enum audio_format_t
#include <stagefright/AVExtensions.h>

#define MIN_BITRATE_AAC 24000
#define MAX_BITRATE_AAC 192000
#define FLAC_SIGN_SIZE  4
#define BLOCK_FLAC_STREAMINFO 0x00
#define FLAC_SIGN_BYTES "fLaC"

#define AUDIO_OFFLOAD_CODEC_WMA_FORMAT_TAG "music_offload_wma_format_tag"
#define AUDIO_OFFLOAD_CODEC_WMA_BLOCK_ALIGN "music_offload_wma_block_align"
#define AUDIO_OFFLOAD_CODEC_WMA_BIT_PER_SAMPLE "music_offload_wma_bit_per_sample"
#define AUDIO_OFFLOAD_CODEC_WMA_CHANNEL_MASK "music_offload_wma_channel_mask"
#define AUDIO_OFFLOAD_CODEC_WMA_ENCODE_OPTION "music_offload_wma_encode_option"
#define AUDIO_OFFLOAD_CODEC_WMA_ENCODE_OPTION1 "music_offload_wma_encode_option1"
#define AUDIO_OFFLOAD_CODEC_WMA_ENCODE_OPTION2 "music_offload_wma_encode_option2"
#define AUDIO_OFFLOAD_CODEC_FORMAT  "music_offload_codec_format"
#define AUDIO_OFFLOAD_CODEC_FLAC_MIN_BLK_SIZE "music_offload_flac_min_blk_size"
#define AUDIO_OFFLOAD_CODEC_FLAC_MAX_BLK_SIZE "music_offload_flac_max_blk_size"
#define AUDIO_OFFLOAD_CODEC_FLAC_MIN_FRAME_SIZE "music_offload_flac_min_frame_size"
#define AUDIO_OFFLOAD_CODEC_FLAC_MAX_FRAME_SIZE "music_offload_flac_max_frame_size"

#define AUDIO_OFFLOAD_CODEC_ALAC_FRAME_LENGTH "music_offload_alac_frame_length"
#define AUDIO_OFFLOAD_CODEC_ALAC_COMPATIBLE_VERSION "music_offload_alac_compatible_version"
#define AUDIO_OFFLOAD_CODEC_ALAC_BIT_DEPTH "music_offload_alac_bit_depth"
#define AUDIO_OFFLOAD_CODEC_ALAC_PB "music_offload_alac_pb"
#define AUDIO_OFFLOAD_CODEC_ALAC_MB "music_offload_alac_mb"
#define AUDIO_OFFLOAD_CODEC_ALAC_KB "music_offload_alac_kb"
#define AUDIO_OFFLOAD_CODEC_ALAC_NUM_CHANNELS "music_offload_alac_num_channels"
#define AUDIO_OFFLOAD_CODEC_ALAC_MAX_RUN "music_offload_alac_max_run"
#define AUDIO_OFFLOAD_CODEC_ALAC_MAX_FRAME_BYTES "music_offload_alac_max_frame_bytes"
#define AUDIO_OFFLOAD_CODEC_ALAC_AVG_BIT_RATE "music_offload_alac_avg_bit_rate"
#define AUDIO_OFFLOAD_CODEC_ALAC_SAMPLING_RATE "music_offload_alac_sampling_rate"
#define AUDIO_OFFLOAD_CODEC_ALAC_CHANNEL_LAYOUT_TAG "music_offload_alac_channel_layout_tag"

#define AUDIO_OFFLOAD_CODEC_APE_COMPATIBLE_VERSION "music_offload_ape_compatible_version"
#define AUDIO_OFFLOAD_CODEC_APE_COMPRESSION_LEVEL "music_offload_ape_compression_level"
#define AUDIO_OFFLOAD_CODEC_APE_FORMAT_FLAGS "music_offload_ape_format_flags"
#define AUDIO_OFFLOAD_CODEC_APE_BLOCKS_PER_FRAME "music_offload_ape_blocks_per_frame"
#define AUDIO_OFFLOAD_CODEC_APE_FINAL_FRAME_BLOCKS "music_offload_ape_final_frame_blocks"
#define AUDIO_OFFLOAD_CODEC_APE_TOTAL_FRAMES "music_offload_ape_total_frames"
#define AUDIO_OFFLOAD_CODEC_APE_BITS_PER_SAMPLE "music_offload_ape_bits_per_sample"
#define AUDIO_OFFLOAD_CODEC_APE_NUM_CHANNELS "music_offload_ape_num_channels"
#define AUDIO_OFFLOAD_CODEC_APE_SAMPLE_RATE "music_offload_ape_sample_rate"
#define AUDIO_OFFLOAD_CODEC_APE_SEEK_TABLE_PRESENT "music_offload_seek_table_present"

#define AUDIO_OFFLOAD_CODEC_VORBIS_BITSTREAM_FMT "music_offload_vorbis_bitstream_fmt"
namespace android {

// TODO: move to a separate file; import all cousins from QCMetaData.h
enum {
    kKeyLocalBatchSize    = 'btch', //int32_t
};

struct MediaCodec;

struct ExtendedUtils : public AVUtils {

    static const char* getMsgFromKey(int key);
    status_t convertMetaDataToMessage(
            const MetaDataBase *meta, sp<AMessage> *format) override;
    virtual status_t convertMetaDataToMessage(
            const sp<MetaData> &meta, sp<AMessage> *format);
    virtual status_t convertMessageToMetaData(
            const sp<AMessage> &msg, sp<MetaData> &meta);
#ifdef P_BRINGUP
    virtual MediaExtractor::SnifferFunc getExtendedSniffer();
#endif //P_BRINGUP
    virtual sp<MediaCodec> createCustomComponentByName(const sp<ALooper> &looper,
            const char* mime, bool encoder, const sp<AMessage> &msg);
    virtual bool isEnhancedExtension(const char *extension);
    virtual bool isAudioSourceAggregate(const audio_attributes_t *attr, uint32_t channelCount);

    virtual status_t mapMimeToAudioFormat(audio_format_t&, const char*);
    virtual status_t sendMetaDataToHal(const sp<MetaData>& meta, AudioParameter *param);
    virtual bool hasAudioSampleBits(const sp<MetaData> &meta);
    virtual bool hasAudioSampleBits(const sp<AMessage> &format);
    virtual int32_t getAudioSampleBits(const sp<MetaData> &meta);
    virtual int32_t getAudioSampleBits(const sp<AMessage> &format);

    virtual audio_format_t updateAudioFormat(audio_format_t audioFormat,
            const sp<AMessage> &format);
    virtual audio_format_t updateAudioFormat(audio_format_t audioFormat,
            const sp<MetaData> &meta);

    virtual bool canOffloadStream(const sp<MetaData> &meta);

    virtual int32_t getAudioMaxInputBufferSize(audio_format_t audioFormat, const sp<AMessage> &);

    virtual bool mapAACProfileToAudioFormat(const sp<MetaData> &,
                          audio_format_t &,
                          uint64_t /*eAacProfile*/);

    virtual bool mapAACProfileToAudioFormat(const sp<AMessage> &,
                          audio_format_t &,
                          uint64_t /*eAacProfile*/);

    virtual void extractCustomCameraKeys(
            const CameraParameters& params, sp<MetaData> &meta);
    virtual void printFileName(int fd);
    virtual void addDecodingTimesFromBatch(MediaBufferBase * buf,
            List<int64_t> &decodeTimeQueue);

    virtual bool useQCHWEncoder(const sp<AMessage> &outputFormat, Vector<AString> *role);

    static bool isHwAudioDecoderSessionAllowed(const sp<AMessage> &format);
    virtual const char *getComponentRole(bool isEncoder, const char *mime);
    bool isAudioMuxFormatSupported(const char *mime);
    virtual void cacheCaptureBuffers(sp<hardware::ICamera> camera, video_encoder encoder);
    virtual void getHFRParams(bool* isHFR, int32_t* batch_size, sp<AMessage> format);
    virtual int64_t overwriteTimeOffset(bool isHFR,
            int64_t inputBufferTimeOffsetUs, int64_t *prevBufferTimestampUs,
            int64_t timeUs,int32_t batchSize);

    virtual bool IsHevcIDR(const sp<ABuffer> &accessUnit);
    virtual sp<AMessage> fillExtradata(sp<MediaCodecBuffer> &buffer, sp<AMessage> &format);

private:
    AVConfigHelper *pConfigsIns;
    // ----- NO TRESSPASSING BEYOND THIS LINE ------
protected:
    virtual ~ExtendedUtils();

private:
    ExtendedUtils(const ExtendedUtils&);
    ExtendedUtils &operator=(const ExtendedUtils&);
    bool isAudioWMAPro(const sp<MetaData> &meta);
    bool isAudioWMAPro(const sp<AMessage> &meta);
    bool overrideMimeType(const sp<AMessage> &msg, AString* mime);
    status_t parseFlacMetaData(const sp<MetaData>& meta, AudioParameter *param);
public:
    ExtendedUtils();
};

extern "C" AVUtils *createExtendedUtils();

} //namespace android

#endif //_EXTENDED_UTILS_H_

