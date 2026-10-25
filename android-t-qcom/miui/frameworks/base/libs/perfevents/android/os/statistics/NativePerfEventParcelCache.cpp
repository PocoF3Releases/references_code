#define LOG_TAG "NativePerfEventParcelCache"

#include <cutils/log.h>


#include "NativePerfEventParcelCache.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

class FreeNativePerfEventParcelCache : public FreeNativeObjectCache {
public:
  FreeNativePerfEventParcelCache(uint32_t cacheCapacity);
  virtual ~FreeNativePerfEventParcelCache();
protected:
  virtual void* createNewObject();
  virtual void deleteObject(void *pObject);
};

FreeNativePerfEventParcelCache::FreeNativePerfEventParcelCache(uint32_t cacheCapacity)
  : FreeNativeObjectCache(cacheCapacity) {
}

FreeNativePerfEventParcelCache::~FreeNativePerfEventParcelCache() {
}

void* FreeNativePerfEventParcelCache::createNewObject() {
  //ALOGE("create new object");
  return new NativePerfEventParcel();
}

void FreeNativePerfEventParcelCache::deleteObject(void *pObject) {
  delete (NativePerfEventParcel *)pObject;
}

FreeNativeObjectCache * NativePerfEventParcelCache::gCache = nullptr;

void NativePerfEventParcelCache::init(uint32_t cacheCapacity, uint32_t initialCount) {
  FreeNativePerfEventParcelCache* pCache = new FreeNativePerfEventParcelCache(cacheCapacity);
  pCache->init(initialCount);
  gCache = pCache;
}

void NativePerfEventParcelCache::extendTo(uint32_t cacheCapacity) {
  gCache->extendTo(cacheCapacity);
}

NativePerfEventParcel* NativePerfEventParcelCache::obtain() {
  return (NativePerfEventParcel*)gCache->obtain();
}

void NativePerfEventParcelCache::recycle(JNIEnv *env, NativePerfEventParcel* globalNativePerfEventParcel) {
  if (CC_UNLIKELY(globalNativePerfEventParcel == nullptr)) return;
  globalNativePerfEventParcel->reset(env);
  gCache->recycle(globalNativePerfEventParcel);
}

} //namespace statistics
} //namespace os
} //namespace android
