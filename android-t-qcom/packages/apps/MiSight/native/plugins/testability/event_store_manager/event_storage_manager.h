/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event store manager plugin head file
 *
 */
#ifndef MISIGH_PLUGIN_TESTABILITY_EVENT_STORE_MANAGER_H
#define MISIGH_PLUGIN_TESTABILITY_EVENT_STORE_MANAGER_H

#include <string>

#include <utils/StrongPointer.h>

#include "event.h"
#include "event_database_helper.h"
#include "plugin.h"


namespace android {
namespace MiSight {

class EventStorageManager : public Plugin {
public:
    EventStorageManager();
    ~EventStorageManager();

    void OnLoad() override;
    void OnUnload() override;
    EVENT_RET OnEvent(sp<Event> &event) override;
    void Dump(int fd, const std::vector<std::string>& cmds) override;

private:
    void CheckEventStore();
    void SaveUserActivate(sp<Event> &event);
    int GetStoreProperity(const std::string& propName, int defVal);
    sp<EventDatabaseHelper> eventDb_;
};
}
}
#endif

