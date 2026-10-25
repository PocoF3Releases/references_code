/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event store manager plugin implement file
 *
 */

#include "event_storage_manager.h"

#include "cmd_processor.h"
#include "dev_info.h"
#include "event_quota_xml_parser.h"
#include "event_upload_xml_parser.h"
#include "event_util.h"
#include "file_util.h"
#include "log.h"
#include "plugin_factory.h"
#include "privacy_xml_parser.h"
#include "public_util.h"
#include "string_util.h"

namespace android {
namespace MiSight {
REGISTER_PLUGIN(EventStorageManager);

const std::string DATABASE_NAME = "event.db";
const uint32_t MAX_DB_SIZE = 1024 * 1024 * 200; // 200M

EventStorageManager::EventStorageManager()
    : eventDb_(new EventDatabaseHelper(std::time(nullptr)))
{

}

EventStorageManager::~EventStorageManager()
{
    eventDb_ = nullptr;
}

int EventStorageManager::GetStoreProperity(const std::string& propName, int defVal)
{
    auto context = GetPluginContext();
    std::string propStr = context->GetSysProperity(propName, "");
    int64_t sizeVal = 0;
    if (propStr != "") {
        sizeVal = StringUtil::ConvertInt(propStr);
    }
    if (sizeVal <= 0) {
        MISIGHT_LOGD("prop %s use default value", propName.c_str());
        sizeVal = defVal;
    }
    return sizeVal;
}
void EventStorageManager::OnLoad()
{
    // set plugin info
    SetVersion("EventStoreManger-1.0");
    SetName("EventStoreManager");
    auto context = GetPluginContext();
    eventDb_->InitDatabase(context, DATABASE_NAME, DevInfo::GetMiuiVersion(context));

     // start task
    if (GetLooper() == nullptr) {
        MISIGHT_LOGE("storage manager looper not set");
    return;
    }
    auto task = std::bind(&EventStorageManager::CheckEventStore, this);
    GetLooper()->AddTask(task);
}

void EventStorageManager::OnUnload()
{

}

EVENT_RET EventStorageManager::OnEvent(sp<Event> &event)
{
    MISIGHT_LOGD("EventStorageManager event=%u,%d", event->eventId_, event->messageType_);
    if (event->messageType_ == Event::MessageType::PLUGIN_MAINTENANCE) {
        if (event->eventId_ == EVENT_SERVICE_USER_ACTIVATE_ID) {
            SaveUserActivate(event);
        }
    } else if (event->messageType_ == Event::MessageType::RAW_EVENT) {
        // raw event, insert event
        if (EventUtil::RAW_EVENT_MIN <= event->eventId_ && event->eventId_ <= EventUtil::RAW_EVENT_MAX) {
            eventDb_->SaveRawEvent(event);
        }
    } else if (EventUtil::FAULT_EVENT_MIN <= event->eventId_ && event->eventId_ <= EventUtil::FAULT_EVENT_MAX) {
        // fault event,check quota and save event
        eventDb_->ProcessFaultEvent(event);
    } else {
        return ON_FAILURE;
    }
    return ON_SUCCESS;
}

void EventStorageManager::CheckEventStore()
{
    auto platform = GetPluginContext();
    std::string dbPath = platform->GetWorkDir() + DATABASE_NAME;
    uint32_t dbMaxSize = GetStoreProperity("misight.store.checksize", MAX_DB_SIZE);

    // check db size, over max, drop event_info and raw_event
    if (FileUtil::GetFileSize(dbPath) >= dbMaxSize) {
        eventDb_->AbridgeDbSize();
    }
    int64_t timePeriod = GetStoreProperity("misight.store.checktime", DAY_SECONDS);
    auto task = std::bind(&EventStorageManager::CheckEventStore, this);
    GetLooper()->AddTask(task, timePeriod);
    return;
}

void EventStorageManager::SaveUserActivate(sp<Event> &event)
{
    eventDb_->SaveUserActivateTime(event);
}

void EventStorageManager::Dump(int fd, const std::vector<std::string>& cmds)
{
    auto context = GetPluginContext();
    // dump quota
    int size = cmds.size();
    if (size < 1) {
        return;
    }
    std::string cmd = cmds[0];
    if (cmds[0] == "query") {
        eventDb_->DumpQueryDb(fd, cmds);
        return;
    }

    if (cmds[0] == "update") {
        std::string check = context->GetSysProperity("persist.sys.misight.safecheck", "1");
        if (check != "0") {
            dprintf(fd, "not support cmd\n");
            return;
        }
        eventDb_->DumpUpdateDb(fd, cmds);
        return;
    }

    // dump quota
    if (cmds[0] == "quota" && size >= 2) {
        int eventId = StringUtil::ConvertInt(cmds[1]);
        if (eventId <= 0) {
            dprintf(fd, "EventId %s invalid\n", cmds[1].c_str());
            return;
        }

        sp<EventQuotaXmlParser> xmlParser = EventQuotaXmlParser::getInstance();
        std::string cfgDir = context->GetConfigDir(xmlParser->GetXmlFileName());
        std::string version = DevInfo::GetMiuiVersion(context);
        EventQuotaXmlParser::DomainQuota eventQuota = xmlParser->GetEventQuota(cfgDir, eventId, version);
        dprintf(fd, "EventId %d quota:\n    dailyCnt = %d\n    logCnt = %d\n    logSpace = %d\n",
            eventId, eventQuota.dailyCnt, eventQuota.maxLogCnt, eventQuota.logSpace);
        return;
    }
}
}
}
