#ifndef ANDROID_OS_STATISTICS_EXECUTIONGROOTEVENTS_H
#define ANDROID_OS_STATISTICS_EXECUTIONGROOTEVENTS_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <cutils/compiler.h>
#include <cutils/atomic.h>

namespace android {
namespace os {
namespace statistics {

class ExecutingRootEvents {
public:
  static void init();
  static inline uint64_t beginMasterRootEvent(int64_t beginUptimeMs, bool isUserPerceptible) {
    return beginRootEvent(beginUptimeMs, true, isUserPerceptible);
  }
  static inline void endMasterRootEvent(uint64_t cookie, int64_t beginUptimeMs, int64_t endUptimeMs, bool isUserPerceptible) {
    endRootEvent(cookie, beginUptimeMs, endUptimeMs, true, isUserPerceptible);
  }
  static inline uint64_t beginRootEvent(int64_t beginUptimeMs) {
    return beginRootEvent(beginUptimeMs, false, false);
  }
  static inline void endRootEvent(uint64_t cookie, int64_t beginUptimeMs, int64_t endUptimeMs) {
    endRootEvent(cookie, beginUptimeMs, endUptimeMs, false, false);
  }
  static bool isUserPerceptibleMasterRootEvent(uint64_t cookie, int64_t beginUptimeMs);
  static int64_t getEarliestBeginUptimeMillis();
  // isReportedSynchronously: 是否在报告该微观事件后，被影响的事件才会继续进行.
  static bool maybeEffectiveMicroscopicEvent(int64_t endUptimeMs, bool isReportedSynchronously);
private:
  static uint64_t beginRootEvent(int64_t beginUptimeMs, bool isMasterEvent, bool isUserPerceptible);
  static void endRootEvent(uint64_t cookie, int64_t beginUptimeMs, int64_t endUptimeMs, bool isMasterEvent, bool isUserPerceptible);
};


} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_EXECUTIONGROOTEVENTS_H
