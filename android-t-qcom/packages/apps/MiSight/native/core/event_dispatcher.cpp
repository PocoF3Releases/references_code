/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event dispatcher implements file
 *
 */
#include "event_dispatcher.h"

#include "log.h"

namespace android {
namespace MiSight {
EventDispatcher::~EventDispatcher()
{
}

EVENT_RET EventDispatcher::OnEvent(sp<Event>& event)
{
    {
        std::unique_lock<std::mutex> lock(mutexLock_);
        if (exit_) {
            return EVENT_RET::ON_FAILURE;
        }
    }
    if (event == nullptr || type_ != event->processType_) {
        MISIGHT_LOGE("unsupported event manage type, %d", (event == nullptr ? -1 : event->processType_));
        return EVENT_RET::ON_FAILURE;
    }

    PostUnOrderedEvent(event);
    return EVENT_RET::ON_SUCCESS;
}

void EventDispatcher::PostUnOrderedEvent(sp<Event> event)
{
    std::list<sp<EventListener>> listeners;
    {
        std::unique_lock<std::mutex> lock(mutexLock_);
        for (const auto& listener : eventListeners_) {
            if (IsEventListened(event, listener)) {
                MISIGHT_LOGD("listener %s", listener->GetListenerName().c_str());
                listeners.push_back(listener);
            }
        }
    }

    for (const auto& listener : listeners) {
        MISIGHT_LOGD("listener %s", listener->GetListenerName().c_str());
        listener->OnUnorderedEvent(event);
    }
    listeners.clear();
}

bool EventDispatcher::IsEventListened(sp<Event> event, sp<EventListener> listener)
{
    std::set<EventListener::EventRange> events;
    if (listener->GetListenerInfo(event->messageType_, events)) {
        return std::any_of(events.begin(), events.end(), [&](const EventListener::EventRange& range) {
            return ((event->eventId_ >= range.begin) && (event->eventId_ <= range.end));
        });
    }
    MISIGHT_LOGD("get events not find size = %d", (int)events.size());
    return false;
}


void EventDispatcher::OnLoad()
{
    exit_ = false;
}

void EventDispatcher::OnUnload()
{
    {
        std::unique_lock<std::mutex> lock(mutexLock_);
        if (exit_) {
            return;
        }
        exit_ = true;
    }
    auto looper = GetLooper();
    if (looper != nullptr) {
        looper->StopLoop();
    }
}

void EventDispatcher::Dump(int fd, const std::vector<std::string>& cmds)
{
    printf("%d, %d", fd, (int)cmds.size());
}

void EventDispatcher::RegisterListener(sp<EventListener> listener)
{
    std::unique_lock<std::mutex> lock(mutexLock_);
    MISIGHT_LOGD("register listenr %s", listener->GetListenerName().c_str());
    eventListeners_.push_back(std::move(listener));
}

} // namespace MiSight
} // namespace android

