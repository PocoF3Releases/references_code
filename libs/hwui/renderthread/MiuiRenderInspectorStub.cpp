#include <iostream>
#include <dlfcn.h>
#include <android-base/logging.h>
#include "MiuiRenderInspectorStub.h"


static void* LibHwuiImpl = dlopen(LIBPATH, RTLD_LAZY);
std::mutex MiuiRenderInspectorStub::StubLock;

IMiuiRenderInspectorStub* MiuiRenderInspectorStub::GetImplInstance(nsecs_t frameIntervalNanos,
                                                                   int fps) {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!ImplInstance) {
        if (!LibHwuiImpl) {
            LibHwuiImpl = dlopen(LIBPATH, RTLD_LAZY);
        }
        if (LibHwuiImpl) {
            CreateRenderInspector* create =
                    (CreateRenderInspector *)dlsym(LibHwuiImpl, "createRenderInspector");
            ImplInstance = create(frameIntervalNanos,fps);
        }
    }
    return ImplInstance;
}

void MiuiRenderInspectorStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!LibHwuiImpl) {
        LibHwuiImpl = dlopen(LIBPATH, RTLD_LAZY);
    }
    if (LibHwuiImpl) {
        DestroyRenderInspector* destroy =
                (DestroyRenderInspector *)dlsym(LibHwuiImpl, "destroyRenderInspector");
        destroy(ImplInstance);
        ImplInstance = NULL;
    }
}

void MiuiRenderInspectorStub::addFrame(const FrameInfo& frame, const std::string& name) {
    if (ImplInstance) {
        ImplInstance->addFrame(frame, name);
    }
}

void MiuiRenderInspectorStub::ignoreNextFrame() {
    if (ImplInstance) {
        ImplInstance->ignoreNextFrame();
    }
}

int MiuiRenderInspectorStub::checkFrameDropped(FrameInfo& frame,
        int64_t vsyncPeriodNanos, bool hardwareLayerUpdated, int height, int width) {
    int dropCount = 0;
    if (ImplInstance) {
        dropCount = ImplInstance->checkFrameDropped(frame,
                vsyncPeriodNanos, hardwareLayerUpdated, height, width);
    }
    return dropCount;
}

void MiuiRenderInspectorStub::getRenderToken() {
    if (ImplInstance) {
        ImplInstance->getRenderToken();
    }
}

void MiuiRenderInspectorStub::releaseRenderToken() {
    if (ImplInstance) {
        ImplInstance->releaseRenderToken();
    }
}

