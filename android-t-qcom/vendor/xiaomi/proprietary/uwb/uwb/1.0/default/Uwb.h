// FIXME: your file license if you have one

#pragma once

#ifndef VENDOR_HARDWARE_UWB_V1_0_UWB_H

#define VENDOR_HARDWARE_UWB_V1_0_UWB_H

#include <vendor/xiaomi/uwb/1.0/IUwb.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <hardware/hardware.h>

namespace vendor::xiaomi::uwb::implementation {

using ::vendor::xiaomi::uwb::V1_0::IUwb;
using ::vendor::xiaomi::uwb::V1_0::IUwbClientCallback;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Uwb : public V1_0::IUwb, public android::hardware::hidl_death_recipient {
    Uwb();
    // Methods from ::vendor::xiaomi::uwb::V1_0::IUwb follow.
    Return<::vendor::xiaomi::uwb::V1_0::UwbStatus> open(const sp<::vendor::xiaomi::uwb::V1_0::IUwbClientCallback>& clientCallback) override;
    Return<uint32_t> write(const hidl_vec<uint8_t>& data) override;
    Return<::vendor::xiaomi::uwb::V1_0::UwbStatus> close() override;
    Return<void> ioctl(uint64_t ioctlType, const hidl_vec<uint8_t>& inputData, ioctl_cb _hidl_cb) override;
    Return<void> getConfig(getConfig_cb _hidl_cb) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    static void eventCallback(uint8_t event, uint8_t status) {
        if (mCallback != nullptr) {
            auto ret = mCallback->sendEvent((::vendor::xiaomi::uwb::V1_0::UwbEvent)event,
                                            (::vendor::xiaomi::uwb::V1_0::UwbStatus)status);
            if (!ret.isOk()) {
                ALOGW("Failed to call back into uwb process.");
            }
        }
    }

    static void dataCallback(uint16_t data_len, uint8_t* p_data) {
        hidl_vec<uint8_t> data;
        data.setToExternal(p_data, data_len);
        if (mCallback != nullptr) {
            auto ret = mCallback->sendData(data);
            if (!ret.isOk()) {
                ALOGW("Failed to call back into uwb process.");
            }
        }
    }

    virtual void serviceDied(uint64_t /*cookie*/,
                             const android::wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
        close();
    }

    private:
    static sp<IUwbClientCallback> mCallback;

    //const uwb_uci_device_t*       mDevice;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IUwb* HIDL_FETCH_IUwb(const char* name);

}  // namespace vendor::xiaomi::hardware::uwb::implementation

#endif
