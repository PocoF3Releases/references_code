#define LOG_TAG "OsUtils"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <cutils/log.h>

#include "OsUtils.h"

namespace android {
namespace os {
namespace statistics {

#define UNSPECIFIED_FD -2

static const char* translating_kernel_address_args_file = "/d/kperfevents/translating_kernel_address_args";
static const char* translating_kernel_address_result_file = "/d/kperfevents/translating_kernel_address_result";
static int fd_translating_kernel_address_args = UNSPECIFIED_FD;
static int fd_translating_kernel_address_result = UNSPECIFIED_FD;

void OsUtils::getThreadSchedStat(int32_t tid, int64_t& runningTimeMs, int64_t& runnableTimeMs) {
  int fd = 0;
  int cnt = 0;
  char buf[84] = {'\0'};
  runningTimeMs = 0;
  runnableTimeMs = 0;
  //schedStat index: 0, running time 1, runnable time
  sprintf(buf, "/proc/self/task/%d/schedstat", tid);
  fd = open(buf, O_RDONLY);
  if (fd > 0) {
    cnt = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (cnt > 0) {
      buf[cnt] = '\0';
      sscanf(buf, "%" PRId64" %" PRId64, &runningTimeMs, &runnableTimeMs);
      runningTimeMs = runningTimeMs / 1000000;
      runnableTimeMs = runnableTimeMs / 1000000;
    }
  }
}

void OsUtils::translateKernelAddress(char* buf, int32_t bufSize, uint64_t val) {
  if (fd_translating_kernel_address_args == UNSPECIFIED_FD) {
    fd_translating_kernel_address_args = open(translating_kernel_address_args_file,O_WRONLY);
    if (fd_translating_kernel_address_args < 0) {
      SLOGE("failed to open %s, errno: %d", translating_kernel_address_args_file, errno);
    }
  }
  if (fd_translating_kernel_address_result == UNSPECIFIED_FD) {
    fd_translating_kernel_address_result = open(translating_kernel_address_result_file,O_RDONLY);
    if (fd_translating_kernel_address_args < 0) {
      SLOGE("failed to open %s, errno: %d", translating_kernel_address_result_file, errno);
    }
  }

  buf[0] = '\0';
  if (fd_translating_kernel_address_args < 0 || fd_translating_kernel_address_result < 0) {
    return;
  }

  char str[32];
  str[0]= '\0';
  int len = snprintf(str, sizeof(str), "%llu", (unsigned long long)val);
  if (write(fd_translating_kernel_address_args, str, len + 1) >= 0) {
    lseek(fd_translating_kernel_address_args, 0, SEEK_SET);
    int count = read(fd_translating_kernel_address_result, buf, bufSize);
    if (count < 0) {
      buf[0] = '\0';
    } else {
      buf[bufSize - 1] = '\0';
      lseek(fd_translating_kernel_address_result, 0, SEEK_SET);
    }
  }
}

} //namespace statistics
} //namespace os
} //namespace android
