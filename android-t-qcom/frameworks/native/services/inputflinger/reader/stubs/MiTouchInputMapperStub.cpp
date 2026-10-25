#include "MiTouchInputMapperStub.h"
#include <dlfcn.h>
#include <iostream>
#include <log/log.h>

namespace android {
void* MiTouchInputMapperStub::LibMiTouchInputMapperImpl = nullptr;
IMiTouchInputMapperStub* MiTouchInputMapperStub::ImplInstance = nullptr;
std::mutex MiTouchInputMapperStub::StubLock;
bool MiTouchInputMapperStub::inited = false;

IMiTouchInputMapperStub* MiTouchInputMapperStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!LibMiTouchInputMapperImpl && !inited) {
        LibMiTouchInputMapperImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibMiTouchInputMapperImpl) {
            CreateMiTouchInputMapperStub* create = (CreateMiTouchInputMapperStub *)dlsym(LibMiTouchInputMapperImpl, "createTouchInputMapper");
            if (create) {
                ImplInstance = create();
            } else {
                ALOGE("dlsym is fail TouchInputMapper");
            }
        } else {
            ALOGE("open %s failed! dlopen - %s", LIBPATH, dlerror());
        }
        inited = true;
    }
    return ImplInstance;
}

void MiTouchInputMapperStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibMiTouchInputMapperImpl) {
        DestroyMiTouchInputMapperStub* destroy = (DestroyMiTouchInputMapperStub *)dlsym(LibMiTouchInputMapperImpl, "destroyTouchInputMapper");
        destroy(ImplInstance);
        dlclose(LibMiTouchInputMapperImpl);
        LibMiTouchInputMapperImpl = nullptr;
        ImplInstance = nullptr;
    }
}

void MiTouchInputMapperStub::init(TouchInputMapper* touchInputMapper) {
    IMiTouchInputMapperStub* IStub = GetImplInstance();
    if (IStub) {
        IStub->init(touchInputMapper);
    }
}

bool MiTouchInputMapperStub::dispatchMotionIntercept(NotifyMotionArgs* args, int surfaceOrientation) {
    bool result = false;
    IMiTouchInputMapperStub* IStub = GetImplInstance();
    if (IStub) {
        result = IStub->dispatchMotionIntercept(args, surfaceOrientation);
    }
    return result;
}

}