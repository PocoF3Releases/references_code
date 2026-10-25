#ifndef ANDROID_EventThreadInspector_IMPL_H
#define ANDROID_EventThreadInspector_IMPL_H
#include <IMiuiEventThreadInspectorStub.h>


namespace android {

extern "C" IMiuiEventThreadInspectorStub* createEventThreadInspector(const char* threadName);
extern "C" void destroyEventThreadInspector(IMiuiEventThreadInspectorStub* impl);

} // namespace android

#endif
