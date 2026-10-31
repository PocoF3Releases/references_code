#include "MiuiSurfaceFlingerInspectorStub.h"
#include <dlfcn.h>
#include <iostream>
#include <log/log.h>

namespace android {

void* MiuiSurfaceFlingerInspectorStub::Libsurfaceflingerinsptr = nullptr;
IMiuiSurfaceFlingerInspectorStub* MiuiSurfaceFlingerInspectorStub::ImplInstance = nullptr;
std::mutex MiuiSurfaceFlingerInspectorStub::StubLock;

IMiuiSurfaceFlingerInspectorStub* MiuiSurfaceFlingerInspectorStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!Libsurfaceflingerinsptr) {
        Libsurfaceflingerinsptr = dlopen(LIBPATH, RTLD_LAZY);
        if (Libsurfaceflingerinsptr) {
            CreateSurfaceFlingerInspector* create = (CreateSurfaceFlingerInspector *)
                    dlsym(Libsurfaceflingerinsptr, "createSurfaceFlingerInspector");
            ImplInstance = create();
        }
    }
    return ImplInstance;
}

void MiuiSurfaceFlingerInspectorStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (Libsurfaceflingerinsptr) {
        DestroySurfaceFlingerInspector* destroy = (DestroySurfaceFlingerInspector *)
                dlsym(Libsurfaceflingerinsptr, "destroySurfaceFlingerInspector");
        destroy(ImplInstance);
        dlclose(Libsurfaceflingerinsptr);
        Libsurfaceflingerinsptr = nullptr;
        ImplInstance = nullptr;
    }
}

void MiuiSurfaceFlingerInspectorStub::monitorFrameComposition(std::string where,
                                                              std::string displayName) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->monitorFrameComposition(where, displayName);
    }
}

void MiuiSurfaceFlingerInspectorStub::markStartMonitorFrame() {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->markStartMonitorFrame();
    }
}

void MiuiSurfaceFlingerInspectorStub::markBufferLatched(String8 layerName) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->markBufferLatched(layerName);
    }
}

void MiuiSurfaceFlingerInspectorStub::markTransactionStateChanged(String8 layerName,
                                                                  uint64_t what) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->markTransactionStateChanged(layerName, what);
    }
}

void MiuiSurfaceFlingerInspectorStub::markTransactionHandled(String8 layerName) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->markTransactionHandled(layerName);
    }
}

void MiuiSurfaceFlingerInspectorStub::monitorSurfaceIfNeeded(String8 layerName, Rect rect) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->monitorSurfaceIfNeeded(layerName, rect);
    }
}

void MiuiSurfaceFlingerInspectorStub::checkFrameDropped(nsecs_t vsyncTimeStamp,
                                                        nsecs_t vsyncPeriod) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->checkFrameDropped(vsyncTimeStamp, vsyncPeriod);
    }
}

void MiuiSurfaceFlingerInspectorStub::producerFrameDropped(int32_t droppedFrameCount,
                                                           int64_t intendedVsyncTime,
                                                           String8 windowName) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->producerFrameDropped(droppedFrameCount, intendedVsyncTime, windowName);
    }
}

void MiuiSurfaceFlingerInspectorStub::dumpFrameInfo(Parcel* reply) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->dumpFrameInfo(reply);
    }
}

void MiuiSurfaceFlingerInspectorStub::monitorAppFrame(std::string layerName, int64_t token,
                                                      int64_t displayFrameToken,
                                                      nsecs_t deadlineDelta,
                                                      nsecs_t period, bool isAppDeadlineMissed) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->monitorAppFrame(layerName, token, displayFrameToken,
                               deadlineDelta, period, isAppDeadlineMissed);
    }
}
void MiuiSurfaceFlingerInspectorStub::recordBufferTxCount(const char* name,  int32_t count) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->recordBufferTxCount(name, count);
    }
}

int MiuiSurfaceFlingerInspectorStub::getBufferTxCount() {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        return Istub->getBufferTxCount();
    }

    return -1;
}

// FrameTimeline: support Jank detection for doframe -1
void MiuiSurfaceFlingerInspectorStub::setVsyncPeriodNsecs(const nsecs_t period) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->setVsyncPeriodNsecs(period);
    }
}

void MiuiSurfaceFlingerInspectorStub::setLastPredictions(const std::string layerName,
                                                         nsecs_t startTime, nsecs_t endTime,
                                                         nsecs_t presentTime) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->setLastPredictions(layerName, startTime, endTime, presentTime);
    }
}

void MiuiSurfaceFlingerInspectorStub::getCurrentSurfaceFrameInfo(const std::string layerName,
        int64_t frameStartTimeNanos, int64_t *outFrameVsyncId, nsecs_t *outStartTime,
        nsecs_t *outEndTime, nsecs_t *outPresentTime) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->getCurrentSurfaceFrameInfo(layerName, frameStartTimeNanos, outFrameVsyncId,
                                          outStartTime, outEndTime, outPresentTime);
    }
}

void MiuiSurfaceFlingerInspectorStub::removePredictionsByLayer(const std::string layerName) {
    IMiuiSurfaceFlingerInspectorStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->removePredictionsByLayer(layerName);
    }
}
// END
} // namespace android
