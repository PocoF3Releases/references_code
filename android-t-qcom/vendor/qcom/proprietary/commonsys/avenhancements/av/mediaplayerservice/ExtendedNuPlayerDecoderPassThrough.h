/*
 * Copyright (c) 2015-2016, Qualcomm Technologies, Inc.
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
#ifndef _EXTENDED_NUPLAYER_DECODER_PASS_THRU_H_
#define _EXTENDED_NUPLAYER_DECODER_PASS_THRU_H_

namespace android {

struct ExtendedNuPlayerDecoderPassThrough : public NuPlayer::DecoderPassThrough {

    ExtendedNuPlayerDecoderPassThrough(
            const sp<AMessage> &notify,
            const sp<NuPlayer::Source> &source,
            const sp<NuPlayer::Renderer> &renderer);

    virtual void onConfigure(const sp<AMessage> &format);

protected:
    virtual ~ExtendedNuPlayerDecoderPassThrough();

    virtual void onResume(bool notifyComplete);
    virtual void onFlush();

private:
    audio_format_t mAudioFormat;

    bool mVorbisHdrRequired;
    bool mVorbisHdrCommitted;
    sp<ABuffer> mVorbisHdrBuffer;
    sp<ABuffer> mAnchorBuffer;

    sp<ABuffer> aggregateBuffer(const sp<ABuffer> &accessUnit);
    sp<ABuffer> assembleVorbisHdr(const sp<MetaData> &meta);

    DISALLOW_EVIL_CONSTRUCTORS(ExtendedNuPlayerDecoderPassThrough);
};

} //namespace android

#endif // _EXTENDED_NUPLAYER_DECODER_PASS_THRU_H_

