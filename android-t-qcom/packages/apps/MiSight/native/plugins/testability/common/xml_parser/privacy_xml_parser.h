/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event privacy xml config file parser head file
 *
 */
#ifndef MISIGH_TESTABILITY_PRIVACY_XML_PASER_DEFINE_H
#define MISIGH_TESTABILITY_PRIVACY_XML_PASER_DEFINE_H


#include <list>
#include <mutex>
#include <string>


#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

#include "dev_info.h"
#include "event_util.h"
#include "xml_parser.h"

namespace android {
namespace MiSight {


class PrivacyXmlParser : public XmlParser {
protected:
    PrivacyXmlParser();
    ~PrivacyXmlParser();
public:
    bool IsMatch(const std::string& localName) override;
    void OnStartElement(const std::string& localName, std::map<std::string, std::string> attributes) override;
    void OnEndElement(const std::string& localName) override;
    bool GetPolicyByLocale(const std::string& workDir, const std::string& version,
        const std::string& locale, EventUtil::FaultLevel &faultLevel, EventUtil::PrivacyLevel &privacyLevel);
    static sp<PrivacyXmlParser> getInstance();

private:
    bool findRegion_;
    std::string version_;
    std::string expName_;
    std::string expValue_;
    std::string workDir_;
    EventUtil::FaultLevel faultLevel_;
    EventUtil::PrivacyLevel privacyLevel_;

    std::mutex parserMutex_;
};
}
}
#endif

