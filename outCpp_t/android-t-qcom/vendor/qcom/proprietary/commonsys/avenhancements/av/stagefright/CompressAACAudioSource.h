/*
 * Copyright (c) 2022, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 *
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

#ifndef COMPRESS_AAC_AUDIO_SOURCE_H_

#define COMPRESS_AAC_AUDIO_SOURCE_H_

#include <media/AudioRecord.h>
#include <media/AudioSystem.h>
#include <media/MicrophoneInfo.h>
#include <media/stagefright/AudioSource.h>
#include <system/audio.h>
#include <utils/List.h>

#include <unordered_map>
#include <vector>

namespace android {

using content::AttributionSourceState;

// RAW MP4FF header
static const constexpr uint32_t AUDAAC_MAX_MP4FF_HEADER_LENGTH = 2;
static const constexpr uint32_t AUDAAC_MP4FF_OBJ_TYPE = 5;
static const constexpr uint32_t AUDAAC_MP4FF_FREQ_IDX = 4;
static const constexpr uint32_t AUDAAC_MP4FF_CH_CONFIG = 4;

static const std::vector<uint32_t> AacEncSampleRateSet = {
    8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000};

static const std::unordered_map<uint32_t, uint32_t> sampleRateToSampleIdx = {
    {8000, 0x0b},  {11025, 0x0a}, {12000, 0x09}, {16000, 0x08}, {22050, 0x07},
    {24000, 0x06}, {32000, 0x05}, {44100, 0x04}, {48000, 0x03}, {64000, 0x02},
};

// must be consistent with the Audio HAL default's
static const std::unordered_map<uint32_t, int32_t> kSampleRateToDefaultBitRate =
    {{8000, 36000},   {11025, 48000},  {12000, 56000},
     {16000, 64000},  {22050, 82500},  {24000, 96000},
     {32000, 110000}, {44100, 128000}, {48000, 156000}};

static const uint16_t kSyncWord = 0xFFF;

static const uint32_t AacEncoderPCMSampleSize = 1024;

static const uint32_t AacEncoderPCMBitWidth = 16;

const static String8 kAudioParameterDSPAacBitRate{"dsp_aac_audio_bitrate"};

class AudioRecord;

struct CompressAACAudioSource : public AudioSource {
    struct adts_fixed_header {
        uint16_t syncword : 12;
        uint8_t id : 1;
        uint8_t layer : 2;
        uint8_t protection_absent : 1;
        uint8_t profile : 2;
        uint8_t sampling_frequency_index : 4;
        uint8_t private_bit : 1;
        uint8_t channel_configuration : 3;
        uint8_t original_copy : 1;
        uint8_t home : 1;
    };
    typedef struct adts_fixed_header adts_fixed_header;

    struct adts_variable_header {
        uint8_t copyright_identification_bit : 1;
        uint8_t copyright_identification_start : 1;
        uint16_t aac_frame_length : 13;
        uint16_t adts_buffer_fullness : 11;
        uint8_t number_of_raw_data_blocks_in_frame : 2;
    };
    typedef struct adts_variable_header adts_variable_header;

    struct adts_header {
        adts_fixed_header fixed_header;
        adts_variable_header variable_header;
    };
    typedef struct adts_header adts_header;

    adts_header *mHeader;


    // Note that the "channels" parameter _is_ the number of channels,
    // _not_ a bitmask of audio_channels_t constants.
    CompressAACAudioSource(
        audio_format_t audioFormat, int32_t audioReqBitRate,
        const audio_attributes_t *attr,
        const AttributionSourceState &attributionSource, uint32_t sampleRate,
        uint32_t channels, uint32_t outSampleRate = 0,
        audio_port_handle_t selectedDeviceId = AUDIO_PORT_HANDLE_NONE,
        audio_microphone_direction_t selectedMicDirection =
            MIC_DIRECTION_UNSPECIFIED,
        float selectedMicFieldDimension = MIC_FIELD_DIMENSION_NORMAL);

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop() { return reset(); }
    virtual sp<MetaData> getFormat();

    virtual status_t read(MediaBufferBase **buffer,
                          const ReadOptions *options = NULL);
    virtual status_t pause();
    virtual int64_t getFirstSampleSystemTimeUs();
    virtual status_t setStopTimeUs(int64_t stopTimeUs);

    status_t dataCallback(AudioRecord::Buffer *buffer);
    void setDSPBitRate();
    int32_t getBitRate() const { return mAudioBitRate; }
    virtual void signalBufferReturned(MediaBufferBase *buffer);

   protected:
    virtual ~CompressAACAudioSource();

    bool mPaused;
    int64_t mFirstSampleSystemTimeUs;
    int64_t mPCMNumFramesReceived;
    int64_t mPCMNumFramesSkipped;
    int32_t mAudioBitRate = 0;
    int32_t mAudioBitRateReq;
    audio_format_t mAudioFormat;

    List<MediaBuffer *> mEncodedBuffersFormed;
    std::vector<uint8_t> mByteStream;

    void queueInputBuffer_l(MediaBuffer *buffer, int64_t timeUs);
    void releaseQueuedFrames_l();
    void waitOutstandingEncodingFrames_l();
    virtual status_t reset();

    CompressAACAudioSource(const CompressAACAudioSource &);
    CompressAACAudioSource &operator=(const CompressAACAudioSource &);

    void set(const audio_attributes_t *attr,
             const AttributionSourceState &attributionSource,
             uint32_t sampleRate, uint32_t channels, uint32_t outSampleRate = 0,
             audio_port_handle_t selectedDeviceId = AUDIO_PORT_HANDLE_NONE,
             audio_microphone_direction_t selectedMicDirection =
                 MIC_DIRECTION_UNSPECIFIED,
             float selectedMicFieldDimension = MIC_FIELD_DIMENSION_NORMAL);

    int32_t doAdtsFrameReConstruction(uint8_t *buf, size_t bufSize);
    bool isNextFramePossible(const std::vector<uint8_t> &byteStream,
                             uint16_t &nextFrameSize);
    uint16_t getAACFramePCMSize() {
        return mRecord->channelCount() * (AacEncoderPCMBitWidth) / 8;
    }
};

}  // namespace android

#endif  // COMPRESS_AAC_AUDIO_SOURCE_H_
