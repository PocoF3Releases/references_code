/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight pipeline class head file
 *
 */
#include "pipeline.h"

#include <memory>

#include "log.h"


namespace android {
namespace MiSight {

bool Pipeline::CanProcessEvent(sp<Event> event)
{
    if (plugins_.empty()) {
        MISIGHT_LOGD("no plugin processor in this pipeline");
        return false;
    }

    sp<Plugin> plugin = plugins_[0];
    if (plugin != nullptr) {
        return plugin->CanProcessEvent(event);
    }
    return false;
}

void Pipeline::ProcessEvent(sp<Event> event, uint32_t index)
{
    auto& plugin = plugins_[index];
    EVENT_RET ret = plugin->OnEvent(event);
    if (ret != ON_FAILURE) {
        if (index + 1 == plugins_.size()) {
            // dispatch or broadcast
            for (auto& pluginEnd : pluginListeners_) {
                auto looper = pluginEnd->GetLooper();
                if (looper == nullptr) {
                    pluginEnd->OnEvent(event);
                } else {
                    looper->AddEvent(pluginEnd, event);
                }
            }
            return;
        }
        return ProcessPipeline(event, index + 1);
    };
    return;

}

void Pipeline::ProcessPipeline(sp<Event> event, uint32_t index)
{
    if (index == plugins_.size()) {
        MISIGHT_LOGE("index over plugins size");
        return;
    }

    auto& plugin = plugins_[index];
    auto workLoop = plugin->GetLooper();
    bool needSched = false;
    if (workLoop != nullptr) {
        needSched = true;
        if (index > 0) {
            auto& prePlugin = plugins_[index -1];
            auto preWorkLoop = prePlugin->GetLooper();
            if ((preWorkLoop != nullptr) && (preWorkLoop->GetName() == workLoop->GetName())) {
                needSched = false;
            }
        }
    }
    if (needSched) {
        auto task = std::bind(&Pipeline::ProcessEvent, this, event, index);
        workLoop->AddTask(task);
    } else {
        ProcessEvent(event, index);
    }
}

void Pipeline::RegisterPluginListener(sp<Plugin> plugin)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pluginListeners_.insert(plugin);

}
} // namespace MiSight
} // namespace android
