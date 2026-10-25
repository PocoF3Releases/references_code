#define LOG_TAG "FastSystemInfoFetcher"
#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <alloca.h>
#include <stdatomic.h>
#include <sys/resource.h>
#include <pthread.h>
#include <sched.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <utils/Log.h>
#include <cutils/ashmem.h>
#include <cutils/atomic.h>
#include <cutils/compiler.h>

#include "OsUtils.h"
#include "PerfSuperviser.h"
#include "FastSystemInfoFetcher.h"

namespace android {
namespace os {
namespace statistics {

#define POWER_2_32 0x100000000ULL

struct mi_sys_info {
  uint32_t size;
  uint32_t version;
  uint32_t config_hz;
  uint32_t jiffies;
  struct {
    uint32_t count;
    uint32_t total_duration_ms;
    uint32_t time_stamp_jiffies;
  } mm_slowpath;
  uint32_t placeholder[32];
};
static struct mi_sys_info* gMiSysInfo = NULL;

void FastSystemInfoFetcher::init(bool isPrimaryZygote, bool hasSecondaryZygote) {
  int fd = open("/dev/misysinfofreader", O_RDONLY);
  if (fd < 0) {
    SLOGE("failed to open /dev/misysinfofreader, errno: %d", errno);
    return;
  }

  const int page_size = (int)sysconf(_SC_PAGE_SIZE);
  void* map_addr = mmap(NULL, page_size, PROT_READ, MAP_SHARED, fd, 0);
  if (map_addr == MAP_FAILED) {
    SLOGE("mmap failed at FastSystemInfoFetcher::init, errno: %d", errno);
    close(fd);
    return;
  }
  mlock(map_addr, page_size);

  gMiSysInfo = (struct mi_sys_info*)map_addr;
  close(fd);
}

bool FastSystemInfoFetcher::hasMiSysInfo() {
  return gMiSysInfo != NULL;
}

int32_t FastSystemInfoFetcher::getConfigHZ() {
  if (gMiSysInfo != NULL) {
    return gMiSysInfo->config_hz;
  } else {
    return -1;
  }
}

inline uint64_t calculateUptimeMillis(uint64_t jiffies, uint32_t configHz) {
  if (configHz == 100) {
    return jiffies * 10;
  } else if (configHz == 250) {
    return jiffies * 4;
  } else if (configHz == 300) {
    return jiffies * 10 / 3;
  } else if (configHz == 1000) {
    return jiffies;
  } else {
    return jiffies * 1000 / configHz;
  }
}

inline int64_t getClockMonotonicCoarseMillis() {
  timespec now;
  int ret = clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
  if (ret < 0) {
    clock_gettime(CLOCK_MONOTONIC, &now);
  }
  return static_cast<int64_t>(now.tv_sec) * INT64_C(1000) + now.tv_nsec / INT64_C(1000000);
}

static int64_t getRawCoarseUptimeMillis() {
  if (gMiSysInfo != NULL) {
    uint64_t jiffies = (uint64_t)gMiSysInfo->jiffies;
    return (int64_t)calculateUptimeMillis(jiffies, gMiSysInfo->config_hz);
  } else {
    return getClockMonotonicCoarseMillis();
  }
}

int64_t FastSystemInfoFetcher::getCoarseUptimeMillisFast() {
  return getRawCoarseUptimeMillis();
}

int64_t FastSystemInfoFetcher::getDeltaToUptimeMillis() {
  if (gMiSysInfo != NULL) {
    int64_t expected = getClockMonotonicCoarseMillis();
    int64_t actualRaw = getRawCoarseUptimeMillis();
    return expected - actualRaw;
  } else {
    return 0;
  }
}

bool FastSystemInfoFetcher::readMemorySlowpathStats(struct MemorySlowpathStats &stats) {
  if (gMiSysInfo == NULL || gMiSysInfo->version < 1) {
    stats.count = 0;
    stats.totalDurationMs = 0;
    stats.timeStampMs = 0;
    return false;
  } else {
    stats.count = gMiSysInfo->mm_slowpath.count;
    stats.totalDurationMs = gMiSysInfo->mm_slowpath.total_duration_ms;
    stats.timeStampMs = calculateUptimeMillis(gMiSysInfo->mm_slowpath.time_stamp_jiffies, gMiSysInfo->config_hz);
    return true;
  }
}

} //namespace statistics
} //namespace os
} //namespace android
