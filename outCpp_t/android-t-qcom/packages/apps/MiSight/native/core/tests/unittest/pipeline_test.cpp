/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight pipeline test implement file
 *
 */

#include "pipeline_test.h"

#include <chrono>
#include <string>
#include <thread>

#include "common.h"
#include "event_source.h"
#include "event_source_sample.h"
#include "platform_common.h"
#include "plugin_platform.h"
#include "plugin_sample1.h"
#include "plugin_sample2.h"
#include "plugin_sample4.h"


using namespace android;
using namespace android::MiSight;

TEST_F(PipelineTest, TestPipeline) {
    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(true));

    // 2. get event source and plugin sample4
    auto &platform = PluginPlatform::getInstance();
    auto pluginMap = platform.GetPluginMap();

    auto it1 = pluginMap.find("PluginSample4");
    ASSERT_TRUE(it1 != pluginMap.end());
    PluginSample4* sample4 = static_cast<PluginSample4*>(it1->second.get());

    auto it = pluginMap.find("EventSourceSample");
    ASSERT_TRUE(it != pluginMap.end());
    EventSource* eventSource = static_cast<EventSource*>(it->second.get());
    EventSourceSample* sourceSample = static_cast<EventSourceSample*>(eventSource);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(sourceSample->IsStart());

    // 3. trigger one event, verify pipeline
    sample4->eventInfo_ = "";
    sourceSample->CreateTestFile(EVENT_901);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(sample4->eventInfo_ == "EventSourceSample-PluginSample1-PluginSample2-PluginSample3-PluginSample4");
}

TEST_F(PipelineTest, TestPipelineNoMatch) {
    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(true));

    // 2. get event source and plugin sample4
    auto &platform = PluginPlatform::getInstance();
    auto pluginMap = platform.GetPluginMap();

    auto it1 = pluginMap.find("PluginSample4");
    ASSERT_TRUE(it1 != pluginMap.end());
    PluginSample4* sample4 = static_cast<PluginSample4*>(it1->second.get());

    auto it = pluginMap.find("EventSourceSample");
    ASSERT_TRUE(it != pluginMap.end());
    EventSource* eventSource = static_cast<EventSource*>(it->second.get());
    EventSourceSample* sourceSample = static_cast<EventSourceSample*>(eventSource);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(sourceSample->IsStart());

    // 3. trigger one event, verify pipeline no match
    sample4->eventInfo_ = "";
    sourceSample->CreateTestFile(EVENT_904);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(sample4->eventInfo_ == "");
}


TEST_F(PipelineTest, TestPipelineRegisterListener) {
    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(true));

    // 2. get event source and plugin sample4
    auto &platform = PluginPlatform::getInstance();
    auto pluginMap = platform.GetPluginMap();

    auto it1 = pluginMap.find("PluginSample4");
    ASSERT_TRUE(it1 != pluginMap.end());
    PluginSample4* sample4 = static_cast<PluginSample4*>(it1->second.get());

    auto it = pluginMap.find("EventSourceSample");
    ASSERT_TRUE(it != pluginMap.end());
    EventSource* eventSource = static_cast<EventSource*>(it->second.get());
    EventSourceSample* sourceSample = static_cast<EventSourceSample*>(eventSource);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(sourceSample->IsStart());

    // 3. trigger one event, verify pipeline
    sample4->eventInfo_ = "";
    sourceSample->CreateTestFile(EVENT_906);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(sample4->eventInfo_ == "EventSourceSample-PluginSample3-PluginSample2-PluginSample4");
}

TEST_F(PipelineTest, TestAbnormal) {
    std::vector<sp<Plugin>> plugins;
    sp<Pipeline> pipeline = new Pipeline("test", plugins);

    ASSERT_EQ(false, pipeline->CanProcessEvent(nullptr));
    pipeline->ProcessPipeline(nullptr, 0);
    pipeline = nullptr;
}
