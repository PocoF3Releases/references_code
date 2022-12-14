#ifndef ANDROID_OS_STATISTICS_PERFSUPERVISER_H
#define ANDROID_OS_STATISTICS_PERFSUPERVISER_H

#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <atomic>
#include <string>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <cutils/compiler.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

namespace android {
namespace os {
namespace statistics {

#define DEBUG_MIUI_PERFSUPERVISER 0

#define PERF_SUPERVISION_OFF 0
#define PERF_SUPERVISION_ON_NORMAL 1
#define PERF_SUPERVISION_ON_HEAVY 2
#define PERF_SUPERVISION_ON_TEST 9

class PerfSuperviser {
public:
  static void init(JNIEnv* env,
    uint32_t softWaitTimeThresholdInMillis,
    uint32_t hardWaitTimeThresholdInMillis,
    uint32_t minSoftWaitTimeThresholdInMillis,
    uint32_t maxEventThresholdInMillis,
    uint32_t divisionRatio);

  inline static void markIsSystemServer(bool isSystemServer) {
    sIsSystemServer = isSystemServer;
  }

  inline static void markIsMiuiDaemon(bool isMiuiDaemon) {
    sIsMiuiDaemon = isMiuiDaemon;
  }

  inline static void start(JNIEnv* env, uint32_t perfSupervisionMode) {
    sMyPid = getpid();
    sHasKernelPerfEvents = (access("/dev/proc_kperfevents", F_OK) == 0);
    sMode = (int32_t)perfSupervisionMode;
    sStarted = true;
  }

  inline static void updateProcessInfo(JNIEnv* env, jstring processName, jstring packageName) {
    sProcessName = (jstring)env->NewGlobalRef(processName);
    sPackageName = (jstring)env->NewGlobalRef(packageName);
  }

  inline static JavaVM* getJavaVM() {
    return (JavaVM*)sVM;
  }

  inline static JNIEnv* getJNIEnv() {
    JavaVM* vm = getJavaVM();
    JNIEnv* env = nullptr;
    if (vm == nullptr || vm->GetEnv((void **)&env, JNI_VERSION_1_4) < 0) {
      return nullptr;;
    }
    return env;
  }

  inline static int32_t myPid() {
    return sMyPid;
  }

  inline static bool isSystemServer() {
    return sIsSystemServer;
  }

  inline static bool isMiuiDaemon() {
    return sIsMiuiDaemon;
  }

  inline static bool isSupervisionOn() {
    return sStarted && sMode >= PERF_SUPERVISION_ON_NORMAL;
  }

  inline static bool isPerfEventReportable() {
    return !sIsMiuiDaemon && isSupervisionOn();
  }

  inline static bool hasKernelPerfEvents() {
    return sHasKernelPerfEvents;
  }

  inline static bool isInHeavyMode() {
    return sStarted && sMode >= PERF_SUPERVISION_ON_HEAVY;
  }

  inline static int32_t getMode() {
    return sStarted ? sMode : PERF_SUPERVISION_OFF;
  }

  inline static int32_t getSoftThresholdMs() {
    return sSoftWaitTimeThresholdMs;
  }

  inline static int32_t getHardThresholdMs() {
    return sHardWaitTimeThresholdMs;
  }

  inline static int32_t getMinSoftThresholdMs() {
    return sMinSoftWaitTimeThresholdMs;
  }

  inline static int32_t getDivisionRatio() {
    return sDivisionRatio;
  }

  inline static int32_t getInnerWaitThresholdMs() {
    return sInnerWaitTimeThresholdMs;
  }

  inline static int32_t getsMaxEventThresholdMs() {
    return sMaxEventThresholdMs;
  }

  inline static int32_t getMinOverlappedMs() {
    return sMinOverlappedMs;
  }

  inline static jstring getProcessName() {
    return sProcessName;
  }

  inline static jstring getPackageName() {
    return sPackageName;
  }

private:
  static volatile JavaVM* sVM;
  static volatile int32_t sMyPid;
  static volatile bool sIsSystemServer;
  static volatile bool sIsMiuiDaemon;
  static volatile bool sStarted;
  static volatile bool sHasKernelPerfEvents;
  static volatile int32_t sMode;
  static volatile int32_t sSoftWaitTimeThresholdMs;
  static volatile int32_t sHardWaitTimeThresholdMs;
  static volatile int32_t sMinSoftWaitTimeThresholdMs;
  static volatile int32_t sDivisionRatio;
  static volatile int32_t sInnerWaitTimeThresholdMs;
  static volatile int32_t sMaxEventThresholdMs;
  static volatile int32_t sMinOverlappedMs;
  static volatile jstring sProcessName;
  static volatile jstring sPackageName;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_PERFSUPERVISER_H
