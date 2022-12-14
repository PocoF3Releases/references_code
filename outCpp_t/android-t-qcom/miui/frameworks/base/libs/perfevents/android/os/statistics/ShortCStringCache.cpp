
#define LOG_TAG "ShortCStringCache"

#include <cutils/log.h>

#include "ShortCStringCache.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

class FreeShortCStringCache : public FreeNativeObjectCache {
public:
  FreeShortCStringCache(uint32_t cacheCapacity);
  virtual ~FreeShortCStringCache();
protected:
  virtual void* createNewObject();
  virtual void deleteObject(void *pObject);
};

FreeShortCStringCache::FreeShortCStringCache(uint32_t cacheCapacity)
  : FreeNativeObjectCache(cacheCapacity) {
}

FreeShortCStringCache::~FreeShortCStringCache() {
}

void* FreeShortCStringCache::createNewObject() {
  //ALOGE("create new object");
  return new ShortCString();
}

void FreeShortCStringCache::deleteObject(void *pObject) {
  delete (ShortCString *)pObject;
}

FreeNativeObjectCache * ShortCStringCache::gCache = nullptr;

void ShortCStringCache::init(uint32_t cacheCapacity, uint32_t initialCount) {
  FreeShortCStringCache* pCache = new FreeShortCStringCache(cacheCapacity);
  pCache->init(initialCount);
  gCache = pCache;
}

void ShortCStringCache::extendTo(uint32_t cacheCapacity) {
  gCache->extendTo(cacheCapacity);
}

ShortCString* ShortCStringCache::obtain() {
  return (ShortCString*)gCache->obtain();
}

void ShortCStringCache::recycle(ShortCString* globalShortCString) {
  if (CC_UNLIKELY(globalShortCString == nullptr)) return;
  gCache->recycle(globalShortCString);
}

} //namespace statistics
} //namespace os
} //namespace android
