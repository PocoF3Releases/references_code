
#define LOG_TAG "PerfEventDetailsCache"

#include <cutils/log.h>

#include "PerfEventDetailsCache.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

class FreePerfEventDetailsCache : public FreeNativeObjectCache {
public:
  FreePerfEventDetailsCache(uint32_t cacheCapacity);
  virtual ~FreePerfEventDetailsCache();
protected:
  virtual void* createNewObject();
  virtual void deleteObject(void *pObject);
};

FreePerfEventDetailsCache::FreePerfEventDetailsCache(uint32_t cacheCapacity)
  : FreeNativeObjectCache(cacheCapacity) {
}

FreePerfEventDetailsCache::~FreePerfEventDetailsCache() {
}

void* FreePerfEventDetailsCache::createNewObject() {
  //ALOGE("create new object");
  return new PerfEventDetails();
}

void FreePerfEventDetailsCache::deleteObject(void *pObject) {
  delete (PerfEventDetails *)pObject;
}

FreeNativeObjectCache * PerfEventDetailsCache::gCache = nullptr;

void PerfEventDetailsCache::init(uint32_t cacheCapacity, uint32_t initialCount) {
  FreePerfEventDetailsCache* pCache = new FreePerfEventDetailsCache(cacheCapacity);
  pCache->init(initialCount);
  gCache = pCache;
}

void PerfEventDetailsCache::extendTo(uint32_t cacheCapacity) {
  gCache->extendTo(cacheCapacity);
}

PerfEventDetails* PerfEventDetailsCache::obtain() {
  return (PerfEventDetails*)gCache->obtain();
}

void PerfEventDetailsCache::recycle(PerfEventDetails* globalPerfEventDetails) {
  if (CC_UNLIKELY(globalPerfEventDetails == nullptr)) return;
  gCache->recycle(globalPerfEventDetails);
}

} //namespace statistics
} //namespace os
} //namespace android
