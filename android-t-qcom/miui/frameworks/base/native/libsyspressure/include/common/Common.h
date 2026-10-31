#pragma once
#include <dlfcn.h>
#include <dirent.h>
#include <pwd.h>
#include <sched.h>
#include <stdbool.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <time.h>
#include <system/thread_defs.h>

#include <utils/Errors.h>
#include <utils/Log.h>
#include <errno.h>
#include <string.h>
#include <log/log.h>
#include <log/log_time.h>
#include <utils/Mutex.h>
#include <utils/Errors.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/pidfd.h>
#include <inttypes.h>
#include <stdio.h>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <queue>
#include <vector>
#include <unistd.h>
#include <atomic>
#include <cutils/properties.h>
#include <string>
#include <iostream>
#include <signal.h>
#include <sys/syscall.h>
#include <regex>
#include <sys/cdefs.h>
#include <utils/Trace.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <utils/RefBase.h>
#include "android-base/stringprintf.h"

#include "common/Utils.h"

namespace android {

namespace syspressure {

#define THREAD_POOL_COUNT           6

// 1000 ms. 100 precent.
// sysconf(_SC_NPROCESSORS_CONF)
#define JIFFY_HZ                    (double)sysconf(_SC_CLK_TCK)
#define JIFFY_MS                    (1000/JIFFY_HZ)

#define MAX_READ_LINE               128
#define MAX_PATH_LENGTH             128

// cpulimit
static const int32_t kCpuExceptionThreshold = property_get_int32(
                                    "persist.sys.spc.cpuexception.threshold", 80);

// pressure
static const bool KEnablePressure = property_get_bool(
                                    "persist.sys.spc.pressure.enable", true);

static const bool DEBUG = property_get_bool("debug.sys.spc", false);
// error code status
/**
 * The type used to return success/failure from frameworks APIs.
 * See the anonymous enum below for valid values.
 */
typedef int32_t status_t;

/*
 * Error codes.
 * All error codes are negative values.
 */
enum {
    STATE_OK            = 0,    // Preferred constant for checking success.

    STATE_UNKNOWN_ERROR       = (-2147483647-1), // INT32_MIN value

    STATE_NO_MEMORY           = -ENOMEM,//-12
    STATE_INVALID_OPERATION   = -ENOSYS,//-38
    STATE_BAD_VALUE           = -EINVAL,//-22
    STATE_BAD_TYPE            = (STATE_UNKNOWN_ERROR + 1),
    STATE_NOT_FOUND      = -ENOENT,//-2
    STATE_PERMISSION_DENIED   = -EPERM,//-1
    STATE_NO_INIT             = -ENODEV,//-19
    STATE_ALREADY_EXISTS      = -EEXIST,//-17
    STATE_DEAD_OBJECT         = -EPIPE,//-32
    STATE_BAD_INDEX           = -EOVERFLOW,//-75
    STATE_NOT_ENOUGH_DATA     = -ENODATA,//-61
    WOULDTIMED_OUT           = -ETIMEDOUT,//-110
};
} // namespace syspressure

} // namespace android