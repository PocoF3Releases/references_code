#include "MiEventHubStub.h"
#include <dlfcn.h>
#include <iostream>
#include <log/log.h>

namespace android {
void* MiEventHubStub::LibMiEventHubImpl = nullptr;
IMiEventHubStub* MiEventHubStub::ImplInstance = nullptr;
std::mutex MiEventHubStub::StubLock;
bool MiEventHubStub::inited = false;

IMiEventHubStub* MiEventHubStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!LibMiEventHubImpl && !inited) {
        LibMiEventHubImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibMiEventHubImpl) {
            CreateMiEventHubStub* create = (CreateMiEventHubStub *)dlsym(LibMiEventHubImpl,
                "createEventHubStubImpl");
            if (create) {
                ImplInstance = create();
            } else {
                ALOGE("dlsym is fail EVENTHUB");
            }
        } else {
            ALOGE("inputreader open %s failed! dlopen - %s", LIBPATH, dlerror());
        }
        inited = true;
    }
    return ImplInstance;
}

void MiEventHubStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibMiEventHubImpl) {
        DestroyMiEventHubStub* destroy = (DestroyMiEventHubStub *)dlsym(LibMiEventHubImpl,
            "destroyEventHubStubImpl");
        destroy(ImplInstance);
        dlclose(LibMiEventHubImpl);
        LibMiEventHubImpl = nullptr;
        ImplInstance = nullptr;
    }
}

void MiEventHubStub::init(EventHub* eventHub) {
    IMiEventHubStub* IStub = GetImplInstance();
    if (IStub) {
        IStub->init(eventHub);
    }
}

}