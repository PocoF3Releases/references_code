#ifndef ANDROID_OS_BINDER_SERVER_MONITOR_H
#define ANDROID_OS_BINDER_SERVER_MONITOR_H

#include <sys/types.h>
#include <string>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unordered_map>
#include <pthread.h>

#include "bindersupervision_callbacks.h"

#include "JNIHelp.h"
#include "core_jni_helpers.h"
#include "jni.h"

using namespace android::os::statistics;

namespace android {
namespace os {
namespace monitor {

class BinderServerMonitor {
public:
  static void init();
  // callbacks
  static void reportBinderStarvation(int32_t maxThreads, int64_t starvationTimeMs);
  static int64_t beginBinderCall(IBinder* binder, uint32_t code, uint32_t flags);
  static void endBinderCall(IBinder* binder, uint32_t code, uint32_t flags, int64_t beginUptimeMs);
  static void beginBinderExecution(IBinder* binder, uint32_t code, uint32_t flags);
  static void endBinderExecution(IBinder* binder, uint32_t code, uint32_t flags);

public:
  static pthread_mutex_t mapLock;
  static std::unordered_map<pid_t, int64_t> binderClientCpuTime;
  static struct BinderSupervisionCallBacks* pBinderSupervisionCallBacksDelegate;
};

}  // namespace monitor
}  // namespace os
}  // namespace android

#endif  // ANDROID_OS_BINDER_SERVER_MONITOR_H