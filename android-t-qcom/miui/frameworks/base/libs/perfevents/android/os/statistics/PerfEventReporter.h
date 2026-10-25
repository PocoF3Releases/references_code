#ifndef ANDROID_OS_STATISTICS_PERFEVENTREPORTER_H
#define ANDROID_OS_STATISTICS_PERFEVENTREPORTER_H

#include <time.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <stdatomic.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>
#include <sched.h>
#include <cutils/sched_policy.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <cutils/compiler.h>
#include <cutils/atomic.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "OsUtils.h"
#include "NativePerfEventParcel.h"
#include "PerfEventBuffer.h"
#include "PerfEvent.h"

namespace android {
namespace os {
namespace statistics {

struct ThreadPriorityInfo {
  int32_t sched_policy;
  int32_t sched_priority;
  bool is_low_priority;
  bool is_boosted;
  int32_t boosted_sched_priority;
};

class PerfEventReporter {
public:
  static void init(JNIEnv* env);
  static void start(JNIEnv* env);
  static bool beginReportPerfEvent(JNIEnv* env, struct ThreadPriorityInfo &threadPriorityInfo);
  static void endReportPerfEvent(JNIEnv* env, const struct ThreadPriorityInfo &threadPriorityInfo);
  static void reportPerfEvent(JNIEnv* env, int32_t eventType, jobject perfEvent, const struct ThreadPriorityInfo &threadPriorityInfo);
  static void reportPerfEvent(JNIEnv* env, PerfEvent& perfEvent, const struct ThreadPriorityInfo &threadPriorityInfo);
  static void waitForPerfEventArrived(JNIEnv* env, int32_t timeoutMillis);
  static uint32_t fetchProcUserspacePerfEvents(JNIEnv* env, jobjectArray fetchingBuffer);
  static uint32_t fetchProcKernelPerfEvents(JNIEnv* env, jobjectArray fetchingBuffer);
private:
  inline static void notifyNewDataArrived() {
    bool expected = false;
    if (atomic_compare_exchange_strong_explicit(gNewDataArrived, &expected, true,
            memory_order_relaxed, memory_order_relaxed)) {
      uint64_t value = 1;
      write(gWakeEventFd, &value, sizeof(uint64_t));
    }
  }
private:
  static volatile int32_t gWakeEventFd;
  static volatile atomic_bool* gNewDataArrived;
  static atomic_int_fast64_t gEventSeq;
};


} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_PERFEVENTREPORTER_H
