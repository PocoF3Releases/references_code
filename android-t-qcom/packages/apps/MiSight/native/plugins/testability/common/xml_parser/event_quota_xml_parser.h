/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight quota xml config file parser head file
 *
 */
#ifndef MISIGH_TESTABILITY_QUOTA_XML_PASER_DEFINE_H
#define MISIGH_TESTABILITY_QUOTA_XML_PASER_DEFINE_H


#include <map>
#include <mutex>
#include <string>

//#include <utils/Singleton.h>
#include <utils/StrongPointer.h>

#include "xml_parser.h"

namespace android {
namespace MiSight {

class EventQuotaXmlParser : public XmlParser {
private:
    EventQuotaXmlParser();
    ~EventQuotaXmlParser();
public:
    struct SummaryQuota {
        int32_t defDailyCnt;
        int32_t defLogCnt;
        int32_t defLogSpace;
        int32_t uploadExpireYear;
        int32_t level1Month;
        int32_t level1Ratio;
        int32_t level2Month;
        int32_t level2Ratio;
        int32_t level3Month;
        int32_t level3Ratio;
        int32_t logStatic;
    };

    struct DomainQuota {
        int32_t eventMin;
        int32_t eventMax;
        int32_t dailyCnt;
        int32_t maxLogCnt;
        int32_t logSpace;
        int32_t realLogCnt;
    };

    bool IsMatch(const std::string& localName) override;
    void OnStartElement(const std::string& localName, std::map<std::string, std::string> attributes) override;
    void OnEndElement(const std::string& localName) override;
    DomainQuota GetEventQuota(const std::string& workDir, uint32_t eventId, const std::string& version);
    bool CheckLogCntLimitAfterIncr(const std::string& workDir, const std::string& version,
        uint32_t eventId, int logCnt);
    SummaryQuota GetSummaryQuota(const std::string& workDir, const std::string& version);
    static sp<EventQuotaXmlParser> getInstance();

private:
    int StartParserXml(const std::string& workDir, const std::string version);
    void SaveTotalQuota(const std::string& localName, std::map<std::string, std::string> attributes);
    void SaveDomainQuota(const std::string& localName, std::map<std::string, std::string> attributes);
    void SaveEventQuota(const std::string& localName, std::map<std::string, std::string> attributes);

    std::string version_;
    std::string workDir_;
    /*total have attr:
        wifiSize; dataSize; journalLogWifi; journalLogData; featureLogWifi; featureLogData; maxPackSize; defDailyCnt;*/
    SummaryQuota totalQuota_;

    std::map<std::string, DomainQuota> domainQuotas_;
    std::map<int, int> eventIdQuotas_;
    std::map<int, int> commonQuotas_; // <901, logcnt>
    std::mutex parserMutex_;

};
}
}
#endif
