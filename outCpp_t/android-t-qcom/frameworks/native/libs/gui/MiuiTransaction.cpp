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
#include <mutex>
#include <private/gui/ParcelUtils.h>
#include <gui/MiuiTransaction.h>
#include <private/gui/ComposerService.h>
namespace android {
status_t MiuiTransaction::apply(bool synchronous) {
    if (mStatus != NO_ERROR) {
        return mStatus;
    }
    sp<ISurfaceComposer> sf(ComposerService::getComposerService());
    Vector<MiuiDisplayState> miuiDisplayStates;
    uint32_t flags = 0;
    mForceSynchronous |= synchronous;
    miuiDisplayStates = mMiuiDisplayStates;
    mMiuiDisplayStates.clear();
    if (mForceSynchronous) {
        flags |= ISurfaceComposer::eSynchronous;
    }
    if (mAnimation) {
        flags |= ISurfaceComposer::eAnimation;
    }
    if (mEarlyWakeup) {
        flags |= ISurfaceComposer::eEarlyWakeupStart;
    } else {
        flags |= ISurfaceComposer::eEarlyWakeupEnd;
    }
    mForceSynchronous = false;
    mAnimation = false;
    mEarlyWakeup = false;
    sf->setMiuiTransactionState(miuiDisplayStates, flags, mDesiredPresentTime);
    mStatus = NO_ERROR;
    return NO_ERROR;
}

void MiuiTransaction::setDiffScreenProjection(const sp<IBinder>& token,
        uint32_t isScreenProjection) {
    MiuiDisplayState& s(getMiuiDisplayState(token));
    s.isScreenProjection = isScreenProjection;
    s.what |= MiuiDisplayState::eIsScreenProjectionChanged;
}
void MiuiTransaction::nativeSetCastMode(const sp<IBinder>& token, bool enable) {
    MiuiDisplayState& s(getMiuiDisplayState(token));
    s.isCastProjection = enable ? 1 : 0;
    s.what |= MiuiDisplayState::eIsCastProjectionChanged;
}
void MiuiTransaction::nativeSetLastFrame(const sp<IBinder>& token, bool enable) {
    MiuiDisplayState& s(getMiuiDisplayState(token));
    s.isLastFrame = enable ? 1 : 0;
    s.what |= MiuiDisplayState::eIsLastFrameChanged;
}

MiuiDisplayState& MiuiTransaction::getMiuiDisplayState(const sp<IBinder>& token) {
    MiuiDisplayState s;
    s.token = token;
    ssize_t index = mMiuiDisplayStates.indexOf(s);
    if (index < 0) {
        // we don't have it, add an initialized layer_state to our list
        s.what = 0;
        index = mMiuiDisplayStates.add(s);
    }
    return mMiuiDisplayStates.editItemAt(static_cast<size_t>(index));
}
} // namespace android
#endif