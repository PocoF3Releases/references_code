#include "BinderThreadLocal.h"
#include <pthread.h>

namespace android {
namespace os {
namespace monitor {

static pthread_once_t initflag = PTHREAD_ONCE_INIT;
static pthread_key_t tlsKey;
static volatile bool hasCreatedKey = false;

static void createTLSKey() {
  if(pthread_key_create(&tlsKey, NULL) != 0) {
    hasCreatedKey = false;
  } else {
    hasCreatedKey = true;
  }
}

struct BinderThreadLocalInfo* BinderThreadLocal::get() {
  if(!hasCreatedKey) {
     pthread_once(&initflag, createTLSKey);
  }
  struct BinderThreadLocalInfo* pThreadData = static_cast<struct BinderThreadLocalInfo*>(pthread_getspecific(tlsKey));
  if(pThreadData == nullptr) {
    pThreadData = new struct BinderThreadLocalInfo();
    pThreadData->depth = 0;
    pThreadData->beginExecutionTimeMills = 0;
    pthread_setspecific(tlsKey, (void*) pThreadData);
  }
  return pThreadData;
}

}  // namespace monitor
}  // namespace os
}  // namespace android