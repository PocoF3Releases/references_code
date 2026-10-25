/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event radiator plugin head file
 *
 */
#ifndef MISIGH_PLUGIN_TESTABILITY_EVENT_RADIATOR_H
#define MISIGH_PLUGIN_TESTABILITY_EVENT_RADIATOR_H

#include <string>

#include <utils/RefBase.h>

#include "event_source.h"

namespace android {
namespace MiSight {

class EventRadiator : public EventSource {
public:
    EventRadiator() : exit_(false), waitTime_(30) {};
    ~EventRadiator() {
        exit_ = true;
    }

    void OnLoad() override;
    void OnUnload() override;
    void StartEventSource() override;

private:
    void ParseEvent(char* buf);
//
    bool exit_;
    int32_t waitTime_;


};
}
}
#endif
