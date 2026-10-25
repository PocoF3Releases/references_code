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

#ifndef _MI_UTILS_H_
#define _MI_UTILS_H_

#define CPU_INFO_FILE "/proc/cpuinfo"
#define CPU_HPS_FILE "/proc/hps/enabled"
#define CPU_POSSIBLE_FILE "/sys/devices/system/cpu/possible"
#define CPU_GOVERNOR_FILE "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_governor"
#define CPU_ONLINE_FILE "/sys/devices/system/cpu/cpu%d/online"
#define CPU_SETSPEED_FILE "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_setspeed"

#define ENABLE "1"
#define DISABLE "0"
#define ENABLE_DEBUG false

enum {DEFAULT, POWERSAVE, CONFIGURATION, FULLLOAD};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

const static uint64_t SPLICE_RECORD_SIZE = 4;
const static uint64_t BUFF_SIZE = 4 * 1024 * 1024;

typedef struct {
  uint32_t fileNameSize;
  std::string fileName;
  uint32_t fileContextSize;
  void* fileContext;
} AppsBlock;

typedef struct
{
    const char* model;                        /* cpu model */
    int l_start;                        /* little core numbers */
    int l_end;                          /* little core numbers */
    int m_start;                        /* middle core numbers */
    int m_end;                          /* middle core numbers */
    int b_start;                        /* big core numbers */
    int b_end;                          /* big core numbers */
    int scn_little_cores;               /* numbers of online little core (0 is offline) */
    int scn_middle_cores;               /* numbers of online middle core (0 is offline) */
    int scn_big_cores;                  /* numbers of online big core (0 is offline) */
    const char* scn_little_governor;    /* governor of core */
    int scn_little_speed;               /* need by userspace */
    const char* scn_middle_governor;    /* governor of core */
    int scn_middle_speed;               /* need by userspace */
    const char* scn_big_governor;       /* governor of core */
    int scn_big_speed;                  /* need by userspace */
}cpuinfo;

void set_cpu_governor(int governor);
void record_time(const char* pathname, float time, const char* time_type);
bool need_erase_userdata(void);
bool wait_for_file(const std::string& path, std::optional<std::chrono::milliseconds> timeout);
void record_package_name(void);
uint64_t gettime_ns();

int delete_minidump();
int copy_demsg();
int copy_demsg_with_minidump(const char *);
int copy_demsg_without_minidump();
void rotate_logs_for_lastdemsg(const char *last_demsg, int keep_log_count);

bool disable_format(void);
void set_disable_factoryreset();
bool isFile(const std::string& path);
bool isDir(const std::string& path);
bool copyFile(const std::string& src, const std::string& dest);
bool copyFolder(std::string src, std::string dest);
int bytes2int(char b[]);
bool unSplicing(char* sourceBlock, std::string destPath, std::string removePrefix, uint64_t size);

class Timer {
 public:
  Timer() : t0(gettime_ns()) {
  }

  double duration() {
    return static_cast<double>(gettime_ns() - t0) / 1000000000.0;
  }

 private:
  uint64_t t0;
};

#endif /* _MI_UTILS_H_ */
