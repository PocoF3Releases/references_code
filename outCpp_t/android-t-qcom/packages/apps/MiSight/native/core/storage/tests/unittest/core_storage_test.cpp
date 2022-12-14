/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability plugin event info pack test head file
 *
 */

#include "core_storage_test.h"


#include <list>
#include <string>
#include <vector>

#include "event_database.h"
#include "event_query.h"
#include "event_table.h"
#include "sql_database.h"
#include "sql_field.h"


using namespace android;
using namespace android::MiSight;

TEST_F(CoreStorageTest, TestDatabaseNormal) {
    std::string dbPath = "/data/test/event_test.db";
    EventDatabase *eventDb = &EventDatabase::getInstance();
    bool ret = eventDb->InitDatabase(dbPath);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(true, eventDb->InitDatabase(dbPath));

    ret = eventDb->TruncateAndCreate(EventTable::TBL_NAME);
    ASSERT_EQ(ret, true);

    EventQuery query;
    std::vector<FieldValueMap> queryRet = eventDb->Query(EventTable::TBL_NAME, query);
    ASSERT_EQ(queryRet.size(), 0);
    uint64_t timestamp = time(nullptr);
    ret = eventDb->Insert(EventTable::TBL_NAME,
        {EventTable::EVENT_ID, EventTable::VERSION_ID, EventTable::FIRST_TIME},
        {"901001001", "2", std::to_string(timestamp)});
    ASSERT_EQ(ret, true);
    int32_t id = eventDb->InsertAndGetID(EventTable::TBL_NAME,
        {EventTable::EVENT_ID, EventTable::VERSION_ID},
        {"902002001", "3"});
    ASSERT_EQ(id, 2);

    sp<SqlDatabase> sqlDb = new SqlDatabase(dbPath);
    ASSERT_EQ(0, sqlDb->GetTableCount(EventTable::TBL_NAME));
    ASSERT_EQ(true, sqlDb->Open());
    ASSERT_EQ(2, sqlDb->GetTableCount(EventTable::TBL_NAME));

    query.Clear();
    query.Where(EventTable::ID, EQ, 2);
    ret = eventDb->Update(EventTable::TBL_NAME,
        {EventTable::FIRST_TIME, EventTable::PARAM_LIST},
        {std::to_string(timestamp), "it param list"},
        query);
    ASSERT_EQ(ret, true);

    auto eventTbl = eventDb->GetTable(EventTable::TBL_NAME);
    query.Clear();
    query.Select(eventTbl->GetFieldNameList())
         .Where(EventTable::ID, EQ, 2);
    queryRet = eventDb->Query(EventTable::TBL_NAME, query);
    ASSERT_EQ(queryRet.size(), 1);
    FieldValueMap mapRet = queryRet.front();
    ASSERT_EQ(mapRet[EventTable::EVENT_ID], "902002001");
    ASSERT_EQ(mapRet[EventTable::VERSION_ID], "3");
    ASSERT_EQ(mapRet[EventTable::FIRST_TIME], std::to_string(timestamp));
    ASSERT_EQ(mapRet[EventTable::PARAM_LIST], "it param list");
    ASSERT_EQ(mapRet[EventTable::SPAN_ID], "");

    query.Clear();
    query.Where(EventTable::EVENT_ID, EQ, "901001001");
    eventDb->Delete(EventTable::TBL_NAME, query);

    query.Clear();
    query.Select(eventTbl->GetFieldNameList());
    queryRet = eventDb->Query(EventTable::TBL_NAME, query);
    ASSERT_EQ(queryRet.size(), 1);
    mapRet = queryRet.front();
    ASSERT_EQ(mapRet[EventTable::EVENT_ID], "902002001");
    eventDb->Close();

    sqlDb = nullptr;
}

TEST_F(CoreStorageTest, TestDatabaseAbnormal) {

    std::string dbPath = "/data/test/event_test.db";
    EventDatabase *eventDb = &EventDatabase::getInstance();

    bool ret = eventDb->InitDatabase(dbPath);
    ASSERT_EQ(ret, true);

    EventQuery query;
    std::vector<FieldValueMap> queryRet = eventDb->Query(EventTable::TBL_NAME, query, 100);

    int initSize = queryRet.size();

    ret = eventDb->TruncateAndCreate("tblNotExist");
    ASSERT_EQ(ret, false);

    ret = eventDb->Insert(EventTable::TBL_NAME, {EventTable::EVENT_ID}, {});
    ASSERT_EQ(ret, false);
    ret = eventDb->Insert("tblNotExist", {EventTable::EVENT_ID, EventTable::VERSION_ID}, {"901001001"});
    ASSERT_EQ(ret, false);
    ret = eventDb->Insert("tblNotExist", {}, {"901001001"});
    ASSERT_EQ(ret, false);
    queryRet = eventDb->Query(EventTable::TBL_NAME, query);
    ASSERT_EQ(initSize, queryRet.size());
    eventDb->Close();
}

TEST_F(CoreStorageTest, TestDatabaseQuery) {
    EventQuery query;
    query.Select({"c1", "c2", "c3", "c4", "c5"})
         .Where("c1", EQ, 32)
         .Where("c2", LK, "tring")
         .And("c3", NLK, "str")
         .AddBeginGroup(true)
         .AddBeginGroup(true)
         .And("c4", GT, 2)
         .And("c4", LE, 1)
         .AddEndGroup()
         .AddBeginGroup(false)
         .Or("c4", GE, 7)
         .And(
"c4", LT, 3)
         .AddEndGroup()
         .AddEndGroup()
         .Order("c4");
    std::string sql = query.BuildQueryStatement("tbl", 100);
    printf("sql is =%s\n", sql.c_str());

    ASSERT_EQ(sql, "select c1,c2,c3,c4,c5 from tbl where c1 = '32' and c2 like '%tring%' and c3 not like '%str%' and ( (c4 > '2' and c4 <= '1')  or ( c4 >= '7' and c4 < '3') )  order by c4 asc limit 100");

    query.Clear();
    sql = query.BuildQueryStatement("tbl", 100);
    ASSERT_EQ(sql, "");

    query.Select({"c1", "c2", "c3", "c4", "c5"})
         .AddBeginGroup(true)
         .And("c1", EQ, 32)
         .Or("c2", LK, "tring")
         .And("c3", NLK, "str")
         .AddEndGroup()
         .AddBeginGroup(false)
         .AddBeginGroup(false)
         .And("c4", GT, 2)
         .And("c4", LE, 1)
         .AddEndGroup()
         .AddBeginGroup(false)
         .Or("c4", GE, 7)
         .And(
"c4", LT, 3)
         .AddEndGroup()
         .AddEndGroup()
         .Order("c4");
    sql = query.BuildQueryStatement("tbl", 100);
    printf("sql is =%s\n", sql.c_str());
    ASSERT_EQ(sql, "select c1,c2,c3,c4,c5 from tbl where  (c1 = '32' or c2 like '%tring%' and c3 not like '%str%')  or (  (c4 > '2' and c4 <= '1')  or ( c4 >= '7' and c4 < '3') )  order by c4 asc limit 100");

}

TEST_F(CoreStorageTest, TestSqlDatabase) {
    sp<SqlDatabase> sqlDb = new SqlDatabase("");
    ASSERT_EQ(false, sqlDb->Open());
    ASSERT_EQ(false, sqlDb->TruncateTable("tblName"));
    std::vector<FieldValueMap> map = sqlDb->QueryRecord("", {});
    ASSERT_EQ(0, map.size());
    ASSERT_EQ(false, sqlDb->ExecuteSql(""));
    ASSERT_EQ(0, sqlDb->GetTableCount(""));
    sqlDb->Close();
    sqlDb = nullptr;
}
