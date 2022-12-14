/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event filter xml config file parser implements file
 *
 */

#include "filter_xml_parser.h"

#include "log.h"
#include "string_util.h"
namespace android {
namespace MiSight {
const std::string TESTNAME_LOCAL_NAME = "TestName-";
const std::string WHITELIST_LOCAL_NAME = "WhiteList";
const std::string BLACKLIST_LOCAL_NAME = "BlackList";
const std::string EVENT_LOCAL_NAME = "Event";
const std::string EVENT_ATTR_MIN = "min";
const std::string EVENT_ATTR_MAX = "max";
const std::string EVENT_FILTER_NAME = "FilterName";
const std::string EVENT_FILTER_PARA_ATTR_NAME = "paraName";
const std::string EVENT_FILTER_VALUE_ATTR_NAME = "value";

EventFilterInfo::EventFilterInfo(int32_t eventId)
    :eventId_(eventId), hasWhite_(false), hasBlack_(false)
{}

EventFilterInfo::~EventFilterInfo()
{
    whiteList_.clear();
    blackList_.clear();
}

void EventFilterInfo::AddWhiteList(const std::string& name, const std::string& value)
{
    if (name == "" || value == "") {
        return;
    }

    auto find = whiteList_.find(name);
    if (find == whiteList_.end()) {
        whiteList_[name] = value;
    } else {
        whiteList_[name] = whiteList_[name] + "," + value;
    }
}

void EventFilterInfo::AddBlackList(const std::string& name, const std::string& value)
{
    if (name == "" || value == "") {
        return;
    }

    auto find = blackList_.find(name);
    if (find == blackList_.end()) {
        blackList_[name] = value;
    } else {
        blackList_[name] = blackList_[name] + "," + value;
    }
}

FilterXmlParser::FilterXmlParser()
    : XmlParser(""), eventMatch_(false), runMode_(""), filePath_(""), filterList_(""), eventFilter_(nullptr)
{}

FilterXmlParser::~FilterXmlParser()
{
    eventFilter_ = nullptr;
}

bool FilterXmlParser::IsMatch(const std::string& localName)
{
    if (localName == runMode_) {
        SetStart();
        return true;
    }
    return false;
}

void FilterXmlParser::OnStartElement(const std::string& localName, std::map<std::string, std::string> attributes)
{
    // white or black list
    if (localName == WHITELIST_LOCAL_NAME || localName == BLACKLIST_LOCAL_NAME) {
        MISIGHT_LOGD("filterList_ %u localName=%s,runMode_=%s",eventFilter_->eventId_,localName.c_str(),runMode_.c_str());
        filterList_ = localName;
        return;
    }
    // event local name
    if (localName == EVENT_LOCAL_NAME) {
        int32_t min = StringUtil::ConvertInt(attributes[EVENT_ATTR_MIN]);
        int32_t max = StringUtil::ConvertInt(attributes[EVENT_ATTR_MAX]);
        if (eventFilter_->eventId_ < min || eventFilter_->eventId_ > max) {
            MISIGHT_LOGD("eventId %d=%d,max=%d,localName=%s,runMode_=%s",eventFilter_->eventId_, min,max,localName.c_str(),runMode_.c_str());
            return;
        }
        if (filterList_ == WHITELIST_LOCAL_NAME) {
            eventFilter_->hasWhite_ = true;
        }
        if (filterList_ == BLACKLIST_LOCAL_NAME) {
             eventFilter_->hasBlack_ = true;
        }
        eventMatch_ = true;
        return;
    }
    // filter name
    if (eventMatch_ && localName == EVENT_FILTER_NAME) {
        std::string name = attributes[EVENT_FILTER_PARA_ATTR_NAME];
        std::string value = attributes[EVENT_FILTER_VALUE_ATTR_NAME];
        if (filterList_ == WHITELIST_LOCAL_NAME) {
            eventFilter_->AddWhiteList(name, value);
        }
        if (filterList_ == BLACKLIST_LOCAL_NAME) {
             eventFilter_->AddBlackList(name, value);
        }
    }
}

void FilterXmlParser::OnEndElement(const std::string& localName)
{
    if (localName == runMode_) {
        SetFinish();
    }
    if (localName == EVENT_LOCAL_NAME && eventMatch_) {
        SetFinish();
    }
}

sp<EventFilterInfo> FilterXmlParser::GetFilterConfig(const std::string& runMode,
    const std::string& filePath, int32_t eventId)
{
    runMode_ = TESTNAME_LOCAL_NAME + runMode;
    filePath_ = filePath;
    eventMatch_ = false;
    filterList_ = "";
    eventFilter_ = new EventFilterInfo(eventId);
    if (ParserXmlFile(filePath) != 0) {
        MISIGHT_LOGW("filter config file %s failed", filePath.c_str());
        eventFilter_ = nullptr;
        return eventFilter_;
    }
    if (!eventMatch_) {
        eventFilter_ = nullptr;
    }

    MISIGHT_LOGD("GetFilterConfig %d, event id =%d finish ", eventMatch_, eventId);
    return eventFilter_;
}
}
}


