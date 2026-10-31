/*
 * Copyright (C) 2021 XIAOMI Technologies, Inc.
 */

#include "heap.h"
#include "runtime.h"
#include "task_processor.h"

namespace art {
namespace gc {

class Heap;

// Minimum amount of remaining bytes before a concurrent GC is triggered.
static constexpr size_t kMiMinConcurrentRemainingBytes = 128 * KB;
// Use Max heap for 2 seconds, this is smaller than the usual 5s window since we don't want to leave
// allocate with relaxed ergonomics for that long.
static constexpr size_t kMiPostForkMaxHeapDurationMS = 2000;

class Heap::DeferGcTask : public HeapTask {
 public:
  explicit DeferGcTask(uint64_t target_time) : HeapTask(target_time) {}
  void Run(Thread* self) override {
    gc::Heap* heap = Runtime::Current()->GetHeap();
    // Trigger a GC, when we growfootprint, will adjust the thresholds to normal levels.
    if (heap->target_footprint_.load(std::memory_order_relaxed) == heap->growth_limit_) {
      LOG(INFO) << "DeferGcTask Do gc";
      heap->RequestConcurrentGC(self, kGcCauseBackground, false, heap->GetCurrentGcNum());
    }
  }
};

void Heap::GrowFootprint() {
  size_t orig_target_footprint = target_footprint_.load(std::memory_order_relaxed);
  if (orig_target_footprint < capacity_) {
    target_footprint_.compare_exchange_strong(orig_target_footprint,
                                              capacity_,
                                              std::memory_order_relaxed);
    concurrent_start_bytes_ = UnsignedDifference(capacity_, kMiMinConcurrentRemainingBytes);
  }
  LOG(INFO) << "GrowFootprint target_footprint_: " << target_footprint_
  << " concurrent_start_bytes_: " << concurrent_start_bytes_;
  Thread* self = Thread::Current();
  GetTaskProcessor()->AddTask(
        self, new DeferGcTask(NanoTime() + MsToNs(kMiPostForkMaxHeapDurationMS)));
}

}  // namespace gc
}  // namespace art
