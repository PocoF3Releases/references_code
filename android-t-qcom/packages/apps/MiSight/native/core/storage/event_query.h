/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event query head file
 *
 */

#ifndef MISIGHT_CORE_EVENT_DB_QUERY_BASE_USED_H
#define MISIGHT_CORE_EVENT_DB_QUERY_BASE_USED_H

#include <memory>
#include <string>
#include <vector>



namespace android {
namespace MiSight {
enum Op { NONE = 0, EQ = 1, NE, LT, LE, GT, GE, LK, NLK };

class  EventQuery  {
public:
    ~EventQuery();
    EventQuery();
    EventQuery &Select(const std::vector<std::string> &eventCols);
    EventQuery &Where(const std::string &col, Op op, const int64_t value);
    EventQuery &Where(const std::string &col, Op op, const std::string &value);

    EventQuery &And(const std::string &col, Op op, const int64_t value);
    EventQuery &And(const std::string &col, Op op, const std::string &value);
    EventQuery &And(const std::string &col, Op op, const std::vector<int64_t> &ints);
    EventQuery &And(const std::string &col, Op op, const std::vector<std::string> &strings);

    EventQuery &Or(const std::string &col, Op op, const int64_t value);
    EventQuery &Or(const std::string &col, Op op, const std::string &value);
    EventQuery &Or(const std::string &col, Op op, const std::vector<int64_t> &ints);
    EventQuery &Or(const std::string &col, Op op, const std::vector<std::string> &strings);

    EventQuery &Order(const std::string &col, bool isAsc = true);
    EventQuery &AddBeginGroup(bool isAnd);
    EventQuery &AddEndGroup();
    std::string BuildQueryStatement(const std::string& tblName, int32_t limit = 1);
    std::string GetWhereStatement();
    std::vector<std::string> GetSelectColList() {
        return eventCols_;
    }
    void Clear();

private:
    std::string GenerateCondStatement(const std::string &col, Op op, const std::string& value);
    void AppendWhereStatement(const std::string &col, Op op, const std::string& value, bool isAnd);
    bool GetSelectStatement(std::string& sql);
    std::string GetOrderStatement();

    std::vector<std::string> eventCols_;
    std::string orderAscCols_;
    std::string orderDescCols_;
    std::string whereStatement_;
};
} // namespace MiSight
} // namespace android
#endif
