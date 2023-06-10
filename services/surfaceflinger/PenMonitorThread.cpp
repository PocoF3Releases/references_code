/*
 * Copyright (C) 2020, Xiaomi Inc. All rights reserved.
 *
 */

//#define LOG_NDEBUG 0	//Open android log
#undef LOG_TAG
#define LOG_TAG "MI-SF\t : PenMonitorThread"

#include "PenMonitorThread.h"
#include <log/log.h>
#include "SurfaceFlinger.h"
#include <sys/poll.h>
#include <sys/inotify.h>
#include <cutils/properties.h>

namespace android {

PenMonitorThread::PenMonitorThread():
        Thread(false) {}

status_t PenMonitorThread::Start(MiSurfaceFlingerImpl* misf, sp<SurfaceFlinger> flinger) {
    mFlinger = flinger;
    mMiSurfaceFlingerImpl = misf;
    return run("SF_PenMonitorThread", PRIORITY_NORMAL);
}

status_t PenMonitorThread::readyToRun() {
    mPollFd.fd = open(kSmartpenActiveStatusPath, O_RDONLY);
    if (mPollFd.fd < 0) {
        ALOGE("Open %s failed.", kSmartpenActiveStatusPath);
        return -1;
    } else {
        ALOGD("Open %s success.", kSmartpenActiveStatusPath);
    }
    mPollFd.events = POLLPRI | POLLERR;
    return NO_ERROR;
}

bool PenMonitorThread::threadLoop() {
    char buf[16] = "";
    int ret = 0;
    char *ptr;
    unsigned long pen_active_status = 0;
    unsigned long charge_active_status = 0;

    ret = poll(&mPollFd, 1, -1);
    if(ret <= 0) {
        ALOGE("Poll %s failed, error = %s", kSmartpenActiveStatusPath, strerror(errno));
        return true;
    }

    if (mPollFd.revents & POLLPRI) {
        memset(buf, 0x0, sizeof(buf));
        if (pread(mPollFd.fd, buf, 16, 0) < 0) {
            ALOGE("Pread pen_active_status first failed.");
            if (pread(mPollFd.fd, buf, 16, 0) < 0) {
                ALOGE("Pread pen_active_status second failed.");
                return true;
            }
        }

        pen_active_status = strtoul(buf, &ptr, 10);
        //ALOGD("[%s] buf = %s, ptr = %s\n", __func__, buf, ptr);
        charge_active_status = strtoul(ptr, NULL, 10);
        ALOGD("pen_active_status = %lu, charge_active_status = %lu\n", pen_active_status, charge_active_status);
        if (pen_active_status) {
            if (!charge_active_status) {
                mPenStatus = 1;
                mMiSurfaceFlingerImpl->forceSkipIdleTimer(true);
                ALOGD("Force skip idle timer[true]");
            } else {
                mPenStatus = 0;
                mMiSurfaceFlingerImpl->forceSkipIdleTimer(false);
                ALOGD("Force skip idle timer[true]");
            }
        } else {
                mPenStatus = 0;
                mMiSurfaceFlingerImpl->forceSkipIdleTimer(false);
                ALOGD("Force skip idle timer[true]");
        }
    }
    return true;
}

int PenMonitorThread::getPenStatus() {
    return mPenStatus;
}

}
