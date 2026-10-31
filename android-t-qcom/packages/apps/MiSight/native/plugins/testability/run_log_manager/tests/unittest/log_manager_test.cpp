/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability plugin log_manager test head file
 *
 */

#include "log_manager_test.h"

#include <string>
#include <thread>

#include <json/json.h>

#include "common.h"
#include "dev_info.h"
#include "event.h"
#include "event_loop.h"
#include "event_util.h"
#include "run_log_manager.h"
#include "file_util.h"
#include "platform_common.h"
#include "plugin_platform.h"
#include "public_util.h"


using namespace android;
using namespace android::MiSight;

TEST_F(LogManagerTest, TestValue) {

    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(false));

    auto &platform = PluginPlatform::getInstance();
    sp<RunLogManager> logManager = new RunLogManager();
    logManager->SetPluginContext(&platform);

    logManager->OnLoad();

    platform.SetSysProperity("persist.sys.log_filter_flag", "0", true);
    platform.SetSysProperity("persist.sys.log_time_gap_sec", "3", true);
    platform.SetSysProperity("persist.sys.log_max_line", "3", true);
    logManager->SetStaticSpace(3);

    sp<EventLoop> loop = new EventLoop("log_manager");
    loop->StartLoop();
    logManager->SetLooper(loop);
    logManager->OnLoad();

    std::this_thread::sleep_for(std::chrono::seconds(10));
    ASSERT_EQ(logManager->GetStaticSpace(), 480);

    sp<Event> event = nullptr;
    logManager->OnEvent(event);
    logManager->OnUnload();

    platform.SetSysProperity("persist.sys.log_filter_flag", "0", true);
    platform.SetSysProperity("persist.sys.log_time_gap_sec", "10", true);
    platform.SetSysProperity("persist.sys.log_max_line", "200", true);
}
