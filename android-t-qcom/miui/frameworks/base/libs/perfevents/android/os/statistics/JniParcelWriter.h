#ifndef ANDROID_OS_STATISTICS_JNIPARCELWRITER_H
#define ANDROID_OS_STATISTICS_JNIPARCELWRITER_H

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <string>
#include <utils/String16.h>

#include "jni.h"

#include "BinderSuperviser.h"

extern void writeLongsToJniParcel(JNIEnv* env, jobject jniParcel, int32_t count, jvalue* values);
extern void writeObjectsToJniParcel(JNIEnv* env, jobject jniParcel, int32_t count, jvalue* objects);

namespace android {
namespace os {
namespace statistics {

class JniParcelWriter {
public:
  JniParcelWriter() : mJniParcel(0), mCountOfLong(0), mCountOfObject(0) {
  }
  inline void beginWrite(JNIEnv* env, jobject jniParcel) {
    mJniParcel = jniParcel;
    mCountOfLong = 0;
    mCountOfObject = 0;
  }
  inline void writeLong(JNIEnv* env, jlong value) {
    mBufferLong[mCountOfLong].j = value;
    mCountOfLong++;
    if (mCountOfLong == 12) {
      batchWriteLongs(env);
    }
  }
  inline void writeString(JNIEnv* env, std::string* str) {
    jstring jstr = (str == nullptr) ? nullptr : env->NewStringUTF(str->c_str());
    internalWriteObject(env, jstr);
  }
  inline void writeString(JNIEnv* env, char* str) {
    jstring jstr = (str == nullptr) ? nullptr : env->NewStringUTF(str);
    internalWriteObject(env, jstr);
  }
  inline void writeBinder(JNIEnv* env, IBinder* binder) {
    jobject jbinder = nullptr;
    if (binder != nullptr) {
      jbinder = BinderSuperviser::wrapNativeBinder(env, binder);
    }
    internalWriteObject(env, jbinder);
  }
  inline void writeObject(JNIEnv* env, jobject object) {
    jobject localRef = (object == nullptr) ? nullptr : env->NewLocalRef(object);
    internalWriteObject(env, localRef);
  }
  inline void endWrite(JNIEnv* env) {
    batchWriteLongs(env);
    batchWriteObjects(env);
    mJniParcel = 0;
  }

private:
  inline void internalWriteObject(JNIEnv* env, jobject object) {
    mBufferObject[mCountOfObject].l = object;
    mCountOfObject++;
    if (mCountOfObject == 9) {
      batchWriteObjects(env);
    }
  }

private:
  inline void batchWriteLongs(JNIEnv *env) {
    if (mCountOfLong > 0) {
      writeLongsToJniParcel(env, mJniParcel, mCountOfLong, mBufferLong);
      mCountOfLong = 0;
    }
  }
  inline void batchWriteObjects(JNIEnv *env) {
    if (mCountOfObject > 0) {
      writeObjectsToJniParcel(env, mJniParcel, mCountOfObject, mBufferObject);
      for (int i = 0; i < mCountOfObject; i++) {
        if (mBufferObject[i].l != nullptr) {
          env->DeleteLocalRef(mBufferObject[i].l);
        }
      }
      mCountOfObject = 0;
    }
  }
private:
  jobject mJniParcel;

  int32_t mCountOfLong;
  jvalue mBufferLong[12];

  int32_t mCountOfObject;
  jvalue mBufferObject[9];
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_JNIPARCELWRITER_H
