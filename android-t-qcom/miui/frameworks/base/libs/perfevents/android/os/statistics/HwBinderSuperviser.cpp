
#define LOG_TAG "HwBinderSuperviser"

#include <cutils/sched_policy.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cutils/compiler.h>
#include <hwbinder/ProcessState.h>
#include <hwbinder/IPCThreadState.h>
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
#include "HwBinderSuperviser.h"
#include "BinderEventThreadLocal.h"
#include "PerfEvent.h"

#include "callbacks/hwbindersupervision_callbacks.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

#define MIN_BINDER_INTERFACE_DESCRIPTOR_LEN 5
#define MAX_BINDER_INTERFACE_DESCRIPTOR_LEN 100

static const char* readInterfaceDescriptor(const Hw::Parcel& parcel) {
  size_t dataPosition = parcel.dataPosition();
  parcel.setDataPosition(0);
  const char* descriptor = parcel.readCString();
  // restore parcel data position
  parcel.setDataPosition(dataPosition);

  if (descriptor != nullptr) {
    int len = strlen(descriptor);
    if (len >= MIN_BINDER_INTERFACE_DESCRIPTOR_LEN && len <= MAX_BINDER_INTERFACE_DESCRIPTOR_LEN) {
      return descriptor;
    }
  }
  return nullptr;
}

static bool isPerfSupervisionOn() {
  return PerfSuperviser::isPerfEventReportable();
}

void HwBinderSuperviser::reportBinderStarvation(int32_t maxThreads, int64_t starvationTimeMs) {
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

int64_t HwBinderSuperviser::beginBinderCall(Hw::IBinder* binder, uint32_t code, uint32_t flags) {
  if (!PerfSuperviser::isPerfEventReportable()) {
    return 0;
  }
  return FastSystemInfoFetcher::getCoarseUptimeMillisFast();
}

void HwBinderSuperviser::endBinderCall(Hw::IBinder* binder, const Hw::Parcel& parcel, uint32_t code, uint32_t flags, int64_t beginUptimeMs) {
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

  const char* interfaceDescriptor = readInterfaceDescriptor(parcel);
  if (interfaceDescriptor == nullptr) {
    ALOGE("endBinderCallInner: write binder info failed!!!");
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
    pNativePartParcel->writeBinder(nullptr); // binder
    pNativePartParcel->writeCString(std::make_shared<std::string>(interfaceDescriptor));
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

void HwBinderSuperviser::beginBinderExecution(Hw::IBinder* binder, uint32_t code, uint32_t flags) {
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

void HwBinderSuperviser::endBinderExecution(Hw::IBinder* binder, const Hw::Parcel& parcel, uint32_t code, uint32_t flags) {
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
  const char* interfaceDescriptor = nullptr;
  if (spentTimeMs < PerfSuperviser::getInnerWaitThresholdMs()) {
    goto FINISH;
  }
  if (PerfSuperviser::hasKernelPerfEvents() &&
      !ExecutingRootEvents::maybeEffectiveMicroscopicEvent(endUptimeMs, true)) {
    goto FINISH;
  }

  interfaceDescriptor = readInterfaceDescriptor(parcel);
  if (interfaceDescriptor == nullptr) {
    ALOGE("endBinderExecutionInner: write binder info failed!!!");
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
    microEvent.mSynchronizationId = generateCoordinationId(Hw::IPCThreadState::self()->getCallingPid(), code);

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
    pNativePartParcel->writeBinder(nullptr); // binder
    pNativePartParcel->writeCString(std::make_shared<std::string>(interfaceDescriptor));
    pNativePartParcel->writeInt32(code);
    pNativePartParcel->writeInt32(Hw::IPCThreadState::self()->getCallingPid());

    microEvent.mDetails.mGlobalDetails = pNativePartParcel;
    microEvent.mDetails.mGlobalJavaBackTrace = nullptr;
    PerfEventReporter::reportPerfEvent(env,microEvent, threadPriorityInfo);
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

void HwBinderSuperviser::init() {
  struct HwBinderSupervisionCallBacks *sHwBinderSupervisionCallBacks = (struct HwBinderSupervisionCallBacks*) malloc(sizeof(struct HwBinderSupervisionCallBacks));
  sHwBinderSupervisionCallBacks->isPerfSupervisionOn = isPerfSupervisionOn;
  sHwBinderSupervisionCallBacks->reportHwBinderStarvation = HwBinderSuperviser::reportBinderStarvation;
  sHwBinderSupervisionCallBacks->beginHwBinderCall = HwBinderSuperviser::beginBinderCall;
  sHwBinderSupervisionCallBacks->endHwBinderCall = HwBinderSuperviser::endBinderCall;
  sHwBinderSupervisionCallBacks->beginHwBinderExecution = HwBinderSuperviser::beginBinderExecution;
  sHwBinderSupervisionCallBacks->endHwBinderExecution = HwBinderSuperviser::endBinderExecution;
  setHwBinderCallbacksInEvn(sHwBinderSupervisionCallBacks, false);
}

} //namespace statistics
} //namespace os
} //namespace android
