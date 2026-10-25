/******************************************************************************
 *  This program is protected under international and U.S. copyright laws as
 *  an unpublished work. This program is confidential and proprietary to the
 *  copyright owners. Reproduction or disclosure, in whole or in part, or the
 *  production of derivative works therefrom without the express permission of
 *  the copyright owners is prohibited.
 *
 *                 Copyright (C) 2019 by Dolby Laboratories.
 *                             All rights reserved.
 ******************************************************************************/

#define LOG_TAG "DlbVqeCb"

#include <stdint.h>
#include <system/audio.h>
#include <log/log.h>
#include <vendor/dolby/hardware/dms/2.0/IDms.h>
#include "DlbVqeCallback.h"

#define AUDIO_TRACK_TYPE_TRACK 0
#define AUDIO_TRACK_TYPE_RECORD 1

#define AUDIO_TRACK_STATE_INACTIVE 0
#define AUDIO_TRACK_STATE_ACTIVE 1

namespace android {

using ::vendor::dolby::hardware::dms::V2_0::IDms;
static sp<IDms> mDms;

void notifyAudioTrackChange(uint32_t trackType, uint32_t trackState, uint32_t sessionId, uint32_t uid,
                            uint32_t sampleRate, uint32_t format, uint32_t channelMask) {
    if (mDms == nullptr) {
        mDms = IDms::tryGetService();
        if (mDms == nullptr) {
            ALOGE("%s Cannot get Dolby manager service.", __FUNCTION__);
        }
    }
    if (mDms != nullptr) {
        mDms->notifyAudioTrackChange(trackType, trackState, sessionId, uid, sampleRate, format, channelMask);
    }
}

void onAddActiveTrack(bool record, bool fast, uint32_t sessionId, uint32_t uid, uint32_t sampleRate,
        uint32_t format, uint32_t channelMask) {
    if (fast) {
        return;
    }
    uint32_t type = record ? AUDIO_TRACK_TYPE_RECORD : AUDIO_TRACK_TYPE_TRACK;
    notifyAudioTrackChange(type, AUDIO_TRACK_STATE_ACTIVE, sessionId, uid, sampleRate, format, channelMask);
}

void onRemoveActiveTrack(bool record, bool fast, uint32_t sessionId, uint32_t uid, uint32_t sampleRate,
        uint32_t format, uint32_t channelMask) {
    if (fast) {
        return;
    }
    uint32_t type = record ? AUDIO_TRACK_TYPE_RECORD : AUDIO_TRACK_TYPE_TRACK;
    notifyAudioTrackChange(type, AUDIO_TRACK_STATE_INACTIVE, sessionId, uid, sampleRate, format, channelMask);
}
} //namespace android

