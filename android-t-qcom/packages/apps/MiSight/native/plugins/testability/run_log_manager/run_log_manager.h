/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: log manager plugin head file
 *
 */
#ifndef MISIGH_PLUGIN_TESTABILITY_LOG_MANAGER_PLUGIN_H
#define MISIGH_PLUGIN_TESTABILITY_LOG_MANAGER_PLUGIN_H

#include <string>

#include <json/json.h>

#include "event.h"
#include "plugin.h"

namespace android {
namespace MiSight {

class RunLogManager : public Plugin {
public:
    RunLogManager();
    ~RunLogManager();

    void OnLoad() override;
    void OnUnload() override;
    EVENT_RET OnEvent(sp<Event> &event) override;
    void SetStaticSpace (uint32_t staticSpace) {
        staticSpace_ = staticSpace;
    }
    uint32_t GetStaticSpace() {
        return staticSpace_;
    }

private:
    void LogFilterStatics();
    bool GetLogFilterDetail(Json::Value& root);
    uint32_t staticSpace_;
};
}
}
#endif
