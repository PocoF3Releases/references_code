/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight config parser head file
 *
 */
#ifndef MISIGH_CONFIG_H
#define MISIGH_CONFIG_H

#include <list>
#include <string>


namespace android {
namespace MiSight {

class Config {
public:
    struct PluginInfo {
        bool isStatic;
        bool isEventSource;
        int32_t delayLoad;
        std::string name;
        std::string threadName;
        std::string threadType;
        std::list <std::string> pipelines;
    };

    struct PipelineInfo {
        std::string name;
        std::list <std::string> plugins;
    };

    explicit Config(const std::string& configPath);
    ~Config();
    const std::list<Config::PipelineInfo>& GetPipelineInfoList() const;
    const std::list<Config::PluginInfo>& GetPluginInfoList() const;
    bool Parse();
private:
    std::string configPath_;
    std::list<Config::PipelineInfo> pipelineInfos_;
    std::list<Config::PluginInfo> pluginInfos_;

    bool ParsePlugin(const std::string& configLine);
    bool ParsePipeline(const std::string& configLine);
    bool ParsePipelineTrigger(const std::string& configLine);
    bool IsPluginExsit(const std::string& pluginName);

};
} // namespace MiSight
} // namespace android
#endif
