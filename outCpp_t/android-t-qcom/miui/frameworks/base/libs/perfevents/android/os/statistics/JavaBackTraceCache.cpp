#define LOG_TAG "JavaBackTraceCache"

#include <cutils/log.h>

#include "JavaBackTraceCache.h"
#include "JavaVMUtils.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

class FreeJavaBackTraceCache : public FreeJavaObjectCache {
public:
  FreeJavaBackTraceCache(uint32_t cacheCapacity);
  virtual ~FreeJavaBackTraceCache();
protected:
  virtual void* createObject(JNIEnv* env);
  virtual void deleteObject(JNIEnv* env, void *pObject);
};

FreeJavaBackTraceCache::FreeJavaBackTraceCache(uint32_t cacheCapacity)
  : FreeJavaObjectCache(cacheCapacity) {
}

FreeJavaBackTraceCache::~FreeJavaBackTraceCache() {
}

void* FreeJavaBackTraceCache::createObject(JNIEnv* env) {
  //ALOGE("create new object");
  jobject temp = JavaVMUtils::createJavaStackBackTrace(env, false);
  jobject globalFreeJavaObject = env->NewGlobalRef(temp);
  env->DeleteLocalRef(temp);
  return (void *)globalFreeJavaObject;
}

void FreeJavaBackTraceCache::deleteObject(JNIEnv* env, void *pObject) {
  env->DeleteGlobalRef((jobject)pObject);
}

FreeJavaObjectCache * JavaBackTraceCache::gCache = nullptr;

void JavaBackTraceCache::init(JNIEnv* env, uint32_t cacheCapacity, uint32_t initialCount) {
  FreeJavaBackTraceCache* pCache = new FreeJavaBackTraceCache(cacheCapacity);
  pCache->init(env, initialCount);
  gCache = pCache;
}

void JavaBackTraceCache::extendTo(JNIEnv* env, uint32_t cacheCapacity) {
  gCache->extendTo(env, cacheCapacity);
}

jobject JavaBackTraceCache::obtain(JNIEnv* env) {
  return (jobject)gCache->obtain(env);
}

void JavaBackTraceCache::recycle(JNIEnv* env, jobject globalJavaBackTrace) {
  if (CC_UNLIKELY(globalJavaBackTrace == nullptr)) return;
  JavaVMUtils::resetJavaStackBackTrace(env, globalJavaBackTrace);
  gCache->recycle(env, (void *)globalJavaBackTrace);
}

} //namespace statistics
} //namespace os
} //namespace android
