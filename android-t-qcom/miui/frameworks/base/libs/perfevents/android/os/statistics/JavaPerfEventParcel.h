#ifndef ANDROID_OS_STATISTICS_JAVAPERFEVENTPARCEL_H
#define ANDROID_OS_STATISTICS_JAVAPERFEVENTPARCEL_H

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <cutils/compiler.h>

#include "jni.h"

#include "JniParcelWriter.h"

namespace android {
namespace os {
namespace statistics {

class JavaPerfEventParcel {
public:
  JavaPerfEventParcel();
  void init(JNIEnv* env, jclass memberClazz, int32_t capacity);
  void reset(JNIEnv* env);
  void destroy(JNIEnv* env);
  void writeString(JNIEnv* env, char* str);
  void writeKernelBacktrace(JNIEnv* env, jlong *kernelBacktrace, uint32_t depth);
  void writeObject(JNIEnv* env, jobject value);
  jobject readObject(JNIEnv* env);
private:
  jobjectArray mObjects;
  int32_t mCapacity;
  int32_t mObjectCount;
  int32_t mReadPosOfObjects;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_JAVAPERFEVENTPARCEL_H
