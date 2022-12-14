/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event source head file
 *
 */

#ifndef MISIGHT_EVENT_SOURCE_H
#define MISIGHT_EVENT_SOURCE_H
#include <list>
#include <string>

#include <utils/StrongPointer.h>

#include "event.h"
#include "pipeline.h"


namespace android {
namespace MiSight {

class EventSource : public Plugin {
public:
    EventSource() {};
    virtual ~EventSource() {};
    virtual void StartEventSource() {};
    bool PublishPipelineEvent(sp<Event> event);
    void AddPipeline(const std::string& pipelineName, sp<Pipeline> pipeline);
private:
    std::list<sp<Pipeline>> listeners_;
};
}// namespace MiSight
}// namespace andrid
#endif

