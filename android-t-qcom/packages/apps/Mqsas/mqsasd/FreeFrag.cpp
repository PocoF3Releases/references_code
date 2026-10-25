#define LOG_TAG "freefrag"

#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <cutils/fs.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <cutils/properties.h>
#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>
#include "utils.h"
#include "FreeFrag.h"
#include <hardware_legacy/power.h>
#include "FragInfo.h"

#define DEFAULT_COMPACT_PATH           "/data"
#define DEFRAG_MODE                    "persist.sys.mem_dmode"
#define COMPACT_POLICY                 "persist.sys.mem_cpolicy"
#define COMPACT_GROUP_MAX              "persist.sys.mem_cmax_groups"
#define COMPACT_GATED                  "persist.sys.mem_cgated"
#define FILE_DEFRAG_GATED              "persist.sys.mem_fgated"

#define EXT4_STORAGE_COMPACT           0x0001
#define EXT4_FILE_DEFRAG               0x0002

#define MAX_SEARCH_GROUP_COUNT         16

#define MAX_HIST                       8

/*
 * In the current implementation, there are two policies used to find group to defrag.
 * GREEDY is based on greedy algorithm and will choose block group which has more free
 * blocks and also more fragments to defrag.
 * LINEAR will search block group linearly from start to end.
 */
enum {
    GREEDY = 0,
    LINEAR,
    MAX_POLICY,
};

static const char* kcompactBinPath = "/system/xbin/msc";
static const char* kdefragBinPath = "/system/xbin/mfd";
static const char* log_root_dir_path = "/data/mqsas/MDG";

static const char* kWakeLock = "FreeFrag";

static std::string log_dir_path;

using namespace android;

std::mutex android::FreeFrag::pidMutex;
pid_t android::FreeFrag::compactPid = -1;
pid_t android::FreeFrag::defragPid = -1;
pid_t android::FreeFrag::stopFlag = 0;


void FreeFrag::getStats(String8& fragInfo) {
    fragInfo.setTo(getFragInfo().c_str());
}

void FreeFrag::stopDefrag(){
    ALOGD("FreeFrag::stopDefrag enter");
    pidMutex.lock();

    stopFlag = 1;
    if (compactPid != -1) {
        ALOGD("begin to send stop signal to MSC process");
        kill(compactPid, SIGUSR1);
    } else {
        ALOGD("MSC process is not running, no need to kill it");
    }

    if (defragPid != -1) {
        ALOGD("begin to send stop signal to MFD process");
        kill(defragPid, SIGUSR1);
    } else {
        ALOGD("MFD process is not running, no need to kill it");
    }

    pidMutex.unlock();
}

void FreeFrag::recoverStopFlag(){
    pidMutex.lock();
    stopFlag = 0;
    pidMutex.unlock();
}

void StringAppendV(std::string* dst, const char* format, va_list ap) {
  // First try with a small fixed size buffer
  char space[1024];

  // It's possible for methods that use a va_list to invalidate
  // the data in it upon use.  The fix is to make a copy
  // of the structure before using it and use that copy instead.
  va_list backup_ap;
  va_copy(backup_ap, ap);
  int result = vsnprintf(space, sizeof(space), format, backup_ap);
  va_end(backup_ap);

  if (result < static_cast<int>(sizeof(space))) {
    if (result >= 0) {
      // Normal case -- everything fit.
      dst->append(space, result);
      return;
    }

    if (result < 0) {
      // Just an error.
      return;
    }
  }

  // Increase the buffer size to the size requested by vsnprintf,
  // plus one for the closing \0.
  int length = result + 1;
  char* buf = new char[length];

  // Restore the va_list before we use it again
  va_copy(backup_ap, ap);
  result = vsnprintf(buf, length, format, backup_ap);
  va_end(backup_ap);

  if (result >= 0 && result < length) {
    // It fit
    dst->append(buf, result);
  }
  delete[] buf;
}

std::string StringPrintf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  std::string result;
  StringAppendV(&result, fmt, ap);
  va_end(ap);
  return result;
}

pid_t ForkExecvpAsync(const std::vector<std::string>& args) {
    size_t argc = args.size();
    char** argv = (char**) calloc(argc + 1, sizeof(char*));
    for (size_t i = 0; i < argc; i++) {
        argv[i] = (char*) args[i].c_str();
        if (i == 0) {
            ALOGD("%s", argv[i]);
        } else {
            ALOGD("    %s", argv[i]);
        }
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        if (execvp(argv[0], argv)) {
            ALOGE("Failed to exec");
        }

        _exit(1);
    }

    if (pid == -1) {
        ALOGE("Failed to exec");
    }

    free(argv);
    return pid;
}

int FreeFrag::RunStorageCompact() {
    char compact_policy[PROPERTY_VALUE_MAX] = {0};
    char default_max_groups[PROPERTY_VALUE_MAX] = {0};
    char max_groups[PROPERTY_VALUE_MAX] = {0};
    int policy;

    //support stopDefrag
    pidMutex.lock();
    if (stopFlag == 1) {
        ALOGD("user cancel MDG process");
        pidMutex.unlock();
        return 0;
    }
    pidMutex.unlock();

    if (access(kcompactBinPath, X_OK)) {
        ALOGD("Not running %s (executable not in system image)", kcompactBinPath);
        return 0;
    }

    ALOGD("Begin to run %s", kcompactBinPath);

    std::vector<std::string> cmd;
    cmd.push_back(kcompactBinPath);
    // Path to log root dir
    cmd.push_back("-o");
    cmd.push_back(log_root_dir_path);
    // Override the generated log subdir name
    cmd.push_back("-O");
    cmd.push_back(StringPrintf("%s", log_dir_path.c_str()));
    // Log to Android logging system instead of stderr
    cmd.push_back("-A");

    // Get storage compact policy
    property_get(COMPACT_POLICY, compact_policy, "greedy");
    if (!strncmp(compact_policy, "greedy", strlen("greedy"))) {
        policy = GREEDY;
    } else if (!strncmp(compact_policy, "linear", strlen("linear"))) {
        policy = LINEAR;
    } else {
        ALOGD("unknown policy specified, greedy policy will be used");
        policy = GREEDY;
    }
    // Set storage compact policy
    cmd.push_back("-p");
    cmd.push_back(StringPrintf("%d", policy));

    // Get maximum block groups compacted each time
    sprintf(default_max_groups, "%u", MAX_SEARCH_GROUP_COUNT);
    property_get(COMPACT_GROUP_MAX, max_groups, default_max_groups);

    // Set block groups needed to be defragged
    cmd.push_back("-m");
    cmd.push_back(StringPrintf("%s", max_groups));
    // Set storage compact path
    cmd.push_back(DEFAULT_COMPACT_PATH);

    pidMutex.lock();
    compactPid = ForkExecvpAsync(cmd);
    pidMutex.unlock();

    if (compactPid == -1) {
        ALOGE("failed to fork msc process: %s", strerror(errno));
        return -1;
    }

    int status;
    while (true) {
        if (waitpid(compactPid, &status, WNOHANG) == compactPid) {
            if (WIFEXITED(status)) {
                ALOGE("msc process has been finished with status %d", WEXITSTATUS(status));
                pidMutex.lock();
                compactPid = -1;
                pidMutex.unlock();
                return (WEXITSTATUS(status) == 0) ? 0 : -1;
            } else {
                break;
            }
        }

        sleep(5);
    }

    pidMutex.lock();
    compactPid = -1;
    pidMutex.unlock();
    return -1;
}

int FreeFrag::RunFileDefrag() {
    //support stopDefrag
    pidMutex.lock();
    if (stopFlag == 1) {
        ALOGD("user cancel MFD process");
        pidMutex.unlock();
        return 0;
    }
    pidMutex.unlock();

    if (access(kdefragBinPath, X_OK)) {
        ALOGD("Not running %s (executable not in system image)", kdefragBinPath);
        return 0;
    }

    ALOGD("Begin to run %s", kdefragBinPath);

    std::vector<std::string> cmd;
    cmd.push_back(kdefragBinPath);
    // Path to log root dir
    cmd.push_back("-o");
    cmd.push_back(log_root_dir_path);
    // Override the generated log subdir name
    cmd.push_back("-O");
    cmd.push_back(StringPrintf("%s", log_dir_path.c_str()));
    // Log to Android logging system instead of stderr
    cmd.push_back("-A");

    pidMutex.lock();
    defragPid = ForkExecvpAsync(cmd);
    pidMutex.unlock();

    if (defragPid == -1) {
        ALOGE("failed to fork mfd process: %s", strerror(errno));
        return -1;
    }

    int status;
    while (true) {
        if (waitpid(defragPid, &status, WNOHANG) == defragPid) {
            if (WIFEXITED(status)) {
                ALOGD("mfd process has been finished with status %d", WEXITSTATUS(status));
                pidMutex.lock();
                defragPid = -1;
                pidMutex.unlock();
                return (WEXITSTATUS(status) == 0) ? 0 : -1;
            } else {
                break;
            }
        }

        sleep(5);
    }

    pidMutex.lock();
    defragPid = -1;
    pidMutex.unlock();
    return -1;
}

void FreeFrag::defragDataPartition() {
    String8 orig_frag, after_compact_frag;
    String8 after_filedefrag_frag;
    int32_t defrag_mode_sel;

    acquire_wake_lock(PARTIAL_WAKE_LOCK, kWakeLock);

    recoverStopFlag();

    defrag_mode_sel = property_get_int32(DEFRAG_MODE, EXT4_STORAGE_COMPACT | EXT4_FILE_DEFRAG);

    std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
    time_t t_c = std::chrono::system_clock::to_time_t(t);
    std::stringstream ss;
    ss << std::put_time(localtime(&t_c), "%y%m%d-%H%M%S");
    log_dir_path = ss.str();

    FreeFrag::getStats(orig_frag);
    ALOGD("frag before MSC: %s", orig_frag.string());

    //do storagecompact work
    if (defrag_mode_sel & EXT4_STORAGE_COMPACT) {
        bool msc_gated;

        msc_gated = property_get_bool(COMPACT_GATED, true);
        if (msc_gated) {
            ALOGD("msc is disabled (cc)");
        } else {
            RunStorageCompact();
            FreeFrag::getStats(after_compact_frag);
            ALOGD("frag after MSC: %s", after_compact_frag.string());
        }
    }

    //do filedefrag work
    if (defrag_mode_sel & EXT4_FILE_DEFRAG) {
        bool mfd_gated;

        mfd_gated = property_get_bool(FILE_DEFRAG_GATED, true);
        if (mfd_gated) {
            ALOGD("mfd is disabled (cc)");
        } else {
            RunFileDefrag();
            FreeFrag::getStats(after_filedefrag_frag);
            ALOGD("frag after MFD: %s", after_filedefrag_frag.string());
        }
    }

    recoverStopFlag();
    release_wake_lock(kWakeLock);
}
