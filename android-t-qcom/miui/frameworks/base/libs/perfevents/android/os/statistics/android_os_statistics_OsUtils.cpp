#define LOG_TAG "OsUtils"

// To make sure cpu_set_t is included from sched.h
#define _GNU_SOURCE 1
#include <utils/Log.h>
#include <cutils/sched_policy.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <private/android_filesystem_config.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"
#include "OsUtils.h"

#include "FastSystemInfoFetcher.h"
#include "OsUtils.h"

#define RUNNING_TINDEX    0
#define RUNNABLE_TINDEX   1
#define SCHED_STAT_NUMS   2

using namespace android;
using namespace android::os::statistics;

// For both of these, err should be in the errno range (positive), not a status_t (negative)

static void signalExceptionForPriorityError(JNIEnv* env, int err)
{
  switch (err) {
    case EINVAL:
      jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
      break;
    case ESRCH:
      jniThrowException(env, "java/lang/IllegalArgumentException", "Given thread does not exist");
      break;
    case EPERM:
      jniThrowException(env, "java/lang/SecurityException", "No permission to modify given thread");
      break;
    case EACCES:
      jniThrowException(env, "java/lang/SecurityException", "No permission to set to given priority");
      break;
    default:
      jniThrowException(env, "java/lang/RuntimeException", "Unknown error");
      break;
  }
}

static jint android_os_statistics_OsUtils_getThreadScheduler(JNIEnv* env, jclass clazz,
                        jint tid)
{
// linux has sched_setscheduler(), others don't.
#if defined(__linux__)
  int rc = sched_getscheduler(tid);
  if (rc < 0) {
    signalExceptionForPriorityError(env, errno);
  }
  return rc;
#else
  signalExceptionForPriorityError(env, ENOSYS);
  return -1;
#endif
}

static void android_os_statistics_OsUtils_getThreadSchedStat(JNIEnv* env, jclass, jlongArray jschedStat) {
  int fd = 0;
  int cnt = 0;
  char buf[84] = {'\0'};
  int64_t schedStat[SCHED_STAT_NUMS] = {0};

  if(env->GetArrayLength(jschedStat) >= SCHED_STAT_NUMS) {
    //schedStat index: 0, running time 1, runnable time
    sprintf(buf, "/proc/self/task/%d/schedstat", gettid());
    fd = open(buf, O_RDONLY);
    if (fd > 0) {
      cnt = read(fd, buf, sizeof(buf) - 1);
      close(fd);
      if (cnt > 0) {
      buf[cnt] = '\0';
      sscanf(buf, "%" PRId64" %" PRId64, schedStat + RUNNING_TINDEX, schedStat + RUNNABLE_TINDEX);
      env->SetLongArrayRegion(jschedStat, 0, SCHED_STAT_NUMS, schedStat);
      }
    }
  } else {
    ALOGE("Length of the jschedStat must be great than or equal to %d", SCHED_STAT_NUMS);
  }
}

static void android_os_statistics_OsUtils_setThreadPriorityUnconditonally(JNIEnv* env, jobject clazz,
    jint tid, jint pri)
{
  setpriority(PRIO_PROCESS, tid, pri);
}

static jlong android_os_statistics_OsUtils_getCoarseUptimeMillisFast(JNIEnv* env, jobject clazz) {
  return FastSystemInfoFetcher::getCoarseUptimeMillisFast();
}

static jlong android_os_statistics_OsUtils_getDeltaToUptimeMillis(JNIEnv* env, jobject clazz) {
  return FastSystemInfoFetcher::getDeltaToUptimeMillis();
}

static jboolean android_os_statistics_OsUtils_isIsolatedProcess(JNIEnv* env, jobject clazz) {
  uid_t appid = getuid() % AID_USER;
  return (jboolean)(appid >= AID_ISOLATED_START && appid <= AID_ISOLATED_END);
}

static jlong android_os_statistics_OsUtils_getRunningTimeMs(JNIEnv* env, jobject clazz) {
  return OsUtils::getRunningTimeMs();
}

#define TRANSLATING_KERNEL_ADDRESS_RESULT_SIZE 128
static jstring android_os_statistics_OsUtils_translateKernelAddress(JNIEnv* env, jobject clazz,
    jlong kernelAddress) {
  char buf[TRANSLATING_KERNEL_ADDRESS_RESULT_SIZE];
  OsUtils::translateKernelAddress(buf, sizeof(buf), (uint64_t)kernelAddress);
  return env->NewStringUTF(buf);
}

static const JNINativeMethod methods[] = {
  {"getThreadScheduler",  "(I)I", (void*)android_os_statistics_OsUtils_getThreadScheduler},
  {"getThreadSchedStat",  "([J)V", (void*)android_os_statistics_OsUtils_getThreadSchedStat},
  {"setThreadPriorityUnconditonally",  "(II)V", (void*)android_os_statistics_OsUtils_setThreadPriorityUnconditonally},
  {"getCoarseUptimeMillisFast","()J", (void*)android_os_statistics_OsUtils_getCoarseUptimeMillisFast},
  {"getDeltaToUptimeMillis","()J", (void*)android_os_statistics_OsUtils_getDeltaToUptimeMillis},
  {"isIsolatedProcess","()Z", (void*)android_os_statistics_OsUtils_isIsolatedProcess},
  {"getRunningTimeMs","()J", (void*)android_os_statistics_OsUtils_getRunningTimeMs},
  {"translateKernelAddress", "(J)Ljava/lang/String;", (void*)android_os_statistics_OsUtils_translateKernelAddress },
};

int register_android_os_statistics_OsUtils(JNIEnv* env)
{
  return RegisterMethodsOrDie(env, "android/os/statistics/OsUtils", methods, NELEM(methods));
}
