/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef _LOGGING_H
#define _LOGGING_H

#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <vector>

#include <log/log_id.h>

static constexpr int KEEP_LOG_COUNT = 10;

struct selabel_handle;

struct saved_log_file {
  std::string name;
  struct stat sb;
  std::string data;
};

// MIUI ADD: START
static constexpr const char* CACHE_ROOT = "/cache";
static constexpr const char* LOG_FILE = "/cache/recovery/log";
static constexpr const char* LAST_INSTALL_FILE_NOT_IN_CACHE = "/data/misc/recovery/last_install";
static constexpr const char* LAST_INSTALL_FILE = "/cache/recovery/last_install";
static constexpr const char* LAST_KMSG_FILE = "/cache/recovery/last_kmsg";
static constexpr const char* LAST_LOG_FILE = "/cache/recovery/last_log";
static constexpr const char* LAST_KMSG_FILTER = "recovery/last_kmsg";
static constexpr const char* LAST_LOG_FILTER = "recovery/last_log";
static constexpr const char* CACHE_LOG_DIR = "/cache/recovery";
static constexpr const char* COMMAND_FILE = "/cache/recovery/command";
static constexpr const char* LOCALE_FILE = "/cache/recovery/last_locale";
static constexpr const char* LAST_EXTENSION_PARAMETERS_FILE = "/cache/recovery/last_extension_parameters";
static const std::vector<std::string> EraseCacheWhitelist{
      "/cache/mqsas",
      "/cache/update_engine_log",
      "/cache/system_optimize/performance",
      "/cache/system_optimize/power",
      "/cache/system_optimize/thermal",
};
// END
static constexpr const char *TEMPORARY_INSTALL_FILE = "/tmp/last_install";
static constexpr const char* PERSIST_ROOT = "/persist";

void SetLoggingSehandle(selabel_handle* handle);

ssize_t logbasename(log_id_t id, char prio, const char* filename, const char* buf, size_t len,
                    void* arg);

ssize_t logrotate(log_id_t id, char prio, const char* filename, const char* buf, size_t len,
                  void* arg);

// Rename last_log -> last_log.1 -> last_log.2 -> ... -> last_log.$max.
// Similarly rename last_kmsg -> last_kmsg.1 -> ... -> last_kmsg.$max.
// Overwrite any existing last_log.$max and last_kmsg.$max.
void rotate_logs(const char* last_log_file, const char* last_kmsg_file);

// In turn fflush(3)'s, fsync(3)'s and fclose(3)'s the given stream.
void check_and_fclose(FILE* fp, const std::string& name);

void copy_log_file_to_pmsg(const std::string& source, const std::string& destination);
void copy_logs(bool save_current_log);
void reset_tmplog_offset();

void save_kernel_log(const char* destination);

std::vector<saved_log_file> ReadLogFilesToMemory();

bool RestoreLogFilesAfterFormat(const std::vector<saved_log_file>& log_files);
FILE* fopen_path(const std::string& path, const char* mode, selabel_handle* sehandle = nullptr);

#endif  //_LOGGING_H
