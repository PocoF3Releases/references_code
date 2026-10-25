#define LOG_TAG "NativeBackTrace"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <stdatomic.h>
#include <utils/Log.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "OsUtils.h"
#include "PerfSuperviser.h"

using namespace android;
using namespace android::os::statistics;

static jint android_os_statistics_NativeBackTrace_nativeUpdateBacktraceMap(
  JNIEnv* env, jobject clazz) {
  return 0;
}

static void android_os_statistics_NativeBackTrace_nativeDispose(
  JNIEnv* env, jobject clazz, jlong nativePtr) {
}

static jobjectArray android_os_statistics_NativeBackTrace_nativeResolve(
  JNIEnv* env, jobject clazz, jlong nativePtr) {
  return nullptr;
}

/*
 * JNI registration.
 */
static const JNINativeMethod gMethods[] = {
  /* name, signature, funcPtr */
  { "nativeDispose",    "(J)V",
      (void*) android_os_statistics_NativeBackTrace_nativeDispose },
  { "nativeResolve",    "(J)[Ljava/lang/String;",
      (void*) android_os_statistics_NativeBackTrace_nativeResolve },
  { "nativeUpdateBacktraceMap",    "()I",
      (void*) android_os_statistics_NativeBackTrace_nativeUpdateBacktraceMap },
};

static const char* const kNativeBackTracePathName = "android/os/statistics/NativeBackTrace";

int register_android_os_statistics_NativeBackTrace(JNIEnv* env)
{
  return RegisterMethodsOrDie(env, kNativeBackTracePathName, gMethods, NELEM(gMethods));
}
