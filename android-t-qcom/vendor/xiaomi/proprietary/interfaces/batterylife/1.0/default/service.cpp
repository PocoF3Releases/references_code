#define LOG_TAG "vendor.xiaomi.hardware.batterylife@1.0-service"

#include <utils/Log.h>
#include "android/log.h"
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/String16.h>
#include "BatteryLife.h"

#include <hidl/LegacySupport.h>

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using vendor::xiaomi::hardware::batterylife::V1_0::IBatteryLife;
using vendor::xiaomi::hardware::batterylife::V1_0::implementation::BatteryLife;
using android::hardware::defaultPassthroughServiceImplementation;
using android::sp;
using android::status_t;
using android::OK;

int main() {
    configureRpcThreadpool(10, true);

    sp<IBatteryLife> batterylifeService = new BatteryLife;
    status_t status = batterylifeService->registerAsService();
    LOG_ALWAYS_FATAL_IF(status != OK, "Could not register batterylifeService");

    // other interface registration comes here
    joinRpcThreadpool();
    return 1;
}
