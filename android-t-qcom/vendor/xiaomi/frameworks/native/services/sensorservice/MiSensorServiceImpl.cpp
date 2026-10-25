/*
 *    Copyright 2021, Xiaomi Corporation All rights reserved.
 *
 *    Add sensorlist whitelist feature for idle task control
 *
 */

#include <utils/Log.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cutils/properties.h>
#include "MiSensorServiceImpl.h"
#include "SensorEventToOneTrack.h"
#include "SensorControl.h"

//Xiaomi Add White list for Sensor User Idle State Control
static std::string SensorUserWhiteList[] = {
    "com.android.camera.ProximitySensorLock",
    "com.tencent.mm.plugin.sport.model.c",
    "com.tencent.mm.plugin.sport.model.f",
    "com.tencent.mm.plugin.sport.model.g",
    "android.view.OrientationEventListener",
    "com.miui.aod.doze.MiuiDozeScreenBrightnessController",
    "com.tencent.mobileqq.msf.core.ad",
    "com.alipay.mobile.healthcommon.stepcounter.APStepProcessor",
    "com.mi.health.scenerecognition.collection.SceneCollector",
    "miui.util.BackTapSensorWrapper",
};
static uint32_t SensorUserWhiteListSize = sizeof(SensorUserWhiteList) / sizeof(SensorUserWhiteList[0]);

namespace android {

bool MiSensorServiceImpl::isonwhitelist(String8 PackageName)
{
    bool boRet = false;
    uint32_t i;
    for (i = 0; i < SensorUserWhiteListSize; i++) {
        if (!strcmp(PackageName.c_str(),SensorUserWhiteList[i].c_str())) {
            ALOGI( "user found in check list, getPackageName  %s" , PackageName.c_str());
            boRet = true ;
            break;
        }
    }
    return boRet;
}

bool MiSensorServiceImpl::isSensorDisableApp(const String8& packageName){
    for(String8 name:mSensorDisableNames){
        if (!strcmp(name.c_str(),packageName.c_str())) {
            return true;
        }
    }
    return false;
}

void MiSensorServiceImpl::setSensorDisableApp(const String8& packageName){
    mSensorDisableNames.push_back(packageName);
}

void MiSensorServiceImpl::removeSensorDisableApp(const String8& packageName){
    Vector<String8>::iterator it = std::find(mSensorDisableNames.begin(),
    mSensorDisableNames.end(),packageName.c_str());

    if (it != mSensorDisableNames.end()) {
        mSensorDisableNames.erase(it);
    }
}

String8 MiSensorServiceImpl::getDumpsysInfo() {
    String8 result;
    result.appendFormat("mSensorDisableNames:\n");
         for (size_t i=0 ; i<mSensorDisableNames.size() ; i++) {
             result.appendFormat("   #%zu package name = %s \n", i, mSensorDisableNames[i].c_str());
         }
    return result;
}
void MiSensorServiceImpl::SaveSensorUsageItem(int32_t sensorType, const String8& sensorName,
                                       const String8& packageName, bool activate){
#ifdef SENSOR_EVENT_REPORT
    return defaultSensorEventToOneTrack()->SaveSensorUsageItem(sensorType, sensorName,
                                       packageName, activate);
#else
    (void) (sensorType);
    (void) (sensorName);
    (void) (packageName);
    (void) (activate);
#endif
}

void MiSensorServiceImpl::handleProxUsageTime(const sensors_event_t& event, bool report){
#ifdef SENSOR_EVENT_REPORT
    return defaultSensorEventToOneTrack()->handleProxUsageTime(event,report);
#else
    (void) (event);
    (void) (report);
#endif
}

void MiSensorServiceImpl::handlePocModeUsageTime(const sensors_event_t& event, bool report){
#ifdef SENSOR_EVENT_REPORT
    return defaultSensorEventToOneTrack()->handlePocModeUsageTime(event,report);
#else
    (void) (event);
    (void) (report);
#endif
}

void* MiSensorServiceImpl::timeToReport(void *arg){
#ifdef SENSOR_EVENT_REPORT
    return defaultSensorEventToOneTrack()->timeToReport(arg);
#else
    return arg;
#endif
}

bool MiSensorServiceImpl::enableSensorEventToOnetrack(){
#ifdef SENSOR_EVENT_REPORT
    return defaultSensorEventToOneTrack()->enableSensorEventToOnetrack();
#else
    return false;
#endif
}

void MiSensorServiceImpl::handleSensorEvent(const sensors_event_t& event){
#ifdef SENSOR_EVENT_REPORT
    return defaultSensorEventToOneTrack()->handleSensorEvent(event);
#else
    (void) (event);
#endif
}

void MiSensorServiceImpl::dumpResultLocked(String8& result) {
    defaultSensorControl()->dumpResultLocked(result);
}

bool MiSensorServiceImpl::appIsControlledLocked(uid_t uid, int32_t type) {
    return defaultSensorControl()->appIsControlledLocked(uid, type);
}
status_t MiSensorServiceImpl::executeCommand(const sp<SensorService>& service, const Vector<String16>& args) {
    return defaultSensorControl()->executeCommand(service, args);
}

extern "C" IMiSensorServiceStub* create() {
    return new MiSensorServiceImpl;
}

extern "C" void destroy(IMiSensorServiceStub* impl) {
    delete impl;
}

}