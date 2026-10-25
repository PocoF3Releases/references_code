#include "JavaPerfEventParcel.h"


namespace android {
namespace os {
namespace statistics {

JavaPerfEventParcel::JavaPerfEventParcel() {
  mObjects = nullptr;
  mObjectCount = 0;
  mReadPosOfObjects = 0;
}

void JavaPerfEventParcel::init(JNIEnv* env, jclass memberClazz, int32_t capacity) {
  mCapacity = capacity;
  jobjectArray temp = env->NewObjectArray(mCapacity, memberClazz, nullptr);
  mObjects = (jobjectArray)env->NewGlobalRef(temp);
  env->DeleteLocalRef(temp);
}

void JavaPerfEventParcel::reset(JNIEnv* env) {
  for (int i = 0; i < mObjectCount; i++) {
    env->SetObjectArrayElement(mObjects, i, nullptr);
  }
  mObjectCount = 0;
  mReadPosOfObjects = 0;
}

void JavaPerfEventParcel::destroy(JNIEnv* env) {
  reset(env);
  env->DeleteGlobalRef(mObjects);
  mObjects = nullptr;
}

void JavaPerfEventParcel::writeString(JNIEnv* env, char* str) {
  if (CC_LIKELY(mObjectCount < mCapacity)) {
    jstring jstr = (str == nullptr) ? nullptr : env->NewStringUTF(str);
    env->SetObjectArrayElement(mObjects, mObjectCount, jstr);
    mObjectCount++;
  }
}

void JavaPerfEventParcel::writeKernelBacktrace(JNIEnv* env, jlong *kernelBacktrace, uint32_t depth) {
  if (CC_LIKELY(mObjectCount < mCapacity)) {
    jlongArray jKernelBacktrace = env->NewLongArray(depth);
    if (jKernelBacktrace != NULL) {
      env->SetLongArrayRegion(jKernelBacktrace, 0, depth, kernelBacktrace);
    }
    env->SetObjectArrayElement(mObjects, mObjectCount, jKernelBacktrace);
    mObjectCount++;
  }
}

void JavaPerfEventParcel::writeObject(JNIEnv* env, jobject value) {
  if (CC_LIKELY(mObjectCount < mCapacity)) {
    env->SetObjectArrayElement(mObjects, mObjectCount, value);
    mObjectCount++;
  }
}

jobject JavaPerfEventParcel::readObject(JNIEnv* env) {
  if (mReadPosOfObjects < mObjectCount) {
    jobject value = env->GetObjectArrayElement(mObjects, mReadPosOfObjects);
    env->SetObjectArrayElement(mObjects, mReadPosOfObjects, nullptr);
    mReadPosOfObjects++;
    return value;
  } else {
    return nullptr;
  }
}

} //namespace statistics
} //namespace os
} //namespace android
