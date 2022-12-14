/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event dispatcher file
 *
 */

#ifndef MISIGHT_BASE_EVENT_DISPATCHER_H
#define MISIGHT_BASE_EVENT_DISPATCHER_H

#include<list>
#include <vector>

#include <utils/StrongPointer.h>

#include "define_util.h"
#include "event.h"
#include "plugin.h"


namespace android {
namespace MiSight {
class EventDispatcher : public Plugin {
public:
    explicit EventDispatcher(Event::ManageType type) : type_(type), exit_(false) {}
    ~EventDispatcher();
    EVENT_RET OnEvent(sp<Event>& event) override;
    void OnLoad() override;
    void OnUnload() override;
    void Dump(int fd, const std::vector<std::string>& cmds) override;

    void RegisterListener(sp<EventListener> listener);
private:
    void PostUnOrderedEvent(sp<Event> event);
    bool IsEventListened(sp<Event> event, sp<EventListener> listener);
    Event::ManageType type_;
    bool exit_;
    std::list<sp<EventListener>> eventListeners_;
    std::mutex mutexLock_;

};
} // namespace MiSight
} // namespace android
#endif

