/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin platform implements
 *
 */
#include "plugin_platform.h"

#include <cinttypes>
#include <fstream>
#include <pthread.h>
#include <unistd.h>
#include <vector>

#include "config.h"
#include "dev_info.h"
#include "file_util.h"
#include "log.h"
#include "plugin_factory.h"
#include "string_util.h"
#include "sys_env.h"
#include "time_util.h"

ANDROID_SINGLETON_STATIC_INSTANCE(::android::MiSight::PluginPlatform)

namespace android {
namespace MiSight {
constexpr uint32_t AID_ROOT = 0;
constexpr uint32_t AID_SYSTEM = 1000;
static const std::string DEF_MISIGHT_WORK_DIR = "/data/miuilog/misight/";
static const std::string DEF_MISIGHT_PID_FILE_NAME = "misight.pid";
static const std::string DEF_CONFIG_PATH = "/system/etc/misight/";
static const std::string DEF_CONFIG_NAME = "plugin_config";
static const std::string DEF_CONFIG_GLOBAL_NAME = "plugin_config_global";
static const std::string DEF_CLOUD_CONFIG_PATH = "/data/miuilog/misight/config/";
static const std::string DEF_CLOUD_CONFIG_NAME0 = "update0";
static const std::string DEF_CLOUD_CONFIG_NAME1 = "update1";


PluginPlatform::PluginPlatform()
    : isReady_(false), configDir_(DEF_CONFIG_PATH),
      cloudUpdateConfigDir_(DEF_CLOUD_CONFIG_PATH),
      workDir_(DEF_MISIGHT_WORK_DIR),
      configName_(DEF_CONFIG_NAME),
      curCloudCfgDir_(DEF_CLOUD_CONFIG_NAME0),
      supportDev_(""),
      region_("cn"),
      unorderQueue_(nullptr),
      sharedWorkLoop_(nullptr)
{

}
PluginPlatform::~PluginPlatform()
{
    if (unorderQueue_ != nullptr) {
        unorderQueue_->OnUnload();
    }

    if (sharedWorkLoop_ != nullptr) {
        sharedWorkLoop_->StopLoop();
    }
}

/* parse input args. args include:
 * -configDir : plugin config file directory
 * -configName : plugin config file name
 * -workDir : platform work directory
 */
int32_t PluginPlatform::ParseArgs(int argc, char* argv[])
{
    MISIGHT_LOGD("agc=%d", argc);
    for (int32_t loop = 1; loop < argc; loop = loop + 2) {
        if (strcmp(argv[loop], "-configDir") == 0) {
            configDir_ = argv[loop + 1];
            configDir_ = "/" + StringUtil::TrimStr(configDir_, '/') + "/";
            continue;
        }
        if (strcmp(argv[loop], "-configName") == 0) {
            configName_ = argv[loop + 1];
            configName_ = StringUtil::TrimStr(configName_);
            continue;
        }
        if (strcmp(argv[loop], "-workDir") == 0) {
            workDir_ = argv[loop + 1];
            workDir_ = "/" + StringUtil::TrimStr(workDir_, '/') + "/";
            continue;
        }
        if (strcmp(argv[loop], "-cloudCfgDir") == 0) {
            cloudUpdateConfigDir_ = argv[loop + 1];
            cloudUpdateConfigDir_ = "/" + StringUtil::TrimStr(cloudUpdateConfigDir_, '/') + "/";
            continue;
        }
        if (strcmp(argv[loop], "-devList") == 0) {
            supportDev_ = argv[loop + 1];
            continue;
        }
        if (strcmp(argv[loop], "-region") == 0) {
            region_ = argv[loop + 1];
            continue;
        }
        MISIGHT_LOGE("unsupport args %d %s", loop, argv[loop]);
        return -1;
    };
    MISIGHT_LOGI("ParseArgs success %d", argc);
    return 0;
}

bool PluginPlatform::IsExist()
{
    std::string filePath = workDir_ + DEF_MISIGHT_PID_FILE_NAME;
    std::ifstream fin(filePath);
    std::string procName = "misight";

    if (fin.is_open()) {
        std::string pidSaved;
        getline(fin, pidSaved);
        std::ifstream procFile("/proc/" + pidSaved + "/cmdline");
        MISIGHT_LOGI("last misight pid %s", pidSaved.c_str());
        if (procFile.is_open()) {
            std::string name;
            getline(procFile, name);
            name = StringUtil::TrimStr(name);
            if (name.rfind(procName, name.size()) != std::string::npos) {
                MISIGHT_LOGE("platform already exist");
                return true;
            }
        } else {
            MISIGHT_LOGW("can't open file %s", pidSaved.c_str());
        }
    }
    std::ofstream fout(filePath);
    if (!fout.is_open()) {
        MISIGHT_LOGE("platform create file failed");
        return true;
    }

    std::string pid = std::to_string(getpid());
    fout.write(pid.c_str(), pid.length());
    fout.flush();
    return false;
}

void PluginPlatform::ForceCreateDirs()
{
    std::string cloudCfg0 = cloudUpdateConfigDir_ + DEF_CLOUD_CONFIG_NAME0 + "/";
    std::string cloudCfg1 = cloudUpdateConfigDir_ + DEF_CLOUD_CONFIG_NAME1 + "/";

    FileUtil::CreateDirectoryWithOwner(cloudCfg1, AID_ROOT, AID_SYSTEM);
    FileUtil::ChangeMode(cloudCfg1, FileUtil::FILE_EXEC_MODE);

    FileUtil::CreateDirectoryWithOwner(cloudCfg0, AID_ROOT, AID_SYSTEM);
    FileUtil::ChangeMode(cloudCfg0, FileUtil::FILE_EXEC_MODE);

    FileUtil::ChangeMode(cloudUpdateConfigDir_, FileUtil::FILE_EXEC_MODE);
    FileUtil::CreateDirectoryWithOwner(workDir_, AID_SYSTEM, AID_SYSTEM);
    FileUtil::ChangeMode(workDir_, FileUtil::FILE_EXEC_MODE);

    curCloudCfgDir_ = (FileUtil::IsFileLatest(cloudCfg0, cloudCfg1) ? cloudCfg0 : cloudCfg1);
    MISIGHT_LOGD("cloudupdateconfg %s, cloudcfg=%s,workdir =%s configDir=%s,", cloudUpdateConfigDir_.c_str(),
        curCloudCfgDir_.c_str(), workDir_.c_str(), configDir_.c_str());
}

bool PluginPlatform::StartPlatform()
{
    // force create dir
    ForceCreateDirs();

    if (IsExist()) {
        return false;
    }

    std::string cfgPath = GetPluginConfigPath();
    Config config(cfgPath);
    if (!config.Parse()) {
        MISIGHT_LOGE("Fail to parse plugin config, exit");
        return false;
    }
    StartDispatchQueue();

    LoadPlugins(config);

    isReady_ = true;
    NotifyPlatformReady();
    return true;
}

void PluginPlatform::StartDispatchQueue()
{
    if (unorderQueue_ == nullptr) {
        unorderQueue_ = new EventDispatcher(Event::ManageType::UNORDERED);
        unorderQueue_->SetName("mi_unorder");
        unorderQueue_->SetPluginContext(this);
        unorderQueue_->SetLooper(GetLooperByName("mi_unorder"));
        unorderQueue_->OnLoad();
    }

    if (sharedWorkLoop_ == nullptr) {
        sharedWorkLoop_ = new EventLoop("plat_shared");
        sharedWorkLoop_->StartLoop();
    }
}

void PluginPlatform::LoadDelayedPlugin(const Config::PluginInfo& pluginInfo)
{
    // only support thread type
    ConstructPlugin(pluginInfo);
    auto& plugin = pluginMap_[pluginInfo.name];
    if (plugin == nullptr) {
        return;
    }

    if (pluginInfo.threadType == "thread") {
        auto workLoop = GetLooperByName(pluginInfo.threadName);
        plugin->SetLooper(workLoop);
    }
    plugin->OnLoad();
}

void PluginPlatform::LoadPlugins(const Config& config)
{
    /*  construct plugin object */
    auto const& pluginInfoList = config.GetPluginInfoList();
    for (auto const& pluginInfo : pluginInfoList) {
        MISIGHT_LOGI("Start to create plugin %s delay:%d", pluginInfo.name.c_str(), pluginInfo.delayLoad);
        if (pluginInfo.delayLoad > 0) {
            auto task = std::bind(&PluginPlatform::LoadDelayedPlugin, this, pluginInfo);
            sharedWorkLoop_->AddTask(task, pluginInfo.delayLoad, false);
        } else {
            ConstructPlugin(pluginInfo);
        }
    }

    /* construct pipelines */
    auto const& pipelineInfoList = config.GetPipelineInfoList();
    for (auto const& pipelineInfo : pipelineInfoList) {
        MISIGHT_LOGI("Start to create pipeline %s", pipelineInfo.name.c_str());
        ConstructPipeline(pipelineInfo);
    }

    /* load plugin and start event source */
    for (auto const& pluginInfo : pluginInfoList) {
        MISIGHT_LOGI("Start to Load plugin %s", pluginInfo.name.c_str());
        LoadPlugin(pluginInfo);
    }
}

void PluginPlatform::ConstructPlugin(const Config::PluginInfo &pluginInfo)
{
    if (pluginInfo.name.empty()) {
        return;
    }

    /* current not support to load dynamic plugin */
    if (!pluginInfo.isStatic)
    {
        MISIGHT_LOGW("plugin %s not supported to load", pluginInfo.name.c_str());
        return;
    }

    sp<Plugin> plugin = PluginFactory::GetInstance().GetPlugin(pluginInfo.name);
    if (plugin == nullptr) {
        MISIGHT_LOGW("plugin %s not loaded, not found in factory", pluginInfo.name.c_str());
        return;
    }

    /* Initialize plugin parameters */
    plugin->SetName(pluginInfo.name);
    plugin->SetPluginContext(this);

    /* call preload, check whether we should release at once */
    if (!plugin->PreToLoad()) {
        return;
    }
    /*  hold the global reference of the plugin */
    pluginMap_[pluginInfo.name] = std::move(plugin);

}

void PluginPlatform::ConstructPipeline(const Config::PipelineInfo &pipelineInfo)
{
    std::vector<sp<Plugin>> pluginList;
    for (auto& pluginName : pipelineInfo.plugins) {
        auto& plugin = pluginMap_[pluginName];
        if (plugin == nullptr) {
            MISIGHT_LOGW("not found plugin %s, skip adding to pipeline %s",
            pluginName.c_str(), pipelineInfo.name.c_str());
            continue;
        }
        pluginList.push_back(plugin);
    }
    sp<Pipeline> pipeline = new Pipeline(pipelineInfo.name, pluginList);
    if (pipeline == nullptr) {
         MISIGHT_LOGW("pipeline %s, plugin %d, nullptr", pipelineInfo.name.c_str(), (int)pluginList.size());
         return;
    }

    pipelines_[pipelineInfo.name] = pipeline;
}

void PluginPlatform::LoadPlugin(const Config::PluginInfo& pluginInfo)
{
    auto& plugin = pluginMap_[pluginInfo.name];
    if (plugin == nullptr)
    {
        MISIGHT_LOGW("plugin %s not constructed", pluginInfo.name.c_str());
        return;
    }

    if (pluginInfo.threadType == "thread") {
        auto workLoop = GetLooperByName(pluginInfo.threadName);
        plugin->SetLooper(workLoop);
    } else {
        MISIGHT_LOGW("%s loop %s not supported, use platform loop", pluginInfo.name.c_str(), pluginInfo.threadType.c_str());
    }

    auto begin = TimeUtil::GetTimestampUS();
    plugin->OnLoad();
    if (pluginInfo.isEventSource) {
        sp<EventSource> eventSource = static_cast<EventSource *>(plugin.get());
        if (eventSource == nullptr) {
            MISIGHT_LOGE("Fail to cast plugin to event source");
            return;
        }
        for (auto& pipelineName : pluginInfo.pipelines) {
            eventSource->AddPipeline(pipelineName, pipelines_[pipelineName]);
        }
        StartEventSource(eventSource);
    }
    auto end = TimeUtil::GetTimestampUS();
    MISIGHT_LOGI("plugin %s isEventSource=%d, loadtime:%" PRIu64, pluginInfo.name.c_str(), pluginInfo.isEventSource, (end - begin));
}

void PluginPlatform::StartEventSource(sp<EventSource> source)
{
    auto looper = source->GetLooper();
    auto& name = source->GetName();
    if (looper == nullptr) {
        MISIGHT_LOGW("No work loop available, start event source[%s] in current thead", name.c_str());
        source->StartEventSource();
    } else {
        MISIGHT_LOGI("Start event source[%s] in thead[%s]", name.c_str(), looper->GetName().c_str());
        auto task = std::bind(&EventSource::StartEventSource, source.get());
        looper->AddTask(task);
    }
}

sp<EventLoop> PluginPlatform::GetLooperByName(const std::string& name)
{
    auto it = workLoopPool_.find(name);
    if (it != workLoopPool_.end()) {
        return it->second;
    }

    sp<EventLoop> pluginLoop = new EventLoop(name);
    if (pluginLoop != nullptr) {
        MISIGHT_LOGI("create event looper name %s", name.c_str());
        pluginLoop->StartLoop();
        workLoopPool_.insert(std::make_pair(name, pluginLoop));
    } else {
         MISIGHT_LOGI("create event looper name %s is null", name.c_str());
    }
    return pluginLoop;
}

const std::string PluginPlatform::GetPluginConfigPath()
{
    std::string cfgPath = configDir_ + configName_;
    std::string gblCfgPath = configDir_ + DEF_CONFIG_GLOBAL_NAME;
    std::string cloudCfgPath = GetCloudConfigDir() + configName_;
    std::string cloudGblCfgPath = configDir_ + DEF_CONFIG_GLOBAL_NAME;

    if (FileUtil::FileExists(cloudGblCfgPath)) {
        gblCfgPath = cloudGblCfgPath;
    }

    // global version, use global config
    // notice: build mk use plugin_config_global renamed plugin_config
    if (region_ != "CN" && region_ != "cn") {
        if (FileUtil::FileExists(gblCfgPath)) {
            return gblCfgPath;
        }
    }
    // cloud config only support plugin_config file
    if (FileUtil::FileExists(cloudCfgPath)) {
        return cloudCfgPath;
    }

    MISIGHT_LOGI("region:%s,support:%s", region_.c_str(), supportDev_.c_str());
    // if cn version, only pre version use plugin_config, stable and dev use plugin_config_global
    if (DevInfo::GetMiuiVersionType(this) != DevInfo::PRE) {
        if (FileUtil::FileExists(gblCfgPath)) {
            return gblCfgPath;
        }
    }
    return cfgPath;
}

void PluginPlatform::PostUnorderedEvent(sp<Plugin> plugin, sp<Event> event)
{
    if (!isReady_) {
        return;
    }
    if (plugin != nullptr) {
        MISIGHT_LOGI("%s send unorder event", plugin->GetName().c_str());
    }
    if (unorderQueue_ != nullptr && event != nullptr) {
        event->processType_ = Event::ManageType::UNORDERED;
        auto workLoop = unorderQueue_->GetLooper();
        if (workLoop == nullptr) {
            return;
        }
        workLoop->AddEvent(unorderQueue_, event);
    }
}

void PluginPlatform::RegisterUnorderedEventListener(sp<EventListener> listener)
{
    if (unorderQueue_ != nullptr) {
        unorderQueue_->RegisterListener(listener);
    }
}

bool PluginPlatform::PostSyncEventToTarget(sp<Plugin> caller, const std::string& callee, sp<Event> event)
{
    if (!isReady_) {
        return false;
    }

    auto it = pluginMap_.find(callee);
    if (it == pluginMap_.end()) {
         return false;
    }

    auto calleePlugin = it->second;
    if (calleePlugin == nullptr) {
        return false;
    }

    auto workLoop = calleePlugin->GetLooper();
    if (workLoop == nullptr) {
        if (caller != nullptr) {
            workLoop = caller->GetLooper();
        }
    }
    bool ret;
    if (workLoop == nullptr) {
        ret = sharedWorkLoop_->AddEventWaitResult(calleePlugin, event);
    } else {
        ret = workLoop->AddEventWaitResult(calleePlugin, event);
    }
    return ret;
}

void PluginPlatform::PostAsyncEventToTarget(sp<Plugin> caller, const std::string& callee, sp<Event> event)
{
    if (!isReady_) {
        return;
    }

    auto it = pluginMap_.find(callee);
    if (it == pluginMap_.end()) {
        return;
    }

    auto calleePlugin = it->second;
    if (calleePlugin == nullptr) {
        return;
    }
    auto workLoop = calleePlugin->GetLooper();
    if (workLoop == nullptr) {
        if (caller != nullptr) {
            workLoop = caller->GetLooper();
        }
    }

    if (workLoop == nullptr) {
        sharedWorkLoop_->AddEvent(calleePlugin, event);
    } else {
        workLoop->AddEvent(calleePlugin, event);
    }
}


sp<EventLoop> PluginPlatform::GetSharedWorkLoop()
{
    return sharedWorkLoop_;
}

void PluginPlatform::InsertEventToPipeline(const std::string& triggerName, sp<Event> event)
{
    auto it = pipelines_.find(triggerName);
    if (it == pipelines_.end()) {
        MISIGHT_LOGW("not find pipeline %s", triggerName.c_str());
        return;
    }
    if (it->second->CanProcessEvent(event)) {
        it->second->ProcessPipeline(event);
    }
}

bool PluginPlatform::IsReady()
{
    return isReady_;
}

std::string PluginPlatform::GetSysProperity(const std::string& key, const std::string& defVal)
{
    auto propPair = sysProperity_.find(key);
    if (propPair != sysProperity_.end()) {
        return propPair->second;
    }
    return SysEnv::GetSysProperity(key, defVal);
}

bool PluginPlatform::SetSysProperity(const std::string& key, const std::string& val, bool update)
{
    update |= SysEnv::SetSysProperity(key, val);
    if (update) {
        sysProperity_[key] = val;
    }
    return update;
}

void PluginPlatform::NotifyPlatformReady()
{
    sp<Event> event = new Event("platform");
    if (event == nullptr) {
        MISIGHT_LOGI("event is null");
        return;
    }
    event->messageType_ = Event::MessageType::PLUGIN_MAINTENANCE;
    event->eventId_ = Event::EventId::PLUGIN_LOADED;
    PostUnorderedEvent(nullptr, event);
}

void PluginPlatform::RegisterPipelineListener(const std::string& triggerName, sp<Plugin> plugin)
{
    auto it = pipelines_.find(triggerName);
    if (it == pipelines_.end()) {
        MISIGHT_LOGW("not find pipeline %s", triggerName.c_str());
        return;
    }
    it->second->RegisterPluginListener(plugin);
}

void PluginPlatform::SetCloudConfigFolder(const std::string& cloudCfgFolder)
{
    if (cloudCfgFolder != DEF_CLOUD_CONFIG_NAME0 && cloudCfgFolder != DEF_CLOUD_CONFIG_NAME1) {
        return;
    }
    curCloudCfgDir_ = cloudUpdateConfigDir_ + cloudCfgFolder + "/";
}

const std::string PluginPlatform::GetConfigDir(const std::string configName)
{
    std::string filePath = GetConfigDir() + configName;
    std::string cloudFilePath = GetCloudConfigDir() + configName;
    MISIGHT_LOGD("configName=%s, GetConfigDir=%s, %s", configName.c_str(), GetConfigDir().c_str(), GetCloudConfigDir().c_str());
    return (FileUtil::IsFileLatest(filePath, cloudFilePath) ? GetConfigDir() : GetCloudConfigDir());
}
} // namespace MiSight
} // namespace android
