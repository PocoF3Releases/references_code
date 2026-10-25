/*
 * Copyright (c) 2015-2022 Qualcomm Technologies, Inc.
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

//#define LOG_NDEBUG 0
#define LOG_TAG "ExtendedNuPlayer"
#include <common/AVLog.h>
#include <cutils/properties.h>

#include <nuplayer/include/nuplayer/NuPlayer.h>
#include <nuplayer/include/nuplayer/NuPlayerDecoderBase.h>
#include <nuplayer/include/nuplayer/NuPlayerDecoderPassThrough.h>
#include <nuplayer/include/nuplayer/NuPlayerSource.h>
#include <nuplayer/include/nuplayer/NuPlayerRenderer.h>
#include <nuplayer/include/nuplayer/GenericSource.h>
#include <nuplayer/include/nuplayer/RTSPSource.h>
#include <nuplayer/include/nuplayer/HTTPLiveSource.h>

#include "stagefright/AVExtensions.h"
#include <media/AudioParameter.h>
#include <media/stagefright/Utils.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/foundation/ABuffer.h>

#ifdef HLS_AUDIO_ONLY_IMG_DISPLAY
#include <gui/IGraphicBufferProducer.h>
#include <gui/Surface.h>
#include <ui/GraphicBufferMapper.h>
#include <system/window.h>
#endif

#include "mediaplayerservice/AVNuExtensions.h"
#include "mediaplayerservice/ExtendedNuPlayer.h"
#include "mediaplayerservice/ExtendedHTTPLiveSource.h"
#include "mediaplayerservice/ExtendedNuUtils.h"

#include "stagefright/ExtendedUtils.h"

namespace android {

ExtendedNuPlayer::ExtendedNuPlayer(pid_t pid, const sp<MediaClock> &mediaClock)
        : NuPlayer(pid, mediaClock),
          mOffloadDecodedPCM(false),
          mPortCallback(NULL),
          mCachedBinaural(BINAURAL_UNINITIALIZED),
          mCachedChannelMask(AUDIO_CHANNEL_NONE),
          mReconfigPending(false) {
    updateLogLevel();
    pConfigsIns = AVConfigHelper::getInstance();
    AVLOGV("ExtendedNuPlayer()");
}

ExtendedNuPlayer::~ExtendedNuPlayer() {
    AVLOGV("~ExtendedNuPlayer()");

    if (mPortCallback != NULL) {
        status_t status = AudioSystem::removeAudioPortCallback(mPortCallback);
        if (status != NO_ERROR) {
            AVLOGE("removeAudioPortCallback returned error %d", status);
        }
        mPortCallback.clear();
    }
}

void ExtendedNuPlayer::setDataSourceAsync(
            const sp<IMediaHTTPService> &httpService,
            const char *url,
            const KeyedVector<String8, String8> *headers) {
    AVLOGV("setDataSource url %s", url);

    sp<AMessage> msg = new AMessage(kWhatSetDataSource, this);
    size_t len = strlen(url);

    sp<AMessage> notify = new AMessage(kWhatSourceNotify, this);

    sp<Source> source;
    if (NuPlayer::IsHTTPLiveURL(url)) {
        char value[PROPERTY_VALUE_MAX];
        property_get("persist.vendor.media.hls.custom", value, "0");
        if(atoi(value)) {
            AVLOGI("new ExtendedHTTPLiveSource");
            source = new ExtendedHTTPLiveSource(notify, httpService, url, headers);
        } else {
            source = new HTTPLiveSource(notify, httpService, url, headers);
        }
        mDataSourceType = DATA_SOURCE_TYPE_HTTP_LIVE;
    } else if (!strncasecmp(url, "rtsp://", 7)) {
        source = new RTSPSource(
                notify, httpService, url, headers, mUIDValid, mUID);
        mDataSourceType = DATA_SOURCE_TYPE_RTSP;
    } else if ((!strncasecmp(url, "http://", 7)
                || !strncasecmp(url, "https://", 8))
                    && ((len >= 4 && !strcasecmp(".sdp", &url[len - 4]))
                    || strstr(url, ".sdp?"))) {
        source = new RTSPSource(
                notify, httpService, url, headers, mUIDValid, mUID, true);
        mDataSourceType = DATA_SOURCE_TYPE_RTSP;
    } else {
        sp<GenericSource> genericSource;
        genericSource = new GenericSource(notify, mUIDValid, mUID, mMediaClock);

        // Don't set FLAG_SECURE on mSourceFlags here for widevine.
        // The correct flags will be updated in Source::kWhatFlagsChanged
        // handler when  GenericSource is prepared.

        status_t err = genericSource->setDataSource(httpService, url, headers);

        if (err == OK) {
            source = genericSource;
        } else {
            AVLOGE("Failed to set data source!");
        }
        mDataSourceType = DATA_SOURCE_TYPE_GENERIC_URL;
    }
    msg->setObject("source", source);
    msg->post();
}

status_t ExtendedNuPlayer::instantiateDecoder(
    bool audio, sp<DecoderBase> *decoder, bool checkAudioModeChange) {
    bool bIgnore = false;
    if (audio) {
        bIgnore = pConfigsIns->isSFPlayerDisabled();
        AVLOGV("Audio disabled %d", bIgnore);

        AString mime;
        sp<AMessage> format = (mSource != NULL) ? mSource->getFormat(audio) : NULL;

        if ((format != NULL) && (format->findString("mime", &mime))) {
            if (!strcasecmp(mime.c_str(), MEDIA_MIMETYPE_AUDIO_MHAS)) {
                if (mCachedBinaural == BINAURAL_UNINITIALIZED && mCachedChannelMask == 0x0) {
                    status_t status;
                    if((status = updateDeviceInformation(false)) != NO_ERROR) {
                        return status;
                    }
                }

                ((ExtendedNuUtils*)ExtendedNuUtils::get())->
                    setMPEGHOutputFormat(mCachedBinaural, mCachedChannelMask);
                initDeviceCallback();
            }
        } else {
            AVLOGD("format is null or empty mime");
        }
    }

    if (!bIgnore) {
        return NuPlayer::instantiateDecoder(audio, decoder, checkAudioModeChange);
    }
    return OK;
}

void ExtendedNuPlayer::onResume() {

    if (!mPaused) {
        return;
    }

    if (mSource != NULL) {
        sp<AMessage> audioformat = mSource->getFormat(true /*audio*/);
        const bool hasaudio = (audioformat != NULL);
        status_t err;

        //check whether decoder can be allowed from utils
        if (hasaudio && !((audioformat->findInt32("err", &err) && err))) {
            if (!ExtendedUtils::isHwAudioDecoderSessionAllowed(audioformat)) {
                AVLOGD("voice_conc: Failed to resume audio decoder");
                notifyListener(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN, 0);
                return;
            }
        }
    }

    NuPlayer::onResume();
}

void ExtendedNuPlayer::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatRestart:
        {
            AVLOGV("kWhatRestart");
            if (mRenderer != NULL) {
                int64_t positionUs;
                if (mRenderer->getCurrentPosition(&positionUs) != OK) {
                    positionUs = mPreviousSeekTimeUs;
                }

                if (!mPaused) {
                    mRenderer->pause();
                }
                restartAudio(positionUs, false /* forceNonOffload */,
                        true /* needsToCreateAudioDecoder */);
                if (!mPaused) {
                    mRenderer->resume();
                }
            }
            break;
        }
        default:
            NuPlayer::onMessageReceived(msg);
    }
}

status_t ExtendedNuPlayer::updateDeviceInformation(bool needConfig) {
    status_t status = NO_ERROR;
    audio_devices_t device;
    int binaural;
    audio_channel_mask_t channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    bool isBinaural;

    if ((status = getOutputDevice(&device, &channel_mask)) != NO_ERROR) {
        AVLOGE("Fails to get current out device");
        goto exit;
    }

    // Enforce binaural mode if output device supports but binaural
    // is not set. Vice versa.
    isBinaural = isBinauralSupported(device);
    if ((isBinaural && (mCachedBinaural == BINAURAL_DISABLE)) ||
            (!isBinaural && (mCachedBinaural == BINAURAL_ENABLE)) ||
            (mCachedBinaural == BINAURAL_UNINITIALIZED)) {
        mCachedBinaural = isBinaural ? BINAURAL_ENABLE : BINAURAL_DISABLE;
        AVLOGV("pending binaural-mode to set %d", mCachedBinaural);
        if (needConfig) {
            mReconfigPending = true;
        }
    }

    // Set channel mask.
    if (channel_mask != mCachedChannelMask) {
        mCachedChannelMask = channel_mask;
        AVLOGV("pending channel-mask to set %d", mCachedChannelMask);
        if (needConfig) {
            mReconfigPending = true;
        }
    }

    if (mReconfigPending) {
        mReconfigPending = false;
        (new AMessage(kWhatRestart, this))->post();
    }

exit:
    return status;
}

void ExtendedNuPlayer::initDeviceCallback() {
    status_t status = NO_ERROR;

    // register port callback upon creating MHAS decoder
    // keep its life cycle along with nuplayer
    if (mPortCallback == NULL) {
        mPortCallback = new AudioPortUpdate(this);
        status = AudioSystem::addAudioPortCallback(mPortCallback);
        if (status != NO_ERROR) {
            AVLOGE("addAudioPortCallback returned error %d", status);
        }
    }
}

status_t ExtendedNuPlayer::getOutputDevice(audio_devices_t *device, audio_channel_mask_t *channel_mask) {
    status_t status = NO_ERROR;
    String8 reply;
    int cmask;

    AudioDeviceTypeAddrVector devices;
    status = AudioSystem::getDevicesForAttributes(attributes_initializer(AUDIO_USAGE_MEDIA),
                                                    &devices, /*forVolume=*/false);
    if (status != OK || devices.empty()) {
        return NAME_NOT_FOUND;
    }
    *device = devices[0].mType;

    if (*device == AUDIO_DEVICE_OUT_AUX_DIGITAL) {
        reply = AudioSystem::getParameters(String8("dp_channel_mask"));
        AudioParameter result = AudioParameter(reply);
        if (result.getInt(String8("dp_channel_mask"), cmask) == NO_ERROR) {
            *channel_mask = (audio_channel_mask_t)cmask;
        }
    }
    AVLOGV("device %d, channel mask %d", *device, *channel_mask);

    return status;
}

void ExtendedNuPlayer::AudioPortUpdate::onAudioPortListUpdate() {
    sp<ExtendedNuPlayer> p = parent.promote();
    if (p != NULL) {
        p->updateDeviceInformation();
    }
}
}
