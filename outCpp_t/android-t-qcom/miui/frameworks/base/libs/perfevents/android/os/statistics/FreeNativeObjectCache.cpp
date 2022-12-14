#define LOG_TAG "FreeNativeObjectCache"

#include <cutils/log.h>
#include <stdlib.h>
#include <cutils/compiler.h>

#include "FreeNativeObjectCache.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

FreeNativeObjectCache::FreeNativeObjectCache(uint32_t cacheCapacity) {
  mCacheCapacity = cacheCapacity;
  mLock = new PILock();
  mActiveCount = 0;
  mActivityHead.mGlobalObject = nullptr;
  mActivityHead.next = nullptr;
  mIdleCount = 0;
  mIdleHead.mGlobalObject = nullptr;
  mIdleHead.next = nullptr;
}

FreeNativeObjectCache::~FreeNativeObjectCache() {
}

void FreeNativeObjectCache::init(uint32_t initialCount) {
  mActiveCount = (initialCount < mCacheCapacity) ? initialCount : mCacheCapacity;
  for (uint32_t i = 0; i < mActiveCount; i++) {
    FreeNativeObjectCacheItem *item = new FreeNativeObjectCacheItem();
    item->mGlobalObject = createNewObject();
    item->next = mActivityHead.next;
    mActivityHead.next = item;
  }
}

void FreeNativeObjectCache::extendTo(uint32_t cacheCapacity) {
  mCacheCapacity = cacheCapacity;
}

void FreeNativeObjectCache::destroy() {
  FreeNativeObjectCacheItem* cur;

  while (true) {
    cur = mActivityHead.next;
    if (cur == nullptr) break;
    mActivityHead.next = cur->next;
    deleteObject(cur->mGlobalObject);
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

void* FreeNativeObjectCache::obtain() {
  void* pObject = nullptr;

  FreeNativeObjectCacheItem* item;
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
    pObject = createNewObject();
  }

  return pObject;
}

void FreeNativeObjectCache::recycle(void* pObject) {
  bool willPlaceInCache = true;
  FreeNativeObjectCacheItem* item = nullptr;
  mLock->lock();
  if (mActiveCount < mCacheCapacity) {
    if (mIdleCount > 0) {
      item = mIdleHead.next;
      mIdleHead.next = item->next;
      mIdleCount--;
      item->mGlobalObject = pObject;
      item->next = mActivityHead.next;
      mActivityHead.next = item;
      mActiveCount++;
    }
  } else {
    willPlaceInCache = false;
  }
  mLock->unlock();
  if (willPlaceInCache && item == nullptr) {
    item = new FreeNativeObjectCacheItem();
    item->mGlobalObject = pObject;
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
    deleteObject(pObject);
    if (item != nullptr) delete item;
  }
}

} //namespace statistics
} //namespace os
} //namespace android
