#ifndef ANDROID_OS_STATISTICS_JNIPARCELCACHE_H
#define ANDROID_OS_STATISTICS_JNIPARCELCACHE_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <atomic>
#include <string>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <cutils/compiler.h>
#include <cutils/atomic.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "FreeJavaObjectCache.h"

namespace android {
namespace os {
namespace statistics {

class JniParcelCache {
public:
  static void init(JNIEnv* env, uint32_t cacheCapacity, uint32_t initialCount);
  static jobject obtain(JNIEnv* env);
  static void reset(JNIEnv* env, jobject globalJniParcel);
  static void recycle(JNIEnv* env, jobject globalJniParcel);
private:
  static FreeJavaObjectCache *gCache;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_JNIPARCELCACHE_H
