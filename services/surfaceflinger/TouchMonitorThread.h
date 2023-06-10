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

#ifndef ANDROID_TOUCH_MONITOR_THREAD_H
#define ANDROID_TOUCH_MONITOR_THREAD_H

#include <stddef.h>

#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <utils/threads.h>
#include <sys/poll.h>
#include "MiSurfaceFlingerImpl.h"
#include "DisplayFeatureGet.h"

namespace android {

class MiSurfaceFlingerImpl;

class TouchMonitorThread : public Thread {

public:
    TouchMonitorThread();
    status_t Start(MiSurfaceFlingerImpl* misf, sp<SurfaceFlinger> flinger);
    int getTouchStatus();
    bool LtpothreadLoop();

private:
    virtual bool threadLoop();
    virtual status_t readyToRun();

    static constexpr const char* kTouchActiveStatusPath = "/sys/class/touch/touch_dev/touch_finger_status";//touch_active_status
    static constexpr const char* kInputPath = "/dev/input";
    static constexpr const char* kTouchDeviceNamefts = "fts";
    static constexpr const char* kTouchDeviceNamegfts = "goodix_ts";
    static constexpr const char* kTouchDeviceNameffts = "fts_ts";
    static const int32_t MAX_STRING_LEN = 1024;

    pollfd mPollFd;
    int mPollWd;
    sp<SurfaceFlinger> mFlinger;
    MiSurfaceFlingerImpl* mMiSurfaceFlingerImpl;
    char mForceTouchPath[255] = {0};
    int mTouchStatus = 0;
    sp<DisplayFeatureGet> mDisplayFeatureGet;
    bool mCCTNeedCheckTouch = false;
};

}

#endif // ANDROID_TOUCH_MONITOR_THREAD_H

