/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event database table head file
 *
 */
#ifndef MISIGH_PLUGIN_COMMON_EVENT_DATABASE_TABLE_USED_H
#define MISIGH_PLUGIN_COMMON_EVENT_DATABASE_TABLE_USED_H

#include <string>
#include <vector>

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

#include "event.h"
#include "sql_database.h"
#include "sql_field.h"

namespace android {
namespace MiSight {
class BaseTable : public RefBase {
public:
    explicit BaseTable(const std::string& name);
    virtual ~BaseTable();
    std::vector<sp<SqlField>> GetFields ()
    {
        return fields_;
    }
    void AddField(sp<SqlField> field);
    std::vector<std::string> GetFieldNameList();

    bool IsFieldExist(const std::string& name);

    std::string GenerateCreateTableStatement();
    std::string BuildInsertStatement(FieldValueMap fieldValue);
    std::string BuildInsertStatement(std::vector<std::string> names, std::vector<std::string> values);
    std::string BuildUpdateStatement(std::vector<std::string> names, std::vector<std::string> values);
    std::string GetName() {
        return name_;
    }

private:
    sp<SqlField> GetField(const std::string& name);
    std::string name_;
    std::vector<sp<SqlField>> fields_;
};

// database events table
class EventTable : public BaseTable {
public:
    EventTable();
    ~EventTable();
    static std::string TBL_NAME;
    static std::string ID;
    static std::string EVENT_ID;
    static std::string TRACE_ID;
    static std::string PSPAN_ID;
    static std::string SPAN_ID;
    static std::string VERSION_ID;
    static std::string OCCURED_CNT;
    static std::string FIRST_TIME;
    static std::string PARAM_LIST;
};

// database eventdaily table
class EventDailyTable : public BaseTable {
public:
    EventDailyTable();
    ~EventDailyTable();
    static std::string TBL_NAME;
    static std::string ID;
    static std::string EVENT_ID;
    static std::string DAILY_CNT;
    static std::string OVER_CNT;
    static std::string HOUR_CNT;
    static std::string NEXT_EXPIRE_TIME;
    static std::string LAST_HAPPEN_TIME;
    static std::string LAST_LOG_TIME;
};

// device version table
class EventVersionTable : public BaseTable {
public:
    EventVersionTable();
    ~EventVersionTable();
    static std::string TBL_NAME;
    static std::string ID;
    static std::string VERSION;
};

//database version table
class EventDBVersionTable : public BaseTable {
public:
    EventDBVersionTable();
    ~EventDBVersionTable();
    static std::string TBL_NAME;
    static std::string ID;
    static std::string DB_VERSION;
    static std::string DEV_VERSION;
    static std::string ACTIVATE_TIME;
    static std::string START_TIME;
};

// upload table, record upload infromation
class EventUploadTable : public BaseTable {
public:
    EventUploadTable();
    ~EventUploadTable();
    static std::string TBL_NAME;
    static std::string ID;
    static std::string YEAR;
    static std::string M1;
    static std::string M2;
    static std::string M3;
    static std::string M4;
    static std::string M5;
    static std::string M6;
    static std::string M7;
    static std::string M8;
    static std::string M9;
    static std::string M10;
    static std::string M11;
    static std::string M12;
};

// raw event table
class RawEventTable : public BaseTable {
public:
    RawEventTable();
    ~RawEventTable();
    static std::string TBL_NAME;
    static std::string ID;
    static std::string EVENT_ID;
    static std::string PARAM_LIST;
    static std::string TIMESTAMP;
    static std::string EXTA_DATA;
};
}
}
#endif

