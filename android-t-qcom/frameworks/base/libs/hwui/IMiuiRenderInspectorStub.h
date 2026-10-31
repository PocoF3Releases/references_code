
#pragma once
#ifndef ANDROID_HWUI_RENDERINSPECTOR_H
#define ANDROID_HWUI_RENDERINSPECTOR_H

#include <FrameInfo.h>
using namespace android::uirenderer;

class IMiuiRenderInspectorStub {
public:
    virtual ~IMiuiRenderInspectorStub() {}
    virtual void addFrame(const FrameInfo& frame, const std::string& name);
    virtual void ignoreNextFrame();
    virtual int checkFrameDropped(FrameInfo& frame, int64_t vsyncPeriodNanos,
            bool hardwareLayerUpdated, int height, int width);
    virtual void getRenderToken();
    virtual void releaseRenderToken();
};
typedef IMiuiRenderInspectorStub* CreateRenderInspector(nsecs_t frameIntervalNanos, int fps);
typedef void DestroyRenderInspector(IMiuiRenderInspectorStub *);
#endif
