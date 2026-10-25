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

#ifndef _EXTENDED_PLAYLIST_FETCHER_H_
#define _EXTENDED_PLAYLIST_FETCHER_H_

#include "httplive/PlaylistFetcher.h"

namespace android {

struct ExtendedPlaylistFetcher : public PlaylistFetcher {
    ExtendedPlaylistFetcher(
            const sp<AMessage> &notify,
            const sp<LiveSession> &session,
            const char *uri,
            int32_t id,
            int32_t subtitleGeneration,
            bool downloadFirstTs);

protected:
    virtual ~ExtendedPlaylistFetcher();
    virtual bool checkSwitchBandwidth();

private:
    bool mIsFirstTsDownload;
    DISALLOW_EVIL_CONSTRUCTORS(ExtendedPlaylistFetcher);
};

} // namespace android

#endif // _EXTENDED_PLAYLIST_FETCHER_H_
