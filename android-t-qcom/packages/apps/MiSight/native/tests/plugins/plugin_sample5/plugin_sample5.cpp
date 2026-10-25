/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample5 implement file
 *
 */

#include "plugin_sample5.h"

#include "common.h"
#include "plugin_factory.h"

namespace android {
namespace MiSight {
REGISTER_PLUGIN(PluginSample5);
bool PluginSample5::CanProcessEvent(sp<Event> event)
{
    if (event->messageType_ == Event::MessageType::FAULT_EVENT &&
        event->eventId_ == EVENT_902) {
        printf("PluginSample5 CanProcessEvent true.\n");
        return true;
    }
    return false;
}

EVENT_RET PluginSample5::OnEvent(sp<Event>& event)
{
    printf("PluginSample5 OnEvent  tid:%d sender:%s %d.\n", gettid(), event->sender_.c_str(), event->eventId_);
    std::string eventIdStr = std::to_string(event->eventId_);
    event->SetValue(eventIdStr, event->GetValue(eventIdStr) + "-PluginSample5");
    event->SetValue("PluginSample5", "Done");
    eventInfo_ = event->GetValue(eventIdStr);
    printf("PluginSample5 OnEvent  tid:%d sender:%s %d %s.\n", gettid(), event->sender_.c_str(), event->eventId_, eventInfo_.c_str());
    return ON_SUCCESS;
}
void PluginSample5::OnLoad()
{
    SetName("PluginSample5");
    SetVersion("PluginSample5.0");
    printf("PluginSample5 OnLoad \n");
    loaded_ = true;
}

void PluginSample5::OnUnload()
{
    printf("PluginSample5 OnUnload \n.");
}

void PluginSample5::OnUnorderedEvent(sp<Event> event)
{

}

}
}
