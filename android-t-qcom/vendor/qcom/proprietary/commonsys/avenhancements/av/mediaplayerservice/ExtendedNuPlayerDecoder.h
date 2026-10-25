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
#ifndef _EXTENDED_NUPLAYER_DECODER_H_
#define _EXTENDED_NUPLAYER_DECODER_H_

#include <cutils/properties.h>

namespace android {
static inline bool getAudioDeepBufferSetting() {
    return property_get_bool("media.stagefright.audio.deep", false /* default_value */);
}
struct ExtendedNuPlayerDecoder : public NuPlayer::Decoder {

    ExtendedNuPlayerDecoder(
            const sp<AMessage> &notify,
            const sp<NuPlayer::Source> &source,
            pid_t pid,
            uid_t uid,
            const sp<NuPlayer::Renderer> &renderer);
    virtual void handleOutputFormatChange(const sp<AMessage> &format);
    virtual void onConfigure(const sp<AMessage> &format);
    virtual void onSetParameters(const sp<AMessage> &params);
protected:
    virtual ~ExtendedNuPlayerDecoder();

private:
    ExtendedNuPlayerDecoder(const ExtendedNuPlayerDecoder &);
    ExtendedNuPlayerDecoder &operator=(ExtendedNuPlayerDecoder &);
};

} //namespace android

#endif // _EXTENDED_NUPLAYER_DECODER_

