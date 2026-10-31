#define LOG_TAG "PerfEventSocket"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
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
#include <poll.h>
#include <time.h>
#include <cutils/atomic.h>
#include <binder/Parcel.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "android_os_Parcel.h"

#include "PerfEventConstants.h"
#include "PerfSuperviser.h"
#include "MemoryBlock.h"
#include "SmallMemoryBlockCache.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

static const char* const kPerfEventPathName = "android/os/statistics/PerfEvent";
static const char* const kPerfEventFactoryPathName = "android/os/statistics/PerfEventFactory";
static const char* const kFilteringPerfEventFactoryPathName = "android/os/statistics/FilteringPerfEventFactory";

static jclass gFilteringPerfEventFactoryClass;
static jmethodID gCreateFilteringPerfEventMethod1;
static jmethodID gCreateFilteringPerfEventMethod2;
static jclass gPerfEventFactoryClass;
static jmethodID gCreatePerfEventMethod1;
static jmethodID gCreatePerfEventMethod2;
static jclass gPerfEventClass;
static jmethodID gReadDetailsFromParcelMethod;
static jmethodID gReadFromParcelMethod;

static jint android_os_statistics_PerfEventSocket_sendPerfEvent(
  JNIEnv* env, jobject clazz, jint sockfd, jobject jparcel, jlong perfEventParcelNativePtr, jint maxPerfEventParcelSize) {
  Parcel* parcel = perfEventParcelNativePtr == 0 ? parcelForJavaObject(env, jparcel) : reinterpret_cast<Parcel*>(perfEventParcelNativePtr);
  if (parcel->dataSize() > (size_t)maxPerfEventParcelSize) {
    return -1 * EMSGSIZE;
  }
  int size = (int)send(sockfd, parcel->data(), parcel->dataSize(), 0);
  if (size >= 0) {
    return size;
  } else {
    return -1 * errno;
  }
}

static jint android_os_statistics_PerfEventSocket_waitForPerfEventArrived(
  JNIEnv* env, jobject clazz, jint sockfd) {
  int res = 0;
  struct pollfd rfds[1];
  while(1){
    rfds[0].fd = sockfd;
    rfds[0].events = POLLIN | POLLRDNORM;
    rfds[0].revents = 0;
    res = poll(rfds, 1, -1);
    if (res >= 0) {
      break;
    } else {
      if (errno == ENOMEM) {
        usleep(1000 * 1000);
        continue;
      } else if (errno == EINTR) {
        continue;
      } else {
        break;
      }
    }
  }
  if (res < 0) {
    return -1 * errno;
  } else if ((rfds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
    return -1 * EBADF;
  } else {
    usleep(PerfSuperviser::getInnerWaitThresholdMs() * 5 * 1000);
    return 0;
  }
}

static jint android_os_statistics_PerfEventSocket_receivePerfEvents(
    JNIEnv* env, jobject clazz,
    jint sockfd, jobject jparcel, jlong perfEventParcelNativePtr, jint maxPerfEventParcelSize, jobjectArray resultBuffer) {
  const int32_t resultBufferSize = env->GetArrayLength(resultBuffer);
  int32_t count = 0;
  Parcel* parcel = perfEventParcelNativePtr == 0 ? parcelForJavaObject(env, jparcel) : reinterpret_cast<Parcel*>(perfEventParcelNativePtr);
  parcel->setDataCapacity(maxPerfEventParcelSize);
  while (count < resultBufferSize) {
    parcel->setDataPosition(0);
    parcel->setDataSize(0);
    void* raw = parcel->writeInplace(maxPerfEventParcelSize);
    int size = (int)recv(sockfd, raw, maxPerfEventParcelSize, MSG_TRUNC | MSG_DONTWAIT);
    if (size > 0 && size <= maxPerfEventParcelSize) {
      parcel->setDataPosition(0);
      parcel->setDataSize(size);
      int32_t eventType = parcel->readInt32();
      int32_t eventFlags =  parcel->readInt32();
      int64_t beginUptimeMillis =  parcel->readInt64();
      int64_t endUptimeMillis =  parcel->readInt64();
      int64_t inclusionId =  parcel->readInt64();
      int64_t synchronizationId =  parcel->readInt64();
      int64_t eventSeq =  parcel->readInt64();
      jobject result = nullptr;
      eventFlags &= ~FLAG_DETAILS_SOURCE_MASK;
      eventFlags |= FLAG_FROM_BLOCK_MEMORY;
      MemoryBlock *memoryBlock = nullptr;
      if (parcel->dataAvail() > 192) {
        memoryBlock = new MemoryBlock(parcel->dataAvail());
        memoryBlock->mNext = nullptr;
      } else {
        int remainedSize = parcel->dataAvail();
        while (remainedSize > 0) {
          MemoryBlock* temp = SmallMemoryBlockCache::obtain();
          temp->mNext = memoryBlock;
          memoryBlock = temp;
          remainedSize -= memoryBlock->mCapacity;
        }
      }
      MemoryBlock* temp = memoryBlock;
      while (temp != nullptr) {
        temp->mSize = std::min((int16_t)parcel->dataAvail(), temp->mCapacity);
        const void* data = parcel->readInplace(temp->mSize);
        memcpy(temp->mData, data, temp->mSize);
        temp = temp->mNext;
      }
      result = env->CallStaticObjectMethod(gFilteringPerfEventFactoryClass, gCreateFilteringPerfEventMethod1,
        (jint)eventType, (jint)eventFlags, (jlong)beginUptimeMillis, (jlong)endUptimeMillis,
        (jlong)inclusionId, (jlong)synchronizationId, (jlong)eventSeq,
        (jlong)memoryBlock);
      env->SetObjectArrayElement(resultBuffer, count, result);
      env->DeleteLocalRef(result);
      count++;
      continue;
    } else if (size == 0) {
      break;
    } else if (size > maxPerfEventParcelSize) {
      continue;
    } else {
      int err = errno;
      if (err == EAGAIN) {
        break;
      } else if (err == EINTR) {
        continue;
      } else {
        break;
      }
    }
  }
  parcel->setDataPosition(0);
  parcel->setDataSize(0);
  return count;
}

/*
 * JNI registration.
 */
static const JNINativeMethod gMethods[] = {
  /* name, signature, funcPtr */
  { "sendPerfEvent",    "(ILandroid/os/Parcel;JI)I",
      (void*) android_os_statistics_PerfEventSocket_sendPerfEvent },
  { "waitForPerfEventArrived",    "(I)I",
      (void*) android_os_statistics_PerfEventSocket_waitForPerfEventArrived },
  { "receivePerfEvents",    "(ILandroid/os/Parcel;JI[Landroid/os/statistics/FilteringPerfEvent;)I",
      (void*) android_os_statistics_PerfEventSocket_receivePerfEvents },
};

const char* const kPerfEventSocketPathName = "android/os/statistics/PerfEventSocket";

int register_android_os_statistics_PerfEventSocket(JNIEnv* env)
{
  jclass clazz = FindClassOrDie(env, kFilteringPerfEventFactoryPathName);
  gFilteringPerfEventFactoryClass = MakeGlobalRefOrDie(env, clazz);
  gCreateFilteringPerfEventMethod1 = GetStaticMethodIDOrDie(env, clazz, "createFilteringPerfEvent",
        "(IIJJJJJJ)Landroid/os/statistics/FilteringPerfEvent;");
  gCreateFilteringPerfEventMethod2 = GetStaticMethodIDOrDie(env, clazz, "createFilteringPerfEvent",
        "(Landroid/os/statistics/PerfEvent;)Landroid/os/statistics/FilteringPerfEvent;");

  clazz = FindClassOrDie(env, kPerfEventFactoryPathName);
  gPerfEventFactoryClass = MakeGlobalRefOrDie(env, clazz);
  gCreatePerfEventMethod1 = GetStaticMethodIDOrDie(env, clazz, "createPerfEvent",
        "(IIJJJJJ)Landroid/os/statistics/PerfEvent;");
  gCreatePerfEventMethod2 = GetStaticMethodIDOrDie(env, clazz, "createPerfEvent",
        "(I)Landroid/os/statistics/PerfEvent;");

  clazz = FindClassOrDie(env, kPerfEventPathName);
  gPerfEventClass = MakeGlobalRefOrDie(env, clazz);
  gReadFromParcelMethod = GetMethodIDOrDie(env, clazz, "readFromParcel", "(Landroid/os/Parcel;)V");
  gReadDetailsFromParcelMethod = GetMethodIDOrDie(env, clazz, "readDetailsFromParcel", "(Landroid/os/Parcel;)V");

  return RegisterMethodsOrDie(env, kPerfEventSocketPathName, gMethods, NELEM(gMethods));
}
