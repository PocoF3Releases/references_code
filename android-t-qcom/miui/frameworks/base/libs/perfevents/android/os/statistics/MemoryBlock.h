#ifndef ANDROID_OS_STATISTICS_MEMORYBLOCK_H
#define ANDROID_OS_STATISTICS_MEMORYBLOCK_H

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
 #include <stdlib.h>
#include <cutils/compiler.h>

namespace android {
namespace os {
namespace statistics {

class MemoryBlock {
public:
  MemoryBlock(int16_t capacity) {
    mNext = nullptr;
    mSize = 0;
    mCapacity = capacity;
    mData = malloc(mCapacity);
  }
  ~MemoryBlock() {
    free(mData);
  }
public:
  MemoryBlock* mNext;
  int16_t mSize;
  int16_t mCapacity;
  void* mData;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_MEMORYBLOCK_H
