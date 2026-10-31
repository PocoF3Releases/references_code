#ifndef VENDOR_XIAOMI_HARDWARE_BATTERYLIFE_V1_0_BATTERYLIFE_H
#define VENDOR_XIAOMI_HARDWARE_BATTERYLIFE_V1_0_BATTERYLIFE_H

#include <vendor/xiaomi/hardware/batterylife/1.0/IBatteryLife.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <utils/Log.h>

namespace vendor {
namespace xiaomi {
namespace hardware {
namespace batterylife {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct BatteryLife : public IBatteryLife {
    // Methods from ::vendor::xiaomi::hardware::batterylife::V1_0::IBatteryLife follow.
    Return<bool> isBatteryLifeFunctionSupported() override;
    Return<int32_t> getBatteryLifeValue() override;
    Return<int32_t> getBatteryChargeFullValue() override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IBatteryLife* HIDL_FETCH_IBatteryLife(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace batterylife
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor

#endif  // VENDOR_XIAOMI_HARDWARE_BATTERYLIFE_V1_0_BATTERYLIFE_H
