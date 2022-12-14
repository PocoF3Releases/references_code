#ifndef ANDROID_OS_STATISTICS_JAVAPERFEVENTPARCELCACHE_H
#define ANDROID_OS_STATISTICS_JAVAPERFEVENTPARCELCACHE_H

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
#include "JavaPerfEventParcel.h"

namespace android {
namespace os {
namespace statistics {

class JavaPerfEventParcelCache {
public:
  static void init(JNIEnv* env, uint32_t cacheCapacity, uint32_t initialCount);
  static void extendTo(JNIEnv* env, uint32_t cacheCapacity);
  static JavaPerfEventParcel* obtain(JNIEnv* env);
  static void recycle(JNIEnv* env, JavaPerfEventParcel* globalJavaPerfEventParcel);
private:
  static FreeJavaObjectCache *gCache;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_JAVAPERFEVENTPARCELCACHE_H
