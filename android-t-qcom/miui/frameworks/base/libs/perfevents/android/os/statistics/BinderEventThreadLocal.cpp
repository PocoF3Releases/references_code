#include "BinderEventThreadLocal.h"

namespace android {
namespace os {
namespace statistics {

void BinderEventThreadLocal::destroyThreadBinderExecutionsData(void *data) {
  if (data != nullptr) {
    delete (struct ThreadBinderExecutionsData*)data;
  }
}

struct ThreadBinderExecutionsData* BinderEventThreadLocal::getThreadBinderExecutionsData(JNIEnv* env) {
  struct ThreadBinderExecutionsData* pLocalData = (struct ThreadBinderExecutionsData*)ThreadLocalPerfData::get(TYPE_SINGLE_BINDER_EXECUTION);
  if (pLocalData == nullptr) {
    pLocalData = new struct ThreadBinderExecutionsData();
    ThreadUtils::getThreadInfo(env, pLocalData->threadId, pLocalData->threadName);
    pLocalData->depth = 0;
    for (int i = 0; i < MAX_BINDER_EXECUTION_DEPTH; i++) {
      pLocalData->binderExecutions[i].beginUptimeMs = 0;
      pLocalData->binderExecutions[i].beginRunningTimeMs = 0;
      pLocalData->binderExecutions[i].beginRunnableTimeMs = 0;
      pLocalData->binderExecutions[i].cookie = 0;
    }
    ThreadLocalPerfData::set(TYPE_SINGLE_BINDER_EXECUTION, pLocalData, destroyThreadBinderExecutionsData);
  }
  return pLocalData;
}

} //namespace statistics
} //namespace os
} //namespace android
