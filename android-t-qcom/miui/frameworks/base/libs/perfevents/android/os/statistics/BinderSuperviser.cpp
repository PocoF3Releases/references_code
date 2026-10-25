
#define LOG_TAG "BinderSuperviser"

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
#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <utils/Log.h>
#include <utils/Trace.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "OsUtils.h"
#include "JavaVMUtils.h"
#include "ThreadUtils.h"
#include "PerfEventConstants.h"
#include "NativePerfEventParcelCache.h"
#include "JavaBackTraceCache.h"
#include "ThreadLocalPerfData.h"
#include "FastSystemInfoFetcher.h"
#include "ExecutingRootEvents.h"
#include "PerfSuperviser.h"
#include "PerfEventReporter.h"
#include "BinderSuperviser.h"
#include "BinderEventThreadLocal.h"
#include "PerfEvent.h"

#include "android_util_Binder.h"

#include "callbacks/bindersupervision_callbacks.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

static bool isPerfSupervisionOn() {
  return PerfSuperviser::isPerfEventReportable();
}

void BinderSuperviser::reportBinderStarvation(int32_t maxThreads, int64_t starvationTimeMs) {
  if (!PerfSuperviser::isPerfEventReportable()) {
    return;
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  if (starvationTimeMs < PerfSuperviser::getInnerWaitThresholdMs()) {
    return;
  }

  JNIEnv* env = PerfSuperviser::getJNIEnv();
  struct ThreadPriorityInfo threadPriorityInfo;
  if (PerfEventReporter::beginReportPerfEvent(env, threadPriorityInfo)) {
    PerfEvent macroEvent;
    macroEvent.mEventType = TYPE_BINDER_STARVATION;
    macroEvent.mEventFlags = 0;
    macroEvent.mEndUptimeMillis = FastSystemInfoFetcher::getCoarseUptimeMillisFast();
    macroEvent.mBeginUptimeMillis = macroEvent.mEndUptimeMillis - starvationTimeMs;
    macroEvent.mInclusionId = 0;
    macroEvent.mSynchronizationId = 0;

    NativePerfEventParcel* pNativePartParcel = NativePerfEventParcelCache::obtain();
    pNativePartParcel->writeInt32(PerfSuperviser::myPid());
    pNativePartParcel->writeInt32(maxThreads);

    macroEvent.mDetails.mGlobalDetails = pNativePartParcel;
    macroEvent.mDetails.mGlobalJavaBackTrace = nullptr;
    PerfEventReporter::reportPerfEvent(env, macroEvent, threadPriorityInfo);
    PerfEventReporter::endReportPerfEvent(env, threadPriorityInfo);
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("reportBinderStarvation spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif
}

int64_t BinderSuperviser::beginBinderCall(IBinder* binder, uint32_t code, uint32_t flags) {
  if (!PerfSuperviser::isPerfEventReportable()) {
    return 0;
  }
  return FastSystemInfoFetcher::getCoarseUptimeMillisFast();
}

void BinderSuperviser::endBinderCall(IBinder* binder, uint32_t code, uint32_t flags, int64_t beginUptimeMs) {
  if (!PerfSuperviser::isPerfEventReportable() || beginUptimeMs == 0) {
    return;
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  int64_t endUptimeMs = FastSystemInfoFetcher::getCoarseUptimeMillisFast();
  if (endUptimeMs - beginUptimeMs < PerfSuperviser::getInnerWaitThresholdMs()) {
    return;
  }
  if (!ExecutingRootEvents::maybeEffectiveMicroscopicEvent(endUptimeMs, true)) {
    return;
  }

  JNIEnv* env = PerfSuperviser::getJNIEnv();
  struct ThreadPriorityInfo threadPriorityInfo;
  if (PerfEventReporter::beginReportPerfEvent(env, threadPriorityInfo)) {
    int32_t threadId;
    std::shared_ptr<std::string> threadName;
    ThreadUtils::getThreadInfo(env, threadId, threadName);

    PerfEvent microEvent;
    microEvent.mEventType = TYPE_SINGLE_BINDER_CALL;
    microEvent.mEventFlags = (FLAG_BLOCKED | FLAG_BLOCKED_BY_CROSS_PROCESS | FLAG_BLOCKED_BY_ONE_INCLUSIVE_BLOCKER | FLAG_HAS_PROC_LAZYINFO);
    microEvent.mBeginUptimeMillis = beginUptimeMs;
    microEvent.mEndUptimeMillis = endUptimeMs;
    microEvent.mInclusionId = generateCoordinationId(PerfSuperviser::myPid(), threadId);
    microEvent.mSynchronizationId = generateCoordinationId(PerfSuperviser::myPid(), code);

    NativePerfEventParcel* pNativePartParcel = NativePerfEventParcelCache::obtain();
    pNativePartParcel->writeInt32(PerfSuperviser::myPid());
    pNativePartParcel->writeInt32(threadId);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_policy);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_priority);
    pNativePartParcel->writeBinder(binder);
    pNativePartParcel->writeShortCString(nullptr); // interface descriptor
    pNativePartParcel->writeInt32(code);

    jclass currentClass = nullptr;
    jobject javaBackTrace = nullptr;
    if (env != nullptr) {
      currentClass = JavaVMUtils::getCurrentClass(env);
      if (currentClass != nullptr) {
        javaBackTrace = JavaBackTraceCache::obtain(env);
        JavaVMUtils::fillInJavaStackBackTrace(env, javaBackTrace);
      }
    }

    if (currentClass != nullptr) {
      env->DeleteLocalRef(currentClass);
    }

    microEvent.mDetails.mGlobalDetails = pNativePartParcel;
    microEvent.mDetails.mGlobalJavaBackTrace = javaBackTrace;
    PerfEventReporter::reportPerfEvent(env, microEvent, threadPriorityInfo);
    PerfEventReporter::endReportPerfEvent(env, threadPriorityInfo);
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("endBinderCall spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif
}

void BinderSuperviser::beginBinderExecution(IBinder* binder, uint32_t code, uint32_t flags) {
  if (!PerfSuperviser::isPerfEventReportable() || (flags & IBinder::FLAG_ONEWAY) != 0) {
    return;
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  JNIEnv* env = PerfSuperviser::getJNIEnv();
  struct ThreadBinderExecutionsData* pLocalData = BinderEventThreadLocal::getThreadBinderExecutionsData(env);
  pLocalData->depth++;
  if (pLocalData->depth > MAX_BINDER_EXECUTION_DEPTH) {
    return;
  }

  const int32_t pos = pLocalData->depth - 1;
  struct BinderExecution* pBinderExecution = pLocalData->binderExecutions + pos;
  int64_t beginUptimeMs = FastSystemInfoFetcher::getCoarseUptimeMillisFast();
  pBinderExecution->beginUptimeMs = beginUptimeMs;
  pBinderExecution->beginRunningTimeMs = OsUtils::getRunningTimeMs();
  if (PerfSuperviser::isInHeavyMode()) {
    int64_t temp;
    OsUtils::getThreadSchedStat(pLocalData->threadId, temp, pBinderExecution->beginRunnableTimeMs);
  } else {
    pBinderExecution->beginRunnableTimeMs = 0;
  }
  if (pos == 0) {
    pBinderExecution->cookie =
      ExecutingRootEvents::beginRootEvent(beginUptimeMs);
  } else {
    pBinderExecution->cookie = 0;
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("beginBinderExecution spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif
}

void BinderSuperviser::endBinderExecution(IBinder* binder, uint32_t code, uint32_t flags) {
  if (!PerfSuperviser::isPerfEventReportable() || (flags & IBinder::FLAG_ONEWAY) != 0) {
    return;
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  JNIEnv* env = PerfSuperviser::getJNIEnv();
  struct ThreadBinderExecutionsData* pLocalData = BinderEventThreadLocal::getThreadBinderExecutionsData(env);
  if (pLocalData->depth == 0) {
    return;
  }
  pLocalData->depth--;
  if (pLocalData->depth >= MAX_BINDER_EXECUTION_DEPTH) {
    return;
  }

  const int pos = pLocalData->depth;
  struct BinderExecution* pBinderExecution = pLocalData->binderExecutions + pos;
  int64_t beginUptimeMs = pBinderExecution->beginUptimeMs;
  int64_t endUptimeMs = FastSystemInfoFetcher::getCoarseUptimeMillisFast();
  int32_t spentTimeMs = (int32_t)(endUptimeMs - beginUptimeMs);
  if (spentTimeMs < PerfSuperviser::getInnerWaitThresholdMs()) {
    goto FINISH;
  }
  if (PerfSuperviser::hasKernelPerfEvents() &&
      !ExecutingRootEvents::maybeEffectiveMicroscopicEvent(endUptimeMs, true)) {
    goto FINISH;
  }

  struct ThreadPriorityInfo threadPriorityInfo;
  if (PerfEventReporter::beginReportPerfEvent(env, threadPriorityInfo)) {
    PerfEvent microEvent;
    microEvent.mEventType = TYPE_SINGLE_BINDER_EXECUTION;
    microEvent.mEventFlags = (FLAG_ROOT_EVENT | FLAG_BLOCKER | FLAG_BLOCKER_TO_CROSS_PROCESS | FLAG_INITIATOR_POSITION_SLAVE | FLAG_HAS_PROC_LAZYINFO);
    microEvent.mBeginUptimeMillis = beginUptimeMs;
    microEvent.mEndUptimeMillis = endUptimeMs;
    microEvent.mInclusionId = generateCoordinationId(PerfSuperviser::myPid(), pLocalData->threadId);
    microEvent.mSynchronizationId = generateCoordinationId(IPCThreadState::self()->getCallingPid(), code);

    NativePerfEventParcel* pNativePartParcel = NativePerfEventParcelCache::obtain();
    pNativePartParcel->writeInt32(PerfSuperviser::myPid());
    pNativePartParcel->writeInt32(pLocalData->threadId);
    pNativePartParcel->writeProcessName();
    pNativePartParcel->writePackageName();
    pNativePartParcel->writeCString(pLocalData->threadName);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_policy);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_priority);
    int64_t endRunningTimeMs = OsUtils::getRunningTimeMs();
    int32_t runningTimeMs = (int32_t)(endRunningTimeMs - pBinderExecution->beginRunningTimeMs);
    int32_t runnableTimeMs;
    int32_t sleepingTimeMs;
    if (PerfSuperviser::isInHeavyMode()) {
      int64_t temp;
      int64_t endRunnableTimeMs = 0;
      OsUtils::getThreadSchedStat(pLocalData->threadId, temp, endRunnableTimeMs);
      runnableTimeMs = (int32_t)(endRunnableTimeMs - pBinderExecution->beginRunnableTimeMs);
      sleepingTimeMs = (int32_t)(endUptimeMs - beginUptimeMs) - runnableTimeMs - runningTimeMs;
      if (sleepingTimeMs < 0) {
        sleepingTimeMs = 0;
      }
    } else {
      runnableTimeMs = 0;
      sleepingTimeMs = 0;
    }
    pNativePartParcel->writeInt32(runningTimeMs);
    pNativePartParcel->writeInt32(runnableTimeMs);
    pNativePartParcel->writeInt32(sleepingTimeMs);
    pNativePartParcel->writeInt64(OsUtils::currentTimeMillis());
    pNativePartParcel->writeBinder(binder);
    pNativePartParcel->writeShortCString(nullptr); // interface descriptor
    pNativePartParcel->writeInt32(code);
    pNativePartParcel->writeInt32(IPCThreadState::self()->getCallingPid());

    microEvent.mDetails.mGlobalDetails = pNativePartParcel;
    microEvent.mDetails.mGlobalJavaBackTrace = nullptr;
    PerfEventReporter::reportPerfEvent(env, microEvent, threadPriorityInfo);
    PerfEventReporter::endReportPerfEvent(env, threadPriorityInfo);
  }

FINISH:
  if (pos == 0) {
    ExecutingRootEvents::endRootEvent(pBinderExecution->cookie, beginUptimeMs, endUptimeMs);
  }

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("endBinderExecution spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif
}

const char* const kBinderWrapperPathName = "android/os/statistics/BinderSuperviser$BinderWrapper";

static struct nativebinderrefernce_offsets_t
{
  // Class state.
  jclass mClass;
  jmethodID mConstructor;

  // Object state.
  jfieldID mObject;
} gBinderWrapperOffsets;

static jobject createBinderWrapper(JNIEnv* env, IBinder* binder) {
  jobject object = env->NewObject(gBinderWrapperOffsets.mClass, gBinderWrapperOffsets.mConstructor);
  if (object != NULL) {
    // The native binder reference holds a reference to the native object.
    env->SetLongField(object, gBinderWrapperOffsets.mObject, (jlong)binder);
    binder->incStrong((void*)createBinderWrapper);
  }
  return object;
}

jobject BinderSuperviser::wrapNativeBinder(JNIEnv* env, IBinder* binder) {
  jobject object;
  if (binder == NULL) {
    object = NULL;
  } else {
#ifdef SUPPORT_IS_JAVA_BINDER_JUDGEMENT
    if (isJavaBinder(env, binder)) {
      object = javaObjectForIBinder(env, binder);
    } else {
      object = createBinderWrapper(env, binder);
    }
#else
    object = NULL;
#endif
  }
  return object;
}

static jstring android_os_statistics_BinderSuperviser_BinderWrapper_getInterfaceDescriptor(JNIEnv* env, jobject obj) {
  IBinder* target = (IBinder*) env->GetLongField(obj, gBinderWrapperOffsets.mObject);
  if (target != NULL) {
    const String16& desc = target->getInterfaceDescriptor();
    return env->NewString(reinterpret_cast<const jchar*>(desc.string()), desc.size());
  }
  jniThrowException(env, "java/lang/RuntimeException", "No binder found for object");
  return NULL;
}

static void android_os_statistics_BinderSuperviser_BinderWrapper_destroy(JNIEnv* env, jobject obj) {
  IBinder* binder = (IBinder*)env->GetLongField(obj, gBinderWrapperOffsets.mObject);
  if (binder != nullptr) {
    env->SetLongField(obj, gBinderWrapperOffsets.mObject, 0);
    binder->decStrong((void*)createBinderWrapper);
  }
}

static const JNINativeMethod gBinderWrapperMethods[] = {
  /* name, signature, funcPtr */
  {"getInterfaceDescriptor", "()Ljava/lang/String;", (void*)android_os_statistics_BinderSuperviser_BinderWrapper_getInterfaceDescriptor},
  {"destroy",             "()V", (void*)android_os_statistics_BinderSuperviser_BinderWrapper_destroy},
};

void BinderSuperviser::init(JNIEnv* env) {
  jclass clazz = FindClassOrDie(env, kBinderWrapperPathName);
  gBinderWrapperOffsets.mClass = MakeGlobalRefOrDie(env, clazz);
  gBinderWrapperOffsets.mConstructor = GetMethodIDOrDie(env, clazz, "<init>", "()V");
  gBinderWrapperOffsets.mObject = GetFieldIDOrDie(env, clazz, "mObject", "J");
  RegisterMethodsOrDie(
    env, kBinderWrapperPathName,
    gBinderWrapperMethods, NELEM(gBinderWrapperMethods));

  struct BinderSupervisionCallBacks *sBinderSupervisionCallBacks = (struct BinderSupervisionCallBacks*) malloc(sizeof(struct BinderSupervisionCallBacks));
  sBinderSupervisionCallBacks->isPerfSupervisionOn = isPerfSupervisionOn;
  sBinderSupervisionCallBacks->reportBinderStarvation = BinderSuperviser::reportBinderStarvation;
  sBinderSupervisionCallBacks->beginBinderCall = BinderSuperviser::beginBinderCall;
  sBinderSupervisionCallBacks->endBinderCall = BinderSuperviser::endBinderCall;
  sBinderSupervisionCallBacks->beginBinderExecution = BinderSuperviser::beginBinderExecution;
  sBinderSupervisionCallBacks->endBinderExecution = BinderSuperviser::endBinderExecution;
#ifdef MIUI_BINDER_MONITOR
  // BinderServerMonitor会重新设置BinderCallBacks至环境变量中, 并会回调以上方法.
  monitor::BinderServerMonitor::pBinderSupervisionCallBacksDelegate = sBinderSupervisionCallBacks;
#endif
  setBinderCallbacksInEvn(sBinderSupervisionCallBacks, false);
}

} //namespace statistics
} //namespace os
} //namespace android
