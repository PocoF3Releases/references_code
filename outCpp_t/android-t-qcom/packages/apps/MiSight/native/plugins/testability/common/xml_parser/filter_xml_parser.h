/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event filter xml config file parser head file
 *
 */
#ifndef MISIGH_TESTABILITY_FILTER_XML_PASER_DEFINE_H
#define MISIGH_TESTABILITY_FILTER_XML_PASER_DEFINE_H

#include <map>
#include <string>

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>


#include "xml_parser.h"

namespace android {
namespace MiSight {
class EventFilterInfo : public RefBase {
public:
    explicit EventFilterInfo(int32_t eventId);
    ~EventFilterInfo();
    void AddWhiteList(const std::string& name, const std::string& value);
    void AddBlackList(const std::string& name, const std::string& value);

    int32_t eventId_;
    bool hasWhite_;
    bool hasBlack_;
    std::map<std::string, std::string> whiteList_;
    std::map<std::string, std::string> blackList_;
};

class FilterXmlParser : public XmlParser {
public:
    FilterXmlParser();
    ~FilterXmlParser();

    bool IsMatch(const std::string& localName) override;
    void OnStartElement(const std::string& localName, std::map<std::string, std::string> attributes) override;
    void OnEndElement(const std::string& localName) override;
    sp<EventFilterInfo> GetFilterConfig(const std::string& runMode, const std::string& filePath, int32_t eventId);
private:
    bool eventMatch_;
    std::string runMode_;
    std::string filePath_;
    std::string filterList_;
    sp<EventFilterInfo> eventFilter_;
};
}
}
#endif

