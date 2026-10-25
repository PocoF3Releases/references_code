/*
 * Copyright (c) 2015-2017, 2019-2022, Qualcomm Technologies, Inc.
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
#define LOG_TAG "ExtendedAudioSource"
#include <common/AVLog.h>
#include <dlfcn.h>

#include <media/AudioSystem.h>
#include <media/AudioParameter.h>
#include <media/stagefright/AudioSource.h>
#include "ExtendedAudioSource.h"
#include <system/audio.h>
#include <cutils/properties.h>

#include <vector>
#include <string>

#define HDR_LIB_NAME "librecpp_intf.so"
#define LOAD_SYMBOL(X, Y, Z) *(void**)(&X) = dlsym(Y, Z)

#define AUDIO_RECORD_DEFAULT_BUFFER_DURATION 20
#define HDR_PROCESS_SAMPLES 1024
#define FACING_NONE 0
#define FACING_BACK 1
#define FACING_FRONT 2

const char* config_base_path = "/system_ext/etc";
const char* hdr_config_file = "RPP_HDR.txt";
const char* hpf_config_file = "RPP_BiquadHPF_config.txt";
const char* agc_config_file = "RPP_InputAGC_config.txt";
const char* compressor1_config_file = "RPP_InputCompressor_config.txt";
const char* adaptive_eq_L_config_file = "RPP_EQLeftTop_config.txt";
const char* adaptive_eq_R_config_file = "RPP_EQRightBottom_config.txt";
const char* compressor2_config_file =
    "RPP_OutputCompressor_config.txt";
const char* limiter_config_file = "RPP_Limiter_config.txt";

const char* wnr_param_file = "wnr_params.txt";
const char* wnr_calib_file = "wnr_config.txt";

const char* ans_config_file = "ans_config.txt";

namespace android {

using content::AttributionSourceState;
static void AudioRecordCallbackFunction(
    int event,
    void *user,
    void *info
    ) {
    ExtendedAudioSource *source = (ExtendedAudioSource *) user;
    source->onEvent(event, info);
}

static std::vector<std::string> getConfigPath(
    const char* base_path,
    bool landscape,
    int facing,
    bool inverted,
    bool wnr,
    bool ans
    ) {
    std::vector<std::string> path_list;

    std::string base(base_path);
    if (base.at(base.length() - 1) != '/') {
        base.push_back('/');
    }
    base.append("hdr_config/");

    std::string wnr_path(base);
    std::string ans_path(base);

    if (facing == FACING_BACK) {
        base.append("main_");
    } else if (facing == FACING_FRONT) {
        base.append("selfie_");
    } else if (facing == FACING_NONE) {
        base.append("none_");
    }

    if (inverted) {
        base.append("inv_");
    }

    if (landscape) {
        base.append("ls/");
    } else {
        base.append("pt/");
    }

    path_list.push_back(base);

    if (wnr) {
        wnr_path.append("wnr_on/");
    } else {
        wnr_path.append("wnr_off/");
    }
    path_list.push_back(wnr_path);

    if (ans) {
        ans_path.append("ans_on/");
    } else {
        ans_path.append("ans_off/");
    }
    path_list.push_back(ans_path);

    return path_list;
}

ExtendedAudioSource::ExtendedAudioSource(
    const audio_attributes_t *attr,
    const AttributionSourceState& attributionSource,
    uint32_t sampleRate,
    uint32_t channelCount,
    uint32_t outSampleRate,
    audio_port_handle_t selectedDeviceId,
    audio_microphone_direction_t selectedMicDirection,
    float selectedMicFieldDimension)
    : AudioSource(
          attr,
          attributionSource,
          sampleRate,
          channelCount,
          outSampleRate,
          selectedDeviceId,
          selectedMicDirection,
          selectedMicFieldDimension
      ) {
    pConfigsIns = AVConfigHelper::getInstance();

    mMaxBufferSize = kMaxBufferSize;
    mTempBuf.mSize = 0;
    mTempBuf.frameCount = 0;
    mTempBuf.i16 = (short*)NULL;
    mTempBuf.sequence = 0;
    mPrevPosition = 0;
    mAudioSessionId = (audio_session_t)-1;
    mAllocBytes = 0;
    mTransferMode = AudioRecord::TRANSFER_CALLBACK;
    mIsHdrSrc = false;
    mEnableHdr = false;
    mHdrEnabled = false;
    mEnableWnr = false;
    mEnableAns = false;
    mIsLandscape = true;
    mIsInverted = false;
    mFacing = FACING_NONE;
    mAudioChannelCount = 0;
    mOutputChannelCount = 0;
    mAudioSamplingRate = 0;
    mProcessSize = 0;

    int buffDuration = AUDIO_RECORD_DEFAULT_BUFFER_DURATION;
    if (pConfigsIns->getRecordAggrBufDuration() >
            AUDIO_RECORD_DEFAULT_BUFFER_DURATION) {
        buffDuration = pConfigsIns->getRecordAggrBufDuration();
    }

    bool bAggregate  = (((channelCount == 1) || (channelCount == 2)) &&
            ((attr != NULL) && (attr->source == AUDIO_SOURCE_CAMCORDER)));

    getConfigParams_l();

    mOutputChannelCount = channelCount;
    if (mAudioChannelCount == 0) {
        mAudioChannelCount = channelCount;
    }
    if (mAudioSamplingRate == 0) {
        mAudioSamplingRate = sampleRate;
    }

    mIsHdrSrc = (mAudioChannelCount == 4) &&
        (mAudioSamplingRate == 48000) &&
        ((attr != NULL) &&(attr->source == AUDIO_SOURCE_UNPROCESSED)) &&
        property_get_bool("vendor.audio.hdr.record.enable", false) &&
        mEnableHdr;

    if (mIsHdrSrc) {
        mProcessSize =
            HDR_PROCESS_SAMPLES *
            audio_bytes_per_frame(mAudioChannelCount , AUDIO_FORMAT_PCM_16_BIT);
    }

    // make sure that the AudioRecord callback never returns more than the maximum
    // buffer size
    if (bAggregate || mIsHdrSrc) {
        AVLOGD("HDR Recording %s", mIsHdrSrc ? "ENABLED" : "DISABLED");
        mMaxBufferSize = 2 * mAudioChannelCount * mAudioSamplingRate *
            audio_bytes_per_sample(AUDIO_FORMAT_PCM_16_BIT) *
            buffDuration / 1000;
    }

    uint32_t frameCount =
        ((bAggregate || mIsHdrSrc) ? mMaxBufferSize : kMaxBufferSize)
        / sizeof(int16_t) / mAudioChannelCount;

    size_t minFrameCount;
    status_t status = AudioRecord::getMinFrameCount(
                          &minFrameCount,
                          mAudioSamplingRate,
                          AUDIO_FORMAT_PCM_16_BIT,
                          audio_channel_in_mask_from_count(mAudioChannelCount));

    // make sure that the AudioRecord total buffer size is large enough
    size_t bufCount = 2;
    while ((bufCount * frameCount) < minFrameCount) {
        bufCount++;
    }
    //decide whether to use callback or event pos callback
    //use position marker only for mono or stereo capture
    //and if input source is camera
    if (bAggregate) {
        AVLOGD("Creating non default AudioRecord");
        //Need audioSession Id in the extended audio record constructor
        //where the transfer mode can be specified
        mAudioSessionId =
            (audio_session_t)
            AudioSystem::newAudioUniqueId(AUDIO_UNIQUE_ID_USE_SESSION);
        AudioSystem::acquireAudioSessionId(mAudioSessionId,
            (pid_t)-1,
            (uid_t)-1
        );
        mRecord = new AudioRecord(
                          AUDIO_SOURCE_DEFAULT,
                          mAudioSamplingRate,
                          AUDIO_FORMAT_PCM_16_BIT,
                          audio_channel_in_mask_from_count(
                              mAudioChannelCount
                          ),
                          attributionSource,
                          (size_t) (bufCount * frameCount),
                          AudioRecordCallbackFunction,
                          this,
                          frameCount /*notificationFrames*/,
                          mAudioSessionId,
                          AudioRecord::TRANSFER_SYNC,
                          AUDIO_INPUT_FLAG_NONE,
                          attr,
                          selectedDeviceId,
                          selectedMicDirection,
                          selectedMicFieldDimension
                      );
        mRecord->setCallerName("media");
        mInitCheck = mRecord->initCheck();
        if (mInitCheck != OK) {
            mRecord.clear();
        } else {
            /* set to update position after frames worth of buffduration
               time for 16 bits */
            mAllocBytes = ((sizeof(uint8_t) * frameCount *
                           2 * mAudioChannelCount));
            AVLOGD("AudioSource in TRANSFER_SYNC with duration =%d ms"
                " bufCount = %zu frameCount = %d mMaxBufferSize = %zu",
                buffDuration, bufCount, frameCount, mMaxBufferSize);
            mTempBuf.i16 = (short*) malloc(mAllocBytes);
            mTransferMode = AudioRecord::TRANSFER_SYNC;
            mRecord->setPositionUpdatePeriod(
                (mAudioSamplingRate * buffDuration)/1000);
        }
    }

    if (mInitCheck != OK) {
        AVLOGD("Creating default AudioRecord");
        mRecord = new AudioRecord(
                          AUDIO_SOURCE_DEFAULT,
                          mAudioSamplingRate,
                          AUDIO_FORMAT_PCM_16_BIT,
                          audio_channel_in_mask_from_count(
                              mAudioChannelCount
                          ),
                          attributionSource,
                          (size_t) (bufCount * frameCount),
                          AudioRecordCallbackFunction,
                          this,
                          frameCount /*notificationFrames*/,
                          AUDIO_SESSION_ALLOCATE,
                          AudioRecord::TRANSFER_DEFAULT,
                          AUDIO_INPUT_FLAG_NONE,
                          attr,
                          selectedDeviceId,
                          selectedMicDirection,
                          selectedMicFieldDimension
                      );
        mRecord->setCallerName("media");
        mInitCheck = mRecord->initCheck();
        if (mInitCheck != OK) {
            AVLOGE("AudioRecord init failed");
            mRecord.clear();
        }
    }

    if (mIsHdrSrc && mEnableHdr &&
        !mHdrEnabled && (mInitCheck == OK)) {
        if (!setupHdr_l()) {
            AVLOGE("HDR setup failed");
            mInitCheck = NO_INIT;
        } else {
            AVLOGI("HDR setup success");
            mHdrEnabled = true;
        }
    }
}

ExtendedAudioSource::~ExtendedAudioSource() {
    if (mStarted) {
        reset();
    }
    //destroy mRecord explicitly before freeing mTempBuf
    mRecord.clear();
    if (mTransferMode == AudioRecord::TRANSFER_SYNC) {
        if (mTempBuf.i16) {
            free(mTempBuf.i16);
            mTempBuf.i16 = (short*)NULL;
        }
    }

    if (mHdrHandle != nullptr) {
        if (mHdrDeleteObj != nullptr) {
            mHdrDeleteObj(mHdrHandle);
            mHdrHandle = nullptr;
            mHdrGetObj = nullptr;
            mHdrDeleteObj = nullptr;
            mHdrConfig = nullptr;
            mHdrSetParam = nullptr;
            mHdrSetupRamp = nullptr;
            mHdrEnable2ChHdr = nullptr;
            mHdrSetup = nullptr;
            mHdrProcess = nullptr;
            AVLOGD("HDR Object removed");
        }
        dlclose(mHdrLibHandle);
        mHdrLibHandle = nullptr;
    }
}

status_t ExtendedAudioSource::reset() {
    status_t ret = AudioSource::reset();
    if (ret == OK && mTransferMode == AudioRecord::TRANSFER_SYNC) {
        if (mAudioSessionId != -1) {
            AudioSystem::releaseAudioSessionId(mAudioSessionId, -1);
        }

        mAudioSessionId = (audio_session_t)-1;
        mTempBuf.mSize = 0;
        mTempBuf.frameCount = 0;
    }
    return ret;
}

sp<MetaData> ExtendedAudioSource::getFormat() {
    sp<MetaData> meta = AudioSource::getFormat();
    AVLOGD("%s: meta %s mIsHdrSrc %u mHdrEnabled %u", __func__,
        meta == NULL ? "NULL" : "NOT NULL", mIsHdrSrc, mHdrEnabled);

    if ((meta != NULL) &&
        (mIsHdrSrc && mHdrEnabled)) {
        AVLOGD("getFormat - channels %d maxInputSize %zu",
            mRecord->channelCount() / 2, mProcessSize);
        meta->setInt32(kKeyChannelCount, mOutputChannelCount);
        meta->setInt32(kKeyMaxInputSize, mProcessSize * 2);
    }
    return meta;
}

void ExtendedAudioSource::signalBufferReturned(
    MediaBufferBase *buffer) {
    AVLOGV("signalBufferReturned: %p", buffer->data());
    AudioSource::signalBufferReturned(buffer);
    return;
}

status_t ExtendedAudioSource::read(
         MediaBufferBase **out, const ReadOptions * /* options */) {
    AVLOGV("%s", __func__);
    status_t ret = NO_INIT;

    if (!mIsHdrSrc || !mHdrEnabled) {
        return AudioSource::read(out);
    }

    while (mStagingBuf.size() < mProcessSize) {
        ret =  AudioSource::read(out);

        if (ret == OK && *out != NULL) {
            MediaBufferBase* in = *out;
            AVLOGV("AudioSource returned buf of size  %zu samples %u",
                in->range_length(),
                (uint32_t)(in->range_length() /
                           (mRecord->channelCount() * sizeof(int16_t))));
            size_t currStagingBufSize = mStagingBuf.size();
            size_t inBufSize = in->range_length();

            AVLOGV("Staging buf size %zu samples %u", currStagingBufSize,
                (uint32_t)(currStagingBufSize /
                           (mRecord->channelCount() * sizeof(int16_t))));

            if (currStagingBufSize + inBufSize >= mProcessSize) {
                AVLOGV("Staging has sufficient data %zu samples %u",
                    currStagingBufSize + inBufSize,
                    (uint32_t)((currStagingBufSize + inBufSize) /
                                (mRecord->channelCount() * sizeof(int16_t))));
                mStagingBuf.resize(currStagingBufSize + inBufSize);
                memcpy(mStagingBuf.data() + currStagingBufSize,
                    (uint8_t*)in->data(), inBufSize);
            } else {
                AVLOGV(
                    "Staging buf has insufficient data %zu samples %u",
                    currStagingBufSize + inBufSize,
                    (uint32_t)((currStagingBufSize + inBufSize) /
                                (mRecord->channelCount() * sizeof(int16_t))));
                mStagingBuf.resize(currStagingBufSize + inBufSize);
                memcpy(mStagingBuf.data() + currStagingBufSize,
                    (uint8_t*)in->data(), inBufSize);
                AVLOGV("Insufficient data Release buffer %p", in->data());
                in->release();
                in = NULL;
                continue;
            }

            size_t numFrames = mStagingBuf.size() / mProcessSize;
            size_t sizeToProcess = mProcessSize * numFrames;
            size_t remainderSize = mStagingBuf.size() - sizeToProcess;
            AVLOGV("Process size %zu Num frames %zu remaining size %zu",
                sizeToProcess, numFrames, remainderSize);

            std::vector<uint8_t*> tmpBuf;
            if (remainderSize > 0) {
                tmpBuf.resize(remainderSize);
                memcpy(tmpBuf.data(),
                    mStagingBuf.data() + sizeToProcess, remainderSize);
            }

            MediaBuffer* mbuf = new MediaBuffer(sizeToProcess / 2);
            uint8_t* buf = (uint8_t*)mbuf->data();

            if (mHdrEnabled) {
                AVLOGV("Using real HDR Process");
                if (mHdrProcess != nullptr &&
                    mHdrHandle != nullptr) {
                    for (int i = 0; i < numFrames; ++i) {
                        bool ret_val = mHdrProcess(
                            mHdrHandle,
                            (int8_t*)mStagingBuf.data() + (i * mProcessSize),
                            (int8_t*)buf + (i * (mProcessSize / 2))
                        );
                        if (ret_val != true) {
                            AVLOGE("HDR process failed");
                            mbuf->release();
                            in->release();
                            *out = NULL;
                            return NO_INIT;
                        }
                    }
                } else {
                    AVLOGE("HDR process uninitialized");
                    mbuf->release();
                    in->release();
                    *out = NULL;
                    return NO_INIT;
                }
            }
            mbuf->set_range(0, sizeToProcess / 2);

            int64_t keyTime = 0;
            if (!(in->meta_data().findInt64(kKeyTime, &keyTime))) {
                AVLOGE("Frame time not available in buffer");
            }
            mbuf->meta_data().setInt64(kKeyTime, keyTime);

            int64_t keyDriftTime = 0;
            if (!(in->meta_data().findInt64(kKeyDriftTime, &keyDriftTime))) {
                AVLOGE("Frame drift time not available in buffer");
            }
            mbuf->meta_data().setInt64(kKeyDriftTime, keyDriftTime);

            AVLOGV("Sufficient data Release buffer %p", in->data());
            in->release();

            mStagingBuf.clear();
            if (tmpBuf.size() > 0) {
                mStagingBuf.resize(tmpBuf.size());
                memcpy(mStagingBuf.data(), tmpBuf.data(), tmpBuf.size());
                tmpBuf.clear();
            }

            {
                Mutex::Autolock autoLock(mLock);
                ++mNumClientOwnedBuffers;
                mbuf->setObserver(this);
                mbuf->add_ref();
                AVLOGV("Give data to client buffer %p", mbuf->data());
            }
            *out = mbuf;
            AVLOGV("Return %d", ret);
            break;
        } else {
            AVLOGE("AudioSource returned error or NULL buffer");
            break;
        }
    }
    AVLOGV("%s %d", __func__, ret);

    return ret;
}

void ExtendedAudioSource::onEvent(int event, void* info) {

    switch (event) {
        case AudioRecord::EVENT_MORE_DATA: {
            AVLOGV("Received data from AudioRecord");
            AudioSource::onMoreData(*((AudioRecord::Buffer *) info));
            break;
        }
        case AudioRecord::EVENT_NEW_POS: {
            uint32_t position = 0;
            mRecord->getPosition(&position);
            size_t framestoRead = position - mPrevPosition;
            size_t bytestoRead = (framestoRead * 2 * mRecord->channelCount());
            if (bytestoRead > mAllocBytes) {
                //try to read only max
                AVLOGI("more than allocated size in callback, adjusting size");
                bytestoRead =  mAllocBytes;
                framestoRead = (mAllocBytes / (2 * mRecord->channelCount()));
            } else if (framestoRead <= 0) {
                AVLOGV("No data to read!");
                break;
            }

            if (mTempBuf.i16 && framestoRead > 0) {
                //read only if you have valid data
                ssize_t bytesRead = mRecord->read(mTempBuf.i16, bytestoRead);
                size_t framesRead = 0;
                AVLOGV("event_new_pos, new pos %d, frames to read %zu",
                        position, framestoRead);
                AVLOGV("bytes read = %zu", bytesRead);
                if (bytesRead > 0) {
                    framesRead = (bytesRead / (2 * mRecord->channelCount()));
                    mPrevPosition += framesRead;
                    mTempBuf.mSize = bytesRead;
                    mTempBuf.frameCount = framesRead;
                    AudioSource::onMoreData(mTempBuf);
                } else {
                    AVLOGE("EVENT_NEW_POS did not return any data");
                }
            } else {
                AVLOGE("Init error");
            }
            break;
        }
        case AudioRecord::EVENT_OVERRUN: {
            AVLOGW("AudioRecord reported overrun!");
            break;
        }
        default:
            // does nothing
            break;
    }
    return;
}

bool ExtendedAudioSource::hdrSymbolLoader_l(void* library_ptr) {
    LOAD_SYMBOL(mHdrGetObj, library_ptr, "RecorderPP_get_obj");
    if (mHdrGetObj == nullptr) {
        AVLOGE("Error : %s", dlerror());
        return false;
    }
    LOAD_SYMBOL(mHdrDeleteObj, library_ptr, "RecorderPP_delete_obj");
    if (mHdrDeleteObj == nullptr) {
        AVLOGE("Error : %s", dlerror());
        return false;
    }
    LOAD_SYMBOL(mHdrConfig, library_ptr, "RecorderPP_configure");
    if (mHdrConfig == nullptr) {
        AVLOGE("Error : %s", dlerror());
        return false;
    }
    LOAD_SYMBOL(mHdrSetParam, library_ptr, "RecorderPP_set_param");
    if (mHdrSetParam == nullptr) {
        AVLOGE("Error : %s", dlerror());
        return false;
    }
    LOAD_SYMBOL(mHdrSetupRamp, library_ptr, "RecorderPP_setup_ramp");
    if (mHdrSetupRamp == nullptr) {
        AVLOGE("Error : %s", dlerror());
        return false;
    }
    LOAD_SYMBOL(mHdrEnable2ChHdr, library_ptr, "RecorderPP_enable_fake_hdr");
    if (mHdrEnable2ChHdr == nullptr) {
        AVLOGE("Error : %s", dlerror());
        return false;
    }
    LOAD_SYMBOL(mHdrSetup, library_ptr, "RecorderPP_setup");
    if (mHdrSetup == nullptr) {
        AVLOGE("Error : %s", dlerror());
        return false;
    }
    LOAD_SYMBOL(mHdrProcess, library_ptr, "RecorderPP_process");
    if (mHdrProcess == nullptr) {
        AVLOGE("Error : %s", dlerror());
        return false;
    }

    return true;
}

bool ExtendedAudioSource::setupHdr_l() {
    mHdrLibHandle = dlopen(HDR_LIB_NAME, RTLD_LAZY);
    if (mHdrLibHandle == nullptr) {
        AVLOGE("Failed to load HDR library");
        return false;
    }

    if (!hdrSymbolLoader_l(mHdrLibHandle)) {
        AVLOGE("Failed to load symbols from HDR library");
        return false;
    }

    mHdrHandle = mHdrGetObj();
    if (mHdrHandle == nullptr) {
        AVLOGE("HDR object Error!!!");
        return false;
    }

    bool ret_val = true;

    const std::vector<int> hdrConfigurations = {
        sizeof(int16_t), //sample size
        (int) mRecord->channelCount(), //input channels
        (int) mRecord->channelCount() / 2, //output channels
        HDR_PROCESS_SAMPLES, //samples per frame
        mAudioSamplingRate //sample rate
    };

    for (int i = 0; i < hdrConfigurations.size(); i++) {
        ret_val = mHdrConfig(mHdrHandle, i + 1, hdrConfigurations[i]);
        if (!ret_val) {
            AVLOGE("Failed to configure HDR");
            return ret_val;
        }
    }

    std::vector<std::string> path = getConfigPath(config_base_path,
                                                  mIsLandscape,
                                                  mFacing,
                                                  mIsInverted,
                                                  mEnableWnr,
                                                  mEnableAns);

    AVLOGV("%s: Config path %s", __func__, path[0].c_str());

    std::string cfg_file(path[0] + hdr_config_file);
    AVLOGV("hdr_config_file %s", cfg_file.c_str());
    ret_val = mHdrSetParam(mHdrHandle,
        1 /* HDR module ID */,
        0,
        cfg_file.c_str());
    if (!ret_val) {
        AVLOGE("Failed to set HDR config file");
        return ret_val;
    }

    cfg_file.clear();
    cfg_file = path[1] + wnr_param_file;
    AVLOGV("wnr_param_file %s", cfg_file.c_str());
    ret_val = mHdrSetParam(mHdrHandle,
        7 /* WNR module ID */,
        1 /* WNR params */,
        cfg_file.c_str());
    if (!ret_val) {
        AVLOGE("Failed to set WNR param file");
        return ret_val;
    }

    cfg_file.clear();
    cfg_file = path[1] + wnr_calib_file;
    AVLOGV("wnr_calib_file %s", cfg_file.c_str());
    ret_val = mHdrSetParam(mHdrHandle,
        7 /* WNR module ID */,
        2 /* WNR calibration */,
        cfg_file.c_str());
    if (!ret_val) {
        AVLOGE("Failed to set WNR calibration file");
        return ret_val;
    }

    cfg_file.clear();
    cfg_file = path[2] + ans_config_file;
    AVLOGV("ans_config_file %s", cfg_file.c_str());
    ret_val = mHdrSetParam(mHdrHandle,
        9 /* ANS module ID */,
        0,
        cfg_file.c_str());
    if (!ret_val) {
        AVLOGE("Failed to set ANS config file for 3D Audio");
        return ret_val;
    }

    cfg_file.clear();
    cfg_file = path[0] + hpf_config_file;
    AVLOGV("hpf_config_file %s", cfg_file.c_str());
    ret_val = mHdrSetParam(mHdrHandle,
        2 /* HPF module ID */,
        0,
        cfg_file.c_str());
    if (!ret_val) {
        AVLOGE("Failed to set HPF config file");
        return ret_val;
    }

    cfg_file.clear();
    cfg_file = path[0] + agc_config_file;
    AVLOGV("agc_config_file %s", cfg_file.c_str());
    ret_val = mHdrSetParam(mHdrHandle,
        3 /* AGC module ID */,
        0,
        cfg_file.c_str());
    if (!ret_val) {
        AVLOGE("Failed to set AGC config file");
        return ret_val;
    }

    cfg_file.clear();
    cfg_file = path[0] + compressor1_config_file;
    AVLOGV("compressor1_config_file %s", cfg_file.c_str());
    ret_val = mHdrSetParam(mHdrHandle,
        4 /* Compressor module ID */,
        1 /* Input Compressor */,
        cfg_file.c_str());
    if (!ret_val) {
        AVLOGE("Failed to set Input Compressor config file");
        return ret_val;
    }

    cfg_file.clear();
    cfg_file = path[0] + compressor2_config_file;
    AVLOGV("compressor2_config_file %s", cfg_file.c_str());
    ret_val = mHdrSetParam(mHdrHandle,
        4 /* Compressor module ID */,
        2 /* Output Compressor */,
        cfg_file.c_str());
    if (!ret_val) {
        AVLOGE("Failed to set Output Compressor config file");
        return ret_val;
    }

    cfg_file.clear();
    cfg_file = path[0] + adaptive_eq_L_config_file;
    AVLOGV("adaptive_eq_L_config_file %s", cfg_file.c_str());
    ret_val = mHdrSetParam(mHdrHandle,
        5 /* Adaptive Eq module ID */,
        1 /* L channel */,
        cfg_file.c_str());
    if (!ret_val) {
        AVLOGE("Failed to set Adaptive Eq config file");
        return ret_val;
    }

    cfg_file.clear();
    cfg_file = path[0] + adaptive_eq_R_config_file;
    AVLOGV("adaptive_eq_R_config_file %s", cfg_file.c_str());
    ret_val = mHdrSetParam(mHdrHandle,
        5 /* Adaptive Eq module ID */,
        2 /* R channel */,
        cfg_file.c_str());
    if (!ret_val) {
        AVLOGE("Failed to set Adaptive Eq config file");
        return ret_val;
    }

    cfg_file.clear();
    cfg_file = path[0] + limiter_config_file;
    AVLOGV("limiter_config_file %s", cfg_file.c_str());
    ret_val = mHdrSetParam(mHdrHandle,
        6 /* Limiter module ID */,
        0 /* Limiter */,
        cfg_file.c_str());
    if (!ret_val) {
        AVLOGE("Failed to set Limiter config file");
        return ret_val;
    }

    if (property_get_bool("vendor.audio.hdr.record.fake", false)) {
        AVLOGV("2 ch HDR enabled");
        if (!mHdrEnable2ChHdr(mHdrHandle)) {
            AVLOGE("Failed to enable Fake HDR");
        }
    } else {
        AVLOGV("Fake HDR disabled");
    }

    ret_val = mHdrSetup(mHdrHandle);
    if (ret_val != true) {
        AVLOGE("Failed to setup recorder interface");
        return false;
    }

    return true;
}

void ExtendedAudioSource::getConfigParams_l() {
    String8 keys = String8(
        "hdr_record_on;orientation;inverted;facing;wnr_on;ans_on;"
        "hdr_audio_channel_count;hdr_audio_sampling_rate;"
    );
    String8 params = AudioSystem::getParameters(keys);
    AVLOGV("AudioSystem::getParameters - %s", params.c_str());
    AudioParameter kvpair(params);

    String8 hdrKey = String8("hdr_record_on");
    String8 orientationKey = String8("orientation");
    String8 invertedKey = String8("inverted");
    String8 facingKey = String8("facing");
    String8 wnrKey = String8("wnr_on");
    String8 ansKey = String8("ans_on");
    String8 hdrAudioChannelCountKey = String8("hdr_audio_channel_count");
    String8 hdrAudioSamplingRateKey = String8("hdr_audio_sampling_rate");

    String8 hdrVal;
    String8 orientationVal, invertedVal, facingVal, wnrVal, ansVal;
    String8 hdrChannelCountVal, hdrSamplingRateVal;

    if (kvpair.get(hdrKey, hdrVal) == NO_ERROR) {
        AVLOGV("HDR val: %s", hdrVal.c_str());
        if (!strcmp(hdrVal.c_str(), "true"))
            mEnableHdr = true;
    } else {
        AVLOGE("HDR key not found");
    }
    if (kvpair.get(orientationKey, orientationVal) == NO_ERROR) {
        AVLOGV("HDR orientation val: %s", orientationVal.c_str());
        if (!strcmp(orientationVal.c_str(), "portrait"))
            mIsLandscape = false;
    } else {
        AVLOGE("HDR orientation key not found");
    }
    if (kvpair.get(invertedKey, invertedVal) == NO_ERROR) {
        AVLOGV("HDR inverted val: %s", invertedVal.c_str());
        if (!strcmp(invertedVal.c_str(), "true"))
            mIsInverted = true;
    } else {
        AVLOGE("HDR inverted key not found");
    }
    if (kvpair.get(facingKey, facingVal) == NO_ERROR) {
        AVLOGV("HDR facing val: %s", facingVal.c_str());
        if (!strcmp(facingVal.c_str(), "back"))
            mFacing = FACING_BACK;
        else if (!strcmp(facingVal.c_str(), "front"))
            mFacing = FACING_FRONT;
        else
            mFacing = FACING_NONE;
    } else {
        AVLOGE("HDR facing key not found");
    }
    if (kvpair.get(wnrKey, wnrVal) == NO_ERROR) {
        AVLOGV("Wnr val: %s", wnrVal.c_str());
        if (!strcmp(wnrVal.c_str(), "true"))
            mEnableWnr = true;
    } else {
        AVLOGE("Wnr key not found");
    }
    if (kvpair.get(ansKey, ansVal) == NO_ERROR) {
        AVLOGV("Ans val: %s", ansVal.c_str());
        if (!strcmp(ansVal.c_str(), "true"))
            mEnableAns = true;
    } else {
        AVLOGE("Ans key not found");
    }
    if (kvpair.get(hdrAudioChannelCountKey, hdrChannelCountVal) == NO_ERROR) {
        AVLOGV("HDR audio channel count val: %s",
            hdrChannelCountVal.c_str());
        if (mEnableHdr && !strcmp(hdrChannelCountVal.c_str(), "4"))
            mAudioChannelCount = 4;
    } else {
        AVLOGE("HDR audio Channel count not found");
    }
    if (kvpair.get(hdrAudioSamplingRateKey, hdrSamplingRateVal) == NO_ERROR) {
        AVLOGV("HDR audio sampling rate val: %s",
            hdrSamplingRateVal.c_str());
        if (mEnableHdr && !strcmp(hdrSamplingRateVal.c_str(), "48000"))
            mAudioSamplingRate = 48000;
    } else {
        AVLOGE("HDR audio Sampling rate not found");
    }
}

}
