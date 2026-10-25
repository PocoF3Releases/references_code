/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin class implements
 *
 */
#include "plugin.h"

#include "log.h"

namespace android {
namespace MiSight {
Plugin::~Plugin()
{
    if (looper_ != nullptr) {
        looper_->StopLoop();
    }
}

EVENT_RET Plugin::OnEvent(sp<Event>& event __UNUSED)
{
    return ON_SUCCESS;
}

bool Plugin::CanProcessEvent(sp<Event> event __UNUSED)
{
    return true;
}

const std::string& Plugin::GetName() const
{
    return name_;
}

const std::string& Plugin::GetVersion() const
{
    return version_;
}

PluginContext* Plugin::GetPluginContext() const
{
    return context_;
}

void Plugin::SetName(const std::string& name)
{
    name_ = name;
}

void Plugin::SetVersion(const std::string& version)
{
    version_ = version;
}

void Plugin::SetPluginContext(PluginContext* context)
{
   std::call_once(context_flag_, [&]() { context_ = context; });
}

void Plugin::SetLooper(sp<EventLoop> loop)
{
    if (loop == nullptr) {
        return;
    }
    looper_ = loop;
}

sp<EventLoop> Plugin::GetLooper()
{
    return looper_;
}

} // namespace MiSight
} // namespace android
