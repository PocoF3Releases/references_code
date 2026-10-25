/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample4 head file
 *
 */

#ifndef MISIGHT_SAMPLE4_PLUGIN_H
#define MISIGHT_SAMPLE4_PLUGIN_H
#include "plugin.h"

namespace android {
namespace MiSight {
class PluginSample4 : public Plugin {
public:
    PluginSample4() : loaded_(false) {}
    ~PluginSample4() {}
    bool IsLoad() {
        return loaded_;
    }
    bool CanProcessEvent(sp<Event> event) override;
    EVENT_RET OnEvent(sp<Event>& event) override;
    void OnLoad() override;
    void OnUnload() override;
    std::string eventInfo_ = "";
private:
    bool loaded_;
};
}
}
#endif
