/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event looper test implement file
 *
 */

#include "event_loop_test.h"

#include <chrono>
#include <string>
#include <thread>

#include "common.h"
#include "event_dispatcher.h"
#include "event_source.h"
#include "event_source_sample.h"
#include "platform_common.h"
#include "plugin.h"
#include "plugin_platform.h"
#include "plugin_sample1.h"
#include "plugin_sample2.h"
#include "plugin_sample4.h"

using namespace android;
using namespace android::MiSight;

TEST_F(EventLoopTest, TestAddEventTestInterval) {
    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(true));

    // 2. get plugin sample1
    auto &platform = PluginPlatform::getInstance();
    auto pluginMap = platform.GetPluginMap();

    auto it1 = pluginMap.find("PluginSample1");
    ASSERT_TRUE(it1 != pluginMap.end());
    PluginSample1* sample1 = static_cast<PluginSample1*>(it1->second.get());

    auto looper = sample1->GetLooper();
    ASSERT_TRUE(nullptr != looper);

    // 3. sample1 add event to looper, event delay 5s
    sp<Event> event = new Event("test1");
    event->eventId_ = EVENT_903;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    std::string eventIdStr = std::to_string(event->eventId_);
    event->SetValue(eventIdStr, "async");

    looper->AddEvent(sample1, event, 5);

    // 4. verify receive event after 5s
    std::this_thread::sleep_for(std::chrono::seconds(3));

    ASSERT_TRUE(event->GetValue(eventIdStr) == "async");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(event->GetValue(eventIdStr) == "async-PluginSample1");

    // 5. sample1 add sync event, wait result
    event = new Event("test2");
    event->eventId_ = EVENT_903;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    eventIdStr = std::to_string(event->eventId_);
    event->SetValue(eventIdStr, "sync");

    bool ret = looper->AddEventWaitResult(sample1, event);
    ASSERT_TRUE(ret);
    ASSERT_TRUE(event->GetValue(eventIdStr) == "sync-PluginSample1");

}


TEST_F(EventLoopTest, TestAddEventTestRepeat) {

    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(true));

    // 2. get plugin sample1
    auto &platform = PluginPlatform::getInstance();
    auto pluginMap = platform.GetPluginMap();

    auto it1 = pluginMap.find("PluginSample1");
    ASSERT_TRUE(it1 != pluginMap.end());
    PluginSample1* sample1 = static_cast<PluginSample1*>(it1->second.get());

    auto looper = sample1->GetLooper();
    ASSERT_TRUE(nullptr != looper);

    // 3. sample1 add repeat
    sp<Event> event = new Event("test");
    event->eventId_ = EVENT_903;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    std::string eventIdStr = std::to_string(event->eventId_);
    event->SetValue(eventIdStr, "repeat");

    auto seq = looper->AddEvent(sample1, event, 5, true);

    // 3. sample1 wait repeat event
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(event->GetValue(eventIdStr) == "repeat");
    std::this_thread::sleep_for(std::chrono::seconds(15));
    ASSERT_TRUE(event->GetValue(eventIdStr) == "repeat-PluginSample1-PluginSample1-PluginSample1");

    looper->RemoveEventOrTask(seq);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    ASSERT_TRUE(event->GetValue(eventIdStr) == "repeat-PluginSample1-PluginSample1-PluginSample1");
}

TEST_F(EventLoopTest, TestEventBundle) {
    sp<Event> event = new Event("test1");
    event->SetValue("number", "1");
    ASSERT_EQ(event->GetIntValue("number"), 1);

    event->SetValue("number", 2);
    ASSERT_EQ(event->GetIntValue("number"), 2);
    std::map<std::string, std::string> bundle;
    bundle["first"] = "1";
    bundle["second"] = "2";
    event->SetKeyValuePairs(bundle);

    auto getBundle = event->GetKeyValuePairs();
    ASSERT_EQ(getBundle["first"], "1");
    ASSERT_EQ(getBundle["second"], "2");
    event = nullptr;
}

TEST_F(EventLoopTest, TestEventLoopFileHandlerRemove) {
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
    sourceSample->CreateTestFile(EVENT_901, 2);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(sample4->eventInfo_ == "EventSourceSample-PluginSample1-PluginSample2-PluginSample3-PluginSample4");
    // 4. verify callback return 0, not monitor any more
    sample4->eventInfo_ = "";
    sourceSample->CreateTestFile(EVENT_901, 2);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(sample4->eventInfo_ == "");

    // 5. trigger one event, verify pipeline
    sample4->eventInfo_ = "";
    sourceSample->CreateTestFile(EVENT_901, 1);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(sample4->eventInfo_ == "EventSourceSample-PluginSample1-PluginSample2-PluginSample3-PluginSample4");
    // 6. verify stop monitor
    sourceSample->StopMonitor(1);
    sample4->eventInfo_ = "";
    sourceSample->CreateTestFile(EVENT_901, 1);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_TRUE(sample4->eventInfo_ == "");
}


TEST_F(PluginTest, TestPluginApi) {
    sp<Plugin> plugin = new Plugin();

    plugin->SetName("testPlugin");
    plugin->SetVersion("testPlugin.1.1");
    PluginContext* context = new PluginContext();
    plugin->SetPluginContext(context);
    sp<EventLoop> looper = new EventLoop("testLooper");
    plugin->SetLooper(looper);

    ASSERT_EQ(plugin->PreToLoad(), true);
    plugin->OnLoad();
    sp<Event> event = new Event("sender");
    ASSERT_EQ(plugin->CanProcessEvent(event), true);
    ASSERT_EQ(plugin->OnEvent(event), 0);
    ASSERT_EQ(plugin->GetLooper(), looper);
    ASSERT_EQ(plugin->GetPluginContext(), context);
    ASSERT_EQ(plugin->GetVersion(), "testPlugin.1.1");
    ASSERT_EQ(plugin->GetName(), "testPlugin");
    plugin->OnUnload();

    context->PostUnorderedEvent(plugin, event);
    context->RegisterUnorderedEventListener(nullptr);

    context->PostSyncEventToTarget(plugin, "plugin", event);
    context->PostAsyncEventToTarget(plugin, "plugin", event);
    context->RegisterPipelineListener("plugin", plugin);

    context->GetSharedWorkLoop();
    context->InsertEventToPipeline("triggerName", event);
    context->IsReady();
    context->GetSysProperity("key", "");
    context->SetSysProperity("key", "val", false);
    context->GetWorkDir();
    context->GetConfigDir();
    context->GetCloudConfigDir();
    context->GetConfigDir("configName");
    plugin = nullptr;
    event = nullptr;
    looper = nullptr;
    delete context;

}

class PluginContextDemo : public PluginContext {
public:
    PluginContextDemo() {
        StartDispatchQueue();
    }
    ~PluginContextDemo() {
        unorderQueue_->OnUnload();
        unorderQueue_ = nullptr;
    }
    void RegisterUnorderedEventListener(sp<EventListener> listener) override {
        unorderQueue_->RegisterListener(listener);
    };
    void StartDispatchQueue()
    {
        unorderQueue_ = new EventDispatcher(Event::ManageType::UNORDERED);
        unorderQueue_->SetName("mi_unorder");
        unorderQueue_->SetPluginContext(this);
        sp<EventLoop> pluginLoop = new EventLoop("mi_unorder");
        pluginLoop->StartLoop();
        unorderQueue_->SetLooper(pluginLoop);
        unorderQueue_->OnLoad();
    }
    sp<EventDispatcher> unorderQueue_;

};
TEST_F(CommonTest, TestEventDispatcher) {

    PluginContextDemo* context = new PluginContextDemo();

    sp<PluginSample1> plugin = new PluginSample1();
    plugin->SetPluginContext(context);
    plugin->OnLoad();

    sp<Event> event = nullptr;
    context->unorderQueue_->OnEvent(event);
    event = new Event("send");
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    event->eventId_ =  EVENT_903;
    context->unorderQueue_->OnEvent(event);
    ASSERT_EQ(plugin->recvUnorderCnt_, 1);

    event->eventId_ =  EVENT_910;
    context->unorderQueue_->OnEvent(event);
    ASSERT_EQ(plugin->recvUnorderCnt_, 2);

    event->eventId_ =  EVENT_901;
    context->unorderQueue_->OnEvent(event);
    ASSERT_EQ(plugin->recvUnorderCnt_, 2);

    event->eventId_ =  EVENT_903;
    event->messageType_ = Event::MessageType::STATISTICS_EVENT;
    context->unorderQueue_->OnEvent(event);
    ASSERT_EQ(plugin->recvUnorderCnt_, 2);

    plugin->OnUnload();
    plugin = nullptr;
    delete context;
}
