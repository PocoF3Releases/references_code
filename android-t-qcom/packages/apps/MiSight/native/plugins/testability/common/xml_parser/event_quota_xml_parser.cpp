/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight quota xml config file parser implements file
 *
 */
#include "event_quota_xml_parser.h"

#include <algorithm>

#include "dev_info.h"
#include "event_util.h"
#include "log.h"
#include "string_util.h"

namespace android {
namespace MiSight {

const std::string QUOTA_XML = "event_quota.xml";
constexpr uint32_t DEF_EVENT_DAILY_MAX_CNT = 200;
constexpr uint32_t DEF_EVENT_LOG_CNT = 50;
constexpr uint32_t DEF_EVENT_LOG_SPACE = 300;

// xml local name
const std::string QUOTA_SUMMARY_NAME = "Summary";
const std::string QUOTA_DOMAIN_NAME = "Domain";
const std::string QUOTA_EVENT_NAME = "Event";

// summary attr
const std::string QUOTA_SUM_DEF_CNT = "defDailyCnt";
const std::string QUOTA_SUM_DEF_LOG_CNT = "defLogCnt";
const std::string QUOTA_SUM_DEF_LOG_SPACE = "defLogSpace";

const std::string QUOTA_SUM_UPLOAD_MAX_YEAR = "UploadExpireYear";
const std::string QUOTA_SUM_LEVEL1_MONTH = "Level1Month";
const std::string QUOTA_SUM_LEVEL1_RATIO = "Level1Max";
const std::string QUOTA_SUM_LEVEL2_MONTH = "Level2Month";
const std::string QUOTA_SUM_LEVEL2_RATIO = "Level2Max";
const std::string QUOTA_SUM_LEVEL3_MONTH = "Level3Month";
const std::string QUOTA_SUM_LEVEL3_RATIO = "Level3Max";
const std::string QUOTA_SUM_LOG_STATIC   = "LogStatic";
// domain attr
const std::string DOMAIN_ATTR_NAME = "name";
const std::string QUOTA_EVENT_MIN_ATTR_NAME = "eventMin";
const std::string QUOTA_EVENT_MAX_ATTR_NAME = "eventMax";
const std::string QUOTA_DAILY_CNT_ATTR_NAME = "dailyCnt";
const std::string QUOTA_MAX_LOG_ATTR_NAME = "maxLogCnt";
const std::string QUOTA_LOG_SPACE_NAME = "logSpace";

//event attr
const std::string QUOTA_EVENT_ID_ATTR_NAME = "eventId";

sp<EventQuotaXmlParser> EventQuotaXmlParser::getInstance()
{
    static sp<EventQuotaXmlParser> intance_ = new EventQuotaXmlParser;
    return intance_;
}


EventQuotaXmlParser::EventQuotaXmlParser()
    :XmlParser(QUOTA_XML), version_(""), workDir_("")
{
    memset(&totalQuota_, 0, sizeof(totalQuota_));
    domainQuotas_.clear();
    eventIdQuotas_.clear();
    commonQuotas_.clear();
}

EventQuotaXmlParser::~EventQuotaXmlParser()
{
    domainQuotas_.clear();
    eventIdQuotas_.clear();
}

void EventQuotaXmlParser::OnStartElement(const std::string& localName, std::map<std::string, std::string> attributes)
{
    SaveTotalQuota(localName, attributes);
    SaveDomainQuota(localName, attributes);
    SaveEventQuota(localName, attributes);
}

void EventQuotaXmlParser::SaveTotalQuota(const std::string& localName,
    std::map<std::string, std::string> attributes)
{
    if (localName != QUOTA_SUMMARY_NAME) {
        return;
    }
    totalQuota_.defDailyCnt = StringUtil::ConvertInt(attributes[QUOTA_SUM_DEF_CNT]);
    totalQuota_.defLogCnt = StringUtil::ConvertInt(attributes[QUOTA_SUM_DEF_LOG_CNT]);
    totalQuota_.defLogSpace = StringUtil::ConvertInt(attributes[QUOTA_SUM_DEF_LOG_SPACE]);
    totalQuota_.uploadExpireYear = StringUtil::ConvertInt(attributes[QUOTA_SUM_UPLOAD_MAX_YEAR]);
    totalQuota_.level1Month = StringUtil::ConvertInt(attributes[QUOTA_SUM_LEVEL1_MONTH]);
    totalQuota_.level1Ratio    = StringUtil::ConvertInt(attributes[QUOTA_SUM_LEVEL1_RATIO]);
    totalQuota_.level2Month    = StringUtil::ConvertInt(attributes[QUOTA_SUM_LEVEL2_MONTH]);
    totalQuota_.level2Ratio    = StringUtil::ConvertInt(attributes[QUOTA_SUM_LEVEL2_RATIO]);
    totalQuota_.level3Month    = StringUtil::ConvertInt(attributes[QUOTA_SUM_LEVEL3_MONTH]);
    totalQuota_.level3Ratio    = StringUtil::ConvertInt(attributes[QUOTA_SUM_LEVEL3_RATIO]);
    totalQuota_.logStatic      = StringUtil::ConvertInt(attributes[QUOTA_SUM_LOG_STATIC]);

}

void EventQuotaXmlParser::SaveDomainQuota(const std::string& localName, std::map<std::string, std::string> attributes)
{
    if (localName != QUOTA_DOMAIN_NAME) {
        return;
    }
    auto it = attributes.find(DOMAIN_ATTR_NAME);
    if (it == attributes.end()) {
         return;
    }

    struct DomainQuota domain;
    domain.dailyCnt = StringUtil::ConvertInt(attributes[QUOTA_DAILY_CNT_ATTR_NAME]);
    domain.eventMax = StringUtil::ConvertInt(attributes[QUOTA_EVENT_MAX_ATTR_NAME]);
    domain.eventMin = StringUtil::ConvertInt(attributes[QUOTA_EVENT_MIN_ATTR_NAME]);
    domain.maxLogCnt = StringUtil::ConvertInt(attributes[QUOTA_MAX_LOG_ATTR_NAME]);
    domain.logSpace = StringUtil::ConvertInt(attributes[QUOTA_LOG_SPACE_NAME]);
    domain.realLogCnt = 0;
    domainQuotas_[it->second] = std::move(domain);
}

void EventQuotaXmlParser::SaveEventQuota(const std::string& localName, std::map<std::string, std::string> attributes)
{
    if (localName != QUOTA_EVENT_NAME) {
        return;
    }

    auto it = attributes.find(QUOTA_EVENT_ID_ATTR_NAME);
    if (it == attributes.end()) {
         return;
    }

    auto itVal = attributes.find(QUOTA_DAILY_CNT_ATTR_NAME);
    if (itVal == attributes.end()) {
         return;
    }
    eventIdQuotas_[StringUtil::ConvertInt(it->second)] = StringUtil::ConvertInt(itVal->second);
}

void EventQuotaXmlParser::OnEndElement(const std::string& localName)
{
    if (localName != version_) {
        return;
    }

    SetFinish();
}

bool EventQuotaXmlParser::IsMatch(const std::string& localName)
{
    if (localName == version_)
    {
        SetStart();
        return true;
    }
    return false;
}

int EventQuotaXmlParser::StartParserXml(const std::string& workDir, const std::string version)
{
    std::lock_guard<std::mutex> lock(parserMutex_);
    if ((workDir_ != "") && (workDir_ == workDir)) {
        return 0;
    }

    version_ = "Stable";
    if (DevInfo::IsInnerVersion(version)) {
        version_ = "Dev";
    }

    memset(&totalQuota_, 0, sizeof(SummaryQuota));
    domainQuotas_.clear();
    eventIdQuotas_.clear();

    if (ParserXmlFile(workDir + QUOTA_XML) != 0) {
        MISIGHT_LOGW("parse xml file failed %s", workDir_.c_str());
        return -1;
    }
    workDir_ = workDir;
    return 0;
}

EventQuotaXmlParser::DomainQuota EventQuotaXmlParser::GetEventQuota(const std::string& workDir, uint32_t eventId,
    const std::string& version)
{
    DomainQuota eventQuota;
    memset(&eventQuota, -1, sizeof(DomainQuota));

    if (StartParserXml(workDir, version) == 0) {
        // preferred event quota config
        auto findEvent = eventIdQuotas_.find(eventId);
        if (findEvent != eventIdQuotas_.end()) {
            eventQuota.dailyCnt = findEvent->second;
        }
        // then check field config
        for (auto& domainMap : domainQuotas_) {
            auto domain = domainMap.second;
            if (domain.eventMin <= eventId &&  domain.eventMax >= eventId) {
                MISIGHT_LOGD("parser retrun domain %d in [%d,%d]", eventId, domain.eventMin, domain.eventMax);
                if (eventQuota.dailyCnt == -1) {
                    eventQuota.dailyCnt = domain.dailyCnt;
                }
                eventQuota.maxLogCnt = domain.maxLogCnt;
                eventQuota.logSpace = domain.logSpace;
                eventQuota.eventMin = domain.eventMin;
                eventQuota.eventMax = domain.eventMax;
                break;
            }
        }
    }

    if (eventQuota.dailyCnt <= 0) {
        eventQuota.dailyCnt = ((totalQuota_.defDailyCnt > 0) ? totalQuota_.defDailyCnt : DEF_EVENT_DAILY_MAX_CNT);
    }
    if (eventQuota.maxLogCnt <= 0) {
        eventQuota.maxLogCnt = (totalQuota_.defLogCnt > 0) ? totalQuota_.defLogCnt : DEF_EVENT_LOG_CNT;
    }
    if (eventQuota.logSpace <= 0) {
        eventQuota.logSpace = (totalQuota_.defLogSpace > 0) ? totalQuota_.defLogSpace : DEF_EVENT_LOG_SPACE;
    }
    return eventQuota;
}

EventQuotaXmlParser::SummaryQuota EventQuotaXmlParser::GetSummaryQuota(const std::string& workDir,
    const std::string& version)
{
    MISIGHT_LOGD("EventQuotaXmlParser= %s", workDir.c_str());
    if (StartParserXml(workDir, version) != 0) {
        MISIGHT_LOGD("EventQuotaXmlParser parser retrun default");
    }
    return totalQuota_;
}

bool EventQuotaXmlParser::CheckLogCntLimitAfterIncr(const std::string& workDir, const std::string& version,
    uint32_t eventId, int logCnt)
{
    if (StartParserXml(workDir, version) != 0) {
        MISIGHT_LOGD("EventQuotaXmlParser parser retrun default");
    }
    std::lock_guard<std::mutex> lock(parserMutex_);
    // then check field config
    for (auto& domainMap : domainQuotas_) {
        DomainQuota& domain = domainMap.second;
        if (domain.eventMin <= eventId && domain.eventMax >= eventId) {
            domain.realLogCnt += logCnt;
            MISIGHT_LOGD("parser retrun domain, rel=%d, max=%d, add %d", domain.realLogCnt, domain.maxLogCnt, logCnt);
            if (domain.realLogCnt > domain.maxLogCnt) {
                return true;
            }
            return false;
        }
    }

    int domainInt = (eventId/EventUtil::FAULT_EVENT_SIX_RANGE);
    auto it = commonQuotas_.find(domainInt);
    if (it == commonQuotas_.end()) {
        commonQuotas_[domainInt] = logCnt;
    } else {
        commonQuotas_[domainInt] += logCnt;
    }

    int defCnt = ((totalQuota_.defDailyCnt > 0) ? totalQuota_.defDailyCnt : DEF_EVENT_DAILY_MAX_CNT);
    MISIGHT_LOGD("parser retrun , rel=%d, max=%d", commonQuotas_[domainInt], defCnt);

    return (commonQuotas_[domainInt] > defCnt);
}
}// namespace MiSight
}// namespace android
