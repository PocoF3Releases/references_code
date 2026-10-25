/*
 * Copyright (C) Xiaomi Inc.
 *
 */
#include "VoldImpl.h"


#include <android-base/logging.h>
#include <hardware_legacy/power.h>

#include <signal.h>
#include <errno.h>
#include <string>

#include <spawn.h>
#include <sys/wait.h>
#include <sched.h>
#include <sys/syscall.h>
#include <Utils.h>

using std::to_string;

namespace android {
namespace vold {

enum class ExtendMStats {
    kStopped = 1,
    kRunning,
    kAbort,
};

static const char* kWakeLock = "ExtendM";
static const char* flushPath = "/sys/block/zram0/writeback";
static ExtendMStats flush_stat(ExtendMStats::kStopped);

static pid_t flushPid = -1;
static std::mutex pidMutex;
static std::mutex flush_mutex;
static std::string flushdata= "idle";
void thread_handler(int sig) {
    // do nothing
}

#define AFFINITY_POLICY_BG 0
#define AFFINITY_BG_LENGTH 4
static int bg_cpus[AFFINITY_BG_LENGTH] = {0, 1, 2, 3};
typedef struct {uint64_t bits[1024 / 64]; } cpu_set_t;
static int set_affinity_policy(int policy) {
    int retval = 0;
    cpu_set_t cpuset;
    memset(&cpuset, 0, sizeof(cpuset));
    if (policy == AFFINITY_POLICY_BG) {
        for (int i = 0; i < AFFINITY_BG_LENGTH; i++) {
            cpuset.bits[bg_cpus[i] / 64] |= 1ULL << (bg_cpus[i] % 64);
        }
        retval = syscall(__NR_sched_setaffinity, gettid(), sizeof(cpuset), &cpuset);
    }
    return retval;
}

int VoldImpl::runExtMFlush(int pageCount, int flushLevel, const android::sp<android::os::IVoldTaskListener>& listener) {

    std::unique_lock<std::mutex> lk(flush_mutex);
    if (flush_stat != ExtendMStats::kStopped) {
        LOG(DEBUG) << "flush task is already running";
        if (listener) {
            android::os::PersistableBundle extras;
            listener->onFinished(0, extras);
        }
        return android::OK;
    }
    flush_stat = ExtendMStats::kRunning;
    lk.unlock();

    LOG(DEBUG) << "flush task started";

    acquire_wake_lock(PARTIAL_WAKE_LOCK, kWakeLock);

    pidMutex.lock();
    pid_t pid = fork();
    pidMutex.unlock();

    if (pid < 0) {
        LOG(DEBUG) << " fork error ";
        if (listener) {
            android::os::PersistableBundle extras;
            listener->onFinished(-1, extras);
        }
        return -1;
    } else if (pid == 0) {
        LOG(DEBUG) << " flush Pid = " << getpid();
        if (!set_affinity_policy(AFFINITY_POLICY_BG)) {
            LOG(DEBUG) << "set cpu affinity policy success";
        } else {
            LOG(DEBUG) << "set cpu affinity policy failed";
        }
        //Resister a  SIGUSR1 signal handle function
        struct sigaction actions;
        memset(&actions, 0, sizeof(actions));
        sigemptyset(&actions.sa_mask);
        actions.sa_flags = 0;
        actions.sa_handler = thread_handler;
        sigaction(SIGUSR1, &actions, NULL);

        int fd = open(flushPath, O_WRONLY | O_CLOEXEC);
        if (fd < 0) {
            LOG(DEBUG) << " open /sys/block/zram0/writeback error:" << fd;
            _exit(-1);
        }
        flushdata = flushdata + " " + std::to_string(pageCount) + " " + std::to_string(flushLevel);
        const char* p = flushdata.data();
        size_t left = flushdata.size();
        // same as: echo idle 262144 5 > /sys/block/zram0/writeback
        ssize_t n = write(fd, p, left);
        if (n == (int)left) {
            close(fd);
            LOG(DEBUG) << " success write idle > /sys/block/zram0/writeback";
            _exit(0);
        } else {
            if (n < 0){
                LOG(DEBUG) << " write stop, errno: " << errno;
            }
            close(fd);
            _exit(-1);
        }
    } else {
        flushPid = pid;
    }

    int status;
    while (true) {
        if (waitpid(flushPid, &status, WNOHANG) == flushPid) {
            if (WIFEXITED(status)) {
                LOG(DEBUG) << "flush process has been finished with status " << WEXITSTATUS(status);
                break;
            }
        }
        usleep(500*1000);
    }

    if (listener) {
        android::os::PersistableBundle extras;
        listener->onFinished(0, extras);
        LOG(DEBUG) << " listener->onFinished ";
    }

    pidMutex.lock();
    flushPid = -1;
    pidMutex.unlock();

    lk.lock();
    flush_stat = ExtendMStats::kStopped;
    lk.unlock();

    release_wake_lock(kWakeLock);
    return android::OK;
}

int VoldImpl::stopExtMFlush(const android::sp<android::os::IVoldTaskListener>& listener) {
    acquire_wake_lock(PARTIAL_WAKE_LOCK, kWakeLock);

    LOG(DEBUG) << "stopFlush enter";

    std::unique_lock<std::mutex> lk(flush_mutex);
    if (flush_stat != ExtendMStats::kStopped) {
        flush_stat = ExtendMStats::kAbort;
    }
    lk.unlock();

    pidMutex.lock();
    if (flushPid != -1) {
        LOG(DEBUG) << "begin to send stop signal to flush process";
        LOG(DEBUG) << " flush Pid = " << flushPid;
        kill(flushPid, SIGUSR1);
        if (listener) {
            android::os::PersistableBundle extras;
            listener->onFinished(0, extras);
            LOG(DEBUG) << "stopFlush end";
        }
    } else {
        LOG(DEBUG) << "flush process is not running";
        if (listener) {
            android::os::PersistableBundle extras;
            listener->onFinished(-1, extras);
        }
    }
    pidMutex.unlock();

    release_wake_lock(kWakeLock);

    return android::OK;
}

} // namespace vold
} // namespace android
