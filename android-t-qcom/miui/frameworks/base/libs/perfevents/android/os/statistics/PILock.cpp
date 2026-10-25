#include <unistd.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <cutils/compiler.h>

#include "PILock.h"

using namespace android;
using namespace android::os;
using namespace android::os::statistics;

#ifndef SYS_futex
#define SYS_futex __NR_futex
#endif

static inline int futex(volatile int *uaddr, int op, int val, const struct timespec *timeout,
                        volatile int *uaddr2, int val3) {
  return syscall(SYS_futex, uaddr, op, val, timeout, uaddr2, val3);
}

PILock::PILock() {
  atomic_init(&mLock, 0);
}

void PILock::lock() {
  int tid = gettid();
  while (true) {
    int expected = 0;
    if (atomic_compare_exchange_strong(&mLock, &expected, tid)) {
      break;
    }

    if (futex((volatile int *)&mLock, FUTEX_LOCK_PI | FUTEX_PRIVATE_FLAG, tid, NULL, NULL, 0) == 0) {
      break;
    }
  }
}

void PILock::unlock() {
  int tid = gettid();
  while (true) {
    int expected = tid;
    if (atomic_compare_exchange_strong(&mLock, &expected, 0)) {
      break;
    }
    if (futex((volatile int *)&mLock, FUTEX_UNLOCK_PI | FUTEX_PRIVATE_FLAG, 0, NULL, NULL, 0) == 0) {
      break;
    }
  }
}
