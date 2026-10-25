#ifndef FRAME_TIME_LINE_INJECTOR_H_
#define FRAME_TIME_LINE_INJECTOR_H_

#include <string>
#include <unordered_map>
#include <utils/Timers.h>

namespace android {

struct PredictionsInfo {
    PredictionsInfo(nsecs_t lastVsyncStartTime = 0,
                    nsecs_t appWorkDurationTime = 0,
                    nsecs_t appReadyDurationTime = 0)
        : lastVsyncStartTime(lastVsyncStartTime),
          appWorkDurationTime(appWorkDurationTime),
          appReadyDurationTime(appReadyDurationTime) {}

    nsecs_t lastVsyncStartTime;
    nsecs_t appWorkDurationTime;
    nsecs_t appReadyDurationTime;
};

class FrameTimelineInjector {
public:
    FrameTimelineInjector();
    static std::unique_ptr<FrameTimelineInjector> createFrameTimelineInjector();
    void setVsyncPeriodNsecs(const nsecs_t period);
    void setLastPredictions(const std::string layerName, nsecs_t startTime,
        nsecs_t endTime, nsecs_t presentTime);
    void getCurrentSurfaceFrameInfo(const std::string layerName, int64_t frameStartTimeNanos,
        int64_t *outFrameVsyncId, nsecs_t *outStartTime, nsecs_t *outEndTime, nsecs_t *outPresentTime);
    void removePredictionsByLayer(const std::string layerName);
    ~FrameTimelineInjector();

private:
    static constexpr int64_t INVALID_VSYNC_ID = -1;
    int64_t mCurrentToken;
    nsecs_t mVsyncPeriodNsecs;
    std::unordered_map<std::string, PredictionsInfo> mLastPredictions;
};

} // namespace android

#endif // FRAME_TIME_LINE_INJECTOR_H_
