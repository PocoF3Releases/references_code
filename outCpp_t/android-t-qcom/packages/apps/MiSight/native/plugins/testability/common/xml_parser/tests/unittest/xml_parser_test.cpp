/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample1 implement file
 *
 */
#include "xml_parser_test.h"

#include <list>

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

#include "dev_info.h"
#include "event.h"
#include "event_quota_xml_parser.h"
#include "event_upload_xml_parser.h"
#include "event_util.h"
#include "event_xml_parser.h"
#include "filter_xml_parser.h"
#include "privacy_xml_parser.h"
#include "string_util.h"


using namespace android;
using namespace android::MiSight;

const std::string CFG_DIR = "/data/test/config/";

// validate QuotaXmlParser
TEST_F(XmlParserTest, TestQuotaXmlParser) {
    sp<EventQuotaXmlParser> quotaXml = EventQuotaXmlParser::getInstance();
    // 1. check Event config valid
    std::string version = "Dev";

    EventQuotaXmlParser::DomainQuota eventQuota = quotaXml->GetEventQuota(CFG_DIR, 901001001, version);
    ASSERT_EQ(50, eventQuota.dailyCnt);
    ASSERT_EQ(2, eventQuota.maxLogCnt);
    ASSERT_EQ(60, eventQuota.logSpace);

    //2. check Domain config valid
    eventQuota = quotaXml->GetEventQuota(CFG_DIR, 903000000, version);
    ASSERT_EQ(1000, eventQuota.dailyCnt);
    ASSERT_EQ(100, eventQuota.maxLogCnt);
    ASSERT_EQ(300, eventQuota.logSpace);
    //3. check Summary config valid
    eventQuota =  quotaXml->GetEventQuota(CFG_DIR, 904000000, version);
    ASSERT_EQ(120, eventQuota.dailyCnt);
    ASSERT_EQ(200, eventQuota.maxLogCnt);
    ASSERT_EQ(300, eventQuota.logSpace);

    EventQuotaXmlParser::SummaryQuota total = quotaXml->GetSummaryQuota(CFG_DIR, version);
    ASSERT_EQ(8, total.level1Ratio);
    ASSERT_EQ(5, total.level2Ratio);
    ASSERT_EQ(3, total.level3Ratio);
    ASSERT_EQ(3, total.level1Month);
    ASSERT_EQ(12, total.level2Month);
    ASSERT_EQ(24, total.level3Month);
    ASSERT_EQ(5, total.uploadExpireYear);

}

// validate EventXmlParser
TEST_F(XmlParserTest, TestEventXmlParser) {
    sp<EventXmlParser> eventXml = EventXmlParser::getInstance();
    uint32_t eventId = 901001001;
    sp<EventInfoConfig> eventInfo = eventXml->GetEventInfoConfig(CFG_DIR, eventId);
    ASSERT_NE(nullptr, eventInfo);

    // check event.xml
    ASSERT_EQ(eventId, eventInfo->eventId_);
    ASSERT_EQ(EventUtil::FaultLevel::CRITICAL, eventInfo->faultLevel_);
    ASSERT_EQ(EventUtil::PrivacyLevel::L2, eventInfo->privacyLevel_);
    ASSERT_EQ(Event::MessageType::FAULT_EVENT, eventInfo->faultType_);

    // check event param
    ASSERT_EQ(14, eventInfo->params_.size());
    ASSERT_EQ(0, eventInfo->params_.front().id);
    ASSERT_EQ("ParaStr", eventInfo->params_.front().name);
    ASSERT_EQ(EventInfoConfig::ParamType::STRING, eventInfo->params_.front().type);
    ASSERT_EQ(30, eventInfo->params_.front().len);
    ASSERT_EQ(0, eventInfo->params_.front().arrLen);
    ASSERT_EQ(0, eventInfo->params_.front().classID);
    ASSERT_EQ(EventUtil::PrivacyLevel::L2, eventInfo->params_.front().privacyLevel);
    ASSERT_EQ("it is default", eventInfo->params_.front().defVal);

    ASSERT_EQ(13, eventInfo->params_.back().id);
    ASSERT_EQ("ParaClassArray", eventInfo->params_.back().name);
    ASSERT_EQ(EventInfoConfig::ParamType::CLASS, eventInfo->params_.back().type);
    ASSERT_EQ(0, eventInfo->params_.back().len);
    ASSERT_EQ(3, eventInfo->params_.back().arrLen);
    ASSERT_EQ(906001001, eventInfo->params_.back().classID);
    ASSERT_EQ(EventUtil::PrivacyLevel::L2, eventInfo->params_.back().privacyLevel);
    ASSERT_EQ("", eventInfo->params_.back().defVal);

    // event not exsit
    eventInfo = eventXml->GetEventInfoConfig(CFG_DIR, 906001999);
    ASSERT_EQ(nullptr, eventInfo);
}


TEST_F(XmlParserTest, TestPrivacyXmlParser) {
    sp<PrivacyXmlParser> privacy = PrivacyXmlParser::getInstance();

    EventUtil::FaultLevel faultLevel = EventUtil::FaultLevel::DEBUG;
    EventUtil::PrivacyLevel privacyLevel = EventUtil::PrivacyLevel::MAX_LEVEL;
    //V12.5.3.0.RFXCNXM
    ASSERT_EQ(true, privacy->GetPolicyByLocale(CFG_DIR, "V12.5.3.0.RFXCNXM", "zh-CN", faultLevel, privacyLevel));
    ASSERT_EQ(EventUtil::FaultLevel::GENERAL, faultLevel);
    ASSERT_EQ(EventUtil::PrivacyLevel::L3, privacyLevel);

    ASSERT_EQ(false, privacy->GetPolicyByLocale(CFG_DIR, "V12.5.3.0.RFXCNXM", "zN", faultLevel, privacyLevel));
    // dev
    ASSERT_EQ(true, privacy->GetPolicyByLocale(CFG_DIR, "12.5.3.0.RFXCNXM", "zh-CN", faultLevel, privacyLevel));
    ASSERT_EQ(EventUtil::FaultLevel::INFORMATIVE, faultLevel);
    ASSERT_EQ(EventUtil::PrivacyLevel::L2, privacyLevel);
}


TEST_F(XmlParserTest, TestUploadXmlParser) {
    sp<EventUploadXmlParser> upload = EventUploadXmlParser::getInstance();

    uint32_t eventId = 901001002;
    ASSERT_EQ(0, upload->GetEventLogCollectInfo(CFG_DIR, eventId).size());

    eventId = 901001001;
    std::list<sp<LogCollectInfo>> logCollect = upload->GetEventLogCollectInfo(CFG_DIR, eventId);
    ASSERT_EQ(7, logCollect.size());
}

TEST_F(XmlParserTest, TestFilterXmlParser) {
    sp<FilterXmlParser> xmlFilter = new FilterXmlParser();
    std::string filterPath = CFG_DIR + "filter_test.xml";
    std::string testName = "MTBF";
    sp<EventFilterInfo> eventFilter = xmlFilter->GetFilterConfig(testName, filterPath, 901001001);
    ASSERT_EQ(eventFilter, nullptr);

    eventFilter = xmlFilter->GetFilterConfig(testName, filterPath, 905001003);
    ASSERT_NE(eventFilter, nullptr);
    ASSERT_EQ(eventFilter->hasWhite_, true);
    ASSERT_EQ(eventFilter->whiteList_.size(), 0);
    ASSERT_EQ(eventFilter->hasBlack_, false);


    eventFilter = xmlFilter->GetFilterConfig(testName, filterPath, 905001005);
    ASSERT_NE(eventFilter, nullptr);
    ASSERT_EQ(eventFilter->hasWhite_, true);
    ASSERT_EQ(eventFilter->whiteList_.size(), 1);
    std::list<std::string> valList = StringUtil::SplitStr(eventFilter->whiteList_["ParaStr"], ',');
    ASSERT_EQ(valList.size(), 4);
    ASSERT_EQ(eventFilter->hasBlack_, false);

    eventFilter = xmlFilter->GetFilterConfig(testName, filterPath, 905001006);
    ASSERT_NE(eventFilter, nullptr);
    ASSERT_EQ(eventFilter->hasWhite_, false);
    ASSERT_EQ(eventFilter->hasBlack_, true);
    ASSERT_EQ(eventFilter->blackList_.size(), 0);

    eventFilter = xmlFilter->GetFilterConfig(testName, filterPath, 905001007);
    ASSERT_NE(eventFilter, nullptr);
    ASSERT_EQ(eventFilter->hasWhite_, false);
    ASSERT_EQ(eventFilter->hasBlack_, true);
    ASSERT_EQ(eventFilter->blackList_.size(), 1);
    valList = StringUtil::SplitStr(eventFilter->blackList_["ParaStr"], ',');
    ASSERT_EQ(valList.size(), 2);
}
