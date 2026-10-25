#define LOG_TAG "FreeJavaObjectCache"

#include <cutils/log.h>
#include <stdlib.h>
#include <string.h>
#include <cutils/compiler.h>

#include "FreeJavaObjectCache.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

FreeJavaObjectCache::FreeJavaObjectCache(uint32_t cacheCapacity) {
  mCacheCapacity = cacheCapacity;
  mLock = new PILock();
  mActiveCount = 0;
  mActivityHead.mGlobalObject = nullptr;
  mActivityHead.next = nullptr;
  mIdleCount = 0;
  mIdleHead.mGlobalObject = nullptr;
  mIdleHead.next = nullptr;
}

FreeJavaObjectCache::~FreeJavaObjectCache() {
  delete mLock;
}

void FreeJavaObjectCache::init(JNIEnv* env, uint32_t initialCount) {
  mActiveCount = (initialCount < mCacheCapacity) ? initialCount : mCacheCapacity;
  for (uint32_t i = 0; i < mActiveCount; i++) {
    FreeJavaObjectCacheItem *item = new FreeJavaObjectCacheItem();
    item->mGlobalObject = createObject(env);
    item->next = mActivityHead.next;
    mActivityHead.next = item;
  }
}

void FreeJavaObjectCache::extendTo(JNIEnv* env, uint32_t cacheCapacity) {
  mCacheCapacity = cacheCapacity;
}

void FreeJavaObjectCache::destroy(JNIEnv* env) {
  FreeJavaObjectCacheItem* cur;

  while (true) {
    cur = mActivityHead.next;
    if (cur == nullptr) break;
    mActivityHead.next = cur->next;
    deleteObject(env, cur->mGlobalObject);
    delete cur;
  }
  mActiveCount = 0;

  while (true) {
    cur = mIdleHead.next;
    if (cur == nullptr) break;
    mIdleHead.next = cur->next;
    delete cur;
  }
  mIdleCount = 0;
}

void* FreeJavaObjectCache::obtain(JNIEnv* env) {
  void* pObject = nullptr;

  FreeJavaObjectCacheItem* item;
  mLock->lock();
  if (mActiveCount != 0) {
    item = mActivityHead.next;
    mActivityHead.next = item->next;
    mActiveCount--;
    pObject = item->mGlobalObject;
    item->mGlobalObject = nullptr;
    item->next = mIdleHead.next;
    mIdleHead.next = item;
    mIdleCount++;
  }
  mLock->unlock();

  if (pObject == nullptr) {
    pObject = createObject(env);
  }

  return pObject;
}

void FreeJavaObjectCache::recycle(JNIEnv* env, void* globalFreeJavaObject) {
  bool willPlaceInCache = true;
  FreeJavaObjectCacheItem* item = nullptr;
  mLock->lock();
  if (mActiveCount < mCacheCapacity) {
    if (mIdleCount > 0) {
      item = mIdleHead.next;
      mIdleHead.next = item->next;
      mIdleCount--;
      item->mGlobalObject = globalFreeJavaObject;
      item->next = mActivityHead.next;
      mActivityHead.next = item;
      mActiveCount++;
    }
  } else {
    willPlaceInCache = false;
  }
  mLock->unlock();
  if (willPlaceInCache && item == nullptr) {
    item = new FreeJavaObjectCacheItem();
    item->mGlobalObject = globalFreeJavaObject;
    mLock->lock();
    if (mActiveCount < mCacheCapacity) {
      item->next = mActivityHead.next;
      mActivityHead.next = item;
      mActiveCount++;
    } else {
      willPlaceInCache = false;
    }
    mLock->unlock();
  }
  if (!willPlaceInCache) {
    //ALOGE("recycle to delete");
    deleteObject(env, globalFreeJavaObject);
    if (item != nullptr) delete item;
  }
}

} //namespace statistics
} //namespace os
} //namespace android
