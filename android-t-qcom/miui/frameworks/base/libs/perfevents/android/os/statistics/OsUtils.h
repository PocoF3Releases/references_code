#ifndef ANDROID_OS_STATISTICS_OSUTILS_H
#define ANDROID_OS_STATISTICS_OSUTILS_H

#include <inttypes.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <utils/ThreadDefs.h>
#include <cutils/sched_policy.h>

namespace android {
namespace os {
namespace statistics {

#define SCHED_POLICY_UNKNOWN -1
#define SCHED_RESET_ON_FORK 0x40000000

class OsUtils {
public:
  inline static int64_t currentTimeMillis() {
    struct timeval now;
    gettimeofday(&now, nullptr);
    return (int64_t)now.tv_sec * 1000 + now.tv_usec / 1000;
  }
  inline static int64_t MilliTime() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<int64_t>(now.tv_sec) * INT64_C(1000) + now.tv_nsec / INT64_C(1000000);
  }
  inline static int64_t NanoTime() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<int64_t>(now.tv_sec) * INT64_C(1000000000) + now.tv_nsec;
  }
  inline static void millisleep(int sleepMillis) {
    struct timespec sleepTime;
    sleepTime.tv_sec = sleepMillis / 1000;
    sleepTime.tv_nsec = (sleepMillis % 1000) * 1000000;
    nanosleep(&sleepTime, nullptr);
  }
  inline static int64_t getRunningTimeMs() {
    struct rusage resourceUsage;
    if (getrusage(RUSAGE_THREAD, &resourceUsage) == 0) {
      return timevalToMs(resourceUsage.ru_utime) + timevalToMs(resourceUsage.ru_stime);
    } else {
      return 0;
    }
  }
  inline static int32_t decodeThreadSchedulePolicy(int32_t scheduler) {
    return scheduler < 0 ? scheduler : (scheduler & (~SCHED_RESET_ON_FORK));
  }
  inline static bool isLowPriority(int32_t schedPolicy, int32_t schedPriority) {
    if (schedPolicy == SCHED_RR || schedPolicy == SCHED_FIFO) {
      return false;
    } else if (schedPolicy == SCHED_OTHER) {
      return schedPriority >= ANDROID_PRIORITY_BACKGROUND;
    } else {
      return true;
    }
  }
  inline static int32_t getSchedGroup(int32_t tid) {
    int32_t schedGroup;
    SchedPolicy sp;
    if (get_sched_policy(tid, &sp) == 0) {
      schedGroup = (int32_t)sp;
    } else {
      schedGroup = (int32_t)SP_DEFAULT;
    }
    return schedGroup;
  }
  static void getThreadSchedStat(int32_t tid, int64_t& runningTimeMs, int64_t& runnableTimeMs);
  static void translateKernelAddress(char* buf, int32_t bufSize, uint64_t val);
  inline static int64_t timevalToMs(struct timeval val) {
    return val.tv_sec * 1000 + val.tv_usec / 1000;
  }
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_OSUTILS_H
