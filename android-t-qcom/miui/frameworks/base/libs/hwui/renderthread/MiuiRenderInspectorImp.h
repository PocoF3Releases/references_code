#pragma once

#ifndef ANDROID_RenderInspector_IMPL_H
#define ANDROID_RenderInspector_IMPL_H
#include <IMiuiRenderInspectorStub.h>
extern "C" IMiuiRenderInspectorStub* createRenderInspector(nsecs_t frameIntervalNanos, int fps = 60);
extern "C" void destroyRenderInspector(IMiuiRenderInspectorStub* impl);
#endif
