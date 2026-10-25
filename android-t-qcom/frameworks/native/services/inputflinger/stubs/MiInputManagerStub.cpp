#include "MiInputManagerStub.h"
#include <dlfcn.h>
#include <iostream>
#include <log/log.h>

namespace android {
void* MiInputManagerStub::LibMiInputManagerImpl = nullptr;
IMiInputManagerStub* MiInputManagerStub::ImplInstance = nullptr;
std::mutex MiInputManagerStub::StubLock;
bool MiInputManagerStub::inited = false;

IMiInputManagerStub* MiInputManagerStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!LibMiInputManagerImpl && !inited) {
        LibMiInputManagerImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibMiInputManagerImpl) {
            CreateMiInputManagerStub* create = (CreateMiInputManagerStub *)dlsym(LibMiInputManagerImpl, "createInputManagerStubImpl");
            if (create) {
                ImplInstance = create();
            } else {
                ALOGE("dlsym is fail MiInputManagerStub");
            }
        } else {
            ALOGE("inputflinger open %s failed! dlopen - %s", LIBPATH, dlerror());
        }
        inited = true;
    }
    return ImplInstance;
}

void MiInputManagerStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibMiInputManagerImpl) {
        DestroyMiInputManagerStub* destroy = (DestroyMiInputManagerStub *)dlsym(LibMiInputManagerImpl, "destroyInputManagerStubImpl");
        destroy(ImplInstance);
        dlclose(LibMiInputManagerImpl);
        LibMiInputManagerImpl = nullptr;
        ImplInstance = nullptr;
    }
}

void MiInputManagerStub::init(InputManager* inputManager) {
    IMiInputManagerStub* IStub = GetImplInstance();
    if (IStub) {
        IStub->init(inputManager);
    }
}

InputListenerInterface* MiInputManagerStub::createMiInputMapper(InputListenerInterface& listener) {
    IMiInputManagerStub* IStub = GetImplInstance();
    if (IStub) {
        return IStub->createMiInputMapper(listener);
    }
    return &listener;
}

status_t MiInputManagerStub::start() {
    IMiInputManagerStub* IStub = GetImplInstance();
    if (IStub) {
        return IStub->start();
    }
    return 0;
}

status_t MiInputManagerStub::stop() {
    IMiInputManagerStub* IStub = GetImplInstance();
    if (IStub) {
        return IStub->stop();
    }
    return 0;
}

}