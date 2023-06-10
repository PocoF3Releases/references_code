/*
* @Copyright [2021] <wangyue25@xiaomi.com>
*/

#ifndef SERVICES_SURFACEFLINGER_FPSSTAT_H_
#define SERVICES_SURFACEFLINGER_FPSSTAT_H_

#define DBG                     1
#define DBG_FPSSTAT             1

#define NANOS_PER_SEC           1000000000
#define MONITOR_STATE_DEAD      0
#define MONITOR_STATE_RUNABLE   1
#define MONITOR_STATE_RUNNING   2
#ifndef MAX_NUM_FRAME_TIMES
#define MAX_NUM_FRAME_TIMES     128
#endif

#include <utils/Timers.h>
#include <utils/Trace.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <cutils/atomic.h>
#include <ui/FenceTime.h>

#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/PersistableBundle.h>
#include <binder/Parcel.h>

#include <memory>
#include <thread>
#include <mutex>

namespace android {

namespace gamelayercontroller {

class FpsStat {
 public:
    explicit FpsStat();
    ~FpsStat();
    void enable(bool enable);
    bool isEnabled() const;
    void insertFrameFenceTime(const std::shared_ptr<FenceTime>& fence);
    void insertFrameTime(nsecs_t time);
    // before enable
    void setBadFpsStandards(int targetFps, int thresh1, int thresh2);
    void setStabilityStandard(int standard, int bias);
    void setContinuousJitterStandard(int maxfps, int maxperiods);
    // after disable
    double getLowFpsRate() const;
    double getLowFpsRate(float threshold) const;
    float getAverageFps() const;
    float getMiddleFps() const;
    double getJitterRate() const;
    double getStabilityRate();
    double getFpsStDev() const;
    int getPauseCounts() const;
    int getMinFps() const;
    int getMaxFps() const;
    bool getIsBadFrame() const;
    String16 getBadFrameDis() const;
    // called before any operation
    void createMonitorThread(int timeout);
    // called after all operations
    void distroyMonitorThread();
    int currentMonitorThreadState();

 private:
    void analysisFpsStat();
    void updateStartTime(nsecs_t time);
    int currentFps(nsecs_t time);
    void updateFrameTimes(nsecs_t time);
    void advanceFrame();
    void onUrgentJitters();
    void threadloop(int timeout);
    void startMonitorThread();
    void stopMonitorThread();
    void touchMonitorThread();
    // notify joyose
    bool notifyJoyose();

    //String8 mLayerName;
    std::atomic<bool> mEnabled = false;
    nsecs_t mFrameTimes[MAX_NUM_FRAME_TIMES];
    std::shared_ptr<FenceTime> mFrameFenceTimes[MAX_NUM_FRAME_TIMES];
    bool isPresentFenceSupport = false;
    bool isBadFrame = false;
    nsecs_t mStartTime = 0;
    int mOffset = 0;
    int mFpsStats[MAX_NUM_FRAME_TIMES];
    int mLastFps = -1;
    int mTotalFpsCounts = 0;
    float mMiddleFps = 0.0;
    int mMinFps = 0;
    int mMaxFps = 0;
    float mAverageFps = 0.0;
    int mPauseCounts = 0;
    int mJitters = 0;

    int mTargetFps = -1;
    int mThresh1 = -1;
    int mThresh2 = -1;
    int mStableFpsStandard = 0;
    int mStableFpsBias = 0;
    int mStableFpsCounts = 0;
    int mContinuousJitterMaxFps = 0;
    int mContinuousJitterMaxPeriods = 0;
    int mCurrContinuousJitters = 0;

    std::thread mFrameMonitorThread;
    std::mutex mFrameMonitorMutex;
    std::condition_variable mFrameMonitorCV;
    int mMonitorThreadState;

    sp<IBinder> mJoyoseService;
    bool isDebug;
};

}  // namespace gamelayercontroller

}  // namespace android

#endif  // SERVICES_SURFACEFLINGER_FPSSTAT_H