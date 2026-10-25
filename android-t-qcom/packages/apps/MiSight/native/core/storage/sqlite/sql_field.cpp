/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight sql field class implements file
 *
 */

#include "sql_field.h"

#include "string_util.h"

namespace android {
namespace MiSight {

SqlField::SqlField(const std::string& name, SqlFieldType type, bool isPrimaryKey, bool isAutoIncr, bool isNull)
    : name_(name), type_(type), isPrimaryKey_(isPrimaryKey), isAutoIncr_(isAutoIncr), isNull_(isNull), value_("")
{

}

SqlField::SqlField(const std::string& name, SqlFieldType type)
    : name_(name), type_(type), isPrimaryKey_(false), isAutoIncr_(false), isNull_(true), value_("")
{

}
SqlField::SqlField(const std::string& name, SqlFieldType type, bool isNull)
    : name_(name), type_(type), isPrimaryKey_(false), isAutoIncr_(false), isNull_(isNull), value_("")
{

}


/*SqlField::SqlField(const SqlField& sqlField)
{
    name_ = sqlField.name_;
    type_ = sqlField.type_;
    isPrimaryKey_ = sqlField.isPrimaryKey_;
    isAutoIncr_ = sqlField.isAutoIncr_;
    isNull_ = sqlField.isNull_;
}*/

/*SqlField::SqlField(const std::string& name, const std::string& value)
    : name_(name), type_(TYPE_UNDEFINED), isPrimaryKey_(false), isAutoIncr_(false), isNull_(true), value_(value)
{

}*/


std::string SqlField::GetName()
{
    return name_;
}

bool SqlField::IsAutoIncrement()
{
    return isAutoIncr_;
}

SqlFieldType SqlField::GetType()
{
    return type_;
}

std::string SqlField::GetTypeStr()
{
    switch (type_) {
        case TYPE_INT:
            return "INTEGER";
        case TYPE_TEXT:
            return "TEXT";
        case TYPE_FLOAT:
            return "REAL";
        default:
            return "";
    }
}

std::string SqlField::GetFieldStatement()
{
    std::string sql = name_ + " " + GetTypeStr();
    if (!IsNULL()) {
        sql += " NOT NULL";
    }

    if (IsPrimaryKey()) {
        sql += " PRIMARY KEY";
    }

    if (IsAutoIncrement()) {
        sql += " AUTOINCREMENT";
    }
    return sql;
}

bool SqlField::IsPrimaryKey()
{
    return isPrimaryKey_;
}

bool SqlField::IsNULL()
{
    return isNull_;
}

std::string SqlField::GetValue()
{
    return value_;
}

int32_t SqlField::GetIntValue()
{
    return StringUtil::ConvertInt(value_);
}
}
}
