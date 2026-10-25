#ifndef ANDROID_OS_STATISTICS_BINDERSUPERVISION_CALLBACKS_H_
#define ANDROID_OS_STATISTICS_BINDERSUPERVISION_CALLBACKS_H_

#include <sstream>
#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>
#include <binder/IBinder.h>
#include <utils/Log.h>

#define UNUSED __attribute__((__unused__))

namespace android {
namespace os {
namespace statistics {

const static char *KEY_BINDER_CALLBACKS = "MIUI_BINDER_CALLBACKS";

struct BinderSupervisionCallBacks {
  bool (*isPerfSupervisionOn)();
  void (*reportBinderStarvation)(int32_t maxThreads, int64_t starvationTimeMs);
  int64_t (*beginBinderCall)(IBinder* binder, uint32_t code, uint32_t flags);
  void (*endBinderCall)(IBinder* binder, uint32_t code, uint32_t flags, int64_t beginUptimeMs);
  void (*beginBinderExecution)(IBinder* binder, uint32_t code, uint32_t flags);
  void (*endBinderExecution)(IBinder* binder, uint32_t code, uint32_t flags);
};

UNUSED static void setBinderCallbacksInEvn(struct BinderSupervisionCallBacks *callbacks, bool overwrite) {
  if (callbacks == NULL) {
    return;
  }
  std::ostringstream oss;
  oss << (intptr_t) callbacks;
  if (setenv(KEY_BINDER_CALLBACKS, oss.str().c_str(), overwrite)) {
    ALOGE("init binder callbacks failed!!!");
    return;
  }
}

} //namespace statistics
} //namespace os
} //namespace android

#endif  // ANDROID_OS_STATISTICS_BINDERSUPERVISION_CALLBACKS_H_
