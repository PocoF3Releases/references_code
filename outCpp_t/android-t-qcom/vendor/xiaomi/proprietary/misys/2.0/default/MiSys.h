#ifndef VENDOR_XIAOMI_HARDWARE_MISYS_V2_0_MISYS_H
#define VENDOR_XIAOMI_HARDWARE_MISYS_V2_0_MISYS_H

#include <vendor/xiaomi/hardware/misys/2.0/IMiSys.h>
#include <vendor/xiaomi/hardware/misys/1.0/IMiSys.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

using ::vendor::xiaomi::hardware::misys::V1_0::IResultValue;
using ::vendor::xiaomi::hardware::misys::V1_0::IFileListResult;
using ::vendor::xiaomi::hardware::misys::V1_0::IReadResult;


namespace vendor {
namespace xiaomi {
namespace hardware {
namespace misys {
namespace V2_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct MiSys : public IMiSys {
    // Methods from ::vendor::xiaomi::hardware::misys::V2_0::IMiSys follow.
    Return<::vendor::xiaomi::hardware::misys::V1_0::IResultValue> MiSysCreateFolder(const hidl_string& path, const hidl_string& folder_name) override;
    Return<::vendor::xiaomi::hardware::misys::V1_0::IResultValue> MiSysWriteBuffer(const hidl_string& path, const hidl_string& file_name, const hidl_vec<uint8_t>& writebuf, uint64_t buf_len) override;
    Return<void> MiSysReadBuffer(const hidl_string& path, const hidl_string& file_name, MiSysReadBuffer_cb _hidl_cb) override;
    Return<void> GetPartitionSize(const hidl_string& path,  const hidl_string& partitionName,  GetPartitionSize_cb _hidl_cb) override;
    Return<bool> IsExists(const hidl_string& path, const hidl_string& file_name) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

extern "C" IMiSys* HIDL_FETCH_IMiSys(const char* name);

}  // namespace implementation
}  // namespace V2_0
}  // namespace misys
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor

#endif  // VENDOR_XIAOMI_HARDWARE_MISYS_V2_0_MISYS_H
