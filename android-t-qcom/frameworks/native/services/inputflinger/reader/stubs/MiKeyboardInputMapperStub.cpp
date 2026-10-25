#include "MiKeyboardInputMapperStub.h"
#include <dlfcn.h>
#include <iostream>
#include <log/log.h>

namespace android {
void* MiKeyboardInputMapperStub::LibMiKeyboardInputMapperImpl = nullptr;
IMiKeyboardInputMapperStub* MiKeyboardInputMapperStub::ImplInstance = nullptr;
std::mutex MiKeyboardInputMapperStub::StubLock;
bool MiKeyboardInputMapperStub::inited = false;

IMiKeyboardInputMapperStub* MiKeyboardInputMapperStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!LibMiKeyboardInputMapperImpl && !inited) {
        LibMiKeyboardInputMapperImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibMiKeyboardInputMapperImpl) {
            CreateMiKeyboardInputMapperStub* create = (CreateMiKeyboardInputMapperStub *)dlsym(LibMiKeyboardInputMapperImpl, "createKeyboardInputMapperStubImpl");
            if (create) {
                ImplInstance = create();
            } else {
                ALOGE("dlsym is fail KeyboardInputMapper");
            }
        } else {
            ALOGE("inputreader open %s failed! dlopen - %s", LIBPATH, dlerror());
        }
        inited = true;
    }
    return ImplInstance;
}

void MiKeyboardInputMapperStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibMiKeyboardInputMapperImpl) {
        DestroyMiKeyboardInputMapperStub* destroy = (DestroyMiKeyboardInputMapperStub *)dlsym(LibMiKeyboardInputMapperImpl, "destroyKeyboardInputMapperStubImpl");
        destroy(ImplInstance);
        dlclose(LibMiKeyboardInputMapperImpl);
        LibMiKeyboardInputMapperImpl = nullptr;
        ImplInstance = nullptr;
    }
}

void MiKeyboardInputMapperStub::init(KeyboardInputMapper* keyboardInputMapper) {
    IMiKeyboardInputMapperStub* IStub = GetImplInstance();
    if (IStub) {
        IStub->init(keyboardInputMapper);
    }
}

bool MiKeyboardInputMapperStub::processIntercept(const RawEvent* rawEvent) {
    IMiKeyboardInputMapperStub* IStub = GetImplInstance();
    if (IStub) {
        return IStub->processIntercept(rawEvent);
    }
    return false;
}

}