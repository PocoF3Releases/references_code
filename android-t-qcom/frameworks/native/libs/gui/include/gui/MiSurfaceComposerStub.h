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
#pragma once
#include <gui/IMiSurfaceComposerStub.h>
#include <mutex>
#include <gui/LayerState.h>
namespace android {
//question: can't remove
class IMiSurfaceComposerStub;
//#if MI_SCREEN_PROJECTION
// MIUI ADD: START
static inline int compare_type(const MiuiDisplayState& lhs, const MiuiDisplayState& rhs) {
    return compare_type(lhs.token, rhs.token);
}
// END
//#endif
class MiSurfaceComposerStub {

private:

    MiSurfaceComposerStub() {}
    static void *LibMiSurfaceComposerImpl;
    static IMiSurfaceComposerStub *ImplInstance;
    static IMiSurfaceComposerStub *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;

    static constexpr const char* LIBPATH = "libmigui.so";

public:
    virtual ~MiSurfaceComposerStub() {}
    static void setMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays,
                    uint32_t flags, int64_t desiredPresentTime,const sp<IBinder>& remote,
                            const String16& interfaceDescriptor,uint32_t code);
//    static void skipSlotRef(sp<Surface> surface, int i);
    static status_t setFrameRateVideoToDisplay(uint32_t cmd, Parcel *data,
                            const String16& interfaceDescriptor, const sp<IBinder>& remote, uint32_t code);
};


} // namespace android
