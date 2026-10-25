#ifndef ANDROID_SURFACEFLINGERINSPECTOR_STUB_H
#define ANDROID_SURFACEFLINGERINSPECTOR_STUB_H

#include "IMiuiSurfaceFlingerInspectorStub.h"
#include <mutex>

namespace android {

class MiuiSurfaceFlingerInspectorStub
{
private:
    MiuiSurfaceFlingerInspectorStub() {}
    static void *Libsurfaceflingerinsptr;
    static IMiuiSurfaceFlingerInspectorStub *ImplInstance;
    static IMiuiSurfaceFlingerInspectorStub *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;

    static constexpr const char* LIBPATH = "/system_ext/lib64/libsurfaceflingerinsptr.so";

public:
    virtual ~MiuiSurfaceFlingerInspectorStub() {}
    static void monitorFrameComposition(std::string where, std::string displayName);
    static void markStartMonitorFrame();

    static void markBufferLatched(String8 layerName);
    static void markTransactionStateChanged(String8 layerName, uint64_t what);
    static void markTransactionHandled(String8 layerName);
    static void monitorSurfaceIfNeeded(String8 layerName, Rect rect);
    static void checkFrameDropped(nsecs_t vsyncTimeStamp, nsecs_t vsyncPeriod);
    static void producerFrameDropped(int32_t droppedFrameCount,
                              int64_t intendedVsyncTime, String8 windowName);
    static void dumpFrameInfo(Parcel* reply);

    static void monitorAppFrame(std::string layerName, int64_t token, int64_t displayFrameToken,
                                nsecs_t deadlineDelta, nsecs_t period, bool isAppDeadlineMissed);
    static void recordBufferTxCount(const char* name, int32_t count);
    static int getBufferTxCount();

    // FrameTimeline: support Jank detection for doframe -1
    static void setVsyncPeriodNsecs(const nsecs_t period);
    static void setLastPredictions(const std::string layerName, nsecs_t startTime,
                                   nsecs_t endTime, nsecs_t presentTime);
    static void getCurrentSurfaceFrameInfo(const std::string layerName, int64_t frameStartTimeNanos,
                                           int64_t *outFrameVsyncId, nsecs_t *outStartTime,
                                           nsecs_t *outEndTime, nsecs_t *outPresentTime);
    static void removePredictionsByLayer(const std::string layerName);
    // END
};

} // namespace android

#endif
