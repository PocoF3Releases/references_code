#pragma once

#include "common/Common.h"

namespace android {

namespace syspressure {

class ThreadPool
{
public:
    using Task = std::function<void()>;

    // TODO: add more feature to enable pre-start, max threads, etc...
    ThreadPool(uint32_t threads);
    ~ThreadPool();
    int enqueue(Task task);

private:
    void loop();
    // need to keep track of threads so we can join them
    std::vector<std::thread> mWorkers;
    // the task queue
    std::queue<Task> mTasks;
    // synchronization
    std::mutex mMutex;
    std::condition_variable mCond;
    bool mStop;
};

} // namespace syspressure

} // namespace android

