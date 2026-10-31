#define LOG_TAG "vendor.xiaomi.hardware.mioob@1.0-service"

#include <utils/Log.h>
#include "android/log.h"
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/String16.h>
#include "MiOob.h"

#include <hidl/LegacySupport.h>

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using vendor::xiaomi::hardware::mioob::V1_0::IMiOob;
using vendor::xiaomi::hardware::mioob::V1_0::implementation::MiOob;
using android::hardware::defaultPassthroughServiceImplementation;
using android::sp;
using android::status_t;
using android::OK;

int main() {
    configureRpcThreadpool(10, true);

    sp<MiOob> mioobService = new MiOob();
    status_t status = mioobService->registerAsService();
    LOG_ALWAYS_FATAL_IF(status != OK, "Could not register mioob Service");

    // other interface registration comes here
	joinRpcThreadpool();
	return 1;
}
