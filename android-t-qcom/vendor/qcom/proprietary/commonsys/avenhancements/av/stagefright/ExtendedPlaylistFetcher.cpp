/*
 * Copyright (c) 2015-2017, Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Not a Contribution.
 * Apache license notifications and license are retained
 * for attribution purposes only.
 */
/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "ExtendedPlaylistFetcher"

#include <common/AVLog.h>
#include <media/stagefright/foundation/ADebug.h>
#include <cutils/properties.h>

#include "M3UParser.h"
#include "ExtendedPlaylistFetcher.h"
#include "ExtendedLiveSession.h"

namespace android {

ExtendedPlaylistFetcher::ExtendedPlaylistFetcher(
        const sp<AMessage> &notify,
        const sp<LiveSession> &session,
        const char *uri,
        int32_t id,
        int32_t subtitleGeneration,
        bool downloadFirstTs)
    : PlaylistFetcher(notify, session, uri, id, subtitleGeneration),
      mIsFirstTsDownload(downloadFirstTs) {
    AVLOGV("Extended Playlist Fetcher created - %d", mIsFirstTsDownload);
}

ExtendedPlaylistFetcher::~ExtendedPlaylistFetcher() {
    AVLOGV("~ExtendedPlaylistFetcher");
}

bool ExtendedPlaylistFetcher::checkSwitchBandwidth() {
    if (mIsFirstTsDownload) {
        mIsFirstTsDownload = false;
        if (static_cast<ExtendedLiveSession *>(mSession.get())->switchToRealBandwidth()) {
            AVLOGV("change bandwidth at the start of first playback, don't download ts anymore");
            return true;
        }
    }
    return false;
}


} // namespace android
