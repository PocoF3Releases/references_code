/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <vector>

#include <android-base/stringprintf.h>
#include <ftl/concat.h>
#include <utils/Trace.h>

#include <scheduler/TimeKeeper.h>

#ifdef MI_FEATURE_ENABLE
#include "MiSurfaceFlingerStub.h"
#endif

#include "VSyncDispatchTimerQueue.h"
#include "VSyncTracker.h"

namespace android::scheduler {

using base::StringAppendF;

namespace {

nsecs_t getExpectedCallbackTime(nsecs_t nextVsyncTime,
                                const VSyncDispatch::ScheduleTiming& timing) {
    return nextVsyncTime - timing.readyDuration - timing.workDuration;
}

nsecs_t getExpectedCallbackTime(VSyncTracker& tracker, nsecs_t now,
                                const VSyncDispatch::ScheduleTiming& timing) {
    const auto nextVsyncTime = tracker.nextAnticipatedVSyncTimeFrom(
            std::max(timing.earliestVsync, now + timing.workDuration + timing.readyDuration));
    return getExpectedCallbackTime(nextVsyncTime, timing);
}

} // namespace

VSyncDispatch::~VSyncDispatch() = default;
VSyncTracker::~VSyncTracker() = default;

VSyncDispatchTimerQueueEntry::VSyncDispatchTimerQueueEntry(std::string name,
                                                           VSyncDispatch::Callback callback,
                                                           nsecs_t minVsyncDistance)
      : mName(std::move(name)),
        mCallback(std::move(callback)),
        mMinVsyncDistance(minVsyncDistance) {}

std::optional<nsecs_t> VSyncDispatchTimerQueueEntry::lastExecutedVsyncTarget() const {
    return mLastDispatchTime;
}

std::string_view VSyncDispatchTimerQueueEntry::name() const {
    return mName;
}

std::optional<nsecs_t> VSyncDispatchTimerQueueEntry::wakeupTime() const {
    if (!mArmedInfo) {
        return {};
    }
    return {mArmedInfo->mActualWakeupTime};
}

std::optional<nsecs_t> VSyncDispatchTimerQueueEntry::readyTime() const {
    if (!mArmedInfo) {
        return {};
    }
    return {mArmedInfo->mActualReadyTime};
}

std::optional<nsecs_t> VSyncDispatchTimerQueueEntry::targetVsync() const {
    if (!mArmedInfo) {
        return {};
    }
    return {mArmedInfo->mActualVsyncTime};
}

ScheduleResult VSyncDispatchTimerQueueEntry::schedule(VSyncDispatch::ScheduleTiming timing,
                                                      VSyncTracker& tracker, nsecs_t now) {
    auto nextVsyncTime = tracker.nextAnticipatedVSyncTimeFrom(
            std::max(timing.earliestVsync, now + timing.workDuration + timing.readyDuration));
    auto nextWakeupTime = nextVsyncTime - timing.workDuration - timing.readyDuration;

    bool const wouldSkipAVsyncTarget =
            mArmedInfo && (nextVsyncTime > (mArmedInfo->mActualVsyncTime + mMinVsyncDistance));
    bool const wouldSkipAWakeup =
            mArmedInfo && ((nextWakeupTime > (mArmedInfo->mActualWakeupTime + mMinVsyncDistance)));
    if (wouldSkipAVsyncTarget && wouldSkipAWakeup) {
        return getExpectedCallbackTime(nextVsyncTime, timing);
    }

    bool const alreadyDispatchedForVsync = mLastDispatchTime &&
            ((*mLastDispatchTime + mMinVsyncDistance) >= nextVsyncTime &&
             (*mLastDispatchTime - mMinVsyncDistance) <= nextVsyncTime);
    if (alreadyDispatchedForVsync) {
        nextVsyncTime =
                tracker.nextAnticipatedVSyncTimeFrom(*mLastDispatchTime + mMinVsyncDistance);
        nextWakeupTime = nextVsyncTime - timing.workDuration - timing.readyDuration;
    }

#ifdef MI_FEATURE_ENABLE
    if (MiSurfaceFlingerStub::isSceneExist(VSYNC_OPTIMIZE_SCENE)) {
        optimizeNextVsyncTime(tracker, nextVsyncTime, now, timing);
        nextWakeupTime = nextVsyncTime - timing.workDuration - timing.readyDuration;
        if(MiSurfaceFlingerStub::isModeChangeOpt()) {
            bool const wouldSkipAVsyncTargetOptimize =
                    mArmedInfo && (nextVsyncTime > (mArmedInfo->mActualVsyncTime + mMinVsyncDistance));
            bool const wouldSkipAWakeupOptimize =
                    mArmedInfo && ((nextWakeupTime > (mArmedInfo->mActualWakeupTime + mMinVsyncDistance)));
            if (wouldSkipAVsyncTargetOptimize && wouldSkipAWakeupOptimize) {
                return getExpectedCallbackTime(nextVsyncTime, timing);
            }
        }
    }
#endif

    auto const nextReadyTime = nextVsyncTime - timing.readyDuration;
    mScheduleTiming = timing;
    mArmedInfo = {nextWakeupTime, nextVsyncTime, nextReadyTime};
    return getExpectedCallbackTime(nextVsyncTime, timing);
}

void VSyncDispatchTimerQueueEntry::addPendingWorkloadUpdate(VSyncDispatch::ScheduleTiming timing) {
    mWorkloadUpdateInfo = timing;
}

bool VSyncDispatchTimerQueueEntry::hasPendingWorkloadUpdate() const {
    return mWorkloadUpdateInfo.has_value();
}

void VSyncDispatchTimerQueueEntry::update(VSyncTracker& tracker, nsecs_t now) {
    if (!mArmedInfo && !mWorkloadUpdateInfo) {
        return;
    }

    if (mWorkloadUpdateInfo) {
        mScheduleTiming = *mWorkloadUpdateInfo;
        mWorkloadUpdateInfo.reset();
    }

    const auto earliestReadyBy = now + mScheduleTiming.workDuration + mScheduleTiming.readyDuration;
    const auto earliestVsync = std::max(earliestReadyBy, mScheduleTiming.earliestVsync);
#ifdef MI_FEATURE_ENABLE
    if (MiSurfaceFlingerStub::isSceneExist(VSYNC_OPTIMIZE_SCENE)) {
        // Not allow to overwrite sf/app armed info to avoid
        // the loss of VSYNC-sf/app events.
        if (mArmedInfo && (mName == "sf" || mName == "app"))
            return;

    }
    auto nextVsyncTime = tracker.nextAnticipatedVSyncTimeFrom(earliestVsync);
    if (MiSurfaceFlingerStub::isSceneExist(VSYNC_OPTIMIZE_SCENE)) {
        optimizeNextVsyncTime(tracker, nextVsyncTime, now, mScheduleTiming);
    }

#else
    const auto nextVsyncTime = tracker.nextAnticipatedVSyncTimeFrom(earliestVsync);
#endif
    const auto nextReadyTime = nextVsyncTime - mScheduleTiming.readyDuration;
    const auto nextWakeupTime = nextReadyTime - mScheduleTiming.workDuration;

    mArmedInfo = {nextWakeupTime, nextVsyncTime, nextReadyTime};
}

void VSyncDispatchTimerQueueEntry::disarm() {
    mArmedInfo.reset();
}

nsecs_t VSyncDispatchTimerQueueEntry::executing() {
    mLastDispatchTime = mArmedInfo->mActualVsyncTime;

#ifdef MI_FEATURE_ENABLE
    if (MiSurfaceFlingerStub::isSceneExist(VSYNC_OPTIMIZE_SCENE)) {
        mLastWakeupTime = mArmedInfo->mActualWakeupTime;
    }
#endif

    disarm();
    return *mLastDispatchTime;
}

void VSyncDispatchTimerQueueEntry::callback(nsecs_t vsyncTimestamp, nsecs_t wakeupTimestamp,
                                            nsecs_t deadlineTimestamp) {
    {
        std::lock_guard<std::mutex> lk(mRunningMutex);
        mRunning = true;
    }

    mCallback(vsyncTimestamp, wakeupTimestamp, deadlineTimestamp);

    std::lock_guard<std::mutex> lk(mRunningMutex);
    mRunning = false;
    mCv.notify_all();
}

void VSyncDispatchTimerQueueEntry::ensureNotRunning() {
    std::unique_lock<std::mutex> lk(mRunningMutex);
    mCv.wait(lk, [this]() REQUIRES(mRunningMutex) { return !mRunning; });
}

void VSyncDispatchTimerQueueEntry::dump(std::string& result) const {
    std::lock_guard<std::mutex> lk(mRunningMutex);
    std::string armedInfo;
    if (mArmedInfo) {
        StringAppendF(&armedInfo,
                      "[wake up in %.2fms deadline in %.2fms for vsync %.2fms from now]",
                      (mArmedInfo->mActualWakeupTime - systemTime()) / 1e6f,
                      (mArmedInfo->mActualReadyTime - systemTime()) / 1e6f,
                      (mArmedInfo->mActualVsyncTime - systemTime()) / 1e6f);
    }

    StringAppendF(&result, "\t\t%s: %s %s\n", mName.c_str(),
                  mRunning ? "(in callback function)" : "", armedInfo.c_str());
    StringAppendF(&result,
                  "\t\t\tworkDuration: %.2fms readyDuration: %.2fms earliestVsync: %.2fms relative "
                  "to now\n",
                  mScheduleTiming.workDuration / 1e6f, mScheduleTiming.readyDuration / 1e6f,
                  (mScheduleTiming.earliestVsync - systemTime()) / 1e6f);

    if (mLastDispatchTime) {
        StringAppendF(&result, "\t\t\tmLastDispatchTime: %.2fms ago\n",
                      (systemTime() - *mLastDispatchTime) / 1e6f);
    } else {
        StringAppendF(&result, "\t\t\tmLastDispatchTime unknown\n");
    }
}

#ifdef MI_FEATURE_ENABLE
void VSyncDispatchTimerQueueEntry::optimizeNextVsyncTime(const VSyncTracker& tracker,
                                                         nsecs_t& nextVsyncTime,
                                                         nsecs_t now,
                                                         const VSyncDispatch::ScheduleTiming& timing) {
    /* Only for VSYNC-sf and VSYNC-app */
    if (mName != "sf" && mName != "app")
        return;

    const nsecs_t period = tracker.currentPeriod();
    if (mLastDispatchTime && mLastWakeupTime && period > 0) {
        const nsecs_t minVsyncDistance = nsecs_t(period * 0.85);
        const nsecs_t maxVsyncDistance = 2 * period - minVsyncDistance;
        const int64_t skipVsyncCount = now > *mLastWakeupTime ?
            (now - *mLastWakeupTime) / period : 0;
        const nsecs_t lastDispatchTime = skipVsyncCount < 3 ? *mLastDispatchTime +
            skipVsyncCount * period : *mLastWakeupTime;
        nsecs_t distance = nextVsyncTime - lastDispatchTime;
        /* Limit the error range of vsync to 0.15 period. */
        if (distance < minVsyncDistance) {
            nextVsyncTime =
                tracker.nextAnticipatedVSyncTimeFrom(lastDispatchTime + minVsyncDistance);
            distance = nextVsyncTime - lastDispatchTime;
        }
        if (distance > maxVsyncDistance &&
                distance < (period + minVsyncDistance)) {
            nextVsyncTime = lastDispatchTime + maxVsyncDistance;
            distance = nextVsyncTime - lastDispatchTime;
        }

        /* Generate vsync immediately for the case that vsync
         * request just missed the last vsync time point within
         * 0.15 period, to improve fluency. */
        auto const nowToLastWakeupTime = now - *mLastWakeupTime;
        auto const nextWakeupTime = nextVsyncTime - timing.workDuration - timing.readyDuration;
        if (nowToLastWakeupTime > minVsyncDistance &&
            (((nowToLastWakeupTime % period) > minVsyncDistance &&
            ((nextWakeupTime - now) > (period - minVsyncDistance))) ||
            (nowToLastWakeupTime % period) < (period - minVsyncDistance))) {
            nextVsyncTime = now + timing.workDuration + timing.readyDuration;
        }
    }
}
#endif

VSyncDispatchTimerQueue::VSyncDispatchTimerQueue(std::unique_ptr<TimeKeeper> tk,
                                                 VSyncTracker& tracker, nsecs_t timerSlack,
                                                 nsecs_t minVsyncDistance)
      : mTimeKeeper(std::move(tk)),
        mTracker(tracker),
        mTimerSlack(timerSlack),
        mMinVsyncDistance(minVsyncDistance) {}

VSyncDispatchTimerQueue::~VSyncDispatchTimerQueue() {
    std::lock_guard lock(mMutex);
    cancelTimer();
}

void VSyncDispatchTimerQueue::cancelTimer() {
    mIntendedWakeupTime = kInvalidTime;
    mTimeKeeper->alarmCancel();
}

void VSyncDispatchTimerQueue::setTimer(nsecs_t targetTime, nsecs_t /*now*/) {
    mIntendedWakeupTime = targetTime;
    mTimeKeeper->alarmAt(std::bind(&VSyncDispatchTimerQueue::timerCallback, this),
                         mIntendedWakeupTime);
    mLastTimerSchedule = mTimeKeeper->now();
}

void VSyncDispatchTimerQueue::rearmTimer(nsecs_t now) {
    rearmTimerSkippingUpdateFor(now, mCallbacks.end());
}

void VSyncDispatchTimerQueue::rearmTimerSkippingUpdateFor(
        nsecs_t now, CallbackMap::iterator const& skipUpdateIt) {
    std::optional<nsecs_t> min;
    std::optional<nsecs_t> targetVsync;
    std::optional<std::string_view> nextWakeupName;
    for (auto it = mCallbacks.begin(); it != mCallbacks.end(); it++) {
        auto& callback = it->second;
        if (!callback->wakeupTime() && !callback->hasPendingWorkloadUpdate()) {
            continue;
        }

        if (it != skipUpdateIt) {
            callback->update(mTracker, now);
        }
        auto const wakeupTime = *callback->wakeupTime();
        if (!min || *min > wakeupTime) {
            nextWakeupName = callback->name();
            min = wakeupTime;
            targetVsync = callback->targetVsync();
        }
    }

    if (min && min < mIntendedWakeupTime) {
#ifdef MI_FEATURE_ENABLE
        if (MiSurfaceFlingerStub::isSceneExist(VSYNC_OPTIMIZE_SCENE)) {
            if (*min < now + mTimerSlack)
                min = now + mTimerSlack;
        }
#endif
        if (ATRACE_ENABLED() && nextWakeupName && targetVsync) {
            ftl::Concat trace(ftl::truncated<5>(*nextWakeupName), " alarm in ", ns2us(*min - now),
                              "us; VSYNC in ", ns2us(*targetVsync - now), "us");
            ATRACE_NAME(trace.c_str());
        }
        setTimer(*min, now);
    } else {
        ATRACE_NAME("cancel timer");
        cancelTimer();
    }
}

void VSyncDispatchTimerQueue::timerCallback() {
    struct Invocation {
        std::shared_ptr<VSyncDispatchTimerQueueEntry> callback;
        nsecs_t vsyncTimestamp;
        nsecs_t wakeupTimestamp;
        nsecs_t deadlineTimestamp;
    };
    std::vector<Invocation> invocations;
    {
        std::lock_guard lock(mMutex);
        auto const now = mTimeKeeper->now();
        mLastTimerCallback = now;
        for (auto it = mCallbacks.begin(); it != mCallbacks.end(); it++) {
            auto& callback = it->second;
            auto const wakeupTime = callback->wakeupTime();
            if (!wakeupTime) {
                continue;
            }

#ifdef MI_FEATURE_ENABLE
        if (MiSurfaceFlingerStub::isSceneExist(VSYNC_OPTIMIZE_SCENE)) {
            if (*wakeupTime > (now + mTimerSlack)) {
                continue;
            }
        }
#endif
            auto const readyTime = callback->readyTime();

            auto const lagAllowance = std::max(now - mIntendedWakeupTime, static_cast<nsecs_t>(0));
            if (*wakeupTime < mIntendedWakeupTime + mTimerSlack + lagAllowance) {
                callback->executing();
                invocations.emplace_back(Invocation{callback, *callback->lastExecutedVsyncTarget(),
                                                    *wakeupTime, *readyTime});
            }
        }

        mIntendedWakeupTime = kInvalidTime;
        rearmTimer(mTimeKeeper->now());
    }

    for (auto const& invocation : invocations) {
        invocation.callback->callback(invocation.vsyncTimestamp, invocation.wakeupTimestamp,
                                      invocation.deadlineTimestamp);
    }
}

VSyncDispatchTimerQueue::CallbackToken VSyncDispatchTimerQueue::registerCallback(
        Callback callback, std::string callbackName) {
    std::lock_guard lock(mMutex);
    return CallbackToken{
            mCallbacks
                    .emplace(++mCallbackToken,
                             std::make_shared<VSyncDispatchTimerQueueEntry>(std::move(callbackName),
                                                                            std::move(callback),
                                                                            mMinVsyncDistance))
                    .first->first};
}

void VSyncDispatchTimerQueue::unregisterCallback(CallbackToken token) {
    std::shared_ptr<VSyncDispatchTimerQueueEntry> entry = nullptr;
    {
        std::lock_guard lock(mMutex);
        auto it = mCallbacks.find(token);
        if (it != mCallbacks.end()) {
            entry = it->second;
            mCallbacks.erase(it);
        }
    }

    if (entry) {
        entry->ensureNotRunning();
    }
}

ScheduleResult VSyncDispatchTimerQueue::schedule(CallbackToken token,
                                                 ScheduleTiming scheduleTiming) {
    ScheduleResult result;
    {
        std::lock_guard lock(mMutex);

        auto it = mCallbacks.find(token);
        if (it == mCallbacks.end()) {
            return result;
        }
        auto& callback = it->second;
        auto const now = mTimeKeeper->now();

        /* If the timer thread will run soon, we'll apply this work update via the callback
         * timer recalculation to avoid cancelling a callback that is about to fire. */
        auto const rearmImminent = now > mIntendedWakeupTime;
        if (CC_UNLIKELY(rearmImminent)) {
            callback->addPendingWorkloadUpdate(scheduleTiming);
            return getExpectedCallbackTime(mTracker, now, scheduleTiming);
        }

        result = callback->schedule(scheduleTiming, mTracker, now);
        if (!result.has_value()) {
            return result;
        }

        if (callback->wakeupTime() < mIntendedWakeupTime - mTimerSlack) {
            rearmTimerSkippingUpdateFor(now, it);
        }
    }

    return result;
}

CancelResult VSyncDispatchTimerQueue::cancel(CallbackToken token) {
    std::lock_guard lock(mMutex);

    auto it = mCallbacks.find(token);
    if (it == mCallbacks.end()) {
        return CancelResult::Error;
    }
    auto& callback = it->second;

    auto const wakeupTime = callback->wakeupTime();
    if (wakeupTime) {
        callback->disarm();

        if (*wakeupTime == mIntendedWakeupTime) {
            mIntendedWakeupTime = kInvalidTime;
            rearmTimer(mTimeKeeper->now());
        }
        return CancelResult::Cancelled;
    }
    return CancelResult::TooLate;
}

void VSyncDispatchTimerQueue::dump(std::string& result) const {
    std::lock_guard lock(mMutex);
    StringAppendF(&result, "\tTimer:\n");
    mTimeKeeper->dump(result);
    StringAppendF(&result, "\tmTimerSlack: %.2fms mMinVsyncDistance: %.2fms\n", mTimerSlack / 1e6f,
                  mMinVsyncDistance / 1e6f);
    StringAppendF(&result, "\tmIntendedWakeupTime: %.2fms from now\n",
                  (mIntendedWakeupTime - mTimeKeeper->now()) / 1e6f);
    StringAppendF(&result, "\tmLastTimerCallback: %.2fms ago mLastTimerSchedule: %.2fms ago\n",
                  (mTimeKeeper->now() - mLastTimerCallback) / 1e6f,
                  (mTimeKeeper->now() - mLastTimerSchedule) / 1e6f);
    StringAppendF(&result, "\tCallbacks:\n");
    for (const auto& [token, entry] : mCallbacks) {
        entry->dump(result);
    }
}

VSyncCallbackRegistration::VSyncCallbackRegistration(VSyncDispatch& dispatch,
                                                     VSyncDispatch::Callback callback,
                                                     std::string callbackName)
      : mDispatch(dispatch),
        mToken(dispatch.registerCallback(std::move(callback), std::move(callbackName))),
        mValidToken(true) {}

VSyncCallbackRegistration::VSyncCallbackRegistration(VSyncCallbackRegistration&& other)
      : mDispatch(other.mDispatch),
        mToken(std::move(other.mToken)),
        mValidToken(std::move(other.mValidToken)) {
    other.mValidToken = false;
}

VSyncCallbackRegistration& VSyncCallbackRegistration::operator=(VSyncCallbackRegistration&& other) {
    mDispatch = std::move(other.mDispatch);
    mToken = std::move(other.mToken);
    mValidToken = std::move(other.mValidToken);
    other.mValidToken = false;
    return *this;
}

VSyncCallbackRegistration::~VSyncCallbackRegistration() {
    if (mValidToken) mDispatch.get().unregisterCallback(mToken);
}

ScheduleResult VSyncCallbackRegistration::schedule(VSyncDispatch::ScheduleTiming scheduleTiming) {
    if (!mValidToken) {
        return std::nullopt;
    }
    return mDispatch.get().schedule(mToken, scheduleTiming);
}

CancelResult VSyncCallbackRegistration::cancel() {
    if (!mValidToken) {
        return CancelResult::Error;
    }
    return mDispatch.get().cancel(mToken);
}

} // namespace android::scheduler
