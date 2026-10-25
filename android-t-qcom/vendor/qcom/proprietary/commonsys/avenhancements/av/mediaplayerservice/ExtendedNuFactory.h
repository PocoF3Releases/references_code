/*
 * Copyright (c) 2015-2018, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef _EXTENDED_NU_FACTORY_H_
#define _EXTENDED_NU_FACTORY_H_

namespace android {

struct ExtendedNuFactory : public AVNuFactory {
    ExtendedNuFactory();

    virtual sp<NuPlayer> createNuPlayer(pid_t pid, const sp<MediaClock> &mediaClock);

    virtual sp<NuPlayer::DecoderBase> createPassThruDecoder(
            const sp<AMessage> &notify,
            const sp<NuPlayer::Source> &source,
            const sp<NuPlayer::Renderer> &renderer);

    virtual sp<NuPlayer::DecoderBase> createDecoder(
            const sp<AMessage> &notify,
            const sp<NuPlayer::Source> &source,
            pid_t pid,
            uid_t uid,
            const sp<NuPlayer::Renderer> &renderer);

    virtual sp<NuPlayer::Renderer> createRenderer(
            const sp<MediaPlayerBase::AudioSink> &sink,
            const sp<MediaClock> &mediaClock,
            const sp<AMessage> &notify,
            uint32_t flags);

protected:
    virtual ~ExtendedNuFactory();

private:
    ExtendedNuFactory(const ExtendedNuFactory &);
    ExtendedNuFactory &operator=(ExtendedNuFactory &);
};

extern "C" AVNuFactory *createExtendedNuFactory();

} //namespace android

#endif // _EXTENDED_NU_FACTORY_H_

