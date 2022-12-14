#ifndef ANDROID_SURFACEFLINGERINSPECTOR_IMPL_H
#define ANDROID_SURFACEFLINGERINSPECTOR_IMPL_H
#include <IMiuiSurfaceFlingerInspectorStub.h>

namespace android {

extern "C" IMiuiSurfaceFlingerInspectorStub* createSurfaceFlingerInspector();
extern "C" void destroySurfaceFlingerInspector(IMiuiSurfaceFlingerInspectorStub* impl);

} // namespace android

#endif
