#ifndef VENDOR_XIAOMI_HARDWARE_MIOOB_V1_0_MIOOB_H
#define VENDOR_XIAOMI_HARDWARE_MIOOB_V1_0_MIOOB_H

#include <vendor/xiaomi/hardware/mioob/1.0/IMiOob.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace vendor {
namespace xiaomi {
namespace hardware {
namespace mioob {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct MiOob : public IMiOob {
    // Methods from ::vendor::xiaomi::hardware::mioob::V1_0::IMiOob follow.
    MiOob();

    Return<int32_t> setBtState(::vendor::xiaomi::hardware::mioob::V1_0::Bt_State state) override;
    Return<int32_t> setRxCr(const hidl_string& data) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    private:
       bool mIsADSP = false;
};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IMiOob* HIDL_FETCH_IMiOob(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace mioob
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor

#endif  // VENDOR_XIAOMI_HARDWARE_MIOOB_V1_0_MIOOB_H
