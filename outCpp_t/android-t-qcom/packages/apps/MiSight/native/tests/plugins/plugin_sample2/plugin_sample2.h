/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample2 head file
 *
 */

#ifndef MISIGHT_PLUGIN_SAMPLE2_H
#define MISIGHT_PLUGIN_SAMPLE2_H
#include "plugin.h"

namespace android {
namespace MiSight {
class PluginSample2 : public Plugin {
public:
    PluginSample2() : loaded_(false) {}
    ~PluginSample2() {}
    bool IsLoad() {
        return loaded_;
    }
    bool CanProcessEvent(sp<Event> event) override;
    EVENT_RET OnEvent(sp<Event>& event) override;
    void OnLoad() override;
    void OnUnload() override;
private:
    bool loaded_;
};
}
}
#endif
