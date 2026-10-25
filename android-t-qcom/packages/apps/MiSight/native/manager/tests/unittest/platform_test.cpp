/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight platform test implement file
 *
 */

#include "platform_test.h"

#include <chrono>
#include <string>
#include <thread>

#include "common.h"
#include "event_source_sample.h"
#include "event_source.h"
#include "platform_common.h"
#include "plugin_platform.h"
#include "plugin_sample1.h"
#include "plugin_sample2.h"
#include "plugin_sample4.h"
#include "plugin_sample5.h"



using namespace android;
using namespace android::MiSight;


TEST_F(PlatformTest, TestPlatformParse) {
    auto &platform = PluginPlatform::getInstance();

    const int argc = 2;
    char argv0[] =  "/system/bin/misight";
    char argv1[] = "-b";
    char* argv[argc] = {argv0, argv1};
    ASSERT_EQ(-1, platform.ParseArgs(argc, argv));

    const int argcA = 7;
    char argvA0[] = "/system/bin/misight";
    char argvA1[] = "-configDir";
    char argvA2[] = "/data/test";
    char argvA3[] = "-configName";
    char argvA4[] = "config";
    char argvA5[] = "-workDir";
    char argvA6[] = "/data/miuilog/misight";
    char* argvA[argcA] = {argvA0, argvA1, argvA2, argvA3, argvA4, argvA5, argvA6};
    ASSERT_EQ(0, platform.ParseArgs(argcA, argvA));

    ASSERT_TRUE(platform.GetPluginConfigPath() == "/data/test/config");
}


TEST_F(PlatformTest, TestPlatformStart) {
    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(true));

    // 2. verify load plugin and pipeline
    auto &platform = PluginPlatform::getInstance();
    auto pluginMap = platform.GetPluginMap();
    ASSERT_EQ(6, pluginMap.size());
    ASSERT_EQ(nullptr, pluginMap["PluginSample5"]);
    ASSERT_EQ(3, platform.GetPipelineMap().size());

    // 3. event source started
    auto it = pluginMap.find("EventSourceSample");
    ASSERT_TRUE(it != pluginMap.end());
    EventSource* eventSource = static_cast<EventSource*>(it->second.get());
    EventSourceSample* sample = static_cast<EventSourceSample*>(eventSource);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(sample->IsStart());

    // 4. verify delay plugin
    std::this_thread::sleep_for(std::chrono::seconds(5));
    pluginMap = platform.GetPluginMap();
    ASSERT_EQ(6, pluginMap.size());
    ASSERT_TRUE(nullptr != pluginMap["PluginSample5"]);
}


TEST_F(PlatformTest, TestPlatformUnorderQueue) {
    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(true));

    // 2. get plugin sample1 and plugin sample2
    auto &platform = PluginPlatform::getInstance();
    auto pluginMap = platform.GetPluginMap();

    auto it1 = pluginMap.find("PluginSample1");
    ASSERT_TRUE(it1 != pluginMap.end());
    PluginSample1* sample1 = static_cast<PluginSample1*>(it1->second.get());

    auto it2 = pluginMap.find("PluginSample2");
    ASSERT_TRUE(it2 != pluginMap.end());
    PluginSample2* sample2 = static_cast<PluginSample2*>(it2->second.get());

    // 3. sample1 subscribe EVENT_903 form sample2 event
    sample1->recvUnorderCnt_ = 0;
    sp<Event> event = new Event("PluginSample2");
    event->eventId_ = EVENT_903;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    sample2->OnEvent(event);

    // 4. verify sample1 received subscribe EVENT_903
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(1, sample1->recvUnorderCnt_);

    // loop3. sample1 subscribe EVENT_907 form sample2 event
    event->eventId_ = EVENT_907;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    sample2->OnEvent(event);

    // loop4. verify sample1 received subscribe EVENT_907
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(2, sample1->recvUnorderCnt_);

    // loop3. sample1 subscribe EVENT_909 form sample2 event
    event->eventId_ = EVENT_909;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    sample2->OnEvent(event);

    // loop4. verify sample1 received subscribe EVENT_909
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(3, sample1->recvUnorderCnt_);

    // loop3. sample1 subscribe EVENT_910 form sample2 event
    event->eventId_ = EVENT_910;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    sample2->OnEvent(event);
    // loop4. verify sample1 received subscribe EVENT_910
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(4, sample1->recvUnorderCnt_);

    // loop3. sample1 subscribe EVENT_911 form sample2 event
    event->eventId_ = EVENT_911;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    sample2->OnEvent(event);

    // loop4. verify sample1 received subscribe EVENT_911
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(5, sample1->recvUnorderCnt_);

    // loop3. sample1 subscribe EVENT_912 form sample2 event
    event->eventId_ = EVENT_912;
    event->messageType_ = Event::MessageType::PLUGIN_MAINTENANCE;
    sample2->OnEvent(event);

    // loop4. verify sample1 not received subscribe EVENT_912
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(5, sample1->recvUnorderCnt_);
}

TEST_F(PlatformTest, TestPlatformPostAsyncEvent) {
    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(true));

    // 2. get plugin sample1 and plugin sample2
    auto &platform = PluginPlatform::getInstance();
    auto pluginMap = platform.GetPluginMap();


    auto it2 = pluginMap.find("PluginSample2");
    ASSERT_TRUE(it2 != pluginMap.end());
    PluginSample2* sample2 = static_cast<PluginSample2*>(it2->second.get());

    // 3. plugin sample2 send async event_904 to plugin sample1
    sp<Event> event = new Event("PluginSample2");
    event->eventId_ = EVENT_904;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    std::string eventIdStr = std::to_string(event->eventId_);
    event->SetValue(eventIdStr, "start");
    sample2->OnEvent(event);

    // 4. verify sample1 receive event
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(event->GetValue(eventIdStr) == "start-PluginSample2-PluginSample1");
}

TEST_F(PlatformTest, TestPlatformPostSyncEvent) {
    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(true));

    // 2. get plugin sample1 and plugin sample2
    auto &platform = PluginPlatform::getInstance();
    auto pluginMap = platform.GetPluginMap();

    auto it2 = pluginMap.find("PluginSample2");
    ASSERT_TRUE(it2 != pluginMap.end());
    PluginSample2* sample2 = static_cast<PluginSample2*>(it2->second.get());

    // 3. plugin sample2 send sync event_905 to plugin sample1
    sp<Event> event = new Event("PluginSample2");
    event->eventId_ = EVENT_905;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    std::string eventIdStr = std::to_string(event->eventId_);
    event->SetValue(eventIdStr, "start");
    sample2->OnEvent(event);

    // 4. verify sample1 receive event
    std::this_thread::sleep_for(std::chrono::seconds(1));
    printf("event get value %s\n", event->GetValue(eventIdStr).c_str());
    ASSERT_TRUE(event->GetValue(eventIdStr) == "start-PluginSample2-PluginSample1");
}


TEST_F(PlatformTest, TestPlatformProperity) {
    // 1. get plugin platform
    auto &platform = PluginPlatform::getInstance();

    // 2. get properity
    std::string env = platform.GetSysProperity("misight.test", "");
    ASSERT_TRUE(env == "");
    ASSERT_TRUE(platform.SetSysProperity("misight.test", "test", false));
    env = platform.GetSysProperity("misight.test", "");
    ASSERT_TRUE(env == "test");
    ASSERT_TRUE(platform.SetSysProperity("misight.test", "", true));
}

TEST_F(PlatformTest, TestPlatformInsertPipeline) {
    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(true));

    // 2. get plugin sample1 and plugin sample4
    auto &platform = PluginPlatform::getInstance();
    auto pluginMap = platform.GetPluginMap();

    auto it1 = pluginMap.find("PluginSample1");
    ASSERT_TRUE(it1 != pluginMap.end());
    PluginSample1* sample1 = static_cast<PluginSample1*>(it1->second.get());

    auto it4 = pluginMap.find("PluginSample4");
    ASSERT_TRUE(it4 != pluginMap.end());
    PluginSample4* sample4 = static_cast<PluginSample4*>(it4->second.get());

    // 3. insert pipeline
    sp<Event> event = new Event("insert");
    event->eventId_ = EVENT_901;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    std::string eventIdStr = std::to_string(event->eventId_);
    event->SetValue(eventIdStr, "insert");

    sample4->eventInfo_ = "";
    sample1->GetPluginContext()->InsertEventToPipeline("PipelineName1", event);


    // 3. trigger one event, verify pipeline
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(sample4->eventInfo_ == "insert-PluginSample1-PluginSample2-PluginSample3-PluginSample4");
}

