#include "common/ThreadPool.h"

namespace android {

namespace syspressure {

// the constructor just launches some amount of workers
// TODO: add more feature to enable pre-start, max threads, etc...
ThreadPool::ThreadPool(uint32_t threads) : mStop(false) {
    for (size_t i = 0; i < threads; ++i)
        mWorkers.emplace_back([this]() { loop(); });
}

void ThreadPool::loop() {
    while (true) {
        Task task;
        std::unique_lock<std::mutex> locker(mMutex);

        mCond.wait(locker, [this]() { return mStop || !mTasks.empty(); });

        if (mStop && mTasks.empty())
            return;

        task = std::move(mTasks.front());
        mTasks.pop();
        locker.unlock();

        task();
    }
}

int ThreadPool::enqueue(Task task) {
    {
        std::lock_guard<std::mutex> locker(mMutex);
        if (mStop) {
            // FIXME: assign a more readable  error code and give some log
            return -1;
        }

        mTasks.emplace(task);
    }

    mCond.notify_one();
    return 0;
}

// the destructor joins all threads
ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> locker(mMutex);
        mStop = true;
    }
    mCond.notify_all();

    for (auto &worker : mWorkers)
        worker.join();
}

} // namespace syspressure

} // namespace android
