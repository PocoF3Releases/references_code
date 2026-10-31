// FIXME: your file license if you have one

#pragma once

#include <vendor/xiaomi/hardware/dtool/1.0/IDtool.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#define CUSTOMIZED_PROPERTY_DUMP_DATA "persist.vendor.sys.fp.dump_data"

namespace vendor::xiaomi::hardware::dtool::implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Dtool : public V1_0::IDtool {
    // Methods from ::vendor::xiaomi::hardware::dtool::V1_0::IDtool follow.
    Return<int32_t> getNum() override;
    Return<void> getName(int32_t num, getName_cb _hidl_cb) override;
    Return<void> getFile(const hidl_string& name, int32_t num, getFile_cb _hidl_cb) override;
    Return<int32_t> getLength(const hidl_string& name) override;
    Return<int32_t> setdumpprop(int32_t num) override;
    Return<int32_t> getdumpprop() override;
    Return<int32_t> deleteFile(const hidl_string& name) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IDtool* HIDL_FETCH_IDtool(const char* name);

}  // namespace vendor::xiaomi::hardware::dtool::implementation
