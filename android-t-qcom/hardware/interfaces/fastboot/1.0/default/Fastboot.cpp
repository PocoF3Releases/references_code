/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Fastboot.h"
// MIUI ADD: START
#include <fs_mgr.h>

using android::fs_mgr::Fstab;
using android::fs_mgr::FstabEntry;
using android::fs_mgr::ReadDefaultFstab;
using Volume = android::fs_mgr::FstabEntry;
// END

namespace android {
namespace hardware {
namespace fastboot {
namespace V1_0 {
namespace implementation {

// MIUI ADD: START
static Fstab fstab;

Volume* volume_for_mount_point(const std::string& mount_point) {
  return android::fs_mgr::GetEntryForMountPoint(&fstab, mount_point);
}
// END

// Methods from ::android::hardware::fastboot::V1_0::IFastboot follow.
// MIUI MOD
// 
// Return<void> Fastboot::getPartitionType(const hidl_string& /* partitionName */,
Return<void> Fastboot::getPartitionType(const hidl_string& partitionName,
                                        getPartitionType_cb _hidl_cb) {
    // MIUI MOD: START
    // 
    // _hidl_cb(FileSystemType::RAW, {Status::SUCCESS, ""});
    std::string mount_point = "/" + std::string(partitionName);
    if(partitionName == "userdata")
        mount_point = "/data";
    Volume* v = volume_for_mount_point(mount_point);
    if (v == nullptr) {
        printf("unknown volume for path [ %s ]\n", partitionName.c_str());
        _hidl_cb(FileSystemType::RAW, {Status::SUCCESS, ""});
        return Void();
    }
    FileSystemType type = FileSystemType::RAW;
    if(v->fs_type == "ext4")
        type = FileSystemType::EXT4;
    else if (v->fs_type == "f2fs")
        type = FileSystemType::F2FS;
    _hidl_cb(type, {Status::SUCCESS, ""});
    // END
    return Void();
}

Return<void> Fastboot::doOemCommand(const hidl_string& /* oemCmd */, doOemCommand_cb _hidl_cb) {
    _hidl_cb({Status::FAILURE_UNKNOWN, "Command not supported in default implementation"});
    return Void();
}

Return<void> Fastboot::getVariant(getVariant_cb _hidl_cb) {
    _hidl_cb("NA", {Status::SUCCESS, ""});
    return Void();
}

Return<void> Fastboot::getOffModeChargeState(getOffModeChargeState_cb _hidl_cb) {
    _hidl_cb(false, {Status::SUCCESS, ""});
    return Void();
}

Return<void> Fastboot::getBatteryVoltageFlashingThreshold(
        getBatteryVoltageFlashingThreshold_cb _hidl_cb) {
    _hidl_cb(0, {Status::SUCCESS, ""});
    return Void();
}

extern "C" IFastboot* HIDL_FETCH_IFastboot(const char* /* name */) {
    // MIUI ADD: START
    if (!ReadDefaultFstab(&fstab)) {
        printf("Failed to read default fstab\n");
    }
    // END
    return new Fastboot();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace fastboot
}  // namespace hardware
}  // namespace android
