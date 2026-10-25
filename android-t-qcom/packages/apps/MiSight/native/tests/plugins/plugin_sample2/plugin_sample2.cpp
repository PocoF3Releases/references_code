/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample2 implement file
 *
 */

#include "plugin_sample2.h"

#include "common.h"
#include "pipeline.h"
#include "plugin_factory.h"

namespace android {
namespace MiSight {
REGISTER_PLUGIN(PluginSample2)
bool PluginSample2::CanProcessEvent(sp<Event> event)
{
    printf("PluginSample2 %d \n", event->eventId_);
    return false;
}

EVENT_RET PluginSample2::OnEvent(sp<Event>& event)
{
    printf("PluginSample2 process %d event.\n", event->eventId_);
    std::string eventIdStr = std::to_string(event->eventId_);
    event->SetValue(eventIdStr, event->GetValue(eventIdStr) + "-PluginSample2");
    if (event->eventId_ == EVENT_904) {
        GetPluginContext()->PostAsyncEventToTarget(this, "PluginSample1", event);
    } else if (event->eventId_ == EVENT_905) {
        bool ret = GetPluginContext()->PostSyncEventToTarget(this, "PluginSample1", event);
        printf("send sync and wait result = %d\n", ret);
    } else if (event->eventId_ == EVENT_903 || event->eventId_ == EVENT_907
           || (event->eventId_ >= EVENT_909 && event->eventId_ <= EVENT_912)) {
        printf("PluginSample2 will broadcast event \n");
        GetPluginContext()->PostUnorderedEvent(this,  event);
    }
    printf("PluginSample2 process %d event %s \n", event->eventId_, event->GetValue(eventIdStr).c_str());
    return ON_SUCCESS;
}
void PluginSample2::OnLoad()
{
    SetName("PluginSample2");
    SetVersion("PluginSample2.0");
    printf("PluginSample2 OnLoad \n");
    loaded_ = true;
}

void PluginSample2::OnUnload()
{
    printf("PluginSample2 OnUnload \n");
}
}
}
