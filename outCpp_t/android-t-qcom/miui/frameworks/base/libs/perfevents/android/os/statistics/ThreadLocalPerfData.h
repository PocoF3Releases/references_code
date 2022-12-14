#ifndef ANDROID_OS_STATISTICS_THREADLOCALPERFDATA_H
#define ANDROID_OS_STATISTICS_THREADLOCALPERFDATA_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <atomic>
#include <string>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <cutils/compiler.h>

namespace android {
namespace os {
namespace statistics {

class ThreadLocalPerfData {
public:
  static void* get(int eventType);
  static void set(int eventType, void* pLocalData, void (*destructor)(void*));
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_THREADLOCALPERFDATA_H
