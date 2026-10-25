#ifndef ANDROID_OS_STATISTICS_PILOCK_H
#define ANDROID_OS_STATISTICS_PILOCK_H

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <cutils/atomic.h>

namespace android {
namespace os {
namespace statistics {

class PILock {
public:
  PILock();
  void lock();
  void unlock();
private:
  atomic_int mLock;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_PILOCK_H
