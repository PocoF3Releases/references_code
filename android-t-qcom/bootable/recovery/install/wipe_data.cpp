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

#include "install/wipe_data.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <functional>
#include <vector>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>

#include "install/snapshot_utils.h"
#include "otautil/dirutil.h"
#include "recovery_ui/ui.h"
#include "recovery_utils/logging.h"
#include "recovery_utils/roots.h"

// MIUI DEL
// constexpr const char* CACHE_ROOT = "/cache";
constexpr const char* DATA_ROOT = "/data";
constexpr const char* METADATA_ROOT = "/metadata";
// MIUI ADD: START
constexpr const char *CACHE_EFS_FILE = "/cache/recovery/efs";
constexpr const char *CACHE_NEW_EFS_FILE = "/cache/recovery/new_efs";
constexpr const char *CACHE_LAST_FORMAT_DATA_FILE = "/cache/recovery/last_format_data";
// END

bool EraseVolume(const char* volume, RecoveryUI* ui, bool convert_fbe, bool is_new_factoryreset) {
  bool is_cache = (strcmp(volume, CACHE_ROOT) == 0);
  bool is_data = (strcmp(volume, DATA_ROOT) == 0);
// MIUI ADD: START
  bool is_efs_file_exist = false;
  bool is_new_efs_file_exist = false;
// END

  std::vector<saved_log_file> log_files;
  if (is_cache) {
    // If we're reformatting /cache, we load any past logs (i.e. "/cache/recovery/last_*") and the
    // current log ("/cache/recovery/log") into memory, so we can restore them after the reformat.
    log_files = ReadLogFilesToMemory();
// MIUI ADD: START
    if (access(CACHE_EFS_FILE, F_OK) == 0) {
      is_efs_file_exist = true;
    } else if (access(CACHE_NEW_EFS_FILE, F_OK) == 0) {
      is_new_efs_file_exist = true;
    }
// END
  }

  // MIUI ADD: START
  // CQR intercept non 1217 mode
  if (is_data && !is_new_factoryreset) {
    android::base::WriteStringToFile("1", CACHE_LAST_FORMAT_DATA_FILE);
    if (chmod(CACHE_LAST_FORMAT_DATA_FILE, 0644) < 0) {
      printf("chmod error: %s\n", strerror(errno));
    }
  }
  // END

  ui->Print("Formatting %s...\n", volume);

  ensure_path_unmounted(volume);

  int result;
  if (is_data && convert_fbe) {
    constexpr const char* CONVERT_FBE_DIR = "/tmp/convert_fbe";
    constexpr const char* CONVERT_FBE_FILE = "/tmp/convert_fbe/convert_fbe";
    // Create convert_fbe breadcrumb file to signal init to convert to file based encryption, not
    // full disk encryption.
    if (mkdir(CONVERT_FBE_DIR, 0700) != 0) {
      PLOG(ERROR) << "Failed to mkdir " << CONVERT_FBE_DIR;
      return false;
    }
    FILE* f = fopen(CONVERT_FBE_FILE, "wbe");
    if (!f) {
      PLOG(ERROR) << "Failed to convert to file encryption";
      return false;
    }
    fclose(f);
    result = format_volume(volume, CONVERT_FBE_DIR);
    remove(CONVERT_FBE_FILE);
    rmdir(CONVERT_FBE_DIR);
  } else {
    result = format_volume(volume);
  }

  if (is_cache) {
    RestoreLogFilesAfterFormat(log_files);
// MIUI ADD: START
    if (is_efs_file_exist) {
      FILE* fp = fopen_path(CACHE_EFS_FILE, "w");
      if (fp == nullptr) {
          LOG(ERROR) << "Can't open " << CACHE_EFS_FILE;
      } else {
          check_and_fclose(fp, CACHE_EFS_FILE);
      }
    } else if (is_new_efs_file_exist) {
      FILE* fp = fopen_path(CACHE_NEW_EFS_FILE, "w");
      if (fp == nullptr) {
          LOG(ERROR) << "Can't open " << CACHE_NEW_EFS_FILE;
      } else {
          check_and_fclose(fp, CACHE_NEW_EFS_FILE);
      }
    }
// END
  }

  return (result == 0);
}

bool WipeCache(RecoveryUI* ui, const std::function<bool()>& confirm_func) {
  bool has_cache = volume_for_mount_point("/cache") != nullptr;
  if (!has_cache) {
    ui->Print("No /cache partition found.\n");
    return false;
  }

  if (confirm_func && !confirm_func()) {
    return false;
  }

  ui->Print("\n-- Wiping cache...\n");
  ui->SetBackground(RecoveryUI::ERASING);
  ui->SetProgressType(RecoveryUI::INDETERMINATE);

  bool success = EraseVolume("/cache", ui, false);
  ui->Print("Cache wipe %s.\n", success ? "complete" : "failed");
  return success;
}

bool WipeData(Device* device, bool convert_fbe, bool is_new_factoryreset) {
  RecoveryUI* ui = device->GetUI();
  ui->Print("\n-- Wiping data...\n");
  ui->SetBackground(RecoveryUI::ERASING);
  ui->SetProgressType(RecoveryUI::INDETERMINATE);

  if (!FinishPendingSnapshotMerges(device)) {
    ui->Print("Unable to check update status or complete merge, cannot wipe partitions.\n");
    return false;
  }

  bool success = device->PreWipeData();
  if (success) {
    success &= EraseVolume(DATA_ROOT, ui, convert_fbe, is_new_factoryreset);
    bool has_cache = volume_for_mount_point("/cache") != nullptr;
    if (has_cache) {
      success &= EraseVolume(CACHE_ROOT, ui, false, is_new_factoryreset);
    }
    if (volume_for_mount_point(METADATA_ROOT) != nullptr) {
      success &= EraseVolume(METADATA_ROOT, ui, false, is_new_factoryreset);
    }
  }
  if (success) {
    success &= device->PostWipeData();
  }
  ui->Print("Data wipe %s.\n", success ? "complete" : "failed");
  return success;
}
