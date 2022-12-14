#ifndef ANDROID_OS_STATISTICS_SMALLMEMORYBLOCKCACHE_H
#define ANDROID_OS_STATISTICS_SMALLMEMORYBLOCKCACHE_H

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
#include "MemoryBlock.h"

namespace android {
namespace os {
namespace statistics {

//必须8字节对齐，因为会与Parcel配合使用
#define SMALL_MEMORY_BLOCK_SIZE 64

class SmallMemoryBlockCache {
public:
  static void init(uint32_t cacheCapacity, uint32_t initialCount);
  static void extendTo(uint32_t cacheCapacity);
  static MemoryBlock* obtain();
  static void recycle(MemoryBlock* globalSmallMemoryBlock);
private:
  static FreeNativeObjectCache *gCache;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_SMALLMEMORYBLOCKCACHE_H
