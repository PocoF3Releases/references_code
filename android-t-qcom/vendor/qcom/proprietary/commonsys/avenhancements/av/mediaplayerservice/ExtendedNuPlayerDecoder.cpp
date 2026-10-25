/*
 * Copyright (c) 2015-2018, Qualcomm Technologies, Inc.
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
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ExtendedNuPlayerDecoder"
#include <common/AVLog.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>

#include <nuplayer/include/nuplayer/NuPlayer.h>
#include <nuplayer/include/nuplayer/NuPlayerDecoderBase.h>
#include <nuplayer/include/nuplayer/NuPlayerDecoder.h>
#include <nuplayer/include/nuplayer/NuPlayerCCDecoder.h>
#include <gui/Surface.h>
#include <nuplayer/include/nuplayer/NuPlayerSource.h>
#include <nuplayer/include/nuplayer/NuPlayerRenderer.h>

#include "mediaplayerservice/AVNuExtensions.h"
#include "stagefright/AVExtensions.h"
#include "mediaplayerservice/ExtendedNuPlayerDecoder.h"
#include "stagefright/ExtendedUtils.h"
#include "mediaplayerservice/ExtendedNuUtils.h"

namespace android {

ExtendedNuPlayerDecoder::ExtendedNuPlayerDecoder(
        const sp<AMessage> &notify,
        const sp<NuPlayer::Source> &source,
        pid_t pid,
        uid_t uid,
        const sp<NuPlayer::Renderer> &renderer)
    : NuPlayer::Decoder(notify, source, pid, uid, renderer) {
    AVLOGV("ExtendedNuPlayerDecoder()");
}

ExtendedNuPlayerDecoder::~ExtendedNuPlayerDecoder() {
    AVLOGV("~ExtendedNuPlayerDecoder()");
}

void ExtendedNuPlayerDecoder::handleOutputFormatChange(const sp<AMessage> &format) {
    if (!mIsAudio) {
        int32_t width, height;
        if (format->findInt32("width", &width)
                && format->findInt32("height", &height)) {
            mStats->setInt32("width", width);
            mStats->setInt32("height", height);
        }
        sp<AMessage> notify = mNotify->dup();
        notify->setInt32("what", kWhatVideoSizeChanged);
        notify->setMessage("format", format);
        notify->post();
    } else if (mRenderer != NULL) {
        uint32_t flags;
        int64_t durationUs = -1;
        bool allowDeepBuffer = false;
        status_t err = NO_ERROR;
        bool hasVideo = (mSource->getFormat(false /* audio */) != NULL);
        if ((mSource->getDuration(&durationUs) == OK)
             && (durationUs > AUDIO_SINK_MIN_DEEP_BUFFER_DURATION_US))
            allowDeepBuffer = true;

        if (getAudioDeepBufferSetting() // override regardless of source duration
                || (!hasVideo && allowDeepBuffer)) {
            flags = AUDIO_OUTPUT_FLAG_DEEP_BUFFER;
        } else if (hasVideo && allowDeepBuffer) {
            flags = AUDIO_OUTPUT_FLAG_NONE;
        } else
            flags = AUDIO_OUTPUT_FLAG_PRIMARY;

        sp<AMessage> audioSrcMsg  = mSource->getFormat(true /* audio */);
        //handle format change for audio
        if (audioSrcMsg != NULL) {
            AVLOGV("Format change for Audio decoder");
            //overwrite decoders output format with the source format in case
            //there are missing fields in decoders output format
            sp <AMessage> dupFormat = format->dup();
            AVNuUtils::get()->overWriteAudioOutputFormat(dupFormat, audioSrcMsg);

            AudioPlaybackRate rate = AUDIO_PLAYBACK_RATE_DEFAULT;
            mRenderer->getPlaybackSettings(&rate);

            //Set direct flag only if primary is not set,
            //pcmOffloadException is false and
            //playback rate is default
            //avoffload property is enabled for video
            if ((flags != AUDIO_OUTPUT_FLAG_PRIMARY) &&
                !AVNuUtils::get()->pcmOffloadException(audioSrcMsg) &&
                isAudioPlaybackRateEqual(rate, AUDIO_PLAYBACK_RATE_DEFAULT) &&
                (!hasVideo || (hasVideo && ((ExtendedNuUtils *)ExtendedNuUtils::get())->avOffloadEnabled()))) {
                    flags = AUDIO_OUTPUT_FLAG_DIRECT;
            }

            sp<AMessage> reply = new AMessage(kWhatAudioOutputFormatChanged, this);
            reply->setInt32("generation", mBufferGeneration);
            mRenderer->changeAudioFormat(dupFormat, false /* offloadOnly */,
                    hasVideo, flags, mSource->isStreaming(), reply);
        } //handle audio format change
    } //mRenderer != NULL
}

void ExtendedNuPlayerDecoder::onConfigure(const sp<AMessage> &format) {
    if (format != NULL) {

        AString mime;
        sp<AMessage> audioformat = mSource->getFormat(true /*audio*/);
        const bool hasaudio = (audioformat != NULL);
        status_t err;

        if (hasaudio  && !((audioformat->findInt32("err", &err) && err))) {

            //check whether decoder can be allowed from utils
            if (!ExtendedUtils::isHwAudioDecoderSessionAllowed(format)) {
                AVLOGD("voice_conc:Failed to create %s audio decoder",
                    mime.c_str());
                handleError(UNKNOWN_ERROR);
                return;
            }
       }
    }

    return Decoder::onConfigure(format);
}

void ExtendedNuPlayerDecoder::onSetParameters(const sp<AMessage> &params) {
    int32_t ret;
    if ((params->findInt32("binaural-mode", &ret)) ||
                (params->findInt32("channel-mask", &ret))) {
        if (mCodec != NULL) {
            mCodec->setParameters(params);
        }
    }

    return Decoder::onSetParameters(params);
}

}
