/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin sample3 head file
 *
 */

#ifndef MISIGHT_PLUGIN_SAMPLE3_H
#define MISIGHT_PLUGIN_SAMPLE3_H

#include "plugin.h"

namespace android {
namespace MiSight {


class PluginSample3 : public Plugin {
public:
    PluginSample3() : loaded_(false) {}
    ~PluginSample3() {}
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
