/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event query implements file
 *
 */
#include "event_query.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "string_util.h"
namespace android {
namespace MiSight {
EventQuery::EventQuery() : eventCols_(), orderAscCols_(), orderDescCols_(""), whereStatement_("")
{
}


EventQuery::~EventQuery()
{
    Clear();
}

void EventQuery::Clear()
{
    eventCols_.clear();
    orderAscCols_ = "";
    orderDescCols_ = "";
    whereStatement_ = "";
}

EventQuery &EventQuery::Select(const std::vector<std::string> &eventCols)
{
    for (std::string col : eventCols) {
        auto it = std::find(eventCols_.begin(), eventCols_.end(), col);
        if (it != eventCols_.end()) {
            continue;
        }
        eventCols_.push_back(col);
    }
    return *this;
}

EventQuery &EventQuery::Where(const std::string &col, Op op, const int64_t value)
{
    AppendWhereStatement(col, op, std::to_string(value), true);
    return *this;
}


EventQuery &EventQuery::Where(const std::string &col, Op op, const std::string &value)
{
    AppendWhereStatement(col, op, value, true);
    return *this;
}

EventQuery &EventQuery::And(const std::string &col, Op op, const int64_t value)
{
    AppendWhereStatement(col, op, std::to_string(value), true);
    return *this;
}

EventQuery &EventQuery::And(const std::string &col, Op op, const std::string &value)
{
    AppendWhereStatement(col, op, value, true);
    return *this;
}

EventQuery &EventQuery::And(const std::string &col, Op op, const std::vector<int64_t> &ints)
{
    for (auto& value : ints) {
        AppendWhereStatement(col, op, std::to_string(value), true);
    }
    return *this;
}

EventQuery &EventQuery::And(const std::string &col, Op op, const std::vector<std::string> &strings)
{
    for (auto& value : strings) {
        AppendWhereStatement(col, op, value, true);
    }
    return *this;
}

EventQuery &EventQuery::Or(const std::string &col, Op op, const int64_t value)
{
    AppendWhereStatement(col, op, std::to_string(value), false);
    return *this;
}

EventQuery &EventQuery::Or(const std::string &col, Op op, const std::string &value)
{
    AppendWhereStatement(col, op, value, false);
    return *this;
}

EventQuery &EventQuery::Or(const std::string &col, Op op, const std::vector<int64_t> &ints)
{
    for (auto& value : ints) {
        AppendWhereStatement(col, op, std::to_string(value), false);
    }
    return *this;
}

EventQuery &EventQuery::Or(const std::string &col, Op op, const std::vector<std::string> &strings)
{
    for (auto& value : strings) {
        AppendWhereStatement(col, op, value, false);
    }
    return *this;
}

EventQuery &EventQuery::AddBeginGroup(bool isAnd)
{
    std::string state = StringUtil::TrimStr(whereStatement_);
    if (state == "" || StringUtil::EndsWith(state, "(")) {
        whereStatement_ += " (";
        return *this;
    }


    std::string connect = " and (";
    if (!isAnd) {
        connect = " or ( ";
    }
    whereStatement_ += connect;
    return *this;
}

EventQuery &EventQuery::AddEndGroup()
{
    whereStatement_ += ") ";
    return *this;
}

EventQuery &EventQuery::Order(const std::string &col, bool isAsc)
{

    if (isAsc) {
        if (orderAscCols_ != "") {
            orderAscCols_ += ", ";
        }
        orderAscCols_ += col;
    } else {
        if (orderDescCols_ != "") {
            orderDescCols_ += ", ";
        }
        orderDescCols_ += col;
    }
    return *this;
}

std::string EventQuery::GenerateCondStatement(const std::string &col, Op op, const std::string& value)
{
    std::string sql = "";
    std::string valFormat = StringUtil::EscapeSqliteChar(value);
    switch (op) {
        case EQ:
            sql = col + " = '" + valFormat + "'";
            break;
        case NE:
            sql = col + " != '" + valFormat + "'";
            break;
        case GT:
            sql = col + " > '" + valFormat + "'";
            break;
        case GE:
            sql = col + " >= '" + valFormat + "'";
            break;
        case LT:
            sql = col + " < '" + valFormat + "'";
            break;
        case LE:
            sql = col + " <= '" + valFormat + "'";
            break;
        case LK:
            sql = col + " like '%" + valFormat + "%'";
            break;
        case NLK:
            sql = col + " not like '%" + valFormat + "%'";
            break;
        default:
            sql = "";
            break;
    }
    return sql;
}

void EventQuery::AppendWhereStatement(const std::string &col, Op op, const std::string& value, bool isAnd)
{
    std::string connect = "";
    if (whereStatement_ != "") {
        std::string state = StringUtil::TrimStr(whereStatement_);
        if (!StringUtil::EndsWith(state, "(")) {
            connect = " or ";
            if (isAnd) {
                connect = " and ";
            }
        }
    }
    std::string cond = GenerateCondStatement(col, op, value);
    if (cond == "") {
        return;
    }
    whereStatement_ += connect + GenerateCondStatement(col, op, value);

}
bool EventQuery::GetSelectStatement(std::string& sql)
{
    int32_t cnt = eventCols_.size();
    if (cnt == 0) {
        return false;
    }

    sql += "select ";
    for (int32_t i = 0; i < cnt; i++) {
        sql += eventCols_[i];
        if (i < cnt-1) {
            sql += ",";
        }
    }
    return true;
}

std::string EventQuery::GetOrderStatement()
{
    if (orderAscCols_ == "" && orderDescCols_ == "") {
        return "";
    }

    std::string order = "order by ";
    if (orderAscCols_ != "") {
        order += orderAscCols_ + " asc,";
    }
    if (orderDescCols_ != "") {
        order += orderDescCols_ + " desc,";
    }
    order = StringUtil::TrimStr(order, ',');
    return order;
}

std::string EventQuery::BuildQueryStatement(const std::string& tblName, int32_t limit)
{
    std::string sql = "";
    if (!GetSelectStatement(sql)) {
        return sql;
    }
    sql += " from " + tblName;
    if (whereStatement_ != "") {
        sql += " where " + whereStatement_;
    }
    std::string order = GetOrderStatement();
    if (order != "") {
        sql += " " + GetOrderStatement();
    }
    sql += " limit " + std::to_string(limit);
    return sql;
}

std::string EventQuery::GetWhereStatement()
{
    std::string sql = "";
    if (whereStatement_ != "") {
        sql += " where " + whereStatement_;
    }
    std::string order = GetOrderStatement();
    if (order != "") {
        sql += GetOrderStatement();
    }
    if (sql != "") {
        sql = " " + sql;
    }
    return sql;
}
} // namespace MiSight
} // namespace android