#include <algorithm>

#include "PerfSuperviser.h"

namespace android {
namespace os {
namespace statistics {

volatile JavaVM* PerfSuperviser::sVM = nullptr;
volatile int32_t PerfSuperviser::sMyPid = -1;
volatile bool PerfSuperviser::sIsSystemServer = false;
volatile bool PerfSuperviser::sIsMiuiDaemon = false;
volatile bool PerfSuperviser::sStarted = false;
volatile bool PerfSuperviser::sHasKernelPerfEvents = false;
volatile int32_t PerfSuperviser::sMode = PERF_SUPERVISION_OFF;
volatile int32_t PerfSuperviser::sSoftWaitTimeThresholdMs = INT32_MAX;
volatile int32_t PerfSuperviser::sHardWaitTimeThresholdMs = INT32_MAX;
volatile int32_t PerfSuperviser::sMinSoftWaitTimeThresholdMs = 10;
volatile int32_t PerfSuperviser::sDivisionRatio = 2;
volatile int32_t PerfSuperviser::sInnerWaitTimeThresholdMs = INT32_MAX;
volatile int32_t PerfSuperviser::sMaxEventThresholdMs = 0;
volatile int32_t PerfSuperviser::sMinOverlappedMs = 0;
volatile jstring PerfSuperviser::sProcessName = nullptr;
volatile jstring PerfSuperviser::sPackageName = nullptr;

void PerfSuperviser::init(JNIEnv* env,
  uint32_t softWaitTimeThresholdInMillis,
  uint32_t hardWaitTimeThresholdInMillis,
  uint32_t minSoftWaitTimeThresholdInMillis,
  uint32_t maxEventThresholdInMillis,
  uint32_t divisionRatio) {
  if (sVM == nullptr) {
    JavaVM* vm = nullptr;
    sVM = env->GetJavaVM(&vm) >= 0 ? vm : nullptr;
  }
  sSoftWaitTimeThresholdMs = (int32_t)softWaitTimeThresholdInMillis;
  sHardWaitTimeThresholdMs = (int32_t)hardWaitTimeThresholdInMillis;
  sMinSoftWaitTimeThresholdMs = (int32_t)minSoftWaitTimeThresholdInMillis;
  sDivisionRatio = divisionRatio;
  sInnerWaitTimeThresholdMs = (int32_t)std::max(
    softWaitTimeThresholdInMillis / divisionRatio,
    minSoftWaitTimeThresholdInMillis);
  sMaxEventThresholdMs = maxEventThresholdInMillis;
  sMinOverlappedMs = std::max(
    (int32_t)sInnerWaitTimeThresholdMs * 4 / 5,
	(int32_t)sMinSoftWaitTimeThresholdMs);
}

} //namespace statistics
} //namespace os
} //namespace android
