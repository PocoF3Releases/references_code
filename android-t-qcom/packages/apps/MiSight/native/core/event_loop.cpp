/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event loop implements
 *
 */

#include "event_loop.h"

#include <chrono>
#include <climits>
#include <unistd.h>


#include "log.h"


namespace android {
namespace MiSight {
EventLoop::EventLoop(const std::string &name)
    : name_(name), thread_(nullptr), looper_(new Looper(false)), currentEvent_(nullptr)
{

}

EventLoop::~EventLoop()
{
    MISIGHT_LOGD("name :%s exit", name_.c_str());
    StopLoop();
}

void EventLoop::StartLoop()
{
    if (thread_) {
        MISIGHT_LOGE("name :%s thread already exist", name_.c_str());
        return;
    }
    thread_ = new ThreadImpl(name_, std::bind(&EventLoop::ProcessEventLoop, this));
    int32_t ret = thread_->run(name_.c_str());
    MISIGHT_LOGI("%s, run ret=%d", name_.c_str(), ret);
}

void EventLoop::StopLoop()
{
    if (thread_ == nullptr) {
        return;
    }

    thread_->requestExit();
    WakeUp();
    thread_->requestExitAndWait();
}

void EventLoop::ProcessEventLoop()
{
    uint64_t leftTimeNanosecond = INT_MAX;
    while (!eventQueue_.empty()) {
        auto now = TimeUtil::GetNanoSecondSinceSystemStart();
        LoopEvent event;
        if (!FetchNextEvent(now, leftTimeNanosecond, event)) {
            break;
        }
        auto begin = TimeUtil::GetTimestampUS();
        ProcessEvent(event);
        auto end = TimeUtil::GetTimestampUS();
        if ((end-begin)/MS_TO_US > 300) {
            MISIGHT_LOGD("onevent name %s  consume time %dms", name_.c_str(), (int)((end-begin)/MS_TO_US));
        }

        ReEnqueueEvent(event);
        std::lock_guard<std::mutex> lock(queueMutex_);
        currentEvent_.store(nullptr, std::memory_order_relaxed);
    }
    uint64_t leftTimeMill = INT_MAX;
    if (leftTimeNanosecond != leftTimeMill) {
        leftTimeMill = (leftTimeNanosecond / MS_TO_NS);
    }
    looper_->pollOnce(leftTimeMill);
}

bool EventLoop::FetchNextEvent(uint64_t now, uint64_t& leftNs, LoopEvent& eventOut)
{
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (eventQueue_.empty()) {
        return false;
    }

    const LoopEvent &event = eventQueue_.top();
    if (event.targetTime > now) {
        leftNs = event.targetTime - now;
        return false;
    }

    eventOut = event;
    eventQueue_.pop();
    currentEvent_.store(&eventOut, std::memory_order_relaxed);
    return true;

}

void EventLoop::ProcessEvent(LoopEvent &event)
{
    if (event.taskType == LOOP_EVENT_ASYNC) {
        if (event.task != nullptr) {
            event.task();
        } else if ((event.handler != nullptr) && (event.event != nullptr)) {
            event.handler->OnEvent(event.event);
        } else {
            MISIGHT_LOGW("Loop event not execute, no task or handler %s", name_.c_str());
        }
    } else if (event.taskType == LOOP_EVENT_SYNC) {
        if (event.syncTask != nullptr) {
            event.syncTask->operator()();
        } else {
            MISIGHT_LOGW("Loop event not execute, no sync task %s", name_.c_str());
        }
    } else {
        MISIGHT_LOGW("unrecognized task type. %s",name_.c_str());
    }
}

void EventLoop::ReEnqueueEvent(LoopEvent &event)
{
    if (event.seq == 0) {
        return;
    }
    if (!event.isRepeat || event.interval == 0) {
        return;
    }

    auto now = TimeUtil::GetNanoSecondSinceSystemStart();
    event.enqueueTime = now;
    event.targetTime = now + event.interval;

    std::lock_guard<std::mutex> lock(queueMutex_);
    eventQueue_.push(event);
}


int64_t EventLoop::AddEvent(sp<EventHandler> handler, sp<Event> event, uint64_t interval, bool repeat)
{
    if (handler == nullptr || event == nullptr) {
        MISIGHT_LOGE("handler or event is nullptr");
        return -1;
    }
    LoopEvent loopEvent = LoopEvent(repeat, interval);
    loopEvent.event = std::move(event);
    loopEvent.handler = handler;

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        eventQueue_.push(loopEvent);
    }
    WakeUp();
    return loopEvent.seq;

}

bool EventLoop::AddEventWaitResult(sp<EventHandler> handler, sp<Event> event)
{
    if (handler == nullptr || event == nullptr) {
        MISIGHT_LOGE("handler or event is nullptr");
        return false;
    }
    auto bind = std::bind(&EventHandler::OnEvent, handler.get(), event);
    std::shared_ptr<std::packaged_task<EVENT_RET()>> task = std::make_shared<std::packaged_task<EVENT_RET()>>(bind);
    auto result = task->get_future();
    LoopEvent loopEvent = LoopEvent();
    loopEvent.taskType = LOOP_EVENT_SYNC;
    loopEvent.event = std::move(event);
    loopEvent.handler = handler;
    loopEvent.syncTask = std::move(task);

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        eventQueue_.push(loopEvent);
    }
    WakeUp();
    EVENT_RET ret = result.get();
    if (ret == ON_FAILURE) {
        return false;
    }
    return true;
}


int64_t EventLoop::AddTask(const Task task, uint64_t interval, bool repeat)
{
    LoopEvent loopEvent = LoopEvent(repeat, interval);
    loopEvent.task = task;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        eventQueue_.push(loopEvent);
    }
    WakeUp();
    return loopEvent.seq;
}

void EventLoop::RemoveEventOrTask(int64_t seq)
{
    std::lock_guard<std::mutex> lock(queueMutex_);
    auto curEvent = currentEvent_.load(std::memory_order_relaxed);
    if ((curEvent != nullptr) && (curEvent->seq == seq)) {
        curEvent->seq = 0;
        MISIGHT_LOGD("removing the current processing event.");
        return;
    }
    eventQueue_.remove(seq);
}

bool EventLoop::AddFileEventHandler(const std::string& name, sp<FileEventHandler> fileHandler)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (fileFdMap_.size() >= (MAX_WATCHED_FDS - 1)) {
            MISIGHT_LOGW("watched fds exceed MAX_WATCHED_FDS %s", name_.c_str());
            return false;
        }

        if (fileFdMap_.find(name) != fileFdMap_.end()) {
            MISIGHT_LOGW("exist fd callback with same name %s", name_.c_str());
            return false;
        }

        int32_t fd = fileHandler->GetPollFd();
        if (fd <= 0) {
            MISIGHT_LOGW("Invalid poll fd, %s", name_.c_str());
            return false;
        }

        fileFdMap_[name] = fd;
        fileHandlerMap_[fd] = fileHandler;
        looper_->addFd(fd, 0, fileHandler->GetPollEventType(), OnFileEventReceiver, this);
        MISIGHT_LOGD("add fd %s %s fd=%d  loper =%p,fileHandler=%p",name_.c_str(),name.c_str(),fd, this,fileHandler.get());
    }
    WakeUp();
    return true;
}

void EventLoop::RemoveFileEventHandler(const std::string& name)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (fileFdMap_.find(name) == fileFdMap_.end()) {
            MISIGHT_LOGW("not find file event Handler %s", name_.c_str());
            return;
        }

        int32_t fd = fileFdMap_[name];
        fileFdMap_.erase(name);
        fileHandlerMap_.erase(fd);
        looper_->removeFd(fd);
    }
    WakeUp();
    return;
}

void EventLoop::RemoveFileEventHandler(int32_t fd)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (fileHandlerMap_.find(fd) == fileHandlerMap_.end()) {
            MISIGHT_LOGW("not find file event Handler %s", name_.c_str());
            return;
        }

        fileHandlerMap_.erase(fd);

        auto iter = fileFdMap_.begin();
        while (iter != fileFdMap_.end()) {
            if (iter->second == fd) {
                fileFdMap_.erase(iter++);
                continue;
            }
            iter++;
        }
        looper_->removeFd(fd);
    }
    WakeUp();
    return;
}


int EventLoop::OnFileEventReceiver(int fd, int events, void* data)
{
    MISIGHT_LOGD("OnFileEventReceiver file handler fd=%d events=%d %p", fd, events, data);
    EventLoop* eventLooper = static_cast<EventLoop*>(data);
    if (eventLooper == nullptr) {
        MISIGHT_LOGW("data is nullptr d=%d events=%d %p", fd, events, data);
        return 0;
    }
    sp<FileEventHandler> fileHandler;
    {
        std::lock_guard<std::mutex> lock(eventLooper->queueMutex_);
        if (eventLooper->fileHandlerMap_.find(fd) == eventLooper->fileHandlerMap_.end()) {
            MISIGHT_LOGW("not find file handler %s", eventLooper->GetName().c_str());
            return 0;
        }
        fileHandler = eventLooper->fileHandlerMap_[fd];
        MISIGHT_LOGD("find file handler fd=%d events=%d fileHandler=%p", fd, events, fileHandler.get());
    }
    if (fileHandler == nullptr) {
        MISIGHT_LOGW("find file handler is nullptr fd=%d events=%d fileHandler=%p", fd, events, fileHandler.get());
        return 0;
    }
    int32_t result = fileHandler->OnFileEvent(fd, events);
    if (result == 0) {
        eventLooper->RemoveFileEventHandler(fd);
    }
    return 1;
}

void EventLoop::WakeUp()
{
    looper_->wake();
}

}// namespace MiSight
}// namespace andrid

