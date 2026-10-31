/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event privacy compliance plugin head file
 *
 */
#ifndef MISIGH_PLUGIN_TESTABILITY_EVENT_PRIVACY_COMPLIANCE_PLUGIN_H
#define MISIGH_PLUGIN_TESTABILITY_EVENT_PRIVACY_COMPLIANCE_PLUGIN_H

#include <string>

#include <json/json.h>
#include <utils/StrongPointer.h>

#include "dev_info.h"
#include "event.h"
#include "event_xml_parser.h"
#include "event_util.h"
#include "plugin.h"


namespace android {
namespace MiSight {

class EventPrivacyCompliance: public Plugin {
public:
    EventPrivacyCompliance();
    ~EventPrivacyCompliance();

    void OnLoad() override;
    void OnUnload() override;

    bool CanProcessEvent(sp<Event> event) override;
    EVENT_RET OnEvent(sp<Event> &event) override;


private:
    bool ProcessEvent(sp<Event> event, Json::Value paramJson, Json::Value &expJson);
    bool IsParamValid(EventInfoConfig::ParamInfoConfig paramCfg, Json::Value memberJson, Json::Value &expJson, bool isArray);
    bool CheckParamValid(EventInfoConfig::ParamInfoConfig paramCfg, Json::Value paramJson, Json::Value &expJson);
    bool CheckStringParamValid(EventInfoConfig::ParamInfoConfig paramCfg,
        Json::Value memberJson, Json::Value &expJson, bool isArray);
    bool CheckClassParamValid(EventInfoConfig::ParamInfoConfig paramCfg,
        Json::Value memberJson, Json::Value &expJson, bool isArray);
    bool IsFilterEvent(sp<Event> event, Json::Value paramJson);
    bool IsInFilterList(std::map<std::string, std::string> filterList, Json::Value paramJson);

    std::string devVersion_;


};
}
}
#endif

