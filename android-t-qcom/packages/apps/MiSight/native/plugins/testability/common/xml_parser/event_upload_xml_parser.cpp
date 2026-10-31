/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event upload xml config file parser head file
 *
 */
#include "event_upload_xml_parser.h"

#include <errno.h>
#include <regex>


#include "file_util.h"
#include "log.h"
#include "string_util.h"


namespace android {
namespace MiSight {

const std::string EVENT_UPLOAD_XML = "event_upload.xml";

void LogCollectInfo::SetAttributes(std::map<std::string, std::string> attributes)
{
    attrs_ = std::move(attributes);
}

std::string LogCollectInfo::GetValue(const std::string& name)
{
    auto it = attrs_.find(name);
    if (it == attrs_.end()) {
        return "";
    }
    return it->second;
}

int32_t LogCollectInfo::GetOptionLatest()
{
    std::string option = GetValue(EventUploadXML::OPTION_ATTR_NAME);
    if (option == "") {
        return 0;
    }
    std::smatch result;
    std::regex re(EventUploadXML::OPTION_ATTR_VAL_LATEST + "=(\\d+)");
    int32_t count = 0;
    if (std::regex_search(option, result, re)) {
        count = atoi(std::string(result[1]).c_str()); // const match 1
    }
    return count;
}

bool LogCollectInfo::IsDeleteLog()
{
    if (isDelete_) {
        return isDelete_;
    }

    std::string option = GetValue(EventUploadXML::OPTION_ATTR_NAME);
    if (option == "") {
        return isDelete_;
    }
    std::smatch result;
    std::regex re(EventUploadXML::OPTION_ATTR_VAL_DEL);
    if (std::regex_search(option, result, re)) {
        isDelete_ = true;
    }
    return isDelete_;
}

void LogCollectInfo::DeleteLogFile()
{
    if (logFiles_.size() == 0) {
        return;
    }

    for (auto& filePath : logFiles_) {
        FileUtil::DeleteFile(filePath);
    }
}

EventUploadXmlParser::EventUploadXmlParser()
    :XmlParser(EVENT_UPLOAD_XML), curEventId_(0), workDir_(""), expLocal_(""), curTemplate_("")
{

}


EventUploadXmlParser::~EventUploadXmlParser()
{
    Clear();
    templates_.clear();

}

sp<EventUploadXmlParser> EventUploadXmlParser::getInstance()
{
    static sp<EventUploadXmlParser> intance_ = new EventUploadXmlParser;
    return intance_;
}


bool EventUploadXmlParser::IsMatch(const std::string& localName)
{
    if (localName != expLocal_) {
        return false;
    }

    if (expLocal_ == EventUploadXML::TEMPLATE_GROUP_NAME) {
        SetStart();
    }

    return true;
}


void EventUploadXmlParser::ProcessEvent(const std::string& localName, std::map<std::string, std::string> attributes)
{
    // set start if event id in [min,max]
    if (localName == EventUploadXML::EVENT_LOCAL_NAME) {
        int minId = StringUtil::ConvertInt(attributes[EventUploadXML::EVENT_ATTR_MIN]);
        int maxId = StringUtil::ConvertInt(attributes[EventUploadXML::EVENT_ATTR_MAX]);\
        MISIGHT_LOGD("localName minId %d check maxId=%d", minId, maxId);
        if (curEventId_ >= minId && curEventId_ <= maxId) {
            MISIGHT_LOGD("localName %s set start expLocal_=%s", localName.c_str(), expLocal_.c_str());
            SetStart();
        }
        return;
    } else if (IsStart()) {
        ProcessLogCollect(localName, attributes);
        return;
    }
}

LogCollectType EventUploadXmlParser::GetLogCollectType(const std::string& localName)
{
    LogCollectType logType = LOG_COLLECT_UNKNOWN;
    if (localName == EventUploadXML::DYNAMIC_PATH_LOCAL_NAME)
    {
        logType = LOG_COLLECT_DYNAMIC;
    }

    if (localName == EventUploadXML::PATH_LOCAL_NAME)
    {
        logType = LOG_COLLECT_STATIC;
    }

    if (localName == EventUploadXML::CMD_LOCAL_NAME)
    {
        logType = LOG_COLLECT_CMD;
    }
    return logType;
}


void EventUploadXmlParser::ProcessLogCollect(const std::string& localName, std::map<std::string, std::string> attributes)
{
    if (localName == EventUploadXML::IMPORT_LOCAL_NAME)
    {
        MISIGHT_LOGD("add template %s, %d", attributes[EventUploadXML::IMPORT_TEMP_ATTR_NAME].c_str(),
            (int)templates_[attributes[EventUploadXML::IMPORT_TEMP_ATTR_NAME]].size());
        curLogInfos_.merge(templates_[attributes[EventUploadXML::IMPORT_TEMP_ATTR_NAME]]);
        return;
    }

    LogCollectType logType = GetLogCollectType(localName);
    sp<LogCollectInfo> logCollect = new LogCollectInfo(logType);
    logCollect->SetAttributes(attributes);
    curLogInfos_.emplace_back(logCollect);
}

void EventUploadXmlParser::ProcessTemplate(const std::string& localName, std::map<std::string, std::string> attributes)
{
    if (localName == EventUploadXML::TEMPLATE_GROUP_NAME) {
        return;
    }
    if (localName == EventUploadXML::TEMPLATE_LOCAL_NAME) {
        curTemplate_ = attributes[EventUploadXML::TEMPLATE_ATTR_NAME];
        return;
    }

    ProcessLogCollect(localName, attributes);
    return;
}

void EventUploadXmlParser::OnStartElement(const std::string& localName, std::map<std::string, std::string> attributes)
{
    if (expLocal_ == EventUploadXML::TEMPLATE_GROUP_NAME) {
        ProcessTemplate(localName, attributes);
    }

    if (expLocal_ == EventUploadXML::EVENT_LOCAL_NAME) {
        ProcessEvent(localName, attributes);
    }
}


void EventUploadXmlParser::OnEndElement(const std::string& localName)
{
    if (expLocal_ == EventUploadXML::TEMPLATE_GROUP_NAME) {
        if (localName == EventUploadXML::TEMPLATE_LOCAL_NAME) {
            templates_[curTemplate_] = std::move(curLogInfos_);
            curTemplate_ = "";
            curLogInfos_.clear();
            return;
        }
    }

    if (expLocal_ == localName) {
        SetFinish();
    }
}

std::list<sp<LogCollectInfo>> EventUploadXmlParser::GetEventLogCollectInfo(const std::string& workDir, int32_t eventId)
{
    std::lock_guard<std::mutex> lock(parserMutex_);
    std::string xmlPath = workDir + EVENT_UPLOAD_XML;
    if (workDir_ == "" || workDir_ != workDir) {
        Clear();
        templates_.clear();
        expLocal_ = EventUploadXML::TEMPLATE_GROUP_NAME;
        if (ParserXmlFile(xmlPath) != 0) {
            MISIGHT_LOGW("upload %s failed", xmlPath.c_str());
            return curLogInfos_;
        }
        workDir_ = workDir;
    }
    Clear();
    expLocal_ = EventUploadXML::EVENT_LOCAL_NAME;
    curEventId_ = eventId;
    ParserXmlFile(xmlPath);
    return curLogInfos_;
}

void EventUploadXmlParser::Clear()
{
    curEventId_ = 0;
    expLocal_ = "";
    curTemplate_ = "";
    curLogInfos_.clear();
}

}
}
