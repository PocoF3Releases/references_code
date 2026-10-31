/*
 * Copyright (C) 2013 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License") = 0;
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

#include "FodSensorMonitor.h"
#include "DisplayFeatureState.h"
#include <media/stagefright/foundation/ADebug.h>
#include "DisplayEffectBase.h"

namespace android {
static void FodSensorCallbackFunction(int event, const Event &info, void *user) {
    FodSensorMonitor *source = (FodSensorMonitor *) user;
    if (source) {
        if (event == SENSOR_TYPE_LIGHT_PRIMARY_SCREEN)
            source->dataCallBack(info);
    }
}

FodSensorMonitor::FodSensorMonitor()
        : mLooper(new ALooper)
        , mGeneration(0)
        , mLastTimeStampUs(-1ll)
        , mState(false) {

    DF_LOGD("FodSensorMonitor");
    mLooper->setName("FodSensorMonitor");
    mLooper->start(false /* runOnCallingThread */,
                   false /* canCallJava */,
                   PRIORITY_NORMAL);
    SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();
    mSensorCtrl.registerCallBack((void*)FodSensorCallbackFunction, this);
}

FodSensorMonitor::~FodSensorMonitor() {
    DF_LOGD("~FodSensorMonitor");

}

void FodSensorMonitor::start(const wp<DisplayEffectBase> &df) {
    DF_LOGD("FodSensorMonitor start");
    mLooper->registerHandler(this);
    mDisplayEffect = df;
    sp<DisplayEffectBase> sdf = mDisplayEffect.promote();
    mFod110nitLux = sdf->mFod110nitLux;
    DF_LOGD("FodSensorMonitor: mFod110nitLux=%d", mFod110nitLux);
}

void FodSensorMonitor::switchState(bool state) {
    if (state == mState)
        return;
    int32_t st = state ? SENSOR_START : SENSOR_STOP;
    int displayId = 0;
    SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();
    sp<DisplayEffectBase> sdf = mDisplayEffect.promote();
    if (sdf != NULL)
        displayId = sdf->mDisplayId;
    mSensorCtrl.sensorControlOP(displayId, st, SENSOR_LIGHT, FOD);
    mState = state;
}

void FodSensorMonitor::dataCallBack(const Event &sEvent) {
    DF_LOGV("FodSensorMonitor dataCallBack");

    if (!mState)
        return;

    if (mLastTimeStampUs < 0
            || ((sEvent.timestamp - mLastTimeStampUs > 1000000000ll)
            && ((mLastLux > mFod110nitLux && sEvent.u.data[0] <= mFod110nitLux)
            || (mLastLux <= mFod110nitLux && sEvent.u.data[0] > mFod110nitLux)))) {

        sp<AMessage> msg = new AMessage(FodSensorMonitor::kWhatFrontSensorLux, this);
        msg->setFloat("UData0", sEvent.u.data[0]);
        msg->setFloat("UData1", sEvent.u.data[1]);
        msg->setFloat("UData2", sEvent.u.data[2]);
        msg->setInt64("TimeStamp", sEvent.timestamp);
        msg->setInt32("SensorType", (int32_t)sEvent.sensorType);
        msg->setInt32("Generation", ++mGeneration);
        msg->post();
        mLastLux = sEvent.u.data[0];

        mLastTimeStampUs = sEvent.timestamp;
    }
}

void FodSensorMonitor::onSensorLux(const sp<AMessage> &msg) {
    float uData0, uData1, uData2;
    int64_t timeStamp;
    int32_t sensorType;
    int32_t generation;
    int32_t displayState;
    int ret = 0;

    CHECK(msg->findFloat("UData0", &uData0));
    CHECK(msg->findFloat("UData1", &uData1));
    CHECK(msg->findFloat("UData2", &uData2));

    CHECK(msg->findInt64("TimeStamp", &timeStamp));
    CHECK(msg->findInt32("SensorType", &sensorType));
    CHECK(msg->findInt32("Generation", &generation));

    if (generation == mGeneration) {
        sp<DisplayEffectBase> sdf = mDisplayEffect.promote();
        if (sdf != NULL) {
            displayState = sdf->getDisplayState();

            DF_LOGD("onSensorLux value=<%9.4f,%9.4f,%9.4f>,"
               "time=%ld, sensor=%d, state=%d",
               uData0, uData1, uData2,
               timeStamp, sensorType, displayState);

            struct OutputParam sParams;
            sParams.function_id = DISP_FEATURE_SENSOR_LUX;
            sParams.param1 = (uint32_t)uData0;
            ret = sdf->setDisplayEffect(SET_PANEL_MODE, &sParams);
        }
    }
}

void FodSensorMonitor::onMessageReceived(const sp<AMessage> &msg) {
    DF_LOGV("onMessageReceived");
    switch (msg->what()) {
        case kWhatFrontSensorLux:
            onSensorLux(msg);
            break;

        default:
            TRESPASS();
            break;
    }
}

}

