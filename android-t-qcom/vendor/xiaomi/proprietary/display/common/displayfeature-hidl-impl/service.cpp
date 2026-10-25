/*
 * Copyright (c) 2017 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#define LOG_TAG "vendor.displayfeature@1.0-service"

#include <vendor/xiaomi/hardware/displayfeature/1.0/IDisplayFeature.h>

#include <binder/ProcessState.h>
#include <hidl/LegacySupport.h>
#include "DisplayFeature.h"

using vendor::xiaomi::hardware::displayfeature::V1_0::IDisplayFeature;
using android::hardware::defaultPassthroughServiceImplementation;

int main() {
    // the conventional HAL might start binder services
    android::ProcessState::initWithDriver("/dev/vndbinder");
    android::ProcessState::self()->setThreadPoolMaxThreadCount(4);
    android::ProcessState::self()->startThreadPool();
    return defaultPassthroughServiceImplementation<IDisplayFeature>();
}


