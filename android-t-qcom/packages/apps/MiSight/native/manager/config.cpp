/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight config parser implements
 *
 */

#include "config.h"

#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

#include "log.h"
#include "string_util.h"

namespace android {
namespace MiSight {

Config::Config(const std::string& configPath)
: configPath_(configPath)
{
}
Config::~Config()
{
    pipelineInfos_.clear();
    pluginInfos_.clear();
    MISIGHT_LOGE("config file exit!");
}

const std::list<Config::PipelineInfo>& Config::GetPipelineInfoList() const
{
    return pipelineInfos_;
}

const std::list<Config::PluginInfo>& Config::GetPluginInfoList() const
{
    return pluginInfos_;
}

bool Config::Parse()
{
    std::ifstream in;
    in.open(configPath_);
    if (!in.is_open()) {
        MISIGHT_LOGE("fail to open config file.");
        return false;
    }
    uint32_t fieldCount = 0;
    std::string field = "";
    std::string buf = "";
    const int32_t fieldNameField = 1;
    const int32_t fieldCountField = 2;
    while (getline(in, buf)) {
        std::string strTmp = StringUtil::TrimStr(buf);
        if (strTmp == "") {
            continue;
        }
        std::smatch result;
        if (fieldCount == 0) {
            if (!regex_search(strTmp, result, std::regex("(plugins|pipelines|pipelinetriggers)\\s*:\\s*(\\d+)"))) {
                MISIGHT_LOGE("match field failed");
                in.close();
                return false;
            }
            field = StringUtil::TrimStr(result[fieldNameField]);
            fieldCount = atoi(std::string(result[fieldCountField]).c_str());
            MISIGHT_LOGI("find field:%s %d", field.c_str(), fieldCount);
            continue;
        }
        if (field == "plugins" && !ParsePlugin(strTmp)) {
            return false;
        } else if (field == "pipelines" && !ParsePipeline(strTmp)) {
            return false;
        } else if (field == "pipelinetriggers" && !ParsePipelineTrigger(strTmp)) {
            return false;
        }
        fieldCount--;
    }
    in.close();
    MISIGHT_LOGI("prase config ok");
    return true;
}

bool Config::ParsePlugin(const std::string& configLine)
{
    /* plguin content : PluginName1[thread:threadName1]:0 static */
    std::smatch result;
    if (!regex_search(configLine, result,
        std::regex("([^\\[]+)\\[([^\\[:]*):?([^:]*)\\]\\s*:(\\d+)\\s+(dynamic|static)"))) {
        MISIGHT_LOGW("no match plugin expression found");
        return false;
    }
    /* match : PluginName1[thread:threadName1]:0 static */
    const int32_t pluginNameField = 1;
    const int32_t workHandlerTypeField = 2;
    const int32_t workHandlerNameField = 3;
    const int32_t loadDelayField = 4;
    const int32_t loadTypeField = 5;
    PluginInfo info;
    info.isStatic = (result[loadTypeField] == "static");
    info.isEventSource = false;

    info.delayLoad = atoi(std::string(result[loadDelayField]).c_str());
    info.name = result[pluginNameField];
    
    info.threadType = result[workHandlerTypeField];
    info.threadName = result[workHandlerNameField];
    if (info.threadType != "" && info.threadType != "thread") {
        MISIGHT_LOGW("plugin %s thread type %s invalid", info.name.c_str(), info.threadType.c_str());
        return false;
    }
    if ((info.threadType != "" && info.threadName == "") || (info.threadType == "" && info.threadName != "")) {
        MISIGHT_LOGW("plugin %s thread type %s or name %s invalid", info.name.c_str(), info.threadType.c_str(), info.threadName.c_str());
        return false;
    }

    if (IsPluginExsit(info.name)) {
        MISIGHT_LOGW("redefine plugin %s", info.name.c_str());
        return false;
    }
    pluginInfos_.push_back(info);
    return true;
}

bool Config::IsPluginExsit(const std::string& pluginName)
{
    auto it = std::find_if(pluginInfos_.begin(), pluginInfos_.end(), [&](PluginInfo& info) {
        if (info.name == pluginName) {
            return true;
        }
        return false;
    });
    if (it == pluginInfos_.end()) {
        return false;
    }
    return true;
}

bool Config::ParsePipeline(const std::string& configLine)
{
    std::smatch result;
    /* pipeline content: PipelineName1:PluginName1 PluginName2 */
    if (!regex_search(configLine, result, std::regex("(\\S+)\\s*:(.+)"))) {
        MISIGHT_LOGW("Fail to match pipeline expression");
        return false;
    }

    const int32_t pipelineNameField = 1;
    const int32_t pluginNameListField = 2;
    PipelineInfo pipelineInfo;
    pipelineInfo.name = result.str(pipelineNameField);
    pipelineInfo.plugins = StringUtil::SplitStr(result.str(pluginNameListField));

    for (auto& pluginName : pipelineInfo.plugins) {
        if (!IsPluginExsit(pluginName)) {
            MISIGHT_LOGW("pipeline %s include undefine plugin %s", pipelineInfo.name.c_str(), pluginName.c_str());
            return false;
        }
    }

    pipelineInfos_.push_back(pipelineInfo);
    return true;
}

bool Config::ParsePipelineTrigger(const std::string& configLine)
{
    std::smatch result;
    /* tringger content: EventSource:PipelineName1 PipeLineName2 */
    if (!regex_search(configLine, result, std::regex("(\\S+)\\s*:(.+)"))) {
        MISIGHT_LOGW("Fail to match pipeline trigger expression");
        return false;
    }

    const int32_t pipelineTriggerNameField = 1;
    const int32_t pipelineNameListField = 2;
    std::string eventSourceName = StringUtil::TrimStr(result.str(pipelineTriggerNameField));
    std::list <std::string> pipelines = StringUtil::SplitStr(result.str(pipelineNameListField));
    auto it = std::find_if(pluginInfos_.begin(), pluginInfos_.end(), [&](PluginInfo& info) {
        if (info.name == eventSourceName) {
            info.isEventSource = true;
            info.pipelines = pipelines;
            return true;
        }
        return false;
    });
    if (it == pluginInfos_.end()) {
        MISIGHT_LOGE("%s not define in plugins", eventSourceName.c_str());
        return false;
    }

    for (auto& pipelineName : pipelines) {
        auto it = std::find_if(pipelineInfos_.begin(), pipelineInfos_.end(), [&](PipelineInfo& pipelineInfo) {
            if (pipelineInfo.name == pipelineName) {
                return true;
            }
            return false;
        });
        if (it == pipelineInfos_.end()) {
            MISIGHT_LOGW("pipelinetriggers %s include undefine pipeline %s", eventSourceName.c_str(), pipelineName.c_str());
            return false;
        }
    }
    MISIGHT_LOGD("%s is an event source", eventSourceName.c_str());
    return true;
}
}; // namespace MiSight
}; // namespace android
