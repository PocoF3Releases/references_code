/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event basic head file
 *
 */
#ifndef MISIGH_COMMON_EVENT_BASE_H
#define MISIGH_COMMON_EVENT_BASE_H

#include <map>
#include <set>
#include <string>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

#include "define_util.h"
#include "log.h"

namespace android {
namespace MiSight {
class Event : public RefBase {
public:
    // use for broadcasting events
    enum MessageType {
        NONE = 0,
        PLUGIN_MAINTENANCE,
        FAULT_EVENT,
        STATISTICS_EVENT,
        RAW_EVENT,
        CROSS_PLATFORM,
        PRIVATE_MESSAGE_TYPE // Expand macro from public_defines.h
    };

    enum ManageType {
        ORDERED,
        UNORDERED,
        PROCESS_TYPE_NUM,
    };
    explicit Event(const std::string &sender)
        : messageType_(MessageType::NONE),
          processType_(ManageType::UNORDERED),
          what_(0),
          eventId_(0),
          happenTime_(0),
          targetDispatchTime_(0),
          createTime_(0),
          sender_(sender),
          isPipeline_(false)
    {
        ResetTimestamp();
    };

    virtual ~Event() {
    };

    enum EventId {
        PLUGIN_LOADED,
    };

    MessageType messageType_;
    ManageType processType_;
    uint16_t what_;
    uint32_t eventId_;
    uint64_t happenTime_;
    uint64_t targetDispatchTime_;
    uint64_t createTime_;
    std::string sender_;
    bool isPipeline_;

    void SetValue(const std::string &name, const std::string &value);
    void SetValue(const std::string &name, int32_t value);
    void SetKeyValuePairs(std::map<std::string, std::string> keyValuePairs);
    const std::string GetValue(const std::string &name) const;
    int32_t GetIntValue(const std::string &name) const;
    std::map<std::string, std::string> GetKeyValuePairs() const;

    void ResetTimestamp();
    bool IsPipelineEvent() const
    {
        return isPipeline_;
    };
protected:
    std::map<std::string, std::string> bundle_;
};

enum EVENT_RET {
    ON_FAILURE = -1,
    ON_SUCCESS,
    ON_FINISH
};

class  EventHandler : public virtual RefBase {
public:
    virtual ~EventHandler(){};
    virtual EVENT_RET OnEvent(sp<Event>& event) = 0;
    virtual bool CanProcessEvent(sp<Event> event __UNUSED)
    {
        return true;
    };
    virtual std::string GetHandlerInfo()
    {
        return "";
    };
};

class EventListener : public virtual RefBase {
public:
    struct EventRange {
        uint32_t begin;
        uint32_t end;
        EventRange(uint32_t id)
        {
            begin = id;
            end = id;
        };

        EventRange(uint32_t begin, uint32_t end)
        {
            this->begin = begin;
            this->end = end;
        };
        bool operator<(const EventRange &range) const
        {
            return (end < range.begin);
        };
        bool operator==(const EventRange &range) const
        {
            return ((begin == range.begin) && (end == range.end));
        };
    };

    EventListener() {};
    virtual ~EventListener(){};
    virtual bool OnOrderedEvent(sp<Event> event __UNUSED) { return false;};
    virtual void OnUnorderedEvent(sp<Event> event) = 0;
    virtual std::string GetListenerName() = 0;
    // Make sure that you insert non-overlayed range
    void AddListenerInfo(uint32_t type, const EventRange &range = EventListener::EventRange(0));
    void AddListenerInfo(uint32_t type, const std::set<EventRange> &listenerInfo);
    bool GetListenerInfo(uint32_t type, std::set<EventRange> &listenerInfo);
private:
    std::map<uint32_t, std::set<EventRange>> listenerInfo_;
};
} // namespace MiSight
} // namespace android
#endif
