/*
 * Copyright (c) 2015-2016,2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "ExtendedMediaUtils"
#include <common/AVLog.h>
#include <cutils/properties.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MetaData.h>
#include <media/AudioTrack.h>
#include <private/media/AudioTrackShared.h>
#include <media/AVMediaExtensions.h>
#include "media/ExtendedMediaUtils.h"

#define DEFAULT_OFFLOAD_SIZE   (32 * 1024)

namespace android {

size_t ExtendedMediaUtils::AudioTrackGetOffloadFrameCount(size_t frameCount) {
    size_t offloadFrameCount;
    //If offload buffer size is multiple of
    // DEFAULT_OFFLOAD_SIZE 32K, 1 buffer is sufficient
    // to start playback for avoiding start-up latency.
    if((frameCount / DEFAULT_OFFLOAD_SIZE) > 1)
        offloadFrameCount = frameCount;
    else
        offloadFrameCount = frameCount * 2;
    AVLOGV("Offload: new frameCount = %zu", offloadFrameCount);
    return offloadFrameCount;
}

bool ExtendedMediaUtils::AudioTrackIsTrackOffloaded(audio_io_handle_t output) {

    bool isTrackOffloaded = false;
    String8 isDirect = AudioSystem::getParameters(output, String8("is_direct_pcm_track"));
    if (!strcasecmp("is_direct_pcm_track=true", isDirect.string())) {
        isTrackOffloaded = true;
        AVLOGV("is track offloaded track");
    } else {
        AVLOGV("not a track offloaded track");
    }

    return isTrackOffloaded;
}
// ----- NO TRESSPASSING BEYOND THIS LINE ------
AVMediaUtils *createExtendedMediaUtils() {
    return new ExtendedMediaUtils;
}

ExtendedMediaUtils::ExtendedMediaUtils() {
    AVLOGV("ExtendedMediaUtils()");
}

ExtendedMediaUtils::~ExtendedMediaUtils() {
    AVLOGV("~ExtendedMediaUtils()");
}

} //namespace android

