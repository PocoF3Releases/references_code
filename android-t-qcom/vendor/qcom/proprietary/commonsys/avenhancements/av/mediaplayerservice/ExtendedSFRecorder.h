/*
 * Copyright (c) 2015-2017, 2019,2021-2022 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 * Apache license notifications and license are retained
 * for attribution purposes only.
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

#ifndef _EXTENDED_SF_RECORDER_H_
#define _EXTENDED_SF_RECORDER_H_

#include <common/AVConfigHelper.h>
#include <binder/AppOpsManager.h>

namespace android {

using content::AttributionSourceState;

struct StagefrightRecorder;
struct MediaSource;

struct ExtendedSFRecorder : public StagefrightRecorder {
    ExtendedSFRecorder(const AttributionSourceState& attributionSource);

    virtual status_t setAudioSource(audio_source_t);
    virtual status_t setAudioEncoder(audio_encoder ae);
    virtual void setupCustomVideoEncoderParams(sp<MediaSource> cameraSource,
            sp<AMessage> &format);
    virtual sp<MediaSource> setPCMRecording();
    virtual status_t handleCustomOutputFormats();
    virtual status_t handleCustomRecording();
    virtual status_t handleCustomAudioSource(sp<AMessage> format);
    virtual status_t handleCustomAudioEncoder();
    virtual status_t pause();
    virtual status_t resume();
    bool isCompressAudioRecordingEligible();
    virtual bool isCompressAudioRecordingSupported() override;
    virtual sp<AudioSource> setCompressAudioRecording() override;

protected:
    sp<MediaSource> mVideoEncoderOMX;
    sp<MediaSource> mAudioEncoderOMX;
    sp<MediaSource> mVideoSourceNode;
    std::shared_ptr<AppOpsManager> mAppOpsManager;
    bool mRecPaused;

protected:
    virtual ~ExtendedSFRecorder();

private:
    bool isAudioDisabled();
    void setEncoderProfile();
    status_t setupWAVERecording();
    status_t setupExtendedRecording();
    ExtendedSFRecorder(const ExtendedSFRecorder &);
    ExtendedSFRecorder &operator=(ExtendedSFRecorder &);
    AVConfigHelper *pConfigsIns;
    AttributionSourceState mAttributionSource;
};

} //namespace android

#endif // _EXTENDED_SF_RECORDER_H_

