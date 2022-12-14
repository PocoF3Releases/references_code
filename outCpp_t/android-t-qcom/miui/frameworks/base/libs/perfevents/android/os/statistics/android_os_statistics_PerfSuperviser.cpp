#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <sstream>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "FastSystemInfoFetcher.h"
#include "JavaVMUtils.h"
#include "ThreadUtils.h"
#include "PerfSuperviser.h"
#include "ExecutingRootEvents.h"
#include "NativePerfEventParcelCache.h"
#include "JavaPerfEventParcelCache.h"
#include "JavaBackTraceCache.h"
#include "JniParcelCache.h"
#include "LooperSuperviser.h"
#include "BinderSuperviser.h"
#include "JavaVMSuperviser.h"
#include "PerfEventReporter.h"
#ifdef HWBINDER_MONITOR_ENABLED
#include "HwBinderSuperviser.h"
#endif
#include "KernelPerfEventsReader.h"
#include "PerfEventDetailsCache.h"
#include "ShortCStringCache.h"
#include "SmallMemoryBlockCache.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

const static char *KEY_CALLBACK_VERIFY_FLAG = "MIUI_CALLBACK_VERIFY_FLAG";

static void android_os_statistics_PerfSuperviser_nativeInit(
  JNIEnv* env, jobject clazz, jboolean isPrimaryZygote, jboolean hasSecondaryZygote,
  jint softWaitTimeThresholdInMillis, jint hardWaitTimeThresholdInMillis,
  jint minSoftWaitTimeThresholdInMillis, jint maxEventThresholdInMillis, jint divisionRatio) {
  FastSystemInfoFetcher::init(isPrimaryZygote, hasSecondaryZygote);
  ExecutingRootEvents::init();
  NativePerfEventParcelCache::init(600, 100);
  ShortCStringCache::init(600, 100);
  SmallMemoryBlockCache::init(600, 0);
  JavaPerfEventParcelCache::init(env, 600, 100);
  PerfEventDetailsCache::init(600, 100);
  JavaBackTraceCache::init(env, 200, 40);
  JniParcelCache::init(env, 2, 2);
  PerfEventBuffer::init(env, 300);
  PerfEventReporter::init(env);
  LooperSuperviser::init(env);
  BinderSuperviser::init(env);
  JavaVMSuperviser::init(env);
#ifdef HWBINDER_MONITOR_ENABLED
  HwBinderSuperviser::init();
#endif
  PerfSuperviser::init(env,
    softWaitTimeThresholdInMillis, hardWaitTimeThresholdInMillis,
    minSoftWaitTimeThresholdInMillis, maxEventThresholdInMillis, divisionRatio);
}

static void initVerifyFlags() {
  std::ostringstream oss;
  oss << getpid();
  if (setenv(KEY_CALLBACK_VERIFY_FLAG, oss.str().c_str(), true)) {
    ALOGE("init verify flags failed!!!");
    return;
  }
}

static void android_os_statistics_PerfSuperviser_nativeStart(
  JNIEnv* env, jobject clazz,
  jboolean isSystemServer, jboolean isMiuiDaemon, jint perfSupervisionMode) {
  PerfSuperviser::markIsSystemServer(isSystemServer);
  PerfSuperviser::markIsMiuiDaemon(isMiuiDaemon);
  if (isMiuiDaemon) {
    NativePerfEventParcelCache::extendTo(6000);
    ShortCStringCache::extendTo(6000);
    SmallMemoryBlockCache::extendTo(6000);
    JavaPerfEventParcelCache::extendTo(env, 6000);
    PerfEventDetailsCache::extendTo(6000);
  }
  if (isSystemServer) {
    NativePerfEventParcelCache::extendTo(1800);
    ShortCStringCache::extendTo(1800);
    JavaPerfEventParcelCache::extendTo(env, 1800);
    PerfEventDetailsCache::extendTo(1800);
    JavaBackTraceCache::extendTo(env, 400);
    PerfEventBuffer::extendTo(env, 500);
  }
  initVerifyFlags();
  PerfSuperviser::start(env, perfSupervisionMode);
  if (PerfSuperviser::isSupervisionOn()) {
    KernelPerfEventsReader::start(env);
  }
  if (PerfSuperviser::isPerfEventReportable()) {
    PerfEventReporter::start(env);
  }
}

static void android_os_statistics_PerfSuperviser_nativeUpdateProcessInfo(
  JNIEnv* env, jobject clazz,
  jstring processName, jstring packageName) {
  PerfSuperviser::updateProcessInfo(env, processName, packageName);
}

static void android_os_statistics_PerfSuperviser_setThreadPerfSupervisionOn(
  JNIEnv* env, jobject clazz,
  jboolean isOn) {
  ThreadUtils::setThreadPerfSupervisionOn(env, (bool)isOn);
}

/*
 * JNI registration.
 */
static const JNINativeMethod gMethods[] = {
  /* name, signature, funcPtr */
  { "nativeInit",    "(ZZIIIII)V",
      (void*) android_os_statistics_PerfSuperviser_nativeInit },
  { "nativeStart",    "(ZZI)V",
      (void*) android_os_statistics_PerfSuperviser_nativeStart },
  { "nativeUpdateProcessInfo",    "(Ljava/lang/String;Ljava/lang/String;)V",
      (void*) android_os_statistics_PerfSuperviser_nativeUpdateProcessInfo },
  { "setThreadPerfSupervisionOn",    "(Z)V",
      (void*) android_os_statistics_PerfSuperviser_setThreadPerfSupervisionOn },
};

const char* const kPerfSuperviserPathName = "android/os/statistics/PerfSuperviser";
extern int register_android_os_statistics_FilteringPerfEvent(JNIEnv* env);
extern int register_android_os_statistics_DeviceKernelPerfEvents(JNIEnv* env);

int register_android_os_statistics_PerfSuperviser(JNIEnv* env)
{
  register_android_os_statistics_FilteringPerfEvent(env);
  register_android_os_statistics_DeviceKernelPerfEvents(env);
  return RegisterMethodsOrDie(env, kPerfSuperviserPathName, gMethods, NELEM(gMethods));
}
