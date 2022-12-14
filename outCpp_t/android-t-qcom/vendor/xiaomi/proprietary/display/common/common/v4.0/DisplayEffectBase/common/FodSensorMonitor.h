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

#ifndef _FOD_SENSOR_MONITOR_H_
#define _FOD_SENSOR_MONITOR_H_


#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/AMessage.h>
#include "DisplayModuleUtils.h"

namespace android {

class DisplayEffectBase;

class FodSensorMonitor : public AHandler
{

public:
    FodSensorMonitor();
    void start(const wp<DisplayEffectBase> &df);
    void dataCallBack(const Event &sEvent);
    void switchState(bool state);

protected:
    virtual ~FodSensorMonitor();
    virtual void onMessageReceived(const sp<AMessage> &msg);

    enum {
        kWhatFrontSensorLux     = 'flux',
    };

    DISALLOW_EVIL_CONSTRUCTORS(FodSensorMonitor);

private:
    void onSensorLux(const sp<AMessage> &msg);

    float mLux;
    float mLastLux = 100.0f;
    sp<ALooper> mLooper;
    wp<DisplayEffectBase> mDisplayEffect;
    int32_t mGeneration;
    int64_t mLastTimeStampUs;
    bool mState = false;

    /* default low brightness fod: <= 3 lux*/
    int mFod110nitLux = 3;
};

};

#endif
