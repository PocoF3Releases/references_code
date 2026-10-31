/*
 * Copyright (c) 2022, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 *
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

#include <inttypes.h>
#include <stdlib.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "CompressAACAudioSource"
#include <binder/IPCThreadState.h>
#include <cutils/properties.h>
#include <media/AidlConversion.h>
#include <media/AudioRecord.h>
#include <media/openmax/OMX_Audio.h>
#include <media/stagefright/AudioSource.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MetaDataUtils.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ALooper.h>
#include <stagefright/AVExtensions.h>
#include <utils/Log.h>

#include "CompressAACAudioSource.h"

#define MIN_ADTS_LENGTH_TO_PARSE 9
#define ADTS_HEADER_LENGTH 7

namespace android {

using content::AttributionSourceState;

static void AudioRecordCallbackFunction(int event, void *user, void *info) {
    CompressAACAudioSource *source = (CompressAACAudioSource *)user;
    switch (event) {
        case AudioRecord::EVENT_MORE_DATA: {
            source->dataCallback((AudioRecord::Buffer *)info);
            break;
        }
        case AudioRecord::EVENT_OVERRUN: {
            ALOGW("AudioRecord reported overrun!");
            break;
        }
        default:
            // does nothing
            break;
    }
}

CompressAACAudioSource::CompressAACAudioSource(
    audio_format_t audioFormat, int32_t audioReqBitRate,
    const audio_attributes_t *attr,
    const AttributionSourceState &attributionSource, uint32_t sampleRate,
    uint32_t channelCount, uint32_t outSampleRate,
    audio_port_handle_t selectedDeviceId,
    audio_microphone_direction_t selectedMicDirection,
    float selectedMicFieldDimension)
    : AudioSource() {
    mAudioFormat = audioFormat;
    mAudioBitRateReq = audioReqBitRate;
    set(attr, attributionSource, sampleRate, channelCount,
        outSampleRate, selectedDeviceId, selectedMicDirection,
        selectedMicFieldDimension);
}

void CompressAACAudioSource::set(
    const audio_attributes_t *attr,
    const AttributionSourceState &attributionSource, uint32_t sampleRate,
    uint32_t channelCount, uint32_t outSampleRate,
    audio_port_handle_t selectedDeviceId,
    audio_microphone_direction_t selectedMicDirection,
    float selectedMicFieldDimension) {
    mStarted = false;
    mPaused = false;
    mSampleRate = sampleRate;
    mOutSampleRate = outSampleRate > 0 ? outSampleRate : sampleRate;
    mTrackMaxAmplitude = false;
    mStartTimeUs = 0;
    mStopSystemTimeUs = -1;
    mFirstSampleSystemTimeUs = -1LL;
    mLastFrameTimestampUs = 0;
    mMaxAmplitude = 0;
    mPrevSampleTimeUs = 0;
    mInitialReadTimeUs = 0;
    mPCMNumFramesReceived = 0;
    mPCMNumFramesSkipped = 0;
    mNumFramesLost = 0;
    mNumClientOwnedBuffers = 0;
    mNoMoreFramesToRead = false;
    CHECK(channelCount == 1 || channelCount == 2);
    CHECK(sampleRate > 0);

    mRecord = new AudioRecord(
        AUDIO_SOURCE_DEFAULT, sampleRate, mAudioFormat,
        audio_channel_in_mask_from_count(channelCount), attributionSource, 200,
        AudioRecordCallbackFunction, this, 100 /*notificationFrames*/,
        AUDIO_SESSION_ALLOCATE,
        AudioRecord::TRANSFER_CALLBACK, /* force to callback to reconstrcut the
                                           encoded frames*/
        AUDIO_INPUT_FLAG_NONE, attr, selectedDeviceId, selectedMicDirection,
        selectedMicFieldDimension);

    // Set caller name so it can be logged in destructor.
    // MediaMetricsConstants.h: AMEDIAMETRICS_PROP_CALLERNAME_VALUE_MEDIA
    mRecord->setCallerName("media");
    mInitCheck = mRecord->initCheck();
    if (mInitCheck != OK) {
        ALOGE("%s: failed to create AudioRecord", __func__);
        mRecord.clear();
        return;
    } else {
        ALOGV("%s: AudioRecord created successfully with session id: %d",
              __func__, mRecord->getSessionId());
    }

    // default bitRate
    mAudioBitRate = kSampleRateToDefaultBitRate.at(mRecord->getSampleRate());

    if (mAudioBitRateReq > 0) setDSPBitRate();

    ALOGI("%s:created compress audio source (%p) with %s format", __func__,
          this, audio_format_to_string(mAudioFormat));
}

void CompressAACAudioSource::setDSPBitRate() {
    status_t err = OK;
    String8 result = kAudioParameterDSPAacBitRate;
    result.appendFormat("=%d;", mAudioBitRateReq);

    err = mRecord->setParameters(result);
    if (err != OK) {
        ALOGE("%s: setParameters failed for CompressAAC audio", __func__);
        return;
    } else {
        ALOGV("%s: setParameters success for CompressAAC audio; %s", __func__,
              result.string());
    }

    int32_t DSPBitRate;
    result = mRecord->getParameters(kAudioParameterDSPAacBitRate);
    AudioParameter audioParam = AudioParameter(result);
    if (audioParam.getInt(kAudioParameterDSPAacBitRate, DSPBitRate) ==
        NO_ERROR) {
        mAudioBitRate = DSPBitRate;
        ALOGI("%s: CompressAAC with DSP bitrate: %d", __func__, mAudioBitRate);
    } else {
        ALOGI("%s: CompressAAC with default bitrate: %d", __func__,
              mAudioBitRate);
    }
}

CompressAACAudioSource::~CompressAACAudioSource() {
    ALOGV("%s: destroying compress aac audio source (%p)", __func__, this);
    if (mStarted) {
        reset();
    }
    mRecord.clear();
    mRecord = 0;
    mByteStream.clear();
}


status_t CompressAACAudioSource::start(MetaData *params) {
    Mutex::Autolock autoLock(mLock);
    /**
     * source started and not paused
     * */
    if (mStarted && !mPaused) {
        ALOGW("%s: source already started but not paused",__func__);
        return OK;
    }

    /**
     * AudioRecord init check
     * */
    if (mInitCheck != OK) {
        ALOGE("%s: source not initialized yet",__func__);
        return NO_INIT;
    }

    /**
     * resume the source when
     * source is started but paused
     * */
    if (mStarted && mPaused) {
        mPaused = false;
        ALOGV("%s: resume the source", __func__);
        return OK;
    }
    mTrackMaxAmplitude = false;
    mMaxAmplitude = 0;
    mInitialReadTimeUs = 0;
    mStartTimeUs = 0;

    int64_t startTimeUs;
    if (params && params->findInt64(kKeyTime, &startTimeUs)) {
        mStartTimeUs = startTimeUs;
    }

    status_t err = mRecord->start();
    if (err == OK) {
        mStarted = true;
        ALOGV("%s: AudioRecord started with session id: %d", __func__,
              mRecord->getSessionId());
    } else {
        ALOGE("%s: AudioRecord start failed", __func__);
        mRecord.clear();
    }

    return err;
}

void CompressAACAudioSource::releaseQueuedFrames_l() {
    ALOGV("releaseQueuedFrames_l");
    List<MediaBuffer *>::iterator it;
    while (!mBuffersReceived.empty()) {
        it = mBuffersReceived.begin();
        // (*it)->release();
        mBuffersReceived.erase(it);
    }
}

void CompressAACAudioSource::waitOutstandingEncodingFrames_l() {
    ALOGV("waitOutstandingEncodingFrames_l: %" PRId64, mNumClientOwnedBuffers);
    while (mNumClientOwnedBuffers > 0) {
        mFrameEncodingCompletionCondition.wait(mLock);
    }
}

status_t CompressAACAudioSource::reset() {
    Mutex::Autolock autoLock(mLock);

    if (!mStarted) {
        return OK;
    }

    if (mInitCheck != OK) {
        return NO_INIT;
    }

    mStarted = false;
    mStopSystemTimeUs = -1;
    mRecord->stop();

    mEncodedBuffersFormed.clear();
    mNoMoreFramesToRead = true;
    mFrameAvailableCondition.signal();
    waitOutstandingEncodingFrames_l();
    releaseQueuedFrames_l();
    ALOGV("%s: AudioRecord stop called with session id: %d", __func__,
          mRecord->getSessionId());
    mStarted = false;
    return OK;
}

sp<MetaData> CompressAACAudioSource::getFormat() {
    uint16_t profile;

    Mutex::Autolock autoLock(mLock);
    if (mInitCheck != OK) {
        return 0;
    }

    sp<MetaData> meta = new MetaData;

    // OMX_AUDIO_AACObjectLC, OMX_AUDIO_AACObjectHE, OMX_AUDIO_AACObjectHE_PS
    if (mAudioFormat == AUDIO_FORMAT_AAC_ADTS_LC ||
        mAudioFormat == AUDIO_FORMAT_AAC_LC) {
        profile = OMX_AUDIO_AACObjectLC - 1;
    } else if (mAudioFormat == AUDIO_FORMAT_AAC_ADTS_HE_V1) {
        profile = OMX_AUDIO_AACObjectHE - 1;
    } else if (mAudioFormat == AUDIO_FORMAT_AAC_ADTS_HE_V2) {
        profile = OMX_AUDIO_AACObjectHE_PS - 1;
    } else {
        ALOGE("%s: metadata not supported for given audio format", __func__);
        return nullptr;
    }

    if (!MakeAACCodecSpecificData(
            *meta, profile, sampleRateToSampleIdx.at(mRecord->getSampleRate()),
            mRecord->channelCount())) {
        ALOGE("%s: metadata failed", __func__);
        return 0;
    }
    meta->setInt32(kKeyMaxInputSize, kMaxBufferSize);
    meta->setInt32(kKeyPcmEncoding, kAudioEncodingPcm16bit);
    meta->setInt32(kKeyBitRate,mAudioBitRate);
    meta->setInt32(kKeyMaxBitRate,mAudioBitRate);

    ALOGD("%s: metadata success", __func__);
    return meta;
}

status_t CompressAACAudioSource::read(MediaBufferBase **out,
                                      const ReadOptions * /* options */) {
    Mutex::Autolock autoLock(mLock);
    *out = NULL;

    if (mInitCheck != OK) {
        ALOGE("%s: AudioRecord init check failed", __func__);
        return NO_INIT;
    }
    if (!mStarted) {
        return OK;
    }

    /**
     * source started but either no buffer available
     * or source paused
     * */
    while (mStarted && (mBuffersReceived.empty() || mPaused)) {
        ALOGV_IF(mPaused, "%s: source paused, wait until resume", __func__);
        ALOGV_IF(mBuffersReceived.empty(), "%s: wait until compress data",
                 __func__);
        mFrameAvailableCondition.wait(mLock);
        if (mNoMoreFramesToRead) {
            return OK;
        }
    }

    MediaBuffer *buffer = *mBuffersReceived.begin();
    mBuffersReceived.erase(mBuffersReceived.begin());
    ++mNumClientOwnedBuffers;
    buffer->setObserver(this);
    buffer->add_ref();

    // Mute/suppress the recording sound
    int64_t timeUs;
    CHECK(buffer->meta_data().findInt64(kKeyTime, &timeUs));
    int64_t elapsedTimeUs = timeUs - mStartTimeUs;
    if (elapsedTimeUs < kAutoRampStartUs) {
        ALOGW("%s: elapsed time is less than  StartTime", __func__);
        memset((uint8_t *)buffer->data(), 0, buffer->range_length());
    } else if (elapsedTimeUs < kAutoRampStartUs + kAutoRampDurationUs) {
        ALOGW("%s: aac data is not adjusted for rampvolume", __func__);
    }

    *out = buffer;
    return OK;
}

status_t CompressAACAudioSource::pause() {
    Mutex::Autolock autoLock(mLock);
    mPaused = true;
    ALOGV("%s: paused", __func__);
    return OK;
}

status_t CompressAACAudioSource::setStopTimeUs(int64_t stopTimeUs) {
    Mutex::Autolock autoLock(mLock);
    ALOGV("Set stoptime: %lld us", (long long)stopTimeUs);

    if (stopTimeUs < -1) {
        ALOGE("Invalid stop time %lld us", (long long)stopTimeUs);
        return BAD_VALUE;
    } else if (stopTimeUs == -1) {
        ALOGI("reset stopTime to be -1");
    }

    mStopSystemTimeUs = stopTimeUs;
    return OK;
}

void CompressAACAudioSource::signalBufferReturned(MediaBufferBase *buffer) {
    ALOGV("signalBufferReturned: %p", buffer->data());
    Mutex::Autolock autoLock(mLock);
    --mNumClientOwnedBuffers;
    buffer->setObserver(0);
    buffer->release();
    mFrameEncodingCompletionCondition.signal();
    return;
}

bool CompressAACAudioSource::isNextFramePossible(
    const std::vector<uint8_t> &byteStream, uint16_t &nextFrameSize) {
    if (byteStream.size() <= MIN_ADTS_LENGTH_TO_PARSE) return false;
    if (kSyncWord ==
        ((((uint16_t)byteStream[0]) << 4) | (byteStream[1] >> 4))) {
        // framestart
        // framesize is between  31 to 43 (both bits included) from
        // framestart
        nextFrameSize =
            ((((uint16_t)(byteStream[3] & 0x03)) << 11) |
             (((uint16_t)byteStream[4]) << 3) | (byteStream[5] >> 5));
    } else {
        ALOGE("%s: No syncword detected", __func__);
    }
    if (byteStream.size() >= nextFrameSize) return true;
    return false;
}

int32_t CompressAACAudioSource::doAdtsFrameReConstruction(uint8_t *buf,
                                                          size_t bufSize) {
    uint16_t nextFrameSize = 0;
    int32_t numEncodedFramesGenerated = 0;
    ALOGV("%s: audio buffer recevied, size:%zu called with source object %x",
          __func__, bufSize, this);
    if (bufSize <= 0) {
        return 0;
    }
    std::copy(buf, buf + bufSize, std::back_inserter(mByteStream));

    while (isNextFramePossible(mByteStream, nextFrameSize) && nextFrameSize > 0) {
        // construct frame with nextFrameSize
        ALOGV("%s: ADTS frame size:%d", __func__, nextFrameSize);
        // generate new buffer
        MediaBuffer *buffer =
            new MediaBuffer(nextFrameSize - ADTS_HEADER_LENGTH);
        memcpy((uint8_t *)buffer->data(),
               mByteStream.data() + ADTS_HEADER_LENGTH,
               nextFrameSize - ADTS_HEADER_LENGTH);
        buffer->set_range(0, nextFrameSize - ADTS_HEADER_LENGTH);
        mEncodedBuffersFormed.push_back(buffer);
        mByteStream.erase(mByteStream.begin(),
                          mByteStream.begin() + nextFrameSize);
        nextFrameSize = 0;
        numEncodedFramesGenerated++;
    }

    return numEncodedFramesGenerated;
}

void CompressAACAudioSource::queueInputBuffer_l(MediaBuffer *buffer,
                                                int64_t timeUs) {
    const size_t bufferSize = buffer->range_length();
    const size_t frameSize = getAACFramePCMSize();
    if (mPCMNumFramesReceived == 0) {
        buffer->meta_data().setInt64(kKeyAnchorTime, mStartTimeUs);
    }
    /* since one encoded buffer is worth of 1024 PCM sample */
    mPCMNumFramesReceived += AacEncoderPCMSampleSize;
    const int64_t timestampUs =
        mStartTimeUs +
        ((1000000LL * mPCMNumFramesReceived) + (mSampleRate >> 1)) /
            mSampleRate;
    if (mFirstSampleSystemTimeUs < 0LL) {
        mFirstSampleSystemTimeUs = timeUs;
    }
    buffer->meta_data().setInt64(kKeyTime, mPrevSampleTimeUs);
    buffer->meta_data().setInt64(kKeyDriftTime, timeUs - mInitialReadTimeUs);
    mPrevSampleTimeUs = timestampUs;
    mBuffersReceived.push_back(buffer);
    mFrameAvailableCondition.signal();
}

status_t CompressAACAudioSource::dataCallback(AudioRecord::Buffer *audioBuf) {
    int64_t timeUs, position, timeNs;
    ExtendedTimestamp ts;
    ExtendedTimestamp::Location location;
    const int32_t usPerSec = 1000000;
    int32_t numEncodedFramesGenerated = 0;

    numEncodedFramesGenerated =
        doAdtsFrameReConstruction((uint8_t *)audioBuf->raw, audioBuf->size());

    while (numEncodedFramesGenerated--) {
        if (mRecord->getTimestamp(&ts) == OK &&
            ts.getBestTimestamp(&position, &timeNs,
                                ExtendedTimestamp::TIMEBASE_MONOTONIC,
                                &location) == OK) {
            // Intention is to always use LOCATION_KERNEL
            if (location == ExtendedTimestamp::LOCATION_KERNEL) {
                ALOGV(
                    "%s: using timestamp of LOCATION_KERNEL: %d, postion: "
                    "%d, timeNs: %d",
                    __func__, location, position, timeNs);
            } else {
                ALOGE(
                    "%s: using timestamp of unintended location: %d, postion: "
                    "%d, timeNs: %d",
                    __func__, location, position, timeNs);
            }
            // Use audio timestamp.
            timeUs = timeNs / 1000 - (position - mPCMNumFramesSkipped -
                                      mPCMNumFramesReceived + mNumFramesLost) *
                                         usPerSec / mSampleRate;
        } else {
            // This should not happen in normal case.
            ALOGW("Failed to get audio timestamp, fallback to use systemclock");
            timeUs = systemTime() / 1000LL;
            // Estimate the real sampling time of the 1st sample in this buffer
            // from AudioRecord's latency. (Apply this adjustment first so that
            // the start time logic is not affected.)
            timeUs -= mRecord->latency() * 1000LL;
        }

        ALOGV("dataCallbackTimestamp: %" PRId64 " us", timeUs);

        Mutex::Autolock autoLock(mLock);
        if (mPaused) {
            mEncodedBuffersFormed.clear();
            ALOGV("%s: need not queue the buffer when paused", __func__);
            return OK;
        }
        if (!mStarted) {
            mEncodedBuffersFormed.clear();
            ALOGW("%s: Spurious callback from AudioRecord. Drop the audio data",
                  __func__);
            return OK;
        }

        // Drop retrieved and previously lost audio data.
        if (mPCMNumFramesReceived == 0 && timeUs < mStartTimeUs) {
            (void)mRecord->getInputFramesLost();
            int64_t receievedFrames = 1;
            ALOGD("Drop audio data(%" PRId64 " frames) at %" PRId64 "/%" PRId64
                  " us",
                  receievedFrames, timeUs, mStartTimeUs);
            mEncodedBuffersFormed.erase(mEncodedBuffersFormed.begin());
            mPCMNumFramesSkipped += AacEncoderPCMSampleSize;
            continue;
        }

        if (mStopSystemTimeUs != -1 && timeUs >= mStopSystemTimeUs) {
            ALOGD("Drop Audio frame at %lld  stop time: %lld us",
                  (long long)timeUs, (long long)mStopSystemTimeUs);
            mNoMoreFramesToRead = true;
            mFrameAvailableCondition.signal();
            mEncodedBuffersFormed.erase(mEncodedBuffersFormed.begin());
            continue;
        }

        if (mPCMNumFramesReceived == 0 && mPrevSampleTimeUs == 0) {
            mInitialReadTimeUs = timeUs;
            // Initial delay
            if (mStartTimeUs > 0) {
                mStartTimeUs = timeUs - mStartTimeUs;
            }
            mPrevSampleTimeUs = mStartTimeUs;
        }
        mLastFrameTimestampUs = timeUs;

        MediaBuffer *buffer = *mEncodedBuffersFormed.begin();
        mEncodedBuffersFormed.erase(mEncodedBuffersFormed.begin());
        queueInputBuffer_l(buffer, timeUs);
    }
    return OK;
}


int64_t CompressAACAudioSource::getFirstSampleSystemTimeUs() {
    Mutex::Autolock autoLock(mLock);
    return mFirstSampleSystemTimeUs;
}

}  // namespace android
