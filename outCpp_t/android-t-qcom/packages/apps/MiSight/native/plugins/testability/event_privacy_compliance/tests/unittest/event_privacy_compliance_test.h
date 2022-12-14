/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin testability event privacy compliance test head file
 *
 */

#ifndef MISIGHT_PLUGINS_TESTABILITY_EVENT_PRIVACY_COMPLIANCE_TEST
#define MISIGHT_PLUGINS_TESTABILITY_EVENT_PRIVACY_COMPLIANCE_TEST

#include <gtest/gtest.h>

#include <json/json.h>

class EventPrivacyComplianceTest : public testing::Test {
public:
    static void SetUpTestCase()
    {

    }
    static void TearDownTestCase();

protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown();

    std::string GenerateAllJson(int strLen, int arrLen);
    bool CompareJsonStr(const std::string srcStr, const std::string dstStr);
private:
    void AddStringMember(Json::Value &root, int len, int arrLen);
    void AddBoolMember(Json::Value &root, int arrLen);
    void AddShortMember(Json::Value &root, int arrLen);
    void AddIntMember(Json::Value &root, int arrLen);
    void AddFloatMember(Json::Value &root, int arrLen);
    void AddLongMember(Json::Value &root, int arrLen);
    void AddClassMember(Json::Value &root, int arrLen);

};

#endif
