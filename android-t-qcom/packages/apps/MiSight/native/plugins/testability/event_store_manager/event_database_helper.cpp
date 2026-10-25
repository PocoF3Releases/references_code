/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event store database  implement file
 *
 */

#include "event_database_helper.h"

#include <inttypes.h>

#include "dev_info.h"
#include "event_database.h"
#include "event_quota_xml_parser.h"
#include "event_table.h"
#include "event_util.h"
#include "log.h"
#include "string_util.h"
#include "time_util.h"


namespace android {
namespace MiSight {

EventDatabaseHelper::EventDatabaseHelper(uint64_t startTime)
    : isUpdate_(false), versionId_(0), dbId_(0), startTime_(startTime), platformContext_(nullptr), curVersion_("")
{}

EventDatabaseHelper::~EventDatabaseHelper()
{
    auto eventDb = &EventDatabase::getInstance();
    eventDb->Close();
}

void EventDatabaseHelper::InitDatabase(PluginContext* context, const std::string& dbName, const std::string& version)
{
    if (context == nullptr) {
        return;
    }
    platformContext_ = context;

    // init data base
    EventDatabase::getInstance().InitDatabase(platformContext_->GetWorkDir() + dbName);

    // get version id
    curVersion_ = version;
    InitDBVersion();

    //set database  start time
    SetDbStartTime();
}

void EventDatabaseHelper::AbridgeDbSize()
{
    auto eventDb = &EventDatabase::getInstance();
    int count = 7; // 7 days of one week
    int64_t zeroTimestamp = TimeUtil::GetZeroTimestampMs() / TIME_UNIT;
    while (true) {
        zeroTimestamp = zeroTimestamp - DAY_SECONDS * count;
        count--;

        EventQuery query;
        query.Select({EventTable::ID})
            .Where(EventTable::FIRST_TIME, LT, zeroTimestamp);
        auto queryRet = eventDb->Query(EventTable::TBL_NAME, query, 1);
        if (queryRet.size() != 0) {
            break;
        }
        if (count == 0) {
            break;
        }
    }

    EventQuery query;
    query.Where(EventTable::FIRST_TIME, LT, zeroTimestamp);
    bool ret = eventDb->Delete(EventTable::TBL_NAME, query);

    query.Clear();
    query.Where(RawEventTable::TIMESTAMP, LT, zeroTimestamp);
    bool retRaw = eventDb->Delete(RawEventTable::TBL_NAME, query);
    if (!ret || !retRaw) {
        MISIGHT_LOGI("clean the db file error at timestamp %" PRId64 "", zeroTimestamp);
        return;
    }
    MISIGHT_LOGI("clean the db file at timestamp %" PRId64 " successful", zeroTimestamp);
}


void EventDatabaseHelper::InitDBVersion()
{
    auto eventDb = &EventDatabase::getInstance();

    EventQuery query;
    query.Select({EventVersionTable::ID})
        .Where(EventVersionTable::VERSION, EQ, curVersion_);
    auto queryRet = eventDb->Query(EventVersionTable::TBL_NAME, query);
    MISIGHT_LOGI("get version id from table %s, result %d", EventVersionTable::TBL_NAME.c_str(), (int)queryRet.size());
    if (queryRet.size() == 0) {
        isUpdate_ = true;
        MISIGHT_LOGI("insert into table %s, version %s", EventVersionTable::TBL_NAME.c_str(), curVersion_.c_str());
        versionId_ = eventDb->InsertAndGetID(EventVersionTable::TBL_NAME, {EventVersionTable::VERSION}, {curVersion_});
        return;
    }

    auto& field = queryRet.front();
    versionId_ = StringUtil::ConvertInt(field[EventVersionTable::ID]);
    MISIGHT_LOGI("get table %s, versionId %d", EventVersionTable::TBL_NAME.c_str(), versionId_);
}

void EventDatabaseHelper::SetDbStartTime()
{
    auto eventDb = &EventDatabase::getInstance();

    EventQuery query;
    query.Select({EventDBVersionTable::ID, EventDBVersionTable::DB_VERSION,
        EventDBVersionTable::DEV_VERSION, EventDBVersionTable::START_TIME, EventDBVersionTable::ACTIVATE_TIME})
        .Order(EventDBVersionTable::ID, false);
    auto queryRet = eventDb->Query(EventDBVersionTable::TBL_NAME, query);
    MISIGHT_LOGI("query %s ret=%zu,start=%" PRId64, EventDBVersionTable::TBL_NAME.c_str(), queryRet.size(), startTime_);
    if (queryRet.size() == 0) {
        DevInfo::SetActivateTime(startTime_);
        dbId_ = eventDb->InsertAndGetID(EventDBVersionTable::TBL_NAME,
            {EventDBVersionTable::DB_VERSION, EventDBVersionTable::DEV_VERSION, EventDBVersionTable::START_TIME,
            EventDBVersionTable::ACTIVATE_TIME},
            {"1", curVersion_, std::to_string(startTime_), std::to_string(startTime_)});
    } else {
        int32_t activeDb = StringUtil::ConvertInt(queryRet.front()[EventDBVersionTable::ACTIVATE_TIME]);
        uint64_t activeTime = activeDb > 0 ? activeDb : 0;
        DevInfo::SetActivateTime(activeTime);
        dbId_ = StringUtil::ConvertInt(queryRet.front()[EventDBVersionTable::ID]);
        bool needTruncate = true;
        if (!isUpdate_) {
            int32_t startDb = StringUtil::ConvertInt(queryRet.front()[EventDBVersionTable::START_TIME]);
            // not ota update and reboot in a day, no need truncate
            MISIGHT_LOGI("db time check need update table %s, dbStartTime=%d,curStartTime=%" PRId64 ,
                EventDBVersionTable::TBL_NAME.c_str(), startDb, startTime_);
            if ((startTime_ - DAY_SECONDS) < startDb) {
                startTime_ = startDb;
                needTruncate = false;
            }
        }
        if (needTruncate) {
            MISIGHT_LOGI("update table %s, curTime=%" PRId64 , EventDBVersionTable::TBL_NAME.c_str(), startTime_);
            query.Clear();
            query.Where(EventDBVersionTable::ID, EQ, dbId_);
            eventDb->Update(EventDBVersionTable::TBL_NAME, {EventDBVersionTable::START_TIME, EventDBVersionTable::DEV_VERSION},
                {std::to_string(startTime_), curVersion_}, query);
            eventDb->TruncateAndCreate(EventDailyTable::TBL_NAME);
        }
    }
    MISIGHT_LOGI("query from table %s %d", EventDBVersionTable::TBL_NAME.c_str(), dbId_);
}

bool EventDatabaseHelper::DetectDropByTime(sp<Event> event, FieldValueMap filedMap)
{
    std::string cfgDir = platformContext_->GetConfigDir(EventQuotaXmlParser::getInstance()->GetXmlFileName());
    EventQuotaXmlParser::DomainQuota eventQuota = EventQuotaXmlParser::getInstance()->GetEventQuota(cfgDir,
        event->eventId_, curVersion_);
    int32_t overCnt = StringUtil::ConvertInt(filedMap[EventDailyTable::OVER_CNT]);
    int32_t dailyCnt = StringUtil::ConvertInt(filedMap[EventDailyTable::DAILY_CNT]);
    int32_t hourCnt = StringUtil::ConvertInt(filedMap[EventDailyTable::HOUR_CNT]);
    uint64_t expireTime = StringUtil::ConvertInt(filedMap[EventDailyTable::NEXT_EXPIRE_TIME]);
    uint64_t lastTime = StringUtil::ConvertInt(filedMap[EventDailyTable::LAST_HAPPEN_TIME]);
    uint64_t lastLogTime = StringUtil::ConvertInt(filedMap[EventDailyTable::LAST_LOG_TIME]);
    uint64_t newLast = lastTime;
    MISIGHT_LOGD("event %d parser maxCnt=%u lastTime=%lu", event->eventId_, eventQuota.dailyCnt, lastTime);
    bool drop = false;

    // check event if over limit
    if (dailyCnt >= eventQuota.dailyCnt) {
        drop = true;
        // over limit, event will drop
        overCnt++;
        event->SetValue(EventUtil::EVENT_DROP_REASON, "over daily quota");
        MISIGHT_LOGD("event %d over daily cnt drop it, dailyCnt=%d maxCnt=%u", event->eventId_, dailyCnt, eventQuota.dailyCnt);
    } else {
        // check one hour occur counts
        newLast = event->happenTime_;

        // if event happen after one hour, must recount
        if (expireTime <= event->happenTime_) {
            hourCnt = 1;
            expireTime = expireTime + HOUR_SECONDS;
        } else {
            /* in one hour count period, if event occur counts less than half of the maximum number per hour, not control,
             * otherwise, it'll be drop in punish time */
            int halfCnt = ((eventQuota.dailyCnt - dailyCnt + DAY_HOUR - 1) / DAY_HOUR + 1) / 2;
            MISIGHT_LOGD("event hourCnt=%d halfCnt=%d, maxCnt=%u", hourCnt, halfCnt, eventQuota.dailyCnt);
            hourCnt++;
            if (hourCnt > halfCnt) {
                uint64_t punishSecond = (hourCnt - halfCnt) * (DAY_SECONDS - (newLast - startTime_))/(eventQuota.dailyCnt - dailyCnt);
                MISIGHT_LOGD("event %d over hour check punish, hourCnt=%d halfCnt=%d, maxCnt=%u puns=%d", event->eventId_, hourCnt, halfCnt, eventQuota.dailyCnt, (int)punishSecond);
                if ((event->happenTime_ - lastTime) < punishSecond) {
                    event->SetValue(EventUtil::EVENT_DROP_REASON, "event happen too sequence, drop it by punish");
                    MISIGHT_LOGD("event %d over hour cnt drop it, hourCnt=%d halfCnt=%d, maxCnt=%u", event->eventId_, hourCnt, halfCnt, eventQuota.dailyCnt);
                    newLast = lastTime;
                    hourCnt--;
                    dailyCnt--;
                    overCnt++;
                    drop = true;
                }
            }
        }
    }

    // if event happen interval in one minute, then not catch log
    if (!drop) {
        if ((event->happenTime_ - lastLogTime) < eventQuota.logSpace) {
            MISIGHT_LOGD("event %d will not catch log %lu, lastlogtime=%lu, space=%d", event->eventId_, event->happenTime_, lastLogTime, eventQuota.logSpace);
            event->SetValue(EventUtil::EVENT_NOCATCH_LOG, "last catch log in " + std::to_string(eventQuota.logSpace) + "s");
        } else {
            lastLogTime = TimeUtil::GetTimestampSecond();
        }
    }

    // daily occur count
    dailyCnt++;

    EventQuery query;
    query.Where(EventDailyTable::ID, EQ, filedMap[EventDailyTable::ID]);
    auto eventDb = &EventDatabase::getInstance();
    eventDb->Update(EventDailyTable::TBL_NAME, {EventDailyTable::DAILY_CNT, EventDailyTable::HOUR_CNT,
        EventDailyTable::NEXT_EXPIRE_TIME, EventDailyTable::OVER_CNT, EventDailyTable::LAST_HAPPEN_TIME,
        EventDailyTable::LAST_LOG_TIME}, {std::to_string(dailyCnt), std::to_string(hourCnt), std::to_string(expireTime),
        std::to_string(overCnt), std::to_string(newLast), std::to_string(lastLogTime)}, query);
    return drop;
}

bool EventDatabaseHelper::ProcessNewDayEvent(sp<Event> event)
{
    auto eventDb = &EventDatabase::getInstance();
    uint64_t nextDay = startTime_ + DAY_SECONDS;
    // check if a new day, then update new day time and insert event into  event daily table
    if (event->happenTime_ < nextDay) {
        return false;
    }
    startTime_ = event->happenTime_;
    MISIGHT_LOGD("daily tbl truncate event happen time %" PRId64 ", db time =%" PRId64, event->happenTime_, nextDay);

    // truncate table
    eventDb->TruncateAndCreate(EventDailyTable::TBL_NAME);

    // update db version
    EventQuery query;
    query.Where(EventDBVersionTable::ID, EQ, dbId_);
    eventDb->Update(EventDBVersionTable::TBL_NAME, {EventDBVersionTable::START_TIME}, {std::to_string(startTime_)}, query);
    return true;
}

bool EventDatabaseHelper::DetectCanDrop(sp<Event> event)
{
    auto eventDb = &EventDatabase::getInstance();

    // check if event happen next day
    if (!ProcessNewDayEvent(event)) {
        // not a new day, then query daily count table
        auto dailyTable = eventDb->GetTable(EventDailyTable::TBL_NAME);
        if (dailyTable == nullptr) {
            MISIGHT_LOGE("failed get tbl %s no drop", EventDailyTable::TBL_NAME.c_str());
            return false;
        }

        EventQuery query;
        query.Select(dailyTable->GetFieldNameList())
            .Where(EventDailyTable::EVENT_ID, EQ, event->eventId_);
        auto queryRet = eventDb->Query(EventDailyTable::TBL_NAME, query);
        if (queryRet.size() > 0) {
            // check event drop
            return DetectDropByTime(event, queryRet.front());
        }
    }
    MISIGHT_LOGD("daily tbl not have event %d, add it", event->eventId_);
    // event not in daily table, then insert event into the daily table
    eventDb->Insert(EventDailyTable::TBL_NAME,
        {EventDailyTable::EVENT_ID, EventDailyTable::DAILY_CNT, EventDailyTable::OVER_CNT,
        EventDailyTable::HOUR_CNT, EventDailyTable::NEXT_EXPIRE_TIME, EventDailyTable::LAST_HAPPEN_TIME,
        EventDailyTable::LAST_LOG_TIME},
        {std::to_string(event->eventId_), "1", "0", "1", std::to_string(event->happenTime_ + HOUR_SECONDS),
        std::to_string(event->happenTime_), std::to_string(TimeUtil::GetTimestampSecond())});
    return false;
}

void EventDatabaseHelper::ProcessFaultEvent(sp<Event> event)
{
    if (platformContext_ == nullptr) {
        return;
    }

    if (DetectCanDrop(event)) {
        return;
    }

    MISIGHT_LOGD("event %d not drop, then insert it into event table", event->eventId_);
    auto eventDb = &EventDatabase::getInstance();

    EventQuery query;
    query.Select({EventTable::ID, EventTable::OCCURED_CNT})
         .Where(EventTable::EVENT_ID, EQ, event->eventId_)
         .And(EventTable::VERSION_ID, EQ, versionId_)
         .And(EventTable::PARAM_LIST, EQ, event->GetValue(EventTable::PARAM_LIST));
    auto queryRet = eventDb->Query(EventTable::TBL_NAME, query);
    if (queryRet.size() > 0) {
        // event exist, update occured count
        int32_t id = StringUtil::ConvertInt(queryRet.front()[EventTable::ID]);
        int32_t occurCntVal = StringUtil::ConvertInt(queryRet.front()[EventTable::OCCURED_CNT]) + 1;

        EventQuery queryTable;
        queryTable.Where(EventTable::ID, EQ, id);
        eventDb->Update(EventTable::TBL_NAME, {EventTable::OCCURED_CNT, EventTable::FIRST_TIME},
            {std::to_string(occurCntVal), std::to_string(event->happenTime_)}, queryTable);
    } else {
        // not exist, add event
        eventDb->Insert(EventTable::TBL_NAME,
            {EventTable::EVENT_ID, EventTable::VERSION_ID, EventTable::OCCURED_CNT,
                EventTable::FIRST_TIME, EventTable::PARAM_LIST},
            {std::to_string(event->eventId_), std::to_string(versionId_), "1",
                std::to_string(event->happenTime_), event->GetValue(EventTable::PARAM_LIST)});
    }
}

void EventDatabaseHelper::SaveRawEvent(sp<Event> event)
{
    auto eventDb = &EventDatabase::getInstance();
    int insertId = eventDb->InsertAndGetID(RawEventTable::TBL_NAME,
        {RawEventTable::EVENT_ID, RawEventTable::PARAM_LIST, RawEventTable::TIMESTAMP},
        {std::to_string(event->eventId_), event->GetValue(EventTable::PARAM_LIST), std::to_string(event->happenTime_)});
    event->SetValue(EventUtil::RAW_EVENT_DB_ID, insertId);
}

void EventDatabaseHelper::SaveUserActivateTime(sp<Event> event)
{
    auto eventDb = &EventDatabase::getInstance();
    EventQuery query;
    query.Where(EventDBVersionTable::ID, EQ, dbId_);
    eventDb->Update(EventDBVersionTable::TBL_NAME, {EventDBVersionTable::ACTIVATE_TIME}, {std::to_string(event->happenTime_)}, query);
    DevInfo::SetActivateTime(event->happenTime_);
    MISIGHT_LOGI("update user activate %" PRId64, event->happenTime_);
}

void EventDatabaseHelper::DumpTable(int fd, const std::string& tblName)
{
    auto eventDb = &EventDatabase::getInstance();

    dprintf(fd, "dump table %s:\n", tblName.c_str());
    auto dailyTable = eventDb->GetTable(tblName);
    if (dailyTable == nullptr) {
        dprintf(fd, "err: table not exist");
        return;
    }

    EventQuery query;
    auto nameList = dailyTable->GetFieldNameList();
    query.Select(nameList);
    query.Order("id", false);
    auto queryRet = eventDb->Query(tblName, query, 10);
    if (queryRet.size() == 0) {
        return;
    }
    std::string head = "table:";
    for (auto name : nameList) {
        head += "    " + name;
    }
    dprintf(fd, "%s\n", head.c_str());

    for (auto loop : queryRet) {
        std::string val = "       ";

        int size = val.size();
        for (auto name : nameList) {
            if (val.size() < size) {
                val += std::string(size - val.size(), ' ');
            }
            val += "    " + loop[name];
            size += 4 + name.size();
        }
        dprintf(fd, "%s\n\n", val.c_str());
    }
}

void EventDatabaseHelper::DumpQueryDb(int fd, const std::vector<std::string>& cmds)
{
    // dump database
    if (cmds.size() >= 2) {
        DumpTable(fd, cmds[1]);
    } else {
        MISIGHT_LOGD("cmds %zu", cmds.size());
        DumpTable(fd, EventDBVersionTable::TBL_NAME);
        DumpTable(fd, EventTable::TBL_NAME);
        DumpTable(fd, EventDailyTable::TBL_NAME);
	DumpTable(fd, EventVersionTable::TBL_NAME);
    }
}

void EventDatabaseHelper::DumpUpdateDb(int fd, const std::vector<std::string>& cmds)
{
    // dump database
    // update event_daily 901001001, mean delete event info
    // update evengt_daily 901001001 daily_cnt 1 over_cnt 2 hour_cnt 3
    if (cmds.size() < 3) {
        return;
    }

    std::string tblName = cmds[1];
    std::string eventId = cmds[2];

    dprintf(fd, "update table %s:\n", tblName.c_str());

    auto eventDb = &EventDatabase::getInstance();
    auto dailyTable = eventDb->GetTable(tblName);
    if (dailyTable == nullptr) {
        dprintf(fd, "err: table not exist");
        return;
    }
    EventQuery query;
    if (eventId != "0") {
        query.Where(EventDailyTable::EVENT_ID, EQ, eventId);
    }
    if (cmds.size() == 3) {
        bool ret = eventDb->Delete(tblName, query);
        dprintf(fd, "delete eventid %s %s", eventId.c_str(), ret ? "OK" : "Fail");
        return;
    }
    if (cmds.size() > 3) {
        std::vector<std::string> nameList;
        std::vector<std::string> valueList;
        for (int i = 3; i < cmds.size() && (i + 1) < cmds.size(); i = i+2) {
            nameList.push_back(cmds[i]);
            valueList.push_back(cmds[i+1]);
        }
        bool ret = eventDb->Update(tblName, nameList, valueList, query);
        dprintf(fd, "update eventId %s %s", eventId.c_str(), ret ? "OK" : "Fail");
    }
}
}
}
