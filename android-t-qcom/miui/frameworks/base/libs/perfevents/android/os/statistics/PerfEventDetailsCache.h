#ifndef ANDROID_OS_STATISTICS_PERFEVENTDETAILSCACHE_H
#define ANDROID_OS_STATISTICS_PERFEVENTDETAILSCACHE_H

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

#include "FreeNativeObjectCache.h"
#include "PerfEvent.h"

namespace android {
namespace os {
namespace statistics {

class PerfEventDetailsCache {
public:
  static void init(uint32_t cacheCapacity, uint32_t initialCount);
  static void extendTo(uint32_t cacheCapacity);
  static PerfEventDetails* obtain();
  static void recycle(PerfEventDetails* globalPerfEventDetails);
private:
  static FreeNativeObjectCache *gCache;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_PERFEVENTDETAILSCACHE_H
