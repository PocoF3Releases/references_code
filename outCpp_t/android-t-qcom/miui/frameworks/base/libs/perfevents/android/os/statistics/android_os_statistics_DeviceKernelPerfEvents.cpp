#define LOG_TAG "PerfEventReporter"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <cutils/atomic.h>
#include <binder/Parcel.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "OsUtils.h"
#include "PerfSuperviser.h"
#include "PerfEventReporter.h"
#include "ExecutingRootEvents.h"
#include "KernelPerfEventsReader.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

static KernelPerfEventsReader *gDeviceKernelPerfEventsReader = NULL;

static jboolean android_os_statistics_DeviceKernelPerfEvents_initDeviceKernelPerfEvents(
  JNIEnv* env, jobject clazz) {
  gDeviceKernelPerfEventsReader = KernelPerfEventsReader::openDeviceKernelPerfEventsReader((int)sysconf(_SC_PAGE_SIZE));
  return (jboolean)(gDeviceKernelPerfEventsReader != NULL);
}

static jboolean android_os_statistics_DeviceKernelPerfEvents_waitForDeviceKernelPerfEventArrived(
  JNIEnv* env, jobject clazz, jint timeoutMillis) {
  int res = 0;

  if (gDeviceKernelPerfEventsReader == NULL) {
    return false;
  }

  if (!gDeviceKernelPerfEventsReader->isEmpty()) {
    return true;
  }

  struct pollfd rfds[1];
  while(1){
    rfds[0].fd = gDeviceKernelPerfEventsReader->getfd();
    rfds[0].events = POLLIN | POLLRDNORM;
    rfds[0].revents = 0;
    res = poll(rfds, 1, timeoutMillis);
    if (res < 0 && errno == EINTR) {
      continue;
    }
    break;
  }
  if (res < 0) {
    return false;
  } else if (rfds[0].revents == POLLERR || rfds[0].revents == POLLHUP) {
    return false;
  } else {
    usleep(std::min(PerfSuperviser::getInnerWaitThresholdMs() * 6, 300) * 1000);
    return true;
  }
}

static jint android_os_statistics_DeviceKernelPerfEvents_fetchDeviceKernelPerfEvents(
  JNIEnv* env, jobject clazz, jobjectArray fetchingBuffer) {
  uint32_t count = 0;
  if (gDeviceKernelPerfEventsReader != NULL) {
    count = gDeviceKernelPerfEventsReader->readPerfEvents(env, fetchingBuffer);
  }
  return count;
}

/*
 * JNI registration.
 */
static const JNINativeMethod gMethods[] = {
  /* name, signature, funcPtr */
  { "initDeviceKernelPerfEvents",    "()Z",
      (void*) android_os_statistics_DeviceKernelPerfEvents_initDeviceKernelPerfEvents },
  { "waitForDeviceKernelPerfEventArrived",    "(I)Z",
      (void*) android_os_statistics_DeviceKernelPerfEvents_waitForDeviceKernelPerfEventArrived },
  { "fetchDeviceKernelPerfEvents",    "([Landroid/os/statistics/FilteringPerfEvent;)I",
      (void*) android_os_statistics_DeviceKernelPerfEvents_fetchDeviceKernelPerfEvents },
};

const char* const kDeviceKernelPerfEventsPathName = "android/os/statistics/DeviceKernelPerfEvents";

int register_android_os_statistics_DeviceKernelPerfEvents(JNIEnv* env)
{
  return RegisterMethodsOrDie(env, kDeviceKernelPerfEventsPathName, gMethods, NELEM(gMethods));
}
