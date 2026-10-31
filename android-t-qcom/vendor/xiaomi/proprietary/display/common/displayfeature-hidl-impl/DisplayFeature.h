/*
 * Copyright (c) 2017 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */
#ifndef VENDOR_XIAOMI_HARDWARE_DISPLAYFEATURE_V1_0_DISPLAYFEATURE_H
#define VENDOR_XIAOMI_HARDWARE_DISPLAYFEATURE_V1_0_DISPLAYFEATURE_H

#include <vendor/xiaomi/hardware/displayfeature/1.0/IDisplayFeature.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include "../hardware/displayfeature_defs.h"

namespace vendor {
namespace xiaomi {
namespace hardware {
namespace displayfeature {
namespace V1_0 {
namespace implementation {

using ::android::hidl::base::V1_0::DebugInfo;
using ::android::hidl::base::V1_0::IBase;
using ::vendor::xiaomi::hardware::displayfeature::V1_0::IDisplayFeature;
using ::vendor::xiaomi::hardware::displayfeature::V1_0::IDisplayFeatureCallback;
using ::vendor::xiaomi::hardware::displayfeature::V1_0::Status;
using ::vendor::xiaomi::hardware::displayfeature::V1_0::FeatureId;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using android::hardware::hidl_death_recipient;
using ::android::sp;
using ::android::wp;

#define MAX_CALLBACK_CNT 5

#define DISPLAYFEATURE_DUMP_ENABLE_PROP "ro.vendor.displayfeature.dump"

class DisplayFeature : public IDisplayFeature, public hidl_death_recipient {
public:
    DisplayFeature(const hw_module_t* module);
    virtual ~DisplayFeature();

    // Methods from ::vendor::xiaomi::hardware::displayfeature::V1_0::IDisplayFeature follow.
    Return<Status> sendPanelCommand(const hidl_string& cmd) override;
    Return<Status> sendRefreshCommand() override;
    Return<void> notifyBrightness(int brightness) override;
    Return<Status> sendPostProcCommand(int cmd, int value) override;
    Return<Status> setFeature(int32_t displayId, int32_t caseId, int32_t modeId, int32_t cookie) override;
    Return<Status> setFunction(int32_t displayId, int32_t caseId, int32_t modeId, int32_t cookie) override;
    Return<void>  sendMessage(int32_t index, int32_t value, const hidl_string& cmd) override;
    Return<Status> registerCallback(int32_t displayId, const sp<IDisplayFeatureCallback>& callback) override;
    Return<void> dumpDebugInfo(dumpDebugInfo_cb _hidl_cb) override;
    void serviceDied(uint64_t cookie, const wp<IBase>& /* who */);
    static void notifyListeners(int32_t displayId, int32_t value, float red, float green, float blue);

    // Methods from ::android::hidl::base::V1_0::IBase follow.
private:
    void initCapabilities();
    template<typename T>
    void initDispatch(df_function_descriptor_t desc, T* outPfn);
    void initDispatch();
    struct {
        DF_PFN_SET_FEATURE setFeature;
        DF_PFN_SET_FUNCTION setFunction;
        DF_PFN_SEND_MESSAGE sendMessage;
        DF_PFN_SET_LISTENER setListener;
        DF_PFN_DUMP dump;
    } mDispatch;

private:
    df_device_t* mDevice;
    static std::mutex callbacks_lock_;
    static sp<IDisplayFeatureCallback> callbacks_[MAX_CALLBACK_CNT];
};

extern "C" IDisplayFeature* HIDL_FETCH_IDisplayFeature(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace displayfeature
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor

#endif  // VENDOR_XIAOMI_HARDWARE_DISPLAYFEATURE_V1_0_DISPLAYFEATURE_H
