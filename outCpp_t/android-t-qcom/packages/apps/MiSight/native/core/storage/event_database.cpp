/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event database common implement file
 *
 */

#include "event_database.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <unistd.h>


#include "log.h"
#include "string_util.h"

ANDROID_SINGLETON_STATIC_INSTANCE(::android::MiSight::EventDatabase);

namespace android {
namespace MiSight {

EventDatabase::EventDatabase() : status_(false), sqlDb_(nullptr)
{

}

EventDatabase::~EventDatabase()
{
    MISIGHT_LOGI("close database");
    Close();
}

void EventDatabase::Close()
{
    if (sqlDb_) {
        MISIGHT_LOGI("event data base closed");
        status_ = false;
        sqlDb_->Close();
        sqlDb_ = nullptr;
        tables_.clear();
    }
}
bool EventDatabase::InitDatabase(const std::string& dbPath)
{
    // if already init, return
    if (status_) {
        MISIGHT_LOGW("init data base already init");
        return true;
    }

    tables_ = {{EventTable::TBL_NAME,    new EventTable()},
        {EventDailyTable::TBL_NAME,     new EventDailyTable()},
        {EventVersionTable::TBL_NAME,    new EventVersionTable()},
        {EventDBVersionTable::TBL_NAME, new EventDBVersionTable()},
        {EventUploadTable::TBL_NAME,    new EventUploadTable()},
        {RawEventTable::TBL_NAME,        new RawEventTable()}
    };

    // open and create db
    sqlDb_ = new SqlDatabase(dbPath);
    if (sqlDb_ == nullptr || !sqlDb_->Open()) {
        return false;
    }
    status_ = true;

    //create database tables
    for (auto& table : tables_) {
        MISIGHT_LOGD("init database %s, create table %s", dbPath.c_str(), table.first.c_str());
        if (table.second == nullptr) {
            return false;
        }
        if (!sqlDb_->ExecuteSql(table.second->GenerateCreateTableStatement())) {
            return false;
        }
    }
    return true;
}

int32_t EventDatabase::InsertAndGetID(const std::string& tblName,
    std::vector<std::string> names, std::vector<std::string> values)
{
    if (!Insert(tblName, names, values)) {
        MISIGHT_LOGE("insert %s failed", tblName.c_str());
        return 0;
    };

    auto result = sqlDb_->QueryRecord("select id from " + tblName + " order by id desc limit 1", {"id"});
    if (result.size() == 0) {
        MISIGHT_LOGD("get id from %s", tblName.c_str());
        return 0;
    }
    return StringUtil::ConvertInt(result.front()["id"]);
}

bool EventDatabase::Insert(const std::string& tblName, std::vector<std::string> names, std::vector<std::string> values)
{
    if ((names.size() == 0) || (names.size() != values.size())) {
        MISIGHT_LOGE("names must not empty, and names size must equal values's");
        return false;
    }
    if (!status_) {
        MISIGHT_LOGE("database not init");
        return false;
    }
    auto tbl = GetTable(tblName);
    if (tbl == nullptr) {
        MISIGHT_LOGE("get tbl %s failed", tblName.c_str());
        return false;
    }
    std::string sql = tbl->BuildInsertStatement(names, values);
    if (sql == "") {
        MISIGHT_LOGE("generate sql failed");
        return false;
    }
    return sqlDb_->ExecuteSql(sql);
}


std::vector<FieldValueMap> EventDatabase::Query(const std::string& tblName, EventQuery query, int limit)
{
    std::vector<FieldValueMap> fieldMaps;
    if (!status_) {
        MISIGHT_LOGE("database not init");
        return fieldMaps;
    }

    std::string sql = query.BuildQueryStatement(tblName, limit);
    if (sql == "")
    {
        return fieldMaps;
    }
    MISIGHT_LOGD("query sql : %s, status=%d", sql.c_str(), status_);
    fieldMaps = sqlDb_->QueryRecord(sql, query.GetSelectColList());
    return fieldMaps;
}

bool EventDatabase::Update(const std::string& tblName, std::vector<std::string> names,
    std::vector<std::string> values, EventQuery query)
{

    /*update BIZ_VOUCHER_TOTAL_INFO set    GRANTRECEDEF_NUM=nvl(GRANTRECEDEF_NUM,0)+1 where
            TERMINAL_ID='3401010X' and BATCH_NO='1921' */
    if ((names.size() == 0) || (names.size() != values.size())) {
        MISIGHT_LOGE("names must not empty, and names size must equal values's");
        return false;
    }
    if (!status_) {
        MISIGHT_LOGE("database not init");
        return false;
    }
    auto tbl = GetTable(tblName);
    if (tbl == nullptr) {
        return false;
    }
    std::string sql = tbl->BuildUpdateStatement(names, values);
    sql += query.GetWhereStatement();
    if (sql == "") {
        return false;
    }
    return sqlDb_->ExecuteSql(sql);
}

bool EventDatabase::Delete(const std::string& tblName, EventQuery query)
{
    if (!status_) {
        MISIGHT_LOGE("database not init");
        return false;
    }
    std::string sql = "delete from " + tblName + query.GetWhereStatement();
    return sqlDb_->ExecuteSql(sql);
}

bool EventDatabase::TruncateAndCreate(const std::string& tblName)
{
    if (!sqlDb_->TruncateTable(tblName)) {
        return false;
    }

    auto tbl = GetTable(tblName);
    if (tbl == nullptr) {
        return false;
    }

    return sqlDb_->ExecuteSql(tbl->GenerateCreateTableStatement());
}

sp<BaseTable> EventDatabase::GetTable(const std::string& tblName)
{
     auto findTbl = tables_.find(tblName);
     if (findTbl == tables_.end()) {
         return nullptr;
     }
     return findTbl->second;
}
}
}
