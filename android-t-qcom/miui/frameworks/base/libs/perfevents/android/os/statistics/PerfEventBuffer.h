#ifndef ANDROID_OS_STATISTICS_PERFEVENTBUFFER_H
#define ANDROID_OS_STATISTICS_PERFEVENTBUFFER_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <cutils/compiler.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "NativePerfEventParcel.h"
#include "PerfEvent.h"

namespace android {
namespace os {
namespace statistics {

class PerfEventBuffer {
public:
  static void init(JNIEnv* env, uint32_t capacity);
  static void extendTo(JNIEnv* env, uint32_t capacity);
  static void addPerfEvent(JNIEnv* env, int32_t eventType, int64_t eventSeq, jobject perfEvent);
  static void addPerfEvent(JNIEnv* env, PerfEvent& perfEvent);
  static uint32_t capacity();
  static uint32_t count();
  static int32_t fetch(JNIEnv* env, jobjectArray fetchingBuffer);
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_PERFEVENTBUFFER_H
