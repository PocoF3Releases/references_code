/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event privacy compliance plugin head file
 *
 */
#include "event_privacy_compliance.h"

#include "dev_info.h"
#include "event_xml_parser.h"
#include "event_util.h"
#include "file_util.h"
#include "filter_xml_parser.h"
#include "json_util.h"
#include "plugin_factory.h"
#include "privacy_xml_parser.h"
#include "string_util.h"

namespace android {
namespace MiSight {
REGISTER_PLUGIN(EventPrivacyCompliance);

EventPrivacyCompliance::EventPrivacyCompliance()
    : devVersion_("")
{}

EventPrivacyCompliance::~EventPrivacyCompliance()
{}

void EventPrivacyCompliance::OnLoad()
{
    // set plugin info
    SetVersion("EventPrivacyCompliance-1.0");
    SetName("EventPrivacyCompliance");

    auto platform = GetPluginContext();
    devVersion_ = DevInfo::GetMiuiVersion(platform);
}


void EventPrivacyCompliance::OnUnload()
{
}

bool EventPrivacyCompliance::IsInFilterList(std::map<std::string, std::string> filterList, Json::Value paramJson)
{
    if (filterList.size() == 0 || !paramJson.isObject()) {
        return true;
    }


    for (auto& filter : filterList) {
        std::string name = filter.first;
        if (!paramJson.isMember(name)) {
            continue;
        }
        std::list<std::string> valList = StringUtil::SplitStr(filter.second, ',');
        std::string value = paramJson[name].asString();
        auto it = std::find_if(valList.begin(), valList.end(), [&](std::string val) {
            if (val == value) {
                return true;
            }
            return false;
        });
        if (it != valList.end()) {
            return true;
        }
    }
    return false;
}


bool EventPrivacyCompliance::IsFilterEvent(sp<Event> event, Json::Value paramJson)
{
    std::string runMode = "";
    std::string filePath = "";
    DevInfo::GetRunMode(GetPluginContext(), runMode, filePath);
    if (filePath == "") {
        return false;
    }
    if (!FileUtil::FileExists(filePath)) {
        MISIGHT_LOGD("filepath %s not exist", filePath.c_str());
        return false;
    }
    sp<FilterXmlParser> filterXml = new FilterXmlParser();
    sp<EventFilterInfo> eventInfo = filterXml->GetFilterConfig(runMode, filePath, event->eventId_);
    if (eventInfo == nullptr) {
        MISIGHT_LOGE("filepath %s event %u not exist", filePath.c_str(), event->eventId_);
        return false;
    }

    // in whitelist
    if (eventInfo->hasWhite_) {
        if (IsInFilterList(eventInfo->whiteList_, paramJson)) {
            return false;
        }
        return true;
    }
    // not in blacklist
    if (eventInfo->hasBlack_) {
        if (IsInFilterList(eventInfo->blackList_, paramJson)) {
            return true;
        }
        return false;
    }
    return false;
}

bool EventPrivacyCompliance::CanProcessEvent(sp<Event> event)
{
    if (EventUtil::FAULT_EVENT_MIN > event->eventId_ || event->eventId_ > EventUtil::FAULT_EVENT_MAX) {
        return false;
    }

    return true;
}


EVENT_RET EventPrivacyCompliance::OnEvent(sp<Event> &event)
{
    if (!CanProcessEvent(event)) {
        MISIGHT_LOGE("check pipeline config %d", event->eventId_);
        return ON_FAILURE;
    }

    // param check
    std::string eventStr = event->GetValue(EventUtil::DEV_SAVE_EVENT);
    MISIGHT_LOGD("EventPrivacyCompliance event=%u %s", event->eventId_, eventStr.c_str());
    if (eventStr == "") {
        MISIGHT_LOGI("event %u have no param", event->eventId_);
        return ON_FAILURE;
    }

    // param must be a json string
    Json::Value paramJson;
    Json::Value expJson;

    if (!JsonUtil::ConvertStrToJson(eventStr, paramJson)) {
        MISIGHT_LOGI("event %u param not json string, %s", event->eventId_, eventStr.c_str());
        return ON_FAILURE;
    }

    if (!ProcessEvent(event, paramJson, expJson)) {
        MISIGHT_LOGI("event %u param compliance process failed", event->eventId_);
        return ON_FAILURE;
    }

    eventStr = JsonUtil::ConvertJsonToStr(expJson);
    if (eventStr == "" || eventStr == "null") {
        MISIGHT_LOGI("event %u param is null after compliance, %s", event->eventId_, eventStr.c_str());
        return ON_FAILURE;
    }

    if (IsFilterEvent(event, paramJson)) {
        return ON_FAILURE;
    }
    event->SetValue(EventUtil::DEV_SAVE_EVENT, eventStr);
    return ON_SUCCESS;
}

bool EventPrivacyCompliance::CheckStringParamValid(EventInfoConfig::ParamInfoConfig paramCfg,
    Json::Value memberJson, Json::Value &expJson, bool isArray)
{
    if (!memberJson.isString()) {
        MISIGHT_LOGD("remove %s, expect string type", paramCfg.name.c_str());
        return false;
    }

    std::string val = memberJson.asString();
    uint32_t len = val.size();
    if (paramCfg.len != 0 && paramCfg.len < val.size()) {
        len = paramCfg.len;
    }
    if (isArray) {
        expJson.append(val.substr(0, len));
    } else {
        expJson[paramCfg.name] = val.substr(0, len);
    }
    return true;
}

bool EventPrivacyCompliance::CheckClassParamValid(EventInfoConfig::ParamInfoConfig paramCfg, Json::Value memberJson,
    Json::Value &expJson, bool isArray)
{
    if (paramCfg.classID == 0) {
        MISIGHT_LOGD("class %s id must not 0", paramCfg.name.c_str());
        return false;
    }
    sp<Event> event = new Event("classId");
    if (event == nullptr) {
        return false;
    }
    event->eventId_ = paramCfg.classID;
    Json::Value expClassJson;
    if (!ProcessEvent(event, memberJson, expClassJson)) {
        return false;
    }
    if (isArray) {
        expJson.append(expClassJson);
    } else {
        expJson[paramCfg.name] = expClassJson;
    }
    return true;
}

bool EventPrivacyCompliance::IsParamValid(EventInfoConfig::ParamInfoConfig paramCfg, Json::Value memberJson, Json::Value &expJson, bool isArray)
{
    switch(paramCfg.type) {
        case EventInfoConfig::ParamType::STRING: {
            return CheckStringParamValid(paramCfg, memberJson, expJson, isArray);
        }
        case EventInfoConfig::ParamType::BOOL:
        case EventInfoConfig::ParamType::SHORT:
        case EventInfoConfig::ParamType::INT:
        case EventInfoConfig::ParamType::FLOAT:
        case EventInfoConfig::ParamType::LONG: {
            if (paramCfg.type == EventInfoConfig::ParamType::BOOL) {
                if (!memberJson.isBool()) {
                    break;
                }
            } else if (!memberJson.isNumeric()) {
                break;
            }
            if (isArray) {
                expJson.append(memberJson);
            } else {
                expJson[paramCfg.name] = memberJson;
            }
            return true;
        }
        case EventInfoConfig::ParamType::CLASS: {
            if (memberJson.isObject()) {
                return CheckClassParamValid(paramCfg, memberJson, expJson, isArray);
            }
            break;
        }
        default:
            break;
    }
    MISIGHT_LOGI("remove %s, not support type %d", paramCfg.name.c_str(), paramCfg.type);
    return false;
}

bool EventPrivacyCompliance::CheckParamValid(EventInfoConfig::ParamInfoConfig paramCfg, Json::Value paramJson, Json::Value &expJson)
{
    if (!paramJson.isObject()) {
        MISIGHT_LOGW("paramJson not valid");
        return false;
    }
    // event.xml config default value, but json not contain, add it to newjson
    if (!paramJson.isMember(paramCfg.name)) {
        if (paramCfg.defVal != "") {
            expJson[paramCfg.name] = paramCfg.defVal;
        }
        MISIGHT_LOGD("not include %s", paramCfg.name.c_str());
        return true;
    }

    Json::Value memberJson = paramJson[paramCfg.name];
    if (paramCfg.arrLen != 0) {
        if (!memberJson.isArray()) {
            MISIGHT_LOGI("remove %s, expect an array", paramCfg.name.c_str());
            return false;
        }
        Json::Value newJson;
        int inIndex = 0;
        for (int index = 0; index < memberJson.size(); index++) {
            if (inIndex >= paramCfg.arrLen) {
                break;
            }
            if (!IsParamValid(paramCfg, memberJson[index], newJson, true)) {
                continue;
            }
            //newJson.append(newMember);
            inIndex++;
        }
        if (inIndex == 0) {
            MISIGHT_LOGI("remove %s, arrLen=%u", paramCfg.name.c_str(), memberJson.size());
            return false;
        }
        expJson[paramCfg.name] = newJson;
    } else {
        if (!IsParamValid(paramCfg, memberJson, expJson, false)) {
            return false;
        }
    }
    return true;
}

bool EventPrivacyCompliance::ProcessEvent(sp<Event> event, Json::Value paramJson, Json::Value &expJson)
{
    auto platform = GetPluginContext();
    std::string cfgDir = platform->GetConfigDir(EventXmlParser::getInstance()->GetXmlFileName());
    sp<EventInfoConfig> eventInfoCfg = EventXmlParser::getInstance()->GetEventInfoConfig(cfgDir, event->eventId_);
    if (eventInfoCfg == nullptr) {
        MISIGHT_LOGD("eventId=%u not found in config", event->eventId_);
        return false;
    }

    EventUtil::FaultLevel faultLevel = EventUtil::FaultLevel::GENERAL;
    EventUtil::PrivacyLevel privacyLevel = EventUtil::PrivacyLevel::L4;
    std::string locale = platform->GetSysProperity(DevInfo::DEV_PRODUCT_LOCALE, "");
	std::string cfgPrvDir = platform->GetConfigDir(PrivacyXmlParser::getInstance()->GetXmlFileName());
    PrivacyXmlParser::getInstance()->GetPolicyByLocale(cfgPrvDir, devVersion_, locale, faultLevel, privacyLevel);
    MISIGHT_LOGD("devVersion==%s,faultLevel=%d,prv=%d",devVersion_.c_str(), faultLevel, privacyLevel);

    // fault level check
    if (faultLevel > eventInfoCfg->faultLevel_) {
        MISIGHT_LOGI("eventId=%u,faultLevel=%d,cfg=%d", event->eventId_, faultLevel, eventInfoCfg->faultLevel_);
        return false;
    }

    // privacy level check
    if (privacyLevel < eventInfoCfg->privacyLevel_) {
        MISIGHT_LOGI("eventId=%u,privacy=%d,cfg=%d", event->eventId_, privacyLevel, eventInfoCfg->privacyLevel_);
        return false;
    }
    event->messageType_ = eventInfoCfg->faultType_;

    // check param valid
    for (auto& paramInfo : eventInfoCfg->params_) {
        if (paramInfo.name == "") {
            MISIGHT_LOGD("eventId=%u, get para name is null,%d", event->eventId_, paramInfo.id);
            continue;
        }
        if (privacyLevel < paramInfo.privacyLevel) {
            MISIGHT_LOGD("eventId=%u, para level is not compliance,%d:%s", event->eventId_, paramInfo.id , paramInfo.name.c_str());
            continue;
        }

        bool ret = CheckParamValid(paramInfo, paramJson, expJson);
        MISIGHT_LOGD("eventId=%u, %s ret=%d", event->eventId_, paramInfo.name.c_str(), ret);
    };
    return true;
}
}
}
