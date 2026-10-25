#ifndef ANDROID_OS_STATISTICS_BINDERSUPERVISER_H
#define ANDROID_OS_STATISTICS_BINDERSUPERVISER_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"
#ifdef MIUI_BINDER_MONITOR
#include "BinderServerMonitor.h"
#endif

namespace android {
namespace os {
namespace statistics {

class BinderSuperviser {
public:
  static void init(JNIEnv* env);
  static void reportBinderStarvation(int32_t maxThreads, int64_t starvationTimeMs);
  static int64_t beginBinderCall(IBinder* binder, uint32_t code, uint32_t flags);
  static void endBinderCall(IBinder* binder, uint32_t code, uint32_t flags, int64_t beginUptimeMs);
  static void beginBinderExecution(IBinder* binder, uint32_t code, uint32_t flags);
  static void endBinderExecution(IBinder* binder, uint32_t code, uint32_t flags);

  static jobject wrapNativeBinder(JNIEnv* env, IBinder* binder);
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_BINDERSUPERVISER_H
