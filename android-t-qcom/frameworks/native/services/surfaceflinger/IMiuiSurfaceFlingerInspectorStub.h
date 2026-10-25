#ifndef ANDROID_ISURFACEFLINGERINSPECTOR_STUB_H
#define ANDROID_ISURFACEFLINGERINSPECTOR_STUB_H

#include <binder/Parcel.h>
#include <ui/Rect.h>
#include <utils/String8.h>
#include <utils/Timers.h>

namespace android {

class IMiuiSurfaceFlingerInspectorStub {
public:
    virtual ~IMiuiSurfaceFlingerInspectorStub() {}
    virtual void monitorFrameComposition(std::string where, std::string displayName);
    virtual void markStartMonitorFrame();

    virtual void markBufferLatched(String8 layerName);
    virtual void markTransactionStateChanged(String8 layerName, uint64_t what);
    virtual void markTransactionHandled(String8 layerName);
    virtual void monitorSurfaceIfNeeded(String8 layerName, Rect rect);
    virtual void checkFrameDropped(nsecs_t vsyncTimeStamp, nsecs_t vsyncPeriod);
    virtual void producerFrameDropped(int32_t droppedFrameCount,
                              int64_t intendedVsyncTime, String8 windowName);
    virtual void dumpFrameInfo(Parcel* reply);

    virtual void monitorAppFrame(std::string layerName, int64_t token, int64_t displayFrameToken,
                                nsecs_t deadlineDelta, nsecs_t period, bool isAppDeadlineMissed);
    virtual void recordBufferTxCount(const char* name, int32_t count);
    virtual int getBufferTxCount();

    // FrameTimeline: support Jank detection for doframe -1
    virtual void setVsyncPeriodNsecs(const nsecs_t period);
    virtual void setLastPredictions(const std::string layerName, nsecs_t startTime,
                                    nsecs_t endTime, nsecs_t presentTime);
    virtual void getCurrentSurfaceFrameInfo(const std::string layerName,int64_t frameStartTimeNanos,
                                            int64_t *outFrameVsyncId, nsecs_t *outStartTime,
                                            nsecs_t *outEndTime, nsecs_t *outPresentTime);
    virtual void removePredictionsByLayer(const std::string layerName);
    // END
};

typedef IMiuiSurfaceFlingerInspectorStub* CreateSurfaceFlingerInspector();
typedef void DestroySurfaceFlingerInspector(IMiuiSurfaceFlingerInspectorStub *);

} // namespace android

#endif
