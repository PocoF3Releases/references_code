/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event store manager plugin head file
 *
 */
#ifndef MISIGH_PLUGIN_TESTABILITY_EVENT_INFO_PACK_PLUGIN_H
#define MISIGH_PLUGIN_TESTABILITY_EVENT_INFO_PACK_PLUGIN_H

#include <string>

#include <utils/StrongPointer.h>

#include "event.h"
#include "plugin.h"


namespace android {
namespace MiSight {

class EventInfoPack : public Plugin {
public:
    EventInfoPack();
    ~EventInfoPack();

    void OnLoad() override;
    void OnUnload() override;
    EVENT_RET OnEvent(sp<Event> &event) override;

private:
    void ProcessEventInfo(sp<Event> &event);
    std::string GetEventFilePath(sp<Event> event, bool &truncate);
};
}
}
#endif
