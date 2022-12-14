#ifndef ANDROID_OS_STATISTICS_HWBINDERSUPERVISER_H
#define ANDROID_OS_STATISTICS_HWBINDERSUPERVISER_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <hwbinder/Parcel.h>
#include <hwbinder/IBinder.h>
#include <hwbinder/IPCThreadState.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

namespace Hw = android::hardware;

namespace android {
namespace os {
namespace statistics {

class HwBinderSuperviser {
public:
  static void init();
  static void reportBinderStarvation(int32_t maxThreads, int64_t starvationTimeMs);
  static int64_t beginBinderCall(Hw::IBinder* binder, uint32_t code, uint32_t flags);
  static void endBinderCall(Hw::IBinder* binder, const Hw::Parcel& parcel, uint32_t code, uint32_t flags, int64_t beginUptimeMs);
  static void beginBinderExecution(Hw::IBinder* binder, uint32_t code, uint32_t flags);
  static void endBinderExecution(Hw::IBinder* binder, const Hw::Parcel& parcel, uint32_t code, uint32_t flags);
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_HWBINDERSUPERVISER_H
