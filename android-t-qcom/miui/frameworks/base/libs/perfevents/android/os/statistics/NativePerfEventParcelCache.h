#ifndef ANDROID_OS_STATISTICS_NATIVEPERFEVENTPARCELCACHE_H
#define ANDROID_OS_STATISTICS_NATIVEPERFEVENTPARCELCACHE_H

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

#include "jni.h"

#include "FreeNativeObjectCache.h"
#include "NativePerfEventParcel.h"

namespace android {
namespace os {
namespace statistics {

class NativePerfEventParcelCache {
public:
  static void init(uint32_t cacheCapacity, uint32_t initialCount);
  static void extendTo(uint32_t cacheCapacity);
  static NativePerfEventParcel* obtain();
  static void recycle(JNIEnv *env, NativePerfEventParcel* globalNativePerfEventParcel);
private:
  static FreeNativeObjectCache *gCache;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_NATIVEPERFEVENTPARCELCACHE_H
