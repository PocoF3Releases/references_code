#define LOG_TAG "PerfEventReporter"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <cutils/atomic.h>
#include <binder/Parcel.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "OsUtils.h"
#include "PerfSuperviser.h"
#include "PerfEventReporter.h"
#include "ExecutingRootEvents.h"
#include "KernelPerfEventsReader.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

static void android_os_statistics_PerfEventReporter_nativeReport(
  JNIEnv* env, jobject clazz, jint eventType, jobject perfEvent) {
  struct ThreadPriorityInfo threadPriorityInfo;
  if (PerfEventReporter::beginReportPerfEvent(env, threadPriorityInfo)) {
    PerfEventReporter::reportPerfEvent(env, eventType, perfEvent, threadPriorityInfo);
    PerfEventReporter::endReportPerfEvent(env, threadPriorityInfo);
  }
}

static void android_os_statistics_PerfEventReporter_waitForPerfEventArrived(
  JNIEnv* env, jobject clazz, jint timeoutMillis) {
  PerfEventReporter::waitForPerfEventArrived(env, timeoutMillis);
}

static jint android_os_statistics_PerfEventReporter_fetchProcUserspacePerfEvents(
  JNIEnv* env, jobject clazz, jobjectArray fetchingBuffer) {
  return PerfEventReporter::fetchProcUserspacePerfEvents(env, fetchingBuffer);
}

static jint android_os_statistics_PerfEventReporter_fetchProcKernelPerfEvents(
  JNIEnv* env, jobject clazz, jobjectArray fetchingBuffer) {
  return PerfEventReporter::fetchProcKernelPerfEvents(env, fetchingBuffer);
}

static jlong android_os_statistics_PerfEventReporter_getEarliestBeginUptimeMillisOfExecutingRootEvents(
  JNIEnv* env, jobject clazz) {
  return ExecutingRootEvents::getEarliestBeginUptimeMillis();
}

/*
 * JNI registration.
 */
static const JNINativeMethod gMethods[] = {
  /* name, signature, funcPtr */
  { "nativeReport",    "(ILandroid/os/statistics/PerfEvent;)V",
      (void*) android_os_statistics_PerfEventReporter_nativeReport },
  { "waitForPerfEventArrived",    "(I)V",
      (void*) android_os_statistics_PerfEventReporter_waitForPerfEventArrived },
  { "fetchProcUserspacePerfEvents",    "([Landroid/os/statistics/FilteringPerfEvent;)I",
      (void*) android_os_statistics_PerfEventReporter_fetchProcUserspacePerfEvents },
  { "fetchProcKernelPerfEvents",    "([Landroid/os/statistics/FilteringPerfEvent;)I",
      (void*) android_os_statistics_PerfEventReporter_fetchProcKernelPerfEvents },
  { "getEarliestBeginUptimeMillisOfExecutingRootEvents",    "()J",
      (void*) android_os_statistics_PerfEventReporter_getEarliestBeginUptimeMillisOfExecutingRootEvents },
};

const char* const kPerfEventReporterPathName = "android/os/statistics/PerfEventReporter";

int register_android_os_statistics_PerfEventReporter(JNIEnv* env)
{
  return RegisterMethodsOrDie(env, kPerfEventReporterPathName, gMethods, NELEM(gMethods));
}
