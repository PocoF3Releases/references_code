
#define LOG_TAG "LooperSuperviser"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <cutils/sched_policy.h>
#include <utils/SystemClock.h>
#include <utils/Log.h>
#include <pthread.h>

#include "PerfEventConstants.h"
#include "OsUtils.h"
#include "JavaVMUtils.h"
#include "ThreadUtils.h"
#include "NativePerfEventParcelCache.h"
#include "ThreadLocalPerfData.h"
#include "FastSystemInfoFetcher.h"
#include "PerfSuperviser.h"
#include "ExecutingRootEvents.h"
#include "PerfEventReporter.h"
#include "LooperSuperviser.h"
#include "PerfEvent.h"

#include "callbacks/loopersupervision_callbacks.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

static const char* const kLooperCheckPointDetectorPathName = "android/os/statistics/LooperCheckPointDetector";

static struct loopercheckpointdetector_offsets_t
{
  // Class state.
  jclass mClass;
  jmethodID mBeginLoopOnce;
  jmethodID mEndLoopOnce;
} gLooperCheckPointDetectorOffsets;

static pthread_key_t sIsLooperMonitorEnabledKey = -1;

static bool isPerfSupervisionOn() {
  return PerfSuperviser::isPerfEventReportable();
}

struct ThreadLooperOnceData {
  int32_t threadId;
  std::shared_ptr<std::string> threadName;
  int32_t depth;
  int64_t beginUptimeMs;
  uint64_t cookie;
  bool hasLooperCheckPoint;
};

static void destroyThreadLooperOnceData(void *data) {
  if (data != nullptr) {
    ((struct ThreadLooperOnceData*)data)->threadName.reset();
    delete (struct ThreadLooperOnceData*)data;
  }
}

static struct ThreadLooperOnceData* getThreadLooperOnceData(JNIEnv* env) {
  struct ThreadLooperOnceData* pLocalData =
    (struct ThreadLooperOnceData*)ThreadLocalPerfData::get(TYPE_LOOPER_ONCE);
  if (pLocalData == nullptr) {
    pLocalData = new struct ThreadLooperOnceData();
    ThreadUtils::getThreadInfo(env, pLocalData->threadId, pLocalData->threadName);
    pLocalData->depth = 0;
    pLocalData->beginUptimeMs = 0;
    pLocalData->cookie = 0;
    pLocalData->hasLooperCheckPoint = false;
    ThreadLocalPerfData::set(TYPE_LOOPER_ONCE, pLocalData, destroyThreadLooperOnceData);
  }
  return pLocalData;
}

static void beginLooperOnce(Looper *looper) {
  JNIEnv* env = PerfSuperviser::getJNIEnv();
  struct ThreadLooperOnceData* pLocalData = getThreadLooperOnceData(env);
  pLocalData->depth++;
  if (pLocalData->depth == 1) {
    int64_t beginUptimeMs = FastSystemInfoFetcher::getCoarseUptimeMillisFast();
    pLocalData->beginUptimeMs = beginUptimeMs;
    // beginRootEvent和CheckPoint的开启不能颠倒，因为是否需要CheckPoint是根据当前正在执行的RootEvent判断得出的。
    pLocalData->cookie =
      ExecutingRootEvents::beginMasterRootEvent(beginUptimeMs, false);
    if (!PerfSuperviser::isSystemServer() && env != nullptr &&
        (!PerfSuperviser::hasKernelPerfEvents() || ExecutingRootEvents::isUserPerceptibleMasterRootEvent(pLocalData->cookie, beginUptimeMs))) {
      pLocalData->hasLooperCheckPoint = true;
      env->CallStaticVoidMethod(gLooperCheckPointDetectorOffsets.mClass,
        gLooperCheckPointDetectorOffsets.mBeginLoopOnce, pLocalData->threadId, beginUptimeMs);
    } else {
      pLocalData->hasLooperCheckPoint = false;
    }
  }
}

static void endLooperOnce(Looper *looper) {
  JNIEnv* env = PerfSuperviser::getJNIEnv();
  struct ThreadLooperOnceData* pLocalData = getThreadLooperOnceData(env);
  if (pLocalData->depth == 0) {
    return;
  }

  pLocalData->depth--;
  if (pLocalData->depth == 0) {
    if (pLocalData->hasLooperCheckPoint) {
      env->CallStaticVoidMethod(gLooperCheckPointDetectorOffsets.mClass, gLooperCheckPointDetectorOffsets.mEndLoopOnce);
    }
    int64_t beginUptimeMs = pLocalData->beginUptimeMs;
    int64_t endUptimeMs = FastSystemInfoFetcher::getCoarseUptimeMillisFast();
    int32_t spentTimeMs = (int32_t)(endUptimeMs - beginUptimeMs);
    bool needReportLooperOnce = spentTimeMs >= PerfSuperviser::getSoftThresholdMs() &&
      (!PerfSuperviser::hasKernelPerfEvents() || ExecutingRootEvents::isUserPerceptibleMasterRootEvent(pLocalData->cookie, beginUptimeMs));

    if (needReportLooperOnce) {
      struct ThreadPriorityInfo threadPriorityInfo;
      if (PerfEventReporter::beginReportPerfEvent(env, threadPriorityInfo)) {
        PerfEvent microEvent;
        microEvent.mEventType = TYPE_LOOPER_ONCE;
        microEvent.mEventFlags = (FLAG_INITIATOR_POSITION_MASTER | FLAG_ROOT_EVENT);
        microEvent.mBeginUptimeMillis = beginUptimeMs;
        microEvent.mEndUptimeMillis = endUptimeMs;
        microEvent.mInclusionId = generateCoordinationId(PerfSuperviser::myPid(), pLocalData->threadId);
        microEvent.mSynchronizationId = generateCoordinationId(PerfSuperviser::myPid(), pLocalData->threadId);

        NativePerfEventParcel* pNativePartParcel = NativePerfEventParcelCache::obtain();
        pNativePartParcel->writeInt32(PerfSuperviser::myPid());
        pNativePartParcel->writeInt32(pLocalData->threadId);

        microEvent.mDetails.mGlobalDetails = pNativePartParcel;
        microEvent.mDetails.mGlobalJavaBackTrace = nullptr;
        PerfEventReporter::reportPerfEvent(env, microEvent, threadPriorityInfo);
        PerfEventReporter::endReportPerfEvent(env, threadPriorityInfo);
      }
    }
    ExecutingRootEvents::endMasterRootEvent(pLocalData->cookie, beginUptimeMs, endUptimeMs, false);
  }
}

void LooperSuperviser::awakenedFromMessagePoll(Looper *looper) {
  int savederrno = errno;
  beginLooperOnce(looper);
  errno = savederrno;
}

void LooperSuperviser::beginWaitForMessagePoll(Looper *looper) {
  int savederrno = errno;
  endLooperOnce(looper);
  errno = savederrno;
}

void LooperSuperviser::pausedMessagePoll(Looper *looper) {
  int savederrno = errno;
  endLooperOnce(looper);
  errno = savederrno;
}

void LooperSuperviser::enableLooperMonitor() {
  pthread_setspecific(sIsLooperMonitorEnabledKey, (void*) 1);
}

bool LooperSuperviser::isLooperMonitorEnabled() {
  if (pthread_getspecific(sIsLooperMonitorEnabledKey) != 0) {
    return true;
  }
  return false;
}

#define NATIVE_LOOPER_MESSAGE_DEFAULT_NAME "<Native>"
static std::shared_ptr<std::string>* sNativeLooperMessageDefaultName = nullptr;

struct ThreadLooperMessageData {
  bool hasPendingFinishedTracePoint;
  int32_t threadId;
  std::shared_ptr<std::string> threadName;
  int64_t beginUptimeMs;
  int64_t beginRunningTimeMs;
  int64_t beginRunnableTimeMs;
};

static void destroyThreadLooperMessageData(void *data) {
  if (data != nullptr) {
    ((struct ThreadLooperMessageData*)data)->threadName.reset();
    delete (struct ThreadLooperMessageData*)data;
  }
}

static struct ThreadLooperMessageData* getThreadLooperMessageData(JNIEnv* env) {
  struct ThreadLooperMessageData* pLocalData =
    (struct ThreadLooperMessageData*)ThreadLocalPerfData::get(TYPE_SINGLE_LOOPER_MESSAGE);
  if (pLocalData == nullptr) {
    pLocalData = new struct ThreadLooperMessageData();
    ThreadUtils::getThreadInfo(env, pLocalData->threadId, pLocalData->threadName);
    pLocalData->hasPendingFinishedTracePoint = false;
    pLocalData->beginUptimeMs = 0;
    pLocalData->beginRunningTimeMs = 0;
    pLocalData->beginRunnableTimeMs = 0;
    ThreadLocalPerfData::set(TYPE_SINGLE_LOOPER_MESSAGE,
      pLocalData, destroyThreadLooperMessageData);
  }
  return pLocalData;
}

static void beginLooperMessage(JNIEnv* env) {
  struct ThreadLooperMessageData* pLocalData = getThreadLooperMessageData(env);
  pLocalData->beginUptimeMs = FastSystemInfoFetcher::getCoarseUptimeMillisFast();
  pLocalData->beginRunningTimeMs = OsUtils::getRunningTimeMs();
  if (PerfSuperviser::isInHeavyMode()) {
    int64_t temp;
    OsUtils::getThreadSchedStat(pLocalData->threadId, temp, pLocalData->beginRunnableTimeMs);
  } else {
    pLocalData->beginRunnableTimeMs = 0;
  }
}

static void endLooperMessage(JNIEnv* env, bool isFromJava,
  std::shared_ptr<std::string>* messageName, jstring messageTarget, int32_t messageWhat, jstring messageCallback,
  int64_t planUptimeMillis)
{
  const struct ThreadLooperOnceData* pLocalLooperOnceData = getThreadLooperOnceData(env);
  const struct ThreadLooperMessageData* pLocalData = getThreadLooperMessageData(env);
  if (pLocalData->beginUptimeMs <= 0) {
    return;
  }

  int64_t beginUptimeMs = pLocalData->beginUptimeMs;
  int64_t endUptimeMs = FastSystemInfoFetcher::getCoarseUptimeMillisFast();
  int32_t spentTimeMs = (int32_t)(endUptimeMs - beginUptimeMs);
  if (spentTimeMs < PerfSuperviser::getInnerWaitThresholdMs()) {
    return;
  }
  if (PerfSuperviser::hasKernelPerfEvents() &&
      !ExecutingRootEvents::maybeEffectiveMicroscopicEvent(endUptimeMs, true)) {
    return;
  }

  struct ThreadPriorityInfo threadPriorityInfo;
  if (PerfEventReporter::beginReportPerfEvent(env, threadPriorityInfo)) {
    PerfEvent microEvent;
    microEvent.mEventType = TYPE_SINGLE_LOOPER_MESSAGE;
    microEvent.mEventFlags = (FLAG_INITIATOR_POSITION_MASTER | FLAG_ROOT_EVENT);
    if (pLocalLooperOnceData->depth > 0 &&
      PerfSuperviser::hasKernelPerfEvents() &&
      ExecutingRootEvents::isUserPerceptibleMasterRootEvent(pLocalLooperOnceData->cookie, pLocalLooperOnceData->beginUptimeMs)) {
      microEvent.mEventFlags |= FLAG_USER_PERCEPTIBLE;
    }
    microEvent.mBeginUptimeMillis = beginUptimeMs;
    microEvent.mEndUptimeMillis = endUptimeMs;
    microEvent.mInclusionId = generateCoordinationId(PerfSuperviser::myPid(), pLocalData->threadId);
    microEvent.mSynchronizationId = generateCoordinationId(PerfSuperviser::myPid(), pLocalData->threadId);

    NativePerfEventParcel* pNativePartParcel = NativePerfEventParcelCache::obtain();
    pNativePartParcel->writeInt32(PerfSuperviser::myPid());
    pNativePartParcel->writeInt32(pLocalData->threadId);
    pNativePartParcel->writeProcessName();
    pNativePartParcel->writePackageName();
    pNativePartParcel->writeCString(pLocalData->threadName);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_policy);
    pNativePartParcel->writeInt32(threadPriorityInfo.sched_priority);
    int64_t endRunningTimeMs = OsUtils::getRunningTimeMs();
    int32_t runningTimeMs = (int32_t)(endRunningTimeMs - pLocalData->beginRunningTimeMs);
    int32_t runnableTimeMs;
    int32_t sleepingTimeMs;
    if (PerfSuperviser::isInHeavyMode()) {
      int64_t temp;
      int64_t endRunnableTimeMs = 0;
      OsUtils::getThreadSchedStat(pLocalData->threadId, temp, endRunnableTimeMs);
      runnableTimeMs = (int32_t)(endRunnableTimeMs - pLocalData->beginRunnableTimeMs);
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
    if (messageName != nullptr) {
      pNativePartParcel->writeCString(*messageName);
    } else {
      pNativePartParcel->writeShortCString(nullptr);
    }
    if (env != nullptr) {
      pNativePartParcel->writeJString(env, messageTarget);
    } else {
      pNativePartParcel->writeShortCString(nullptr);
    }
    pNativePartParcel->writeInt32(messageWhat);
    if (env != nullptr) {
      pNativePartParcel->writeJString(env, messageCallback);
    } else {
      pNativePartParcel->writeShortCString(nullptr);
    }
    pNativePartParcel->writeInt64(planUptimeMillis);

    microEvent.mDetails.mGlobalDetails = pNativePartParcel;
    microEvent.mDetails.mGlobalJavaBackTrace = nullptr;
    PerfEventReporter::reportPerfEvent(env, microEvent, threadPriorityInfo);
    PerfEventReporter::endReportPerfEvent(env, threadPriorityInfo);
  }
}

void LooperSuperviser::beginNativeLooperMessage(Looper* lopper) {
#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  JNIEnv* env = PerfSuperviser::getJNIEnv();
  beginLooperMessage(env);

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("beginNativeLooperMessage spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif
}

void LooperSuperviser::endNativeLooperMessage(Looper* lopper, std::shared_ptr<std::string>* messageName) {
#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  JNIEnv* env = PerfSuperviser::getJNIEnv();
  endLooperMessage(env, false,
    messageName != nullptr ? messageName : sNativeLooperMessageDefaultName,
    nullptr, 0, nullptr, 0);

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("endNativeLooperMessage spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif
}

void LooperSuperviser::beginJavaLooperMessage(JNIEnv* env) {
#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  beginLooperMessage(env);

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("beginJavaLooperMessage spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif
}

void LooperSuperviser::endJavaLooperMessage(JNIEnv* env,
  jstring messageTarget, int32_t messageWhat, jstring messageCallback, int64_t planUptimeMillis) {
#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_begin_time = OsUtils::NanoTime();
#endif

  endLooperMessage(env, true, nullptr, messageTarget, messageWhat, messageCallback, planUptimeMillis);

#if DEBUG_MIUI_PERFSUPERVISER
  int64_t measure_end_time = OsUtils::NanoTime();
  ALOGE("endJavaLooperMessage spends %dns", (int32_t)(measure_end_time - measure_begin_time));
#endif
}

void LooperSuperviser::init(JNIEnv* env) {
  sNativeLooperMessageDefaultName = new std::shared_ptr<std::string>();
  *sNativeLooperMessageDefaultName = std::make_shared<std::string>(NATIVE_LOOPER_MESSAGE_DEFAULT_NAME);

  struct LooperSupervisionCallBacks *sLooperSupervisionCallBacks = (struct LooperSupervisionCallBacks *) malloc(sizeof(struct LooperSupervisionCallBacks));
  sLooperSupervisionCallBacks->isPerfSupervisionOn = isPerfSupervisionOn;
  sLooperSupervisionCallBacks->awakenedFromMessagePoll = LooperSuperviser::awakenedFromMessagePoll;
  sLooperSupervisionCallBacks->beginWaitForMessagePoll = LooperSuperviser::beginWaitForMessagePoll;
  sLooperSupervisionCallBacks->pausedMessagePoll = LooperSuperviser::pausedMessagePoll;
  sLooperSupervisionCallBacks->beginLooperMessage = LooperSuperviser::beginNativeLooperMessage;
  sLooperSupervisionCallBacks->endLooperMessage = LooperSuperviser::endNativeLooperMessage;
  sLooperSupervisionCallBacks->enableLooperMonitor = LooperSuperviser::enableLooperMonitor;
  sLooperSupervisionCallBacks->isLooperMonitorEnabled = LooperSuperviser::isLooperMonitorEnabled;
  setLooperCallBacksInEvn(sLooperSupervisionCallBacks);

  jclass clazz = FindClassOrDie(env, kLooperCheckPointDetectorPathName);
  gLooperCheckPointDetectorOffsets.mClass = MakeGlobalRefOrDie(env, clazz);
  gLooperCheckPointDetectorOffsets.mBeginLoopOnce = GetStaticMethodIDOrDie(env, clazz, "beginLoopOnce", "(IJ)V");
  gLooperCheckPointDetectorOffsets.mEndLoopOnce = GetStaticMethodIDOrDie(env, clazz, "endLooperOnce", "()V");

  pthread_key_create(&sIsLooperMonitorEnabledKey, NULL);
}

} //namespace statistics
} //namespace os
} //namespace android
