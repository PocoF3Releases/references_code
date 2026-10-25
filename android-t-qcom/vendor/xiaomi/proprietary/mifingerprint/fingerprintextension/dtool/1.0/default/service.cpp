# define LOG_TAG "vendor.xiaomi.hardware.dtool@1.0-service"

#include <android/log.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <vendor/xiaomi/hardware/dtool/1.0/IDtool.h>
//#include <android/hardware/dtool/1.0/types.h>
#include "Dtool.h"

using vendor::xiaomi::hardware::dtool::V1_0::IDtool;
using vendor::xiaomi::hardware::dtool::implementation::Dtool;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;

int main() {
    android::sp<IDtool> gdp = new Dtool();

    configureRpcThreadpool(1, true /*callerWillJoin*/);

    if (gdp != nullptr) {
        if (::android::OK != gdp->registerAsService()) {
            return 1;
        }
    } else {
        ALOGE("Can't create instance of dtool, nullptr");
    }

    joinRpcThreadpool();

    return 0; // should never get here
}
