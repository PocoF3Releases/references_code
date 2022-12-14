#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "LooperSuperviser.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

static void android_os_statistics_LooperMessageSuperviser_nativeBeginLooperMessage(
  JNIEnv* env, jobject clazz) {
  LooperSuperviser::beginJavaLooperMessage(env);
}

static void android_os_statistics_LooperMessageSuperviser_nativeEndLooperMessage(
  JNIEnv* env, jobject clazz,
  jstring messageTarget, jint messageWhat, jstring messageCallback, jlong planUptimeMillis)
{
  LooperSuperviser::endJavaLooperMessage(env,
    messageTarget, messageWhat, messageCallback, planUptimeMillis);
}

/*
 * JNI registration.
 */
static const JNINativeMethod gMethods[] = {
  /* name, signature, funcPtr */
  { "nativeBeginLooperMessage",    "()V",
      (void*) android_os_statistics_LooperMessageSuperviser_nativeBeginLooperMessage },
  { "nativeEndLooperMessage",    "(Ljava/lang/String;ILjava/lang/String;J)V",
      (void*) android_os_statistics_LooperMessageSuperviser_nativeEndLooperMessage },
};

static const char* const kLooperMessageSuperviserPathName = "android/os/statistics/LooperMessageSuperviser";

int register_android_os_statistics_LooperMessageSuperviser(JNIEnv* env)
{
  return RegisterMethodsOrDie(env, kLooperMessageSuperviserPathName, gMethods, NELEM(gMethods));
}
