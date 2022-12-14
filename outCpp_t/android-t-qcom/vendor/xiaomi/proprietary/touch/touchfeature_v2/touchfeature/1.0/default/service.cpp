/*
 * Copyright (c) 2017 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#define LOG_TAG "vendor.touchfeature@1.0-service"

#include <vendor/xiaomi/hw/touchfeature/1.0/ITouchFeature.h>

#include <binder/ProcessState.h>
#include <hidl/LegacySupport.h>
#include <utils/Log.h>
#include "android/log.h"
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/String16.h>
#include "TouchFeature.h"

using vendor::xiaomi::hw::touchfeature::V1_0::ITouchFeature;
using vendor::xiaomi::hw::touchfeature::V1_0::implementation::TouchFeature;
using android::hardware::defaultPassthroughServiceImplementation;
using android::hardware::configureRpcThreadpool;
using android::hardware::ProcessState;
using android::hardware::joinRpcThreadpool;
using android::sp;
using android::status_t;
using android::OK;

int main() {
    configureRpcThreadpool(10, true);

    sp<ITouchFeature> TouchFeatureService = new TouchFeature();
    status_t status = TouchFeatureService->registerAsService();
    LOG_ALWAYS_FATAL_IF(status != OK, "Could not register TouchFeatureService");
    // other interface registration comes here
    joinRpcThreadpool();

    return 0;
}

