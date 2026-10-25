#ifndef ANDROID_OS_STATISTICS_SHORTCSTRINGCACHE_H
#define ANDROID_OS_STATISTICS_SHORTCSTRINGCACHE_H

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
#include "ShortCString.h"

namespace android {
namespace os {
namespace statistics {

class ShortCStringCache {
public:
  static void init(uint32_t cacheCapacity, uint32_t initialCount);
  static void extendTo(uint32_t cacheCapacity);
  static ShortCString* obtain();
  static void recycle(ShortCString* globalShortCString);
private:
  static FreeNativeObjectCache *gCache;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_SHORTCSTRINGCACHE_H
