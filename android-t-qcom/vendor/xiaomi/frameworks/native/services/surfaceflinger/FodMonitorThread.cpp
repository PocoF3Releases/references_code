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
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <utils/Trace.h>
#include "FodMonitorThread.h"
#include <log/log.h>
#include "MiSurfaceFlingerImpl.h"
#include "FingerPrintDaemonGet.h"
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/inotify.h>
#include <linux/input.h>
#include <cutils/properties.h>

#include "DisplayFeatureGet.h"

namespace android {

FodMonitorThread::FodMonitorThread():
        Thread(false) {}

status_t FodMonitorThread::Start(MiSurfaceFlingerImpl* misf, sp<SurfaceFlinger> flinger) {
    mFlinger = flinger;
    mMiFlinger = misf;
    global_fod_delay_ms = property_get_int32("ro.vendor.mi_sf.global_fod_delay_ms", 0);
    mPlatform_8250 = property_get_bool("ro.vendor.display.platform_8250", false);
    if (mPlatform_8250) {
        return run("SurfaceFlinger::FodCaptureMonitorThread", PRIORITY_NORMAL);
    } else {
        mLastBrightnessClone = mMiFlinger->mSkipIdleThreshold + 1;
        return run("SF_FodCapMonitorThread", PRIORITY_NORMAL);
    }
}

status_t FodMonitorThread::readyToRun() {
    if (mPlatform_8250) {
        mPollBlFd.fd = open("/sys/class/drm/card0-DSI-1/fod_ui_ready", O_RDONLY);
        if (mPollBlFd.fd < 0) {
            ALOGE("open /sys/class/drm/card0-DSI-1/fod_ui_ready  failed");
            return -1;
        } else {
            ALOGD("open /sys/class/drm/card0-DSI-1/fod_ui_ready success");
        }

        mPollBlFd.events = POLLPRI | POLLERR;

        mFingerPrintDaemonGet = new FingerPrintDaemonGet();
        mDisplayFeatureGet = new DisplayFeatureGet();

        return NO_ERROR;
    } else {
        int ret = 0;
        unsigned int display_device_id = 0;
        struct disp_event_req event_req;
        struct disp_brightness_req brightness_req;
        int fod_event_index = 0;

        mFodEventPollFd.fd = open(kFodHbmStatusPath, O_RDWR | O_CLOEXEC);
        if (mFodEventPollFd.fd  < 0) {
            ALOGE("open %s failed", kFodHbmStatusPath);
            return -1;
        } else {
            ALOGD("open %s success", kFodHbmStatusPath);
        }

        brightness_req.base.flag = 0;
        brightness_req.base.disp_id = MI_DISP_PRIMARY;
        ret = ioctl(mFodEventPollFd.fd, MI_DISP_IOCTL_GET_BRIGHTNESS, &brightness_req);
        if(ret) {
            ALOGE("%s, ioctl MI_DISP_IOCTL_GET_BRIGHTNESS fail\n", __func__);
            return -1;
        }
        mBrightness = (int)brightness_req.brightness;
        ALOGE("%s, get brightness:%d\n", __func__, mBrightness);
        {
           Mutex::Autolock _l(mMiFlinger->mFlinger->mStateLock);
           mMiFlinger->updateHBMOverlayAlphaLocked(mBrightness);
        }

        mFodEventPollFd.events = POLLIN | POLLRDNORM | POLLERR;

        for(display_device_id = MI_DISP_PRIMARY; display_device_id < MI_DISP_MAX;display_device_id++) {
            for (fod_event_index = 0; fod_event_index < FOD_EVENT_MAX; fod_event_index++) {
                event_req.base.flag = 0;
                event_req.base.disp_id = display_device_id;

                switch (fod_event_index) {
                    case FOD_EVENT_BACKLIGHT:
                        event_req.type = MI_DISP_EVENT_BACKLIGHT;
                        break;
                    case FOD_EVENT_UI_READY:
                        /*Don't need to monitor UI Ready event as fodengine enabled*/
                        if (mMiFlinger->isFodEngineEnabled()) {
                            continue;
                        }
                        event_req.type = MI_DISP_EVENT_FOD;
                        break;
                    case FOD_EVENT_BACKLIGHT_CLONE:
                        event_req.type = MI_DISP_EVENT_BRIGHTNESS_CLONE;
                        break;
                    default:
                        break;
                }

                ALOGD("%s, event type name: %s\n", __func__, getDispEventTypeName(event_req.type));
                ret = ioctl(mFodEventPollFd.fd, MI_DISP_IOCTL_REGISTER_EVENT, &event_req);
                if(ret) {
                    ALOGE("%s, ioctl MI_DISP_IOCTL_REGISTER_EVENT fail\n", __func__);
                    return -1;
                }
            }
        }
        mFingerPrintDaemonGet = new FingerPrintDaemonGet();
        return NO_ERROR;
    }
}

bool FodMonitorThread::threadLoop() {
    if (mPlatform_8250) {
        char buf[16] = "";
        int ret= 0;
        int fod_ui_ready = 0;

        ret = poll(&mPollBlFd, 1, -1);
        if (ret <= 0) {
            ALOGE("poll failed. error = %s", strerror(errno));
            return true;
        }

        if (mPollBlFd.revents & POLLPRI) {
            memset(buf, 0x0, sizeof(buf));
            if (pread(mPollBlFd.fd, buf, 16, 0) < 0) {
                ALOGE("pread fod_ui_ready first failed");
                if (pread(mPollBlFd.fd, buf, 16, 0) < 0) {
                    ALOGE("pread fod_ui_ready second failed");
                    return true;
                }
            }
            fod_ui_ready = static_cast<int>(strtoul(buf, NULL, 10));
            ALOGE("xiaomi-debug fod fod_ui_ready:%d", fod_ui_ready);
            if (fod_ui_ready == (MI_LAYER_FOD_HBM_OVERLAY|MI_LAYER_FOD_ICON) >> 24) {
                mDisplayFeatureGet->setFeature(0, 24, 1, 0);

                ATRACE_NAME("capture start");
                ALOGD("fod capture start");
                if(global_fod_delay_ms)
                    usleep((unsigned int)(global_fod_delay_ms*1000));
                mFingerPrintDaemonGet->sendCmd(10, FingerPrintDaemonGet::TARGET_BRIGHTNESS_600NIT);
                mCaptureStart = true;
            } else if (fod_ui_ready == 0x7) {
                ATRACE_NAME("capture start");
                ALOGD("fod capture start");
                if(global_fod_delay_ms)
                    usleep((unsigned int)(global_fod_delay_ms*1000));
                mFingerPrintDaemonGet->sendCmd(10, FingerPrintDaemonGet::TARGET_BRIGHTNESS_110NIT);
                mCaptureStart = true;
            } else {
                if (mCaptureStart) {
                    mDisplayFeatureGet->setFeature(0, 24, 0, 0);

                    ATRACE_NAME("capture stop");
                    ALOGD("fod capture stop");
                    mFingerPrintDaemonGet->sendCmd(10, FingerPrintDaemonGet::TARGET_BRIGHTNESS_OFF);
                    mCaptureStart = false;
                }
            }
        }

        return true;
    } else {
        char event_data[1024] = {0};
        int ret = 0;
        int fod_ui_ready = 0;
        struct disp_event_resp *event_resp = NULL;
        //int *event_payload;
        ssize_t size;
        int i;

        ret = poll(&mFodEventPollFd, 1, -1);
        if (ret <= 0) {
            ALOGE("poll failed. error = %s", strerror(errno));
            return true;
        }

        if (mFodEventPollFd.revents & (POLLIN | POLLRDNORM | POLLERR)) {
            memset(event_data, 0x0, sizeof(event_data));
            if ((size = read(mFodEventPollFd.fd, event_data, sizeof(event_data))) < 0) {
                ALOGE("read fod event failed\n");
                return true;
            }
            if ((unsigned long)size < sizeof(struct disp_event_resp)) {
                ALOGE("Invalid event size %zd, expect %zd\n", size, sizeof(struct disp_event_resp));
                return true;
            }

            i = 0;
            while (i < size - 15) {
                DisplayDevice* display = mMiFlinger->getActiveDisplayDevice().get();

                event_resp = (struct disp_event_resp *)&event_data[i];
                switch (event_resp->base.type) {
                case MI_DISP_EVENT_BACKLIGHT:
                    if (event_resp->base.length - sizeof(struct disp_event_resp) < 2) {
                        ALOGE("Invalid Backlight value\n");
                        break;
                    }
                    mBrightness = (int)*((int*)(event_resp->data));
                    ALOGV("MI_DISP_EVENT_BACKLIGHT:%d\n", mBrightness);
                    {
                    Mutex::Autolock _l(mMiFlinger->mFlinger->mStateLock);
                    mMiFlinger->updateHBMOverlayAlphaLocked(mBrightness);
                    }
                    break;
                case MI_DISP_EVENT_FOD:
                    fod_ui_ready = (int)*((int*)(event_resp->data));
                    ALOGD("MI_DISP_EVENT_FOD:%d\n", fod_ui_ready);
                    if ((fod_ui_ready & (MI_LAYER_FOD_HBM_OVERLAY >> 24)) && (fod_ui_ready & (MI_LAYER_FOD_ICON >> 24))) {
                        ATRACE_NAME("capture start");
                        ALOGD("fod capture start");
                        if (fod_ui_ready & FOD_LOW_BRIGHTNESS_CAPTURE) {
                            ALOGD("fod low brightness capture start");
                            mFingerPrintDaemonGet->sendCmd(10, FingerPrintDaemonGet::TARGET_BRIGHTNESS_110NIT);
                        } else {
                            ALOGD("fod high brightness capture start");
                            mFingerPrintDaemonGet->sendCmd(10, FingerPrintDaemonGet::TARGET_BRIGHTNESS_600NIT);
                        }
                        mCaptureStart = true;
                    } else if (fod_ui_ready & (MI_LAYER_LOCAL_HBM >> 24)) {
                        if (fod_ui_ready & FOD_LOW_BRIGHTNESS_CAPTURE) {
                            ALOGD("fod low brightness capture start");
                            mFingerPrintDaemonGet->sendCmd(10, FingerPrintDaemonGet::TARGET_BRIGHTNESS_110NIT);
                        } else {
                            ALOGD("fod high brightness capture start");
                            mFingerPrintDaemonGet->sendCmd(10, FingerPrintDaemonGet::TARGET_BRIGHTNESS_600NIT);
                        }
                        mCaptureStart = true;
                    } else {
                        if (mCaptureStart) {
                            ATRACE_NAME("capture stop");
                            ALOGD("fod capture stop");
                            mFingerPrintDaemonGet->sendCmd(10, FingerPrintDaemonGet::TARGET_BRIGHTNESS_OFF);
                            mCaptureStart = false;
                        }
                    }
                    break;
                case MI_DISP_EVENT_BRIGHTNESS_CLONE:
                    if (event_resp->base.length - sizeof(struct disp_event_resp) < 2) {
                        ALOGE("Invalid Backlight value\n");
                        break;
                    }

                    mBrightnessClone = (unsigned long)*((int *)(event_resp->data));
                    if (mMiFlinger->mIsLtpoPanel) {
                        setIdleFpsByBrightness((disp_display_type)event_resp->base.disp_id);
                    }
                    else {
                        updateIdleStateByBrightness();
                    }

                    mLastBrightnessClone = mBrightnessClone;
                    break;
                default:
                    break;
                }
                i += event_resp->base.length;
            }
        }
        return true;
    }
}

void FodMonitorThread::WriteFPStatusToKernel(int fp_status) {

    ALOGD("WriteFPStatusToKernel: %d", fp_status);
    if(mDisplayFeatureGet != NULL)
        mDisplayFeatureGet->setFingerprintState(fp_status);
}

void FodMonitorThread::writeFPStatusToKernel(int fp_status, int lowBrightnessFOD) {
   int ret = 0;

   /* lowBrightnessFOD (1: allow) (0: not allow) */
   char txBuf = lowBrightnessFOD ? 1 : 0;

   struct disp_feature_req feature_req = {
       .base.flag = 0, // MI_DISP_FLAG_BLOCK
       .base.disp_id = MI_DISP_PRIMARY,
       .feature_id = DISP_FEATURE_FP_STATUS,
       .feature_val = fp_status,
       .tx_len = 1,
       .tx_ptr = (__u64)&txBuf,
   };

   // mi_disp_ioctl_set_feature
   ret = ioctl(mFodEventPollFd.fd, MI_DISP_IOCTL_SET_FEATURE, feature_req);
   if(ret) {
       ALOGE("%s, ioctl fail\n", __func__);
   } else {
       ALOGD("%s: %d", __func__, feature_req.feature_val);
   }
}

int FodMonitorThread::getCurrentBrightness() {
    return mBrightness;
}

unsigned long FodMonitorThread::getCurrentBrightnessClone() {
    return mBrightnessClone;
}


void FodMonitorThread::updateIdleStateByBrightness() {
        if (mBrightnessClone <= mMiFlinger->mSkipIdleThreshold && mLastBrightnessClone > mMiFlinger->mSkipIdleThreshold) {
            ALOGE("Force skip idle timer[true]\n");
            mMiFlinger->forceSkipIdleTimer(true);
            mMiFlinger->setVideoCodecFpsSwitchState();
        } else if (mBrightnessClone > mMiFlinger->mSkipIdleThreshold && mLastBrightnessClone <= mMiFlinger->mSkipIdleThreshold) {
            ALOGE("Force skip idle timer[false]\n");
            mMiFlinger->forceSkipIdleTimer(false);
            mMiFlinger->setVideoCodecFpsSwitchState();
        }
}

void FodMonitorThread::setIdleFpsByBrightness(disp_display_type displaytype) {
    if (displaytype == MI_DISP_PRIMARY) {
        unsigned long i = 0;
        for(i = 0; i < mMiFlinger->primaryIdleBrightnessList.size(); i++) {
            if (mBrightnessClone <= (unsigned long)mMiFlinger->primaryIdleBrightnessList[i]) {
                mMiFlinger->mIdleFrameRate = mMiFlinger->primaryIdleRefreshRateList[i];
                break;
            }
        }

        if (i == mMiFlinger->primaryIdleBrightnessList.size()) {
            mMiFlinger->mIdleFrameRate = mMiFlinger->primaryIdleRefreshRateList[i];
            if (mMiFlinger->mIsSyncTpForLtpo) {
                if (!mMiFlinger->mSetEbookIdleFlag) {
                    mMiFlinger->mIdleFrameRate = mMiFlinger->primaryIdleRefreshRateList[i -1];
                }
            }
        }

        if (mMiFlinger->mIsSupportSkipIdle) {
            updateIdleStateByBrightness();
        }
    }
    else {
        updateIdleStateByBrightness();
    }
}

} // namespace android
