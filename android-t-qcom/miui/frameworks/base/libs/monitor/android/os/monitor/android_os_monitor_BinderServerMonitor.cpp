#define LOG_TAG "BinderServerMonitor"

#include <android-base/logging.h>
#include <nativehelper/ScopedLocalRef.h>
#include <unordered_map>

#include "JNIHelp.h"
#include "core_jni_helpers.h"
#include "jni.h"

#include "BinderServerMonitor.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;
using namespace android::os::monitor;

static const char* kClassHashMap = "java/util/HashMap";

static jclass hashMapClz;
static jmethodID hashMapPutMethodID;
static jclass integerClazz;
static jmethodID integerInitMethodID;
static jclass longClazz;
static jmethodID longInitMethodID;

void android_os_monitor_BinderServerMonitor_dumpBinderClientCpuTimeUsed(
    JNIEnv* env, jobject clazz, jobject outMap) {
  std::unordered_map<pid_t, int64_t> binderCpuTimes;
  pthread_mutex_lock(&BinderServerMonitor::mapLock);
  BinderServerMonitor::binderClientCpuTime.swap(binderCpuTimes);
  pthread_mutex_unlock(&BinderServerMonitor::mapLock);

  std::unordered_map<pid_t, int64_t>::iterator iter = binderCpuTimes.begin();
  for (; iter != binderCpuTimes.end(); ++iter) {
    env->CallObjectMethod(outMap, hashMapPutMethodID,
    env->NewObject(integerClazz, integerInitMethodID, (int32_t)iter->first),
    env->NewObject(longClazz, longInitMethodID, iter->second));
  }
  binderCpuTimes.clear();
}

jlong android_os_monitor_BinderServerMonitor_clearSpecificBinderClientCpuTimeUsed(
    JNIEnv* env, jobject clazz, jint pid) {
  pthread_mutex_lock(&BinderServerMonitor::mapLock);
  jlong res = BinderServerMonitor::binderClientCpuTime[pid];
  if(res != 0) {
    BinderServerMonitor::binderClientCpuTime.erase(pid);
  }
  pthread_mutex_unlock(&BinderServerMonitor::mapLock);
  return res;
}

void android_os_monitor_BinderServerMonitor_nativeStart(JNIEnv *env, jobject clazz) {
  BinderServerMonitor::init();
}

/*
 * JNI registration.
 */
static const JNINativeMethod gMethods[] = {
    /* name, signature, funcPtr */
    {"dumpBinderClientCpuTimeUsed", "(Ljava/util/HashMap;)V",
     (void*) android_os_monitor_BinderServerMonitor_dumpBinderClientCpuTimeUsed},
    {"clearBinderClientCpuTimeUsed", "(I)J",
     (void*) android_os_monitor_BinderServerMonitor_clearSpecificBinderClientCpuTimeUsed},
    {"nativeStart", "()V",
     (void*) android_os_monitor_BinderServerMonitor_nativeStart},
};

static const char* const kBinderServerMonitorPathName =
    "android/os/statistics/BinderServerMonitor";

int register_android_os_monitor_BinderServerMonitor(JNIEnv* env) {
  hashMapClz = env->FindClass(kClassHashMap);
  hashMapClz = reinterpret_cast<jclass>(env->NewGlobalRef(hashMapClz));
  CHECK(hashMapClz != NULL);
  hashMapPutMethodID = env->GetMethodID(hashMapClz, "put",
      "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
  CHECK(hashMapPutMethodID != NULL);

  integerClazz = env->FindClass("java/lang/Integer");
  integerClazz = reinterpret_cast<jclass>(env->NewGlobalRef(integerClazz));
  CHECK(integerClazz != NULL);
  integerInitMethodID = env->GetMethodID(integerClazz, "<init>", "(I)V");
  CHECK(integerInitMethodID != NULL);

  longClazz = env->FindClass("java/lang/Long");
  longClazz = reinterpret_cast<jclass>(env->NewGlobalRef(longClazz));
  CHECK(longClazz != NULL);
  longInitMethodID = env->GetMethodID(longClazz, "<init>", "(J)V");
  CHECK(longInitMethodID != NULL);

  return RegisterMethodsOrDie(env, kBinderServerMonitorPathName, gMethods, NELEM(gMethods));
}
