#ifndef ANDROID_OS_STATISTICS_THREADUTILS_H
#define ANDROID_OS_STATISTICS_THREADUTILS_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <cutils/compiler.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

namespace android {
namespace os {
namespace statistics {

class ThreadUtils {
public:
  static void setThreadPerfSupervisionOn(bool isOn);
  static void setThreadPerfSupervisionOn(JNIEnv* env, bool isOn);
  static bool isThreadPerfSupervisionOn();
  static bool isThreadPerfSupervisionOn(JNIEnv* env);
  static void getThreadInfo(int32_t& thread_id, std::shared_ptr<std::string>& thread_name);
  static void getThreadInfo(JNIEnv* env, int32_t& thread_id, std::shared_ptr<std::string>& thread_name);
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_THREADUTILS_H
