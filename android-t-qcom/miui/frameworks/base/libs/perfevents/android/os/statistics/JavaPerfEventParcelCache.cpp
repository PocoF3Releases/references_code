#define LOG_TAG "JavaPerfEventParcelCache"

#include <cutils/log.h>

#include "JavaPerfEventParcelCache.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

#define JAVAPERFEVENTPARCEL_OBJECT_MAX_COUNT 4

static const char* const kObjectPathName = "java/lang/Object";

static struct object_offset_t {
  jclass mObjectClass;
} gObjectOffsets;

class FreeJavaPerfEventParcelCache : public FreeJavaObjectCache {
public:
  FreeJavaPerfEventParcelCache(uint32_t cacheCapacity);
  virtual ~FreeJavaPerfEventParcelCache();
protected:
  virtual void* createObject(JNIEnv* env);
  virtual void deleteObject(JNIEnv* env, void *pObject);
};

FreeJavaPerfEventParcelCache::FreeJavaPerfEventParcelCache(uint32_t cacheCapacity)
  : FreeJavaObjectCache(cacheCapacity) {
}

FreeJavaPerfEventParcelCache::~FreeJavaPerfEventParcelCache() {
}

void* FreeJavaPerfEventParcelCache::createObject(JNIEnv* env) {
  //ALOGE("create new object");
  JavaPerfEventParcel* pObject = new JavaPerfEventParcel();
  pObject->init(env, gObjectOffsets.mObjectClass, JAVAPERFEVENTPARCEL_OBJECT_MAX_COUNT);
  return pObject;
}

void FreeJavaPerfEventParcelCache::deleteObject(JNIEnv* env, void *pObject) {
  ((JavaPerfEventParcel*)pObject)->destroy(env);
  delete (JavaPerfEventParcel*)pObject;
}

FreeJavaObjectCache * JavaPerfEventParcelCache::gCache = nullptr;

void JavaPerfEventParcelCache::init(JNIEnv* env, uint32_t cacheCapacity, uint32_t initialCount) {
  jclass clazz = FindClassOrDie(env, kObjectPathName);
  gObjectOffsets.mObjectClass = MakeGlobalRefOrDie(env, clazz);
  FreeJavaPerfEventParcelCache* pCache = new FreeJavaPerfEventParcelCache(cacheCapacity);
  pCache->init(env, initialCount);
  gCache = pCache;
}

void JavaPerfEventParcelCache::extendTo(JNIEnv* env, uint32_t cacheCapacity) {
  gCache->extendTo(env, cacheCapacity);
}

JavaPerfEventParcel* JavaPerfEventParcelCache::obtain(JNIEnv* env) {
  return (JavaPerfEventParcel*)gCache->obtain(env);
}

void JavaPerfEventParcelCache::recycle(JNIEnv* env, JavaPerfEventParcel* globalJavaPerfEventParcel) {
  if (CC_UNLIKELY(globalJavaPerfEventParcel == nullptr)) return;
  globalJavaPerfEventParcel->reset(env);
  gCache->recycle(env, (void *)globalJavaPerfEventParcel);
}

} //namespace statistics
} //namespace os
} //namespace android
