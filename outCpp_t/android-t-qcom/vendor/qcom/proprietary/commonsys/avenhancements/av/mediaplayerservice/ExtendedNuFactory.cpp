/*
 * Copyright (c) 2015-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "ExtendedNuFactory"
#include <common/AVLog.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MetaData.h>

#include <nuplayer/include/nuplayer/NuPlayer.h>
#include <nuplayer/include/nuplayer/NuPlayerDecoderBase.h>
#include <nuplayer/include/nuplayer/NuPlayerDecoderPassThrough.h>
#include <nuplayer/include/nuplayer/NuPlayerDecoder.h>
#include <nuplayer/include/nuplayer/NuPlayerCCDecoder.h>
#include <gui/Surface.h>
#include <nuplayer/include/nuplayer/NuPlayerSource.h>
#include <nuplayer/include/nuplayer/NuPlayerRenderer.h>

#include <cutils/properties.h>

#include "mediaplayerservice/AVNuExtensions.h"
#include "mediaplayerservice/ExtendedNuFactory.h"
#include "mediaplayerservice/ExtendedNuPlayer.h"
#include "mediaplayerservice/ExtendedNuPlayerRenderer.h"
#include "mediaplayerservice/ExtendedNuPlayerDecoderPassThrough.h"
#include "mediaplayerservice/ExtendedNuPlayerDecoder.h"

namespace android {

AVNuFactory *createExtendedNuFactory() {
    return new ExtendedNuFactory;
}

sp<NuPlayer> ExtendedNuFactory::createNuPlayer(pid_t pid, const sp<MediaClock> &mediaClock) {
    return new ExtendedNuPlayer(pid, mediaClock);
}


sp<NuPlayer::DecoderBase> ExtendedNuFactory::createDecoder(
        const sp<AMessage> &notify,
        const sp<NuPlayer::Source> &source,
        pid_t pid,
        uid_t uid,
        const sp<NuPlayer::Renderer> &renderer) {
    return new ExtendedNuPlayerDecoder(notify, source, pid, uid, renderer);
}


sp<NuPlayer::DecoderBase> ExtendedNuFactory::createPassThruDecoder(
        const sp<AMessage> &notify,
        const sp<NuPlayer::Source> &source,
        const sp<NuPlayer::Renderer> &renderer) {
    return new ExtendedNuPlayerDecoderPassThrough(notify, source, renderer);
}

sp<NuPlayer::Renderer> ExtendedNuFactory::createRenderer(
        const sp<MediaPlayerBase::AudioSink> &sink,
        const sp<MediaClock> &mediaClock,
        const sp<AMessage> &notify,
        uint32_t flags) {
    return new ExtendedNuPlayerRenderer(sink, mediaClock, notify, flags);
}

ExtendedNuFactory::ExtendedNuFactory() {
    AVLOGV("ExtendedNuFactory()");
}

ExtendedNuFactory::~ExtendedNuFactory() {
    AVLOGV("~ExtendedNuFactory()");
}

}




