/*
 * Copyright (C) 2020, Xiaomi Inc. All rights reserved.
 *
 */

#ifndef ANDROID_BACKLIGHT_MONITOR_THREAD_H
#define ANDROID_BACKLIGHT_MONITOR_THREAD_H

#include <stddef.h>

#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <utils/threads.h>
#include <sys/poll.h>
#include "MiSurfaceFlingerImpl.h"

namespace android {

class SurfaceFlinger;
class MiSurfaceFlingerImpl;

class BacklightMonitorThread : public Thread {

public:
    BacklightMonitorThread();
    status_t Start(MiSurfaceFlingerImpl* misf, const sp<SurfaceFlinger>& flinger);
    unsigned long getCurrentBrightness();

private:
    virtual bool threadLoop();
    virtual status_t readyToRun();

    static constexpr const char* kBacklightPath = "/sys/class/backlight/panel0-backlight/brightness_clone";
    static constexpr const char* kDozeStatePath = "/sys/class/drm/card0-DSI-1/doze_brightness";
    pollfd mPollBlFd[2];

    sp<SurfaceFlinger> mFlinger;
    MiSurfaceFlingerImpl* mMiFlinger;
    unsigned long mBrightness;
    unsigned long mLastBrightness;
};

}

#endif // ANDROID_BACKLIGHT_MONITOR_THREAD_H
