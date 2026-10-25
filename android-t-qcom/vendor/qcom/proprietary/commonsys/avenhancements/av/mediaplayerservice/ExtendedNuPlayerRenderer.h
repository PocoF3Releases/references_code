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
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _EXTENDED_NUPLAYER_RENDERER_H_
#define _EXTENDED_NUPLAYER_RENDERER_H_

namespace android {

struct ExtendedNuPlayerRenderer : public NuPlayer::Renderer {

    ExtendedNuPlayerRenderer(
            const sp<MediaPlayerBase::AudioSink> &sink,
            const sp<MediaClock> &mediaClock,
            const sp<AMessage> &notify,
            uint32_t flags);

protected:
    virtual ~ExtendedNuPlayerRenderer();
    virtual status_t onOpenAudioSink(
                const sp<AMessage> &format,
                bool offloadOnly,
                bool hasVideo,
                uint32_t flags,
                bool isStreaming) override;

private:
    ExtendedNuPlayerRenderer(const ExtendedNuPlayerRenderer &);
    ExtendedNuPlayerRenderer &operator=(ExtendedNuPlayerRenderer &);
    enum {
        kWhatGetAudioStreamType  = 'gAST',
    };
};

} //namespace android

#endif // _EXTENDED_NUPLAYER_RENDERER_H_

