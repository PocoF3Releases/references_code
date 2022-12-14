/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event xml config file parser implements file
 *
 */

#include "event_xml_parser.h"

#include "log.h"
#include "string_util.h"


namespace android {
namespace MiSight {
const std::string EVENT_XML = "event.xml";

// Event xml config
// <Event id="901001000" FaultType="FAULT" FaultLevel="CRITICAL" PrivacyLevel="L2">
const std::string EVENT_LOCAL_NAME = "Event";
const std::string EVENT_ATTR_ID = "id";
const std::string EVENT_ATTR_FAULT_TYPE = "FaultType";
const std::string EVENT_ATTR_FALULT_LEVEL = "FaultLevel";
const std::string EVENT_ATTR_PRIVACY_LEVEL = "PrivacyLevel";

// param xml config
// <Param id="0" name="" type="" len="" arrLen="" defVal="" classID="" PrivacyLevel=""/>
const std::string EVENT_PARAM_LOCAL_NAME = "Param";
const std::string EVENT_PARAM_ATTR_ID = "id";
const std::string EVENT_PARAM_ATTR_NAME = "name";
const std::string EVENT_PARAM_ATTR_TYPE = "type";
const std::string EVENT_PARAM_ATTR_LEN = "len";
const std::string EVENT_PARAM_ATTR_ARRLEN = "arrLen";
const std::string EVENT_PARAM_ATTR_DEFVAL = "defVal";
const std::string EVENT_PARAM_ATTR_CLASSID = "classID";
const std::string EVENT_PARAM_ATTR_PRIVACY_LEVEL = "PrivacyLevel";


EventXmlParser::EventXmlParser()
    : XmlParser(EVENT_XML), findEvent_(false)
{}

EventXmlParser::~EventXmlParser()
{

}

sp<EventXmlParser> EventXmlParser::getInstance()
{
    static sp<EventXmlParser> intance_ = new EventXmlParser;
    return intance_;
}

bool EventXmlParser::IsMatch(const std::string& localName)
{
    if (localName == EVENT_LOCAL_NAME) {
        return true;
    }
    return false;
}


void EventXmlParser::SetFaultType(const std::string& faultType)
{
    Event::MessageType type = Event::MessageType::NONE;

    if (faultType == "FAULT") {
        type = Event::MessageType::FAULT_EVENT;
    }
    if (faultType == "STATICS") {
        type = Event::MessageType::STATISTICS_EVENT;
    }
    curEventInfoCfg_->faultType_ = type;
}

void EventXmlParser::InitEventInfoConfig(std::map<std::string, std::string> attributes)
{
    findEvent_ = true;
    curEventInfoCfg_->faultLevel_ = EventUtil::GetFaultLevel(attributes[EVENT_ATTR_FALULT_LEVEL]);
    SetFaultType(attributes[EVENT_ATTR_FAULT_TYPE]);
    curEventInfoCfg_->privacyLevel_ = EventUtil::GetPrivacyLevel(attributes[EVENT_ATTR_PRIVACY_LEVEL]);
}

EventInfoConfig::ParamType EventXmlParser::GetParamType(const std::string& type)
{
    EventInfoConfig::ParamType paramType = EventInfoConfig::ParamType::NONE;

    if (type == "string") {
        paramType = EventInfoConfig::ParamType::STRING;
    }
    if (type == "bool") {
        paramType = EventInfoConfig::ParamType::BOOL;
    }
    if (type == "short") {
        paramType = EventInfoConfig::ParamType::SHORT;
    }
    if (type == "int") {
        paramType = EventInfoConfig::ParamType::INT;
    }
    if (type == "float") {
        paramType = EventInfoConfig::ParamType::FLOAT;
    }
    if (type == "long") {
        paramType = EventInfoConfig::ParamType::LONG;
    }
    if (type == "class") {
        paramType = EventInfoConfig::ParamType::CLASS;
    }
    return paramType;
}


void EventXmlParser::AddEventParamInfoCfg(std::map<std::string, std::string> attributes)
{
    EventInfoConfig::ParamInfoConfig param;
    param.id = StringUtil::ConvertInt(attributes[EVENT_PARAM_ATTR_ID]);
    param.len = StringUtil::ConvertInt(attributes[EVENT_PARAM_ATTR_LEN]);
    param.arrLen = StringUtil::ConvertInt(attributes[EVENT_PARAM_ATTR_ARRLEN]);
    param.classID = StringUtil::ConvertInt(attributes[EVENT_PARAM_ATTR_CLASSID]);
    param.name = attributes[EVENT_PARAM_ATTR_NAME];
    param.type = GetParamType(attributes[EVENT_PARAM_ATTR_TYPE]);
    param.privacyLevel = curEventInfoCfg_->privacyLevel_;
    if (attributes[EVENT_PARAM_ATTR_PRIVACY_LEVEL] != "") {
        param.privacyLevel = EventUtil::GetPrivacyLevel(attributes[EVENT_PARAM_ATTR_PRIVACY_LEVEL]);
    }
    param.defVal = attributes[EVENT_PARAM_ATTR_DEFVAL];
    curEventInfoCfg_->params_.push_back(param);
}

void EventXmlParser::OnStartElement(const std::string& localName, std::map<std::string, std::string> attributes)
{
    int eventId = StringUtil::ConvertInt(attributes[EVENT_ATTR_ID]);
    if (eventId == curEventInfoCfg_->eventId_) {
        SetStart();
        InitEventInfoConfig(attributes);
    } else if (IsStart()) {
        if (localName == EVENT_PARAM_LOCAL_NAME) {
            AddEventParamInfoCfg(attributes);
        }
    }
}

void EventXmlParser::OnEndElement(const std::string& localName)
{
    if (localName != EVENT_LOCAL_NAME) {
        return;
    }
    SetFinish();
}


sp<EventInfoConfig> EventXmlParser::GetEventInfoConfig(const std::string& workDir, uint32_t eventId)
{
    curEventInfoCfg_ = new EventInfoConfig();
    curEventInfoCfg_->eventId_ = eventId;
    findEvent_ = false;

    if (ParserXmlFile(workDir + EVENT_XML) != 0) {
        MISIGHT_LOGW("upload failed, %s, %s", workDir.c_str(), EVENT_XML.c_str());
        curEventInfoCfg_ = nullptr;
        return nullptr;
    }
    if (!findEvent_) {
        curEventInfoCfg_ = nullptr;
        return nullptr;
    }
    return curEventInfoCfg_;
}
};
}
