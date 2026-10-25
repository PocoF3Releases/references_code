#ifndef ANDROID_OS_STATISTICS_FASTSYSTEMINFOFETCHER_H
#define ANDROID_OS_STATISTICS_FASTSYSTEMINFOFETCHER_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

namespace android {
namespace os {
namespace statistics {

struct MemorySlowpathStats {
  uint32_t count;
  uint32_t totalDurationMs;
  uint64_t timeStampMs;
};

class FastSystemInfoFetcher {
public:
  static void init(bool isPrimaryZygote, bool hasSecondaryZygote);

  static bool hasMiSysInfo();
  static int32_t getConfigHZ();

  //快速获取Uptime。为了实现效率，时间是根据32位长度的jiffies计算得出的
  //overflow时间取决于时钟中断频率。
  // 100HZ是为490天
  // 300HZ时为165天
  // 1000HZ时为49天
  //overflow后会从0开始.
  //该函数仅能用于perfevents的性能打点，采用此种方式计算线程性能事件的执行时间是可行的，
  //因为overflow时丢一部分数据是可以接受的。
  static int64_t getCoarseUptimeMillisFast();
  //通过getCoarseUptimeMillisFast获取的uptime是根据jiffies计算得出的
  //算法与通过CLOCK_MONOTONIC/CLOCK_MONOTONIC_COARSE获取的uptime有所不同,
  //随着时间的流逝，二者会产生差别。在获取uptime时，
  //部分事件使用CLOCK_MONOTONIC/CLOCK_MONOTONIC_COARSE获取uptime,
  //为了不给分析造成困扰，需要将这些时间矫正为根据jiffies计算得出的时间.
  //本方法返回二者之间的delta，将CLOCK_MONOTONIC/CLOCK_MONOTONIC_COARSE时间减去delta,
  //就获得了期望值。
  static int64_t getDeltaToUptimeMillis();

  static bool readMemorySlowpathStats(struct MemorySlowpathStats &stats);
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_FASTSYSTEMINFOFETCHER_H
