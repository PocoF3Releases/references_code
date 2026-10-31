/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight sqlite data base class implements file
 *
 */

#include "sql_database.h"


#include "log.h"
#include "string_util.h"

namespace android {
namespace MiSight {

SqlDatabase::SqlDatabase(const std::string& file)
    : sqlDb_(nullptr), dbFile_(file), errMsg_("")
{}

SqlDatabase::~SqlDatabase()
{}

void SqlDatabase::Close()
{
    if (sqlDb_) {
        sqlite3_close(sqlDb_);
        Clear();
        sqlDb_ = nullptr;
    }
}

void SqlDatabase::Clear()
{
    errMsg_.clear();
    records_.clear();
}

bool SqlDatabase::Execute(const std::string& sql, bool isCallback)
{
    Clear();

    if (sqlDb_ == nullptr) {
        return false;
    }

    MISIGHT_LOGD("start execute sql %s:%s", dbFile_.c_str(), sql.c_str());
    char *errStr = nullptr;
    int32_t ret = 0;
    if (!isCallback) {
        ret = sqlite3_exec(sqlDb_, sql.c_str(), nullptr, nullptr, &errStr);
    } else {
        ret = sqlite3_exec(sqlDb_, sql.c_str(), GetExecResult, this, &errStr);
    }
    if (ret != SQLITE_OK) {
        if (errStr != nullptr) {
            errMsg_ = std::string(errStr);
            sqlite3_free(errStr);
        }
        return false;
    }
    return true;
}

bool SqlDatabase::Open()
{
    if (dbFile_ == "") {
        return false;
    }

    Close();

    sqlite3_config(SQLITE_CONFIG_SMALL_MALLOC, true); // set small malloc config
    sqlite3_soft_heap_limit64(100 * 1024); // set soft heap limit as 100 * 1024B = 100KB
    int32_t ret = sqlite3_open(dbFile_.c_str(), &(sqlDb_));
    if (ret != SQLITE_OK) {
        errMsg_ = sqlite3_errmsg(sqlDb_);
        MISIGHT_LOGE("failed open db %s, %s", dbFile_.c_str(), errMsg_.c_str());
        return false;
    }
    // set cache size as 10 pages
    bool retExec = Execute("PRAGMA cache_size=10;");
    if (!retExec) {
        MISIGHT_LOGE("failed db %s set cache size, %s", dbFile_.c_str(), errMsg_.c_str());
    }
    // set normal synchronous
    retExec = Execute("PRAGMA synchronous=1;");
    if (!retExec) {
        MISIGHT_LOGE("failed db %s set synchronous, %s", dbFile_.c_str(), errMsg_.c_str());
    }
    return true;
}

bool SqlDatabase::ExecuteSql(const std::string& sql)
{
    if (Execute(sql)) {
        return true;
    }
    MISIGHT_LOGW("failed execute sql %s:%s, %s", dbFile_.c_str(), sql.c_str(), errMsg_.c_str());
    return false;
}

bool SqlDatabase::TruncateTable(const std::string& tblName)
{
    if (sqlDb_ == nullptr) {
        return false;
    }

    if (Execute("delete from " + tblName)) {
        Execute("DELETE FROM sqlite_sequence WHERE name='" + tblName + "'");
        return true;
    }
    MISIGHT_LOGW("failed truncate table %s:%s, %s", dbFile_.c_str(), tblName.c_str(), errMsg_.c_str());
    return false;
}

int SqlDatabase::GetExecResult(void* param, int columnCnt, char** columnVal, char** columnName)
{
    SqlDatabase* sqlDb = reinterpret_cast<SqlDatabase*>(param);

    if ((sqlDb == nullptr) || (columnVal == nullptr) || (columnName == nullptr)) {
        return 0;
    }

    if (sqlDb->curSelectCols_.size() != columnCnt) {
        MISIGHT_LOGE("failed result cnt=%d,expCnt=%d,", columnCnt, (int)sqlDb->curSelectCols_.size());
        return 0;
    }

    FieldValueMap maps;
    for (int32_t index = 0; index < columnCnt; index++) {
        std::string value = "";
        if (columnVal[index] != nullptr) {
            value = columnVal[index];
        }
        std::string name = sqlDb->curSelectCols_[index];
        maps[name] = value;
    }
    if (maps.size() > 0) {
        sqlDb->records_.push_back(maps);
    }
    return 0;
}

std::vector<FieldValueMap> SqlDatabase::QueryRecord(const std::string& sql, std::vector<std::string> selectCols)
{
    if (sqlDb_ == nullptr) {
        return records_;
    }
    curSelectCols_.clear();
    curSelectCols_ = selectCols;
    bool ret = Execute(sql, true);
    if (!ret) {
        MISIGHT_LOGE("failed QueryRecord %s %s, %s", dbFile_.c_str(), sql.c_str(), errMsg_.c_str());
    }
    curSelectCols_.clear();
    return records_;
}

int32_t SqlDatabase::GetTableCount(const std::string& tblName)
{
    curSelectCols_.clear();
    curSelectCols_.push_back("cnt");
    bool ret = Execute("select count() as cnt from " + tblName, true);
    if (!ret) {
        MISIGHT_LOGE("failed %s:%s, %s", dbFile_.c_str(), tblName.c_str(), errMsg_.c_str());
        return 0;
    }
    if (records_.size() == 0) {
        return 0;
    }
    FieldValueMap row = records_.front();
    auto findCnt =  row.find("cnt");
    if (findCnt == row.end()) {
        return 0;
    }
    return StringUtil::ConvertInt(findCnt->second);
}
}
}
