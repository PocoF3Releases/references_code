#ifndef VENDOR_XIAOMI_HARDWARE_MISYS_V3_0_MISYS_H
#define VENDOR_XIAOMI_HARDWARE_MISYS_V3_0_MISYS_H

#include <utils/Log.h>
#include "cutils/properties.h"

#include <logwrap/logwrap.h>
#include <include/CheckPid.h>

#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>

#include <vendor/xiaomi/hardware/misys/1.0/IMiSys.h>
#include <vendor/xiaomi/hardware/misys/3.0/IMiSys.h>

namespace vendor {
namespace xiaomi {
namespace hardware {
namespace misys {
namespace V3_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct MiSys : public IMiSys {
    // Methods from ::vendor::xiaomi::hardware::misys::V3_0::IMiSys follow.
    Return<void> AllocateBlock(uint64_t size, AllocateBlock_cb _hidl_cb) override;
    Return<IResultValue> ReleaseBlock(const ::android::hardware::hidl_memory& block) override;
    Return<IResultValue> OnDataReady(const ::android::hardware::hidl_memory& block, uint64_t memorySize, uint32_t moduleID) override;
    Return<IResultValue> SetCallBack(const sp<::vendor::xiaomi::hardware::misys::V3_0::IMiSysCallBack>& callback, uint32_t moduleID, int32_t pid) override;
    Return<IResultValue> ReleaseCallBack(const sp<::vendor::xiaomi::hardware::misys::V3_0::IMiSysCallBack>& callback, uint32_t moduleID, int32_t pid) override;
    Return<void> GetCallBackPid(uint32_t moduleID, GetCallBackPid_cb _hidl_cb) override;
    Return<IResultValue> WriteToFile(const ::android::hardware::hidl_memory& block, const hidl_string& path, const hidl_string& fileName, uint64_t size) override;
    Return<void> ReadFromFile(const ::android::hardware::hidl_memory& block, const hidl_string& path, const hidl_string& fileName, ReadFromFile_cb _hidl_cb) override;
    Return<void> GetFileSize(const hidl_string& path, const hidl_string& fileName, GetFileSize_cb _hidl_cb) override;
    Return<IResultValue> MiSysSetProp(const hidl_string& key, const hidl_string& value) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

private:
    struct CallBackEvent {
      uint32_t moduleID;
      int32_t  pid;
      sp<IMiSysCallBack> callback;
    };
    std::mutex cbLock;
    std::vector<struct CallBackEvent> cbList;
    sp<IMiSysCallBack> cb;
};

    extern "C" IMiSys* HIDL_FETCH_IMiSys(const char* name);

}  // namespace implementation
}  // namespace V3_0
}  // namespace misys
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor

#endif  // VENDOR_XIAOMI_HARDWARE_MISYS_V3_0_MISYS_H
