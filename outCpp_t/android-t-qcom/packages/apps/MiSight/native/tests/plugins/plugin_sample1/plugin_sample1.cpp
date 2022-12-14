/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample1 implement file
 *
 */

#include "plugin_sample1.h"

#include "common.h"

#include "plugin_factory.h"

namespace android {
namespace MiSight {
REGISTER_PLUGIN(PluginSample1);
bool PluginSample1::CanProcessEvent(sp<Event> event)
{
    printf("PluginSample1 CanProcessEvent.\n");
    if (event == nullptr) {
        return false;
    }

    if (event->messageType_ == Event::MessageType::FAULT_EVENT &&
        event->eventId_ == EVENT_901) {
        printf("PluginSample1 CanProcessEvent true.\n");
        return true;
    }
    printf("PluginSample1 CanProcessEvent false. %d %d\n",event->messageType_,event->eventId_ );
    return false;
}

EVENT_RET PluginSample1::OnEvent(sp<Event>& event)
{
    printf("PluginSample1 OnEvent, tid:%d, id=%d\n", gettid(), event->eventId_);
    std::string eventIdStr = std::to_string(event->eventId_);
    event->SetValue(eventIdStr, event->GetValue(eventIdStr) + "-PluginSample1");
    printf("PluginSample1 OnEvent, tid:%d, id=%d, %s\n", gettid(), event->eventId_, event->GetValue(eventIdStr).c_str());
    return ON_SUCCESS;
}

void PluginSample1::OnLoad()
{
    SetName("PluginSample1");
    SetVersion("PluginSample1.0");
    printf("PluginSample1 OnLoad \n");
    loaded_ = true;

    EventRange range(EVENT_903);
    AddListenerInfo(Event::MessageType::FAULT_EVENT, range);
    EventRange range7(EVENT_907);
    AddListenerInfo(Event::MessageType::FAULT_EVENT, range7);

    std::set<EventRange> rangeSet;
    rangeSet.insert(EVENT_909);
    rangeSet.insert(EVENT_910);
    rangeSet.insert(EVENT_911);
    AddListenerInfo(Event::MessageType::FAULT_EVENT, rangeSet);
    GetPluginContext()->RegisterUnorderedEventListener(this);
}

void PluginSample1::OnUnload()
{
    printf("PluginSample1 OnUnload \n");
}

void PluginSample1::OnUnorderedEvent(sp<Event> event)
{
    recvUnorderCnt_++;
    printf("PluginSample1 receive unorder event %d\n", event->eventId_);
}

}
}
