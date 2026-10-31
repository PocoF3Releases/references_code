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


#include "SensorEventToOneTrack.h"
#include <utils/String8.h>
#include <utils/String16.h>
#include <ctime>
#include <log/log.h>
#include <map>
#include <json/json.h>
#include <iostream>
#include <cutils/properties.h>
#include <numeric>
#include "MQSServiceManager.h"
using namespace std;
using namespace Json;
namespace android {
Mutex mSensorEventToOneTrackLock;
sp<SensorEventToOneTrack> mInstance = nullptr;
#define TIME_THRESHOLD 86500 //second check for judge time is over 24h or not

SensorEventToOneTrack::SensorEventToOneTrack() {
    FLAG = 1 << 1;
    deviceOrientVerticalFlag = -1;
    deviceOrientLastVerticalFlag = -1;
    mRealtimeSec = 0;
    mActivated = -1;
    deviceOrientVerticalStartSec = 0;
    deviceOrientLandscapeStartSec = 0;
    prxFlag = -1;
    awayFirstTimes = 0;
    callTimes = 0;
    lastPrxTime = 0;
    prxRegFlag = -1;
    prxStartSec = 0;
    awayStartSec = 0;
#ifdef REPORTFIRSTPRXTIME
    firstPrxTimeFlag = -1;
#endif
    invalidTimes = 0;
    pocketModeStartMsec = 0;
    pocModeRegFlag = -1;
    pocketModeFlag = -1;
    pocketModeLastFlag = -1;
    regPocCount = 0;
    onetrackProp = property_get_bool("persist.vendor.sensor.onetrack", false);
    deviceOrientProp = property_get_bool("persist.vendor.sensor.deviceorient", false);
    ALOGI("sensorservice SensorEventToOneTrack:onetrackProp:%d", onetrackProp);
    ALOGI("sensorservice SensorEventToOneTrack:deviceOrientProp:%d", deviceOrientProp);
}
sp<SensorEventToOneTrack> defaultSensorEventToOneTrack()
{
    if (mInstance) return mInstance;
    {
        AutoMutex _l(mSensorEventToOneTrackLock);
        if (!mInstance) {
            mInstance = new SensorEventToOneTrack();
        }
     }
    return mInstance;
}
void SensorEventToOneTrack::freeTimeVector() {
    mSavedItemVector.clear();
    mSensorTimeVector.clear();
    mAppSensorTimeVector.clear();
    mSensorTypeTimeVector.clear();
    mAppSensorTypeTimeVector.clear();
    SensorTimeList.clear();
    appUseSensorTimeList.clear();
    SensorTypeTimeList.clear();
    appUseSensorTypeTimeList.clear();
    deviceOrientData.clear();
    deviceOrientVerticalTime.clear();
    deviceOrientLandscapeTime.clear();
    prxSensorTimeItem.clear();
    prxTimes.clear();
    awayTimes.clear();
    totalPrxTime.clear();
    totalAwayTime.clear();
    awayFirstTimes = 0;
    callTimes = 0;
#ifdef REPORTFIRSTPRXTIME
    firstPrxTime.clear();
    firstPrxTimeFlag = -2;
#endif
    pocketModeTime.clear();
    pocketModeData.clear();
    regPocCount = 0;
    collDetData.clear();
}

void SensorEventToOneTrack::SaveSensorUsageItem(int32_t sensorType, const String8& sensorName,
                                       const String8& packageName,
                                       bool activate){
    clock_gettime(CLOCK_REALTIME_COARSE, &curTime);
    savedItem_I saveditem;
    savedItem_I savedProxItem;
    savedItem_I savedPocModeItem;
    saveditem.packageName = packageName;
    saveditem.sensorName = sensorName;
    saveditem.sensorType = sensorType;
    saveditem.timeSec = curTime.tv_sec;
    saveditem.timeNsec = curTime.tv_nsec;
    saveditem.activate = activate;
    mSavedItemVector.push_back(saveditem);
    if(sensorType == SENSOR_TYPE_PROXIMITY){
        savedProxItem.packageName = packageName;
        savedProxItem.sensorName = sensorName;
        savedProxItem.sensorType = sensorType;
        savedProxItem.timeSec = curTime.tv_sec;
        savedProxItem.timeNsec = curTime.tv_nsec;
        savedProxItem.activate = activate;
        mProxVector.push_back(savedProxItem);
    }
    if(sensorType == 33171095){
        savedPocModeItem.packageName = packageName;
        savedPocModeItem.sensorName = sensorName;
        savedPocModeItem.sensorType = sensorType;
        savedPocModeItem.timeSec = curTime.tv_sec;
        savedPocModeItem.timeNsec = curTime.tv_nsec;
        savedPocModeItem.activate = activate;
        mPocModeVector.push_back(savedPocModeItem);
    }
}

void SensorEventToOneTrack::handleSensorUsageTime()
{
    int n = mSavedItemVector.size();
    //ALOGI("SensorEventToOneTrack handleSensorUsageTime:n:%d", n);
    for(int i = 0; i < n; i++)
    {
        mActivated = mSavedItemVector[i].activate;
        if (mActivated)
        {
            iterSensorName = sensorTimeStartSec.find(mSavedItemVector[i].sensorName);
            iterSensorType = sensorTypeTimeStartSec.find(mSavedItemVector[i].sensorType);
            if (iterSensorName == sensorTimeStartSec.end() ||
                mUserCount[mSavedItemVector[i].sensorName] == 0)
            {
                sensorTimeStartSec[mSavedItemVector[i].sensorName] = mSavedItemVector[i].timeSec;
            }
            if (iterSensorType == sensorTypeTimeStartSec.end() ||
                mSensorTypeUserCount[mSavedItemVector[i].sensorType] == 0)
            {
                sensorTypeTimeStartSec[mSavedItemVector[i].sensorType] = mSavedItemVector[i].timeSec;
            }
            mUserCount[mSavedItemVector[i].sensorName]++;
            // ALOGD("SensorEventToOneTrack:handleSensorUsageTime:sensorName:%s,UserCount:%d,activate:%d",
            // sensorName.c_str(),mUserCount[sensorName],activate);
            mSensorTypeUserCount[mSavedItemVector[i].sensorType]++;
        }
        else
        {
            mUserCount[mSavedItemVector[i].sensorName]--;
            // ALOGD("SensorEventToOneTrack:handleSensorUsageTime:sensorName:%s,mUserCount:%d,activate:%d",
            // sensorName.c_str(),mUserCount[sensorName],activate);
            mSensorTypeUserCount[mSavedItemVector[i].sensorType]--;
            if (mUserCount[mSavedItemVector[i].sensorName] == 0)
            {
                sensorTime_t sensortime;
                sensortime.sensorName = mSavedItemVector[i].sensorName;
                sensortime.mDeltatimeSec = mSavedItemVector[i].timeSec -
                                           sensorTimeStartSec[mSavedItemVector[i].sensorName];
                iterSensorTime =
                    find(mSensorTimeVector.begin(), mSensorTimeVector.end(),
                    mSavedItemVector[i].sensorName);
                if (iterSensorTime != mSensorTimeVector.end())
                {
                    iterSensorTime->mDeltatimeSec += sensortime.mDeltatimeSec;
                }
                else
                {
                    mSensorTimeVector.push_back(sensortime);
                }
            }
            if (mSensorTypeUserCount[mSavedItemVector[i].sensorType] == 0)
            {
                sensorTypeTime_t sensortypetime;
                sensortypetime.sensorType = mSavedItemVector[i].sensorType;
                sensortypetime.mDeltatimeSec = mSavedItemVector[i].timeSec -
                                               sensorTypeTimeStartSec[mSavedItemVector[i].sensorType];
                iterSensorTypeTime =
                    find(mSensorTypeTimeVector.begin(), mSensorTypeTimeVector.end(),
                    mSavedItemVector[i].sensorType);
                if (iterSensorTypeTime != mSensorTypeTimeVector.end())
                {
                    iterSensorTypeTime->mDeltatimeSec += sensortypetime.mDeltatimeSec;
                }
                else
                {
                    mSensorTypeTimeVector.push_back(sensortypetime);
                }
            }
        }
    }
}
void SensorEventToOneTrack::handleAppSensorUsageTime() {
    int32_t sensorType;
    String8 sensorName;
    String8 packageName;
    bool activate;
    time_t timeSec;
    int n;
    n = mSavedItemVector.size();
    //ALOGI("SensorEventToOneTrack handleAppSensorUsageTime:n:%d", n);
    for(int i = 0; i < n; i++){
        sensorType = mSavedItemVector[i].sensorType;
        sensorName = mSavedItemVector[i].sensorName;
        packageName = mSavedItemVector[i].packageName;
        activate = mSavedItemVector[i].activate;
        timeSec = mSavedItemVector[i].timeSec;
        //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:sensor:%s,app:%s,activate:%d",
        //sensorName.c_str(), packageName.c_str(), activate);
        if (activate) {
            iterAppPackage = mAppRealtimeStartSec.find(packageName);
            iterAppSensorTypePackage = mAppSensorTypeStartSec.find(packageName);
            if (iterAppPackage == mAppRealtimeStartSec.end()) {
                mAppRealtimeStartSec[packageName][sensorName] = timeSec;
            } else {
                iterSensorName = iterAppPackage->second.find(sensorName);
                if (iterSensorName == iterAppPackage->second.end() ||
                    mAppUserCount[packageName][sensorName] == 0)
                    mAppRealtimeStartSec[packageName][sensorName] = timeSec;
            }
            if (iterAppSensorTypePackage == mAppSensorTypeStartSec.end()) {
                mAppSensorTypeStartSec[packageName][sensorType] = timeSec;
            } else {
                iterSensorType = iterAppSensorTypePackage->second.find(sensorType);
                if (iterSensorType == iterAppSensorTypePackage->second.end() ||
                    mAppSensorTypeUserCount[packageName][sensorType] == 0)
                    mAppSensorTypeStartSec[packageName][sensorType] = timeSec;
            }
            mAppUserCount[packageName][sensorName]++;
            //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:sensor:%s,app:%s,UserCount:%d,activate:%d",
            //sensorName.c_str(), packageName.c_str(), mAppUserCount[packageName][sensorName],activate);
            mAppSensorTypeUserCount[packageName][sensorType]++;
        } else {
            mAppUserCount[packageName][sensorName]--;
            //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:sensor:%s,app:%s,UserCount:%d,activate:%d",
            //sensorName.c_str(), packageName.c_str(), mAppUserCount[packageName][sensorName],activate);
            mAppSensorTypeUserCount[packageName][sensorType]--;
            if (mAppUserCount[packageName][sensorName] == 0) {
                appSensorTime_t appSensorTime;
                appSensorTime.mDeltatimeSec = 0;
                appSensorTime.packageName = packageName;
                appSensorTime.sensorName = sensorName;
                iterAppPackage = mAppRealtimeStartSec.find(packageName);
                if (iterAppPackage != mAppRealtimeStartSec.end()) {
                    appSensorTime.mDeltatimeSec =
                            timeSec - mAppRealtimeStartSec[packageName][sensorName];
                    mAppRealtimeStartSec[packageName].erase(sensorName);
                }
                iterAppSensorTime =
                        find(mAppSensorTimeVector.begin(), mAppSensorTimeVector.end(), appSensorTime);
                if (iterAppSensorTime != mAppSensorTimeVector.end()) {
                    iterAppSensorTime->mDeltatimeSec += appSensorTime.mDeltatimeSec;
                } else {
                    mAppSensorTimeVector.push_back(appSensorTime);
                }
            }
            if (mAppSensorTypeUserCount[packageName][sensorType] == 0) {
                appSensorTypeTime_t appSensorTypeTime;
                appSensorTypeTime.mDeltatimeSec = 0;
                appSensorTypeTime.packageName = packageName;
                appSensorTypeTime.sensorType = sensorType;
                iterAppSensorTypePackage = mAppSensorTypeStartSec.find(packageName);
                if (iterAppSensorTypePackage != mAppSensorTypeStartSec.end()) {
                    appSensorTypeTime.mDeltatimeSec =
                            timeSec - mAppSensorTypeStartSec[packageName][sensorType];
                    mAppSensorTypeStartSec[packageName].erase(sensorType);
                }
                iterAppSensorTypeTime =
                        find(mAppSensorTypeTimeVector.begin(), mAppSensorTypeTimeVector.end(),
                        appSensorTypeTime);
                if (iterAppSensorTypeTime != mAppSensorTypeTimeVector.end()) {
                    iterAppSensorTypeTime->mDeltatimeSec += appSensorTypeTime.mDeltatimeSec;
                } else {
                    mAppSensorTypeTimeVector.push_back(appSensorTypeTime);
                }
            }
        }
    }
}

void SensorEventToOneTrack::handleProxUsageTime(const sensors_event_t& event, bool report) {
    if (event.type == SENSOR_TYPE_PROXIMITY || report) {
        int32_t sensorType;
        String8 sensorName;
        String8 packageName;
        bool activate;
        time_t timeSec;
        time_t timeNsec;
        int n;
        n = mProxVector.size();
        if(n != 0){
            for(int i = 0; i < n; i++){
                sensorType = mProxVector[i].sensorType;
                sensorName = mProxVector[i].sensorName;
                packageName = mProxVector[i].packageName;
                activate = mProxVector[i].activate;
                timeSec = mProxVector[i].timeSec;
                timeNsec = mProxVector[i].timeNsec;
                //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:sensor:%s,app:%s,activate:%d",
                //sensorName.c_str(), packageName.c_str(), activate);
                if (activate) {
                    mProxUserCount[packageName][sensorType] ++;
                    String8 packageNameWeixin =
                            String8("com.tencent.mm.sdk.platformtools.SensorController");
                    String8 packageNamePhone =
                            String8("com.android.server.display.DisplayPowerController");
                    if (packageName == packageNamePhone
                            || packageName == packageNameWeixin)  {
                        prxRegFlag = 1;
                        if (packageName == packageNamePhone &&
                                (mProxUserCount[packageNameWeixin][sensorType] == 0 ||
                                prxFlag == 0)) {
                            prxClientStaMsec[packageNamePhone] = (timeSec * 1000) +
                                                    (timeNsec / 1000000);
                        } else if (packageName == packageNameWeixin) {
                            prxClientStaMsec[packageNameWeixin] = (timeSec * 1000) +
                                                    (timeNsec / 1000000);
                        }
                    }
                } else {
                    mProxUserCount[packageName][sensorType] --;
                    String8 packageNameWeixin =
                            String8("com.tencent.mm.sdk.platformtools.SensorController");
                    String8 packageNamePhone =
                            String8("com.android.server.display.DisplayPowerController");
                    //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:sensor:%s,app:%s,activate:%d",
                            //sensorName.c_str(), packageName.c_str(), activate);
                    if ((packageName == packageNamePhone &&
                            mProxUserCount[packageNameWeixin][sensorType] == 0)
                            || packageName == packageNameWeixin) {
                        if (mAppSensorTypeUserCount[packageNameWeixin][sensorType] == 0
                                || mProxUserCount[packageNamePhone][sensorType] == 0) {
                            if (!(count(prxSensorData.begin(), prxSensorData.end(), 0) == 0 &&
                                    count(prxSensorData.begin(), prxSensorData.end(), 5) == 0)) {
                                prxTimes.push_back(count(prxSensorData.begin(), prxSensorData.end(), 0));
                                awayTimes.push_back(count(prxSensorData.begin(), prxSensorData.end(), 5));
                            }
                            //for (int i = 0; i < prxTimes.size(); i++) {
                                //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:prxTimes[%d]:%d", i ,
                                        //prxTimes[i]);
                            //}
                            //for (int i = 0; i < awayTimes.size(); i++) {
                                //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:awayTimes[%d]:%d", i ,
                                        //awayTimes[i]);
                            //}
                            if (prxFlag == 1) {
                                if((timeSec - prxStartSec) >= 0 &&
                                        (timeSec - prxStartSec) <= TIME_THRESHOLD) {
                                    prxTime.push_back(timeSec - prxStartSec);
                                    //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:prxTime:%ld",
                                            //prxTime[prxTime.size()-1]);
                                }
                            } else if (prxFlag == 0) {
                                if((timeSec - awayStartSec) >= 0 &&
                                        (timeSec - awayStartSec) <= TIME_THRESHOLD) {
                                    awayTime.push_back(timeSec - awayStartSec);
                                    //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:awayTime:%ld",
                                            //awayTime[awayTime.size()-1]);
                                }
                            }
                            if (!(accumulate(prxTime.begin(), prxTime.end(),0) == 0 &&
                                    accumulate(awayTime.begin(), awayTime.end(),0) == 0) &&
                                    totalPrxTime.size() == (prxTimes.size() - 1)) {
                                totalPrxTime.push_back(accumulate(prxTime.begin(), prxTime.end(),0));
                                totalAwayTime.push_back(accumulate(awayTime.begin(), awayTime.end(),0));
                            }
                            //for (int i = 0; i < totalPrxTime.size(); i++) {
                                //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:totalPrxTime[%d]:%ld",i,
                                        //totalPrxTime[i]);
                            //}
                            //for (int i = 0; i < totalAwayTime.size(); i++) {
                                //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:totalAwayTime[%d]:%ld",i,
                                        //totalAwayTime[i]);
                            //}
                            if ((packageName == packageNamePhone &&
                                mProxUserCount[packageNameWeixin][sensorType] == 0 &&
                                prxClientStaMsec[packageNamePhone] != 0 &&
                                (((timeSec * 1000) + (timeNsec/1000000)) -
                                prxClientStaMsec[packageNamePhone]) > 600) ||
                                (packageName == packageNameWeixin &&
                                prxClientStaMsec[packageNameWeixin] != 0 &&
                                (((timeSec * 1000) + (timeNsec/1000000)) -
                                prxClientStaMsec[packageNameWeixin]) > 600)) {
                                if (prxSensorData.size() != 0) {
                                    if (*(prxSensorData.begin()) == 5 )
                                        awayFirstTimes ++;
                                } else {
                                    invalidTimes ++;
                                }
                                callTimes ++;
                                prxClientStaMsec.erase(packageName);
                                //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:awayFirstTimes:%d,callTimes:%d",
                                        //awayFirstTimes, callTimes);
                            }
                            prxSensorData.clear();
                            prxTime.clear();
                            awayTime.clear();
                            mProxVector.clear();
                         }
                        if (mProxUserCount[packageNameWeixin][sensorType] == 0
                                    && mProxUserCount[packageNamePhone][sensorType] == 0) {
                            prxRegFlag = 0;
                            prxFlag = -1;
                            prxStartSec = -1;
                            awayStartSec = -1;
#ifdef REPORTFIRSTPRXTIME
                            firstPrxTimeFlag = -1;
#endif
                        } else if (mProxUserCount[packageNameWeixin][sensorType] == 0
                                && mProxUserCount[packageNamePhone][sensorType] != 0) {
                            if (prxFlag == 0) {
                                prxSensorData.push_back(5);
                                awayStartSec = timeSec;
                            }
                        }
                    }
                }
            }
        }
        mProxVector.clear();
    }
}
void SensorEventToOneTrack::handlePocModeUsageTime(const sensors_event_t& event, bool report) {
    if (event.type == 33171095 || report) {
        int32_t sensorType;
        String8 sensorName;
        String8 packageName;
        bool activate;
        time_t timeSec;
        time_t timeNsec;
        String8 packageNamePocMode =
            String8("com.android.server.input.pocketmode.MiuiPocketModeSensorWrapper");
        int n;
        n = mPocModeVector.size();
        if(n != 0){
            for(int i = 0; i < n; i++){
                sensorType = mPocModeVector[i].sensorType;
                sensorName = mPocModeVector[i].sensorName;
                packageName = mPocModeVector[i].packageName;
                activate = mPocModeVector[i].activate;
                timeSec = mPocModeVector[i].timeSec;
                timeNsec = mPocModeVector[i].timeNsec;
                //ALOGD("SensorEventToOneTrack:poc:sensor:%s,app:%s,activate:%d,usercount:%d",
                    //sensorName.c_str(), packageName.c_str(), activate,
                    //mPocModeUserCount[packageName][sensorType]);
                if (activate) {
                    mPocModeUserCount[packageName][sensorType] ++;
                    if (packageName == packageNamePocMode)  {
                        pocModeRegFlag = 1;
                        regPocCount++;
                    }
                } else {
                    mPocModeUserCount[packageName][sensorType] --;
                    if (packageName == packageNamePocMode && pocketModeFlag == 1 &&
                        mPocModeUserCount[packageName][sensorType] == 0) {
                        if(((timeSec * 1000 + timeNsec / 1000000) -pocketModeStartMsec) >= 0 &&
                                    ((timeSec * 1000 + timeNsec / 1000000) -pocketModeStartMsec) <=
                                    TIME_THRESHOLD * 1000) {
                            pocketModeTime.push_back((timeSec * 1000 + timeNsec / 1000000)
                                   -pocketModeStartMsec);
                        }
                        //for (int i = 0; i < pocketModeTime.size(); i++) {
                            //ALOGD("SensorEventToOneTrack:AppSensorUsageTime:pocketModeTime[%d]:%ld",
                            //i,pocketModeTime[i]);
                        //}
                        pocketModeData.clear();
                    }
                }
                if (mPocModeUserCount[packageName][sensorType] == 0) {
                    pocModeRegFlag = 0;
                    pocketModeFlag = -1;
                    pocketModeLastFlag = -1;
                    pocketModeStartMsec = -1;
                    mPocModeVector.clear();
                }
            }
        }
        mPocModeVector.clear();
    }
}

void SensorEventToOneTrack::handleNotDisableSensorUsageTime() {
    int count = 0;
    for (auto& m1 : mUserCount) {
        if (m1.second > 0) {
            count++;
            //ALOGD("SensorEventToOneTrack:not disable sensors and usercount:%s, %d",
                    //m1.first.c_str(), m1.second);
            sensorTime_t sensortime;
            sensortime.sensorName = m1.first;
            clock_gettime(CLOCK_REALTIME_COARSE, &curTime);
            sensortime.mDeltatimeSec = curTime.tv_sec - sensorTimeStartSec[m1.first];
            sensorTimeStartSec[m1.first] = curTime.tv_sec;
            iterSensorTime = find(mSensorTimeVector.begin(), mSensorTimeVector.end(), m1.first);
            if (iterSensorTime != mSensorTimeVector.end()) {
                iterSensorTime->mDeltatimeSec += sensortime.mDeltatimeSec;
            } else {
                mSensorTimeVector.push_back(sensortime);
            }
        }
    }
    //ALOGD("SensorEventToOneTrack:nums of not disable sensors:%d", count);
    for (auto& m4 : mSensorTypeUserCount) {
        if (m4.second > 0) {
            sensorTypeTime_t sensortypetime;
            sensortypetime.sensorType = m4.first;
            clock_gettime(CLOCK_REALTIME_COARSE, &curTime);
            sensortypetime.mDeltatimeSec = curTime.tv_sec - sensorTypeTimeStartSec[m4.first];
            sensorTypeTimeStartSec[m4.first] = curTime.tv_sec;
            iterSensorTypeTime = find(mSensorTypeTimeVector.begin(), mSensorTypeTimeVector.end(),
                m4.first);
            if (iterSensorTypeTime != mSensorTypeTimeVector.end()) {
                iterSensorTypeTime->mDeltatimeSec += sensortypetime.mDeltatimeSec;
            } else {
                mSensorTypeTimeVector.push_back(sensortypetime);
            }
        }
    }
    for (auto& m2 : mAppRealtimeStartSec) {
        if (!m2.second.empty()) {
            for (auto& m3 : m2.second) {
                //ALOGD("SensorEventToOneTrack:handleNotDisableSensorUsageTime:sensor:%s,app:%s",
                        //m3.first.c_str(),m2.first.c_str());
                appSensorTime_t appSensorTime;
                appSensorTime.packageName = m2.first;
                appSensorTime.sensorName = m3.first;
                clock_gettime(CLOCK_REALTIME_COARSE, &curTime);
                appSensorTime.mDeltatimeSec =
                        curTime.tv_sec - mAppRealtimeStartSec[m2.first][m3.first];
                mAppRealtimeStartSec[m2.first][m3.first] = curTime.tv_sec;
                iterAppSensorTime = find(mAppSensorTimeVector.begin(), mAppSensorTimeVector.end(),
                                         appSensorTime);
                if (iterAppSensorTime != mAppSensorTimeVector.end()) {
                    iterAppSensorTime->mDeltatimeSec += appSensorTime.mDeltatimeSec;
                } else {
                    mAppSensorTimeVector.push_back(appSensorTime);
                }
            }
        }
    }
    for (auto& m5 : mAppSensorTypeStartSec) {
        if (!m5.second.empty()) {
            for (auto& m6 : m5.second) {
                appSensorTypeTime_t appSensorTypeTime;
                appSensorTypeTime.packageName = m5.first;
                appSensorTypeTime.sensorType = m6.first;
                clock_gettime(CLOCK_REALTIME_COARSE, &curTime);
                appSensorTypeTime.mDeltatimeSec =
                        curTime.tv_sec - mAppSensorTypeStartSec[m5.first][m6.first];
                mAppSensorTypeStartSec[m5.first][m6.first] = curTime.tv_sec;
                iterAppSensorTypeTime = find(mAppSensorTypeTimeVector.begin(),
                    mAppSensorTypeTimeVector.end(), appSensorTypeTime);
                if (iterAppSensorTypeTime != mAppSensorTypeTimeVector.end()) {
                    iterAppSensorTypeTime->mDeltatimeSec += appSensorTypeTime.mDeltatimeSec;
                } else {
                    mAppSensorTypeTimeVector.push_back(appSensorTypeTime);
                }
            }
        }
    }
}

void SensorEventToOneTrack::handleSensorEvent(const sensors_event_t& event) {
    if (deviceOrientProp == true) {
        if(event.type == SENSOR_TYPE_DEVICE_ORIENTATION) {
            clock_gettime(CLOCK_REALTIME_COARSE, &curTime);
            deviceOrientData.push_back(event.data[0]);
            if (event.data[0] == 0.0f || event.data[0] == 2.0f) {
                deviceOrientVerticalFlag = 1;
            } else if (event.data[0] == 1.0f || event.data[0] == 3.0f) {
                deviceOrientVerticalFlag = 0;
            }
            if (deviceOrientData.size() == 1 && deviceOrientLastVerticalFlag == -1) {
                if (deviceOrientVerticalFlag) {
                    deviceOrientVerticalStartSec = curTime.tv_sec;
                } else {
                    deviceOrientLandscapeStartSec = curTime.tv_sec;
                }
            } else if (deviceOrientData.size() > 1) {
                if (*(deviceOrientData.end() - 2) == 0.0f ||
                    *(deviceOrientData.end() - 2) == 2.0f) {
                    deviceOrientLastVerticalFlag = 1;
                } else if (*(deviceOrientData.end() - 2) == 1.0f ||
                           *(deviceOrientData.end() - 2) == 3.0f) {
                    deviceOrientLastVerticalFlag = 0;
                }
            }
            if (deviceOrientLastVerticalFlag == 1 && deviceOrientVerticalFlag == 0) {
                deviceOrientVerticalTime.push_back(curTime.tv_sec -deviceOrientVerticalStartSec);
                //ALOGD("SensorEventToOneTrack:handleSensorEvent:deviceOrientVerticalTime:%ld",
                    //deviceOrientVerticalTime[deviceOrientVerticalTime.size()-1]);
                deviceOrientLandscapeStartSec = curTime.tv_sec;
            } else if (deviceOrientLastVerticalFlag == 0 && deviceOrientVerticalFlag == 1) {
                deviceOrientLandscapeTime.push_back(curTime.tv_sec -deviceOrientLandscapeStartSec);
                //ALOGD("SensorEventToOneTrack:handleSensorEvent:deviceOrientLandscapeTime:%ld",
                    //deviceOrientLandscapeTime[deviceOrientLandscapeTime.size()-1]);
                deviceOrientVerticalStartSec = curTime.tv_sec;
            }
        }
    }
    if (event.type == SENSOR_TYPE_PROXIMITY) {
        //ALOGD("SensorEventToOneTrack:handleSensorEvent:prx:%f",
                    //event.data[0]);
        if (prxRegFlag == 1)  {
            clock_gettime(CLOCK_REALTIME_COARSE, &curTime);
            prxSensorData.push_back((int)event.data[0]);
            if ((int)event.data[0] == 5) {
                prxFlag = 0;
            } else if ((int)event.data[0] == 0) {
                prxFlag = 1;
            }
            if (prxFlag == 1) {
                awayTime.push_back(curTime.tv_sec - awayStartSec);
                //ALOGD("SensorEventToOneTrack:handleSensorEvent:awayTime:%ld",
                        //awayTime[awayTime.size()-1]);
                prxStartSec = curTime.tv_sec;
#if REPORTFIRSTPRXTIME
                if (firstPrxTimeFlag == -1 && prxSensorData.size() > 1) {
                    firstPrxTime.push_back(*(awayTime.end() - 1) + lastPrxTime);
                    firstPrxTimeFlag = 0;
                    ALOGD("SensorEventToOneTrack:handleSensorEvent:awayTime:%ld,lastPrxTime:%ld",
                            *(awayTime.end() - 1), lastPrxTime);
                    for (int i = 0; i < firstPrxTime.size(); i++) {
                        ALOGD("SensorEventToOneTrack:handleSensorEvent:firstPrxTime:%ld",
                                firstPrxTime[i]);
                    }
                }
#endif
            } else if (prxFlag == 0) {
                if (prxSensorData.size() > 1) {
                    prxTime.push_back(curTime.tv_sec - prxStartSec);
                    //ALOGD("SensorEventToOneTrack:handleSensorEvent:prxTime:%ld",
                        //prxTime[prxTime.size()-1]);
                }
                awayStartSec = curTime.tv_sec;
            }
        }
    }
    if (event.type == 33171095) {
        //ALOGD("SensorEventToOneTrack:handleSensorEvent:pocket:%f", event.data[0]);
        if (pocModeRegFlag == 1)  {
            String8 packageNamePocMode =
                String8("com.android.server.input.pocketmode.MiuiPocketModeSensorWrapper");
            clock_gettime(CLOCK_REALTIME_COARSE, &curTime);
            pocketModeData.push_back(event.data[0]);
            if (event.data[0] == 5.0f) {
                pocketModeFlag = 0;
            } else if (event.data[0] == 0.0f) {
                pocketModeFlag = 1;
            }
            //ALOGD("SensorEventToOneTrack:handleSensorEvent:pocket:pocketModeFlag:%d,pocketModeLastFlag:%d",
                    //pocketModeFlag, pocketModeLastFlag);
            if(pocketModeFlag == 1 && (pocketModeLastFlag == 0 || pocketModeLastFlag == -1)) {
                pocketModeStartMsec = curTime.tv_sec * 1000 + curTime.tv_nsec / 1000000;
                pocketModeLastFlag = 1;
            }
            //ALOGD("SensorEventToOneTrack:handleSensorEvent:pocket:pocketModeStartMsec:%ld,curTime:%ld",
                    //pocketModeStartMsec, (curTime.tv_sec * 1000 + curTime.tv_nsec / 1000000));
            if(pocketModeLastFlag == 1 && pocketModeFlag == 0 &&
                mPocModeUserCount[packageNamePocMode][33171095] != 0
                && (((curTime.tv_sec * 1000 + curTime.tv_nsec / 1000000) -pocketModeStartMsec) >= 0
                && ((curTime.tv_sec * 1000 + curTime.tv_nsec / 1000000) -pocketModeStartMsec) <=
                    TIME_THRESHOLD * 1000)) {
                pocketModeTime.push_back((curTime.tv_sec * 1000 + curTime.tv_nsec / 1000000)
                    -pocketModeStartMsec);
                pocketModeLastFlag = 0;
                //ALOGD("SensorEventToOneTrack:handleSensorEvent:pocketModeTime:%ld",
                    //pocketModeTime[pocketModeTime.size()-1]);
            }
        }
    }

    if (event.type == 33171115) {
        //ALOGD("SensorEventToOneTrack:handleSensorEvent:collDetData:%f, %f", event.data[0], event.data[1]);
        time_t curTimeCollDet;
        struct tm* timeinfoCollDet;
        time(&curTimeCollDet);
        timeinfoCollDet = localtime(&curTimeCollDet);
        string collDetTime;
        stringstream ss;
        ss << timeinfoCollDet -> tm_year + 1900 << "-" << timeinfoCollDet -> tm_mon + 1 << "-" << timeinfoCollDet ->
        tm_mday << " " << timeinfoCollDet -> tm_hour << ":" << timeinfoCollDet -> tm_min << ":" << timeinfoCollDet -> tm_sec;
        collDetTime = ss.str();
        //ALOGD("SensorEventToOneTrack collDetTime:%s", collDetTime.c_str());
        collDet_c collDet;
        collDet.hight = event.data[0];
        collDet.posture = event.data[1];
        collDet.time = collDetTime;
        collDetData.push_back(collDet);
    }
}

void SensorEventToOneTrack::handleDeviceOrientationLastEventTime() {
    clock_gettime(CLOCK_REALTIME_COARSE, &curTime);
    if (deviceOrientVerticalFlag == 1) {
        deviceOrientVerticalTime.push_back(curTime.tv_sec -deviceOrientVerticalStartSec);
        deviceOrientVerticalStartSec = curTime.tv_sec;
        deviceOrientLastVerticalFlag = 1;
    } else if (deviceOrientVerticalFlag == 0) {
        deviceOrientLandscapeTime.push_back(curTime.tv_sec -deviceOrientLandscapeStartSec);
        deviceOrientLandscapeStartSec = curTime.tv_sec;
        deviceOrientLastVerticalFlag = 0;
    }
}
void SensorEventToOneTrack::getSensorUsageTime() {
    int n = mSensorTypeTimeVector.size();
    for (int i = 0; i < n; i++) {
        SensorTime[to_string(mSensorTypeTimeVector[i].sensorType)] =
            to_string(mSensorTypeTimeVector[i].mDeltatimeSec);
    }
}
void SensorEventToOneTrack::getAppUsingSensorTime() {
    int n = mAppSensorTypeTimeVector.size();
    for (int i = 0; i < n; i++) {
        appUseSensorTime[mAppSensorTypeTimeVector[i].packageName.string()]
        [to_string(mAppSensorTypeTimeVector[i].sensorType)]
            = to_string(mAppSensorTypeTimeVector[i].mDeltatimeSec);
    }
}
void SensorEventToOneTrack::getSensorUsingByAppTime() {
    int n = mAppSensorTypeTimeVector.size();
    for (int i = 0; i < n; i++) {
        sensorUsedByAppTime[to_string(mAppSensorTypeTimeVector[i].sensorType)]
        [mAppSensorTypeTimeVector[i].packageName.string()]
            = to_string(mAppSensorTypeTimeVector[i].mDeltatimeSec);
    }
}

void SensorEventToOneTrack::getSensorUsageTimeList() {
    int n = mSensorTimeVector.size();
    int m = mSensorTypeTimeVector.size();
    map<string, string> SensorTimeItem;
    map<string, string> SensorTypeTimeItem;
    for (int i = 0; i < n; i++) {
        SensorTimeItem["sensor_name"] = mSensorTimeVector[i].sensorName.string();
        SensorTimeItem["time"] = to_string(mSensorTimeVector[i].mDeltatimeSec);
        if(mSensorTimeVector[i].mDeltatimeSec > 0 && mSensorTimeVector[i].mDeltatimeSec
            <= TIME_THRESHOLD){
            SensorTimeList.push_back(SensorTimeItem);
        } else {
            //ALOGE("SensorEventToOneTrack getSensorUsageTimeList: SensorTime: %ld",
            //mSensorTimeVector[i].mDeltatimeSec);
        }
    }
    for (int j = 0; j < m; j++) {
        SensorTypeTimeItem["sensor_type"] =  to_string(mSensorTypeTimeVector[j].sensorType);
        SensorTypeTimeItem["time"] = to_string(mSensorTypeTimeVector[j].mDeltatimeSec);
        if(mSensorTypeTimeVector[j].mDeltatimeSec > 0 && mSensorTypeTimeVector[j].mDeltatimeSec
            <= TIME_THRESHOLD){
            SensorTypeTimeList.push_back(SensorTypeTimeItem);
        } else {
            //ALOGE("sensorservice SensorEventToOneTrack getSensorUsageTimeList: SensorTypeTime: %ld",
                //mSensorTypeTimeVector[j].mDeltatimeSec);
        }
    }
}
void SensorEventToOneTrack::getAppUsingSensorTimeList() {
    int n = mAppSensorTimeVector.size();
    int m = mAppSensorTypeTimeVector.size();
    map<string, string> appUseSensorTimeItem;
    map<string, string> appUseSensorTypeTimeItem;
    for (int i = 0; i < n; i++) {
        appUseSensorTimeItem["app_name"] = mAppSensorTimeVector[i].packageName.string();
        appUseSensorTimeItem["sensor_name"] = mAppSensorTimeVector[i].sensorName.string();
        appUseSensorTimeItem["time"] = to_string(mAppSensorTimeVector[i].mDeltatimeSec);
        if(mAppSensorTimeVector[i].mDeltatimeSec > 0 && mAppSensorTimeVector[i].mDeltatimeSec
            <= TIME_THRESHOLD){
            appUseSensorTimeList.push_back(appUseSensorTimeItem);
        } else {
            //ALOGE("SensorEventToOneTrack getAppUsingSensorTimeList: time: %ld",
                //mAppSensorTimeVector[i].mDeltatimeSec);
        }
    }
    for (int j = 0; j < m; j++) {
        appUseSensorTypeTimeItem["app_name"] = mAppSensorTypeTimeVector[j].packageName.string();
        appUseSensorTypeTimeItem["sensor_type"] =  to_string(mAppSensorTypeTimeVector[j].sensorType);
        appUseSensorTypeTimeItem["time"] = to_string(mAppSensorTypeTimeVector[j].mDeltatimeSec);
        if(mAppSensorTypeTimeVector[j].mDeltatimeSec > 0 && mAppSensorTimeVector[j].mDeltatimeSec
            <= TIME_THRESHOLD){
            appUseSensorTypeTimeList.push_back(appUseSensorTypeTimeItem);
        } else {
            //ALOGE("SensorEventToOneTrack getAppUsingSensorTimeList: SensorTypeTime:%ld",
                //mAppSensorTimeVector[j].mDeltatimeSec);
        }
    }
}
void SensorEventToOneTrack::onetrackApi(){
    ALOGI("SensorEventToOneTrack start inside onetrackApi");
    mInstance->handleNotDisableSensorUsageTime();
    //clock_gettime(CLOCK_REALTIME_COARSE, &curTime);
	//time_t a =curTime.tv_nsec;
    //ALOGI("SensorEventToOneTrack onetrackApi:a:%ld",a);
    mInstance->handleSensorUsageTime();
    //clock_gettime(CLOCK_REALTIME_COARSE, &curTime);
	//time_t b =curTime.tv_nsec;
    //ALOGI("SensorEventToOneTrack onetrackApi:b:%ld,diff:%ld",b,b-a);
    mInstance->handleAppSensorUsageTime();
    //clock_gettime(CLOCK_REALTIME_COARSE, &curTime);
	//time_t c =curTime.tv_nsec;
    //ALOGI("SensorEventToOneTrack onetrackApi:c:%ld,diff:%ld",c,c-b);
    sensors_event_t event;
    mInstance->handleProxUsageTime(event, 1);
    mInstance->handlePocModeUsageTime(event, 1);
    mInstance->getSensorUsageTimeList();
    mInstance->getAppUsingSensorTimeList();
    Json::Value jObject_SensorTime;
    Json::Value jObject_SensorTime_1;
    Json::Value jObject_SensorTime_2;
    Json::Value jObject_appUseSensorTime;
    Json::Value jObject_appUseSensorTime_1;
    Json::Value jObject_appUseSensorTime_2;
    Json::StreamWriterBuilder builder;
    const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    for(int i = 0; i < SensorTimeList.size(); i++){
        jObject_SensorTime_1[i]["sensor_name"] = Value(SensorTimeList[i]["sensor_name"]);
        jObject_SensorTime_1[i]["time"] = Value(SensorTimeList[i]["time"]);
    }
    for(int j = 0; j < SensorTypeTimeList.size(); j++){
        jObject_SensorTime_2[j]["sensor_type"] = Value(SensorTypeTimeList[j]["sensor_type"]);
        jObject_SensorTime_2[j]["time"] = Value(SensorTypeTimeList[j]["time"]);
    }
    jObject_SensorTime["sensor_time"] = jObject_SensorTime_1;
    jObject_SensorTime["sensor_type_time"] = jObject_SensorTime_2;
    if (mInstance->deviceOrientProp) {
        mInstance->handleDeviceOrientationLastEventTime();
        if((deviceOrientVerticalTime.size() != 0 || deviceOrientLandscapeTime.size() != 0)){
            Json::Value jObject_DeviceOrientTime;
            Json::Value jObject_DeviceOrientTime_1;
            Json::Value jObject_DeviceOrientTime_2;
            if(deviceOrientVerticalTime.size() != 0){
                for (uint i = 0; i < deviceOrientVerticalTime.size(); i++) {
                    if (deviceOrientVerticalTime[i] >= 0 &&
                        deviceOrientVerticalTime[i] <= TIME_THRESHOLD) {
                        jObject_DeviceOrientTime_1["vertical_time"][i] =
                                                  Value(to_string(deviceOrientVerticalTime[i]));
                    }
                }
                jObject_DeviceOrientTime.append(jObject_DeviceOrientTime_1);
            }
            if(deviceOrientLandscapeTime.size() != 0){
                for (uint i = 0; i < deviceOrientLandscapeTime.size(); i++) {
                    if (deviceOrientLandscapeTime[i] >= 0 &&
                        deviceOrientLandscapeTime[i] <= TIME_THRESHOLD) {
                        jObject_DeviceOrientTime_2["landscape_time"][i] =
                                                 Value(to_string(deviceOrientLandscapeTime[i]));
                    }
                }
                jObject_DeviceOrientTime.append(jObject_DeviceOrientTime_2);
            }
            jObject_SensorTime["device_orient_time"] = jObject_DeviceOrientTime;
        }
    }
#if REPORTFIRSTPRXTIME
    Json::Value jObjectFirstPrxTime;
    if (firstPrxTime.size() != 0) {
        for (int i = 0; i < firstPrxTime.size(); i++) {
            if (firstPrxTime[i] > 0) {
                jObjectFirstPrxTime[i] = Value(to_string(firstPrxTime[i]));
            }
        }
        jObject_SensorTime["firstPrxTime"] = jObjectFirstPrxTime;
    }
#endif
    Json::Value jObjectTotalPrxTime;
    if (totalPrxTime.size() != 0) {
        for (int i = 0; i < totalPrxTime.size(); i++) {
            if (totalPrxTime[i] >= 0 && totalPrxTime[i] <= TIME_THRESHOLD) {
                jObjectTotalPrxTime[i] = Value(to_string(totalPrxTime[i]));
            }
        }
        jObject_SensorTime["prxTime"] = jObjectTotalPrxTime;
    }
    Json::Value jObjectTotalAwayTime;
    if (totalAwayTime.size() != 0) {
        for (int i = 0; i < totalAwayTime.size(); i++) {
            if (totalAwayTime[i] >= 0 && totalAwayTime[i] <= TIME_THRESHOLD) {
                jObjectTotalAwayTime[i] = Value(to_string(totalAwayTime[i]));
            }
        }
        jObject_SensorTime["awayTime"] = jObjectTotalAwayTime;
    }
    Json::Value jObjectPrxTimes;
    if (prxTimes.size() != 0) {
        for (int i = 0; i < prxTimes.size(); i++) {
            jObjectPrxTimes[i] = Value(to_string(prxTimes[i]));
        }
        jObject_SensorTime["prxTimes"] = jObjectPrxTimes;
    }
    Json::Value jObjectAwayTimes;
    if (awayTimes.size() != 0) {
        for (int i = 0; i < awayTimes.size(); i++) {
            jObjectAwayTimes[i] = Value(to_string(awayTimes[i]));
        }
        jObject_SensorTime["awayTimes"] = jObjectAwayTimes;
    }
    Json::Value jObjectAwayFirAndCallTimes;
    if (!(awayFirstTimes == 0 && callTimes == 0)) {
        jObjectAwayFirAndCallTimes[0] = Value(to_string(awayFirstTimes));
        jObjectAwayFirAndCallTimes[1] = Value(to_string(callTimes));
        jObjectAwayFirAndCallTimes[2] = Value(to_string(invalidTimes));
        jObject_SensorTime["awayFirAndCallTimes"] = jObjectAwayFirAndCallTimes;
    }
    Json::Value jObjectPocketMode;
    if (pocketModeTime.size() != 0) {
        for (int i = 0; i < pocketModeTime.size(); i++) {
            jObjectPocketMode[i] = Value(to_string(pocketModeTime[i]));
        }
        jObject_SensorTime["pocketMode"] = jObjectPocketMode;
    }
    Json::Value jObjectRegPocCount;
    if (regPocCount != 0) {
        jObjectRegPocCount = Value(to_string(regPocCount));
        jObject_SensorTime["regPocCount"] = jObjectRegPocCount;
    }

    Json::Value jObject_CollDet;
    for(int i = 0; i < collDetData.size(); i++){
        jObject_CollDet[i]["hight"] = Value(to_string(collDetData[i].hight));
        jObject_CollDet[i]["posture"] = Value(to_string(collDetData[i].posture));
        jObject_CollDet[i]["time"] = Value(collDetData[i].time);
    }
    jObject_SensorTime["collDet"] = jObject_CollDet;

    for(int m = 0; m < appUseSensorTimeList.size(); m++){
        jObject_appUseSensorTime_1[m]["app_name"] = Value(appUseSensorTimeList[m]["app_name"]);
        jObject_appUseSensorTime_1[m]["sensor_name"] =
             Value(appUseSensorTimeList[m]["sensor_name"]);
        jObject_appUseSensorTime_1[m]["time"] = Value(appUseSensorTimeList[m]["time"]);
    }
    for(int n = 0; n < appUseSensorTypeTimeList.size(); n++){
        jObject_appUseSensorTime_2[n]["app_name"] = Value(appUseSensorTypeTimeList[n]["app_name"]);
        jObject_appUseSensorTime_2[n]["sensor_type"] =
            Value(appUseSensorTypeTimeList[n]["sensor_type"]);
        jObject_appUseSensorTime_2[n]["time"] = Value(appUseSensorTypeTimeList[n]["time"]);
    }
    jObject_appUseSensorTime["app_use_sensor_time"] = jObject_appUseSensorTime_1;
    jObject_appUseSensorTime["app_use_sensor_type_time"] = jObject_appUseSensorTime_2;

    jObject_SensorTime["EVENT_NAME"] = Value("SensorTime");
    jObject_appUseSensorTime["EVENT_NAME"] = Value("appUseSensorTime");
    string SensorTime_data = Json::writeString(builder, jObject_SensorTime);
    string appUseSensorTime_data = Json::writeString(builder, jObject_appUseSensorTime);
    //ALOGD("SensorEventToOneTrack onetrackApi: json: %s", SensorTime_data.c_str());
    //ALOGD("SensorEventToOneTrack onetrackApi: json appUseSensorTime: %s",
        //appUseSensorTime_data.c_str());
try{
          MQSServiceManager::getInstance().reportOneTrackEvent(String8("31000000621"),
              String8("sensor"), String8(SensorTime_data.c_str()), FLAG);
          MQSServiceManager::getInstance().reportOneTrackEvent(String8("31000000621"),
              String8("sensor"), String8(appUseSensorTime_data.c_str()), FLAG);
} catch (...){
        ALOGE("SensorEventToOneTrack MQSServiceManager get failed!");
}
    mInstance->freeTimeVector();
    ALOGI("SensorEventToOneTrack end inside onetrackApi");
}

void* SensorEventToOneTrack::timeToReport(void *arg){
    clock_gettime(CLOCK_REALTIME_COARSE, &(mInstance->curTime));
    mInstance->mRealtimeSec = mInstance->curTime.tv_sec;
    struct tm* timeinfo = localtime(&(mInstance->mRealtimeSec));
    const int8_t hour = static_cast<int8_t>(timeinfo->tm_hour);
    const int8_t min = static_cast<int8_t>(timeinfo->tm_min);
    const int8_t sec = static_cast<int8_t>(timeinfo->tm_sec);
    timeinfo->tm_hour = 22;
    timeinfo->tm_min = 0;
    timeinfo->tm_sec = 0;
    time_t diffTime = difftime(mInstance->mRealtimeSec, mktime(timeinfo));
    ALOGD("SensorEventToOneTrack timeToReport:localtime(%d:%d:%d) diffTime:%ld",
        hour,min,sec,diffTime);
    if (diffTime == 0) {
        mInstance->onetrackApi();
        mInstance->setTimer(24*60*60);
    } else if (diffTime > 0){
            //ALOGI("SensorEventToOneTrack timeToReport:diffTime>0 start");
            timeinfo->tm_hour += 24;
            time_t difftime_nextround = difftime(mktime(timeinfo), mInstance->mRealtimeSec);
            sleep(abs(difftime_nextround));
            mInstance->onetrackApi();
            //ALOGI("SensorEventToOneTrack timeToReport:diffTime>0 onetrackApi end");
            mInstance->setTimer(24*60*60);
    } else {
            //ALOGI("SensorEventToOneTrack timeToReport:diffTime<0 start");
            sleep(abs(diffTime));
            mInstance->onetrackApi();
            //ALOGI("SensorEventToOneTrack timeToReport:diffTime<0 onetrackApi end");
            mInstance->setTimer(24*60*60);
    }
    return arg;
}
void SensorEventToOneTrack::setTimer(time_t timer_count){
    struct sigevent sigev = {};
    struct itimerspec timerspec = {};
    timer_t one_track_timer = nullptr;
    sigev.sigev_notify = SIGEV_THREAD;
    sigev.sigev_notify_attributes = nullptr;
    sigev.sigev_value.sival_ptr = (void *) this;
    sigev.sigev_notify_function = onetrackApiProcess;
    if (timer_create(CLOCK_REALTIME_ALARM, &sigev, &one_track_timer) != 0) {
        ALOGE("one_track_timer creation failed: %s", strerror(errno));
    }

    timerspec.it_value.tv_sec = timer_count;
    timerspec.it_value.tv_nsec = 0;
    timerspec.it_interval.tv_sec = timer_count;
    timerspec.it_interval.tv_nsec = 0;

    if (timer_settime(one_track_timer, 0, &timerspec, nullptr) != 0) {
        ALOGE("%s: failed to start offset update timer: %s", __FUNCTION__, strerror(errno));
    } else {
        ALOGV("%s : offset update timer started", __FUNCTION__);
    }

}

bool SensorEventToOneTrack::enableSensorEventToOnetrack(){
    return onetrackProp;
}

std::string SensorEventToOneTrack::dump() const {
    String8 result;
    result.appendFormat("begin sensor's usage duration:\n");
    for (auto& m : SensorTime) {
        result.appendFormat("sensorname: ");
        result.append(m.first.c_str());
        result.appendFormat(" time:");
        result.append(m.second.c_str());
        result.appendFormat("\n");
    }
    result.appendFormat("begin app using sensor duration:\n");
    for (auto& m : appUseSensorTime) {
        result.appendFormat("\nbegin %s use sensor duration\n", m.first.c_str());
        for (auto& k : m.second) {
            result.appendFormat("sensorname: ");
            result.append(k.first.c_str());
            result.appendFormat(" time:");
            result.append(k.second.c_str());
            result.appendFormat("\n");
        }
    }
    result.appendFormat("begin sensor used by app duration:\n");
    for (auto& m : sensorUsedByAppTime) {
        result.appendFormat("\nbegin %s  used by app duration\n", m.first.c_str());
        for (auto& k : m.second) {
            result.appendFormat("packageName: ");
            result.append(k.first.c_str());
            result.appendFormat(" time:");
            result.append(k.second.c_str());
            result.appendFormat("\n");
        }
    }

    return result.string();
}
}; // namespace android

