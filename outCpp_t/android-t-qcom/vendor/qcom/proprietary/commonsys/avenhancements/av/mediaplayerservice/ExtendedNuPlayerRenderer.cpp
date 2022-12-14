/*
 * Copyright (c) 2015-2019, Qualcomm Technologies, Inc.
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
#define LOG_TAG "ExtendedNuPlayerRenderer"
#include <common/AVLog.h>

#include <nuplayer/include/nuplayer/NuPlayer.h>
#include <nuplayer/include/nuplayer/NuPlayerDecoderBase.h>
#include <nuplayer/include/nuplayer/NuPlayerDecoderPassThrough.h>
#include <nuplayer/include/nuplayer/NuPlayerSource.h>
#include <nuplayer/include/nuplayer/NuPlayerRenderer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>

#include "mediaplayerservice/AVNuExtensions.h"
#include "mediaplayerservice/ExtendedNuPlayerRenderer.h"
#include "mediaplayerservice/ExtendedNuUtils.h"
#include <media/stagefright/VideoFrameScheduler.h>
#include <media/MediaCodecBuffer.h>



namespace android {

ExtendedNuPlayerRenderer::ExtendedNuPlayerRenderer(
        const sp<MediaPlayerBase::AudioSink> &sink,
        const sp<MediaClock> &mediaClock,
        const sp<AMessage> &notify,
        uint32_t flags)
    : NuPlayer::Renderer(sink, mediaClock, notify, flags) {
    AVLOGV("ExtendedNuPlayerRenderer()");
}

ExtendedNuPlayerRenderer::~ExtendedNuPlayerRenderer() {
    AVLOGV("~ExtendedNuPlayerRenderer()");
}

status_t ExtendedNuPlayerRenderer::onOpenAudioSink(
        const sp<AMessage> &format,
        bool offloadOnly,
        bool hasVideo,
        uint32_t flags,
        bool isStreaming) {
    AVLOGV("onOpenAudioSinki:offloadOnly(%d) offloadingAudio(%d) flags(0x%x) isStreaming(%d)",
            offloadOnly, offloadingAudio(), flags, isStreaming);
    status_t status = NuPlayer::Renderer::onOpenAudioSink(format,
                                              offloadOnly, hasVideo, flags, isStreaming);
    // update mNumFramesWritten for direct pcm using mAudioSink->getPosition()
    // as mAudioSink->getFramesWritten() is not supported for offload or direct tracks.
    if (status == OK && !offloadOnly &&
        !offloadingAudio() && flags == AUDIO_OUTPUT_FLAG_DIRECT) {
        uint32_t written;
        if (mAudioSink->getPosition(&written) == OK) {
            mNumFramesWritten = written;
        }
    }
    return status;
}

}



