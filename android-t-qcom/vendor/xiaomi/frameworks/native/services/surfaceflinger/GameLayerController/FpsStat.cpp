/*
* @Copyright [2021] <wangyue25@xiaomi.com>
*/
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "FpsStat.h"
#include <cutils/properties.h>
#include <math.h>
#include <log/log.h>
#include <memory>
#include <utility>
#include <string>

using android::os::PersistableBundle;

namespace android {

namespace gamelayercontroller {

static bool debugFpsStat() {
    char value[PROPERTY_VALUE_MAX] = {};
    property_get("debug.sf.fpsstat", value, "false");
    return std::string(value) == "true";
}

static sp<IBinder> getJoyoseService() {
    sp<IServiceManager> sm = defaultServiceManager();
    return sm->getService(String16("xiaomi.joyose"));
}

void FpsStat::analysisFpsStat() {
    int i, minFps, maxFps, totalTime, tmpCount, halfCount, totalDev;
    int flg;

    if (mEnabled.load())
        return;
    ATRACE_CALL();
    for (i = 0, minFps = -1, maxFps = -1, totalTime = 0, totalDev = 0;
            i < MAX_NUM_FRAME_TIMES; i++) {
        if (mFpsStats[i] > 0) {
            if (minFps == -1)
                minFps = i;
            maxFps = i;
            totalTime += mFpsStats[i] * i;
            mTotalFpsCounts += mFpsStats[i];
            if (i < mThresh1)
                totalDev += abs(mTargetFps - i) * mFpsStats[i];
            if (DBG & isDebug) ALOGD("[FpsStat] fps: %d (%d times)", i, mFpsStats[i]);
        }
    }
    mMinFps = (minFps == -1 ? 0 : minFps);
    mMaxFps = (maxFps == -1 ? 0 : maxFps);
    // compute mid fps
    flg = mTotalFpsCounts % 2;
    halfCount = mTotalFpsCounts / 2;
    tmpCount = 0;
    for (i = mMinFps; i <= mMaxFps; i++) {
        tmpCount += mFpsStats[i];
        if (flg && tmpCount > halfCount) {
            mMiddleFps = static_cast<float>(i);
            break;
        } else if (!flg) {
            if (tmpCount == halfCount) {
                mMiddleFps = static_cast<float>(2 * i + 1) / 2;
                break;
            } else if (tmpCount > halfCount) {
                mMiddleFps = static_cast<float>(i);
                break;
            }
        }
    }
    mAverageFps = static_cast<float>(totalTime) / mTotalFpsCounts;
    // judge is or not bad fps
    if (mAverageFps < mThresh1) {
        float tmpThresh2 = static_cast<float>(totalDev) / mTotalFpsCounts;
        if (tmpThresh2 > mThresh2) {
            isBadFrame = true;
            if (DBG & isDebug) ALOGD("[FpsStat] occured critical stuck and Thresh1:%d, thresh1:%.2f, Thresh2:%d, thresh2:%.2f",
                mThresh1, mAverageFps, mThresh2, tmpThresh2);
        }
    }
}

void FpsStat::enable(bool enable) {
    int i;

    if (enable) {
        isBadFrame = false;
        mLastFps = -1;
        mTotalFpsCounts = 0;
        mMinFps = 0;
        mMaxFps = 0;
        mAverageFps = 0.0;
        mMiddleFps = 0.0;
        mPauseCounts = 0;
        mJitters = 0;
        mStableFpsCounts = 0;
        mStartTime = systemTime(SYSTEM_TIME_MONOTONIC);
        mOffset = 0;
        for (i = 0; i < MAX_NUM_FRAME_TIMES; i++) {
            mFrameTimes[i] = 0;
            mFrameFenceTimes[i] = nullptr;
            mFpsStats[i] = 0;
        }
        startMonitorThread();
        mEnabled.store(true);
    } else {
        mEnabled.store(false);
        analysisFpsStat();
        mStableFpsStandard = 0;
        mStableFpsBias = 0;
        mContinuousJitterMaxFps = 0;
        mContinuousJitterMaxPeriods = 0;
        mCurrContinuousJitters = 0;
        stopMonitorThread();
    }
}

bool FpsStat::isEnabled() const {
    return mEnabled.load();
}

// before enable
void FpsStat::setBadFpsStandards(int targetFps, int thresh1, int thresh2) {
    if (mEnabled.load())
        return;
    mTargetFps = targetFps;
    mThresh1 = thresh1;
    mThresh2 = thresh2;
}

void FpsStat::setStabilityStandard(int standard, int bias) {
    if (mEnabled.load())
        return;
    mStableFpsStandard = standard;
    mStableFpsBias = bias;
}

void FpsStat::setContinuousJitterStandard(int maxfps, int maxperiods) {
    if (mEnabled.load())
        return;
    mContinuousJitterMaxFps = maxfps;
    mContinuousJitterMaxPeriods = maxperiods;
}

int FpsStat::currentMonitorThreadState() {
    std::unique_lock<std::mutex> lock(mFrameMonitorMutex);
    return mMonitorThreadState;
}

void FpsStat::createMonitorThread(int timeout) {
    mJoyoseService = getJoyoseService();
    {
        std::unique_lock<std::mutex> lock(mFrameMonitorMutex);
        if (mMonitorThreadState == MONITOR_STATE_DEAD) {
            if (DBG & isDebug) ALOGD("[FpsStat] create thread");
            mMonitorThreadState = MONITOR_STATE_RUNABLE;
            mFrameMonitorThread = std::thread(&FpsStat::threadloop,
                this, timeout);
        }
    }
}

void FpsStat::distroyMonitorThread() {
    bool killable = false;
    {
        std::unique_lock<std::mutex> lock(mFrameMonitorMutex);
        if (mMonitorThreadState != MONITOR_STATE_DEAD) {
            if (DBG & isDebug) ALOGD("[FpsStat] distroy thread");
            killable = true;
            mMonitorThreadState = MONITOR_STATE_DEAD;
            mFrameMonitorCV.notify_one();
        }
    }
    if (killable && mFrameMonitorThread.joinable())
        mFrameMonitorThread.join();

    if (killable) {
        /*
        ALOGI("[FpsStat] fps monitor thread [%s] exit successfully!",
            mLayerName.string());
        */
        ALOGI("[FpsStat] fps monitor thread exit successfully!");
    }
}

void FpsStat::startMonitorThread() {
    std::unique_lock<std::mutex> lock(mFrameMonitorMutex);
    if (mMonitorThreadState == MONITOR_STATE_RUNABLE) {
        if (DBG & isDebug) ALOGD("[FpsStat] start thread");
        mMonitorThreadState = MONITOR_STATE_RUNNING;
        mFrameMonitorCV.notify_one();
    }
}

void FpsStat::stopMonitorThread() {
    std::unique_lock<std::mutex> lock(mFrameMonitorMutex);
    if (mMonitorThreadState == MONITOR_STATE_RUNNING) {
        if (DBG & isDebug) ALOGD("[FpsStat] stop thread");
        mMonitorThreadState = MONITOR_STATE_RUNABLE;
        mFrameMonitorCV.notify_one();
    }
}

void FpsStat::touchMonitorThread() {
    std::unique_lock<std::mutex> lock(mFrameMonitorMutex);
    if (mMonitorThreadState == MONITOR_STATE_RUNNING) {
        mFrameMonitorCV.notify_one();
    }
}

void FpsStat::threadloop(int timeout) {
    bool notified = false;
    bool failed = false;

    pthread_setname_np(pthread_self(), "FpsMonitor");
    std::unique_lock<std::mutex> lock(mFrameMonitorMutex);
    while (mMonitorThreadState != MONITOR_STATE_DEAD) {
        if (mMonitorThreadState == MONITOR_STATE_RUNABLE) {
            mFrameMonitorCV.wait(lock);
        } else if (mMonitorThreadState == MONITOR_STATE_RUNNING) {
            if (mFrameMonitorCV.wait_for(lock,
                    std::chrono::milliseconds(timeout))
                    == std::cv_status::timeout) {
                if (!notified && !failed) {
                    if (DBG & isDebug) ALOGD("[FpsStat] long frame emergency!");
                    failed = notifyJoyose();
                    notified = true;
                }
            } else {
                notified = false;
            }
        }
    }
}

bool FpsStat::notifyJoyose() {
    bool failed = false;

    if (CC_LIKELY(mJoyoseService != NULL)) {
        Parcel data, reply;
        PersistableBundle bundle;
        uint32_t code = IBinder::FIRST_CALL_TRANSACTION + 13;

        data.writeInterfaceToken(
            String16("com.xiaomi.joyose.IJoyoseInterface"));
        data.writeString16(String16("notifyLongFrameSF"));
        data.writeParcelable(bundle);
        status_t ret = mJoyoseService->transact(code, data, &reply,
            IBinder::FLAG_ONEWAY);
        if (ret != NO_ERROR) {
            ALOGE("[FpsStat] call js method failed");
            failed = true;
        }
    } else {
        ALOGE("[FpsStat] connect to js failed");
        failed = true;
    }
    return failed;
}

void FpsStat::updateStartTime(nsecs_t time) {
    int fps;
    nsecs_t tmpTime;
    bool flg;

    tmpTime = mStartTime;
    flg = false;
    while (time > tmpTime + NANOS_PER_SEC) {
        tmpTime += NANOS_PER_SEC;
        fps = currentFps(tmpTime);
        if (DBG & isDebug) ALOGI("[FpsStat] current fps: %d", fps);
        mFpsStats[fps] += 1;
        if (fps == 0 && mLastFps != 0)
            mPauseCounts++;
        if (mLastFps != -1)
            mJitters += abs(mLastFps - fps);
        if (mStableFpsStandard != 0 &&
                abs(fps - mStableFpsStandard) <= mStableFpsBias)
            mStableFpsCounts++;
        if (fps <= mContinuousJitterMaxFps) {
            mCurrContinuousJitters++;
            // onUrgentJitters() be called only once
            if (mCurrContinuousJitters == mContinuousJitterMaxPeriods) {
                onUrgentJitters();
            }
        } else {
            mCurrContinuousJitters = 0;
        }

        mLastFps = fps;
    }
    mStartTime = tmpTime;
}

int FpsStat::currentFps(nsecs_t time) {
    int fps = 0;
    int i;

    ATRACE_CALL();
    i = (mOffset + 1) % MAX_NUM_FRAME_TIMES;
    if (isPresentFenceSupport) {
        while (i != mOffset) {
            if (mFrameFenceTimes[i] != nullptr &&
                    mFrameFenceTimes[i]->getSignalTime() ==
                    Fence::SIGNAL_TIME_INVALID) {
                mFrameFenceTimes[i] = nullptr;
                ALOGE("[FpsStat] invalid signal time");
            } else if (mFrameFenceTimes[i] != nullptr &&
                    mFrameFenceTimes[i]->getSignalTime() < time) {
                mFrameFenceTimes[i] = nullptr;
                fps++;
            }
            i = (i + 1) % MAX_NUM_FRAME_TIMES;
        }
    } else {
        while (i != mOffset) {
            if (mFrameTimes[i] != 0 && mFrameTimes[i] < time) {
                mFrameTimes[i] = 0;
                fps++;
            }
            i = (i + 1) % MAX_NUM_FRAME_TIMES;
        }
    }
    return fps;
}

void FpsStat::advanceFrame() {
    touchMonitorThread();
    mOffset = (mOffset + 1) % MAX_NUM_FRAME_TIMES;
}

void FpsStat::insertFrameFenceTime(const std::shared_ptr<FenceTime>& fence) {
    nsecs_t current;

    if (!mEnabled.load())
        return;
    ATRACE_CALL();
    if (!isPresentFenceSupport)
        isPresentFenceSupport = true;
    current = systemTime(SYSTEM_TIME_MONOTONIC);
    updateStartTime(current);
    mFrameFenceTimes[mOffset] = std::move(fence);
    advanceFrame();
}

void FpsStat::insertFrameTime(nsecs_t time) {
    nsecs_t current;

    if (!mEnabled.load())
        return;
    ATRACE_CALL();
    if (isPresentFenceSupport)
        isPresentFenceSupport = false;
    current = systemTime(SYSTEM_TIME_MONOTONIC);
    updateStartTime(current);
    mFrameTimes[mOffset] = time;
    advanceFrame();
}

// todo notify joyose
void FpsStat::onUrgentJitters() {
    return;
}

double FpsStat::getLowFpsRate(float threshold) const {
    int i, lowFpsCounts;

    if (mEnabled.load())
        return 0;
    if (threshold < 0)
        return 0;
    if (threshold > MAX_NUM_FRAME_TIMES)
        threshold = MAX_NUM_FRAME_TIMES;
    for (i = 0, lowFpsCounts = 0; i < MAX_NUM_FRAME_TIMES; i++) {
        if (i < threshold)
            lowFpsCounts += mFpsStats[i];
    }
    return static_cast<double>(lowFpsCounts) / mTotalFpsCounts;
}

double FpsStat::getLowFpsRate() const {
    float threshold = mMiddleFps * (float)0.9;
    return getLowFpsRate(threshold);
}

float FpsStat::getAverageFps() const {
    if (mEnabled.load())
        return 0.0;
    return mAverageFps;
}

float FpsStat::getMiddleFps() const {
    if (mEnabled.load())
        return 0.0;
    return mMiddleFps;
}

double FpsStat::getJitterRate() const {
    if (mEnabled.load())
        return 0;
    if (DBG & isDebug)
        ALOGD("[FpsStat] jitters:%d, total_fps:%d", mJitters, mTotalFpsCounts);
    return static_cast<double>(mJitters) / (mTotalFpsCounts - 1);
}

double FpsStat::getStabilityRate() {
    int i;
    if (mEnabled.load())
        return 0;
    if (mStableFpsStandard == 0) {
        // if didn't config stable standard
        mStableFpsCounts = 0;
        for (i = 0; i < MAX_NUM_FRAME_TIMES; i++) {
            if (fabs(mMiddleFps - i) <= 1.0) {
                mStableFpsCounts += mFpsStats[i];
            }
        }
    }
    return static_cast<double>(mStableFpsCounts) / mTotalFpsCounts;
}

double FpsStat::getFpsStDev() const {
    int i;
    float diff, variance;

    for (i = 0, variance = 0.0; i < MAX_NUM_FRAME_TIMES; i++) {
        if (mFpsStats[i] > 0) {
            diff = fabs(mAverageFps - i);
            variance += mFpsStats[i] * (float)pow(diff, 2);
        }
    }
    return sqrt(static_cast<double>(variance) / mTotalFpsCounts);
}

int FpsStat::getPauseCounts() const {
    return mPauseCounts;
}

bool FpsStat::getIsBadFrame() const {
    return isBadFrame;
}

String16 FpsStat::getBadFrameDis() const {
    String16 res = String16("");
    int i, j;
    int num = 0;
    for (i=0,j=1; i < mThresh1; i++, j++) {
        if (mFpsStats[i] > 0) {
            num += mFpsStats[i];
            if (DBG & isDebug) ALOGD("[FpsStat] mFpsStats[%d] : %d", i, mFpsStats[i]);
        }
        // 帧率分布以20为一档
        if (j%20 == 0) {
            std::string tmp = std::to_string(num) + ",";
            res += String16(tmp.c_str());
            num = 0;
            j = 0;
        }
    }
    //计算最后一档区间不足20的分布
    if (num != 0) {
        std::string last = std::to_string(num);
        res += String16(last.c_str());
    }
    if (DBG & isDebug) ALOGD("[FpsStat] getBadFrameDis : %s", String8(res).c_str());
    return res;
}

FpsStat::FpsStat() {
    mMonitorThreadState = MONITOR_STATE_DEAD;
    isDebug = debugFpsStat();
    if (DBG & DBG_FPSSTAT) ALOGD("[FpsStat] construct...");
}

FpsStat::~FpsStat() {
    distroyMonitorThread();
    if (DBG & DBG_FPSSTAT) ALOGD("[FpsStat] distroy...");
}

}  // namespace gamelayercontroller

}  // namespace android