#ifndef ANDROID_OS_STATISTICS_LOOPERSUPERVISION_CALLBACKS_H_
#define ANDROID_OS_STATISTICS_LOOPERSUPERVISION_CALLBACKS_H_

#include <inttypes.h>
#include <sstream>
#include <stdint.h>
#include <sys/types.h>
#include <memory>
#include <string>
#include <utils/Looper.h>
#include <utils/Log.h>
namespace android {
namespace os {
namespace statistics {

const static char *KEY_LOOPER_CALLBACKS = "MIUI_LOOPER_CALLBACKS";

struct LooperSupervisionCallBacks {
  bool (*isPerfSupervisionOn)();
  void (*awakenedFromMessagePoll)(Looper *looper);
  void (*beginWaitForMessagePoll)(Looper *looper);
  void (*pausedMessagePoll)(Looper *looper);
  void (*beginLooperMessage)(Looper *looper);
  void (*endLooperMessage)(Looper *looper, std::shared_ptr<std::string> *messageName);
  void (*enableLooperMonitor)();
  bool (*isLooperMonitorEnabled)();
};

static void setLooperCallBacksInEvn(struct LooperSupervisionCallBacks *callbacks) {
  if (callbacks == NULL) {
    return;
  }
  std::ostringstream oss;
  oss << (intptr_t) callbacks;
  if (setenv(KEY_LOOPER_CALLBACKS, oss.str().c_str(), false)) {
    ALOGE("init looper callbacks failed!!!");
    return;
  }
}

} //namespace statistics
} //namespace os
} //namespace android

#endif  // ANDROID_OS_STATISTICS_LOOPERSUPERVISION_CALLBACKS_H_
