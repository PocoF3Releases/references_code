#ifndef ANDROID_OS_BINDER_EVENT_THREADLOCAL_H
#define ANDROID_OS_BINDER_EVENT_THREADLOCAL_H

#include <string>
#include <sys/types.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "PerfEventConstants.h"
#include "ThreadUtils.h"
#include "ThreadLocalPerfData.h"

#define MAX_BINDER_EXECUTION_DEPTH 5

namespace android {
namespace os {
namespace statistics {

struct BinderExecution {
  int64_t beginUptimeMs;
  int64_t beginRunningTimeMs;
  int64_t beginRunnableTimeMs;
  uint64_t cookie;
};

struct ThreadBinderExecutionsData {
  int32_t threadId;
  std::shared_ptr<std::string> threadName;
  int32_t depth;
  struct BinderExecution binderExecutions[MAX_BINDER_EXECUTION_DEPTH];
};

class BinderEventThreadLocal {
public:
  static struct ThreadBinderExecutionsData* getThreadBinderExecutionsData(JNIEnv* env);
private:
  static void destroyThreadBinderExecutionsData(void *data);
};

} //namespace statistics
} //namespace os
} //namespace android

#endif // ANDROID_OS_BINDER_EVENT_THREADLOCAL_H
