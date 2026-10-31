#define LOG_TAG "MiuiPerfSuperviser"

#include <stdlib.h>
#include <cutils/log.h>

#include "PerfEventConstants.h"
#include "JavaVMUtils.h"
#include "PerfEventBuffer.h"
#include "NativePerfEventParcelCache.h"
#include "JavaBackTraceCache.h"
#include "JniParcelCache.h"
#include "PerfEventDetailsCache.h"
#include "PILock.h"

using namespace android;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

class JavaPerfEvent {
public:
  int32_t mEventType;
  int64_t mEventSeq;
};

enum EventSource {
  JavaLayer,
  NativeLayer,
};

class PerfEventInfo {
public:
  enum EventSource mEventSource;
  union {
    JavaPerfEvent mJavaPerfEvent;
    PerfEvent mNativePerfEvent;
  };
};

class FreePerfEventInfoCache : public FreeNativeObjectCache {
public:
  FreePerfEventInfoCache(uint32_t cacheCapacity);
  virtual ~FreePerfEventInfoCache();
protected:
  virtual void* createNewObject();
  virtual void deleteObject(void *pObject);
};

FreePerfEventInfoCache::FreePerfEventInfoCache(uint32_t cacheCapacity)
  : FreeNativeObjectCache(cacheCapacity) {
}

FreePerfEventInfoCache::~FreePerfEventInfoCache() {
}

void* FreePerfEventInfoCache::createNewObject() {
  return new PerfEventInfo();
}

void FreePerfEventInfoCache::deleteObject(void *pObject) {
  delete (PerfEventInfo *)pObject;
}

static const char* const kPerfEventPathName = "android/os/statistics/PerfEvent";
static const char* const kFilteringPerfEventFactoryPathName = "android/os/statistics/FilteringPerfEventFactory";

static jclass gFilteringPerfEventFactoryClass;
static jmethodID gCreateFilteringPerfEventMethod1;
static jmethodID gCreateFilteringPerfEventMethod2;
static jclass gPerfEventClass;
static jmethodID gFillInSeqMethod;

static uint32_t gCapacity;
static FreePerfEventInfoCache* gPerfEventInfoCache;
static PerfEventInfo** gPerfEventInfos;
static jobjectArray gJavaPerfEventObjects;
static volatile uint32_t gHead;
static volatile uint32_t gCount;
static PILock* gLock;

void PerfEventBuffer::init(JNIEnv* env, uint32_t capacity) {
  jclass clazz = FindClassOrDie(env, kFilteringPerfEventFactoryPathName);
  gFilteringPerfEventFactoryClass = MakeGlobalRefOrDie(env, clazz);
  gCreateFilteringPerfEventMethod1 = GetStaticMethodIDOrDie(env, clazz, "createFilteringPerfEvent",
        "(IIJJJJJJ)Landroid/os/statistics/FilteringPerfEvent;");
  gCreateFilteringPerfEventMethod2 = GetStaticMethodIDOrDie(env, clazz, "createFilteringPerfEvent",
        "(Landroid/os/statistics/PerfEvent;)Landroid/os/statistics/FilteringPerfEvent;");

  clazz = FindClassOrDie(env, kPerfEventPathName);
  gPerfEventClass = MakeGlobalRefOrDie(env, clazz);
  gFillInSeqMethod = GetMethodIDOrDie(env, clazz, "fillInSeq", "(J)V");

  gCapacity = capacity;
  gPerfEventInfoCache = new FreePerfEventInfoCache(gCapacity);
  gPerfEventInfoCache->init(gCapacity / 5);
  gPerfEventInfos = (PerfEventInfo**)malloc(sizeof(PerfEventInfo*) * gCapacity);
  jobjectArray temp = env->NewObjectArray(gCapacity, gPerfEventClass, nullptr);
  gJavaPerfEventObjects = (jobjectArray)env->NewGlobalRef(temp);
  env->DeleteLocalRef(temp);
  gHead = 0;
  gCount = 0;
  gLock = new PILock();
}

void PerfEventBuffer::extendTo(JNIEnv* env, uint32_t capacity) {
  PerfEventInfo** oldPerfEventInfos = gPerfEventInfos;
  jobjectArray oldJavaPerfEventObjects = gJavaPerfEventObjects;
  if (capacity > gCapacity) {
    gCapacity = capacity;
    gPerfEventInfoCache->extendTo(gCapacity);
    gPerfEventInfos = (PerfEventInfo**)malloc(sizeof(PerfEventInfo*) * gCapacity);
    jobjectArray temp = env->NewObjectArray(gCapacity, gPerfEventClass, nullptr);
    gJavaPerfEventObjects = (jobjectArray)env->NewGlobalRef(temp);
    env->DeleteLocalRef(temp);

    free(oldPerfEventInfos);
    env->DeleteGlobalRef(oldJavaPerfEventObjects);
  }
}

void PerfEventBuffer::addPerfEvent(JNIEnv* env, int32_t eventType, int64_t eventSeq, jobject perfEvent) {
  PerfEventInfo* perfEventInfo = (PerfEventInfo*)gPerfEventInfoCache->obtain();
  perfEventInfo->mEventSource = JavaLayer;
  perfEventInfo->mJavaPerfEvent.mEventType = eventType;
  perfEventInfo->mJavaPerfEvent.mEventSeq = eventSeq;
  int pos = -1;
  gLock->lock();
  uint32_t count = gCount;
  if (count < gCapacity) {
    pos = (gHead + count) % gCapacity;
    gCount = count + 1;
    gPerfEventInfos[pos] = perfEventInfo;
    env->SetObjectArrayElement(gJavaPerfEventObjects, pos, perfEvent);
  }
  gLock->unlock();

  if (pos < 0) {
    ALOGE("java perfevent recycled");
    gPerfEventInfoCache->recycle(perfEventInfo);
  }
}

void PerfEventBuffer::addPerfEvent(JNIEnv* env, PerfEvent& perfEvent) {
  PerfEventInfo* perfEventInfo = (PerfEventInfo*)gPerfEventInfoCache->obtain();
  perfEventInfo->mEventSource = NativeLayer;
  perfEventInfo->mNativePerfEvent = perfEvent;
  int pos = -1;
  gLock->lock();
  uint32_t count = gCount;
  if (count < gCapacity) {
    pos = (gHead + count) % gCapacity;
    gCount = count + 1;
    gPerfEventInfos[pos] = perfEventInfo;
  }
  gLock->unlock();
  if (pos < 0) {
    ALOGE("native micro event recycled");
    NativePerfEventParcelCache::recycle(env, perfEvent.mDetails.mGlobalDetails);
    if (env != nullptr) {
      JavaBackTraceCache::recycle(env, perfEvent.mDetails.mGlobalJavaBackTrace);
    }
    gPerfEventInfoCache->recycle(perfEventInfo);
  }
}

uint32_t PerfEventBuffer::count() {
  return gCount;
}

int32_t PerfEventBuffer::fetch(JNIEnv* env, jobjectArray fetchingBuffer) {
  const int32_t fetchingBufferSize = env->GetArrayLength(fetchingBuffer);

  int32_t size = 0;
  while (size < fetchingBufferSize) {
    PerfEventInfo* perfEventInfo = nullptr;
    jobject perfEvent = nullptr;
    uint32_t head;
    uint32_t count;
    gLock->lock();
    head = gHead;
    count = gCount;
    if (count > 0) {
      perfEventInfo = gPerfEventInfos[head];
      if (perfEventInfo->mEventSource == JavaLayer) {
        perfEvent = env->GetObjectArrayElement(gJavaPerfEventObjects, head);
        env->SetObjectArrayElement(gJavaPerfEventObjects, head, nullptr);
      }
      head = (head + 1) % gCapacity;
      count--;
      gHead = head;
      gCount = count;
    }
    gLock->unlock();
    if (perfEventInfo != nullptr) {
      enum EventSource eventSource = perfEventInfo->mEventSource;
      jobject result;
      if (eventSource == JavaLayer) {
        env->CallVoidMethod(perfEvent, gFillInSeqMethod, (jlong)perfEventInfo->mJavaPerfEvent.mEventSeq);
        result = env->CallStaticObjectMethod(gFilteringPerfEventFactoryClass, gCreateFilteringPerfEventMethod2,
          perfEvent);
      } else {
        PerfEventDetails* details = PerfEventDetailsCache::obtain();
        *details = perfEventInfo->mNativePerfEvent.mDetails;
        if ((details->mGlobalDetails != nullptr && details->mGlobalDetails->hasProcLazyInfo()) ||
            details->mGlobalJavaBackTrace != nullptr) {
          perfEventInfo->mNativePerfEvent.mEventFlags |= FLAG_HAS_PROC_LAZYINFO;
        }
        perfEventInfo->mNativePerfEvent.mEventFlags |= FLAG_FROM_PERFEVENT_DETAILS;
        result = env->CallStaticObjectMethod(gFilteringPerfEventFactoryClass, gCreateFilteringPerfEventMethod1,
          (jint)perfEventInfo->mNativePerfEvent.mEventType, (jint)perfEventInfo->mNativePerfEvent.mEventFlags,
          (jlong)perfEventInfo->mNativePerfEvent.mBeginUptimeMillis, (jlong)perfEventInfo->mNativePerfEvent.mEndUptimeMillis,
          (jlong)perfEventInfo->mNativePerfEvent.mInclusionId, (jlong)perfEventInfo->mNativePerfEvent.mSynchronizationId,
          (jlong)perfEventInfo->mNativePerfEvent.mEventSeq,
          (jlong)details);
      }
      if (result != nullptr) {
        env->SetObjectArrayElement(fetchingBuffer, size, result);
        env->DeleteLocalRef(result);
        size++;
      }
      gPerfEventInfoCache->recycle(perfEventInfo);
    }
    if (count == 0) {
      break;
    }
  }

  return size;
}

} //namespace statistics
} //namespace os
} //namespace android
