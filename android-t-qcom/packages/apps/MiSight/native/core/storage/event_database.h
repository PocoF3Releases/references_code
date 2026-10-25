/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event database head file
 *
 */
#ifndef MISIGH_PLUGIN_COMMON_EVENT_DATABASE_USED_H
#define MISIGH_PLUGIN_COMMON_EVENT_DATABASE_USED_H
#include <list>
#include <map>
#include <string>
#include <vector>

#include <utils/Singleton.h>
#include <utils/StrongPointer.h>

#include "event.h"
#include "event_table.h"
#include "event_query.h"
#include "sql_database.h"
#include "sql_field.h"

namespace android {
namespace MiSight {

class EventDatabase : public Singleton<EventDatabase> {
protected:
    EventDatabase();
    ~EventDatabase();
public:
    bool InitDatabase(const std::string& dbPath);
    int32_t InsertAndGetID(const std::string& tblName,
        std::vector<std::string> names, std::vector<std::string> values);
    bool Insert(const std::string& tblName, std::vector<std::string> names,
        std::vector<std::string> values);
    std::vector<FieldValueMap> Query(const std::string& tblName,
        EventQuery query, int limit = 1);
    bool Update(const std::string& tblName, std::vector<std::string> names,
        std::vector<std::string> values, EventQuery query);
    bool Delete(const std::string& tblName, EventQuery query);
    bool TruncateAndCreate(const std::string& tblName);
    sp<BaseTable> GetTable(const std::string& tblName);
    void Close();
private:
    friend class Singleton<EventDatabase>;
    bool status_;
    sp<SqlDatabase> sqlDb_;
    std::map<std::string, sp<BaseTable>> tables_;
};
}
}
#endif
