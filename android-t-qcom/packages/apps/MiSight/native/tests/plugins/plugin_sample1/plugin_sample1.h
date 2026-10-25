/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample1 head file
 *
 */

#ifndef MISIGHT_PLUGIN_SAMPLE1_H
#define MISIGHT_PLUGIN_SAMPLE1_H
#include "plugin.h"

namespace android {
namespace MiSight {
class PluginSample1 : public Plugin, public EventListener {
public:
    PluginSample1() : loaded_(false) {}
    ~PluginSample1() {}
    bool CanProcessEvent(sp<Event> event) override;
    EVENT_RET OnEvent(sp<Event>& event) override;
    void OnLoad() override;
    void OnUnload() override;
    bool IsLoad() {
        return loaded_;
    }

    void OnUnorderedEvent(sp<Event> event) override;
    std::string GetListenerName() override {
        return GetName();
    }
    int recvUnorderCnt_ = 0;

private:
    bool loaded_;
};
}
}
#endif
