#include "FrameTimelineInjector.h"
#include <cutils/properties.h>

namespace android {

FrameTimelineInjector::FrameTimelineInjector()
    : mCurrentToken(INVALID_VSYNC_ID) {}

std::unique_ptr<FrameTimelineInjector> FrameTimelineInjector::createFrameTimelineInjector() {
    // Disabled by default, only enabled when needed for debug(such as FluencyTest).
    bool enable = property_get_bool("persist.sys.frametimeline.invalid_id.enable", false);
    if (enable) {
        return std::make_unique<FrameTimelineInjector>();
    }
    return nullptr;
}

void FrameTimelineInjector::setVsyncPeriodNsecs(const nsecs_t period) {
    mVsyncPeriodNsecs = period;
}

void FrameTimelineInjector::setLastPredictions(const std::string layerName,
        nsecs_t startTime, nsecs_t endTime, nsecs_t presentTime) {
    mLastPredictions[layerName] = {startTime,
                                   endTime - startTime,
                                   presentTime - endTime};
}

void FrameTimelineInjector::getCurrentSurfaceFrameInfo(const std::string layerName,
        int64_t frameStartTimeNanos, int64_t *outFrameVsyncId, nsecs_t *outStartTime,
        nsecs_t *outEndTime, nsecs_t *outPresentTime) {
    if (frameStartTimeNanos > 0 && mVsyncPeriodNsecs > 0) {
        auto iter = mLastPredictions.find(layerName);
        if (iter != mLastPredictions.end()) {
            auto &lastPredictionsInfo = iter->second;

            nsecs_t predStartTime = frameStartTimeNanos;
            nsecs_t predEndTime = lastPredictionsInfo.lastVsyncStartTime + mVsyncPeriodNsecs +
                                  lastPredictionsInfo.appWorkDurationTime;
            nsecs_t predPresentTime = predEndTime + lastPredictionsInfo.appReadyDurationTime;

            do {
                // Set to the next Vsync start time, the current predictions start time will
                // always be less than the next Vsync start time.
                lastPredictionsInfo.lastVsyncStartTime += mVsyncPeriodNsecs;
            } while (lastPredictionsInfo.lastVsyncStartTime + mVsyncPeriodNsecs < predStartTime);

            if (predStartTime > predEndTime) {
                // Refer to the processing method of PredictionState::Expired. The Jank occurred
                // in the previous frame and the current frame doFrame triggers a delay, Both
                // will cause startTime to be delayed, and we can't use the predicted startTime.
                // Instead, just use a start time a little earlier than the end time so that
                // FrameTimeline has some info about this frame in the trace.
                predStartTime = predEndTime - lastPredictionsInfo.appWorkDurationTime;
            }

            *outFrameVsyncId = --mCurrentToken;
            *outStartTime = predStartTime;
            *outEndTime = predEndTime;
            *outPresentTime = predPresentTime;
        }
    }
}

void FrameTimelineInjector::removePredictionsByLayer(const std::string layerName) {
    mLastPredictions.erase(layerName);
}

FrameTimelineInjector::~FrameTimelineInjector() {}

} // namespace android
