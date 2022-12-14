/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "Motor.test"

#include <utils/Log.h>
#include "android/log.h"
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/String16.h>
#include "Motor.h"

#include <hidl/LegacySupport.h>

using vendor::xiaomi::hardware::motor::V1_0::IMotor;
using vendor::xiaomi::hardware::motor::V1_0::IMotorCallback;
using vendor::xiaomi::hardware::motor::V1_0::MotorEvent;


using android::sp;
using android::hardware::Return;
using android::hardware::Void;


class MotorCallback: public IMotorCallback{

public:
    MotorCallback(){

    }
   ~MotorCallback(){

   }

   Return<void> onNotify(const MotorEvent& event){
      ALOGD("onNotify: value = %d",event.value);
      return Void();
   }



};

int main() {
    sp<IMotor> service = IMotor::getService();
    if(service == nullptr){
    ALOGD("main: fail to get motor service");
    return -1;
    }
    sp<MotorCallback> callback = new MotorCallback();
    service->setMotorCallback(callback);
    service->popupMotor(111);
    service->takebackMotor(222);
    service->init();

    ::sleep(10);
    service->release();
    return 0;
}
