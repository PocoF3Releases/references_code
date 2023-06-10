/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include "TouchMonitorThread.h"
#include <log/log.h>
#include "SurfaceFlinger.h"
#include <linux/input.h>
#include <sys/poll.h>
#include <sys/inotify.h>
#include <cutils/properties.h>
#include "DisplayFeatureGet.h"

namespace android {

TouchMonitorThread::TouchMonitorThread():
        Thread(false) {}

status_t TouchMonitorThread::Start(MiSurfaceFlingerImpl* misf, sp<SurfaceFlinger> flinger) {
    mFlinger = flinger;
    mMiSurfaceFlingerImpl = misf;
    return run("SF_TouchMonitorThread", PRIORITY_NORMAL);
}

status_t TouchMonitorThread::readyToRun() {
    char devname[255] = {0};
    char *filename;
    struct dirent *dirEntry;
    int32_t fd;
    bool hasForceTouchDev = false;

    mDisplayFeatureGet = new DisplayFeatureGet();
    mCCTNeedCheckTouch = property_get_bool("ro.vendor.mi_sf.cct.need.check.touch.enable", false);
    mCCTNeedCheckTouch |= property_get_bool("ro.vendor.cct.need.check.touch.enable", false);
    ALOGD("mCCTNeedCheckTouch:%d", mCCTNeedCheckTouch);

    if (mMiSurfaceFlingerImpl->mIsLtpoPanel) {
        mPollFd.fd = open(kTouchActiveStatusPath, O_RDONLY);
        if (mPollFd.fd < 0) {
            ALOGE("open %s failed", kTouchActiveStatusPath);
            return -1;
        } else {
            ALOGD("open %s success", kTouchActiveStatusPath);
        }

        mPollFd.events = POLLPRI | POLLERR;

        return NO_ERROR;
    }

    DIR *inputDir = opendir(kInputPath);
    if (inputDir == NULL) {
        ALOGE("open %s failed", kInputPath);
        return -1;
    }

    strcpy(devname, kInputPath);
    filename = devname + strlen(devname);
    *filename++ = '/';

    while ((dirEntry = readdir(inputDir))) {
        if (dirEntry->d_name[0] == '.'
            && (dirEntry->d_name[1] == '\0' || (dirEntry->d_name[1] == '.' && dirEntry->d_name[2] == '\0'))) {
            continue;
        }

        strcpy(filename, dirEntry->d_name);
        ALOGD("[%s] open %s", __func__, devname);
        fd = open(devname, O_RDONLY);
        if (fd < 0) {
            ALOGE("[%s] open %s fail", __func__, devname);
            continue;
        }

        char name[80] = {0};
        if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
            close(fd);
        } else {
            ALOGD("[%s] get %s name=%s", __func__, devname, name);

            if (!strcmp(name, kTouchDeviceNamefts)
                || !strcmp(name, kTouchDeviceNamegfts)
                || !strcmp(name, kTouchDeviceNameffts)) {
                hasForceTouchDev = true;
                strcpy(mForceTouchPath, devname);
                ALOGI("[%s] touch_event_path = %s, %s", __func__, mForceTouchPath, name);
                close(fd);
                break;
            } else {
                close(fd);
            }
        }
    }

    closedir(inputDir);

    if (!hasForceTouchDev) {
        return -1;
    }
#if 0 // inotify
    mPollFd.fd = inotify_init();
    if (mPollFd.fd < 0) {
        ALOGE("Could not initialize inotify fd");
        return NO_INIT;
    }
    mPollFd.events = POLLIN;

    mPollWd = inotify_add_watch(mPollFd.fd, mForceTouchPath, IN_ALL_EVENTS);
    if (mPollWd < 0) {
        close(mPollFd.fd);
        mPollFd.fd = -1;
        ALOGE("Could not add watch for %s", mForceTouchPath);
        return NO_INIT;
    }
#else
    mPollFd.fd = open(mForceTouchPath, O_RDONLY | O_NONBLOCK);
    if (mPollFd.fd < 0) {
        ALOGE("[%s] open %s failed", __func__, mForceTouchPath);
        return NO_INIT;
    }
    mPollFd.events = POLLIN;
    mPollWd = 0;
#endif
    return NO_ERROR;
}

bool TouchMonitorThread::threadLoop() {
    ssize_t size;
    struct input_event *touchEvent = NULL;
    char event_data[MAX_STRING_LEN] = {0};

    if (mMiSurfaceFlingerImpl->mIsLtpoPanel) {
        return LtpothreadLoop();
    }

    //ALOGD("TouchMonitorThread poll");
    ssize_t pollResult = poll(&mPollFd, 1, -1);
    if (pollResult == 0) {
        ALOGE("TouchMonitorThread pollResult == 0");
        return true;
    } else if (pollResult < 0) {
        ALOGE("TouchMonitorThread Could not poll events");
        return true;
    }

    if (mPollFd.fd < 0) {
        ALOGE("TouchMonitorThread mPollFd.fd < 0");
        return true;
    }

    //ALOGD("TouchMonitorThread read");
    if (mPollFd.revents & POLLIN) {
        size = read(mPollFd.fd, event_data, MAX_STRING_LEN);
        if (size < 0) {
            ALOGE("TouchMonitorThread read failed");
            return true;
        }

        if (size > MAX_STRING_LEN) {
            ALOGE("TouchMonitorThread event size %zd is greater than event buffer size %d\n", size, MAX_STRING_LEN);
            return true;
        }

        if (size < (int32_t)sizeof(*touchEvent)) {
            ALOGE("TouchMonitorThread size %zd exp %zd\n", size, sizeof(*touchEvent));
            return true;
        }

        touchEvent = (struct input_event *)&event_data;

        int32_t i = 0;
        while (i < size) {
            touchEvent = (struct input_event *)&event_data[i];
            //ALOGD("TouchMonitorThread size %zd [%d, %d]\n", size, touchEvent->code, touchEvent->type);
            if (touchEvent->type == 1 && ((touchEvent->code == 0x152) || (touchEvent->code == 0x155))) {
                if (touchEvent->value == 0x1) {
                    ALOGD("TouchMonitorThread touch panel detected finger down");
                    mTouchStatus = 1;
                    mMiSurfaceFlingerImpl->updateIconOverlayAlpha(1.0f);
                } else if (touchEvent->value == 0x0) {
                    ALOGD("TouchMonitorThread touch panel detected finger up");
                    mTouchStatus = 0;
                    mMiSurfaceFlingerImpl->updateIconOverlayAlpha(0.0f);
                }
            }
            if(mMiSurfaceFlingerImpl->mCCTNeedCheckTouch) {
                if (touchEvent->type == 1 && touchEvent->code == 0x14a) {
                    if (mMiSurfaceFlingerImpl->mDisplayFeatureGet) {
                        if (touchEvent->value == 0x1) {
                            mMiSurfaceFlingerImpl->mDisplayFeatureGet->setFeature(0, (int32_t)FeatureId::TOUCH_STATE, 1, 0);
                        } else {
                            mMiSurfaceFlingerImpl->mDisplayFeatureGet->setFeature(0, (int32_t)FeatureId::TOUCH_STATE, 0, 0);
                        }
                    }
                }
            }
            i += sizeof(*touchEvent);
        }
    }

    return true;
}

bool TouchMonitorThread::LtpothreadLoop() {
    char buf[16] = "";
    int ret= 0;
    int touch_active_status = 0;

    ret = poll(&mPollFd, 1, -1);
    if (ret <= 0) {
        ALOGE("poll failed. error = %s", strerror(errno));
        return true;
    }

    if (mPollFd.revents & POLLPRI) {
        memset(buf, 0x0, sizeof(buf));
        if (pread(mPollFd.fd, buf, 16, 0) < 0) {
            ALOGE("pread touch_active_status first failed");
            if (pread(mPollFd.fd, buf, 16, 0) < 0) {
                ALOGE("pread touch_active_status second failed");
                return true;
            }
        }
        mMiSurfaceFlingerImpl->mTouchflag = touch_active_status = static_cast<int>(strtoul(buf, NULL, 10));
        if (mMiSurfaceFlingerImpl->mIsSyncTpForLtpo) {
            ALOGD("touch_active_status:%d", touch_active_status);
            mMiSurfaceFlingerImpl->forceSkipIdleTimer(touch_active_status);
        }
        if(mCCTNeedCheckTouch) {
            if (mDisplayFeatureGet) {
                mDisplayFeatureGet->setFeature(0, (int32_t)FeatureId::TOUCH_STATE, touch_active_status, 0);
            }
        }
    }

    return true;
}

int TouchMonitorThread::getTouchStatus() {
    return mTouchStatus;
}

} // namespace android
