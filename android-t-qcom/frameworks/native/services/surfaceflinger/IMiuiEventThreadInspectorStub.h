#ifndef ANDROID_IEVENTTHREADINSPECTOR_STUB_H
#define ANDROID_IEVENTTHREADINSPECTOR_STUB_H

#include <utils/Timers.h>
#include <ui/DisplayId.h>

namespace android {

class IMiuiEventThreadInspectorStub
{
public:
    virtual ~IMiuiEventThreadInspectorStub() {}
    virtual void checkVsyncWaitTime(nsecs_t waitVsyncStartTime, const char* state,
                            PhysicalDisplayId displayId);
};

typedef IMiuiEventThreadInspectorStub* CreateEventThreadInspector(const char* threadName);
typedef void DestroyEventThreadInspector(IMiuiEventThreadInspectorStub *);

} // namespace android

#endif