/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event xml config file parser head file
 *
 */
#ifndef MISIGH_TESTABILITY_EVENT_XML_PASER_DEFINE_H
#define MISIGH_TESTABILITY_EVENT_XML_PASER_DEFINE_H

#include <list>
#include <map>
#include <string>


#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

#include "event.h"
#include "event_util.h"
#include "xml_parser.h"

namespace android {
namespace MiSight {



class EventInfoConfig : public RefBase {
public:
    enum ParamType {
        NONE,
        STRING,
        BOOL,
        SHORT,
        INT,
        FLOAT,
        LONG,
        CLASS
    };

    struct ParamInfoConfig {
        uint32_t id;
        uint32_t len;
        uint32_t arrLen;
        uint32_t classID;
        ParamType type;
        EventUtil::PrivacyLevel privacyLevel;
        std::string name;
        std::string defVal;

        ParamInfoConfig()
            : id(0), len(0), arrLen(0), classID(0),
              type(NONE),  privacyLevel(EventUtil::PrivacyLevel::MAX_LEVEL),
          name(""), defVal("")
        {}
    };

    uint32_t eventId_;
    Event::MessageType faultType_;
    EventUtil::FaultLevel faultLevel_;
    EventUtil::PrivacyLevel privacyLevel_;
    std::list<ParamInfoConfig> params_;
};


class EventXmlParser : public XmlParser {
protected:
    EventXmlParser();
    ~EventXmlParser();
public:
    bool IsMatch(const std::string& localName) override;
    void OnStartElement(const std::string& localName, std::map<std::string, std::string> attributes) override;
    void OnEndElement(const std::string& localName) override;
    sp<EventInfoConfig> GetEventInfoConfig(const std::string& workDir, uint32_t eventId);
    static sp<EventXmlParser> getInstance();
private:
    void SetFaultType(const std::string& faultType);
    void InitEventInfoConfig(std::map<std::string, std::string> attributes);
    EventInfoConfig::ParamType GetParamType(const std::string& type);
    void AddEventParamInfoCfg(std::map<std::string, std::string> attributes);

    sp<EventInfoConfig> curEventInfoCfg_;
    bool findEvent_;
};
}
}
#endif

