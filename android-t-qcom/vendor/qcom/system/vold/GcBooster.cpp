/*
 * Copyright (C) Xiaomi Inc.
 *
 */
#include "VoldImpl.h"

#include <list>
#include <string>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <fs_mgr.h>
#include <FileDeviceUtils.h>
#include <model/PrivateVolume.h>
#include <Utils.h>
#include <VolumeManager.h>

#include <sys/mount.h>

using android::base::Basename;
using android::base::Realpath;
using android::base::WriteStringToFile;
using android::fs_mgr::Fstab;
using android::fs_mgr::ReadDefaultFstab;

namespace android {
namespace vold {

static void addBlkDeviceFromVolumeManager(std::list<std::string>* paths) {
    VolumeManager* vm = VolumeManager::Instance();
    std::list<std::string> privateIds;
    vm->listVolumes(VolumeBase::Type::kPrivate, privateIds);
    for (const auto& id : privateIds) {
        PrivateVolume* vol = static_cast<PrivateVolume*>(vm->findVolume(id).get());
        if (vol != nullptr && vol->getState() == VolumeBase::State::kMounted) {
            std::string gc_path;
            const std::string& fs_type = vol->getFsType();
            if (fs_type == "f2fs" && (Realpath(vol->getRawDmDevPath(), &gc_path) ||
                                      Realpath(vol->getRawDevPath(), &gc_path))) {
                paths->push_back(std::string("/sys/fs/") + fs_type + "/" + Basename(gc_path));
            }
        }
    }
}

static void addBlkDeviceFromFstab(std::list<std::string>* paths) {
    Fstab fstab;
    ReadDefaultFstab(&fstab);

    std::string previous_mount_point;
    for (const auto& entry : fstab) {
        // Skip raw partitions.
        if (entry.fs_type == "emmc" || entry.fs_type == "mtd") {
            continue;
        }
        // Skip read-only filesystems
        if (entry.flags & MS_RDONLY) {
            continue;
        }
        // Skip the multi-type partitions, which are required to be following each other.
        // See fs_mgr.c's mount_with_alternatives().
        if (entry.mount_point == previous_mount_point) {
            continue;
        }

        std::string gc_path;
        if (entry.fs_type == "f2fs" &&
            Realpath(android::vold::BlockDeviceForPath(entry.mount_point + "/"), &gc_path)) {
            paths->push_back("/sys/fs/" + entry.fs_type + "/" + Basename(gc_path));
        }

        previous_mount_point = entry.mount_point;
    }
}

void VoldImpl::gcBoosterControl(const std::string& enable) {
    if (enable != "0" && enable != "1") return;

    std::list<std::string> paths;
    addBlkDeviceFromFstab(&paths);
    addBlkDeviceFromVolumeManager(&paths);

    for (const auto& path : paths) {
        LOG(DEBUG) << "Write GcBooster on " << path;
        if (!WriteStringToFile(enable, path + "/gc_booster")) {
            PLOG(WARNING) << "Write GcBooster fail on " << path;
        }
    }
}

}  // namespace vold
}  // namespace android