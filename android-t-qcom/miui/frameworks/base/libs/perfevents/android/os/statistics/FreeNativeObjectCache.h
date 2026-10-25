#ifndef ANDROID_OS_STATISTICS_FREENATIVEOBJECTCACHE_H
#define ANDROID_OS_STATISTICS_FREENATIVEOBJECTCACHE_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <cutils/compiler.h>

#include "PILock.h"

namespace android {
namespace os {
namespace statistics {

class FreeNativeObjectCache {
private:
  class FreeNativeObjectCacheItem {
  public:
    void* mGlobalObject;
    FreeNativeObjectCacheItem* next;
  };
public:
  FreeNativeObjectCache(uint32_t cacheCapacity);
  virtual ~FreeNativeObjectCache();
  void init(uint32_t initialCount);
  void extendTo(uint32_t cacheCapacity);
  void destroy();
  void* obtain();
  void recycle(void* pObject);
protected:
  virtual void* createNewObject() = 0;
  virtual void deleteObject(void *pObject) = 0;
private:
  uint32_t mCacheCapacity;
  PILock* mLock;
  uint32_t mActiveCount;
  FreeNativeObjectCacheItem mActivityHead;
  uint32_t mIdleCount;
  FreeNativeObjectCacheItem mIdleHead;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_FREENATIVEOBJECTCACHE_H
