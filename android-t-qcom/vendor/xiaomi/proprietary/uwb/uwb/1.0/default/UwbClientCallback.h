// FIXME: your file license if you have one

#pragma once

#ifndef VENDOR_HARDWARE_UWB_V1_0_UWBCLIENTCALLBACK_H

#define VENDOR_HARDWARE_UWB_V1_0_UWBCLIENTCALLBACK_H

#include <vendor/xiaomi/uwb/1.0/IUwbClientCallback.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace vendor::xiaomi::uwb::implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct UwbClientCallback : public V1_0::IUwbClientCallback {
    // Methods from ::vendor::xiaomi::uwb::V1_0::IUwbClientCallback follow.
    Return<void> sendEvent(::vendor::xiaomi::uwb::V1_0::UwbEvent event, ::vendor::xiaomi::uwb::V1_0::UwbStatus status) override;
    Return<void> sendData(const hidl_vec<uint8_t>& data) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IUwbClientCallback* HIDL_FETCH_IUwbClientCallback(const char* name);

}  // namespace vendor::xiaomi::hardware::uwb::implementation

#endif
