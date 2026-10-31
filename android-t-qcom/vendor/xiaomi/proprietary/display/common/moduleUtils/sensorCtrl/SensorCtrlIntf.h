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
#ifndef _SENSOR_CTRL_INTF_H_
#define _SENSOR_CTRL_INTF_H_

#include <utils/Singleton.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include <utils/Thread.h>
#include "DisplayFeatureHal.h"
#include "DFComDefine.h"
#include "SensorControl.h"
#include "df_log.h"
namespace android {
const unsigned int sensor_user_max = 20;
#define SENSOR_SAMPLE_RATE_MS 500
enum {
    SENSOR_LIGHT = 0,
    SENSOR_FOLD,
    SENSOR_FRONT_CCT,
    SENSOR_MAX
};

enum {
    SENSOR_STOP = 0,
    SENSOR_START,
};

enum LUX_LEVEL_INDEX{
    LUX_LEVEL_ABOVE_5000,
    LUX_LEVEL_ABOVE_10000,
    LUX_LEVEL_ABOVE_20000,
    LUX_LEVEL_ABOVE_35000,
    LUX_LEVEL_ABOVE_50000,
    LUX_LEVEL_ABOVE_70000,
    LUX_LEVEL_BELOW_5000,
};

enum SENSOR_USER {
    SRE = 0,
    TRUETONE = 1,
    DITHER = 2,
    FOLD = 3,
    FOD = 4,
    SENSOR_USER_MAX,
};

typedef void (*PFN_SENSORCTRL_CB)(int event, const Event & sEvent, void* user);

/* sensor control*/
class SensorCtrlIntf : public RefBase, public Singleton<SensorCtrlIntf>
{
public:
    SensorCtrlIntf();
    virtual ~SensorCtrlIntf();

    int isSensorRunning(unsigned int type,int displayId);
    int sensorControlOP(int displayId, int op, int type, SENSOR_USER user);
    void registerCallBack(void *callback, void *user);
    void sensorCallback(int event, const Event &info);
    void FoldSensorCallback(const Event &info);
    void setDisplayState(int display_state);

private:
    int checkActiveUserList(int type, SENSOR_USER user);
    void addToActiveUser(int type, SENSOR_USER user);
    void removeFromActiveUser(int type, SENSOR_USER user);
    sp<SensorControl> mSensorHandle[SENSOR_MAX][DISPLAY_MAX];
    int mSensorInitialized[SENSOR_MAX][DISPLAY_MAX] = {{0}};
    int mSensorRunning[SENSOR_MAX][DISPLAY_MAX] = {{0}};
    int mSensorUsers[SENSOR_MAX][SENSOR_USER_MAX] = {{0}};
    int mSensorUserCnt[SENSOR_MAX][DISPLAY_MAX] = {{0}};
    PFN_SENSORCTRL_CB mSensorCallBack[sensor_user_max] = {NULL};
    void * mUser[sensor_user_max] = {NULL};
    int mDisplayState;
    int mFoldStatus = -1;

    int mCallbackNum = 0;
};

}; //namespace android

#endif
