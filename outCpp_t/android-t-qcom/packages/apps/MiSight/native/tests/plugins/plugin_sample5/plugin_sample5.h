/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample5 head file
 *
 */

#ifndef MISIGHT_SAMPLE5_PLUGIN_H
#define MISIGHT_SAMPLE5_PLUGIN_H
#include "plugin.h"

namespace android {
namespace MiSight {
class PluginSample5 : public Plugin, public EventListener {
public:
    PluginSample5() : loaded_(false) {}
    ~PluginSample5() {}
    bool IsLoad() {
        return loaded_;
    }
    bool CanProcessEvent(sp<Event> event) override;
    EVENT_RET OnEvent(sp<Event>& event) override;
    void OnLoad() override;
    void OnUnload() override;
    void OnUnorderedEvent(sp<Event> event) override;

    std::string GetListenerName() override {
        return GetName();
    }
    std::string eventInfo_ = "";
private:
    bool loaded_;
};
}
}
#endif
