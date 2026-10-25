#ifndef VENDOR_XIAOMI_HARDWARE_MISYS_V1_0_MISYS_H
#define VENDOR_XIAOMI_HARDWARE_MISYS_V1_0_MISYS_H

#include <vendor/xiaomi/hardware/misys/1.0/IMiSys.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <string>
#include <iostream>
#include <sys/stat.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <stdio.h>

namespace vendor {
namespace xiaomi {
namespace hardware {
namespace misys {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::vendor::xiaomi::hardware::misys::V1_0::IResultValue;
using ::vendor::xiaomi::hardware::misys::V1_0::IFileListResult;
using ::vendor::xiaomi::hardware::misys::V1_0::IReadResult;
using ::android::sp;

struct MiSys : public IMiSys {
    // Methods from ::vendor::xiaomi::hardware::misys::V1_0::IMiSys follow.
    Return<void> DirListFiles(const hidl_string& path, DirListFiles_cb _hidl_cb) override;
    Return<::vendor::xiaomi::hardware::misys::V1_0::IResultValue> MiSysWriteFile(const hidl_string& path, const hidl_string& file_name, const hidl_string& writebuf, uint32_t sbuf_len, uint8_t append_data) override;
    Return<void> MiSysReadFile(const hidl_string& path, const hidl_string& file_name, MiSysReadFile_cb _hidl_cb) override;
    Return<::vendor::xiaomi::hardware::misys::V1_0::IResultValue> EraseFileOrDirectory(const hidl_string& path, const hidl_string& file_name) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

// FIXME: most likely delete, this is only for passthrough implementations
   extern "C" IMiSys* HIDL_FETCH_IMiSys(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace misys
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor

#endif  // VENDOR_XIAOMI_HARDWARE_MISYS_V1_0_MISYS_H
