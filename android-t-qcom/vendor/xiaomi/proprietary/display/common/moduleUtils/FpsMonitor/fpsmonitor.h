#ifndef _FPS_MONITOR_H_
#define _FPS_MONITOR_H_
#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <string>
#include <fstream>
#include <utils/Singleton.h>
#include <cutils/properties.h>
#include <unistd.h>
#include <pthread.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include <poll.h>
#include "DisplayFeatureHal.h"

namespace android {
class FpsMonitor : public Singleton<FpsMonitor>
{
public:
    FpsMonitor();
    ~FpsMonitor() {}
    void registerCallback(void* callback);
    void checkFps(int enable);
    void notifyBrightnessUpdate();
    void SetDisplayState(int displayId, int state);
    typedef void (*NOTIFY_LOCK_FPS)(int value);
private:
    static void *ThreadWrapperFpsMonitor(void *);
    static void *ThreadWrapperCheckUpdateStatus(void *);
    int threadFuncFpsMonitor();
    int threadFuncCheckUpdateStatus();
    int registerPollEvent();
    bool getUpdateStatus();
    bool checkBrightnessUpdateInterval();
    static constexpr const char* kDispFeaturePath = "/dev/mi_display/disp_feature";
    pthread_t mThreadFpsMonitor;
    pthread_t mThreadCheckUpdateStatus;
    pollfd mDFPollFd;
    pthread_cond_t mCheckCond;
    pthread_mutex_t mChecklock;
    static NOTIFY_LOCK_FPS mCallback;
    int mCurStatus = 0;
    int mFps[DISPLAY_MAX] = {1000, 1000};
    int mLowFpsSupport[DISPLAY_MAX] = {0};
    int mDispState[DISPLAY_MAX] = {0};
    struct timeval mBlUpdateTime = {0};
};
} //namespace android

#endif
