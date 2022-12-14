#include "MiCursorInputMapperStub.h"
#include <dlfcn.h>
#include <iostream>
#include <log/log.h>

namespace android {
void* MiCursorInputMapperStub::LibMiCursorInputMapperImpl = nullptr;
IMiCursorInputMapperStub* MiCursorInputMapperStub::ImplInstance = nullptr;
std::mutex MiCursorInputMapperStub::StubLock;
bool MiCursorInputMapperStub::inited = false;

IMiCursorInputMapperStub* MiCursorInputMapperStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!LibMiCursorInputMapperImpl && !inited) {
        LibMiCursorInputMapperImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibMiCursorInputMapperImpl) {
            Create* create = (Create *)dlsym(LibMiCursorInputMapperImpl, "createCursorInputMapperStubImpl");
            if (create) {
                ImplInstance = create();
            } else {
                ALOGE("dlsym is fail READER");
            }
        } else {
            ALOGE("MiCursorInputMapper open %s failed! dlopen - %s", LIBPATH, dlerror());
        }
        inited = true;
    }
    return ImplInstance;
}

void MiCursorInputMapperStub::init() {
    IMiCursorInputMapperStub* IStub = GetImplInstance();
    if (IStub) {
        IStub->init();
    }
}

void MiCursorInputMapperStub::sync(float*  vscroll, float*  hscroll) {
    IMiCursorInputMapperStub* IStub = GetImplInstance();
    if (IStub) {
        IStub->sync(vscroll, hscroll);
    }
}

}