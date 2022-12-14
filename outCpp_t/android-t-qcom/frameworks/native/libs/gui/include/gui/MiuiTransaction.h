/*
 * Copyright (C) 2007 The Android Open Source Project
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
#if MI_SCREEN_PROJECTION

#pragma once
#include <mutex>
#include <gui/SurfaceComposerClient.h>
namespace android {
//question: can't remove
class SurfaceComposerClient;

    // MIUI ADD: START
    class MiuiTransaction : public SurfaceComposerClient::Transaction {

        SortedVector<MiuiDisplayState > mMiuiDisplayStates;
        uint32_t                    mForceSynchronous = 0;
        bool                        mAnimation = false;
        bool                        mEarlyWakeup = false;

        int64_t mDesiredPresentTime = -1;

        int mStatus = NO_ERROR;

        MiuiDisplayState& getMiuiDisplayState(const sp<IBinder>& token);

    public:
        MiuiTransaction() = default;
        ~MiuiTransaction() = default;

        status_t apply(bool synchronous = false);

        void setDiffScreenProjection(const sp<IBinder>& token, uint32_t isScreenProjection);
        void nativeSetCastMode(const sp<IBinder>& token, bool enable);
        void nativeSetLastFrame(const sp<IBinder>& token, bool enable);
    };
    // END
} // namespace android
#endif