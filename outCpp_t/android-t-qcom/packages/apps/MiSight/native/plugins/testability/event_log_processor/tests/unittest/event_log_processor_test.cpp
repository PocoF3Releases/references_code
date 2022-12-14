/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability plugin event log processor test implement file
 *
 */
#include "event_log_processor_test.h"


#include <string>

#include "common.h"
#include "dev_info.h"
#include "event.h"
#include "event_util.h"
#include "event_log_processor.h"
#include "file_util.h"
#include "platform_common.h"
#include "plugin_platform.h"
#include "public_util.h"



using namespace android;
using namespace android::MiSight;
android::sp<android::MiSight::EventLogProcessor> EventLogProcessorTest::eventLog_ = nullptr;

void EventLogProcessorTest::SetUpTestCase()
{
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(false));
    auto &platform = PluginPlatform::getInstance();
    DevInfo::SetUploadSwitch(&platform, DevInfo::UE_SWITCH_ON);
    DevInfo::SetActivateTime(time(nullptr));

    eventLog_ = new EventLogProcessor();
    eventLog_->SetPluginContext(&platform);
    eventLog_->OnLoad();
}

void EventLogProcessorTest::TearDownTestCase()
{
    auto &platform = PluginPlatform::getInstance();
    DevInfo::SetUploadSwitch(&platform, DevInfo::UE_SWITCH_OFF);
}


TEST_F(EventLogProcessorTest, TestEventLogPack) {
    // 1. star plugin platform
    sp<Event> event = new Event("testEventLogPack");
    event->eventId_ = EVENT_901;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    eventLog_->OnEvent(event);

    std::string zipName = event->GetValue(EventUtil::EVENT_LOG_ZIP_NAME);
    ASSERT_NE("", zipName);

    sp<Event> eventPack = new Event("eventPack");
    eventPack->eventId_ = EVENT_SERVICE_UPLOAD_ID;
    eventPack->messageType_ = Event::MessageType::PLUGIN_MAINTENANCE;
    eventLog_->OnEvent(eventPack);
    printf("event zip path=%s\n", zipName.c_str());
    ASSERT_EQ(true, FileUtil::FileExists(zipName));
};

TEST_F(EventLogProcessorTest, TestEventDropNoLog) {

    // 1. star plugin platform
    sp<Event> event = new Event("testEventLogPack");
    event->eventId_ = EVENT_901;
    event->SetValue(EventUtil::EVENT_DROP_REASON, "drop it");
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    eventLog_->OnEvent(event);
    ASSERT_EQ("", event->GetValue(EventUtil::EVENT_LOG_ZIP_NAME));
};

TEST_F(EventLogProcessorTest, TestEventNoLog) {
    // 1. star plugin platform

    sp<Event> event = new Event("testEventLogPack");
    event->eventId_ = EVENT_902;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    eventLog_->OnEvent(event);
    ASSERT_EQ("", event->GetValue(EventUtil::EVENT_LOG_ZIP_NAME));
};

TEST_F(EventLogProcessorTest, TestEventLogQuota)
{
    // 1. star plugin platform
    auto &platform = PluginPlatform::getInstance();
    std::string workDir = platform.GetWorkDir() + "901";

    sp<Event> event = new Event("testEventLogPack");
    event->eventId_ = EVENT_901;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    eventLog_->OnEvent(event);
    std::string zipName = event->GetValue(EventUtil::EVENT_LOG_ZIP_NAME);
    ASSERT_NE("", zipName);
    printf("1.%s\n",zipName.c_str());
    event->SetValue(EventUtil::EVENT_LOG_ZIP_NAME, "");
    eventLog_->OnEvent(event);
    zipName = event->GetValue(EventUtil::EVENT_LOG_ZIP_NAME);
    ASSERT_NE("", zipName);
    printf("2.%s\n",zipName.c_str());
    event->SetValue(EventUtil::EVENT_LOG_ZIP_NAME, "");
    eventLog_->OnEvent(event);
    zipName = event->GetValue(EventUtil::EVENT_LOG_ZIP_NAME);
    ASSERT_NE("", zipName);
    printf("3.%s\n",zipName.c_str());

    std::string zipPattern = "EventId_\\S+_\\d{14}_901\\d{6}.zip";
    std::vector<std::string> zipFiles = eventLog_->GetFileInDirByPattern(workDir, zipPattern, true, true);

    for (auto zip : zipFiles) {
        printf("loop.%s\n",zip.c_str());
    }
    ASSERT_EQ(1, zipFiles.size());
}

TEST_F(EventLogProcessorTest, TestEventLogPackWithUeOff) {
    // 1. star plugin platform
    auto &platform = PluginPlatform::getInstance();
    DevInfo::SetUploadSwitch(&platform, DevInfo::UE_SWITCH_OFF);
    std::string workDir = platform.GetWorkDir() + "901";
    workDir = workDir + "/ue_off/";

    std::string zipPattern = "EventId_\\S+_\\d{14}_901\\d{6}.zip";
    /*std::vector<std::string> zipFiles = eventLog_->GetFileInDirByPattern(workDir, zipPattern, true, true);
    for (auto file: zipFiles) {
        FileUtil::DeleteFile(file);
    }*/

    sp<Event> event = new Event("testEventLogPack");
    event->eventId_ = EVENT_901;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    eventLog_->OnEvent(event);


    std::vector<std::string> zipFilesNew = eventLog_->GetFileInDirByPattern(workDir, zipPattern, true, true);
    ASSERT_EQ(1, zipFilesNew.size());
};

