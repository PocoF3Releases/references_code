#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "BatteryLife"

#include <log/log.h>
#include <stdio.h>
#include <unistd.h>
#include "BatteryLife.h"

#define BATTERY_LIFE_SYSFS_NODE "/sys/class/power_supply/bms/soh"
#define BATTERY_CHARGE_FULL_SYSFS_NODE "/sys/class/power_supply/bms/charge_full"


namespace vendor {
namespace xiaomi {
namespace hardware {
namespace batterylife {
namespace V1_0 {
namespace implementation {

// Methods from ::vendor::xiaomi::hardware::batterylife::V1_0::IBatteryLife follow.
Return<bool> BatteryLife::isBatteryLifeFunctionSupported() {
    if (access(BATTERY_LIFE_SYSFS_NODE, F_OK) != 0) {
        ALOGE("Cannot access sysfs node for batterylife.");
        return false;
    }

    return true;
}

Return<int32_t> BatteryLife::getBatteryLifeValue() {
    int ret, fd;
    int value = 0;
    char result[4] = {0};

    fd = open(BATTERY_LIFE_SYSFS_NODE, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        ALOGE("Failed to open batterylife node.");
        return -1;
    }

    ret = read(fd, result, sizeof(result));
    if (ret < 0) {
        ALOGE("read batterylife sysfs node failed.");
        close(fd);
        return -1;
    }

    value = atoi(result);
    close(fd);

    return value;
}

Return<int32_t> BatteryLife::getBatteryChargeFullValue() {
    int ret, fd;
    int value = 0;
    char result[10] = {0};

    fd = open(BATTERY_CHARGE_FULL_SYSFS_NODE, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        ALOGE("Failed to open chargefull node.");
        return -1;
    }

    ret = read(fd, result, sizeof(result));
    if (ret < 0) {
        ALOGE("read chargefull sysfs node failed.");
        close(fd);
        return -1;
    }

    value = atoi(result);
    close(fd);

    return value;
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

//IBatteryLife* HIDL_FETCH_IBatteryLife(const char* /* name */) {
    //return new BatteryLife();
//}
//
}  // namespace implementation
}  // namespace V1_0
}  // namespace batterylife
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor
