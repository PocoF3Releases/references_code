#include "MiuiEventThreadInspectorImpl.h"
#include <stdint.h>
#include "EventThreadInspector.h"

namespace android {

extern "C" IMiuiEventThreadInspectorStub* createEventThreadInspector(const char* threadName) {
    return new EventThreadInspector(threadName);
}

extern "C" void destroyEventThreadInspector(IMiuiEventThreadInspectorStub* impl) {
    delete impl;
}

} // namespace android
