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
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <cutils/properties.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/parseint.h>
#include <android-base/unique_fd.h>
#include <android-base/file.h>
#include <android-base/properties.h>
#include <private/android_filesystem_config.h> /* for AID_SYSTEM */
#include <fs_mgr.h>
#include <sys/mount.h>
#include <fcntl.h>
#include "recovery_utils/logging.h"

#include <cstring>
#include <fstream>
#include <string>

#include "miutil/mi_utils.h"
#include "recovery_utils/logging.h"
#include "updater/mounts.h"
#include "recovery_utils/roots.h"
#include "otautil/dirutil.h"
#include <thread>

#ifndef MINIZ_HEADER_FILE_ONLY
#define MINIZ_HEADER_FILE_ONLY
#include "minadbd/mi_miniz.c"
#undef MINIZ_HEADER_FILE_ONLY
#endif

#ifdef MTK_VENDOR
#include "mt_roots.h"
#endif

int min_core = -1;
int max_core = -1;
int biglittle_threshold = -1;
cpuinfo* current_cpu= NULL;

cpuinfo cpu_list[] = {
    {"Qualcomm Technologies, Inc MSM8956", 0, 3, -1, -1, 4, 5, 2, 0, 1,  "performance", 0, "performance", 0, "performance", 0},   //h3a b3 h3b
    {"Qualcomm Technologies, Inc MSM8996", 0, 1, -1, -1, 2, 3, 2, 0, 1,  "performance", 0, "performance", 0, "performance", 0},   //a1
    {"Qualcomm Technologies, Inc MSM8996pro", 0, 1, -1, -1, 2, 3, 2, 0, 1,  "performance", 0, "performance", 0, "performance", 0},   //b7 a4 a8 a7
    {"Qualcomm Technologies, Inc MSM8937", 4, 7, -1, -1, 0, 3, 2, 0, 2,  "performance", 0, "performance", 0, "performance", 0},   //a12 c5   1:1401000    4-7:1094400
    {"Qualcomm Technologies, Inc MSM8940", 4, 7, -1, -1, 0, 3, 2, 0, 2,  "performance", 0, "performance", 0, "performance", 0},   //a13
    {"Qualcomm Technologies, Inc MSM8953", 0, 7, -1, -1, -1, -1, 4, 0, 0,  "performance", 0, "performance", 0, "performance", 0},   //b5w  0-7:2016000
    {"Qualcomm Technologies, Inc MSM8917", 0, 3, -1, -1, -1, -1, 4, 0, 0,  "performance", 0, "performance", 0, "performance", 0},   //c3q  0-3:1401000
    {"MT6797M", 0, 3, 4, 7, 8, 9, 2, 0, 1, "performance", 0, "performance", 0, "performance", 0},   //b6l,h3c  0-3:1391000   4-7:   1846000   8-9:2106000
    {"MT6797T", 0, 3, 4, 7, 8, 9, 2, 2, 1, "performance", 0, "performance", 0, "performance", 0},   //h3c 0:1547000   4-7:2002000  8-9:2522000
    {"MT6765", 0, 3, -1, -1, -1, -1, 4, 0, 0, "performance", 0, "performance", 0, "performance", 0}    //C3C C3D  0-3 2001000
};

static const char *PROC_KMSG = "/proc/last_kmsg";
static const char *CONSOLE_RAMOOPS_0 = "/sys/fs/pstore/console-ramoops-0";
static const char *CONSOLE_RAMOOPS = "/sys/fs/pstore/console-ramoops";
static const std::string CONFIG_DISABLE_FORMAT = "config.disable_format";
static std::string LAST_DEMSG = android::base::StringPrintf("%s/last_demsg", CACHE_LOG_DIR);

enum class WaitResult { Wait, Done, Fail };
static bool wait_for_condition(const std::function<WaitResult()>& condition,
                             std::optional<std::chrono::milliseconds> timeout) {
  auto start_time = std::chrono::steady_clock::now();
  while (true) {
    auto result = condition();
    if (result == WaitResult::Done) return true;
    if (result == WaitResult::Fail) return false;

    std::this_thread::sleep_for(std::chrono::milliseconds (20));

    auto now = std::chrono::steady_clock::now();
    auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
    if (time_elapsed > timeout) return false;
  }
}

bool wait_for_file(const std::string& path, std::optional<std::chrono::milliseconds> timeout) {
  auto condition = [&]() -> WaitResult {
      // If the file exists but returns EPERM or something, we consider the
      // condition met.
      if (access(path.c_str(), F_OK) != 0) {
        if (errno == ENOENT) {
          return WaitResult::Wait;
        }
        PLOG(ERROR) << "access failed: " << path;
        return WaitResult::Fail;
      }
      return WaitResult::Done;
  };
  return wait_for_condition(condition, timeout);
}

bool need_erase_userdata(void)
{
    bool is_encrypt = false;

    if (ensure_path_mounted("/data") == 0) {
        return false;
    } else if (errno != EBUSY && errno != EACCES) {
        is_encrypt = true;
    }

    char has_cust[PROPERTY_VALUE_MAX] = {0};
    property_get("ro.miui.has_cust_partition", has_cust, "false");
    if (strcmp(has_cust, "true") == 0 && is_encrypt) {
        return true;
    }

    return false;
}

uint64_t gettime_ns() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<uint64_t>(now.tv_sec) * UINT64_C(1000000000) + now.tv_nsec;
}

// close a file, log an error if the error indicator is set
static void check_and_fclose(FILE *fp, const char *name) {
    fflush(fp);
    if (ferror(fp)) LOG(ERROR) << "Error in " << name << ":" << strerror(errno);
    fclose(fp);
}

static char* load_info_from_local(const char *path) {
    char*msg = NULL;
    FILE* fp = fopen(path, "r");
    char buffer[80];
    if (fp != NULL) {
        fgets(buffer, sizeof(buffer), fp);
        msg = strdup(buffer);
        check_and_fclose(fp, path);
    }
    return msg;
}

static void write_info_to_local(const char *path, const char *info) {
    if (info != NULL) {
        FILE* fp = fopen(path, "w");
        if (fp == NULL) {
            LOG(ERROR) << "Can't open " << path << ":" << strerror(errno);
        } else {
            fwrite(info, 1, strlen(info), fp);
            fsync(fileno(fp));
            check_and_fclose(fp, path);
        }
    }
}

static char* get_cpu_model() {
    char * temp = NULL;
    char  hardware[16];
    size_t sz = 0;

    char *cpumodel = (char *)malloc (sizeof (char) * 64);
    if (cpumodel == NULL){
        LOG(ERROR) << "cpumodel failed to allocate 64 bytes";
        return NULL;
    }

    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f == NULL) {
        LOG(ERROR) << "Can't open /proc/cpuinfo: " << strerror(errno);
        free(cpumodel);
        return NULL;
    }
    do {
        ssize_t lsz = getline(&temp, &sz, f);
        if (lsz<0) break;
        if (strcspn(temp, "Hardware") == 0) {
            if (sscanf(temp, "%s\t: %[^\n]", hardware, cpumodel) == 2) {
            break;
            }
        }
    } while (!feof (f));
    free(temp);
    fclose(f);
    return cpumodel;
}

static int strlen(char *str) {
    int len=0;
    while(*str++!='\0') {
        len++;
    }
    return len;
}

static int sparse_cpuinfo(){
    char* model = get_cpu_model();
    if (model != NULL) {
        if (!strncmp(model, "MT", 2)) {
            printf("disable HPS  for cpu model:%s\n", model);
            write_info_to_local(CPU_HPS_FILE, DISABLE);
        }

        if (current_cpu == NULL) {
            printf("init cpu info:%s\n", model);
            for (int i = 0; i < (int)ARRAY_SIZE(cpu_list); ++i) {
                if (!strcmp(model, cpu_list[i].model)) {
                    printf("cpu config set:%s\n", cpu_list[i].model);
                    current_cpu = &cpu_list[i];
                    break;
                }
            }
        }
        free(model);
    }
    if(min_core == -1){
        char *cpu_possible = load_info_from_local(CPU_POSSIBLE_FILE);
        if (cpu_possible != NULL && sscanf(cpu_possible, "%d-%d", &min_core, &max_core) == 2) {
            printf("refresh cpu possible number\n");
            // default big core is the biggest two number.
            // MTK is the biggest number of four , QCOM MSM8939(ido) is the smallest number of four.
            if (max_core > 2) {
                biglittle_threshold = max_core - 2;
            }
            free(cpu_possible);
            return 0;
        }
        if (cpu_possible != NULL) {
            printf("failed to parse cpu possible number: %s\n", cpu_possible);
            free(cpu_possible);
        }
        return -1;
    }
    return 0;
}

static void set_cpu_online(int core, const char* status){
    char cpu_online[64];
    sprintf(cpu_online, CPU_ONLINE_FILE, core);
    char *cpu_status = load_info_from_local(cpu_online);
    if (cpu_status != NULL && strncmp(status, cpu_status, 1)) {
        write_info_to_local(cpu_online, status);
    }
    if (cpu_status != NULL){
        free(cpu_status);
    }
}

static void scaling_cpu_governor(int core, const char* governor, int speed){
    char cpu_scaling[64];
    set_cpu_online(core, ENABLE);
    sprintf(cpu_scaling, CPU_GOVERNOR_FILE, core);
    write_info_to_local(cpu_scaling, governor);
    if (!strcmp(governor, "userspace")) {
        char cpu_speed[64];
        char freq[10];
        snprintf(freq, sizeof(freq), "%d", speed);
        sprintf(cpu_speed, CPU_SETSPEED_FILE, core);
        write_info_to_local(cpu_speed, freq);
    }
}

static void set_cpus_status(const char* governor, int scn_core_big, int scn_freq_big,
    int scn_core_little, int scn_freq_little) {
    int ll_core = 0;
    int bg_core = 0;
    for (int i = min_core; i <= max_core; ++i) {
        if (current_cpu == NULL) {
            if(i <= biglittle_threshold && ll_core < scn_core_little) {
                scaling_cpu_governor(i, governor, scn_freq_little);
                ++ll_core;
                continue;
            }
            if(i > biglittle_threshold && bg_core < scn_core_big) {
                scaling_cpu_governor(i, governor, scn_freq_big);
                ++bg_core;
                continue;
            }
        } else {
            if (i >= current_cpu->l_start && i <= current_cpu->l_end && ll_core < scn_core_little) {
                scaling_cpu_governor(i, governor, scn_freq_little);
                ++ll_core;
                continue;
            }
            if (i >= current_cpu->b_start && i <= current_cpu->b_end && bg_core < scn_core_big) {
                scaling_cpu_governor(i, governor, scn_freq_big);
                ++bg_core;
                continue;
            }
        }
        set_cpu_online(i, DISABLE);
    }
}

void set_cpu_governor(int governor) {
    if (sparse_cpuinfo() < 0 ) {
        printf("failed to parse cpuinfo \n");
        return;
    }
    switch (governor) {
        case DEFAULT:
            set_cpus_status("interactive", 0, 0, 2, 0);
            break;
        case POWERSAVE:
            set_cpus_status("interactive", 0, 0 ,1, 0);
            break;
        case CONFIGURATION:
            if (current_cpu == NULL) {
                set_cpus_status("performance", 1, 0, 2, 0);
            } else {
                set_cpus_status(current_cpu->scn_little_governor, current_cpu->scn_big_cores,
                    current_cpu->scn_big_speed, current_cpu->scn_little_cores,
                    current_cpu->scn_little_speed);
            }
            break;
        case FULLLOAD:
            for (int i = min_core; i <= max_core; ++i) {
                scaling_cpu_governor(i, "performance", 0);
            }
            break;
    }
}

void record_time(const char* pathname, float time, const char* time_type) {
    std::string pathstr(pathname);
    FILE* fp = fopen_path(pathstr, "w+");
    if (fp == NULL) {
        printf("Can't open %s: %s\n", pathname, strerror(errno));
    } else {
        char buf[1024] = { 0 };
        sprintf(buf, "%s:%.2f\n", time_type, time);
        fwrite(buf, 1, strlen(buf), fp);
        fsync(fileno(fp));
        check_and_fclose(fp, pathname);
        if (chmod(pathname, 0644) < 0) {  // enable group and others have read permission, like Updater.
            printf("chmod error: %s\n", strerror(errno));
        }
    }
}

void record_package_name(void) {
    const std::string uncrypt_file = android::base::StringPrintf("%s/last_updatefile", CACHE_LOG_DIR);
    if (!access(uncrypt_file.c_str(), F_OK)) {
        FILE* fp = fopen_path(uncrypt_file, "r");
        if (fp) {
            char buf[4096] = { 0 };
            if (fgets(buf, 4096, fp) != NULL || feof(fp)) {
                LOG(ERROR) << "the real name of update package is:" << buf;
            } else {
                LOG(ERROR) << "failed to read " << uncrypt_file << ":" << strerror(errno);
            }
            fclose(fp);
        } else {
            LOG(ERROR) << "failed to open " << uncrypt_file << ":" << strerror(errno);
        }
    } else {
        LOG(ERROR) << "find " << uncrypt_file << "  failed:" << strerror(errno);
    }
}

void rotate_logs_for_lastdemsg(const char *last_demsg, int keep_log_count)
{
    static bool rotated = false;
    if (rotated) {
        return;
    }
    rotated = true;
    for (int i = keep_log_count - 1; i >= 0; --i) {
        std::string old_log = android::base::StringPrintf("%s", last_demsg);
        if (i > 0) {
          old_log += "." + std::to_string(i);
        }
        std::string new_log = android::base::StringPrintf("%s.%d", last_demsg, i+1);
        // Ignore errors if old_log doesn't exist.
        rename(old_log.c_str(), new_log.c_str());
    }
}

int delete_minidump() {
    const char *minidumps[] = {"last_minidump.bin", "last_minidump.bin.1", "last_minidump.bin.2",
                "last_minidump.bin.3", "last_minidump.bin.4", "last_minidump.bin.5"};
    bool delete_all = true;
    for(int i = 0; i < 6; i ++) {
        char filename[64] = { 0 };
        sprintf(filename, "%s/%s", CACHE_LOG_DIR, minidumps[i]);
        if(!access(filename, F_OK)) {
            if(!unlink(filename)) {
                printf("Delete %s success!\n", filename);
            } else {
                printf("Delete %s failed!\n", filename);
                delete_all = false;
            }
        }
    }
    return delete_all ? 0 : -1;
}

int copy_demsg()
{
    printf("========Get last_demsg========\n");
    const char *minidump_path0 = "/dev/block/platform/bootdevice/by-name/minidump";
    const char *minidump_path1 = "/dev/block/bootdevice/by-name/minidump";
    if(!access(minidump_path0, F_OK))
        return copy_demsg_with_minidump(minidump_path0);
    else if (!access(minidump_path1, F_OK))
        return copy_demsg_with_minidump(minidump_path1);
    else
        return copy_demsg_without_minidump();
    printf("==============================\n");
}

static void copy_log_file(const char* source, const char* destination) {
  FILE* dest_fp = fopen_path(destination, "we");
  if (dest_fp == nullptr) {
    // PLOG(ERROR) << "Can't open " << destination;
    printf("cannot open %s\n", destination);
  } else {
    FILE* source_fp = fopen(source, "re");
    if (source_fp != nullptr) {
      char buf[4096];
      size_t bytes;
      while ((bytes = fread(buf, 1, sizeof(buf), source_fp)) != 0) {
        fwrite(buf, 1, bytes, dest_fp);
      }
      check_and_fclose(source_fp, source);
    }
    check_and_fclose(dest_fp, destination);
  }
}

/*
   this function will be called in the recovery.cpp:main()
   the content of last_demsg maybe existed in one of three files:
   1. /proc/last_kmsg
   2. /sys/fs/pstore/console-ramoops-0
   3. /sys/fs/pstore/console-ramoops
   if there is any file existed, which means a kernel crashed in the last boot,
   the reason of kernel crash would be saved in three files below, copy it to
   /cache/recovery/last_demsg
*/
int copy_demsg_without_minidump()
{
    int fd = -1;
    const char *sources[] = { PROC_KMSG, CONSOLE_RAMOOPS_0, CONSOLE_RAMOOPS };
    int i;
    for(i = 0; i < 3; i ++) {
        if(access(sources[i], F_OK) == 0) {
            fd = open(sources[i], O_RDONLY);
            if(fd == -1) {
                printf("open %s FAILED %d(%s)\n", sources[i], errno, strerror(errno));
            } else {
                off_t off = lseek(fd, 0, SEEK_END);
                if(off != 0)
                    break;
                else
                    printf("%s's length is 0", sources[i]);
                close(fd);
                fd = -1;
            }
        } else {
            printf("%s is not existed\n", sources[i]);
        }
    }

    if(fd != -1)
        close(fd);

    if(i == 3) {
        printf("/proc/last_kmsg, \
                /sys/fs/pstore/console-ramoops-0 \
                /sys/fs/pstore/console-ramoops \
                all read FAILED\n");
        return -1;
    }

    printf("collecting last_demsg from %s\n", sources[i]);
    rotate_logs_for_lastdemsg(sources[i], 5);
    ensure_path_mounted("/cache");
    FILE *dst = fopen(LAST_DEMSG.c_str(), "w+");
    if(dst != nullptr) {
        copy_log_file(sources[i], LAST_DEMSG.c_str());
        sync();
        fclose(dst);
        return 0;
    } else {
        printf("open %s FAILED %d(%s)\n", LAST_DEMSG.c_str(), errno, strerror(errno));
        return -1;
    }

}

int copy_demsg_with_minidump(const char *minidump_path)
{
    int fd = open(minidump_path, O_RDWR);
    if(fd != -1) {
        char buf[16];
        memset(buf, '\0', 16);
        /* the value of dump_header_signature can refer to
           vendor/xiaomi/proprietary/minidump/minidump.c

           struct mini_parition_dump_header {
               uint8 signature[8]; // dump_header_signature
               uint32 version;
               ......
               uint32 sections_count;
           } dump_header;
        */
        lseek(fd, 0, SEEK_SET);
        const int dump_header_signature = 8;
        read(fd, buf, dump_header_signature);
        if(!strcmp(buf, "Raw_Dmp!")) {
            printf("collecting last_demsg from %s\n", minidump_path);
            compress_minidump();

            // Invalidate the signature
            lseek(fd, 0, SEEK_SET);
            write(fd, "b", 1);
            close(fd);
            return 0;
        } else {
            printf("minidump is invalid\n");
            close(fd);
            return -1;
        }
    } else {
        printf("open %s FAILED %d(%s)\n", minidump_path, errno, strerror(errno));
        return -1;
    }
}

// MIUI ADD: START
#define MMC_BLOCK_SIZE (512)
#define OEM_UNLOCK_FLAG_SIZE (1)
#define FRP_CREDENTIAL_RESERVED_SIZE (1000)
#define ENT_ACCESS_BLOCK_SIZE (2048)
#define PAYJOY_CTRL_FLAG_POS(size) (size - OEM_UNLOCK_FLAG_SIZE \
                                    - FRP_CREDENTIAL_RESERVED_SIZE \
                                    - ENT_ACCESS_BLOCK_SIZE)
#define FRP_LINK "/dev/block/bootdevice/by-name/frp"
inline uint8_t get_control_bitpos() {
  // get realpath
  std::string realpath;
  char frp_path[PROPERTY_VALUE_MAX] = { 0 };

  property_get("ro.frp.pst", frp_path, FRP_LINK);
  if(!android::base::Realpath(frp_path, &realpath)) {
    fprintf(stderr, "failed to get realpath %s(%s)\n", frp_path, strerror(errno));
    return -1;
  }

  android::base::unique_fd fd(open(realpath.c_str(), O_RDONLY));
  if(fd == -1) {
    fprintf(stderr, "failed to open frp %d(%s)\n", errno, strerror(errno));
    return -1;
  }
  uint64_t partition_size = -1;
  if((partition_size = lseek(fd, 0, SEEK_END)) == -1) {
    fprintf(stderr, "failed to open lseek %d(%s)\n", errno, strerror(errno));
    return -1;
  }

  uint64_t control_bit_pos = PAYJOY_CTRL_FLAG_POS(partition_size);
  if(lseek(fd, control_bit_pos, SEEK_SET) == -1) {
    fprintf(stderr, "failed to open lseek %d(%s)\n", errno, strerror(errno));
    return -1;
  }

  uint8_t bit;
  int nread = -1;
  if((nread = read(fd, &bit, 1)) != 1) {
    fprintf(stderr, "failed to open read %d(%s)\n", errno, strerror(errno));
    return -1;
  }
  return bit;
}

bool disable_format(void) {
  if(get_control_bitpos() == 1 || android::base::GetProperty(CONFIG_DISABLE_FORMAT, "0") == "1") {
    return true;
  } else {
    return false;
  }
}

void set_disable_factoryreset() {
    if (!android::base::SetProperty(CONFIG_DISABLE_FORMAT, "1")) {
        LOG(ERROR) << "Failed to reset the warm reset flag";
    }
}

inline bool isFile(const std::string& path) {
    FILE *fp = fopen(path.c_str(), "r");
    struct stat fileInfo;
    fstat(fileno(fp), &fileInfo);
    if (!S_ISREG(fileInfo.st_mode)) {
        fclose(fp);
        return false;
    } else {
        fclose(fp);
        return true;
    }
}

inline bool isDir(const std::string& path) {
    FILE *fp = fopen(path.c_str(), "r");
    struct stat fileInfo;
    fstat(fileno(fp), &fileInfo);
    if (!S_ISDIR(fileInfo.st_mode)) {
        fclose(fp);
        return false;
    } else {
        fclose(fp);
        return true;
    }
}

bool copyFile(const std::string& src, const std::string& dest) {
    LOG(INFO) << "Copy file " << src << " to " << dest;
    if (isFile(src)) {
        android::base::unique_fd fromFd(open(src.c_str(), O_RDONLY));
        if (fromFd == -1) {
            LOG(ERROR) << "Failed to open copy from " << src;
            return false;
        }
        android::base::unique_fd toFd(open(dest.c_str(), O_CREAT | O_WRONLY, 0774));
        if (toFd == -1) {
            LOG(ERROR) << "Failed to open copy to " << dest;
            return false;
        }

        struct stat sstat = {};
        if (stat(src.c_str(), &sstat) == -1) {
            LOG(ERROR) << "Failed to sstat file " << src;
            return false;
        }

        off_t length = sstat.st_size;
        int ret = 0;

        off_t offset = 0;
        while (offset < length) {
            ssize_t transfer_length = std::min(length - offset, (off_t) BUFF_SIZE);
            ret = sendfile(toFd, fromFd, &offset, transfer_length);
            if (ret != transfer_length) {
                ret = -1;
                LOG(ERROR) << "Copying failed from:" << src << " to " << dest;
                break;
            }
        }
        chown(dest.c_str(), AID_SYSTEM, AID_SYSTEM);
        chmod(dest.c_str(), 0771);
        // apps.block file lose
        if (fsync(toFd)) {
            LOG(ERROR) << "fsync file failed! " << strerror(errno);
        }
        return true;
    } else {
        LOG(ERROR) << "File (" << src << ") not a file!";
    }
    return false;
}

bool copyFolder(std::string src, std::string dest) {
    DIR *dir = nullptr;
    struct dirent *entry;
    
    if ((dir = opendir(src.c_str())) == nullptr) {
        return false;
    }
    
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        std::string inentry = src + "/" + entry->d_name;
        std::string indest = dest + "/" + entry->d_name;
        if (isDir(inentry)) {
            mkdir(indest.c_str(), 0775);
            chown(indest.c_str(), AID_SYSTEM, AID_SYSTEM);
            chmod(indest.c_str(), 0771);
            copyFolder(inentry, indest);
        } else {
            copyFile(inentry, indest);
        }
    }
    
    closedir(dir);
    sync();
    
    return true;
}

int bytes2int(char b[]) {
    int mask = 0xff;
    int temp = 0;
    int res = 0;
    for(int i = 0;i<4;i++) {
        res <<= 8;
        temp = b[i] & mask;
        res |= temp;
    }
    return res;
}

void dataCopy(char* dest, char* &source, const uint64_t &length, uint64_t &leftSize) {
    memcpy(dest, source, length);
    source += length;
    leftSize -= length;
}

bool unSplicing(char* sourceBlock, std::string destPath, std::string removePrefix, uint64_t size) {
    bool result = false;
    AppsBlock appsBlock;
    char numBuffer[SPLICE_RECORD_SIZE];
    char *splicedBuffer = (char*)malloc(sizeof(char) * BUFF_SIZE);

    if (!splicedBuffer) {
        LOG(ERROR) << "unSplicing malloc memory failed!";
        return false;
    }

    while (size > 0) {
        memset(&appsBlock, 0, sizeof(appsBlock));
        if (size < SPLICE_RECORD_SIZE) {
            LOG(ERROR) << "failed to unsplicing: read file name size failed(need " << SPLICE_RECORD_SIZE << "): " << size;
            goto splicingEnd;
        }

        // 1. Get filename size
        dataCopy(numBuffer, sourceBlock, SPLICE_RECORD_SIZE, size);
        appsBlock.fileNameSize = bytes2int(numBuffer);
        if (size < appsBlock.fileNameSize) {
            LOG(ERROR) << "failed to unsplicing: read file name failed(need " << appsBlock.fileNameSize << "): " << size;
            goto splicingEnd;
        }

        // 2. Get filename
        char fileNameBytes[appsBlock.fileNameSize + 1];
        memset(fileNameBytes, 0, appsBlock.fileNameSize + 1);
        dataCopy(fileNameBytes, sourceBlock, appsBlock.fileNameSize, size);
        appsBlock.fileName = fileNameBytes;
        if (appsBlock.fileName.find(removePrefix) == 0) {
            appsBlock.fileName = destPath + "/" + appsBlock.fileName.substr(removePrefix.length());
        }
        if (ENABLE_DEBUG) LOG(INFO) << "File name: " << appsBlock.fileName;
        if (mkdir_recursively(appsBlock.fileName.substr(0, appsBlock.fileName.rfind('/')), 0755, false, nullptr) != 0) {
            LOG(ERROR) << "failed to mkdir " << appsBlock.fileName << " (" << strerror(errno) << ")";
            goto splicingEnd;
        }

        // 3. Get file content size
        if (size < SPLICE_RECORD_SIZE) {
            LOG(ERROR) << "failed to unsplicing: read file content size failed(need " << SPLICE_RECORD_SIZE << "): " << size;
            goto splicingEnd;
        }

        dataCopy(numBuffer, sourceBlock, SPLICE_RECORD_SIZE, size);
        uint64_t fileSize = bytes2int(numBuffer);
        if (size < fileSize) {
            LOG(ERROR) << "failed to unsplicing: read file content failed(need " << fileSize << "): " << size;
            goto splicingEnd;
        }

        if (ENABLE_DEBUG) LOG(INFO) << "file size: " << fileSize;

        // 4. Get and write file content to file
        std::fstream destFile(appsBlock.fileName, std::ios::out | std::ios::binary);
        if (!destFile) {
            LOG(ERROR) << "open copy dest file error: " << destPath.c_str() << " (" << strerror(errno) << ")";
            goto splicingEnd;
        }

        LOG(INFO) << "unSplice Info: File name: " << appsBlock.fileName << " File size: " << fileSize;
        while (fileSize > 0) {
            uint64_t nextReadCount = std::min(BUFF_SIZE, fileSize);
            dataCopy(splicedBuffer, sourceBlock, nextReadCount, size);
            destFile.write(splicedBuffer, nextReadCount);
            fileSize -= nextReadCount;
        }
        destFile.close();
    }
    LOG(INFO) << "UnSplicing of reserved apps completed. ";
    result = true;
splicingEnd:
    free(splicedBuffer);
    return result;
}

// END
