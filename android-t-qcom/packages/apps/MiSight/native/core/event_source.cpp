/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event source implement file
 *
 */
#include "event_source.h"

#include "log.h"
namespace android {
namespace MiSight {

bool EventSource::PublishPipelineEvent(sp<Event> event)
{
    if (event == nullptr) {
        return false;
    }

    for (auto& pipeline : listeners_) {
        if ((pipeline != nullptr) && (pipeline->CanProcessEvent(event))) {
            // one event can only be processed by on pipeline
            pipeline->ProcessPipeline(event);
            return true;
        }
    }
    return false;
}

void EventSource::AddPipeline(const std::string& pipelineName, sp<Pipeline> pipeline)
{
    if (pipeline == nullptr) {
        MISIGHT_LOGW("%s pipeline nullptr, not add to %s.", pipelineName.c_str(), GetName().c_str());
        return;
    }
    MISIGHT_LOGD("EventSource %s add pipeline %s.", GetName().c_str(), pipeline->GetName().c_str());
    listeners_.push_back(pipeline);
}
}// namespace MiSight
}// namespace andrid


