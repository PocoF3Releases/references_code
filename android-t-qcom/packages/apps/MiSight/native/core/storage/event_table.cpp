/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event database table implements file
 *
 */

#include "event_table.h"


#include "string_util.h"
namespace android {
namespace MiSight {
std::string EventTable::TBL_NAME = "event_info";
std::string EventTable::ID = "id";
std::string EventTable::EVENT_ID = "event_id";
std::string EventTable::TRACE_ID = "trace_id";
std::string EventTable::PSPAN_ID = "pspan_id";
std::string EventTable::SPAN_ID = "span_id";
std::string EventTable::VERSION_ID = "version_id";
std::string EventTable::OCCURED_CNT = "occured_cnt";
std::string EventTable::FIRST_TIME = "first_time";
std::string EventTable::PARAM_LIST = "param_list";

std::string EventDailyTable::TBL_NAME = "event_daily";
std::string EventDailyTable::ID = "id";
std::string EventDailyTable::EVENT_ID = "event_id";
std::string EventDailyTable::DAILY_CNT = "daily_cnt";
std::string EventDailyTable::OVER_CNT = "over_cnt";
std::string EventDailyTable::HOUR_CNT = "hour_cnt";
std::string EventDailyTable::NEXT_EXPIRE_TIME = "next_expire_time";
std::string EventDailyTable::LAST_HAPPEN_TIME = "last_happen_time";
std::string EventDailyTable::LAST_LOG_TIME = "last_log_time";


std::string EventVersionTable::TBL_NAME = "version";
std::string EventVersionTable::ID = "id";
std::string EventVersionTable::VERSION = "version";

std::string EventDBVersionTable::TBL_NAME = "db_version";
std::string EventDBVersionTable::ID = "id";
std::string EventDBVersionTable::DB_VERSION = "db_version";
std::string EventDBVersionTable::DEV_VERSION = "dev_version";
std::string EventDBVersionTable::ACTIVATE_TIME = "activate_time";
std::string EventDBVersionTable::START_TIME = "start_time";

std::string EventUploadTable::TBL_NAME = "upload";
std::string EventUploadTable::ID = "id";
std::string EventUploadTable::YEAR = "year";
std::string EventUploadTable::M1 = "m1";
std::string EventUploadTable::M2 = "m2";
std::string EventUploadTable::M3 = "m3";
std::string EventUploadTable::M4 = "m4";
std::string EventUploadTable::M5 = "m5";
std::string EventUploadTable::M6 = "M6";
std::string EventUploadTable::M7 = "M7";
std::string EventUploadTable::M8 = "M8";
std::string EventUploadTable::M9 = "m9";
std::string EventUploadTable::M10 = "m10";
std::string EventUploadTable::M11 = "m11";
std::string EventUploadTable::M12 = "m12";

std::string RawEventTable::TBL_NAME = "raw_events";
std::string RawEventTable::ID = "id";
std::string RawEventTable::EVENT_ID = "event_id";
std::string RawEventTable::PARAM_LIST = "param_list";
std::string RawEventTable::TIMESTAMP = "timestamp";
std::string RawEventTable::EXTA_DATA = "extra_data";
BaseTable::BaseTable(const std::string& name) : name_(name)
{
    fields_.clear();
}

BaseTable::~BaseTable()
{
    fields_.clear();
}

std::vector<std::string> BaseTable::GetFieldNameList()
{
    std::vector<std::string> fieldNames;
    for (auto& field : fields_) {
        fieldNames.emplace_back(field->GetName());
    }
    return fieldNames;
}

bool BaseTable::IsFieldExist(const std::string& name)
{
    auto it = std::find_if(fields_.begin(), fields_.end(), [&](sp<SqlField>& field) {
        if (field->GetName() == name) {
            return true;
        }
        return false;
    });
    if (it == fields_.end()) {
        return false;
    }
    return true;
}

sp<SqlField> BaseTable::GetField(const std::string& name)
{
    auto it = std::find_if(fields_.begin(), fields_.end(), [&](sp<SqlField>& field) {
        if (field->GetName() == name) {
            return true;
        }
        return false;
    });
    if (it == fields_.end()) {
        return nullptr;
    }
    return *it;
}

void BaseTable::AddField(sp<SqlField> field)
{
    fields_.emplace_back(field);
}

std::string BaseTable::GenerateCreateTableStatement()
{
    int size = fields_.size();
    if (size == 0) {
        return "";
    }

    std::string sql = "create table IF NOT EXISTS " + name_ + "(";

    for (int i = 0; i < size; i++) {
        auto field = fields_[i];
        sql += field->GetFieldStatement();
        if (i < size-1) {
            sql += ",";
        }
    }
    sql += ")";
    return sql;
}

std::string BaseTable::BuildUpdateStatement(std::vector<std::string> names, std::vector<std::string> values)
{
    std::string sql = "";
    for (int i = 0; i < names.size(); i++) {
        std::string name = names[i];
        std::string value = values[i];

        auto sqlField = GetField(name);
        if (sqlField == nullptr) {
            continue;
        }
        if (TYPE_TEXT == sqlField->GetType()) {
            value = "'" + StringUtil::EscapeSqliteChar(value) + "'";
        }
        std::string splitStr = " ";
        if (sql != "") {
            splitStr = ", ";
        }
        sql += splitStr + name + "=" + value;
    }

    if (sql == "") {
        return "";
    }

    sql = "update " + name_ + " set " + sql;
    return sql;


}

std::string BaseTable::BuildInsertStatement(std::vector<std::string> names, std::vector<std::string> values)
{
    std::string nameList = "";
    std::string valueList = "";

    for (int i = 0; i < names.size(); i++) {
        std::string name = names[i];
        std::string value = values[i];

        auto sqlField = GetField(name);
        if (sqlField == nullptr) {
            continue;
        }
        if (TYPE_TEXT == sqlField->GetType()) {
            value = "'" + StringUtil::EscapeSqliteChar(value) + "'";
        }
        valueList += value + ",";
        nameList += name + ",";
    }

    if (nameList == "") {
        return "";
    }

    valueList = " values ( " + StringUtil::TrimStr(valueList, ',') + ") ";
    nameList = " (" + StringUtil::TrimStr(nameList, ',') + ") ";
    std::string sql = "insert into " + name_ + nameList + valueList;
    return sql;
}


std::string BaseTable::BuildInsertStatement(FieldValueMap fieldValue)
{
    std::string nameList = "";
    std::string valueList = "";

    for (auto& oneLoop : fieldValue) {
        std::string name = oneLoop.first;
        std::string value = oneLoop.second;

        auto sqlField = GetField(name);
        if (sqlField == nullptr) {
            continue;
        }
        if (TYPE_TEXT == sqlField->GetType()) {
             value = "'" + StringUtil::EscapeSqliteChar(value) + "'";
        }
        valueList += value + ",";
        nameList += name + ",";
    }

    if (nameList == "") {
        return "";
    }

    valueList = " values ( " + StringUtil::TrimStr(valueList, ',') + ") ";
    nameList = " (" + StringUtil::TrimStr(nameList, ',') + ") ";
    std::string sql = "insert into " + name_ + nameList + valueList;
    return sql;
}

EventTable::EventTable() : BaseTable(TBL_NAME)
{
    sp<SqlField> idField = new SqlField(ID, TYPE_INT, true, true, false);
    AddField(idField);

    sp<SqlField> eventIdField = new SqlField(EVENT_ID, TYPE_INT, false);
    AddField(eventIdField);

    sp<SqlField> traceIdField = new SqlField(TRACE_ID, TYPE_TEXT);
    AddField(traceIdField);

    sp<SqlField> pspanId = new SqlField(PSPAN_ID, TYPE_TEXT);
    AddField(pspanId);

    sp<SqlField> spanId = new SqlField(SPAN_ID, TYPE_TEXT);
    AddField(spanId);

    sp<SqlField> versionIdField = new SqlField(VERSION_ID, TYPE_INT, false);
    AddField(versionIdField);

    sp<SqlField> occuredCnt = new SqlField(OCCURED_CNT, TYPE_INT);
    AddField(occuredCnt);

    sp<SqlField> firstTime = new SqlField(FIRST_TIME, TYPE_INT);
    AddField(firstTime);

    sp<SqlField> paraList = new SqlField(PARAM_LIST, TYPE_TEXT);
    AddField(paraList);
}

EventTable::~EventTable()
{

}

EventDailyTable::EventDailyTable() : BaseTable(TBL_NAME)
{
    sp<SqlField> idField = new SqlField(ID, TYPE_INT, true, true, false);
    AddField(idField);

    sp<SqlField> eventIdField = new SqlField(EVENT_ID, TYPE_INT, false);
    AddField(eventIdField);

    sp<SqlField> dailyCntField = new SqlField(DAILY_CNT, TYPE_INT, false);
    AddField(dailyCntField);

    sp<SqlField> overCntField = new SqlField(OVER_CNT, TYPE_INT, false);
    AddField(overCntField);

    sp<SqlField> hourCntField = new SqlField(HOUR_CNT, TYPE_INT, false);
    AddField(hourCntField);

    sp<SqlField> expireField = new SqlField(NEXT_EXPIRE_TIME, TYPE_INT, true);
    AddField(expireField);

    sp<SqlField> lastField = new SqlField(LAST_HAPPEN_TIME, TYPE_INT, true);
    AddField(lastField);


    sp<SqlField> lastLogField = new SqlField(LAST_LOG_TIME, TYPE_INT, true);
    AddField(lastLogField);
}

EventDailyTable::~EventDailyTable()
{

}

EventVersionTable::EventVersionTable() : BaseTable(TBL_NAME)
{
    sp<SqlField> idField = new SqlField(ID, TYPE_INT, true, true, false);
    AddField(idField);

    sp<SqlField> versionField = new SqlField(VERSION, TYPE_TEXT, false);
    AddField(versionField);
}

EventVersionTable::~EventVersionTable()
{

}

EventDBVersionTable::EventDBVersionTable() : BaseTable(TBL_NAME)
{
    sp<SqlField> idField = new SqlField(ID, TYPE_INT, true, true, false);
    AddField(idField);

    sp<SqlField> dbVersionField = new SqlField(DB_VERSION, TYPE_INT, false);
    AddField(dbVersionField);

    sp<SqlField> verField = new SqlField(DEV_VERSION, TYPE_TEXT, false);
    AddField(verField);

    sp<SqlField> timeField = new SqlField(START_TIME, TYPE_INT, false);
    AddField(timeField);

    sp<SqlField> activateTimeField = new SqlField(ACTIVATE_TIME, TYPE_INT, true);
    AddField(activateTimeField);
}

EventDBVersionTable::~EventDBVersionTable()
{

}

EventUploadTable::EventUploadTable() : BaseTable(TBL_NAME)
{
    sp<SqlField> idField = new SqlField(ID, TYPE_INT, true, true, false);
    AddField(idField);

    sp<SqlField> yearField = new SqlField(YEAR, TYPE_INT, false);
    AddField(yearField);

    std::list<std::string> month = {M1, M2, M3, M4, M5, M6, M7, M8, M9, M10, M11, M12};
    for (auto& temp : month) {
        sp<SqlField> monthField = new SqlField(temp, TYPE_TEXT);
        AddField(monthField);
    }
}

EventUploadTable::~EventUploadTable()
{

}

RawEventTable::RawEventTable() : BaseTable(TBL_NAME)
{
    sp<SqlField> idField = new SqlField(ID, TYPE_INT, true, true, false);
    AddField(idField);

    sp<SqlField> eventIdField = new SqlField(EVENT_ID, TYPE_INT, false);
    AddField(eventIdField);

    sp<SqlField> extraDataField = new SqlField(EXTA_DATA, TYPE_TEXT);
    AddField(extraDataField);


    sp<SqlField> timestampField = new SqlField(TIMESTAMP, TYPE_INT, false);
    AddField(timestampField);

    sp<SqlField> paraList = new SqlField(PARAM_LIST, TYPE_TEXT);
    AddField(paraList);
}

RawEventTable::~RawEventTable()
{

}
}
}
