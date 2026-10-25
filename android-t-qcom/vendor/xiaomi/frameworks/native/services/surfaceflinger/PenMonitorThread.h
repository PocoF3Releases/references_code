/*
 * Copyright (C) 2020, Xiaomi Inc. All rights reserved.
 *
 */

#ifndef ANDROID_PEN_MONITOR_THREAD_H
#define ANDROID_PEN_MONITOR_THREAD_H

#include <stddef.h>

#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <utils/threads.h>
#include <sys/poll.h>
#include "MiSurfaceFlingerImpl.h"
#include "DisplayFeatureGet.h"

namespace android {

class SurfaceFlinger;
class MiSurfaceFlingerImpl;

class PenMonitorThread : public Thread {
public:
    PenMonitorThread();
    status_t Start(MiSurfaceFlingerImpl* misf, sp<SurfaceFlinger> flinger);
    int getPenStatus();

private:
    virtual bool threadLoop();
    virtual status_t readyToRun();

    static constexpr const char* kSmartpenActiveStatusPath = "/sys/class/himax/himax0/pen_connect_strategy";
    pollfd mPollFd;
    int mPenStatus = 0;

    sp<SurfaceFlinger> mFlinger;
    MiSurfaceFlingerImpl* mMiSurfaceFlingerImpl;
};

}

#endif // ANDROID_PEN_MONITOR_THREAD_H