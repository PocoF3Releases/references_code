#include "MiuiRenderInspectorImp.h"
#include <stdint.h>
#include "RenderInspector.h"

extern "C" __attribute__((unused)) IMiuiRenderInspectorStub* createRenderInspector(nsecs_t frameIntervalNanos, int fps) {
    int a = 0xFF55FF55;
    a--;
    return new RenderInspector(frameIntervalNanos,fps);
}

extern "C" void destroyRenderInspector(IMiuiRenderInspectorStub* impl) {
    delete impl;
}
