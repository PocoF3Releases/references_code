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

//#define LOG_NDEBUG 0

#include <log/log.h>
#include <cutils/properties.h>
#include "FingerPrintDaemonGet.h"
#include <hidl/Status.h>
#include <hidl/HidlTransportSupport.h>
#include <binder/IBinder.h>
#include <media/stagefright/foundation/ADebug.h>

using android::hidl::base::V1_0::IBase;
using vendor::xiaomi::hardware::fingerprintextension::V1_0::IXiaomiFingerprint;

using android::hidl::manager::V1_0::IServiceManager;
using vendor::xiaomi::hardware::fingerprintextension::V1_0::IXiaomiFingerprint;
/*using vendor::xiaomi::hardware::fingerprintextension::V1_0::implementation::XiaomiFingerprint;*/

using ::android::hardware::hidl_string;
using ::android::hardware::Return;
/*using ::android::hardware::defaultServiceManager;*/
using android::hardware::IPCThreadState;
using android::hardware::ProcessState;

namespace android {

const sp<vendor::xiaomi::hardware::fingerprintextension::V1_0::IXiaomiFingerprint> FingerPrintDaemonGet::getFingerPrintDaemon()
{
    Mutex::Autolock autoLock(mServiceLock);

    if (mFingerPrintDaemon.get() == NULL) {
        mFingerPrintDaemon = IXiaomiFingerprint::tryGetService();
        if (mFingerPrintDaemon.get() == NULL) {
            ALOGE("Query IXiaomiFingerprint service returned null!");
            return mFingerPrintDaemon;
        }

        mFingerPrintDaemon->linkToDeath(this, /*cookie*/1);

        if (mFingerPrintDaemonDied) {
            mNeedResendStatus = true;
            mFingerPrintDaemonDied = false;
        }
    }

    return mFingerPrintDaemon;
}

bool FingerPrintDaemonGet::sendCmd(uint32_t cmd, uint32_t param) {
    if (getFingerPrintDaemon().get() != NULL) {
        if(!mFingerPrintDaemon->extCmd(cmd, param).isOk()) {
            ALOGE("Failed to sendMessage to FingerPrintDaemon");
        }
    }
    return true;
}

void FingerPrintDaemonGet::serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/)
{
    Mutex::Autolock autoLock(mServiceLock);
    ALOGW("receive the notification, FingerPrintDaemon service die!");
    mFingerPrintDaemonDied = true;
    if (mFingerPrintDaemon.get()) {
        mFingerPrintDaemon->unlinkToDeath(this);
        mFingerPrintDaemon.clear();
        ALOGD("clean FingerPrintDaemon handle!");
    }
}

bool FingerPrintDaemonGet::getNeedResendStatus() {
    return mNeedResendStatus;
}

void FingerPrintDaemonGet::resetNeedResendStatus() {
    mNeedResendStatus = false;
}

void FingerprintStateManager::init() {
    mFingerPrintDaemonGet = new FingerPrintDaemonGet();
    if (mLooper == NULL) {
        mLooper = new ALooper;
        mLooper->setName("FingerPrintDaemonGet");
        mLooper->start();
        mLooper->registerHandler(this);
    }
}

void FingerprintStateManager::resendFingerprintCondition() {
    ALOGD("resendFingerprintCondition");
    sendMessageAsync(DISPLAY_POWER_STATE, mLastDisplayPowerState);
    sendMessageAsync(FOD_ICON_LAYER_STATE, mLastFodIconLayerState);
    sendMessageAsync(COLOR_FADE_LAYER_STATE, mLastColorFadeLayerState);
    sendMessageAsync(FOD_LOWBRIGHTNESS_ENABLE_STATE, mLastFodLowBrightnessEnableState);
    sendMessageAsync(FOD_LOWBRIGHTNESS_ALLOW_STATE, mLastFodLowBrightnessAllowState);
}

void FingerprintStateManager::fingerprintConditionUpdate(int stateIndex, int value) {
    if (mFingerPrintDaemonGet->getNeedResendStatus()) {
        resendFingerprintCondition();
        mFingerPrintDaemonGet->resetNeedResendStatus();
    }
    ALOGD("FodEngineCore  onChangeState:  stateIndex:%d  value:%d", stateIndex, value);
    switch (stateIndex) {
        case DISPLAY_POWER_STATE:
            mLastDisplayPowerState = value;
            sendMessageSync(stateIndex, value);
            break;
        case FOD_ICON_LAYER_STATE:
            mLastFodIconLayerState = value;
            sendMessageAsync(stateIndex, value);
            break;
        case COLOR_FADE_LAYER_STATE:
            mLastColorFadeLayerState = value;
            sendMessageAsync(stateIndex, value);
            break;
        case FOD_LOWBRIGHTNESS_ENABLE_STATE:
            mLastFodLowBrightnessEnableState = value;
            sendMessageAsync(stateIndex, value);
            break;
        case FOD_LOWBRIGHTNESS_ALLOW_STATE:
            mLastFodLowBrightnessAllowState = value;
            sendMessageAsync(stateIndex, value);
            break;
        default: {
            ALOGE("%s unknown stateIndex:%d value:%d", __func__, stateIndex, value);
        } break;
    }
    return;
}

void FingerprintStateManager::sendMessageAsync(int stateIndex, int value)
{
    sp<AMessage> msg = new AMessage(kASYNC, this);
    msg->setInt32("stateIndex", stateIndex);
    msg->setInt32("value", value);
    msg->post();
}

void FingerprintStateManager::sendMessageSync(int stateIndex, int value)
{
    int err = 0;
    sp<AMessage> response;
    sp<AMessage> msg = new AMessage(kSYNC, this);
    msg->setInt32("stateIndex", stateIndex);
    msg->setInt32("value", value);
    err = msg->postAndAwaitResponse(&response);
    if (err != OK || !response->findInt32("err", &err) || err != OK)
        ALOGE("%s sendMessageSync stateIndex:%d value:%d failed", __func__, stateIndex, value);
    else
        ALOGE("%s sendMessageSync stateIndex:%d value:%d done", __func__, stateIndex, value);
}

void FingerprintStateManager::onMessageReceived(const sp<AMessage> &msg) {
    int32_t stateIndex;
    int32_t value;
    switch (msg->what()) {
        case kASYNC:
        {
            CHECK(msg->findInt32("stateIndex", &stateIndex));
            CHECK(msg->findInt32("value", &value));
            mFingerPrintDaemonGet->sendCmd((uint32_t)stateIndex, (uint32_t)value);
            break;
        }
        case kSYNC:
        {
            CHECK(msg->findInt32("stateIndex", &stateIndex));
            CHECK(msg->findInt32("value", &value));
            mFingerPrintDaemonGet->sendCmd((uint32_t)stateIndex, (uint32_t)value);
            sp<AReplyToken> replyID;
            CHECK(msg->senderAwaitsResponse(&replyID));
            sp<AMessage> response = new AMessage;
            response->setInt32("err", OK);
            response->postReply(replyID);
            break;
        }
        default:
           ALOGE("%s Invalid message %d", __func__, msg->what());
            break;
    }
}

FPMessageThread::FPMessageThread():
        Thread(false) {}

status_t FPMessageThread::Start() {
    return run("SF_FPMsgThread", PRIORITY_NORMAL);
}

status_t FPMessageThread::readyToRun() {

    mFingerPrintDaemonGet = new FingerPrintDaemonGet();

    return NO_ERROR;
}

bool FPMessageThread::threadLoop() {
    status_t err;

    {
        Mutex::Autolock lock(mMutex);
        err = mCond.wait(mMutex);
        if (err != NO_ERROR) {
            ALOGE("error waiting for DFMessage: %s (%d)", strerror(-err), err);
            return true;
        }
    }

    if (mFingerPrintDaemonGet != NULL) {
        mFingerPrintDaemonGet->sendCmd(mCmd, mValue);
    }

    return true;
}

void FPMessageThread::saveSurfaceCompose() {
    Mutex::Autolock lock(mMutex);
    mCmd = 0x0B;
    mValue = 0x00;
    mCond.signal();
}

}
