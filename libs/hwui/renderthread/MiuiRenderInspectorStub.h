#pragma once

#include "../IMiuiRenderInspectorStub.h"
#include <mutex>

#if defined(__LP64__)
    static constexpr const char* LIBPATH = "/system_ext/lib64/libhwuiinsptrdl.so";
#else
    static constexpr const char* LIBPATH = "/system_ext/lib/libhwuiinsptrdl.so";
#endif

class MiuiRenderInspectorStub {
private:
    IMiuiRenderInspectorStub *ImplInstance;
    static std::mutex StubLock;
    IMiuiRenderInspectorStub *GetImplInstance(nsecs_t frameIntervalNanos, int fps = 60);
    void DestroyImplInstance();

public:
    MiuiRenderInspectorStub(nsecs_t frameIntervalNanos, int fps = 60) {
        ImplInstance = NULL;
        GetImplInstance(frameIntervalNanos, fps);
    }

    ~MiuiRenderInspectorStub() {
        DestroyImplInstance();
    }
    void addFrame(const FrameInfo& frame, const std::string& name);
    void ignoreNextFrame();
    int checkFrameDropped(FrameInfo& frame, int64_t vsyncPeriodNanos,
            bool hardwareLayerUpdated, int height, int width);
    void getRenderToken();
    void releaseRenderToken();
};
