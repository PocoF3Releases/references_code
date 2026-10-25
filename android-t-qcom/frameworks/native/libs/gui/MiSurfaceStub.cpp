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

#include "MiSurfaceStub.h"
#include <dlfcn.h>
#include <iostream>
#include <log/log.h>

namespace android {

void* MiSurfaceStub::LibMiSurfaceImpl = nullptr;
IMiSurfaceStub* MiSurfaceStub::ImplInstance = nullptr;
std::mutex MiSurfaceStub::StubLock;

IMiSurfaceStub* MiSurfaceStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!LibMiSurfaceImpl) {
        LibMiSurfaceImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibMiSurfaceImpl) {
            Create* create = (Create *)dlsym(LibMiSurfaceImpl, "create");
            ImplInstance = create();
        } else {
            static bool onlyOnceOutput = true;
            if (onlyOnceOutput) {
                ALOGE("open %s failed! dlopen - %s", LIBPATH, dlerror());
                onlyOnceOutput = false;
            }
        }
    }
    return ImplInstance;
}

void MiSurfaceStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibMiSurfaceImpl) {
        Destroy* destroy = (Destroy *)dlsym(LibMiSurfaceImpl, "destroy");
        destroy(ImplInstance);
        dlclose(LibMiSurfaceImpl);
        LibMiSurfaceImpl = nullptr;
        ImplInstance = nullptr;
    }
}

void MiSurfaceStub::skipSlotRef(sp<Surface> surface, int i) {
    IMiSurfaceStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->skipSlotRef(surface, i);
    }
}

void MiSurfaceStub::doDynamicFps() {
    IMiSurfaceStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->doDynamicFps();
    }
}

int MiSurfaceStub::doNotifyFbc(const int32_t& value) {
    int ret = 0;
    IMiSurfaceStub* Istub = GetImplInstance();
    if (Istub) {
        ret = Istub->doNotifyFbc(value);
    }
    return ret;
}

int MiSurfaceStub::DoNotifyFbc(const int32_t& value, const uint64_t& bufID, const int32_t& param, const uint64_t& id) {
    int ret = 0;
    IMiSurfaceStub* Istub = GetImplInstance();
    if (Istub) {
        ret = Istub->DoNotifyFbc(value,bufID,param,id);
    }
    return ret;
}

bool MiSurfaceStub::supportFEAS() {
    bool ret = false;
    IMiSurfaceStub* Istub = GetImplInstance();
    if (Istub) {
        ret = Istub->supportFEAS();
    }
    return ret;
}

} // namespace android
