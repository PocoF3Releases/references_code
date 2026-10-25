/*
 * Copyright (C) 2020, Xiaomi Inc. All rights reserved.
 *
 */
#include "BacklightMonitorThread.h"
#include <log/log.h>
#include "SurfaceFlinger.h"

namespace android {

BacklightMonitorThread::BacklightMonitorThread():
        Thread(false) {}

status_t BacklightMonitorThread::Start(MiSurfaceFlingerImpl* misf, const sp<SurfaceFlinger>& flinger) {
    mFlinger = flinger;
    mMiFlinger = misf;
    mLastBrightness = mMiFlinger->mSkipIdleThreshold + 1;
    return run("SurfaceFlinger::BacklightMonitorThread", PRIORITY_NORMAL);
}


status_t BacklightMonitorThread::readyToRun() {

    int i = 0;
    char value[256] = {0};
    mPollBlFd[0].fd = open(kBacklightPath, O_RDONLY);
    if (mPollBlFd[0].fd < 0) {
        ALOGE("open backlight node:%s failed", kBacklightPath);
        return -1;
    } else {
        ssize_t ret = read(mPollBlFd[0].fd, value, sizeof(value));
        if (ret) {
            mBrightness = strtoul(value, NULL, 10);
            if (mBrightness && mBrightness < mMiFlinger->mSkipIdleThreshold) {
                mMiFlinger->forceSkipIdleTimer(true);
                mLastBrightness = mBrightness;
                ALOGD("Force skip idle timer[true], mBrightness[%lu]", mBrightness);
            }
        }
    }

    if(mBrightness){
       Mutex::Autolock _l(mMiFlinger->mFlinger->mStateLock);
       mMiFlinger->updateHBMOverlayAlphaLocked(static_cast<int>(mBrightness));
       ALOGD("update brightness mBrightness %lu",mBrightness);
    }

    mPollBlFd[1].fd = open(kDozeStatePath, O_RDONLY);
    if (mPollBlFd[1].fd < 0) {
        ALOGE("open doze state node:%s failed", kDozeStatePath);
        return -1;
    }

    for (i = 0; i < 2; i++) {
        mPollBlFd[i].events = POLLPRI | POLLERR;
    }
    return NO_ERROR;
}

bool BacklightMonitorThread::threadLoop() {

    char buf[16] = "";
    int ret = 0;
    unsigned long brightness = 0;
    unsigned long doze_state = 0;

    ret = poll(mPollBlFd, 2, -1);
    if (ret <= 0) {
        ALOGE("poll failed. error = %s", strerror(errno));
        return true;
    }

    memset(buf, 0x0, sizeof(buf));
    if (mPollBlFd[0].revents & POLLPRI) {
        if (pread(mPollBlFd[0].fd, buf, 16, 0) < 0) {
            ALOGE("pread brightness failed");
            return true;
        }

        brightness = strtoul(buf, NULL, 10);
        mBrightness = brightness;
        {
            Mutex::Autolock _l(mMiFlinger->mFlinger->mStateLock);
            mMiFlinger->updateHBMOverlayAlphaLocked(static_cast<int>(brightness));
        }

        /* disable Idle timer in low-brightness to prevent flicker */
        if (mMiFlinger->mSkipIdleThreshold) {
            if (mBrightness <= mMiFlinger->mSkipIdleThreshold && mLastBrightness > mMiFlinger->mSkipIdleThreshold) {
                ALOGD("Force skip idle timer[true]\n");
                mMiFlinger->forceSkipIdleTimer(true);
            } else if (mBrightness > mMiFlinger->mSkipIdleThreshold && mLastBrightness <= mMiFlinger->mSkipIdleThreshold) {
                ALOGD("Force skip idle timer[false]\n");
                mMiFlinger->forceSkipIdleTimer(false);
            }
        }
        mLastBrightness = mBrightness;
    }

    if (mPollBlFd[1].revents & POLLPRI) {
        if (pread(mPollBlFd[1].fd, buf, 16, 0) < 0) {
            ALOGE("pread doze state failed");
            return true;
        }

        doze_state = strtoul(buf, NULL, 10);
        mMiFlinger->saveDisplayDozeState(doze_state);
    }

    return true;
}

unsigned long BacklightMonitorThread::getCurrentBrightness() {
    return mBrightness;
}

} // namespace android
