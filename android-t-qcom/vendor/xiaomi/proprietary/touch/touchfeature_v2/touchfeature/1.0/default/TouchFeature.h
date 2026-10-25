/*
 * Copyright (c) 2017 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */
#ifndef VENDOR_XIAOMI_HW_TOUCHFEATURE_V1_0_TOUCHFEATURE_H
#define VENDOR_XIAOMI_HW_TOUCHFEATURE_V1_0_TOUCHFEATURE_H

#include <vendor/xiaomi/hw/touchfeature/1.0/ITouchFeature.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <stdio.h>
#include <unistd.h>

namespace vendor {
namespace xiaomi {
namespace hw {
namespace touchfeature {
namespace V1_0 {
namespace implementation {

using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

class TouchFeature : public ITouchFeature {
public:
    TouchFeature();
    ~TouchFeature();

    // Methods from ::vendor::xiaomi::touchfeature::V1_0::ITouchFeature follow.
    Return<int32_t> setModeValue(int32_t touchId, int32_t ControlMode, int32_t ModeValue) override;
    Return<int32_t> getModeCurValue(int32_t touchId, int32_t ControlMode) override;
    Return<int32_t> getModeMaxValue(int32_t touchId, int32_t ControlMode) override;
    Return<int32_t> getModeMinValue(int32_t touchId, int32_t ControlMode) override;
    Return<int32_t> getModeDefaultValue(int32_t touchId, int32_t ControlMode) override;
    Return<int32_t> modeReset(int32_t touchId, int32_t ControlMode) override;
    Return<void> getModeValue(int32_t touchId, int32_t mode, getModeValue_cb _hidl_cb) override;
    Return<int32_t> setModeLongValue(int32_t touchId, int32_t ControlMode, int32_t ValueLen, const hidl_vec<int32_t>& ValueBuf) override;
    Return<void> getTouchEvent(getTouchEvent_cb _hidl_cb) override;
    // Methods from ::android::hidl::base::V1_0::IBase follow.
};

extern "C" ITouchFeature* HIDL_FETCH_ITouchFeature(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace touchfeature
}  // namespace hw
}  // namespace xiaomi
}  // namespace vendor

#endif  // VENDOR_XIAOMI_HW_TOUCHFEATURE_V1_0_TOUCHFEATURE_H
