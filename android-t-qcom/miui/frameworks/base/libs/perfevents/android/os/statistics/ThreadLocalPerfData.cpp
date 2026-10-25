#include "ThreadLocalPerfData.h"

#include "PerfEventConstants.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

namespace android {
namespace os {
namespace statistics {

static pthread_once_t initflag = PTHREAD_ONCE_INIT;
static pthread_key_t pthread_key_threadperfdata;
static volatile bool key_created = false;

struct ThreadPerfData {
  void* data[TOTAL_EVENT_TYPE_COUNT];
  void* destructor[TOTAL_EVENT_TYPE_COUNT];
};

static void destroy_threadperfdata(void *data)
{
  if (data != nullptr) {
    struct ThreadPerfData* pThreadPerfData = (struct ThreadPerfData*)data;
    for (int i = 0; i < TOTAL_EVENT_TYPE_COUNT; i++) {
      void *typedPerfData = pThreadPerfData->data[i];
      void *typedDestructor = pThreadPerfData->destructor[i];
      if (typedPerfData != nullptr && typedDestructor != nullptr) {
        ((void (*)(void*))typedDestructor)(typedPerfData);
      }
    }
    delete pThreadPerfData;
  }
}

static void create_key_threadperfdata()
{
  int rc = pthread_key_create(&pthread_key_threadperfdata, destroy_threadperfdata);
  if (rc != 0) {
    key_created = false;
  } else {
    key_created = true;
  }
}

static struct ThreadPerfData* get_threadperfdata() {
  if (!key_created) {
    pthread_once(&initflag, create_key_threadperfdata);
  }
  struct ThreadPerfData* pThreadPerfData =
    static_cast<struct ThreadPerfData*>(pthread_getspecific(pthread_key_threadperfdata));
  if (pThreadPerfData == nullptr) {
    pThreadPerfData = new struct ThreadPerfData();
    for (int i = 0; i < TOTAL_EVENT_TYPE_COUNT; i++) {
      pThreadPerfData->data[i] = nullptr;
      pThreadPerfData->destructor[i] = nullptr;
    }
    pthread_setspecific(pthread_key_threadperfdata, (void *)pThreadPerfData);
  }
  return pThreadPerfData;
}

inline int32_t getEventTypeIndex(int32_t eventType) {
  return CC_LIKELY(eventType < MACRO_EVENT_TYPE_START) ?
    (eventType - MICRO_EVENT_TYPE_START) :
    (eventType - MACRO_EVENT_TYPE_START + MICRO_EVENT_TYPE_COUNT);
}

void* ThreadLocalPerfData::get(int eventType) {
  struct ThreadPerfData* pThreadPerfData = get_threadperfdata();
  return pThreadPerfData->data[getEventTypeIndex(eventType)];
}

void ThreadLocalPerfData::set(int eventType, void* pLocalData, void (*destructor)(void*)) {
  struct ThreadPerfData* pThreadPerfData = get_threadperfdata();
  int32_t eventTypeIndex = getEventTypeIndex(eventType);
  pThreadPerfData->data[eventTypeIndex] = pLocalData;
  pThreadPerfData->destructor[eventTypeIndex] = (void *)destructor;
}

} //namespace statistics
} //namespace os
} //namespace android
