/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event loop head file
 *
 */

#ifndef MISIGHT_EVENT_LOOP_H
#define MISIGHT_EVENT_LOOP_H

#include <algorithm>
#include <atomic>
#include <deque>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

#include <utils/Thread.h>
#include <utils/Looper.h>

#include "event.h"
#include "log.h"
#include "time_util.h"

namespace android {
namespace MiSight {
constexpr uint32_t MAX_WATCHED_FDS = 64;

using Task = std::function<void()>;
enum LoopEventType {
    LOOP_EVENT_ASYNC,
    LOOP_EVENT_SYNC,
};

struct LoopEvent {
    bool isRepeat = false;
    LoopEventType taskType = LOOP_EVENT_ASYNC;
    uint64_t interval = 0;
    uint64_t seq = 0;
    uint64_t enqueueTime = 0;
    uint64_t targetTime = 0;
    sp<Event> event = nullptr;
    sp<EventHandler> handler = nullptr;
    Task task = nullptr;
    std::shared_ptr<std::packaged_task<EVENT_RET()>> syncTask = nullptr;

    LoopEvent(bool repeat = false, uint64_t period = 0)
        : isRepeat(repeat), taskType(LOOP_EVENT_ASYNC), interval(period * S_TO_NS)
    {
        auto now = TimeUtil::GetNanoSecondSinceSystemStart();
        seq = now;
        enqueueTime = now;
        targetTime = now;
        if (interval != 0) {
            targetTime += interval;
        }
    }

    bool operator<(const LoopEvent &obj) const
    {
        return (this->targetTime > obj.targetTime);
    }

};

class ThreadImpl : public Thread {
public:
    explicit ThreadImpl(const std::string& name, Task treadLoop) : Thread(false), name_(name), threadLoop_(treadLoop)
    {
    }
    ~ThreadImpl()
    {
    }


private:
    std::string name_;
    Task threadLoop_;

    bool threadLoop() override {
        threadLoop_();
        return true;
    }
};

class EventLoopQueue : public std::priority_queue<LoopEvent, std::deque<LoopEvent>> {
public:
    void remove(uint64_t seq)
    {
        auto it = std::find_if(this->c.begin(), this->c.end(), [seq](LoopEvent event) {
            return event.seq == seq;
        });

        if (it != this->c.end()) {
            this->c.erase(it);
        }
    }
};

class FileEventHandler : public virtual RefBase {
public:
    /* return 0 will close monitor fd, else monitor fd */
    virtual int32_t OnFileEvent(int fd, int Type) = 0;
    virtual int32_t GetPollFd() = 0;
    virtual int32_t GetPollEventType() = 0;
};

class EventLoop : public RefBase {
public:
    explicit EventLoop(const std::string &name);
    virtual ~EventLoop();
    void StartLoop();
    void StopLoop();
    int64_t AddEvent(sp<EventHandler> handler, sp<Event> event, uint64_t interval = 0, bool repeat = false);
    bool AddEventWaitResult(sp<EventHandler> handler, sp<Event> event);
    /*
     * interval : unit s
     * repeat   : repeat execute task
     */
    int64_t AddTask(const Task task, uint64_t interval = 0, bool repeat = false);

    void RemoveEventOrTask(int64_t seq);
    bool AddFileEventHandler(const std::string& name, sp<FileEventHandler> fileHandler);
    void RemoveFileEventHandler(const std::string &name);

    const std::string GetName()
    {
        return name_;
    }
private:
    void WakeUp();
    void ProcessEventLoop();
    bool FetchNextEvent(uint64_t now, uint64_t& leftNs, LoopEvent& eventOut);
    void ProcessEvent(LoopEvent &event);
    void ReEnqueueEvent(LoopEvent &event);
    static int OnFileEventReceiver(int fd, int events, void* data);
    void RemoveFileEventHandler(int32_t fd);
    std::string name_;
    sp<ThreadImpl> thread_;
    sp<Looper> looper_;
    std::atomic<LoopEvent *> currentEvent_;
    std::mutex queueMutex_;
    EventLoopQueue eventQueue_;
    std::map<int32_t, sp<FileEventHandler>> fileHandlerMap_;
    std::map<std::string, int32_t> fileFdMap_;

};
}// namespace MiSight
}// namespace andrid
#endif

