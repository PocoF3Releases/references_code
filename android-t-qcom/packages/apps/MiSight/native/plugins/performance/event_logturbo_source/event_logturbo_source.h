/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event radiator plugin head file
 *
 */
#ifndef MISIGH_PLUGIN_PERFORMANCE_EVENT_LOGTURBO_SOURCE_H
#define MISIGH_PLUGIN_PERFORMANCE_EVENT_LOGTURBO_SOURCE_H

#include <string>
#include <utils/RefBase.h>
#include "event_source.h"
#include "file_monitor.h"

namespace android {
namespace MiSight {

class EventLogturboSource : public EventSource {
public:
    EventLogturboSource() : exit_(false), waitTime_(30) {};
    ~EventLogturboSource() {
            exit_ = true;
    }

    void OnLoad() override;
    void OnUnload() override;
    void StartEventSource() override;
    void AddPipeline(const std::string& pipelineName, sp<Pipeline> pipeline);
    // Logturbo
    int mEpollFd;
    int mExceptionLogFd;
    bool mInited;
    FileMonitor* mFileMonitor;
private:
    bool exit_;
    int32_t waitTime_;
    void ParseEvent(const std::string& eventStr);
    bool threadLoop();
    int init();
};
}
}
#endif
