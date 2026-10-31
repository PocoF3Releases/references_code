#ifndef ANDROID_OS_STATISTICS_LOOPERSUPERVISER_H
#define ANDROID_OS_STATISTICS_LOOPERSUPERVISER_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <memory>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <utils/Looper.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

namespace android {
namespace os {
namespace statistics {

class LooperSuperviser {
public:
  static void init(JNIEnv* env);
  static void awakenedFromMessagePoll(Looper *looper);
  static void beginWaitForMessagePoll(Looper *looper);
  static void pausedMessagePoll(Looper *looper);
  static void beginNativeLooperMessage(Looper* lopper);
  static void endNativeLooperMessage(Looper* lopper, std::shared_ptr<std::string> *messageName);
  static void beginJavaLooperMessage(JNIEnv* env);
  static void endJavaLooperMessage(JNIEnv* env,
    jstring messageTarget, int32_t messageWhat, jstring messageCallback, int64_t planUptimeMillis);
  static void enableLooperMonitor();
  static bool isLooperMonitorEnabled();
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_LOOPERSUPERVISER_H
