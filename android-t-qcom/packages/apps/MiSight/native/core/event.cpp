/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event basic head file
 *
 */
#include "event.h"

#include "string_util.h"
#include "time_util.h"

namespace android {
namespace MiSight {
void Event::SetValue(const std::string &name, const std::string &value)
{
    if (name.length() > 0) {
        bundle_[name] = std::move(value);
    }
}

const std::string Event::GetValue(const std::string &name) const
{
    auto it = bundle_.find(name);
    if (it != bundle_.end()) {
        return it->second;
    }
    return std::string("");
}

void Event::SetValue(const std::string &name, int32_t value)
{
    std::string str = std::to_string(value);
    if (name.length() > 0) {
        bundle_[name] = str;
    }
}

int32_t Event::GetIntValue(const std::string &name) const
{
    auto it = bundle_.find(name);
    if (it != bundle_.end()) {
        return StringUtil::ConvertInt(it->second);
    }
    return -1;
}

void Event::ResetTimestamp()
{
    createTime_ = TimeUtil::GetTimestampUS();
}

void Event::SetKeyValuePairs(std::map<std::string, std::string> keyValuePairs)
{
    bundle_.insert(keyValuePairs.begin(), keyValuePairs.end());
}

std::map<std::string, std::string> Event::GetKeyValuePairs() const
{
    return bundle_;
}

void EventListener::AddListenerInfo(uint32_t type, const EventRange& range)
{
    auto it = listenerInfo_.find(type);
    if (it != listenerInfo_.end()) {
        it->second.insert(range);
        return;
    }

    std::set<EventRange> listenerSet;
    listenerSet.insert(range);
    listenerInfo_[type] = std::move(listenerSet);
}

void EventListener::AddListenerInfo(uint32_t type, const std::set<EventRange> &listenerInfo)
{
    auto it = listenerInfo_.find(type);
    if (it != listenerInfo_.end()) {
        it->second.insert(listenerInfo.begin(), listenerInfo.end());
        return;
    }
    listenerInfo_[type] = listenerInfo;
}

bool EventListener::GetListenerInfo(uint32_t type, std::set<EventRange> &listenerInfo)
{
    auto it = listenerInfo_.find(type);
    if (it != listenerInfo_.end()) {
        listenerInfo.clear();
        listenerInfo.insert(it->second.begin(), it->second.end());
        return true;
    }
    return false;
}
} // namespace MiSight
} // namespace andrid
