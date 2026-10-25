/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin testability event privacy compliance test implement file
 *
 */
#include "event_privacy_compliance_test.h"

#include <string>

#include "common.h"
#include "event.h"
#include "event_util.h"
#include "event_privacy_compliance.h"
#include "json_util.h"
#include "platform_common.h"
#include "plugin_platform.h"
#include "public_util.h"


using namespace android;
using namespace android::MiSight;

void EventPrivacyComplianceTest::TearDown()
{
    auto &platform = PluginPlatform::getInstance();
    DevInfo::SetRunMode(&platform, "", "");
}
void EventPrivacyComplianceTest::TearDownTestCase()
{
    auto &platform = PluginPlatform::getInstance();
    DevInfo::SetRunMode(&platform, "", "");
}


void EventPrivacyComplianceTest::AddStringMember(Json::Value &root, int len, int arrLen)
{
    std::string testStr = std::string(len, 'x');
    root["ParaStr"] = testStr;
    if (arrLen != 0) {
        Json::Value jsonStrArray;
        for (int i = 0; i < arrLen; i++) {
            jsonStrArray.append(testStr);
        }
        root["ParaStrArray"] = jsonStrArray;
    }
}

void EventPrivacyComplianceTest::AddBoolMember(Json::Value &root, int arrLen)
{
    root["ParaBool"] = true;
    if (arrLen != 0) {
        Json::Value jsonStrArray;
        for (int i = 0; i < arrLen; i++) {
            jsonStrArray.append((bool)(i/2));
        }
        root["ParaBoolArray"] = jsonStrArray;
    }
}

void EventPrivacyComplianceTest::AddShortMember(Json::Value &root, int arrLen)
{
    short val = -1;
    root["ParaShort"] = val;
    if (arrLen != 0) {
        Json::Value jsonStrArray;
        for (int i = 0; i < arrLen; i++) {
            jsonStrArray.append(val*i);
        }
        root["ParaShortArray"] = jsonStrArray;
    }
}

void EventPrivacyComplianceTest::AddIntMember(Json::Value &root, int arrLen)
{
    root["ParaInt"] = -1;
    if (arrLen != 0) {
        Json::Value jsonStrArray;
        for (int i = 0; i < arrLen; i++) {
            jsonStrArray.append(-i);
        }
        root["ParaIntArray"] = jsonStrArray;
    }
}

void EventPrivacyComplianceTest::AddFloatMember(Json::Value &root, int arrLen)
{
    root["ParaFloat"] = 1.0;
    if (arrLen != 0) {
        Json::Value jsonStrArray;
        for (int i = 0; i < arrLen; i++) {
            jsonStrArray.append(2.0);
        }
        root["ParaFloatArray"] = jsonStrArray;
    }
}

void EventPrivacyComplianceTest::AddLongMember(Json::Value &root, int arrLen)
{
    Json::Int64 val = 5000000;
    root["ParaLong"] = val;
    if (arrLen != 0) {
        Json::Value jsonStrArray;
    for (int i = 0; i < arrLen; i++) {
        jsonStrArray.append(val * i);
        }
        root["ParaLongArray"] = jsonStrArray;
    }
}

void EventPrivacyComplianceTest::AddClassMember(Json::Value &root, int arrLen)
{
    Json::Value paraClass;
    AddStringMember(paraClass, 20, 0);
    root["ParaClass"] = paraClass;
    if (arrLen != 0) {
        Json::Value jsonStrArray;
        for (int i = 0; i < arrLen; i++) {
            jsonStrArray.append(paraClass);
        }
        root["ParaClassArray"] = jsonStrArray;
    }
}

std::string EventPrivacyComplianceTest::GenerateAllJson(int strLen, int arrLen)
{
    Json::Value root;
    AddStringMember(root, strLen, arrLen);
    AddBoolMember(root, arrLen);
    AddShortMember(root, arrLen);
    AddIntMember(root, arrLen);
    AddFloatMember(root, arrLen);
    AddLongMember(root, arrLen);
    AddClassMember(root, arrLen);
    return JsonUtil::ConvertJsonToStr(root);
}

bool EventPrivacyComplianceTest::CompareJsonStr(const std::string srcStr, const std::string dstStr)
{
    Json::Value srcJson;
    Json::Value dstJson;
    if (!JsonUtil::ConvertStrToJson(srcStr, srcJson) || !JsonUtil::ConvertStrToJson(dstStr, dstJson)) {
        printf("convert json failed srcStr=%s, dstStr=%s\n", srcStr.c_str(), dstStr.c_str());
        return false;
    }

    Json::Value::Members names = srcJson.getMemberNames();
    Json::Value::Members dstNames = dstJson.getMemberNames();
    if (dstNames.size() != names.size()) {
        printf("dst name %zu not equal src name %zu,src=%s,dst=%s\n", dstNames.size(), names.size(),srcStr.c_str(), dstStr.c_str());
        return false;
    }

    for (auto name : names) {
        if (!dstJson.isMember(name)) {
            printf("dst have no %s member, src=%s,dst=%s\\n", name.c_str(), srcStr.c_str(), dstStr.c_str());
            return false;
        }
        if (srcJson[name] != dstJson[name]) {
            printf("dst and src not equal %s, src=%s,dst=%s\\n", name.c_str(), srcStr.c_str(), dstStr.c_str());
            return false;
        }
    }
    return true;
}

TEST_F(EventPrivacyComplianceTest, TestNormalEvent) {
    // all param valid
    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(false));

    auto &platform = PluginPlatform::getInstance();
    DevInfo::SetRunMode(&platform, "", "");
    sp<EventPrivacyCompliance> eventProcess = new EventPrivacyCompliance();
    eventProcess->SetPluginContext(&platform);
    eventProcess->OnLoad();

    // 2.create event and onEvent
    sp<Event> event = new Event("testEvent");
    event->eventId_ = EVENT_901;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    event->happenTime_ = std::time(nullptr);
    std::string eventStr = GenerateAllJson(20,3);
    printf("TestNormalEvent: eventStr=%s\n", eventStr.c_str());
    event->SetValue(EventUtil::DEV_SAVE_EVENT, eventStr);
    ASSERT_EQ(ON_SUCCESS, eventProcess->OnEvent(event));
    ASSERT_EQ(true, CompareJsonStr(eventStr, event->GetValue(EventUtil::DEV_SAVE_EVENT)));
}

// fault level or privacy level not valid
TEST_F(EventPrivacyComplianceTest, TestEventLevelInValid) {
    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(false));

    auto &platform = PluginPlatform::getInstance();
    DevInfo::SetRunMode(&platform, "", "");
    sp<EventPrivacyCompliance> eventProcess = new EventPrivacyCompliance();
    eventProcess->SetPluginContext(&platform);
    eventProcess->OnLoad();

    // 2.create event and onEvent
    sp<Event> event = new Event("testEvent");
    event->eventId_ = EVENT_902; // in event.xml fault level invalid
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    event->happenTime_ = std::time(nullptr);
    std::string eventStr = "{\"ParaStr\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}";
    event->SetValue(EventUtil::DEV_SAVE_EVENT, eventStr);

    // fault level invalid
    ASSERT_EQ(ON_FAILURE, eventProcess->OnEvent(event));

    // privacy level invalid
    event->eventId_ = EVENT_903; // in event.xml privacy level invalid
    ASSERT_EQ(ON_FAILURE, eventProcess->OnEvent(event));
}

// param not valid
TEST_F(EventPrivacyComplianceTest, TestParaInvalid) {

    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(false));

    auto &platform = PluginPlatform::getInstance();
    DevInfo::SetRunMode(&platform, "", "");
    sp<EventPrivacyCompliance> eventProcess = new EventPrivacyCompliance();
    eventProcess->SetPluginContext(&platform);
    eventProcess->OnLoad();


    // 2.create event and param partly value invalid
    sp<Event> event = new Event("testEvent");
    event->eventId_ = EVENT_901;
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    event->happenTime_ = std::time(nullptr);

    std::string eventStr = GenerateAllJson(60, 5);
    std::string expStr = GenerateAllJson(30, 3);


    printf("TestNormalEvent: eventStr=%s, expStr=%s\n", eventStr.c_str(), expStr.c_str());
    event->SetValue(EventUtil::DEV_SAVE_EVENT, eventStr);
    ASSERT_EQ(ON_SUCCESS, eventProcess->OnEvent(event));
    ASSERT_EQ(expStr, event->GetValue(EventUtil::DEV_SAVE_EVENT));

    // 3. create event and param privacy partly not compliance
    event->eventId_ = EVENT_904;
    event->SetValue(EventUtil::DEV_SAVE_EVENT, eventStr);
    ASSERT_EQ(ON_SUCCESS, eventProcess->OnEvent(event));
    ASSERT_EQ(true, event->GetValue(EventUtil::DEV_SAVE_EVENT).find("ParaBool") == std::string::npos);

    //4. param used default value
    event->eventId_ = EVENT_901;
    event->SetValue(EventUtil::DEV_SAVE_EVENT, "{\"ParaBool\":true}");
    ASSERT_EQ(ON_SUCCESS, eventProcess->OnEvent(event));
    printf("test 4=== %s\n",event->GetValue(EventUtil::DEV_SAVE_EVENT).c_str());
    ASSERT_EQ(true, event->GetValue(EventUtil::DEV_SAVE_EVENT).find("ParaStr") != std::string::npos);

    //5. param used unconfiged name
    event->eventId_ = EVENT_901;
    event->SetValue(EventUtil::DEV_SAVE_EVENT, "{\"ParaBool\":true, \"unconfig\":\"aaa\"}");
    ASSERT_EQ(ON_SUCCESS, eventProcess->OnEvent(event));
    printf("test 5=== %s\n",event->GetValue(EventUtil::DEV_SAVE_EVENT).c_str());
    ASSERT_EQ(true, event->GetValue(EventUtil::DEV_SAVE_EVENT).find("unconfig") == std::string::npos);

    //6. param config type not match
    event->eventId_ = EVENT_901;
    event->SetValue(EventUtil::DEV_SAVE_EVENT, "{\"ParaBool\":true, \"ParaInt\":\"sss\", \"ParaStr\":232}");
    ASSERT_EQ(ON_SUCCESS, eventProcess->OnEvent(event));
    printf("test 6=== %s\n",event->GetValue(EventUtil::DEV_SAVE_EVENT).c_str());
    ASSERT_EQ(true, event->GetValue(EventUtil::DEV_SAVE_EVENT).find("ParaInt") == std::string::npos);
}


TEST_F(EventPrivacyComplianceTest, TestFilterEvent) {

    // 1. star plugin platform
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(false));

    auto &platform = PluginPlatform::getInstance();
    sp<EventPrivacyCompliance> eventProcess = new EventPrivacyCompliance();
    eventProcess->SetPluginContext(&platform);
    eventProcess->OnLoad();

    DevInfo::SetRunMode(&platform, "UAT", platform.GetConfigDir() + "filter_test.xml");

    // 2.create event and param partly invalid
    sp<Event> event = new Event("testEvent");
    // in whitelist
    event->eventId_ = EVENT_901;
    std::string str = "{\"paraInt\":30}";

    // "{\"PackageName\":\"com.xiaomi.zzz\", \"test\":10}";
    event->SetValue(EventUtil::DEV_SAVE_EVENT, str);
    ASSERT_EQ(ON_SUCCESS, eventProcess->OnEvent(event));

    // in white list and param in whitelist
    event->eventId_ = EVENT_904;
    str = "{\"ParaStr\":\"com.xiaomi.xx1\"}";
    event->SetValue(EventUtil::DEV_SAVE_EVENT, str);
    ASSERT_EQ(ON_SUCCESS, eventProcess->OnEvent(event));

    // in white list but pare not in whitelist
    event->eventId_ = EVENT_904;
    str = "{\"ParaStr\":\"com.xiaomi.xx6\"}";
    event->SetValue(EventUtil::DEV_SAVE_EVENT, str);
    ASSERT_EQ(ON_FAILURE, eventProcess->OnEvent(event));
    // in black list
    event->eventId_ = EVENT_905;
    str = "{\"ParaStr\":\"com.xiaomi.xx6\"}";
    event->SetValue(EventUtil::DEV_SAVE_EVENT, str);
    ASSERT_EQ(ON_FAILURE, eventProcess->OnEvent(event));
    // in black list and para in blacklist
    event->eventId_ = EVENT_906;
    str = "{\"ParaStr\":\"com.xiaomi.xx3\"}";
    event->SetValue(EventUtil::DEV_SAVE_EVENT, str);
    ASSERT_EQ(ON_FAILURE, eventProcess->OnEvent(event));
    // in black list but para not in blacklist
    event->eventId_ = EVENT_906;
    str = "{\"ParaStr\":\"com.xiaomi.xx5\"}";
    event->SetValue(EventUtil::DEV_SAVE_EVENT, str);
    ASSERT_EQ(ON_SUCCESS, eventProcess->OnEvent(event));

    // not in whitelist and not in blacklist
    event->eventId_ = EVENT_907;
    str = "{\"ParaStr\":\"com.xiaomi.xx5\"}";
    event->SetValue(EventUtil::DEV_SAVE_EVENT, str);
    ASSERT_EQ(ON_SUCCESS, eventProcess->OnEvent(event));
}
