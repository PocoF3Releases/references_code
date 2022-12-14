#include <stdlib.h>
#include <algorithm>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>

#include "ExecutingRootEvents.h"
#include "OsUtils.h"
#include "FastSystemInfoFetcher.h"
#include "KernelPerfEventsReader.h"
#include "PerfSuperviser.h"
#include "PILock.h"

namespace android {
namespace os {
namespace statistics {

#define BUFFER_SIZE 256

#ifndef container_of
#define container_of(ptr, type, member) \
  ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

struct effective_interval {
  int64_t earliest_begin_uptimemillis;
  int64_t latest_end_uptimemillis;
  int64_t user_perceptible_count;
};

struct RootEventsListNode {
  struct RootEventsListNode* next;
  struct RootEventsListNode* prev;
};

struct ExecutingRootEvent {
  struct RootEventsListNode listNode;
  struct RootEventsListNode masterListNode;
  bool isIdle;
  bool isMasterEvent;
  bool isUserPerceptible;
  int64_t beginUptimeMillis;
};

static PILock *gLock;
static atomic_int_fast64_t gEarliestBeginUptimeMillis;
static atomic_int_fast64_t gLatestEndUptimeMillis;
static atomic_int_fast64_t gEarliestMasterBeginUptimeMillis;
static atomic_int_fast64_t gLatestMasterEndUptimeMillis;
static atomic_int_fast32_t gUserPerceptibleMasterEventCount;
static struct RootEventsListNode gIdleListHead;
static struct RootEventsListNode gExecutingRootEventsHead;
static struct RootEventsListNode gExecutingMasterRootEventsHead;

static inline bool isEmtpyList(struct RootEventsListNode *head) {
  return head->next == head;
}

static inline void addToListTail(struct RootEventsListNode *head, struct RootEventsListNode *item) {
  item->next = head;
  item->prev = head->prev;
  head->prev->next = item;
  head->prev = item;
}

static inline void insertToList(struct RootEventsListNode *prev, struct RootEventsListNode *item) {
  item->next = prev->next;
  item->prev = prev;
  prev->next->prev = item;
  prev->next = item;
}

static inline void detachFromList(struct RootEventsListNode *item) {
  item->next->prev = item->prev;
  item->prev->next = item->next;
  item->prev = item;
  item->next = item;
}

static inline void addToIdleListTail(struct ExecutingRootEvent* item) {
  item->isIdle = true;
  addToListTail(&gIdleListHead, &item->listNode);
}

static inline struct ExecutingRootEvent* fetchFromIdleList() {
  struct ExecutingRootEvent* item;
  if (isEmtpyList(&gIdleListHead)) {
    item = NULL;
  } else {
    item = container_of(gIdleListHead.next, struct ExecutingRootEvent, listNode);
    detachFromList(&item->listNode);
    item->isIdle = false;
  }
  return item;
}

#define SET_EFFECTIVE_PERFEVENTS_INTERVAL _IOW('p', 1, struct effective_interval)
#define GET_DEVICE_EFFECTIVE_PERFEVENTS_INTERVAL _IOR('p', 1, struct effective_interval)

static inline int setEffectivePerfEventsInterval() {
  KernelPerfEventsReader* reader = KernelPerfEventsReader::getProcKernelPerfEventsReader();
  if (reader == NULL) {
    return 0;
  }
  struct effective_interval interval;
  interval.earliest_begin_uptimemillis = (int64_t)atomic_load_explicit(&gEarliestMasterBeginUptimeMillis, memory_order_relaxed);
  interval.latest_end_uptimemillis = (int64_t)atomic_load_explicit(&gLatestMasterEndUptimeMillis, memory_order_relaxed);
  interval.user_perceptible_count = (int64_t)atomic_load_explicit(&gUserPerceptibleMasterEventCount, memory_order_relaxed);
  return ioctl(reader->getfd(), SET_EFFECTIVE_PERFEVENTS_INTERVAL, &interval);
}

static inline bool getDeviceEffectivePerfEventsInterval(struct effective_interval* interval) {
  KernelPerfEventsReader* reader = KernelPerfEventsReader::getProcKernelPerfEventsReader();
  if (reader == NULL) {
    return false;
  }
  return ioctl(reader->getfd(), GET_DEVICE_EFFECTIVE_PERFEVENTS_INTERVAL, interval) == 0;
}

void ExecutingRootEvents::init() {
  gLock = new PILock();
  atomic_init(&gEarliestBeginUptimeMillis, (int_fast64_t)0);
  atomic_init(&gLatestEndUptimeMillis, (int_fast64_t)0);
  atomic_init(&gEarliestMasterBeginUptimeMillis, (int_fast64_t)0);
  atomic_init(&gLatestMasterEndUptimeMillis, (int_fast64_t)0);
  atomic_init(&gUserPerceptibleMasterEventCount, (int_fast32_t)0);

  struct ExecutingRootEvent* itemsBegin = (struct ExecutingRootEvent*)malloc(sizeof(struct ExecutingRootEvent) * BUFFER_SIZE);
  struct ExecutingRootEvent* itemsEnd = itemsBegin + BUFFER_SIZE;
  gIdleListHead.prev = &gIdleListHead;
  gIdleListHead.next = &gIdleListHead;
  for (struct ExecutingRootEvent* item = itemsBegin; item < itemsEnd; item++) {
    addToIdleListTail(item);
  }

  gExecutingRootEventsHead.prev = &gExecutingRootEventsHead;
  gExecutingRootEventsHead.next = &gExecutingRootEventsHead;
  gExecutingMasterRootEventsHead.prev = &gExecutingMasterRootEventsHead;
  gExecutingMasterRootEventsHead.next = &gExecutingMasterRootEventsHead;
}

static inline void updateGlobalTimestampLocked(
    bool needUpdateEarliestBeginUptime,
    bool needUpdateEarliestMasterBeginUptime) {
  if (needUpdateEarliestBeginUptime) {
    if (isEmtpyList(&gExecutingRootEventsHead)) {
      atomic_store_explicit(&gEarliestBeginUptimeMillis, (int_fast64_t)0, memory_order_relaxed);
    } else {
      struct ExecutingRootEvent* first = container_of(gExecutingRootEventsHead.next, struct ExecutingRootEvent, listNode);
      atomic_store_explicit(&gEarliestBeginUptimeMillis, (int_fast64_t)first->beginUptimeMillis, memory_order_relaxed);
    }
  }

  if (needUpdateEarliestMasterBeginUptime) {
    if (isEmtpyList(&gExecutingMasterRootEventsHead)) {
      atomic_store_explicit(&gEarliestMasterBeginUptimeMillis, (int_fast64_t)0, memory_order_relaxed);
    } else {
      struct ExecutingRootEvent* first = container_of(gExecutingMasterRootEventsHead.next, struct ExecutingRootEvent, masterListNode);
      atomic_store_explicit(&gEarliestMasterBeginUptimeMillis, (int_fast64_t)first->beginUptimeMillis, memory_order_relaxed);
    }
  }
}

uint64_t ExecutingRootEvents::beginRootEvent(int64_t beginUptimeMillis, bool isMasterEvent, bool isUserPerceptible) {
  uint64_t cookie = UINT64_MAX;
  bool needUpdateEarliestBeginUptime = false;
  bool needUpdateEarliestMasterBeginUptime = false;

  struct ExecutingRootEvent* item;

  gLock->lock();

  if (isMasterEvent && isUserPerceptible) {
    atomic_fetch_add(&gUserPerceptibleMasterEventCount, (int_fast32_t)1);
  }

  while (!isEmtpyList(&gExecutingRootEventsHead)) {
    struct ExecutingRootEvent* first = container_of(gExecutingRootEventsHead.next, struct ExecutingRootEvent, listNode);
    if (first->beginUptimeMillis + PerfSuperviser::getsMaxEventThresholdMs() >= beginUptimeMillis) {
      break;
    }
    detachFromList(&first->listNode);
    needUpdateEarliestBeginUptime = true;
    if (first->isMasterEvent) {
      detachFromList(&first->masterListNode);
      needUpdateEarliestMasterBeginUptime = true;
    }
    addToIdleListTail(first);
  }

  while (!isEmtpyList(&gExecutingMasterRootEventsHead)) {
    struct ExecutingRootEvent* first = container_of(gExecutingMasterRootEventsHead.next, struct ExecutingRootEvent, masterListNode);
    if (first->beginUptimeMillis + PerfSuperviser::getsMaxEventThresholdMs() >= beginUptimeMillis) {
      break;
    }
    detachFromList(&first->masterListNode);
    needUpdateEarliestMasterBeginUptime = true;
    addToIdleListTail(first);
  }

  item = fetchFromIdleList();
  if (item != NULL) {
    item->isMasterEvent = isMasterEvent;
    item->isUserPerceptible = isUserPerceptible;
    item->beginUptimeMillis = beginUptimeMillis;

    struct RootEventsListNode* prev = gExecutingRootEventsHead.prev;
    while (prev != &gExecutingRootEventsHead) {
      struct ExecutingRootEvent* prevItem = container_of(prev, struct ExecutingRootEvent, listNode);
      if (prevItem->beginUptimeMillis > beginUptimeMillis) {
        prev = prev->prev;
      } else {
        break;
      }
    }
    insertToList(prev, &item->listNode);
    if (gExecutingRootEventsHead.next == &item->listNode) {
      needUpdateEarliestBeginUptime = true;
    }

    if (isMasterEvent) {
      prev = gExecutingMasterRootEventsHead.prev;
      while (prev != &gExecutingMasterRootEventsHead) {
        struct ExecutingRootEvent* prevItem = container_of(prev, struct ExecutingRootEvent, masterListNode);
        if (prevItem->beginUptimeMillis > beginUptimeMillis) {
          prev = prev->prev;
        } else {
          break;
        }
      }
      insertToList(prev, &item->masterListNode);
      if (gExecutingMasterRootEventsHead.next == &item->masterListNode) {
        needUpdateEarliestMasterBeginUptime = true;
      }
    }
    cookie = (uint64_t)((uintptr_t)item);
  }

  updateGlobalTimestampLocked(needUpdateEarliestBeginUptime, needUpdateEarliestMasterBeginUptime);

  gLock->unlock();

  if (needUpdateEarliestMasterBeginUptime || (isMasterEvent && isUserPerceptible)) {
    int result = setEffectivePerfEventsInterval();
    item->isUserPerceptible = (result > 0);
  }

  return cookie;
}

void ExecutingRootEvents::endRootEvent(
  uint64_t cookie, int64_t beginUptimeMillis, int64_t endUptimeMillis, bool isMasterEvent, bool isUserPerceptible) {
  if (CC_UNLIKELY(cookie == 0)) {
    return;
  }

  bool needUpdateEarliestBeginUptime = false;
  bool needUpdateEarliestMasterBeginUptime = false;
  bool latestMasterEndUptimeChanged = false;

  gLock->lock();

  if (endUptimeMillis - beginUptimeMillis >= PerfSuperviser::getInnerWaitThresholdMs()) {
    if ((int64_t)atomic_load_explicit(&gLatestEndUptimeMillis, memory_order_relaxed) < endUptimeMillis) {
      atomic_store_explicit(&gLatestEndUptimeMillis, (int_fast64_t)endUptimeMillis, memory_order_relaxed);
    }
  }

  if (isMasterEvent && (endUptimeMillis - beginUptimeMillis >= PerfSuperviser::getSoftThresholdMs())) {
    if ((int64_t)atomic_load_explicit(&gLatestMasterEndUptimeMillis, memory_order_relaxed) < endUptimeMillis) {
      atomic_store_explicit(&gLatestMasterEndUptimeMillis, (int_fast64_t)endUptimeMillis, memory_order_relaxed);
    }
    latestMasterEndUptimeChanged = true;
  }

  if (CC_LIKELY(cookie != UINT64_MAX)) {
    struct ExecutingRootEvent* item = (struct ExecutingRootEvent*)((uintptr_t)cookie);
    if (!item->isIdle && item->beginUptimeMillis == beginUptimeMillis) {
      needUpdateEarliestBeginUptime = gExecutingRootEventsHead.next == &item->listNode;
      needUpdateEarliestMasterBeginUptime = item->isMasterEvent && gExecutingMasterRootEventsHead.next == &item->masterListNode;
      detachFromList(&item->listNode);
      if (item->isMasterEvent) {
        detachFromList(&item->masterListNode);
      }
      addToIdleListTail(item);
    }
  }

  updateGlobalTimestampLocked(needUpdateEarliestBeginUptime, needUpdateEarliestMasterBeginUptime);

  if (isMasterEvent && isUserPerceptible) {
    atomic_fetch_sub(&gUserPerceptibleMasterEventCount, (int_fast32_t)1);
  }

  gLock->unlock();

  if (needUpdateEarliestMasterBeginUptime || latestMasterEndUptimeChanged || (isMasterEvent && isUserPerceptible)) {
    setEffectivePerfEventsInterval();
  }
}

bool ExecutingRootEvents::isUserPerceptibleMasterRootEvent(uint64_t cookie, int64_t beginUptimeMillis) {
  if (CC_UNLIKELY(cookie == 0 || cookie == UINT64_MAX)) {
    return false;
  }
  struct ExecutingRootEvent* item = (struct ExecutingRootEvent*)((uintptr_t)cookie);
  if (item->beginUptimeMillis == beginUptimeMillis) {
    return item->isUserPerceptible;
  } else {
    return false;
  }
}

int64_t ExecutingRootEvents::getEarliestBeginUptimeMillis() {
  int64_t earliest;
  struct effective_interval interval;
  if (getDeviceEffectivePerfEventsInterval(&interval)) {
    earliest = interval.earliest_begin_uptimemillis;
  } else {
    earliest = (int64_t)atomic_load_explicit(&gEarliestBeginUptimeMillis, memory_order_relaxed);
  }
  if (earliest == 0) {
    return FastSystemInfoFetcher::getCoarseUptimeMillisFast();
  } else {
    return earliest;
  }
}

bool ExecutingRootEvents::maybeEffectiveMicroscopicEvent(int64_t endUptimeMillis, bool isReportedSynchronously) {
  int64_t earliestBegin;
  int64_t latestEnd;
  struct effective_interval interval;
  if (getDeviceEffectivePerfEventsInterval(&interval)) {
    earliestBegin = interval.earliest_begin_uptimemillis;
    latestEnd = interval.latest_end_uptimemillis;
  } else {
    earliestBegin = (int64_t)atomic_load_explicit(&gEarliestBeginUptimeMillis, memory_order_relaxed);
    latestEnd = (int64_t)atomic_load_explicit(&gLatestEndUptimeMillis, memory_order_relaxed);
  }
  if (earliestBegin > 0 && (endUptimeMillis - earliestBegin) >= PerfSuperviser::getMinOverlappedMs()) {
    return endUptimeMillis - earliestBegin <= PerfSuperviser::getsMaxEventThresholdMs();
  }
  return isReportedSynchronously ? false : latestEnd >= endUptimeMillis;
}

} //namespace statistics
} //namespace os
} //namespace android
