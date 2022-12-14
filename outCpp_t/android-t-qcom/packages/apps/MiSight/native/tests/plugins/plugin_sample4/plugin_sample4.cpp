/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample4 implement file
 *
 */

#include "plugin_sample4.h"

#include "common.h"
#include "plugin_factory.h"

namespace android {
namespace MiSight {
REGISTER_PLUGIN(PluginSample4);
bool PluginSample4::CanProcessEvent(sp<Event> event)
{
    if (event->messageType_ == Event::MessageType::FAULT_EVENT &&
        event->eventId_ == EVENT_902) {
        printf("PluginSample4 CanProcessEvent true.\n");
        return true;
    }
    return false;
}

EVENT_RET PluginSample4::OnEvent(sp<Event>& event)
{
    printf("PluginSample4 OnEvent  tid:%d sender:%s %d.\n", gettid(), event->sender_.c_str(), event->eventId_);
    std::string eventIdStr = std::to_string(event->eventId_);
    event->SetValue(eventIdStr, event->GetValue(eventIdStr) + "-PluginSample4");
    event->SetValue("PluginSample4", "Done");
    eventInfo_ = event->GetValue(eventIdStr);
    printf("PluginSample4 OnEvent  tid:%d sender:%s %d %s.\n", gettid(), event->sender_.c_str(), event->eventId_, eventInfo_.c_str());
    return ON_SUCCESS;
}
void PluginSample4::OnLoad()
{
    SetName("PluginSample4");
    SetVersion("PluginSample4.0");
    printf("PluginSample4 OnLoad \n");
    loaded_ = true;
    GetPluginContext()->RegisterPipelineListener("PipelineName3", this);
}

void PluginSample4::OnUnload()
{
    printf("PluginSample4 OnUnload \n.");
}
}
}
