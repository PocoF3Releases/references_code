#ifndef ANDROID_OS_STATISTICS_PERFEVENT_H
#define ANDROID_OS_STATISTICS_PERFEVENT_H

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <cutils/compiler.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "NativePerfEventParcel.h"

namespace android {
namespace os {
namespace statistics {

class PerfEventDetails {
public:
  NativePerfEventParcel* mGlobalDetails;
  jobject mGlobalJavaBackTrace;
};

class PerfEvent {
public:
  int32_t mEventType;
  int32_t mEventFlags;
  int64_t mBeginUptimeMillis;
  int64_t mEndUptimeMillis;
  int64_t mInclusionId;
  int64_t mSynchronizationId;
  int64_t mEventSeq;
  PerfEventDetails mDetails;
};

static inline int64_t generateCoordinationId(int32_t pid, int32_t code) {
  return (((int64_t)pid) << 32) + code;
}

static inline int64_t generateCoordinationId(int32_t pid) {
  return ((int64_t)pid) << 32;
}

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_PERFEVENT_H
