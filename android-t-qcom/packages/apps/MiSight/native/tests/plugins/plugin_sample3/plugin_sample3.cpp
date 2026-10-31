/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample3 implement file
 *
 */
#include "plugin_sample3.h"

#include "common.h"
#include "pipeline.h"
#include "plugin_factory.h"

namespace android {
namespace MiSight {
REGISTER_PLUGIN(PluginSample3);
bool PluginSample3::CanProcessEvent(sp<Event> event)
{
    printf("PluginSample3 CanProcessEvent: %d.\n", event->eventId_);
    if (event->eventId_ == EVENT_903 || event->eventId_ == EVENT_906) {
        printf("PluginSample3 CanProcessEvent true.\n");
        return true;
    }
    return false;
}

EVENT_RET PluginSample3::OnEvent(sp<Event>& event)
{
    printf("PluginSample3 OnEvent  tid:%d sender:%s, %d.\n", gettid(), event->sender_.c_str(), event->eventId_);

    std::string eventIdStr = std::to_string(event->eventId_);
    event->SetValue(eventIdStr, event->GetValue(eventIdStr) + "-PluginSample3");
    printf("PluginSample3 OnEvent  tid:%d sender:%s, %d %s.\n", gettid(), event->sender_.c_str(), event->eventId_, event->GetValue(eventIdStr).c_str());
    return ON_SUCCESS;


}

void PluginSample3::OnLoad()
{
    SetName("PluginSample3");
    SetVersion("PluginSample3.0");
    printf("PluginSample3 OnLoad\n");
    loaded_ = true;
}

void PluginSample3::OnUnload()
{
    printf("PluginSample3 OnUnload \n");
}
}
}