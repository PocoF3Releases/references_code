#ifndef ANDROID_OS_BINDER_THREAD_LOCAL_H
#define ANDROID_OS_BINDER_THREAD_LOCAL_H

#include <sys/types.h>

namespace android {
namespace os {
namespace monitor {

struct BinderThreadLocalInfo {
    int32_t depth;
    int64_t beginExecutionTimeMills;
};

class BinderThreadLocal {
public:
  static BinderThreadLocalInfo* get();
};

}  // namespace monitor
}  // namespace os
}  // namespace android

#endif