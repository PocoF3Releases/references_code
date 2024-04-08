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

#include <gui/MiSurfaceComposerStub.h>
#include <dlfcn.h>
#include <iostream>
#include <log/log.h>
#include <private/gui/ParcelUtils.h>
namespace android {

void* MiSurfaceComposerStub::LibMiSurfaceComposerImpl = nullptr;
IMiSurfaceComposerStub* MiSurfaceComposerStub::ImplInstance = nullptr;
std::mutex MiSurfaceComposerStub::StubLock;

IMiSurfaceComposerStub* MiSurfaceComposerStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!LibMiSurfaceComposerImpl) {
        LibMiSurfaceComposerImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibMiSurfaceComposerImpl) {
            CreateMiSurfaceComposerStub* create = (CreateMiSurfaceComposerStub *)dlsym(LibMiSurfaceComposerImpl, "createMiSurfaceComposerImpl");
            ImplInstance = create();
        }
    }
    return ImplInstance;
}

void MiSurfaceComposerStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibMiSurfaceComposerImpl) {
        DestroyMiSurfaceComposerStub* destroy = (DestroyMiSurfaceComposerStub *)dlsym(LibMiSurfaceComposerImpl, "destroyMiSurfaceComposerImpl");
        destroy(ImplInstance);
        dlclose(LibMiSurfaceComposerImpl);
        LibMiSurfaceComposerImpl = nullptr;
        ImplInstance = nullptr;
    }
}
void MiSurfaceComposerStub::setMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays,
                    uint32_t flags, int64_t desiredPresentTime,const sp<IBinder>& remote,
                                     const String16& interfaceDescriptor,uint32_t code) {
         IMiSurfaceComposerStub* Istub = GetImplInstance();
         if (Istub) {
             Istub->setMiuiTransactionState(miuiDisplays, flags, desiredPresentTime,
                                remote, interfaceDescriptor, code);
         }
}

status_t MiSurfaceComposerStub::setFrameRateVideoToDisplay(uint32_t cmd, Parcel *data,
                    const String16& interfaceDescriptor, const sp<IBinder>& remote, uint32_t code) {
    status_t ret = NO_ERROR;
    IMiSurfaceComposerStub* Istub = GetImplInstance();
    if (Istub) {
        ret = Istub->setFrameRateVideoToDisplay(cmd, data, interfaceDescriptor, remote, code);
    }
    return ret;
}


//#if MI_SCREEN_PROJECTION
// MIUI ADD: START
MiuiDisplayState::MiuiDisplayState() :
    what(0),
    isScreenProjection(0),

    isCastProjection(0),
    isLastFrame(0) {}

status_t MiuiDisplayState::write(Parcel& output) const {
    SAFE_PARCEL(output.writeStrongBinder, token);
    SAFE_PARCEL(output.writeStrongBinder, IInterface::asBinder(surface));
    SAFE_PARCEL(output.writeUint32, what);
    SAFE_PARCEL(output.writeUint32, isScreenProjection);
    SAFE_PARCEL(output.writeUint32, isCastProjection);
    SAFE_PARCEL(output.writeUint32, isLastFrame);
    return NO_ERROR;
}

status_t MiuiDisplayState::read(const Parcel& input) {
    SAFE_PARCEL(input.readStrongBinder, &token);
    surface = interface_cast<IGraphicBufferProducer>(input.readStrongBinder());
    SAFE_PARCEL(input.readUint32, &what);
    SAFE_PARCEL(input.readUint32, &isScreenProjection);
    SAFE_PARCEL(input.readUint32, &isCastProjection);
    SAFE_PARCEL(input.readUint32, &isLastFrame);
    return NO_ERROR;
}
// END
//#endif
} // namespace android
