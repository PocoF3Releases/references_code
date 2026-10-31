/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event feature log collector plugin head file
 *
 */
#ifndef MISIGH_PLUGIN_TESTABILITY_EVENT_LOG_PROCESSOR_PLUGIN_H
#define MISIGH_PLUGIN_TESTABILITY_EVENT_LOG_PROCESSOR_PLUGIN_H

#include <list>
#include <string>
#include <vector>

#include <utils/StrongPointer.h>

#include "event.h"
#include "event_upload_xml_parser.h"
#include "event_util.h"
#include "plugin.h"

namespace android {
namespace MiSight {

class EventLogProcessor : public Plugin, public EventListener {
public:
    EventLogProcessor();
    ~EventLogProcessor() {}

    void OnLoad() override;
    void OnUnload() override;
    EVENT_RET OnEvent(sp<Event> &event) override;
    std::vector<std::string> GetFileInDirByPattern(const std::string& dirPath, const std::string& pattern,
        bool timeSort, bool addDir, int maxCnt = -1);
    void OnUnorderedEvent(sp<Event> event);
    std::string GetListenerName() {
        return name_;
    }

private:
    bool CanUpload();
    bool CanUploadRatio(int32_t level);
    //std::string GetSnMd5Base64();
    std::string GetLogPackName(sp<Event> event);
    bool IsPrivacycompliance(sp<LogCollectInfo> logCollectInfo);
    void AddStaticLog(sp<LogCollectInfo>& logCollectInfo, std::vector<std::string>& logPaths);
    void AddDynamicLog(sp<LogCollectInfo>& logCollectInfo, std::vector<std::string>& logPaths);

    void AddCmdLog(sp<Event> event, sp<LogCollectInfo>& logCollectInfo, std::vector<std::string>& logPaths);
    std::vector<std::string> GetSrcLogFileList(sp<Event> event, std::list<sp<LogCollectInfo>>& logInfoList);
    bool PackLogToZip(sp<Event> event, std::vector<std::string> logPaths);
    void DeleteLogFile(std::list<sp<LogCollectInfo>>& logInfoList);
    void ProcessEventLog(sp<Event> event);
    void ProcessUploadingLog(sp<Event> &event);
    void ProcessUploadingEvent(sp<Event> &event);
    void InitRecordLogCnt();
    void InitLogFolder(const std::string& eventDir, const std::string& dirName);
    void CheckAndClearLogFolder(sp<Event> event);
    void GetLogFiles(const std::string& eventDir, const std::string& dirName,
        std::vector<std::string>& zipFiles);
    std::string GetEventDirPath(sp<Event> event);
    bool IsNotExpired();

    std::string miuiVersion_;
};
}
}
#endif

