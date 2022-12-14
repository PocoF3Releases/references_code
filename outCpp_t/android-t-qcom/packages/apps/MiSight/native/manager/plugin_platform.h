/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin platform head file
 *
 */
#ifndef MISIGH_PLUGIN_PLATFORM_H
#define MISIGH_PLUGIN_PLATFORM_H

#include <map>
#include <string>

#include <utils/Singleton.h>
#include <utils/StrongPointer.h>

#include "config.h"
#include "event.h"
#include "event_dispatcher.h"
#include "event_loop.h"
#include "event_source.h"
#include "pipeline.h"
#include "plugin.h"

namespace android {
namespace MiSight {
class PluginPlatform : public PluginContext, public Singleton<PluginPlatform> {
protected:
    PluginPlatform();
    ~PluginPlatform();
public:
    /* parse input args. args include:
     * -configDir : plugin config file directory
     * -configName : plugin config file name
     * -workDir : platform work directory
     * -name : platform name
     */
    int32_t ParseArgs(int argc, char* argv[]);
    bool StartPlatform();

    /* post event to broadcast queue, the event will be delivered to all plugin that concern this event */
    void PostUnorderedEvent(sp<Plugin> plugin, sp<Event> event) override;

    /* register listener to unordered broadcast queue */
    void RegisterUnorderedEventListener(sp<EventListener> listener) override;

    /* send a event to a specific plugin and wait the return of the OnEvent */
    bool PostSyncEventToTarget(sp<Plugin> caller, const std::string& callee,
                                       sp<Event> event) override;
    /* send a event to a specific plugin and return at once */
    void PostAsyncEventToTarget(sp<Plugin> caller, const std::string& callee,
                                        sp<Event> event) override;
    /* get the shared event loop reference */
    sp<EventLoop> GetSharedWorkLoop() override;

    /* insert  event into pipeline */
    void InsertEventToPipeline(const std::string& triggerName, sp<Event> event) override;

    std::string GetSysProperity(const std::string& key, const std::string& defVal) override;
    bool SetSysProperity(const std::string& key, const std::string& val, bool update) override;
    void RegisterPipelineListener(const std::string& triggerName, sp<Plugin> plugin) override;

    /* check if all non-pending loaded plugins are loaded */
    bool IsReady() override;

    const std::map<std::string, sp<Plugin>>& GetPluginMap()
    {
        return pluginMap_;
    }

    const std::map<std::string, sp<Pipeline>>& GetPipelineMap()
    {
        return pipelines_;
    }

    const std::string GetPluginConfigPath();
    const std::string GetWorkDir() override
    {
        return workDir_;
    }
    const std::string GetConfigDir() override
    {
        return configDir_;
    }
    const std::string GetConfigDir(const std::string configName) override;
    const std::string GetCloudConfigDir() override
    {
        return curCloudCfgDir_;
    }
    void SetCloudConfigFolder(const std::string& cloudCfgFolder);

private:
    friend class Singleton<PluginPlatform>;
    bool IsExist();
    void StartDispatchQueue();
    void LoadPlugins(const Config& config);
    void ConstructPlugin(const Config::PluginInfo &pluginInfo);
    void ConstructPipeline(const Config::PipelineInfo &pipelineInfo);
    void LoadPlugin(const Config::PluginInfo& pluginInfo);
    void StartEventSource(sp<EventSource> source);
    sp<EventLoop> GetLooperByName(const std::string& name);
    void LoadDelayedPlugin(const Config::PluginInfo& pluginInfo);
    void NotifyPlatformReady();
    void ForceCreateDirs();
    bool isReady_;
    std::string configDir_;
    std::string cloudUpdateConfigDir_;
    std::string workDir_;
    std::string configName_;
    std::string curCloudCfgDir_;
    std::string supportDev_;
    std::string region_;
    sp<EventDispatcher> unorderQueue_;
    sp<EventLoop> sharedWorkLoop_;
    std::map<std::string, sp<Plugin>> pluginMap_;
    std::map<std::string, sp<Pipeline>> pipelines_;
    std::map<std::string, sp<EventLoop>> workLoopPool_;
    std::map<std::string, std::string> sysProperity_;
};
} // namespace MiSight
} // namespace android
#endif
