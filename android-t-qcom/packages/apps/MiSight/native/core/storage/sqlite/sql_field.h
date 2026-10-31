/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight sql field class head file
 *
 */
#ifndef MISIGH_CORE_SQLLITE_FIELD_BASE_DEFINE_H
#define MISIGH_CORE_SQLLITE_FIELD_BASE_DEFINE_H

#include <string>

#include <utils/RefBase.h>
namespace android {
namespace MiSight {
enum SqlFieldType {
    TYPE_UNDEFINED,
    TYPE_INT,
    TYPE_TEXT,
    TYPE_FLOAT,
};

class SqlField : public RefBase {
public:
    SqlField(const std::string& name, SqlFieldType type, bool isPrimaryKey, bool isAutoIncr, bool isNull);
    SqlField(const std::string& name, SqlFieldType type);
    SqlField(const std::string& name, SqlFieldType type, bool isNull);
    //SqlField(const std::string& name, const std::string& value);
    //SqlField(const SqlField& sqlField);

    virtual ~SqlField(){}

    std::string GetName();
    SqlFieldType GetType();
    std::string GetTypeStr();
    std::string GetValue();
    int32_t GetIntValue();
    std::string GetFieldStatement();
    bool IsPrimaryKey();
    bool IsNULL();
    bool IsAutoIncrement();

private:
    std::string name_;     // fieldName
    SqlFieldType type_;       // attr: type
    bool isPrimaryKey_; // attr: primary key
    bool isAutoIncr_; // attr: auto increment
    bool isNull_;          // attr: NULL
    std::string value_;
};
}
}
#endif
