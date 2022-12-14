/*
 * Copyright (C) 2020 Xiaomi Corporation
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

#ifndef _DISPLAY_MODULE_UTILS_H_
#define _DISPLAY_MODULE_UTILS_H_

#include <utils/Singleton.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include "SensorControl.h"
#include "cust_define.h"
#include "DisplayFeatureHal.h"
#include "DisplayPostprocessing.h"
#include "SensorCtrlIntf.h"
#ifndef MTK_PLATFORM
#include "sre.h"
#endif
namespace android {

typedef void (*PFN_SENSOR_CB)(const Event & sEvent);
typedef struct HBMConfigParams {
    int hbmTrigerLux;
    int hbmExitThreshold;
    int hbmKeepOnMaxTime;
    int hbmRestTime;
    int maxBL;
    int closeDelay;
} HBMConfigParams;

typedef struct HBMStatusParams {
    int hbmStatus;
    int fodScreenOn;
    int backlightFodStatus;
} HBMStatusParams;

class DisplayEffectBase;

/* display BCBC function*/
class DisplayBCBC : public RefBase
{
public:
    DisplayBCBC(const sp<DisplayEffectBase>& display_effect);
    virtual ~DisplayBCBC() { };
    void configureBCBC();
    int setBCBCFunction(int mode, int  cookie);
    int getBCBCStatus();
    void setDisplayState(int display_state);

private:
    int histogram_operation(int cmd);
    static void *ThreadWrapperBCBCHist(void *);
    int threadFuncBCBCHist();

    sp<DisplayEffectBase> mDisplayEffect;
    pthread_t mThreadBCBCHist;
    pthread_cond_t mBCBCHistCond;
    pthread_mutex_t mBCBCHistlock;
    int mThreadHistExit;
    int mHistStatus = HIST_CMD_STOP;
    Mutex mHistLock;
    int mDisplayState;
    int mBCBCEnabled;
};

class DisplayFramerateSwitchThread: public Thread
{
public:
    DisplayFramerateSwitchThread(const char *name, int display_id);
    virtual ~DisplayFramerateSwitchThread();

    virtual bool threadLoop();
    void runThreadLoop();
    void setFramerateIndex(int index);

private:
    const char* const mName;
    int mDisplayId;
    int mFramerateIndex;
    bool mRunning;
};

}
#endif
