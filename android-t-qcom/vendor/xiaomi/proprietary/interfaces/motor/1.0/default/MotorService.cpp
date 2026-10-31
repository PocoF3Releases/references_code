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

#define LOG_TAG "vendor.xiaomi.hardware.motor@1.0-service"

#include <utils/Log.h>
#include "android/log.h"
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/String16.h>
#include "Motor.h"

#include <hidl/LegacySupport.h>

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using vendor::xiaomi::hardware::motor::V1_0::IMotor;
using vendor::xiaomi::hardware::motor::V1_0::implementation::Motor;
using android::hardware::defaultPassthroughServiceImplementation;
using android::sp;
using android::status_t;
using android::OK;

int main() {
    configureRpcThreadpool(5, true);

    sp<IMotor> motorService = new Motor;
    status_t status = motorService->registerAsService();
    LOG_ALWAYS_FATAL_IF(status != OK, "Could not register MotorService");
    // other interface registration comes here
    joinRpcThreadpool();
    return 0;
}
