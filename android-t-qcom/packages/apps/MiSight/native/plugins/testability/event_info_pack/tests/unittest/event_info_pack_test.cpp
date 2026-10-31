/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability plugin event info pack test head file
 *
 */

#include "event_info_pack_test.h"

#include <string>

#include <json/json.h>

#include "common.h"
#include "dev_info.h"
#include "event.h"
#include "event_util.h"
#include "event_info_pack.h"
#include "file_util.h"
#include "platform_common.h"
#include "plugin_platform.h"
#include "public_util.h"


using namespace android;
using namespace android::MiSight;

TEST_F(EventInfoPackTest, TestFaultEventInfo) {

    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(false));

    auto &platform = PluginPlatform::getInstance();
	DevInfo::SetUploadSwitch(&platform, DevInfo::UE_SWITCH_ON);
    sp<EventInfoPack> eventPack = new EventInfoPack();
    eventPack->SetPluginContext(&platform);
    eventPack->OnLoad();

    // 2.delete eventinfo.901 file
    std::string eventDir = platform.GetWorkDir() + EventUtil::FAULT_EVENT_DEFAULT_DIR;
	FileUtil::DeleteFile(eventDir);
    eventDir +=  "/eventinfo.901";
    FileUtil::DeleteFile(eventDir);

    sp<Event> event = new Event("testEventInfoPack");
    event->eventId_ = EVENT_901;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    event->happenTime_ = std::time(nullptr);
    event->SetValue(EventUtil::DEV_SAVE_EVENT, "{\"a1\":10,\"a2\": \"string\"}");
    eventPack->OnEvent(event);
    eventPack->OnEvent(event);
    eventPack->OnEvent(event);
    eventPack->OnEvent(event);

    ASSERT_EQ(true, FileUtil::FileExists(eventDir));

}


TEST_F(EventInfoPackTest, TestRawEventInfo) {
}


TEST_F(EventInfoPackTest, TestFaultEventDropNoInfo) {
    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(false));

    auto &platform = PluginPlatform::getInstance();
    sp<EventInfoPack> eventPack = new EventInfoPack();
    eventPack->SetPluginContext(&platform);
    eventPack->OnLoad();

    // 2.delete eventinfo.901 file
    DevInfo::SetUploadSwitch(&platform, DevInfo::UE_SWITCH_ON);
    std::string eventDir = platform.GetWorkDir() + EventUtil::FAULT_EVENT_DEFAULT_DIR + "/eventinfo.902";
	FileUtil::DeleteFile(eventDir);

    sp<Event> event = new Event("testEventInfoPack");
    event->eventId_ = EVENT_902;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    event->happenTime_ = std::time(nullptr);
    event->SetValue(EventUtil::DEV_SAVE_EVENT, "{\"a1\":10,\"a2\": \"string\"}");
    event->SetValue(EventUtil::EVENT_DROP_REASON, "drop");
    eventPack->OnEvent(event);

    ASSERT_EQ(false, FileUtil::FileExists(eventDir));
	eventPack->OnUnload();
}

TEST_F(EventInfoPackTest, TestUeOffEventInfo) {
    // 1. star plugin platform
    auto &platform = PluginPlatform::getInstance();
    DevInfo::SetUploadSwitch(&platform, DevInfo::UE_SWITCH_OFF);
    std::string workDir = platform.GetWorkDir() + EventUtil::FAULT_EVENT_DEFAULT_DIR + "/ue_off/eventinfo.902";
    FileUtil::DeleteFile(workDir);
    sp<EventInfoPack> eventPack = new EventInfoPack();
    eventPack->SetPluginContext(&platform);
    eventPack->OnLoad();

    sp<Event> event = new Event("testEventInfoPack");
    event->eventId_ = EVENT_902;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    event->happenTime_ = std::time(nullptr);
    event->SetValue(EventUtil::DEV_SAVE_EVENT, "{\"a1\":10,\"a2\": \"string\"}");
    eventPack->OnEvent(event);
    eventPack->OnEvent(event);
    eventPack->OnEvent(event);
    eventPack->OnEvent(event);

    ASSERT_EQ(true, FileUtil::FileExists(workDir));
}

TEST_F(EventInfoPackTest, TestEventInfoOverSize) {
	// 1. star plugin platform
    auto &platform = PluginPlatform::getInstance();
    DevInfo::SetUploadSwitch(&platform, DevInfo::UE_SWITCH_ON);
    std::string workDir = platform.GetWorkDir() + EventUtil::FAULT_EVENT_DEFAULT_DIR + "/eventinfo.902";
	std::string workDir0 = workDir+".0";
    FileUtil::DeleteFile(workDir);
	FileUtil::DeleteFile(workDir0);
    sp<EventInfoPack> eventPack = new EventInfoPack();
    eventPack->SetPluginContext(&platform);
    eventPack->OnLoad();

    sp<Event> event = new Event("testEventInfoPack");
    event->eventId_ = EVENT_902;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    event->happenTime_ = std::time(nullptr);
    event->SetValue(EventUtil::DEV_SAVE_EVENT, "{\"Detail\":[{\"MaxFilterCnt\":26,\"MaxHappenTime\":\"2022-11-18 09:37:24\",\"PID\":5486,\"ProcessName\":\"com.miui.analytics\",\"TotalFilterCnt\":1,\"TotalFilterLog\":26,\"UID\":10137},{\"MaxFilterCnt\":16,\"MaxHappenTime\":\"2022-11-18 09:35:27\",\"PID\":2264,\"ProcessName\":\"system_server\",\"TotalFilterCnt\":0,\"TotalFilterLog\":16,\"UID\":1000},{\"MaxFilterCnt\":1,\"MaxHappenTime\":\"2022-11-18 09:36:06\",\"PID\":4475,\"ProcessName\":\"com.android.systemui\",\"TotalFilterCnt\":2,\"TotalFilterLog\":1,\"UID\":1000},{\"MaxFilterCnt\":1,\"MaxHappenTime\":\"2022-11-18 09:36:06\",\"PID\":4475,\"ProcessName\":\"com.android.systemui\",\"TotalFilterCnt\":2,\"TotalFilterLog\":1,\"UID\":1000},{\"MaxFilterCnt\":1,\"MaxHappenTime\":\"2022-11-18 09:36:06\",\"PID\":4475,\"ProcessName\":\"com.android.systemui\",\"TotalFilterCnt\":2,\"TotalFilterLog\":1,\"UID\":1000}],\"FilterLine\":20,\"FilterSecond\":3}");

	for(int i=0; i < 60; i++) {
        eventPack->OnEvent(event);
    }

	struct stat srcState;
    memset(&srcState, 0, sizeof(struct stat));
    (void)stat(workDir.c_str(), &srcState);
    ASSERT_EQ(true, (srcState.st_size >= 51200));

    ASSERT_EQ(true, FileUtil::FileExists(workDir0));
	for(int i=0; i < 60; i++) {
        eventPack->OnEvent(event);
    }
	memset(&srcState, 0, sizeof(struct stat));
	(void)stat(workDir0.c_str(), &srcState);
	ASSERT_EQ(true, (srcState.st_size >= 51200));

	memset(&srcState, 0, sizeof(struct stat));
	(void)stat(workDir.c_str(), &srcState);
	ASSERT_EQ(true, (srcState.st_size < 51200));
}
