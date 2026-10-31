
#define LOG_TAG "SmallMemoryBlockCache"

#include <cutils/log.h>

#include "SmallMemoryBlockCache.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

class FreeSmallMemoryBlockCache : public FreeNativeObjectCache {
public:
  FreeSmallMemoryBlockCache(uint32_t cacheCapacity);
  virtual ~FreeSmallMemoryBlockCache();
protected:
  virtual void* createNewObject();
  virtual void deleteObject(void *pObject);
};

FreeSmallMemoryBlockCache::FreeSmallMemoryBlockCache(uint32_t cacheCapacity)
  : FreeNativeObjectCache(cacheCapacity) {
}

FreeSmallMemoryBlockCache::~FreeSmallMemoryBlockCache() {
}

void* FreeSmallMemoryBlockCache::createNewObject() {
  //ALOGE("create new object");
  return new MemoryBlock(SMALL_MEMORY_BLOCK_SIZE);
}

void FreeSmallMemoryBlockCache::deleteObject(void *pObject) {
  delete (MemoryBlock *)pObject;
}

FreeNativeObjectCache * SmallMemoryBlockCache::gCache = nullptr;

void SmallMemoryBlockCache::init(uint32_t cacheCapacity, uint32_t initialCount) {
  FreeSmallMemoryBlockCache* pCache = new FreeSmallMemoryBlockCache(cacheCapacity);
  pCache->init(initialCount);
  gCache = pCache;
}

void SmallMemoryBlockCache::extendTo(uint32_t cacheCapacity) {
  gCache->extendTo(cacheCapacity);
}

MemoryBlock* SmallMemoryBlockCache::obtain() {
  return (MemoryBlock*)gCache->obtain();
}

void SmallMemoryBlockCache::recycle(MemoryBlock* globalSmallMemoryBlock) {
  if (CC_UNLIKELY(globalSmallMemoryBlock == nullptr)) return;
  gCache->recycle(globalSmallMemoryBlock);
}

} //namespace statistics
} //namespace os
} //namespace android
