#ifndef ANDROID_OS_STATISTICS_HWBINDER_BINDERSUPERVISION_CALLBACKS_H_
#define ANDROID_OS_STATISTICS_HWBINDER_BINDERSUPERVISION_CALLBACKS_H_

#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>
#include <sstream>
#include <stdlib.h>
#include <utils/Log.h>
#include <hwbinder/Parcel.h>
#include <hwbinder/IBinder.h>

namespace Hw = android::hardware;

namespace android {
namespace os {
namespace statistics {

const static char *KEY_HWBINDER_CALLBACKS = "MIUI_HWBINDER_CALLBACKS";

struct HwBinderSupervisionCallBacks {
  bool (*isPerfSupervisionOn)();
  void (*reportHwBinderStarvation)(int32_t maxThreads, int64_t starvationTimeMs);
  int64_t (*beginHwBinderCall)(Hw::IBinder* binder, uint32_t code, uint32_t flags);
  void (*endHwBinderCall)(Hw::IBinder* binder, const Hw::Parcel& data, uint32_t code, uint32_t flags, int64_t beginUptimeMs);
  void (*beginHwBinderExecution)(Hw::IBinder* binder, uint32_t code, uint32_t flags);
  void (*endHwBinderExecution)(Hw::IBinder* binder, const Hw::Parcel& data, uint32_t code, uint32_t flags);
};

static void setHwBinderCallbacksInEvn(struct HwBinderSupervisionCallBacks *callbacks, bool overwrite = false) {
  if (callbacks == NULL) {
    return;
  }
  std::ostringstream oss;
  oss << (intptr_t) callbacks;
  if (setenv(KEY_HWBINDER_CALLBACKS, oss.str().c_str(), overwrite)) {
    ALOGE("init hwbinder callbacks failed!!!");
    return;
  }
}

} //namespace statistics
} //namespace os
} //namespace android

#endif  // ANDROID_OS_STATISTICS_HWBINDER_BINDERSUPERVISION_CALLBACKS_H_
