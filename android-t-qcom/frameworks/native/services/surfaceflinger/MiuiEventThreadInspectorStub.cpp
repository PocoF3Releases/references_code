#include "MiuiEventThreadInspectorStub.h"
#include <dlfcn.h>
#include <iostream>
#include <log/log.h>

namespace android {

void* MiuiEventThreadInspectorStub::Libsurfaceflingerinsptr = nullptr;
std::mutex MiuiEventThreadInspectorStub::StubLock;
int MiuiEventThreadInspectorStub::OpenNum = 0;

IMiuiEventThreadInspectorStub* MiuiEventThreadInspectorStub::GetImplInstance(const char* threadName) {
    std::lock_guard<std::mutex> lock(StubLock);
    OpenNum ++;
    if (!ImplInstance) {
        if (!Libsurfaceflingerinsptr) {
            Libsurfaceflingerinsptr = dlopen(LIBPATH, RTLD_LAZY);
        }
        if (Libsurfaceflingerinsptr) {
            CreateEventThreadInspector* create = (CreateEventThreadInspector *)
                    dlsym(Libsurfaceflingerinsptr, "createEventThreadInspector");
            ImplInstance = create(threadName);
        }
    }
    return ImplInstance;
}

void MiuiEventThreadInspectorStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    OpenNum --;
    if (!Libsurfaceflingerinsptr) {
        Libsurfaceflingerinsptr = dlopen(LIBPATH, RTLD_LAZY);
    }
    if (Libsurfaceflingerinsptr) {
        DestroyEventThreadInspector* destroy = (DestroyEventThreadInspector *)
                dlsym(Libsurfaceflingerinsptr, "destroyEventThreadInspector");
        destroy(ImplInstance);
        if (OpenNum == 0) {
            dlclose(Libsurfaceflingerinsptr);
            Libsurfaceflingerinsptr = nullptr;
        }
        ImplInstance = nullptr;
    }
}

void MiuiEventThreadInspectorStub::checkVsyncWaitTime(nsecs_t waitVsyncStartTime,
                        const char* state, PhysicalDisplayId displayId) {
    if (ImplInstance) {
        ImplInstance->checkVsyncWaitTime(waitVsyncStartTime,
                state, displayId);
    }
}

} // namespace android