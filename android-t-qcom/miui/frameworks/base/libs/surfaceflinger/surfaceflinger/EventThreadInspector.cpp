#undef LOG_TAG
#define LOG_TAG "EventThreadInspector"

#include "EventThreadInspector.h"

#include <log/log.h>

static const nsecs_t DELAYED_VYNSC_TIMEMILLIS = 30 * 1000000;
static const int MAX_BAD_VSYNC_COUNT = 60;
static const int MAX_NORMAL_VSYNC_COUNT = 3;

namespace android {

EventThreadInspector::EventThreadInspector(const char* threadName):
    mThreadName(threadName) {
    reset();
}

EventThreadInspector::~EventThreadInspector() {}

void EventThreadInspector::checkVsyncWaitTime(nsecs_t waitVsyncStartTime, const char* state,
        PhysicalDisplayId displayId) {
    nsecs_t duration = systemTime(SYSTEM_TIME_MONOTONIC) - waitVsyncStartTime;

    if (duration >= DELAYED_VYNSC_TIMEMILLIS) {
        if (mBadVsyncCount < MAX_BAD_VSYNC_COUNT) {
            mNormalVsyncTotalDuration = 0;
            mSerialNormalVsyncCount = 0;
        } else {
            reportDelayedVsyncs(state, displayId);
        }

        mBadVsyncTotalDuration += duration;
        mBadVsyncCount++;
        mMaxDelayedVsyncDuration = (duration > mMaxDelayedVsyncDuration) ?
                duration : mMaxDelayedVsyncDuration;
    } else if (mBadVsyncTotalDuration > 0) {
        if (mSerialNormalVsyncCount < MAX_NORMAL_VSYNC_COUNT) {
            mBadVsyncTotalDuration += duration;
            mBadVsyncCount++;
            mNormalVsyncTotalDuration += duration;
            mSerialNormalVsyncCount++;
        } else {
            reportDelayedVsyncs(state, displayId);
        }
    }
}

void EventThreadInspector::reportDelayedVsyncs(const char* state, PhysicalDisplayId displayId) {
    SLOGW("slow %s-%s on display(%s) count=%d, avg=%ld ms, max=%ld ms.",
            state, mThreadName, to_string(displayId).c_str(),
            mBadVsyncCount - mSerialNormalVsyncCount,
            long(ns2ms((mBadVsyncTotalDuration - mNormalVsyncTotalDuration) /
            (mBadVsyncCount - mSerialNormalVsyncCount))),
            long(ns2ms(mMaxDelayedVsyncDuration))
        );
    reset();
}

void EventThreadInspector::reset() {
    mBadVsyncTotalDuration = 0;
    mBadVsyncCount = 0;
    mNormalVsyncTotalDuration = 0;
    mSerialNormalVsyncCount = 0;
    mMaxDelayedVsyncDuration = 0;
}

} // namespace android
