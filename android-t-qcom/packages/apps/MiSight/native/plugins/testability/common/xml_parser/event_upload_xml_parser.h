/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event upload xml config file parser head file
 *
 */
#ifndef MISIGH_TESTABILITY_EVENT_UPLOAD_XML_PASER_DEFINE_H
#define MISIGH_TESTABILITY_EVENT_UPLOAD_XML_PASER_DEFINE_H


#include <list>
#include <map>
#include <mutex>
#include <string>
#include <vector>


#include <utils/RefBase.h>
#include <utils/StrongPointer.h>


#include "xml_parser.h"

namespace android {
namespace MiSight {
namespace EventUploadXML {
// Template
const std::string TEMPLATE_GROUP_NAME = "TemplateGroup";
const std::string TEMPLATE_LOCAL_NAME = "Template";
const std::string TEMPLATE_ATTR_NAME = "name";

// Event
const std::string EVENT_LOCAL_NAME = "Event";
const std::string EVENT_ATTR_MIN = "min";
const std::string EVENT_ATTR_MAX = "max";

// Log collect
const std::string DYNAMIC_PATH_LOCAL_NAME = "DynamicPath";
const std::string PATH_LOCAL_NAME = "Path";
const std::string CMD_LOCAL_NAME = "Cmd";
const std::string IMPORT_LOCAL_NAME = "Import";
// import attr
const std::string IMPORT_TEMP_ATTR_NAME = "template";


// log attr
const std::string FAULT_LEVEL_ATTR_NAME = "FaultLevel";
const std::string PRI_LEVEL_ATTR_NAME = "PrivacyLevel";
const std::string PATH_ATTR_NAME = "path";
const std::string DIR_LEVEL_ATTR_NAME = "dir";
const std::string PATTERN_ATTR_NAME = "pattern";
const std::string OPTION_ATTR_NAME = "option";
const std::string OPTION_ATTR_VAL_DEL = "delete";
const std::string OPTION_ATTR_VAL_LATEST = "latest";
const std::string RUN_ATTR_NAME = "run";
const std::string OUT_ATTR_NAME = "output";
};

enum LogCollectType {
    LOG_COLLECT_DYNAMIC,
    LOG_COLLECT_STATIC,
    LOG_COLLECT_CMD,
    LOG_COLLECT_UNKNOWN
};
class LogCollectInfo : public RefBase {
public:
    explicit LogCollectInfo(LogCollectType logType)
        : logType_(logType), isDelete_(false)
    {}
    ~LogCollectInfo()
    {
        logFiles_.clear();
        attrs_.clear();
    }

    bool operator<(const LogCollectInfo &obj) const
    {
        return (this->logType_ > obj.logType_);
    }

    void SetAttributes(std::map<std::string, std::string> attributes);
    void SetValue(const std::string& name, const std::string& val);
    std::string GetValue(const std::string& name);
    void SetDelete(bool delFile) {
        isDelete_ = delFile;
    }
    int32_t GetOptionLatest();
    LogCollectType GetLogCollectType() {
        return logType_;
    }

    void SetLogFile(std::vector<std::string> logFile) {
        logFiles_.clear();
        logFiles_ = std::move(logFile);
    }
    bool IsDeleteLog();
    void DeleteLogFile();
private:

    LogCollectType logType_;
    bool isDelete_;
    std::vector<std::string> logFiles_;
    std::map<std::string, std::string> attrs_;
};

class EventUploadXmlParser : public XmlParser {

protected:
    EventUploadXmlParser();
    ~EventUploadXmlParser();
public:
    bool IsMatch(const std::string& localName) override;
    void OnStartElement(const std::string& localName, std::map<std::string, std::string> attributes) override;
    void OnEndElement(const std::string& localName) override;
    std::list<sp<LogCollectInfo>> GetEventLogCollectInfo(const std::string& workDir, int32_t eventId);
    static sp<EventUploadXmlParser> getInstance();


private:
    void ProcessEvent(const std::string& localName, std::map<std::string, std::string> attributes);
    LogCollectType GetLogCollectType(const std::string& localName);
    void ProcessTemplate(const std::string& localName, std::map<std::string, std::string> attributes);
    void ProcessLogCollect(const std::string& localName, std::map<std::string, std::string> attributes);
    void Clear();

    int32_t curEventId_;
    std::string workDir_;
    std::string expLocal_;
    std::string curTemplate_;
    std::list<sp<LogCollectInfo>> curLogInfos_;
    std::map<std::string, std::list<sp<LogCollectInfo>>> templates_;
    std::mutex parserMutex_;
};
}
}
#endif

