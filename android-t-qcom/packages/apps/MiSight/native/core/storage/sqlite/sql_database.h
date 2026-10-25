/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight sqlite data base class head file
 *
 */
#ifndef MISIGH_CORE_SQLLITE_DATABASE_BASE_DEFINE_H
#define MISIGH_CORE_SQLLITE_DATABASE_BASE_DEFINE_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include <sqlite3.h>
#include <utils/StrongPointer.h>

#include "define_util.h"
#include "sql_field.h"

namespace android {
namespace MiSight {
typedef std::map<std::string, std::string> FieldValueMap;
class SqlDatabase : public RefBase  {
public:
    explicit SqlDatabase(const std::string& file);
    ~SqlDatabase();

    void Close();
    bool Open();
    bool ExecuteSql(const std::string& sql);

    bool TruncateTable(const std::string& tblName);
    std::vector<FieldValueMap> QueryRecord(const std::string& sql, std::vector<std::string> selectCols);

    int32_t GetTableCount(const std::string& tblName);
    std::string GetErrMsg() {
        return errMsg_;
    }
protected:
    std::vector<std::string> curSelectCols_;

private:
    void Clear();
    bool Execute(const std::string& sql, bool isCallback = false);
    static int GetExecResult(void* param, int columnCnt, char** columnVal, char** columnName __UNUSED);

    sqlite3* sqlDb_;
    std::string dbFile_;
    std::string errMsg_;
    std::vector<FieldValueMap> records_;
};
}
}
#endif

