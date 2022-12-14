/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin testability event store manager implement file
 *
 */

#include "event_store_manager_test.h"


#include <string>

#include "common.h"
#include "dev_info.h"
#include "event.h"
#include "event_database.h"
#include "event_database_helper.h"
#include "event_query.h"
#include "event_table.h"
#include "event_util.h"
#include "file_util.h"
#include "platform_common.h"
#include "plugin_platform.h"
#include "public_util.h"


using namespace android;
using namespace android::MiSight;

android::sp<android::MiSight::EventStorageManager> EventStoreManagerTest::eventStore_ = nullptr;

void EventStoreManagerTest::SetUpTestCase()
{
    // 1. star plugin
    ASSERT_EQ(true, InitEventStorePlugin());

}

bool EventStoreManagerTest::InitEventStorePlugin()
{
    if (eventStore_ != nullptr) {
        printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< only init 1\n");
        return true;
    }
    if (!PlatformCommon::InitPluginPlatform(false)) {
        printf("init plugin platform failed\n");
        return false;
    }

    auto &platform = PluginPlatform::getInstance();
    std::string dbPath = platform.GetWorkDir() + "event.db";
    FileUtil::DeleteFile(dbPath);
    eventStore_ = new EventStorageManager();
    eventStore_->SetPluginContext(&platform);

    sp<EventLoop> pluginLoop = new EventLoop("eventdb");
    pluginLoop->StartLoop();

    eventStore_->SetLooper(pluginLoop);
    eventStore_->OnLoad();

    return true;
}

TEST_F(EventStoreManagerTest, TestSaveFaultEvent) {
    // 1. star plugin
    // according test file event_quotal.xml, EVENT_901 can save 2 times in one hour

    sp<Event> event = new Event("testEventStoreManager");
    event->eventId_ = EVENT_901;
    int startTime = TimeUtil::GetTimestampSecond();
    event->happenTime_ = startTime;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    event->SetValue(EventTable::PARAM_LIST, "{\"par1\": 1, \"par2\":\"it's par2\", \"par3\": [1,2,3],\"par4\": (1,2,3)}");

    // first send
    printf("start 1 event %d:\n", startTime);
    eventStore_->OnEvent(event);
    ASSERT_EQ("", event->GetValue(EventUtil::EVENT_DROP_REASON));

    // after 1 second send, will drop
    event->happenTime_ = startTime + 1;
    event->SetValue(EventUtil::EVENT_DROP_REASON, "");
    printf("start 2 event %d:\n", (int)event->happenTime_);
    eventStore_->OnEvent(event);
    ASSERT_EQ("", event->GetValue(EventUtil::EVENT_DROP_REASON));

    // after 30 second send, not drop
    event->happenTime_ = startTime + 30;
    event->SetValue(EventUtil::EVENT_DROP_REASON, "");
    printf("start 3 event %d:\n", (int)event->happenTime_);
    eventStore_->OnEvent(event);
    ASSERT_NE("", event->GetValue(EventUtil::EVENT_DROP_REASON));

    // after 31 second send, will drop
    event->happenTime_ = startTime + 31;
    event->SetValue(EventUtil::EVENT_DROP_REASON, "");
    printf("start 4 event %d:\n", (int)event->happenTime_);
    eventStore_->OnEvent(event);
    ASSERT_NE("", event->GetValue(EventUtil::EVENT_DROP_REASON));

    // after 1 hour send, not drop
    event->happenTime_ = startTime + HOUR_SECONDS + 1;
    event->SetValue(EventUtil::EVENT_DROP_REASON, "");
    printf("start 5 event %d:\n", (int)event->happenTime_);
    eventStore_->OnEvent(event);
    ASSERT_EQ("", event->GetValue(EventUtil::EVENT_DROP_REASON));

    auto eventDb = &EventDatabase::getInstance();
    EventQuery query;
    query.Select({EventTable::ID});
    auto queryRet = eventDb->Query(EventTable::TBL_NAME, query, 10);
    ASSERT_EQ(1, (int)queryRet.size());
};

TEST_F(EventStoreManagerTest, TestSaveRawEvent) {
    // 1. star plugin
    //ASSERT_EQ(true, InitEventStorePlugin());

    sp<Event> event = new Event("testEventStoreManager");
    event->eventId_ = EventUtil::RAW_EVENT_MIN +1;
    event->happenTime_ = TimeUtil::GetTimestampSecond();
    event->messageType_ = Event::MessageType::RAW_EVENT;

    event->SetValue(EventTable::PARAM_LIST, "{\"par1\": 1, \"par2\":\"it's par2\", \"par3\": [1,2,3]}");
    eventStore_->OnEvent(event);
    ASSERT_EQ("1", event->GetValue(EventUtil::RAW_EVENT_DB_ID));
    event->happenTime_ += 1;
    eventStore_->OnEvent(event);
    ASSERT_EQ("2", event->GetValue(EventUtil::RAW_EVENT_DB_ID));


    auto eventDb = &EventDatabase::getInstance();
    EventQuery query;
    query.Select({RawEventTable::ID});
    auto queryRet = eventDb->Query(RawEventTable::TBL_NAME, query, 10);
    ASSERT_EQ(2, (int)queryRet.size());

};

TEST_F(EventStoreManagerTest, TestEventDailyQuota) {
    // 1. star plugin
   // ASSERT_EQ(true, InitEventStorePlugin());


    sp<Event> event = new Event("testEventStoreManager");
    event->eventId_ = EVENT_902;
    event->happenTime_ = TimeUtil::GetTimestampSecond();
    event->messageType_ = Event::MessageType::FAULT_EVENT;

    event->SetValue(EventTable::PARAM_LIST, "{\"par1\": 1, \"par2\":\"it's par2\", \"par3\": [1,2,3]}");
    eventStore_->OnEvent(event);

    event->happenTime_ = TimeUtil::GetTimestampSecond() + 2 * HOUR_SECONDS;
    eventStore_->OnEvent(event);
    ASSERT_NE("", event->GetValue(EventUtil::EVENT_DROP_REASON));
};

TEST_F(EventStoreManagerTest, TestAbridgeDbSize) {
    int dayWeek = 7;
    sp<Event> event = new Event("testEventStoreManager");
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    int startTime = TimeUtil::GetTimestampSecond();


    event->SetValue(EventTable::PARAM_LIST, "{\"par1\": 1, \"par2\":\"it's par2\", \"par3\": [1,2,3]}" );

    for (int i = 0; i < dayWeek; i++) {
        event->eventId_ = EVENT_903 + i;
        event->happenTime_ = startTime - (dayWeek + i) * DAY_SECONDS;
        printf("startTime=%d,event->happenTime_=%lu\n", startTime, event->happenTime_);

        eventStore_->OnEvent(event);
        ASSERT_EQ("", event->GetValue(EventUtil::EVENT_DROP_REASON));

        event->eventId_ = EVENT_903 - i;
        event->happenTime_ = startTime - (dayWeek - i) * DAY_SECONDS;
        printf("startTime=%d,event->happenTime_=%lu\n", startTime, event->happenTime_);

        eventStore_->OnEvent(event);
        ASSERT_EQ("", event->GetValue(EventUtil::EVENT_DROP_REASON));
    }

    sp<EventDatabaseHelper> dbHelper = new EventDatabaseHelper(startTime);
    dbHelper->AbridgeDbSize();
    auto eventDb = &EventDatabase::getInstance();
    EventQuery query;
    query.Select({EventTable::ID});
    auto queryRet = eventDb->Query(EventTable::TBL_NAME, query, 100);
    ASSERT_EQ(dayWeek + 2, (int)queryRet.size());
};


// event database helper test case
TEST_F(EventDatabaseHelperTest, TestInitDatabase) {

    uint64_t startTime = TimeUtil::GetTimestampSecond();
    auto &platform = PluginPlatform::getInstance();
    std::string dbPath = "event" + std::to_string(startTime) + ".db";
    std::string cfgDir = platform.GetConfigDir();
    std::string version = DevInfo::GetMiuiVersion(&platform);

    // 1. check init first
    sp<EventDatabaseHelper> dbHelper = new EventDatabaseHelper(startTime);
    dbHelper->InitDatabase(&platform, dbPath, version);
    // check version table saved
    auto eventDb = &EventDatabase::getInstance();
    EventQuery query;
    query.Select({EventVersionTable::ID, EventVersionTable::VERSION});
    auto queryRet = eventDb->Query(EventVersionTable::TBL_NAME, query, 10);
    ASSERT_EQ(1, (int)queryRet.size());
    ASSERT_EQ(version, queryRet.front()[EventVersionTable::VERSION]);

    query.Clear();
    query.Select({EventDBVersionTable::DB_VERSION, EventDBVersionTable::START_TIME});
    queryRet = eventDb->Query(EventDBVersionTable::TBL_NAME, query, 10);
    ASSERT_EQ(1, (int)queryRet.size());
    ASSERT_EQ(std::to_string(startTime), queryRet.front()[EventDBVersionTable::START_TIME]);

    // 2. check restart
    dbHelper = new EventDatabaseHelper(startTime + 5); // 5s
    dbHelper->InitDatabase(&platform, dbPath, version);
    // check version table saved
    query.Clear();
    query.Select({EventVersionTable::ID, EventVersionTable::VERSION});
    queryRet = eventDb->Query(EventVersionTable::TBL_NAME, query, 10);
    ASSERT_EQ(1, (int)queryRet.size());
    ASSERT_EQ(version, queryRet.back()[EventVersionTable::VERSION]);

    // 2. check over one day restart
    startTime = startTime + DAY_SECONDS + 5;
    dbHelper = new EventDatabaseHelper(startTime); // 5s
    dbHelper->InitDatabase(&platform, dbPath, version);
    // check version table saved
    query.Clear();
    query.Select({EventVersionTable::ID, EventVersionTable::VERSION});
    queryRet = eventDb->Query(EventVersionTable::TBL_NAME, query, 10);
    ASSERT_EQ(1, (int)queryRet.size());
    ASSERT_EQ(version, queryRet.back()[EventVersionTable::VERSION]);


    query.Clear();
    query.Select({EventDBVersionTable::DB_VERSION, EventDBVersionTable::START_TIME});
    queryRet = eventDb->Query(EventDBVersionTable::TBL_NAME, query, 10);
    ASSERT_EQ(1, (int)queryRet.size());
    // starttime change
    ASSERT_EQ(std::to_string(startTime), queryRet.front()[EventDBVersionTable::START_TIME]);


    // 3.ota update check
    printf("ota update check>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    version = version + version;
    startTime += 5;
    dbHelper = new EventDatabaseHelper(startTime); // 5s
    dbHelper->InitDatabase(&platform, dbPath, version);

    // check version table saved
    query.Clear();
    query.Select({EventVersionTable::ID, EventVersionTable::VERSION});
    queryRet = eventDb->Query(EventVersionTable::TBL_NAME, query, 10);
    ASSERT_EQ(2, (int)queryRet.size());
    ASSERT_EQ(version, queryRet.back()[EventVersionTable::VERSION]);

    query.Clear();
    query.Select({EventDBVersionTable::DB_VERSION, EventDBVersionTable::START_TIME});
    queryRet = eventDb->Query(EventDBVersionTable::TBL_NAME, query, 10);
    ASSERT_EQ(1, (int)queryRet.size());
    ASSERT_EQ(std::to_string(startTime), queryRet.front()[EventDBVersionTable::START_TIME]);
};


TEST_F(EventStoreManagerTest, TestEventSaveActivate) {
    uint64_t startTime = TimeUtil::GetTimestampSecond();
    auto &platform = PluginPlatform::getInstance();
    std::string dbPath = "event" + std::to_string(startTime) + ".db";
    std::string cfgDir = platform.GetConfigDir();
    std::string version = DevInfo::GetMiuiVersion(&platform);

    // 1. check init first
    sp<EventDatabaseHelper> dbHelper = new EventDatabaseHelper(startTime);
    dbHelper->InitDatabase(&platform, dbPath, version);

    sp<Event> event = new Event("testEventStoreManager");
    event->messageType_ = Event::MessageType::PLUGIN_MAINTENANCE;
    event->eventId_ = EVENT_SERVICE_USER_ACTIVATE_ID;
    event->happenTime_ = startTime + 6000;
    eventStore_->OnEvent(event);

    auto eventDb = &EventDatabase::getInstance();
    EventQuery query;
    query.Select({EventDBVersionTable::ID, EventDBVersionTable::DB_VERSION,
        EventDBVersionTable::DEV_VERSION, EventDBVersionTable::START_TIME, EventDBVersionTable::ACTIVATE_TIME})
        .Order(EventDBVersionTable::ID, false);
    auto queryRet = eventDb->Query(EventDBVersionTable::TBL_NAME, query, 10);
    ASSERT_EQ(1, (int)queryRet.size());
    ASSERT_EQ(std::to_string(event->happenTime_), queryRet.front()[EventDBVersionTable::ACTIVATE_TIME]);
};

TEST_F(EventStoreManagerTest, TestStorageDumpsys) {
    int fd = open("/dev/null", O_WRONLY);
    std::vector<std::string> cmds;
    cmds.clear();
    eventStore_->Dump(fd, cmds);
    cmds.push_back("query");
    eventStore_->Dump(fd, cmds);
    cmds.clear();
    auto &platform = PluginPlatform::getInstance();
    platform.SetSysProperity("persist.sys.misight.safecheck", "1", true);
    cmds.push_back("update");
    cmds.push_back("test");
    eventStore_->Dump(fd, cmds);
    cmds.push_back("0");
    platform.SetSysProperity("persist.sys.misight.safecheck", "0", true);
    eventStore_->Dump(fd, cmds);

    cmds.clear();
    cmds.push_back("quota");
    cmds.push_back("902001001");

    eventStore_->Dump(fd, cmds);
}
