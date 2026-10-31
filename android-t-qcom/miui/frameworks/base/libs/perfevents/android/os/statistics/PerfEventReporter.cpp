#define LOG_TAG "PerfEventReporter"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <thread>
#include <cutils/sched_policy.h>

#include "PerfSuperviser.h"
#include "PerfEventReporter.h"
#include "JavaVMUtils.h"
#include "ThreadUtils.h"
#include "NativePerfEventParcelCache.h"
#include "JavaBackTraceCache.h"
#include "FastSystemInfoFetcher.h"
#include "KernelPerfEventsReader.h"

namespace android {
namespace os {
namespace statistics {

#define LOWEST_PERF_EVENTS_REPORT_PRIORITY 10

static const char* const kPerfEventPathName = "android/os/statistics/PerfEvent";
static const char* const kObjectPathName = "java/lang/Object";

static jclass gPerfEventClass = nullptr;
static jclass gObjectClass = nullptr;

volatile int32_t PerfEventReporter::gWakeEventFd = -1;
volatile atomic_bool* PerfEventReporter::gNewDataArrived = nullptr;
atomic_int_fast64_t PerfEventReporter::gEventSeq;

static KernelPerfEventsReader *gProcKernelPerfEventsReader = NULL;

void PerfEventReporter::init(JNIEnv* env) {
  jclass clazz = FindClassOrDie(env, kPerfEventPathName);
  gPerfEventClass = MakeGlobalRefOrDie(env, clazz);

  clazz = FindClassOrDie(env, kObjectPathName);
  gObjectClass = MakeGlobalRefOrDie(env, clazz);

  gNewDataArrived = new atomic_bool[1];
  atomic_init(gNewDataArrived, false);

  //用户空间事件的seq从int64取值范围的中间开始，内核事件的seq从0开始，
  //这样，对于同一线程相同时间范围内的事件，可以保证内核事件总是被包含在用户空间事件中。
  atomic_init(&gEventSeq, (int64_t)10000000000llu);
}

void PerfEventReporter::start(JNIEnv* env) {
  gProcKernelPerfEventsReader = KernelPerfEventsReader::openProcKernelPerfEventsReader((int)sysconf(_SC_PAGE_SIZE));
}

bool PerfEventReporter::beginReportPerfEvent(JNIEnv* env, struct ThreadPriorityInfo &threadPriorityInfo) {
  if (CC_UNLIKELY(!ThreadUtils::isThreadPerfSupervisionOn(env) || gWakeEventFd == -1)) {
    return false;
  }
  ThreadUtils::setThreadPerfSupervisionOn(env, false);
  int32_t schedPolicy = OsUtils::decodeThreadSchedulePolicy(sched_getscheduler(0));
  int32_t schedPriority = getpriority(PRIO_PROCESS, 0);
  threadPriorityInfo.sched_policy = schedPolicy;
  threadPriorityInfo.sched_priority = schedPriority;
  threadPriorityInfo.is_low_priority = false;
  threadPriorityInfo.is_boosted = false;
  threadPriorityInfo.boosted_sched_priority = threadPriorityInfo.sched_priority;
  if ((schedPolicy == SCHED_OTHER || schedPolicy == SCHED_BATCH || schedPolicy == SCHED_IDLE) &&
      (schedPriority > LOWEST_PERF_EVENTS_REPORT_PRIORITY)) {
    threadPriorityInfo.is_low_priority = true;
    threadPriorityInfo.is_boosted = true;
    threadPriorityInfo.boosted_sched_priority = LOWEST_PERF_EVENTS_REPORT_PRIORITY;
    setpriority(PRIO_PROCESS, 0, LOWEST_PERF_EVENTS_REPORT_PRIORITY);
  }
  return true;
}

void PerfEventReporter::endReportPerfEvent(JNIEnv* env, const struct ThreadPriorityInfo &threadPriorityInfo) {
  if (threadPriorityInfo.is_boosted) {
    setpriority(PRIO_PROCESS, 0, threadPriorityInfo.sched_priority);
  }
  ThreadUtils::setThreadPerfSupervisionOn(env, true);
}

void PerfEventReporter::reportPerfEvent(JNIEnv* env, int32_t eventType, jobject perfEvent,
  const struct ThreadPriorityInfo &threadPriorityInfo) {
  if (CC_UNLIKELY(gWakeEventFd == -1)) return;
  int64_t eventSeq = atomic_fetch_add_explicit(&gEventSeq, (int64_t)1, memory_order_relaxed);
  PerfEventBuffer::addPerfEvent(env, eventType, eventSeq, perfEvent);
  notifyNewDataArrived();
}

void PerfEventReporter::reportPerfEvent(JNIEnv* env, PerfEvent& perfEvent, const struct ThreadPriorityInfo &threadPriorityInfo) {
  if (CC_UNLIKELY(gWakeEventFd == -1)) {
    NativePerfEventParcelCache::recycle(env, perfEvent.mDetails.mGlobalDetails);
    if (env != nullptr) {
      JavaBackTraceCache::recycle(env, perfEvent.mDetails.mGlobalJavaBackTrace);
    }
    return;
  }
  perfEvent.mEventSeq = atomic_fetch_add_explicit(&gEventSeq, (int64_t)1, memory_order_relaxed);
  PerfEventBuffer::addPerfEvent(env, perfEvent);
  notifyNewDataArrived();
}

inline void millisleep(int sleepMillis) {
  struct timespec sleepTime;
  sleepTime.tv_sec = sleepMillis / 1000;
  sleepTime.tv_nsec = (sleepMillis % 1000) * 1000000;
  nanosleep(&sleepTime, nullptr);
}

void PerfEventReporter::waitForPerfEventArrived(JNIEnv* env, int32_t timeoutMillis) {
  int res = 0;
  bool hasError = false;
  bool timeOut = false;

  if (atomic_load_explicit(gNewDataArrived, memory_order_relaxed) ||
      (gProcKernelPerfEventsReader != NULL && !gProcKernelPerfEventsReader->isEmpty())) {
    goto FINISH;
  }

  if (gWakeEventFd == -1) {
    gWakeEventFd = eventfd(0, EFD_CLOEXEC);
  }
  if (gWakeEventFd == -1) {
    hasError = true;
    goto FINISH;
  }

  struct pollfd rfds[2];
  while(1){
    rfds[0].fd = gWakeEventFd;
    rfds[0].events = POLLIN | POLLRDNORM;
    rfds[0].revents = 0;
    if (gProcKernelPerfEventsReader != NULL) {
      rfds[1].fd = gProcKernelPerfEventsReader->getfd();
      rfds[1].events = POLLIN | POLLRDNORM;
      rfds[1].revents = 0;
    } else {
      rfds[1].fd = -1;
    }
    res = poll(rfds, 2, timeoutMillis);
    if (res < 0 && errno == EINTR) {
      continue;
    }
    break;
  }
  if (res == 0) {
    timeOut = true;
  } else if (res < 0) {
    hasError = true;
  } else if (res > 0) {
    uint64_t result;
    if ((rfds[0].revents & (POLLIN | POLLRDNORM)) != 0) {
      read(gWakeEventFd, &result, sizeof(uint64_t));
    }
  }

FINISH:
  if (hasError) {
    millisleep(1000);
  } else if (!timeOut) {
    if (PerfSuperviser::isSystemServer()) {
      usleep(std::min(PerfSuperviser::getInnerWaitThresholdMs() * 4, 200) * 1000);
    } else {
      usleep(std::min(PerfSuperviser::getInnerWaitThresholdMs() * 6, 300) * 1000);
    }
  }
}

uint32_t PerfEventReporter::fetchProcUserspacePerfEvents(JNIEnv* env, jobjectArray fetchingBuffer) {
  bool expected = true;
  atomic_compare_exchange_strong_explicit(gNewDataArrived, &expected, false,
    memory_order_relaxed, memory_order_relaxed);
  return PerfEventBuffer::fetch(env, fetchingBuffer);
}

uint32_t PerfEventReporter::fetchProcKernelPerfEvents(JNIEnv* env, jobjectArray fetchingBuffer) {
  uint32_t count = 0;
  if (gProcKernelPerfEventsReader != NULL) {
    count = gProcKernelPerfEventsReader->readPerfEvents(env, fetchingBuffer);
  }
  return count;
}

} //namespace statistics
} //namespace os
} //namespace android
