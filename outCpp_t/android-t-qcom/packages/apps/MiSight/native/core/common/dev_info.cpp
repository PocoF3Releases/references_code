/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight device info implement file
 *
 */
#include "dev_info.h"

#include <list>
#include <regex>

#include "cmd_processor.h"
#include "log.h"
#include "string_util.h"


namespace android {
namespace MiSight {
namespace DevInfo {

MiuiVersionType GetMiuiVersionType(PluginContext* context)
{
    std::string devStr = GetMiuiVersion(context);
    if (IsMiuiStableVersion(devStr)) {
        return STABLE;
    }
    if (IsMiuiDevVersion(devStr)) {
        return DEV;
    }
    return PRE;
}

std::string GetMiuiVersion(PluginContext* context)
{
    if (context == nullptr) {
       return "";
    }
    std::string devStr = context->GetSysProperity(DEV_VERSION_PROP, "");
    return devStr;
}

bool IsMiuiStableVersion(const std::string& version)
{
    if (version == "") {
        return false;
    }
    std::string pattern = "^(V\\d{1,})(\\.\\d{1,})*(\\.([A-Z]{4,}))$";
    std::smatch result;
    if (!regex_search(version, result, std::regex(pattern))) {
        return false;
    }
    return true;
}

bool IsMiuiDevVersion(const std::string& version)
{
    if (version == "") {
        return false;
    }
    std::string pattern = "^(V\\d{1,})(\\.\\d{1,})*(\\.DEV)$";
    std::smatch result;
    if (!regex_search(version, result, std::regex(pattern))) {
        return false;
    }
    return true;
}

bool IsInnerVersion(const std::string& version)
{
    if (version == "") {
        return true;
    }
    if (IsMiuiDevVersion(version) || IsMiuiStableVersion(version)) {
        return false;
    }
    return true;
}

void SetUploadSwitch(PluginContext* context, int onSwitch)
{
    if (context == nullptr) {
       return;
    }
    std::string ue = "";
    switch (onSwitch) {
        case UE_SWITCH_ON:
            ue = "on";
            break;
        case UE_SWITCH_ONCTRL:
            ue = "onctrl";
            break;
        case UE_SWITCH_OFF:
        default:
            ue = "off";
            break;
    }
    context->SetSysProperity(DEV_MISIGHT_UE_MODE, ue, true);
}

std::string GetUploadSwitch(PluginContext* context)
{
    int ue = UE_SWITCH_OFF;
    if (context == nullptr) {
        return "off";
    }

    std::string userSwitch = context->GetSysProperity(DEV_MISIGHT_UE_MODE, "");

    if (userSwitch == "") {
        //settings get secure upload_log_pref
        std::string ueStr = "";
        std::string ueDebugStr = "";
        if (!CmdProcessor::GetCommandResult("settings get secure upload_log_pref", ueStr)) {
            return "off";
        }
        if (!CmdProcessor::GetCommandResult("settings get secure upload_debug_log_pref", ueDebugStr)) {
            return "off";
        }
        int ueInt = StringUtil::ConvertInt(ueStr);
        int ueDebug = StringUtil::ConvertInt(ueDebugStr);
        if (ueInt == 1 && ueDebug == 1) {
            userSwitch = "on";
            ue = UE_SWITCH_ON;
        } else {
            userSwitch = "off";
        }
        SetUploadSwitch(context, ue);
    }
    return userSwitch;
}

std::string GetDefaultRunMode(PluginContext* context)
{
    std::string runMode = "InnerDev";
    if (!IsInnerVersion(GetMiuiVersion(context))) {
        runMode = "Commerical";
    }
    return runMode;
}

void SetRunMode(PluginContext* context, const std::string& runMode, const std::string& filePath)
{
    if (context == nullptr) {
       MISIGHT_LOGE("it's null");
       return;
    }

    std::string devStr = runMode;
    if (runMode == "") {
        devStr = GetDefaultRunMode(context);
    } else if (filePath != "") {
        devStr += "|" + filePath;
    }
    context->SetSysProperity(DEV_MISIGHT_TEST_MODE, devStr, true);
    MISIGHT_LOGI("set run mode %s", devStr.c_str());
}

bool GetRunMode(PluginContext* context, std::string& runMode, std::string& filePath)
{
    std::string runModeProp = "";
    if (context != nullptr) {
        runModeProp = context->GetSysProperity(DEV_MISIGHT_TEST_MODE, "");
    }

    if (runModeProp != "") {
        std::list<std::string> modeList = StringUtil::SplitStr(runModeProp, '|');
        runMode = modeList.front();
        if (modeList.size() == 2) {
            filePath = modeList.back();
        }
    } else {
        runMode = GetDefaultRunMode(context);
    }
    return true;
}

static uint64_t devActiveTime = 0;
void SetActivateTime(uint64_t activeTime)
{
    devActiveTime = activeTime;
}

uint64_t GetActivateTime()
{
    return devActiveTime;
}

} // namespace DevInfo
} // namespace MiSight
} // namespace android

