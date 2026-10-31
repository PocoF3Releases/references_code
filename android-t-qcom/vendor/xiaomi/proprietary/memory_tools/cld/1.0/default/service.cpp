#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/Log.h>
#include <utils/String16.h>
#include "Cld.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "mem_cld"

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using vendor::xiaomi::hardware::cld::V1_0::ICld;
using vendor::xiaomi::hardware::cld::implementation::Cld;
using android::sp;
using android::status_t;
using android::OK;

int main() {
    configureRpcThreadpool(10, true);
    ALOGD("CLD is running...");

    sp<Cld> cldService = new Cld;
    status_t status = cldService->registerAsService();
    LOG_ALWAYS_FATAL_IF(status != OK, "Could not register Cld service");

    // other interface registration comes here
    joinRpcThreadpool();
    return 1;
}

