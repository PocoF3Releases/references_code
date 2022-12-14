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

#ifndef FINGERPRINTDAEMON_GET_H
#define FINGERPRINTDAEMON_GET_H

#include <utils/StrongPointer.h>
#include <utils/Mutex.h>
#include <android/hidl/base/1.0/IBase.h>
#include <android/hidl/base/1.0/BpHwBase.h>

#include <hidl/ServiceManagement.h>
#include <hwbinder/ProcessState.h>
#include <android/hidl/manager/1.0/IServiceManager.h>

#include <utils/StrongPointer.h>
#include <utils/Mutex.h>

#include <vendor/xiaomi/hardware/fingerprintextension/1.0/IXiaomiFingerprint.h>

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/AMessage.h>

using android::hidl::base::V1_0::IBase;
using android::hardware::hidl_death_recipient;

namespace android {

class FingerPrintDaemonGet : public hidl_death_recipient {
public:
    FingerPrintDaemonGet() { }
    virtual ~FingerPrintDaemonGet() { }

    // DeathRecipient
    virtual void serviceDied(uint64_t cookie, const wp<IBase>& who);
    const sp<vendor::xiaomi::hardware::fingerprintextension::V1_0::IXiaomiFingerprint> getFingerPrintDaemon();
    bool sendCmd(uint32_t cmd, uint32_t param);
    bool getNeedResendStatus();
    void resetNeedResendStatus();

    static const unsigned long LOW_BRIGHTNESS_THRESHHOLD = 100;

    static const unsigned long TARGET_BRIGHTNESS_OFF = 0;
    static const unsigned long TARGET_BRIGHTNESS_600NIT = 1;
    static const unsigned long TARGET_BRIGHTNESS_500NIT = 2;
    static const unsigned long TARGET_BRIGHTNESS_400NIT = 3;
    static const unsigned long TARGET_BRIGHTNESS_300NIT = 4;
    static const unsigned long TARGET_BRIGHTNESS_200NIT = 5;
    static const unsigned long TARGET_BRIGHTNESS_110NIT = 6;
private:
    Mutex mServiceLock;
    sp<vendor::xiaomi::hardware::fingerprintextension::V1_0::IXiaomiFingerprint> mFingerPrintDaemon;

    bool mFingerPrintDaemonDied = false;
    bool mNeedResendStatus = false;
};

class FingerprintStateManager : public AHandler {
public:
    FingerprintStateManager() { }
    virtual ~FingerprintStateManager() { }
    void init();
    void fingerprintConditionUpdate(int stateIndex, int value);
    void resendFingerprintCondition();
    enum {
        kASYNC    = 'a',
        kSYNC     = 's',
    };

    enum {
        FOD_ICON_LAYER_STATE = 1,
        COLOR_FADE_LAYER_STATE = 2,
        DISPLAY_POWER_STATE = 3,
        FINGERPRINT_MONITOR_STATE = 4,
        LOCKOUT_MODE_STATE = 5,
        FOD_LOWBRIGHTNESS_ENABLE_STATE = 6,  //fod low brightness feature enabled
        FOD_LOWBRIGHTNESS_ALLOW_STATE = 7,   //only allow low brightness unlock in first keyguard authtication
    };

    Mutex mFingerprintStateManagerMutex;

    int mLastDisplayPowerState = 0;
    int mLastFodIconLayerState = 0;
    int mLastColorFadeLayerState = 0;
    int mLastFodLowBrightnessEnableState = 0;
    int mLastFodLowBrightnessAllowState = 0;

protected:
    void onMessageReceived(const sp<AMessage> &msg);

private:
    void sendMessageAsync(int stateIndex, int value);
    void sendMessageSync(int stateIndex, int value);

    sp<ALooper> mLooper;
    sp<FingerPrintDaemonGet> mFingerPrintDaemonGet;
};

class FPMessageThread : public Thread {

public:
    FPMessageThread();
    status_t Start();
    void saveSurfaceCompose();

private:
    virtual bool threadLoop();
    virtual status_t readyToRun();

    Mutex mMutex;
    Condition mCond;
    uint32_t mCmd = 0;
    uint32_t mValue = 0;

    sp<FingerPrintDaemonGet> mFingerPrintDaemonGet;
};

}

#endif
