/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event store database head file
 *
 */
#ifndef MISIGH_PLUGIN_TESTABILITY_EVENT_DATABASE_HELPER_H
#define MISIGH_PLUGIN_TESTABILITY_EVENT_DATABASE_HELPER_H

#include <string>

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>


#include "event.h"
#include "event_quota_xml_parser.h"
#include "plugin.h"
#include "sql_database.h"
#include "sql_field.h"

namespace android {
namespace MiSight {

class EventDatabaseHelper : public RefBase {
public:
    explicit EventDatabaseHelper(uint64_t startTime);
    ~EventDatabaseHelper();
    void InitDatabase(PluginContext* context, const std::string& dbName, const std::string& version);
    void AbridgeDbSize();
    void ProcessFaultEvent(sp<Event> event);
    void SaveRawEvent(sp<Event> event);
    void SaveUserActivateTime(sp<Event> event);
    void DumpQueryDb(int fd, const std::vector<std::string>& cmds);
    void DumpUpdateDb(int fd, const std::vector<std::string>& cmds);

private:
    void InitDBVersion();
    void SetDbStartTime();
    bool DetectDropByTime(sp<Event> event, FieldValueMap filedMap);
    bool DetectCanDrop(sp<Event> event);
    bool ProcessNewDayEvent(sp<Event> event);
    void DumpTable(int fd, const std::string& tblName);

    bool isUpdate_;
    int32_t versionId_;
    int32_t dbId_;
    uint64_t startTime_;
    PluginContext* platformContext_;
    std::string curVersion_;
};
}
}
#endif

