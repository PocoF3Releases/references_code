/*
 * Copyright (c) 2015-2019 Qualcomm Technologies, Inc.
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

#ifndef _EXTENDED_NUPLAYER_H_
#define _EXTENDED_NUPLAYER_H_

#include <common/AVConfigHelper.h>
#include <media/AudioSystem.h>

namespace android {

struct ExtendedNuPlayer : public NuPlayer {
    ExtendedNuPlayer(pid_t pid, const sp<MediaClock> &mediaClock);

    virtual void setDataSourceAsync(
            const sp<IMediaHTTPService> &httpService,
            const char *url,
            const KeyedVector<String8, String8> *headers);

    virtual status_t instantiateDecoder(
        bool audio, sp<DecoderBase> *decoder, bool checkAudioModeChange = true);
    virtual void onResume();
    void performSeek(int64_t seekTimeUs, MediaPlayerSeekMode mode);

protected:
    enum {
        kWhatRestart = 'rstr',
    };
    virtual ~ExtendedNuPlayer();
    virtual void onMessageReceived(const sp<AMessage> &msg);

private:
    ExtendedNuPlayer(const ExtendedNuPlayer &);
    ExtendedNuPlayer &operator=(ExtendedNuPlayer &);

private:
    bool mOffloadDecodedPCM;
    AVConfigHelper *pConfigsIns;

    void initDeviceCallback();
    status_t updateDeviceInformation(bool needConfig = true);
    status_t getOutputDevice(audio_devices_t *, audio_channel_mask_t *);
    bool isBinauralSupported(audio_devices_t device) const {
        return ((device & (AUDIO_DEVICE_OUT_WIRED_HEADSET |
                           AUDIO_DEVICE_OUT_WIRED_HEADPHONE |
                           AUDIO_DEVICE_OUT_USB_HEADSET |
                           AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES)) != 0);
    }

    class AudioPortUpdate: public AudioSystem::AudioPortCallback {
    public:
        AudioPortUpdate(wp<ExtendedNuPlayer> p) : parent(p){}
        ~AudioPortUpdate() {}
        virtual void onAudioPatchListUpdate() {};
        virtual void onAudioPortListUpdate();
        virtual void onServiceDied() {};
    private:
        wp<ExtendedNuPlayer> parent;

    };
    sp<AudioPortUpdate> mPortCallback;
    enum {
        BINAURAL_UNINITIALIZED = -1,
        BINAURAL_DISABLE,
        BINAURAL_ENABLE,
    } mCachedBinaural;
    audio_channel_mask_t mCachedChannelMask;
    bool mReconfigPending;
};

} // namespace android

#endif // _EXTENDED_NUPLAYER_H_

