/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight service implements file
 *
 */

#include "misight_service.h"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>


#include "dev_info.h"
#include "misight_native_service.h"
#include "public_util.h"
#include "time_util.h"

namespace android {
namespace MiSight {
const std::string SERVICE_NAME = "misight";

MiSightService::MiSightService(PluginPlatform *platform)
    : platform_(platform)
{}

MiSightService::~MiSightService()
{}

void MiSightService::StartService()
{
    ProcessState::self()->setThreadPoolMaxThreadCount(4);
    sp<ProcessState> ps(ProcessState::self());
    ps->startThreadPool();
    ps->giveThreadPoolName();
    sp<IServiceManager> sm(defaultServiceManager());
    std::string check = platform_->GetSysProperity("persist.sys.misight.safecheck", "1");
    bool safeCheck = true;
    if (check == "0") {
        safeCheck = false;
    }
    sm->addService(String16(SERVICE_NAME.c_str()), new MiSightNativeService(this, safeCheck));
    IPCThreadState::self()->joinThreadPool();
}

int MiSightService::NotifyPackLog()
{
    sp<Event> event = new Event(SERVICE_NAME);
    if (event == nullptr) {
        MISIGHT_LOGI("event is null");
        return -1;
    }
    event->messageType_ = Event::MessageType::PLUGIN_MAINTENANCE;
    event->eventId_ = EVENT_SERVICE_UPLOAD_ID;
    if (platform_->PostSyncEventToTarget(nullptr, "EventLogProcessor", event)) {
        return event->GetIntValue(EVENT_SERVICE_UPLOAD_RET);
    }
    return 0;
}

void MiSightService::SetRunMode(const std::string& runName, const std::string& filePath)
{
    DevInfo::SetRunMode(platform_, runName, filePath);
}

void MiSightService::NotifyUploadSwitch(bool isOn)
{
    sp<Event> event = new Event(SERVICE_NAME);
    if (event == nullptr) {
        MISIGHT_LOGI("event is null");
        return;
    }
    event->messageType_ = Event::MessageType::PLUGIN_MAINTENANCE;
    event->eventId_ = EVENT_SERVICE_UE_UPDATE_ID;
    event->SetValue(EVENT_SERVICE_UE_UPDATE, isOn);
    platform_->PostSyncEventToTarget(nullptr, "EventLogProcessor", event);
}

void MiSightService::NotifyConfigUpdate(const std::string& folderName)
{
    platform_->SetCloudConfigFolder(folderName);
    sp<Event> event = new Event(SERVICE_NAME);
    if (event == nullptr) {
        MISIGHT_LOGI("event is null");
        return;
    }
    event->messageType_ = Event::MessageType::PLUGIN_MAINTENANCE;
    event->eventId_ = EVENT_SERVICE_CONFIG_UPDATE_ID;
    platform_->PostUnorderedEvent(nullptr, event);
}

void MiSightService::NotifyUserActivated()
{
    sp<Event> event = new Event(SERVICE_NAME);
    if (event == nullptr) {
        MISIGHT_LOGI("event is null");
        return;
    }
    event->messageType_ = Event::MessageType::PLUGIN_MAINTENANCE;
    event->eventId_ = EVENT_SERVICE_USER_ACTIVATE_ID;
    event->happenTime_ = TimeUtil::GetTimestampSecond();
    platform_->PostAsyncEventToTarget(nullptr, "EventStorageManager", event);
}

void MiSightService::Dump(int fd, const std::vector<std::string>& args)
{
    if (args.size() == 0) {
        return DumpPluginSummaryInfo(fd);
    }

    if (args.size() >= 2 && args[0] == "plugin") {
        std::vector<std::string> cmds(args.begin()+2, args.end());
        return DumpPluginInfo(fd, args[1], cmds);
    }
}

void MiSightService::DumpPluginSummaryInfo(int fd)
{
    auto& pluginMaps = platform_->GetPluginMap();
    dprintf(fd, "Load plugin %u:\n", (uint32_t)pluginMaps.size());
    for (auto& plugin : pluginMaps) {
        auto& pluginName = plugin.first;
        if (plugin.second != nullptr) {
            dprintf(fd, "PluginName:%s Version:%s Thread:%s\n", pluginName.c_str(),
                (plugin.second->GetVersion().c_str()),
                ((plugin.second->GetLooper() == nullptr) ? "Null" : plugin.second->GetLooper()->GetName().c_str()));
        } else {
            dprintf(fd, "PluginName:%s not Loaded\n", pluginName.c_str());
        }
    }
    auto& pipelineMaps = platform_->GetPipelineMap();
    dprintf(fd, "Load pipeline %u:\n", (uint32_t)pipelineMaps.size());
    for (auto& pipeline : pipelineMaps) {
        auto& pipelineName = pipeline.first;
        if (pipeline.second == nullptr) {
            continue;
        }
        dprintf(fd, "pipelineName:%s\n", pipelineName.c_str());
    }
}

void MiSightService::DumpPluginInfo(int fd, const std::string& pluginName, const std::vector<std::string>& args)
{
    auto& pluginMaps = platform_->GetPluginMap();
    for (auto& plugin : pluginMaps) {
        if (pluginName != plugin.first) {
            continue;
        }
        if (plugin.second != nullptr) {
            return plugin.second->Dump(fd, args);
        }
    }
    dprintf(fd, "plugin %s not exist", pluginName.c_str());
}
} // namespace MiSight
} // namespace android

