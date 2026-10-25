/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <android-base/logging.h>
#include <libdm/dm.h>
#include <libsnapshot/snapshot.h>
#include "recovery_utils/roots.h"
#include "cutils/properties.h"

#include "install/mi_wipe.h"
#include "install/wipe_data.h"
#include "miutil/mi_utils.h"
#include "install/snapshot_utils.h"
using android::snapshot::UpdateState;
static constexpr const char *CACHE_EFS_FILE = "/cache/recovery/efs";
static constexpr const char *CACHE_NEW_EFS_FILE = "/cache/recovery/new_efs";

static bool has_efs_file() {
  if (access(CACHE_EFS_FILE, F_OK) == 0) {
    return true;
  }
  return false;
}

static bool has_new_efs_file() {
  if (access(CACHE_NEW_EFS_FILE, F_OK) == 0) {
    return true;
  }
  return false;
}

static int erase_efs(const char *partition_name) {
  char *dev_path = NULL;
  int fd;
  int blk_size;
  unsigned long long total_size;
  int blocks;
  void *buf = NULL;
  int len = 0;

  LOG(WARNING) << "erase " << partition_name;

  asprintf(&dev_path, "/dev/block/bootdevice/by-name/%s", partition_name);
  if (dev_path == NULL) {
    return ENOMEM;
  }
  fd = open(dev_path, O_RDWR);
  free(dev_path);
  if (fd < 0) {
    LOG(ERROR) << "no partition named: " << partition_name;
    return ENOENT;
  }

  if (ioctl(fd, BLKSSZGET, &blk_size) != 0) {
    close(fd);
    LOG(ERROR) << "get blk size for " << partition_name << "fail";
    return EIO;
  }

  if (ioctl(fd, BLKGETSIZE64, &total_size) != 0) {
    close(fd);
    LOG(ERROR) << "get total_size for " << partition_name << "fail";
    return EIO;
  }

  blocks = total_size / blk_size;

  buf = malloc(blk_size);
  if (buf == NULL) {
    close(fd);
    LOG(ERROR) << "malloc blk fail";
    return ENOMEM;
  }
  memset(buf, 0, blk_size);

  len = 0;
  while (blocks-- > 0) {
    len += write(fd, buf, blk_size);
  }
  close(fd);
  free(buf);
  LOG(ERROR) << "erase " << len << " bytes for " << partition_name;
  return 0;
}

static int fixFilePerm(const char *path)
{
    int fd;
    unsigned int flags;
    int ret = -1;

    printf("Fixing %s\n", path);
    fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    if (ioctl(fd, FS_IOC_GETFLAGS, &flags) == -1) {
        perror("ioctl");
        goto out;
    }

    if ((flags & (FS_SECRM_FL | FS_IMMUTABLE_FL)) != 0) {
        flags &= ~(FS_SECRM_FL | FS_IMMUTABLE_FL);
        if (ioctl(fd, FS_IOC_SETFLAGS, &flags) == -1) {
            perror("ioctl");
            goto out;
        } else {
            printf("Fix %s success\n", path);
            ret = 0;
            goto out;
        }
    }

out:
    close(fd);
    return ret;
}

static int dirUnlinkHierarchy(const char *path, const char* skip_paths[])
{
    struct stat st;
    DIR *dir;
    struct dirent *de;
    int fail = 0;

    /* is it a file or directory? */
    if (lstat(path, &st) < 0) {
        return -1;
    }

    /* a file, so unlink it */
    if (!S_ISDIR(st.st_mode)) {
        int ret = unlink(path);
        if (ret) {
            // C3C(may be all MTK products) has a file /data/vendor/dumpsys/mrdump_preallocated
            // with FS_SECRM_FL and FS_IMMUTABLE_FL file flag set. We try to
            // remove these flag bits.
            int saved_errno = errno;
            if (fixFilePerm(path) == 0) {
                // Try again, and we use the new errno.
                ret = unlink(path);
            } else {
                // Fix failed, so we reset errno
                errno = saved_errno;
            }
        }
        return ret;
    }

    /* a directory, so open handle */
    dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }

    /* recurse over components */
    errno = 0;
    while ((de = readdir(dir)) != NULL) {
        char dn[PATH_MAX];
        if (!strcmp(de->d_name, "..") || !strcmp(de->d_name, ".")) {
            continue;
        }
        snprintf(dn, sizeof(dn), "%s/%s", path, de->d_name);
        if (skip_paths != NULL) {
            const char** p = skip_paths;
            for (; *p != NULL; p++) {
                if (strcmp(*p, dn) == 0) {
                    if (strcmp("/data/miui", dn) == 0 && dirUnlinkHierarchy(dn, skip_paths) < 0) {
                        fail = 1;
                        break;
                    }
                    errno = 0;
                    break;
                }
            }
            if (*p != NULL) continue;
        }
        if (dirUnlinkHierarchy(dn, NULL) < 0) {
            fail = 1;
            break;
        }
        errno = 0;
    }
    /* in case readdir or unlink_recursive failed */
    if (fail || errno < 0) {
        int save = errno;
        closedir(dir);
        errno = save;
        return -1;
    }

    /* close directory handle */
    if (closedir(dir) < 0) {
        return -1;
    }

    /* delete target directory */
    return skip_paths != NULL ? 0 : rmdir(path);
}

int internal_erase_cache(RecoveryUI* ui)
{
    printf("** Erase cache **\n");

    bool ret = EraseVolume("/cache", ui);
    if (ret) {
        LOG(ERROR) << "Erase cache successfully";
        return 0;
    } else {
        LOG(ERROR) << "Failed to erase cache: " << strerror(errno);
        return -1;
    }
}

static int
_internal_wipe_data_partial_erase_cache(const char* skip_paths[], RecoveryUI* ui)
{
    printf("** Wipe data partial and erase cache **\n");
    auto mergestate = ReadSnapshotMergeState();
    if(mergestate==UpdateState::Merging || mergestate==UpdateState::Unverified){
      if (!FinishPendingSnapshotMerges(ui->GetDevice())) {
        ui->Print("Unable to check update status or complete merge, cannot wipe partitions.\n");
        return false;
      }
    }
    //1.print information
    printf("Skip paths:\n");
    for (int i = 0; skip_paths[i] != NULL; ++i) {
        printf("%s\n", skip_paths[i]);
    }
    //2.format cache partition
    bool ret = EraseVolume("/cache", ui);
    if (!ret) {
        LOG(ERROR) << "Failed to erase cache: " << strerror(errno);
    }
    //3.delete files in data partition and skip some path.
    ret = (ensure_path_mounted("/data") == 0) &&
          (dirUnlinkHierarchy("/data", skip_paths) == 0);

    if (ret) {
        LOG(ERROR) << "Wipe data partial successfully.";
        std::vector<std::string> ls_args = {
          "/system/bin/ls", "-al", "/data",
        };
        exec_cmd(ls_args);
        std::vector<std::string> lsof_args = {
          "/system/bin/lsof",
        };
        exec_cmd(lsof_args);
        ret = ensure_path_unmounted("/data");
        printf("ensure_path_unmounted data ret %d\n", ret);
        android::dm::DeviceMapper& dm = android::dm::DeviceMapper::Instance();
        ret = dm.DeleteDevice("userdata");
        if (!ret) {
            LOG(ERROR) << "Delete Data Devices Failed!";
        }
        unlink("/metadata/vold/checkpoint");
        return 0;
    } else {
        LOG(ERROR) << "Failed to wipe data partitial: " << strerror(errno);

        return -1;
    }
}

int internal_wipe_data_partial_erase_cache(RecoveryUI* ui)
{
    //Only support root dirs. we should hard code internel.
    const char* skip_paths[] = {
        "/data/miui",
        "/data/miui/app",
        NULL,
    };

    return _internal_wipe_data_partial_erase_cache(skip_paths, ui);
}

int internal_wipe_efs(void)
{
    erase_efs("modemst1");
    erase_efs("modemst2");
    erase_efs("fsc");
    return 0;
}


static const char *WIPE_EFS_WHITE_LIST[] = {
    "sagit",    // C1
    "centaur",  // C2
    "oxygen",   // D4
    "jason",    // C8
    "chiron",   // D5
    "dipper",   // E1
    "polaris",  // D5X
    "beryllium",// E10
    "sirius",   // E2
    "comet",    // E20
    "nitrogen", // E4
    "ursa",     // E8
    "sakura",   // D1S
    "perseus",  // E5
    "platina",  // D2T
    "equuleus", // E1S
    "cepheus",  // F1
    "andromeda",  // e5g
    "grus",     // F2
    "vela",     // F3M
    "pyxis",    // F3B
    "davinci",  // F10
    "davinciin",// F10-india
    "hercules", // F1B
    "raphael", // F11
    "raphaelin", // F11-india
    "crux", // F1X
    "draco", // U2
    "phoenix", //G7B
    "phoenixin", //G7B-india
    "picasso", //G7A
    "picassoin", //G7A-india
    "cmi", //J1
    "umi", //J2
    "toco", //F4L
    "tucana", //F4
    "merlin", //J15S
    "monet", //J9
    "monetin", //J9-india
    "vangogh", //J9A
    "courbet", //K9A
    "courbetin", //K9A_IN
    "sweet", //K6
    "sweetin", //K6_IN
    "gauguin", //J17
    "gauguinpro", //J17-Pro
    "gauguininpro", //J17in-Pro
    "vayu", //J20S-global
    "bhima", //J20S-india
    "ingres", //L10
};

int should_wipe_efs_while_wipe_data() {
    char device_name[PROPERTY_VALUE_MAX] = {0};
    int pos;

    if (has_new_efs_file()) {
        return 1;
    }

    if (!has_efs_file()) {
        LOG(ERROR) << "no efs flag file, no efs clear";
        return 0;
    }

    property_get("ro.product.device", device_name, "");
    for (pos = 0; pos < (int)(sizeof(WIPE_EFS_WHITE_LIST) / sizeof(WIPE_EFS_WHITE_LIST[0])); pos++) {
        if (strcmp(device_name, WIPE_EFS_WHITE_LIST[pos]) == 0) {
            return 1;
        }
    }
    LOG(ERROR) << "This device is not support clear efs";
    return 0;
}

int internal_erase_data_cache(Device* device, bool convert_fbe)
{
    bool has_cache = volume_for_mount_point("/cache") != nullptr;
    bool success = WipeData(device, convert_fbe, device->GetReason().value_or("") == "factroy_reset(1217)");
    if (success) {
        int ret = erase_efs("vm-data");
        if (ret != 0 && ret != ENOENT) {
            success = false;
        }
    }
    if (success) {
        LOG(ERROR) << "Erase cache and data successfully!";
        return 0;
    } else {
        LOG(ERROR) << "Erase cache and data failed: " << strerror(errno);
        return -1;
    }
}

int internal_erase_storage(RecoveryUI* ui)
{
    bool ret = true;

    printf("** Erase storage **\n");
    if (!FinishPendingSnapshotMerges(ui->GetDevice())) {
      ui->Print("Unable to check update status or complete merge, cannot wipe partitions.\n");
      return false;
    }
#if defined(WITH_STORAGE_INT)
    ret = erase_volume("/storage_int");
#else
    ret = (ensure_path_mounted("/data") == 0);
    if (!ret) goto done;
    if (access("/data/media", F_OK)) {
        LOG(ERROR) << "/data/media not exist";
        goto done;
    }

    ret = (dirUnlinkHierarchy("/data/media", NULL) == 0);
    if (ret) {
        ret = (unlink("/data/.layout_version") == 0);
    }
#endif /* WITH_STORAGE_INT */

done:
    if (ret) {
        LOG(ERROR) << "Erase storage successfully.";
        return 0;
    } else {
        LOG(ERROR) << "Failed to erase storage: " << strerror(errno);
        return -1;
    }
}
