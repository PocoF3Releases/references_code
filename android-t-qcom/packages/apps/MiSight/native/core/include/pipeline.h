/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight pipeline class head file
 *
 */
#ifndef MISIGH_PIPELINE_BASE_H
#define MISIGH_PIPELINE_BASE_H

#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "event.h"
#include "event_loop.h"
#include "plugin.h"


namespace android {
namespace MiSight {
class Pipeline : public RefBase {
public:
    Pipeline(const std::string& name, std::vector<sp<Plugin>> plugins)
        : name_(name), plugins_(std::move(plugins)){};
    bool CanProcessEvent(sp<Event> event);
    void ProcessPipeline(sp<Event> event, uint32_t index = 0);
    ~Pipeline() {};

    const std::string& GetName() const
    {
        return name_;
    };

    const std::vector<sp<Plugin>> GetPlugins() const
    {
        return plugins_;
    };

    void RegisterPluginListener(sp<Plugin> plugin);

private:
    void ProcessEvent(sp<Event> event, uint32_t index);
    std::string name_;
    std::vector<sp<Plugin>> plugins_;
    std::set<sp<Plugin>> pluginListeners_;
    std::mutex mutex_;
};
} // namespace MiSight
} // namespace android
#endif

