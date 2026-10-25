#include "MiuiSurfaceFlingerInspectorImpl.h"
#include <stdint.h>
#include "SurfaceFlingerInspector.h"

namespace android {

extern "C" IMiuiSurfaceFlingerInspectorStub* createSurfaceFlingerInspector() {
    return new SurfaceFlingerInspector();
}

extern "C" void destroySurfaceFlingerInspector(IMiuiSurfaceFlingerInspectorStub* impl) {
    delete impl;
}

} // namespace android
