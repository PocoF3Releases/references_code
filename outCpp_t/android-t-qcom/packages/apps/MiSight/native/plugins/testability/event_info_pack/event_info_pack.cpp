/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event infp storage and pack plugin implements file
 *
 */


#include "event_info_pack.h"

#include "dev_info.h"
#include "event_util.h"
#include "file_util.h"
#include "json_util.h"
#include "log.h"
#include "plugin_factory.h"
#include "string_util.h"


namespace android {
namespace MiSight {

REGISTER_PLUGIN(EventInfoPack);

constexpr int32_t EVENT_INFO_MAX_SIZE = 50 * 1024;

EventInfoPack::EventInfoPack()
{

}

EventInfoPack::~EventInfoPack()
{}

void EventInfoPack::OnLoad()
{
    // set plugin info
    SetVersion("EventInfoPack-1.0");
    SetName("EventInfoPack");
    MISIGHT_LOGI("EventInfo OnLoad ok");

}

void EventInfoPack::OnUnload()
{
}


EVENT_RET EventInfoPack::OnEvent(sp<Event> &event)
{
    MISIGHT_LOGD("EventInfoPack event %d", event->eventId_);
    if (EventUtil::FAULT_EVENT_MIN <= event->eventId_ && event->eventId_ <= EventUtil::FAULT_EVENT_MAX) {
        if (event->GetValue(EventUtil::EVENT_DROP_REASON) == "") {
             // fault event drop reason must null
             ProcessEventInfo(event);
        }
    }
    return ON_SUCCESS;

}

std::string EventInfoPack::GetEventFilePath(sp<Event> event, bool &truncate)
{
    auto context = GetPluginContext();
    std::string eventDir = context->GetWorkDir() + EventUtil::FAULT_EVENT_DEFAULT_DIR;
    truncate = false;

    //bool ueSwitchOn = true;
    if (DevInfo::GetUploadSwitch(context) != "on") {
        eventDir += "/ue_off";
        //ueSwitchOn = false;
    }

    if (!FileUtil::FileExists(eventDir)) {
        FileUtil::CreateDirectory(eventDir, FileUtil::FILE_EXEC_MODE);
    }
    eventDir = eventDir + "/eventinfo." + std::to_string(event->eventId_ / EventUtil::FAULT_EVENT_SIX_RANGE);
    //if (!ueSwitchOn) {
        struct stat fileInfo{};
        stat(eventDir.c_str(), &fileInfo);
        if (fileInfo.st_size >= EVENT_INFO_MAX_SIZE) {
            std::string eventDir0 = eventDir + ".0";
            struct stat fileInfo0{};
            stat(eventDir0.c_str(), &fileInfo0);
            if (FileUtil::FileExists(eventDir0) && fileInfo0.st_size >= EVENT_INFO_MAX_SIZE) {
                if (fileInfo.st_mtime > fileInfo0.st_mtime) {
                    eventDir = eventDir0;
                }
                truncate = true;
            } else {
                eventDir = eventDir0;
            }
        }
    //}

    return eventDir;
}

void EventInfoPack::ProcessEventInfo(sp<Event> &event)
{
    Json::Value root;
    Json::Value eventInfo;

    root["event_id"] = event->eventId_;
    root["event_time"] = TimeUtil::GetTimeDateStr(event->happenTime_, "%Y%m%d%H%M%S");
    root["event_log"] = event->GetValue(EventUtil::EVENT_LOG_ZIP_NAME);

    std::string runMode = "";
    std::string filterPath = "";
    DevInfo::GetRunMode(GetPluginContext(), runMode, filterPath);
    root["user_type"] = runMode;

    std::string eventStr = event->GetValue(EventUtil::DEV_SAVE_EVENT);
    if (eventStr != "") {
       if (JsonUtil::ConvertStrToJson(eventStr, eventInfo)) {
           root["event_info"] = eventInfo;
       } else {
           MISIGHT_LOGE("event %d %s convert to json failed", event->eventId_, eventStr.c_str());
       }
    }
    bool truncate = false;
    std::string eventPath = GetEventFilePath(event, truncate);
    std::string jsonStr = JsonUtil::ConvertJsonToStr(root) + "\n";
    bool changeMode = FileUtil::FileExists(eventPath);
    if (!FileUtil::SaveStringToFile(eventPath, jsonStr, truncate)) {
        MISIGHT_LOGE("write event %d info failed", event->eventId_);
    }
    // first file need changemode for misight service
    if (!changeMode) {
        FileUtil::ChangeMode(eventPath, FileUtil::FILE_EXEC_MODE);
    }

}
}
}

