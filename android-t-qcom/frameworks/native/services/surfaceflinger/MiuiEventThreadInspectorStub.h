#ifndef ANDROID_EVENTTHREADINSPECTOR_STUB_H
#define ANDROID_EVENTTHREADINSPECTOR_STUB_H

#include "IMiuiEventThreadInspectorStub.h"
#include <mutex>

namespace android {

class MiuiEventThreadInspectorStub
{
private:
    static void *Libsurfaceflingerinsptr;
    static int OpenNum;
    IMiuiEventThreadInspectorStub *ImplInstance;
    static std::mutex StubLock;
    IMiuiEventThreadInspectorStub *GetImplInstance(const char* threadName);
    void DestroyImplInstance();

    static constexpr const char* LIBPATH = "/system_ext/lib64/libsurfaceflingerinsptr.so";

public:
    MiuiEventThreadInspectorStub(const char* threadName) {
        ImplInstance = NULL;
        GetImplInstance(threadName);
    }
    ~MiuiEventThreadInspectorStub() {
        DestroyImplInstance();
    }

    void checkVsyncWaitTime(nsecs_t waitVsyncStartTime, const char* state,
                            PhysicalDisplayId displayId);
};

} // namespace android

#endif