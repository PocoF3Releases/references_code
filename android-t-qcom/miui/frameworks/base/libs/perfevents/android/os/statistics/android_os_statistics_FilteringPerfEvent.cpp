#define LOG_TAG "FilteringPerfEvent"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "android_os_Parcel.h"

#include "PerfEventConstants.h"
#include "PerfEvent.h"
#include "JavaVMUtils.h"
#include "PerfSuperviser.h"
#include "NativePerfEventParcelCache.h"
#include "JavaBackTraceCache.h"
#include "JniParcelCache.h"
#include "PerfEventDetailsCache.h"
#include "MemoryBlock.h"
#include "SmallMemoryBlockCache.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

static const char* const kStringPathName = "java/lang/String";
static const char* const kPerfEventPathName = "android/os/statistics/PerfEvent";
static const char* const kFilteringPerfEventPathName = "android/os/statistics/FilteringPerfEvent";

static jclass gStringClass;
static jclass gPerfEventClass;
static jmethodID gFillInDetailsMethod;
static jmethodID gReadDetailsFromParcelMethod;
static jclass gFilteringPerfEventClass;

static void nativeResolveFilteringPerfEventFromPerfEventDetails(JNIEnv* env, jobject perfEvent, jlong detailsPtr) {
  PerfEventDetails* details = (PerfEventDetails*)detailsPtr;
  jobject jniParcel = JniParcelCache::obtain(env);
  jobject dataParcel;
  if (details->mGlobalDetails != nullptr ) {
    dataParcel = jniParcel;
    JniParcelWriter jniParcelWriter;
    jniParcelWriter.beginWrite(env, dataParcel);
    details->mGlobalDetails->readToJniParcel(env, jniParcelWriter);
    jniParcelWriter.endWrite(env);
  } else {
    dataParcel = nullptr;
  }
  jobjectArray javaStackTraceClasses = nullptr;
  jobjectArray javaStackTraceElements = nullptr;
  if (details->mGlobalJavaBackTrace != nullptr) {
    javaStackTraceClasses = JavaVMUtils::resolveClassesOfBackTrace(env, details->mGlobalJavaBackTrace);
    javaStackTraceElements =  JavaVMUtils::resolveJavaStackTrace(env, details->mGlobalJavaBackTrace);
  }
  env->CallVoidMethod(perfEvent, gFillInDetailsMethod, dataParcel, javaStackTraceClasses, javaStackTraceElements, nullptr);
  if (javaStackTraceClasses != nullptr) {
    env->DeleteLocalRef(javaStackTraceClasses);
  }
  if (javaStackTraceElements != nullptr) {
    env->DeleteLocalRef(javaStackTraceElements);
  }

  JniParcelCache::recycle(env, jniParcel);
}

static void nativeResolveFilteringPerfEventFromMemoryBlock(JNIEnv* env,
    jobject perfEvent, jlong detailsPtr, jobject jparcel, jlong jparcelNativePtr) {
  MemoryBlock *memoryBlock = (MemoryBlock *)detailsPtr;
  Parcel* parcel = jparcelNativePtr == 0 ? parcelForJavaObject(env, jparcel) : reinterpret_cast<Parcel*>(jparcelNativePtr);
  parcel->setDataPosition(0);
  parcel->setDataSize(0);
  while (memoryBlock != nullptr) {
    void* data = parcel->writeInplace(memoryBlock->mSize);
    memcpy(data, memoryBlock->mData, memoryBlock->mSize);
    memoryBlock = memoryBlock->mNext;
  }
  parcel->setDataPosition(0);
  env->CallVoidMethod(perfEvent, gReadDetailsFromParcelMethod, jparcel);
}

static void android_os_statistics_FilteringPerfEvent_nativeResolve(
    JNIEnv* env, jobject clazz,
    jobject perfEvent, jlong detailsPtr, jint eventFlags, jobject tempParcel, jlong tempParcelNativePtr) {
  int32_t detailsSource = (eventFlags & FLAG_DETAILS_SOURCE_MASK);
  if (detailsSource == FLAG_FROM_PERFEVENT_DETAILS) {
    nativeResolveFilteringPerfEventFromPerfEventDetails(env, perfEvent, detailsPtr);
  } else if (detailsSource == FLAG_FROM_BLOCK_MEMORY) {
    nativeResolveFilteringPerfEventFromMemoryBlock(env, perfEvent, detailsPtr, tempParcel, tempParcelNativePtr);
  }
}

static void android_os_statistics_FilteringPerfEvent_nativeDipose(
    JNIEnv* env, jobject clazz,
    jlong detailsPtr, jint eventFlags) {
  int32_t detailsSource = (eventFlags & FLAG_DETAILS_SOURCE_MASK);
  if (detailsSource == FLAG_FROM_PERFEVENT_DETAILS) {
    PerfEventDetails* details = (PerfEventDetails*)detailsPtr;
    NativePerfEventParcelCache::recycle(env, details->mGlobalDetails);
    JavaBackTraceCache::recycle(env, details->mGlobalJavaBackTrace);
    PerfEventDetailsCache::recycle(details);
  } else if (detailsSource == FLAG_FROM_BLOCK_MEMORY) {
    MemoryBlock *memoryBlock = (MemoryBlock *)detailsPtr;
    while (memoryBlock != nullptr) {
      MemoryBlock *next = memoryBlock->mNext;
      if (memoryBlock->mCapacity == SMALL_MEMORY_BLOCK_SIZE) {
        SmallMemoryBlockCache::recycle(memoryBlock);
      } else {
        delete memoryBlock;
      }
      memoryBlock = next;
    }
  }
}

static void android_os_statistics_FilteringPerfEvent_nativeWriteToParcel(
    JNIEnv* env, jobject clazz,
    jobject jparcel, jlong parcelNativePtr,
    jint eventType, jint eventFlags, jlong beginUptimeMillis, jlong endUptimeMillis, jlong inclusionId, jlong synchronizationId, jlong eventSeq,
    jlong detailsPtr) {
  Parcel* parcel = parcelNativePtr == 0 ? parcelForJavaObject(env, jparcel) : reinterpret_cast<Parcel*>(parcelNativePtr);
  PerfEventDetails* details = (PerfEventDetails*)detailsPtr;
  parcel->writeInt32(eventType);
  parcel->writeInt32(eventFlags);
  parcel->writeInt64(beginUptimeMillis);
  parcel->writeInt64(endUptimeMillis);
  parcel->writeInt64(inclusionId);
  parcel->writeInt64(synchronizationId);
  parcel->writeInt64(eventSeq);
  if (details->mGlobalDetails != nullptr) {
    details->mGlobalDetails->readToParcel(env, parcel);
  }
}

/*
 * JNI registration.
 */
static const JNINativeMethod gMethods[] = {
  /* name, signature, funcPtr */
  { "nativeResolve",    "(Landroid/os/statistics/PerfEvent;JILandroid/os/Parcel;J)V",
      (void*) android_os_statistics_FilteringPerfEvent_nativeResolve },
  { "nativeDipose",    "(JI)V",
      (void*) android_os_statistics_FilteringPerfEvent_nativeDipose },
  { "nativeWriteToParcel",    "(Landroid/os/Parcel;JIIJJJJJJ)V",
      (void*) android_os_statistics_FilteringPerfEvent_nativeWriteToParcel },
};

int register_android_os_statistics_FilteringPerfEvent(JNIEnv* env)
{
  jclass clazz = FindClassOrDie(env, kStringPathName);
  gStringClass = MakeGlobalRefOrDie(env, clazz);

  clazz = FindClassOrDie(env, kPerfEventPathName);
  gPerfEventClass = MakeGlobalRefOrDie(env, clazz);
  gFillInDetailsMethod = GetMethodIDOrDie(env, clazz, "fillInDetails",
    "(Landroid/os/statistics/JniParcel;[Ljava/lang/Class;[Ljava/lang/StackTraceElement;Landroid/os/statistics/NativeBackTrace;)V");
  gReadDetailsFromParcelMethod = GetMethodIDOrDie(env, clazz, "readDetailsFromParcel", "(Landroid/os/Parcel;)V");

  clazz = FindClassOrDie(env, kFilteringPerfEventPathName);
  gFilteringPerfEventClass = MakeGlobalRefOrDie(env, clazz);

  return RegisterMethodsOrDie(env, kFilteringPerfEventPathName, gMethods, NELEM(gMethods));
}
