#define LOG_TAG "BinderServerMonitor"
#include "BinderServerMonitor.h"

#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <inttypes.h>
#include <utils/Log.h>

#include "BinderThreadLocal.h"

namespace android {
namespace os {
namespace monitor {

pthread_mutex_t BinderServerMonitor::mapLock = PTHREAD_MUTEX_INITIALIZER;
std::unordered_map<pid_t, int64_t> BinderServerMonitor::binderClientCpuTime = {};

struct BinderSupervisionCallBacks* BinderServerMonitor::pBinderSupervisionCallBacksDelegate = nullptr;

static int64_t timevalToMs(struct timeval val) {
  return val.tv_sec * 1000 + val.tv_usec / 1000;
}

static int64_t getRunningTimeMs() {
  struct rusage resourceUsage;
  if (getrusage(RUSAGE_THREAD, &resourceUsage) == 0) {
    return timevalToMs(resourceUsage.ru_utime) + timevalToMs(resourceUsage.ru_stime);
  } else {
    return 0;
  }
  return 0;
}

static bool isPerfSupervisionOn() {
  return true;
}

static bool isCallBackDelegateAvailable() {
  return BinderServerMonitor::pBinderSupervisionCallBacksDelegate != nullptr &&
    BinderServerMonitor::pBinderSupervisionCallBacksDelegate->isPerfSupervisionOn();
}

void BinderServerMonitor::beginBinderExecution(IBinder* binder, uint32_t code, uint32_t flags) {
  if(isCallBackDelegateAvailable()) {
    BinderServerMonitor::pBinderSupervisionCallBacksDelegate->beginBinderExecution(binder, code, flags);
  }

  struct BinderThreadLocalInfo* pLocalData = BinderThreadLocal::get();
  pLocalData->depth++;
  if (pLocalData->depth > 1) {
    return;
  }
  int64_t beginRunningTime = getRunningTimeMs();
  pLocalData->beginExecutionTimeMills = beginRunningTime;
}

void BinderServerMonitor::endBinderExecution(IBinder* binder, uint32_t code, uint32_t flags) {
  if(isCallBackDelegateAvailable()) {
    BinderServerMonitor::pBinderSupervisionCallBacksDelegate->endBinderExecution(binder, code, flags);
  }

  struct BinderThreadLocalInfo* pLocalData = BinderThreadLocal::get();
  pLocalData->depth--;
  if (pLocalData->depth >= 1) {
    return;
  }
  int64_t callerPid = IPCThreadState::self()->getCallingPid();
  int64_t endRunningTime = getRunningTimeMs();
  int64_t binderCpuTime = endRunningTime - pLocalData->beginExecutionTimeMills;
  if(binderCpuTime != 0) {
    pthread_mutex_lock(&mapLock);
    binderClientCpuTime[callerPid] += binderCpuTime;
    pthread_mutex_unlock(&mapLock);
  }
}

void BinderServerMonitor::reportBinderStarvation(int32_t maxThreads, int64_t starvationTimeMs) {
  if(isCallBackDelegateAvailable()) {
    BinderServerMonitor::pBinderSupervisionCallBacksDelegate->reportBinderStarvation(maxThreads, starvationTimeMs);
  }
}

int64_t BinderServerMonitor::beginBinderCall(IBinder* binder, uint32_t code, uint32_t flags) {
  if(isCallBackDelegateAvailable()) {
    return BinderServerMonitor::pBinderSupervisionCallBacksDelegate->beginBinderCall(binder, code, flags);
  }
  return 0;
}

void BinderServerMonitor::endBinderCall(IBinder* binder, uint32_t code, uint32_t flags, int64_t beginUptimeMs) {
  if(isCallBackDelegateAvailable()) {
    BinderServerMonitor::pBinderSupervisionCallBacksDelegate->endBinderCall(binder, code, flags, beginUptimeMs);
  }
}

void BinderServerMonitor::init() {
  struct BinderSupervisionCallBacks *sBinderSupervisionCallBacks = (struct BinderSupervisionCallBacks*) malloc(sizeof(struct BinderSupervisionCallBacks));
  sBinderSupervisionCallBacks->isPerfSupervisionOn = isPerfSupervisionOn;
  sBinderSupervisionCallBacks->reportBinderStarvation = reportBinderStarvation;
  sBinderSupervisionCallBacks->beginBinderCall = beginBinderCall;
  sBinderSupervisionCallBacks->endBinderCall = endBinderCall;
  sBinderSupervisionCallBacks->beginBinderExecution = beginBinderExecution;
  sBinderSupervisionCallBacks->endBinderExecution = endBinderExecution;
  setBinderCallbacksInEvn(sBinderSupervisionCallBacks, true);
}

}  // namespace monitor
}  // namespace os
}  // namespace android