
#define LOG_TAG "JavaVMSupervisionCallbacks"
#define ATRACE_TAG ATRACE_TAG_DALVIK

#include <cutils/sched_policy.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cutils/compiler.h>
#include <utils/Log.h>
#include <utils/Trace.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "PerfEventConstants.h"
#include "javavmsupervision_callbacks.h"
#include "OsUtils.h"
#include "JavaVMUtils.h"
#include "NativePerfEventParcelCache.h"
#include "JavaBackTraceCache.h"
#include "ExecutingRootEvents.h"
#include "PerfSuperviser.h"
#include "FastSystemInfoFetcher.h"
#include "JavaVMSuperviser.h"
#include "PerfEventReporter.h"
#include "PerfEvent.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

static const char* const KObjectPathName = "java/lang/Object";
static jclass sObjectClass = nullptr;

static const char* const KThreadPathName = "java/lang/Thread";
static jclass sThreadClass = nullptr;

static const char* const KMessageQueuePathName = "android/os/MessageQueue";
static jclass sMessageQueueClass = nullptr;

static const char* const KBinderProxyPathName = "android/os/BinderProxy";
static jclass sBinderProxyClass = nullptr;

static const char* const KHwBinderPathName = "android/os/HwBinder";
static jclass sHwBinderClass = nullptr;

struct JavaVMInterface *g_pJavaVMInterface = nullptr;

void JavaVMSuperviser::init(JNIEnv* env) {
  jclass clazz = FindClassOrDie(env, KObjectPathName);
  sObjectClass = MakeGlobalRefOrDie(env, clazz);

  clazz = FindClassOrDie(env, KThreadPathName);
  sThreadClass = MakeGlobalRefOrDie(env, clazz);

  clazz = FindClassOrDie(env, KMessageQueuePathName);
  sMessageQueueClass = MakeGlobalRefOrDie(env, clazz);

  clazz = FindClassOrDie(env, KBinderProxyPathName);
  sBinderProxyClass = MakeGlobalRefOrDie(env, clazz);

#ifdef HWBINDER_MONITOR_ENABLED
  clazz = FindClassOrDie(env, KHwBinderPathName);
  sHwBinderClass = MakeGlobalRefOrDie(env, clazz);
#endif
}

static void exportJavaVMInterface(struct JavaVMInterface *interface) {
  g_pJavaVMInterface = interface;
}

static bool isPerfSupervisionOn() {
  return PerfSuperviser::isPerfEventReportable();
}

static int64_t getUptimeMillisFast() {
  return FastSystemInfoFetcher::getCoarseUptimeMillisFast();
}

static void reportLockReleased(JNIEnv* env,
  int64_t monitorId, int64_t beginUptimeMs, int64_t endUptimeMs)
{
  if (!PerfSuperviser::isPerfEventReportable() ||
      endUptimeMs - beginUptimeMs < PerfSuperviser::getInnerWaitThresholdMs()) {
    return;
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  if (!ExecutingRootEvents::maybeEffectiveMicroscopicEvent(endUptimeMs, false)) {
    return;
  }

  if (ATRACE_ENABLED()) {
    ATRACE_BEGIN("reportLockReleased");
  }

  struct ThreadPriorityInfo threadPriorityInfo;
  if (PerfEventReporter::beginReportPerfEvent(env, threadPriorityInfo)) {
    int32_t threadId;
    std::shared_ptr<std::string> threadName;
    JavaVMUtils::getThreadInfo(env, threadId, threadName);

    PerfEvent microEvent;
    microEvent.mEventType = TYPE_SINGLE_LOCK_HOLD;
    microEvent.mEventFlags = (FLAG_BLOCKER | FLAG_BLOCKER_TO_SAME_PROCESS | FLAG_HAS_PROC_LAZYINFO);
    microEvent.mBeginUptimeMillis = beginUptimeMs;
    microEvent.mEndUptimeMillis = endUptimeMs;
    microEvent.mInclusionId = generateCoordinationId(PerfSuperviser::myPid(), threadId);
    microEvent.mSynchronizationId = monitorId;

    NativePerfEventParcel* pNativePartParcel = NativePerfEventParcelCache::obtain();
    pNativePartParcel->writeInt32(PerfSuperviser::myPid());
    pNativePartParcel->writeInt32(threadId);
    pNativePartParcel->writeCString(threadName);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_policy);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_priority);
    pNativePartParcel->writeInt64(monitorId);

    jobject javaBackTrace = JavaBackTraceCache::obtain(env);
    JavaVMUtils::fillInJavaStackBackTrace(env, javaBackTrace);

    microEvent.mDetails.mGlobalDetails = pNativePartParcel;
    microEvent.mDetails.mGlobalJavaBackTrace = javaBackTrace;
    PerfEventReporter::reportPerfEvent(env, microEvent, threadPriorityInfo);
    PerfEventReporter::endReportPerfEvent(env, threadPriorityInfo);
  }

  if (ATRACE_ENABLED()) {
    ATRACE_END();
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("reportLockReleased spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif
}

static void reportLockAccquired(JNIEnv* env, int64_t monitorId, int64_t beginUptimeMs, int64_t endUptimeMs) {
  if (!PerfSuperviser::isPerfEventReportable() ||
      endUptimeMs - beginUptimeMs < PerfSuperviser::getInnerWaitThresholdMs()) {
    return;
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  if (!ExecutingRootEvents::maybeEffectiveMicroscopicEvent(endUptimeMs, true)) {
    return;
  }

  if (ATRACE_ENABLED()) {
    ATRACE_BEGIN("reportLockAccquired");
  }

  struct ThreadPriorityInfo threadPriorityInfo;
  if (PerfEventReporter::beginReportPerfEvent(env, threadPriorityInfo)) {
    int32_t threadId;
    std::shared_ptr<std::string> threadName;
    JavaVMUtils::getThreadInfo(env, threadId, threadName);

    PerfEvent microEvent;
    microEvent.mEventType = TYPE_SINGLE_LOCK_WAIT;
    microEvent.mEventFlags = (FLAG_BLOCKED | FLAG_BLOCKED_BY_SAME_PROCESS | FLAG_BLOCKED_BY_MULTIBPLE_BLOCKER | FLAG_HAS_PROC_LAZYINFO);
    microEvent.mBeginUptimeMillis = beginUptimeMs;
    microEvent.mEndUptimeMillis = endUptimeMs;
    microEvent.mInclusionId = generateCoordinationId(PerfSuperviser::myPid(), threadId);
    microEvent.mSynchronizationId = monitorId;

    NativePerfEventParcel* pNativePartParcel = NativePerfEventParcelCache::obtain();
    pNativePartParcel->writeInt32(PerfSuperviser::myPid());
    pNativePartParcel->writeInt32(threadId);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_policy);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_priority);
    pNativePartParcel->writeInt64(monitorId);

    jobject javaBackTrace = JavaBackTraceCache::obtain(env);
    JavaVMUtils::fillInJavaStackBackTrace(env, javaBackTrace);

    microEvent.mDetails.mGlobalDetails = pNativePartParcel;
    microEvent.mDetails.mGlobalJavaBackTrace = javaBackTrace;
    PerfEventReporter::reportPerfEvent(env, microEvent, threadPriorityInfo);
    PerfEventReporter::endReportPerfEvent(env, threadPriorityInfo);
  }

  if (ATRACE_ENABLED()) {
    ATRACE_END();
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("reportLockAccquired spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif
}

static void reportConditionAwaken(JNIEnv* env, int64_t monitorId, int32_t peerThreadId, int64_t beginUptimeMs, int64_t endUptimeMs) {
  if (!PerfSuperviser::isPerfEventReportable() ||
      endUptimeMs - beginUptimeMs < PerfSuperviser::getInnerWaitThresholdMs()) {
    return;
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  if (!ExecutingRootEvents::maybeEffectiveMicroscopicEvent(endUptimeMs, false)) {
    return;
  }

  if (ATRACE_ENABLED()) {
    ATRACE_BEGIN("reportConditionAwaken");
  }

  struct ThreadPriorityInfo threadPriorityInfo;
  if (PerfEventReporter::beginReportPerfEvent(env, threadPriorityInfo)) {
    int32_t threadId;
    std::shared_ptr<std::string> threadName;
    JavaVMUtils::getThreadInfo(env, threadId, threadName);

    PerfEvent microEvent;
    microEvent.mEventType = TYPE_SINGLE_CONDITION_AWAKEN;
    microEvent.mEventFlags = (FLAG_BLOCKER | FLAG_BLOCKER_TO_SAME_PROCESS | FLAG_HAS_PROC_LAZYINFO);
    microEvent.mBeginUptimeMillis = beginUptimeMs;
    microEvent.mEndUptimeMillis = endUptimeMs;
    microEvent.mInclusionId = generateCoordinationId(PerfSuperviser::myPid(), threadId);
    microEvent.mSynchronizationId = monitorId;

    NativePerfEventParcel* pNativePartParcel = NativePerfEventParcelCache::obtain();
    pNativePartParcel->writeInt32(PerfSuperviser::myPid());
    pNativePartParcel->writeInt32(threadId);
    pNativePartParcel->writeCString(threadName);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_policy);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_priority);
    pNativePartParcel->writeInt64(monitorId);
    pNativePartParcel->writeInt32(peerThreadId);

    jobject javaBackTrace = JavaBackTraceCache::obtain(env);
    JavaVMUtils::fillInJavaStackBackTrace(env, javaBackTrace);

    microEvent.mDetails.mGlobalDetails = pNativePartParcel;
    microEvent.mDetails.mGlobalJavaBackTrace = javaBackTrace;
    PerfEventReporter::reportPerfEvent(env, microEvent, threadPriorityInfo);
    PerfEventReporter::endReportPerfEvent(env, threadPriorityInfo);
  }

  if (ATRACE_ENABLED()) {
    ATRACE_END();
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("reportConditionAwaken spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif
}

static void reportConditionSatisfied(JNIEnv* env, int64_t monitorId, int32_t awakenReason, int64_t beginUptimeMs, int64_t endUptimeMs) {
  if (!PerfSuperviser::isPerfEventReportable() ||
      endUptimeMs - beginUptimeMs < PerfSuperviser::getInnerWaitThresholdMs()) {
    return;
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  if (!ExecutingRootEvents::maybeEffectiveMicroscopicEvent(endUptimeMs, true)) {
    return;
  }

  if (ATRACE_ENABLED()) {
    ATRACE_BEGIN("reportConditionSatisfied");
  }

  struct ThreadPriorityInfo threadPriorityInfo;
  if (PerfEventReporter::beginReportPerfEvent(env, threadPriorityInfo)) {
    int32_t threadId;
    std::shared_ptr<std::string> threadName;
    JavaVMUtils::getThreadInfo(env, threadId, threadName);

    PerfEvent microEvent;
    microEvent.mEventType = TYPE_SINGLE_CONDITION_WAIT;
    microEvent.mEventFlags = FLAG_BLOCKED;
    if (awakenReason == CONDITION_AWAKEN_NOTIFIED) {
      microEvent.mEventFlags |= (FLAG_BLOCKED_BY_SAME_PROCESS | FLAG_BLOCKED_BY_ONE_OVERLAPPED_BLOCKER | FLAG_HAS_PROC_LAZYINFO);
    }
    microEvent.mBeginUptimeMillis = beginUptimeMs;
    microEvent.mEndUptimeMillis = endUptimeMs;
    microEvent.mInclusionId = generateCoordinationId(PerfSuperviser::myPid(), threadId);
    microEvent.mSynchronizationId = monitorId;

    NativePerfEventParcel* pNativePartParcel = NativePerfEventParcelCache::obtain();
    pNativePartParcel->writeInt32(PerfSuperviser::myPid());
    pNativePartParcel->writeInt32(threadId);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_policy);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_priority);
    pNativePartParcel->writeInt64(monitorId);
    pNativePartParcel->writeInt32(awakenReason);

    jobject javaBackTrace = JavaBackTraceCache::obtain(env);
    JavaVMUtils::fillInJavaStackBackTrace(env, javaBackTrace);

    microEvent.mDetails.mGlobalDetails = pNativePartParcel;
    microEvent.mDetails.mGlobalJavaBackTrace = javaBackTrace;
    PerfEventReporter::reportPerfEvent(env, microEvent, threadPriorityInfo);
    PerfEventReporter::endReportPerfEvent(env, threadPriorityInfo);
  }

  if (ATRACE_ENABLED()) {
    ATRACE_END();
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("reportConditionSatisfied spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif
}

static void reportJniMethodInvocation(JNIEnv* env, int64_t beginUptimeMs, int64_t endUptimeMs, int32_t& reportedTimeMillis) {
  int32_t spentTimeMillis = (int32_t)(endUptimeMs - beginUptimeMs);
  if (!PerfSuperviser::isPerfEventReportable() ||
      spentTimeMillis < PerfSuperviser::getInnerWaitThresholdMs() ||
      spentTimeMillis - reportedTimeMillis < PerfSuperviser::getInnerWaitThresholdMs() / 2) {
    return;
  }

  // if an exception has been raised in current thread, we cannot invoke any other
  // JNI method. Give up to report this JNI call.
  if (env->ExceptionCheck()) {
    return;
  }
#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  if (!ExecutingRootEvents::maybeEffectiveMicroscopicEvent(endUptimeMs, true)) {
    return;
  }

  jclass currentClass = JavaVMUtils::getCurrentClass(env);
  if (env->IsSameObject(currentClass, sObjectClass) ||
      env->IsSameObject(currentClass, sThreadClass) ||
      env->IsSameObject(currentClass, sMessageQueueClass) ||
      env->IsSameObject(currentClass, sBinderProxyClass) ||
      (sHwBinderClass != nullptr && env->IsSameObject(currentClass, sHwBinderClass))) {
    env->DeleteLocalRef(currentClass);
    return;
  }
  env->DeleteLocalRef(currentClass);

  struct ThreadPriorityInfo threadPriorityInfo;
  if (PerfEventReporter::beginReportPerfEvent(env, threadPriorityInfo)) {
    int32_t threadId;
    std::shared_ptr<std::string> threadName;
    JavaVMUtils::getThreadInfo(env, threadId, threadName);

    PerfEvent microEvent;
    microEvent.mEventType = TYPE_SINGLE_JNI_METHOD;
    microEvent.mEventFlags = FLAG_HAS_PROC_LAZYINFO;
    microEvent.mBeginUptimeMillis = beginUptimeMs;
    microEvent.mEndUptimeMillis = endUptimeMs;
    microEvent.mInclusionId = generateCoordinationId(PerfSuperviser::myPid(), threadId);
    microEvent.mSynchronizationId = generateCoordinationId(PerfSuperviser::myPid(), threadId);

    NativePerfEventParcel* pNativePartParcel = NativePerfEventParcelCache::obtain();
    pNativePartParcel->writeInt32(PerfSuperviser::myPid());
    pNativePartParcel->writeInt32(threadId);

    jobject javaBackTrace = JavaBackTraceCache::obtain(env);
    JavaVMUtils::fillInJavaStackBackTrace(env, javaBackTrace);

    microEvent.mDetails.mGlobalDetails = pNativePartParcel;
    microEvent.mDetails.mGlobalJavaBackTrace = javaBackTrace;
    PerfEventReporter::reportPerfEvent(env, microEvent, threadPriorityInfo);
    PerfEventReporter::endReportPerfEvent(env, threadPriorityInfo);
    reportedTimeMillis = spentTimeMillis;
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("reportJniMethodInvocation spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif

}

} //namespace statistics
} //namespace os
} //namespace android

struct JavaVMSupervisionCallBacks gJavaVMSupervisionCallBacks = {
  exportJavaVMInterface,
  isPerfSupervisionOn,
  getUptimeMillisFast,
  reportLockReleased,
  reportLockAccquired,
  reportConditionAwaken,
  reportConditionSatisfied,
  reportJniMethodInvocation,
};
