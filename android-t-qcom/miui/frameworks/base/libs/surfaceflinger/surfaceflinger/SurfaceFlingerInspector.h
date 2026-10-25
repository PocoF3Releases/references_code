#ifndef SURFACE_FLINGER_INSPECTOR_H_
#define SURFACE_FLINGER_INSPECTOR_H_

#include <binder/Parcel.h>
#include <gui/LayerState.h>
#include <ui/GraphicTypes.h>
#include <ui/Rect.h>
#include <utils/Mutex.h>
#include <utils/String8.h>
#include <utils/Timers.h>
#include <utils/Trace.h>
#include "IMiuiSurfaceFlingerInspectorStub.h"
#include "FrameTimelineInjector.h"

#include <list>
#include <map>

using namespace std;

namespace android {

struct DropRecordItem {
    int times = 0;
    int droppedFrameCount = 0;
    int duration = 0;
    list<int> counts;
};

struct ProducerDropItem {
    int droppedFrameCount = 0;
    int64_t intendedVsyncTime = 0;
};

enum DumpState {
    Dumping = 0,
    Sleep,
};

#define FRAME_RECORD_INFO_SIZE 6
enum FrameRecordInfoIndex {
    DrewFrameCount = 0,
    DroppedFrameCount,
    PerceptibleDroppedFrameCount,
    SeriousDroppedFrameCount,
    SerialDropTimes,
    SingleDropTimes,
};

class SurfaceInspector {
public:
    SurfaceInspector();
    ~SurfaceInspector();
    void init(String8 layerName);
    void setSuffix(String8 suffix);
    void setPreffix(String8 preffix) { mPreffix = preffix; }

    void markBufferLatched();
    void producerFrameDropped(int32_t droppedFrameCount, int64_t intendedVsyncTime);
    void removeLastDropIfNeeded(int64_t intendedVsyncTime);
    void checkFrameDropped(nsecs_t vsyncTimeStamp);
    void dump(Parcel* reply);
    String8 getLayerName() { return mPreffix + mLayerName + mSuffix; }
    void reset();
    String8 dropRecordToString();
    bool noRefreshState(nsecs_t now);

    static int64_t sVsyncPeriod;

private:
    int checkProducerDropped(nsecs_t vsyncTimeStamp);
    int checkConsumerDropped(nsecs_t vsyncTimeStamp, nsecs_t now);
    void updateFrameRecord(int drewFrameCount, int droppedFrameCount);
    void completeCurrentSerialDrop();
    bool isTransactionInspector() { return mPreffix.size() > 0; }

    String8 mLayerName;
    String8 mPreffix;
    String8 mSuffix;

    int mFrameRecordInfo[FRAME_RECORD_INFO_SIZE] = {0};

    DropRecordItem* mSerialDropRecord;

    int mProducerReportCount = 0;
    int64_t mIntendedVsyncTime = 0;
    int64_t mRemoveIntendedVsyncTime = 0;

    int64_t mLastFrameEndTime = 0;
    int mSleepedVsyncCount = 0;
    int64_t mLastLatchedFrameEndTime = 0;
    bool mBufferLatched = false;

    bool mConsumerLastFrameDropped = false;
    nsecs_t mConsumerDropDuration = 0;

    list<DropRecordItem> mDropRecordList;
};

class SurfaceFlingerInspector : public IMiuiSurfaceFlingerInspectorStub {
public:
    SurfaceFlingerInspector();
    ~SurfaceFlingerInspector();
    void monitorFrameComposition(std::string where, std::string displayName);
    void markStartMonitorFrame();

private:
    nsecs_t mLastStageEndTime;

public:
    void markBufferLatched(String8 layerName);
    void markTransactionStateChanged(String8 layerName, uint64_t what);
    void markTransactionHandled(String8 layerName);
    void monitorSurfaceIfNeeded(String8 layerName, Rect rect);
    void checkFrameDropped(nsecs_t vsyncTimeStamp, nsecs_t vsyncPeriod);
    void producerFrameDropped(int32_t droppedFrameCount,
                              int64_t intendedVsyncTime, String8 windowName);
    void dumpFrameInfo(Parcel* reply);

    void recordBufferTxCount(const char* name, int32_t count);
    int getBufferTxCount();
    void monitorAppFrame(std::string layerName, int64_t token, int64_t displayFrameToken,
                        nsecs_t deadlineDelta, nsecs_t period, bool isAppDeadlineMissed);
    // FrameTimeline: support Jank detection for doframe -1
    void setVsyncPeriodNsecs(const nsecs_t period);
    void setLastPredictions(const std::string layerName, nsecs_t startTime,
                                   nsecs_t endTime, nsecs_t presentTime);
    void getCurrentSurfaceFrameInfo(const std::string layerName, int64_t frameStartTimeNanos,
                                    int64_t *outFrameVsyncId, nsecs_t *outStartTime,
                                    nsecs_t *outEndTime, nsecs_t *outPresentTime);
    void removePredictionsByLayer(const std::string layerName);
    // END
private:
    void resetData();

    list<String8> mTopLayersList;
    list<String8> mLastTopLayerList GUARDED_BY(mLock);
    mutable std::mutex mLock;
    map<String8, SurfaceInspector> mSurfaceInspectorMap GUARDED_BY(mLock);
    map<String8, SurfaceInspector> mTransactionInspectorMap GUARDED_BY(mLock);

    nsecs_t mLastDumpTime;
    DumpState mDumpState = Sleep;

    int mExceptionLogFd;
    int mLogTurboVersion;
    std::string mRomVersion;
    nsecs_t mLongFrameRefreshThreshold;

    std::string mBufferTxName;
    int mBufferTxCount;
    std::unique_ptr<FrameTimelineInjector> mFrameTimelineInjector;
};

} // namespace android

#endif /*SURFACE_FLINGER_INSPECTOR_H_ */
