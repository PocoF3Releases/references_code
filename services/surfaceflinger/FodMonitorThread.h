/*
 * Copyright (C) 2017 The Android Open Source Project
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
#ifndef ANDROID_FOD_CAPTURE_MONITOR_THREAD_H
#define ANDROID_FOD_CAPTURE_MONITOR_THREAD_H

#include <stddef.h>

#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <utils/threads.h>
#include <sys/poll.h>
#include "mi_disp.h"
#include "MiSurfaceFlingerImpl.h"

#define SKIP_IDLE_THRESHOLD "880"
#define FOD_LOW_BRIGHTNESS_CAPTURE 0x4

namespace android {

enum {
    MI_LAYER_FOD_HBM_OVERLAY = 0x1000000,
    MI_LAYER_FOD_ICON = 0x2000000,
    MI_LAYER_AOD_LAYER = 0x4000000,
    MI_LAYER_LOCAL_HBM = 0x8000000,
    MI_LAYER_MAX,
};

enum {
    MI_IDLE_FPS_1HZ   = 1,
    MI_IDLE_FPS_10HZ = 10,
    MI_IDLE_FPS_30HZ = 30,
    MI_IDLE_FPS_60HZ = 60,
    MI_IDLE_FPS_90HZ = 90,
    MI_IDLE_FPS_120HZ = 120,
};

enum {
    MI_DDIC_IDLE_MODE_GROUP = (0x01 << 24),
    MI_DDIC_AUTO_MODE_GROUP = (0x02 << 24),
    MI_DDIC_QSYNC_MODE_GROUP = (0x03 << 24),
    MI_DDIC_MODE_GROUP_MASK = (0x03 << 24),
};

enum {
    MI_IDLE_FPS_60HZ_BRIGHTNESS_30NIT = 492, //dbv 123, 30nit
    MI_IDLE_FPS_60HZ_BRIGHTNESS = 984, //dbv 246, 60nit
    MI_IDLE_FPS_10HZ_BRIGHTNESS = 1312, //dbv 328, 80nit
    MI_IDLE_FPS_1HZ_BRIGHTNESS   = 7368, //dbv 1842, 450nit
};

enum Fod_monitor_event {
    FOD_EVENT_BACKLIGHT,
    FOD_EVENT_UI_READY,
    FOD_EVENT_BACKLIGHT_CLONE,
    FOD_EVENT_MAX,
};

class SurfaceFlinger;
class MiSurfaceFlingerImpl;
class FingerPrintDaemonGet;
class DisplayFeatureGet;

class FodMonitorThread : public Thread {

public:
    FodMonitorThread();
    status_t Start(MiSurfaceFlingerImpl* misf, sp<SurfaceFlinger> flinger);
    int getCurrentBrightness();
    unsigned long getCurrentBrightnessClone();
    void WriteFPStatusToKernel(int fp_status);
    void writeFPStatusToKernel(int fp_status, int lowBrightnessFOD);
    void updateIdleStateByBrightness();
    void setIdleFpsByBrightness(disp_display_type displaytype);

private:

    enum {
        MI_LAYER_FOD_HBM_OVERLAY = 0x1000000,
        MI_LAYER_FOD_ICON = 0x2000000,
        MI_LAYER_AOD_LAYER = 0x4000000,
        MI_LAYER_LOCAL_HBM = 0x8000000,
        MI_LAYER_MAX,
    };

    virtual bool threadLoop();
    virtual status_t readyToRun();

    pollfd mPollBlFd; //BSP_DISPLAY_8250S

    static constexpr const char* kFodHbmStatusPath = "/dev/mi_display/disp_feature";
    pollfd mFodEventPollFd;

    sp<SurfaceFlinger> mFlinger;
    MiSurfaceFlingerImpl* mMiFlinger;
    bool mCaptureStart = false;
    sp<FingerPrintDaemonGet> mFingerPrintDaemonGet;
    sp<DisplayFeatureGet> mDisplayFeatureGet;

    int mBrightness;

    unsigned long mBrightnessClone;
    unsigned long mLastBrightnessClone;
    int32_t global_fod_delay_ms = 0;
    bool mPlatform_8250 = false;
};

}

#endif // ANDROID_FRAME_RENDER_MONITOR_THREAD_H
