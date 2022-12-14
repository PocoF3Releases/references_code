// FIXME: your file license if you have one

#pragma once

#include <mutex>
#include <thread>
#include <vector>
#include <vendor/xiaomi/hardware/cld/1.0/ICld.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace vendor::xiaomi::hardware::cld::implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Cld : public V1_0::ICld {
    // Methods from ::vendor::xiaomi::hardware::cld::V1_0::ICld follow.
    Cld();
    Return<bool> isCldSupported() override;
    Return<::vendor::xiaomi::hardware::cld::V1_0::FragmentLevel> getFragmentLevel() override;
    Return<void> triggerCld(int32_t val) override;
    Return<::vendor::xiaomi::hardware::cld::V1_0::CldOperationStatus> getCldOperationStatus() override;
    Return<void> registerCallback(const sp<::vendor::xiaomi::hardware::cld::V1_0::ICldCallback>& callback) override;
    Return<bool> unregisterCallback(const sp<::vendor::xiaomi::hardware::cld::V1_0::ICldCallback>& callback) override;

    private:
        std::vector<sp<::vendor::xiaomi::hardware::cld::V1_0::ICldCallback>> mCallbacks;
        std::thread th;
        std::recursive_mutex callback_lock;
        void UEventEvent();
        void UEventWorker();
        int uevent_fd;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" ICld* HIDL_FETCH_ICld(const char* name);

}  // namespace vendor::xiaomi::hardware::cld::implementation
