/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: log manager plugin implement file
 *
 */

#include "run_log_manager.h"

#include "dev_info.h"
#include "event_quota_xml_parser.h"
#include "event_util.h"
#include "json_util.h"
#include "plugin_factory.h"
#include "string_util.h"
#include "time_util.h"

namespace android {
namespace MiSight {

REGISTER_PLUGIN(RunLogManager);

RunLogManager::RunLogManager() : staticSpace_(180) // 180: after start 3minutes
{}

RunLogManager::~RunLogManager()
{}

void RunLogManager::OnLoad()
{
    // set plugin info
    SetVersion("RunLogManager-1.0");
    SetName("RunLogManager");

    // start task
    if (GetLooper() == nullptr) {
        MISIGHT_LOGE("run log manager looper not set");
        return;
    }
    auto task = std::bind(&RunLogManager::LogFilterStatics, this);
    GetLooper()->AddTask(task, staticSpace_);
}

void RunLogManager::OnUnload()
{
}

EVENT_RET RunLogManager::OnEvent(sp<Event> &event)
{
    // noues
    return ON_SUCCESS;
}

void RunLogManager::LogFilterStatics()
{
    auto context = GetPluginContext();
    int filterFlag = StringUtil::ConvertInt(context->GetSysProperity("persist.sys.log_filter_flag", "-1"));
    int gapSec = StringUtil::ConvertInt(context->GetSysProperity("persist.sys.log_time_gap_sec", "-1"));
    int maxLine = StringUtil::ConvertInt(context->GetSysProperity("persist.sys.log_max_line", "-1"));

    if (filterFlag != -1 && gapSec != -1 && maxLine != -1) {
        Json::Value root;
        root["FilterSecond"] = gapSec;
        root["FilterLine"] = maxLine;
        if (GetLogFilterDetail(root) && root.isMember("Detail")) {
            sp<Event> event = new Event("LogFilter");
            if (event == nullptr) {
                MISIGHT_LOGI("event is null");
                return;
            }
            event->eventId_ = 908002001;
            event->happenTime_ = std::time(nullptr);
            event->messageType_ = Event::MessageType::STATISTICS_EVENT;
            event->SetValue(EventUtil::DEV_SAVE_EVENT, JsonUtil::ConvertJsonToStr(root));
            context->InsertEventToPipeline("dftFaultEvent", event);
        }
    }

    sp<EventQuotaXmlParser> xmlParser = EventQuotaXmlParser::getInstance();
    std::string cfgDir = context->GetConfigDir(xmlParser->GetXmlFileName());
    std::string version = DevInfo::GetMiuiVersion(context);
    EventQuotaXmlParser::SummaryQuota quota = xmlParser->GetSummaryQuota(cfgDir, version);
    staticSpace_ = (quota.logStatic > 0) ? quota.logStatic : 8 * MINUTE_SECONDS; // default 8 hour;
    auto task = std::bind(&RunLogManager::LogFilterStatics, this);
    GetLooper()->AddTask(task, staticSpace_ * MINUTE_SECONDS);
}

bool RunLogManager::GetLogFilterDetail(Json::Value& root)
{
    char buffer[256]; // 256: cmdline size
    FILE* fp = popen("logcat -S", "r");

    if (fp == nullptr) {
        MISIGHT_LOGW("open logcat failed");
        return false;
    }

    const int nameLen = 128;
    bool find = false;
    int arrLen = 0;
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        if (!find) {
            if(strstr(buffer, "Log Filter Top PIDs")) {
                find = true;
            }
            continue;
        }

        int32_t pid = 0;
        int32_t uid = 0;
        int32_t total = 0;
        int32_t count = 0;
        int32_t maxCnt = 0;
        int32_t time = 0;
        char name[nameLen] = {}; // 128: processname len

        // "logcat -S|grep \"Log Filter Top\" -A100"
        // Log Filter Top PIDs: (Total size:239)
        // PID/UID   COMMAND LINE                     Total Count Max Time
        // 7051/1000  com.miui.powerkeeper           6181 3 5838 1663332810
        int32_t ret = sscanf(buffer, "%d/%d %s %d %d %d %d", &pid, &uid, name, &total, &count, &maxCnt, &time);
        if (ret != 7) {
            continue;
        }
        name[nameLen-1]= '\0';
        Json::Value member;
        member["PID"] = pid;
        member["UID"] = uid;
        member["ProcessName"] = name;
        member["TotalFilterLog"] = total;
        member["TotalFilterCnt"] = count;
        member["MaxFilterCnt"] = maxCnt;
        member["MaxHappenTime"] = TimeUtil::GetTimeDateStr(time, "%Y-%m-%d %H:%M:%S");
        root["Detail"].append(member);
        arrLen++;
        if (arrLen == 100) {
            break;
        }
    }
    pclose(fp);
    return find;
}
}
}

