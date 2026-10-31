#include "MiInputReaderStub.h"
#include <dlfcn.h>
#include <iostream>
#include <log/log.h>

namespace android {
void* MiInputReaderStub::LibMiInputReaderImpl = nullptr;
IMiInputReaderStub* MiInputReaderStub::ImplInstance = nullptr;
std::mutex MiInputReaderStub::StubLock;
bool MiInputReaderStub::inited = false;

IMiInputReaderStub* MiInputReaderStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!LibMiInputReaderImpl && !inited) {
        LibMiInputReaderImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibMiInputReaderImpl) {
            Create* create = (Create *)dlsym(LibMiInputReaderImpl, "createInputReaderStubImpl");
            if (create) {
                ImplInstance = create();
            } else {
                ALOGE("dlsym is fail READER");
            }
        } else {
            ALOGE("inputreader open %s failed! dlopen - %s", LIBPATH, dlerror());
        }
        inited = true;
    }
    return ImplInstance;
}

void MiInputReaderStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibMiInputReaderImpl) {
        Destroy* destroy = (Destroy *)dlsym(LibMiInputReaderImpl, "destroyInputReaderStubImpl");
        destroy(ImplInstance);
        dlclose(LibMiInputReaderImpl);
        LibMiInputReaderImpl = nullptr;
        ImplInstance = nullptr;
    }
}

void MiInputReaderStub::init(InputReader* inputReader) {
    IMiInputReaderStub* IStub = GetImplInstance();
    if (IStub) {
        IStub->init(inputReader);
    }
}

bool MiInputReaderStub::getInputReaderAll() {
    IMiInputReaderStub* IStub = GetImplInstance();
    if (IStub) {
        return IStub->getInputReaderAll();
    }
    return true;
}

void MiInputReaderStub::addDeviceLocked(std::shared_ptr<InputDevice> inputDevice) {
    IMiInputReaderStub* IStub = GetImplInstance();
    if (IStub) {
        IStub->addDeviceLocked(inputDevice);
    }
}


void MiInputReaderStub::removeDeviceLocked(std::shared_ptr<InputDevice> inputDevice) {
    IMiInputReaderStub* IStub = GetImplInstance();
    if (IStub) {
        IStub->removeDeviceLocked(inputDevice);
    }
}


bool MiInputReaderStub::handleConfigurationChangedLockedIntercept(nsecs_t when) {
    bool result = false;
    IMiInputReaderStub* IStub = GetImplInstance();
    if (IStub) {
        result = IStub->getInputReaderAll();
    }
    return result;
}

void MiInputReaderStub::loopOnceChanges(uint32_t changes) {
    IMiInputReaderStub* IStub = GetImplInstance();
    if (IStub) {
        IStub->loopOnceChanges(changes);
    }
}

}