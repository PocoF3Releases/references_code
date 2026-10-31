/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include "recovery_utils/roots.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <android-base/chrono_utils.h>
#include <thread>
#include <ext4_utils/ext4_utils.h>
#include <fs_mgr.h>
#include <fs_mgr/roots.h>

#include "otautil/sysutil.h"
#include <linux/loop.h>
#include <ziparchive/zip_archive.h>
#include <android-base/scopeguard.h>
#include <sys/mount.h>

#ifdef MTK_VENDOR
#include "mt_roots.h"
#endif

using android::fs_mgr::Fstab;
using android::fs_mgr::FstabEntry;
using android::fs_mgr::ReadDefaultFstab;

static Fstab fstab;
// MIUI ADD: START
static constexpr std::chrono::milliseconds kMapTimeout{ 1000 };
static const char* kApexRuntimeMountPoint     = "/apex";
static const std::string kSystemMountPoint    = "/system_root";
static const std::string kApexArtFile         = "com.android.art.apex";
static const std::string kApexArtDebugFile    = "com.android.art.debug.apex";
static const std::string kApexArtReleaseFile  = "com.android.art.release.apex";
static const std::string kCapexArtFile        = "com.android.art.capex";
static const std::string kCapexGooleArtFile   = "com.google.android.art_compressed.apex";
static const std::string kApexi18nFile        = "com.android.i18n.apex";
static const std::string kApexStatsdFile      = "com.android.os.statsd.apex";
static const std::string kApexDir = kSystemMountPoint + "/system/apex/";
// END

constexpr const char* CACHE_ROOT = "/cache";

void load_volume_table() {
  if (!ReadDefaultFstab(&fstab)) {
    LOG(ERROR) << "Failed to read default fstab";
    return;
  }

  fstab.emplace_back(FstabEntry{
      .blk_device = "ramdisk",
      .mount_point = "/tmp",
      .fs_type = "ramdisk",
      .length = 0,
  });

  // MIUI ADD: START for system_root copy from fstab entry "system or /"
  Volume* v = nullptr;
  if ((v = volume_for_mount_point("/system")) ||
      (v = volume_for_mount_point("/")) ) {
    Volume system_root_entry{*v};
    system_root_entry.mount_point = kSystemMountPoint.c_str();
    fstab.emplace_back(system_root_entry);
  } else LOG(ERROR) << "Failed to find system entry";
  // END

  // MIUI ADD: START
#ifdef MTK_VENDOR
  mt_load_volume_table(&fstab);
#endif //MTK_VENDOR
  // END

  std::cout << "recovery filesystem table" << std::endl << "=========================" << std::endl;
  for (size_t i = 0; i < fstab.size(); ++i) {
    const auto& entry = fstab[i];
    std::cout << "  " << i << " " << entry.mount_point << " "
              << " " << entry.fs_type << " " << entry.blk_device << " " << entry.length
              << std::endl;
  }
  std::cout << std::endl;
}

// MIUI ADD START: for lazy mount_point mount
static int wait_for_file(const char* filename, std::chrono::nanoseconds timeout) {
    android::base::Timer t;
    while (t.duration() < timeout) {
        struct stat sb;
        if (stat(filename, &sb) != -1) {
            LOG(INFO) << "wait for '" << filename << "' took " << t;
            return 0;
        }
        std::this_thread::sleep_for(10ms);
    }
    LOG(WARNING) << "wait for '" << filename << "' timed out and took " << t;
    return -1;
}

void ensure_dev_ready(const std::string& mount_point) {
  LOG(INFO) << "ensure_dev_ready " << mount_point;
  if (mount_point.empty()) {
    LOG(ERROR) << "invalid mount point " << mount_point;
    return;
  }

  if (fstab.empty()) {
    load_volume_table();
  }

  Volume* v = GetEntryForMountPoint(&fstab, mount_point);
  if (wait_for_file(v->blk_device.c_str(), std::chrono::seconds(5)) == -1) {
    LOG(ERROR) << "Failed waiting for device path: " << v->blk_device;
  }
}
// MIUI ADD END

Volume* volume_for_mount_point(const std::string& mount_point) {
  return android::fs_mgr::GetEntryForMountPoint(&fstab, mount_point);
}

// Mount the volume specified by path at the given mount_point.
int ensure_path_mounted_at(const std::string& path, const std::string& mount_point) {
  return android::fs_mgr::EnsurePathMounted(&fstab, path, mount_point) ? 0 : -1;
}

int ensure_path_mounted(const std::string& path) {
  // Mount at the default mount point.
  return android::fs_mgr::EnsurePathMounted(&fstab, path) ? 0 : -1;
}

int ensure_path_unmounted(const std::string& path) {
  return android::fs_mgr::EnsurePathUnmounted(&fstab, path) ? 0 : -1;
}

// MIUI MOD
int exec_cmd(const std::vector<std::string>& args) {
  CHECK(!args.empty());
  auto argv = StringVectorToNullTerminatedArray(args);

  pid_t child;
  if ((child = fork()) == 0) {
    execv(argv[0], argv.data());
    _exit(EXIT_FAILURE);
  }

  int status;
  waitpid(child, &status, 0);
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    LOG(ERROR) << args[0] << " failed with status " << WEXITSTATUS(status);
  }
  return WEXITSTATUS(status);
}

static int64_t get_file_size(int fd, uint64_t reserve_len) {
  struct stat buf;
  int ret = fstat(fd, &buf);
  if (ret) return 0;

  int64_t computed_size;
  if (S_ISREG(buf.st_mode)) {
    computed_size = buf.st_size - reserve_len;
  } else if (S_ISBLK(buf.st_mode)) {
    uint64_t block_device_size = get_block_device_size(fd);
    if (block_device_size < reserve_len ||
        block_device_size > std::numeric_limits<int64_t>::max()) {
      computed_size = 0;
    } else {
      computed_size = block_device_size - reserve_len;
    }
  } else {
    computed_size = 0;
  }

  return computed_size;
}

// Copied from system/core/fs_mgr/fs_mgr_format.cpp
static int get_dev_sz(const std::string& fs_blkdev, uint64_t* dev_sz) {
      android::base::unique_fd fd(
        TEMP_FAILURE_RETRY(open(fs_blkdev.c_str(), O_RDONLY | O_CLOEXEC)));

    if (fd < 0) {
        LOG(ERROR) << "Cannot open block device";
        return -1;
    }

    if ((ioctl(fd, BLKGETSIZE64, dev_sz)) == -1) {
        LOG(ERROR) << "Cannot get block device size";
        return -1;
    }

    return 0;
}

// Copied from system/core/fs_mgr/fs_mgr.cpp
static std::string get_resize_ovp_option(const std::string& blk_device) {
    uint64_t dev_sz = 0;
    std::string ovp_option;
    std::map<int, std::string> flash_ovp_map {
        {32, "4.0"},
        {64, "3.0"},
        {128, "2.0"},
        {256, "1.4"},
        {512, "1.0"},
    };

    if (get_dev_sz(blk_device, &dev_sz) == 0) {
        // flash 向上取整到 2 的幂次，忽略 flash 大小等于 2 的幂的情况
        int flash = (int) pow(2, (int) log2(dev_sz >> 30) + 1);

        // 若没有则取默认值，不使用 -o 参数
        if (flash_ovp_map.find(flash) != flash_ovp_map.end())
            ovp_option = flash_ovp_map[flash];

        LOG(INFO) << "Setting resize overprovision ratio to " <<
            ovp_option << " for " << blk_device << "(" << flash << "G)";
    }

    return ovp_option;
}

int format_volume(const std::string& volume, const std::string& directory) {
  const FstabEntry* v = android::fs_mgr::GetEntryForPath(&fstab, volume);
  if (v == nullptr) {
    LOG(ERROR) << "unknown volume \"" << volume << "\"";
    return -1;
  }
  if (v->fs_type == "ramdisk") {
    LOG(ERROR) << "can't format_volume \"" << volume << "\"";
    return -1;
  }
  if (v->mount_point != volume) {
    LOG(ERROR) << "can't give path \"" << volume << "\" to format_volume";
    return -1;
  }
  if (ensure_path_unmounted(volume) != 0) {
    LOG(ERROR) << "format_volume: Failed to unmount \"" << v->mount_point << "\"";
    return -1;
  }
  if (v->fs_type != "ext4" && v->fs_type != "f2fs") {
    LOG(ERROR) << "format_volume: fs_type \"" << v->fs_type << "\" unsupported";
    return -1;
  }

  bool needs_casefold = false;
  bool needs_projid = false;
  if (volume == "/data") {
    needs_casefold = android::base::GetBoolProperty("external_storage.casefold.enabled", false);
    // MIUI MOD
    // needs_projid = android::base::GetBoolProperty("external_storage.projid.enabled", false);
    needs_projid = android::base::GetBoolProperty("external_storage.casefold.enabled", false);
  }

  int64_t length = 0;
  if (v->length > 0) {
    length = v->length;
  } else if (v->length < 0) {
    android::base::unique_fd fd(open(v->blk_device.c_str(), O_RDONLY));
    if (fd == -1) {
      PLOG(ERROR) << "format_volume: failed to open " << v->blk_device;
      return -1;
    }
    length = get_file_size(fd.get(), -v->length);
    if (length <= 0) {
      LOG(ERROR) << "get_file_size: invalid size " << length << " for " << v->blk_device;
      return -1;
    }
  }

  if (v->fs_type == "ext4") {
    static constexpr int kBlockSize = 4096;
    std::vector<std::string> mke2fs_args = {
      "/system/bin/mke2fs", "-F", "-t", "ext4", "-b", std::to_string(kBlockSize),
    };

    // Following is added for Project ID's quota as they require wider inodes.
    // The Quotas themselves are enabled by tune2fs on boot.
    mke2fs_args.push_back("-I");
    mke2fs_args.push_back("512");

    if (v->fs_mgr_flags.ext_meta_csum) {
      mke2fs_args.push_back("-O");
      mke2fs_args.push_back("metadata_csum");
      mke2fs_args.push_back("-O");
      mke2fs_args.push_back("64bit");
      mke2fs_args.push_back("-O");
      mke2fs_args.push_back("extent");
    }

    int raid_stride = v->logical_blk_size / kBlockSize;
    int raid_stripe_width = v->erase_blk_size / kBlockSize;
    // stride should be the max of 8KB and logical block size
    if (v->logical_blk_size != 0 && v->logical_blk_size < 8192) {
      raid_stride = 8192 / kBlockSize;
    }
    if (v->erase_blk_size != 0 && v->logical_blk_size != 0) {
      mke2fs_args.push_back("-E");
      mke2fs_args.push_back(
          android::base::StringPrintf("stride=%d,stripe-width=%d", raid_stride, raid_stripe_width));
    }
    mke2fs_args.push_back(v->blk_device);
    if (length != 0) {
      mke2fs_args.push_back(std::to_string(length / kBlockSize));
    }

    int result = exec_cmd(mke2fs_args);
    if (result == 0 && !directory.empty()) {
      std::vector<std::string> e2fsdroid_args = {
        "/system/bin/e2fsdroid", "-e", "-f", directory, "-a", volume, v->blk_device,
      };
      result = exec_cmd(e2fsdroid_args);
    }

    if (result != 0) {
      PLOG(ERROR) << "format_volume: Failed to make ext4 on " << v->blk_device;
      return -1;
    }
    return 0;
  }

  // Has to be f2fs because we checked earlier.
  static constexpr int kSectorSize = 4096;
  std::string ovp_option = get_resize_ovp_option(v->blk_device);
  std::vector<std::string> make_f2fs_cmd = {
    "/system/bin/make_f2fs",
    "-g",
    "android",
  };

  make_f2fs_cmd.push_back("-O");
  make_f2fs_cmd.push_back("project_quota,extra_attr");

  if (needs_casefold) {
    make_f2fs_cmd.push_back("-O");
    make_f2fs_cmd.push_back("casefold");
    make_f2fs_cmd.push_back("-C");
    make_f2fs_cmd.push_back("utf8");
  }
  if (v->fs_mgr_flags.fs_compress) {
    make_f2fs_cmd.push_back("-O");
    make_f2fs_cmd.push_back("compression");
    make_f2fs_cmd.push_back("-O");
    make_f2fs_cmd.push_back("extra_attr");
  }
  if (!ovp_option.empty()) {
    make_f2fs_cmd.push_back("-o");
    make_f2fs_cmd.push_back(ovp_option);
  }
  make_f2fs_cmd.push_back(v->blk_device);
  if (length >= kSectorSize) {
    make_f2fs_cmd.push_back(std::to_string(length / kSectorSize));
  }

  if (exec_cmd(make_f2fs_cmd) != 0) {
    PLOG(ERROR) << "format_volume: Failed to make_f2fs on " << v->blk_device;
    return -1;
  }
  if (!directory.empty()) {
    std::vector<std::string> sload_f2fs_cmd = {
      "/system/bin/sload_f2fs", "-f", directory, "-t", volume, v->blk_device,
    };
    if (exec_cmd(sload_f2fs_cmd) != 0) {
      PLOG(ERROR) << "format_volume: Failed to sload_f2fs on " << v->blk_device;
      return -1;
    }
  }
  return 0;
}

int format_volume(const std::string& volume) {
  return format_volume(volume, "");
}

int setup_install_mounts() {
  if (fstab.empty()) {
    LOG(ERROR) << "can't set up install mounts: no fstab loaded";
    return -1;
  }
  for (const FstabEntry& entry : fstab) {
    // We don't want to do anything with "/".
    if (entry.mount_point == "/") {
      continue;
    }

    if (entry.mount_point == "/tmp" || entry.mount_point == "/cache") {
      if (ensure_path_mounted(entry.mount_point) != 0) {
        LOG(ERROR) << "Failed to mount " << entry.mount_point;
        return -1;
      }
    } else {
      if (ensure_path_unmounted(entry.mount_point) != 0) {
        LOG(ERROR) << "Failed to unmount " << entry.mount_point;
        return -1;
      }
    }
  }
  return 0;
}

bool HasCache() {
  CHECK(!fstab.empty());
  static bool has_cache = volume_for_mount_point(CACHE_ROOT) != nullptr;
  return has_cache;
}

//MIUI ADD: START for use com.android.runtime
static int extract_to_file(const std::string extract_path, ZipArchiveHandle zip, const std::string apex_file) {
  std::string_view apex_file_name(apex_file.c_str());
  ZipEntry apex_file_entry;
  if (FindEntry(zip, apex_file_name, &apex_file_entry) != 0) {
    LOG(ERROR) << "Failed to find apex_zip " << apex_file.c_str();
    return -1;
  }

  unlink(extract_path.c_str());
  android::base::unique_fd fd(
          open(extract_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 0755));
  if (fd == -1) {
    PLOG(ERROR) << "Failed to create " << extract_path;
    return -1;
  }

  int32_t error = ExtractEntryToFile(zip, &apex_file_entry, fd);
  if (error != 0) {
    LOG(ERROR) << "Failed to extract " << apex_file.c_str() << ": " << strerror(error);
    return -1;
  }
  return 0;
}

static int mount_loop_image(std::string apex_zip) {
  ZipArchiveHandle handle;
  int apexNameStart = apex_zip.rfind("/");
  int apexNameEnd = apex_zip.rfind(".");
  const std::string mountPoint = kApexRuntimeMountPoint + apex_zip.substr(apexNameStart, apexNameEnd - apexNameStart);
  if (mkdir(mountPoint.c_str(), 0700)) {
    LOG(ERROR) << "Failed to creat mount Point " << mountPoint;
    return -1;
  }
  // if apex zip is Compressed Apex Package, than uncompress the CAPEX
  if (apex_zip.find(".capex") != std::string::npos || apex_zip.find("compressed.apex") != std::string::npos) {
    const std::string extract_path = "/tmp/original_apex";
    const std::string capex_file = "original_apex";
    int rc = OpenArchive(apex_zip.c_str(), &handle);
    if(rc != 0) {
      LOG(ERROR) << "Failed to open zip " << apex_zip;
      return -1;
    }
    auto close_guard =
          android::base::make_scope_guard([&handle]() { CloseArchive(handle); });
    if(extract_to_file(extract_path, handle, capex_file) != 0) {
      LOG(ERROR) << "Failed to extract " << capex_file << " to " << extract_path;
      return -1;
    }
    // replace apex_zip
    apex_zip = extract_path;
  }
  // Ready to mount apex file
  int rc = OpenArchive(apex_zip.c_str(), &handle);
  if(rc != 0) {
    LOG(ERROR) << "Failed to open zip " << apex_zip;
    return -1;
  }
  auto close_guard =
          android::base::make_scope_guard([&handle]() { CloseArchive(handle); });
  const std::string extract_path = "/tmp/loop.img";
  const std::string apex_file = "apex_payload.img";
  if(extract_to_file(extract_path, handle, apex_file) != 0) {
    LOG(ERROR) << "Failed to extract " << apex_file << " to " << extract_path;
    return -1;
  }
  android::base::unique_fd fd(TEMP_FAILURE_RETRY(open(extract_path.c_str(), O_RDONLY | O_CLOEXEC)));
  for (size_t n = 0;; n++) {
    std::string tmp = android::base::StringPrintf("/dev/block/loop%zu", n);
    android::base::unique_fd loop(TEMP_FAILURE_RETRY(open(tmp.c_str(), O_RDONLY | O_CLOEXEC)));
    if (loop < 0) {
      LOG(ERROR) << "Failed to open " << tmp;
      return -1;
    }
    loop_info l_info;
    /* if it is a blank loop device */
    if (ioctl(loop, LOOP_GET_STATUS, &l_info) < 0 && errno == ENXIO) {
      /* if it becomes our loop device */
      if (ioctl(loop, LOOP_SET_FD, fd.get()) >= 0) {
        if (mount(tmp.c_str(), mountPoint.c_str(), "ext4", MS_RDONLY, nullptr) < 0) {
          ioctl(loop, LOOP_CLR_FD, 0);
          LOG(ERROR) << "Failed to mount()";
          return -1;
        }
        return 0;
      }
    }
  }
  LOG(ERROR) << "Failed to mount loop: out of loopback devices";
  return -1;
}

int mount_apex_runtime() {
  std::string art_apex_zip;
  std::string i18n_apex_zip;
  std::string statsd_apex_zip;
  
  if(access((kApexDir + kApexArtDebugFile).c_str(), F_OK) == 0) {
    art_apex_zip = kApexDir + kApexArtDebugFile;
  } else if (access((kApexDir + kApexArtReleaseFile).c_str(), F_OK) == 0) {
    art_apex_zip = kApexDir + kApexArtReleaseFile;
  } else if (access((kApexDir + kApexArtFile).c_str(), F_OK) == 0) {
    art_apex_zip = kApexDir + kApexArtFile;
  } else if (access((kApexDir + kCapexArtFile).c_str(), F_OK) == 0) {
    art_apex_zip = kApexDir + kCapexArtFile;
  } else if (access((kApexDir + kCapexGooleArtFile).c_str(), F_OK) == 0) {
    art_apex_zip = kApexDir + kCapexGooleArtFile;
  } else {
    LOG(ERROR) << "art apex file don't exist, may be system has not mount";
    return -1;
  }

  if(mount_loop_image(art_apex_zip) !=0 ) {
    LOG(ERROR) << "can't find and mount loop image for " << art_apex_zip;
    return -1;
  }

  if (access((kApexDir + kApexi18nFile).c_str(), F_OK) == 0) {
    i18n_apex_zip = kApexDir + kApexi18nFile;
    if (mount_loop_image(i18n_apex_zip) !=0 ) {
      LOG(ERROR) << "can't find and mount loop image for " << i18n_apex_zip;
    }
  } else {
    LOG(ERROR) << "i18n apex file don't exist, may be system has not mount";
  }

  if (access((kApexDir + kApexStatsdFile).c_str(), F_OK) == 0) {
    statsd_apex_zip = kApexDir + kApexStatsdFile;
    if (mount_loop_image(statsd_apex_zip) !=0 ) {
      LOG(ERROR) << "can't find and mount loop image for " << statsd_apex_zip;
    }
  } else {
    LOG(ERROR) << "statsd apex file don't exist, may be system has not mount";
  }
  return 0;
}
// END
