/*
 * Copyright (C) 2010 The Android Open Source Project
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


#include <utils/Singleton.h>
#include <utils/String8.h>
#include <utils/Timers.h>
#include <ctime>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "SensorServiceUtils.h"
#include <log/log.h>
#include "MiSensorServiceImpl.h"
#include <utils/StrongPointer.h>

using namespace std;
namespace android {
typedef struct sensorTime {
    String8 sensorName;
    time_t mDeltatimeSec;
    bool operator==(const String8& findSensorName) { return (this->sensorName == findSensorName); }
} sensorTime_t;
typedef struct sensorTypeTime {
    int sensorType;
    time_t mDeltatimeSec;
    bool operator==(const int& findSensorType) { return (this->sensorType == findSensorType); }
} sensorTypeTime_t;
typedef struct appSensorTime {
    String8 packageName;
    String8 sensorName;
    time_t mDeltatimeSec;
    bool operator==(const appSensorTime& appSensor) {
        return (this->sensorName == appSensor.sensorName) &&
                (this->packageName == appSensor.packageName);
    }
} appSensorTime_t;
typedef struct appSensorTypeTime {
    String8 packageName;
    int sensorType;
    time_t mDeltatimeSec;
    bool operator==(const appSensorTypeTime& appSensorType) {
        return (this->sensorType == appSensorType.sensorType) &&
                (this->packageName == appSensorType.packageName);
    }
} appSensorTypeTime_t;
typedef struct savedItem {
    String8 packageName;
    String8 sensorName;
    int sensorType;
    time_t timeSec;
    time_t timeNsec;
    bool activate;
} savedItem_I;
typedef struct collDet {
    float hight;
    int posture;
    string time;
} collDet_c;
class SensorEventToOneTrack :  public SensorServiceUtil::Dumpable, public virtual RefBase {
public:
    //~SensorEventToOneTrack();
    SensorEventToOneTrack();
    void SaveSensorUsageItem(int32_t sensorType, const String8& sensorName,
                                       const String8& packageName, bool activate);
    void handleSensorUsageTime();
    void handleAppSensorUsageTime();
    void handleProxUsageTime(const sensors_event_t& event, bool report);
    void handlePocModeUsageTime(const sensors_event_t& event, bool report);
    void handleSensorEvent(const sensors_event_t& event);
    void handleDeviceOrientationLastEventTime();
    std::string dump() const override;
    void freeTimeVector();
    void getSensorUsageTime();
    void getAppUsingSensorTime();
    void getSensorUsageTimeList();
    void getAppUsingSensorTimeList();
    void getSensorUsingByAppTime();
    void handleNotDisableSensorUsageTime();
    static void* timeToReport(void *arg);
    void setTimer(time_t timer_count);
    static void onetrackApiProcess(union sigval param) {
        auto *obj = static_cast<SensorEventToOneTrack *>(param.sival_ptr);
        obj->onetrackApi();
    }
    void onetrackApi();
    bool enableSensorEventToOnetrack();
private:
    bool onetrackProp;
    bool deviceOrientProp;
    int FLAG;
    vector<savedItem_I> mSavedItemVector;
    vector<savedItem_I> mProxVector;
    vector<savedItem_I> mPocModeVector;
    time_t mRealtimeSec;
    timespec curTime;
    map<String8, time_t> sensorTimeStartSec;
    map<int32_t, time_t> sensorTypeTimeStartSec;
    bool mActivated;
    map<String8, int> mUserCount;
    map<int32_t, int> mSensorTypeUserCount;
    vector<sensorTime_t> mSensorTimeVector;
    vector<sensorTypeTime_t> mSensorTypeTimeVector;
    map<string, string> SensorTime;
    vector<map<string, string>> SensorTimeList;
    vector<map<string, string>> SensorTypeTimeList;
    map<String8, time_t>::iterator iterSensorName;
    map<int32_t, time_t>::iterator iterSensorType;
    vector<sensorTime_t>::iterator iterSensorTime;
    vector<sensorTypeTime_t>::iterator iterSensorTypeTime;
    map<String8, map<String8, int>> mAppUserCount;
    map<String8, map<int32_t, int>> mAppSensorTypeUserCount;
    map<String8, map<int32_t, int>> mProxUserCount;
    map<String8, map<int32_t, int>> mPocModeUserCount;
    vector<appSensorTime_t> mAppSensorTimeVector;
    vector<appSensorTypeTime_t> mAppSensorTypeTimeVector;
    map<String8, map<String8, time_t>>::iterator iterAppPackage;
    map<String8, map<int32_t, time_t>>::iterator iterAppSensorTypePackage;
    map<String8, map<String8, time_t>> mAppRealtimeStartSec;
    map<String8, map<int32_t, time_t>> mAppSensorTypeStartSec;
    vector<appSensorTime_t>::iterator iterAppSensorTime;
    vector<appSensorTypeTime_t>::iterator iterAppSensorTypeTime;
    map<string, map<string, string>> appUseSensorTime;
    vector<map<string, string>> appUseSensorTimeList;
    vector<map<string, string>> appUseSensorTypeTimeList;
    map<string, map<string, string>> sensorUsedByAppTime;
    vector <float> deviceOrientData;
    time_t deviceOrientVerticalStartSec;
    time_t deviceOrientLandscapeStartSec;
    vector <time_t> deviceOrientVerticalTime;
    vector <time_t> deviceOrientLandscapeTime;
    int deviceOrientLastVerticalFlag;
    int deviceOrientVerticalFlag;
    vector <int> prxSensorData;
    time_t prxStartSec;
    time_t awayStartSec;
    vector <time_t> prxTime;
    vector <time_t> awayTime;
    int prxFlag;
    int prxRegFlag;
    vector <int> prxTimes;
    vector <int> awayTimes;
    vector <time_t> totalPrxTime;
    vector <time_t> totalAwayTime;
    int awayFirstTimes;
    int callTimes;
    vector <time_t> firstPrxTime;
#ifdef REPORTFIRSTPRXTIME
    int firstPrxTimeFlag;
#endif
    time_t lastPrxTime;
    map<string, string> prxSensorTimeItem;
    map<String8, time_t> prxClientStaMsec;
    int invalidTimes;
    vector <float> pocketModeData;
    time_t pocketModeStartMsec;
    int pocModeRegFlag;
    int pocketModeFlag;
    int pocketModeLastFlag;
    vector <time_t> pocketModeTime;
    int regPocCount;

    vector<collDet_c> collDetData;
};
sp<SensorEventToOneTrack> defaultSensorEventToOneTrack();
}; // namespace android

