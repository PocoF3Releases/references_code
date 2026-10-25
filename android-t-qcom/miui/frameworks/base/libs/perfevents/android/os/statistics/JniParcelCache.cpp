#define LOG_TAG "JniParcelCache"

#include <cutils/log.h>

#include "JniParcelCache.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

#define JNIPARCEL_NUMBER_MAX_COUNT 32
#define JNIPARCEL_OBJECT_MAX_COUNT 16

const char* const kJniParcelPathName = "android/os/statistics/JniParcel";

static struct jniparcel_offset_t {
  jclass mJniParcelClass;
  jmethodID mJniParcelConstructor;
  jmethodID mReset;
} gJniParcelOffsets;

class FreeJniParcelCache : public FreeJavaObjectCache {
public:
  FreeJniParcelCache(uint32_t cacheCapacity);
  virtual ~FreeJniParcelCache();
protected:
  virtual void* createObject(JNIEnv* env);
  virtual void deleteObject(JNIEnv* env, void *pObject);
};

FreeJniParcelCache::FreeJniParcelCache(uint32_t cacheCapacity)
  :FreeJavaObjectCache(cacheCapacity) {
}

FreeJniParcelCache::~FreeJniParcelCache() {
}

void* FreeJniParcelCache::createObject(JNIEnv* env) {
  //ALOGE("create new object");
  jobject temp = env->NewObject(gJniParcelOffsets.mJniParcelClass, gJniParcelOffsets.mJniParcelConstructor,
    (jint)JNIPARCEL_NUMBER_MAX_COUNT, (jint)JNIPARCEL_OBJECT_MAX_COUNT);
  jobject globalFreeJavaObject = env->NewGlobalRef(temp);
  env->DeleteLocalRef(temp);
  return globalFreeJavaObject;
}

void FreeJniParcelCache::deleteObject(JNIEnv* env, void *pObject) {
  env->DeleteGlobalRef((jobject)pObject);
}

FreeJavaObjectCache* JniParcelCache::gCache = nullptr;

void JniParcelCache::init(JNIEnv* env, uint32_t cacheCapacity, uint32_t initialCount) {
  jclass clazz = FindClassOrDie(env, kJniParcelPathName);
  gJniParcelOffsets.mJniParcelClass = MakeGlobalRefOrDie(env, clazz);
  gJniParcelOffsets.mJniParcelConstructor = GetMethodIDOrDie(env, clazz, "<init>", "(II)V");
  gJniParcelOffsets.mReset = GetMethodIDOrDie(env, clazz, "reset", "()V");
  FreeJniParcelCache* pCache = new FreeJniParcelCache(cacheCapacity);
  pCache->init(env, initialCount);
  gCache = pCache;
}

jobject JniParcelCache::obtain(JNIEnv* env) {
  return (jobject)gCache->obtain(env);
}

void JniParcelCache::reset(JNIEnv* env, jobject globalJniParcel) {
  if (CC_UNLIKELY(globalJniParcel == nullptr)) return;
  env->CallVoidMethod(globalJniParcel, gJniParcelOffsets.mReset);
}

void JniParcelCache::recycle(JNIEnv* env, jobject globalJniParcel) {
  if (CC_UNLIKELY(globalJniParcel == nullptr)) return;
  env->CallVoidMethod(globalJniParcel, gJniParcelOffsets.mReset);
  gCache->recycle(env, (void *)globalJniParcel);
}

} //namespace statistics
} //namespace os
} //namespace android
