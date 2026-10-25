#ifndef ANDROID_OS_STATISTICS_FREEJAVAOBJECTCACHE_H
#define ANDROID_OS_STATISTICS_FREEJAVAOBJECTCACHE_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <cutils/compiler.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "PILock.h"

namespace android {
namespace os {
namespace statistics {

class FreeJavaObjectCache {
private:
  class FreeJavaObjectCacheItem {
  public:
    void* mGlobalObject;
    FreeJavaObjectCacheItem* next;
  };
public:
  FreeJavaObjectCache(uint32_t cacheCapacity);
  virtual ~FreeJavaObjectCache();
  void init(JNIEnv* env, uint32_t initialCount);
  void extendTo(JNIEnv* env, uint32_t cacheCapacity);
  void destroy(JNIEnv* env);
  void* obtain(JNIEnv* env);
  void recycle(JNIEnv* env, void* globalFreeJavaObject);
protected:
  virtual void* createObject(JNIEnv* env) = 0;
  virtual void deleteObject(JNIEnv* env, void *pObject) = 0;
private:
  uint32_t mCacheCapacity;
  PILock* mLock;
  uint32_t mActiveCount;
  FreeJavaObjectCacheItem mActivityHead;
  uint32_t mIdleCount;
  FreeJavaObjectCacheItem mIdleHead;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_FREEJAVAOBJECTCACHE_H
