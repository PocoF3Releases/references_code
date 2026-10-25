/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin class head file
 *
 */
#ifndef MISIGH_PLUGIN_BASE_H
#define MISIGH_PLUGIN_BASE_H

#include <ctime>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <utils/RefBase.h>

#include "define_util.h"
#include "event.h"
#include "event_loop.h"

namespace android {
namespace MiSight {
class PluginContext;
class Plugin : public EventHandler {
public:
    Plugin() : context_(nullptr){};
    virtual ~Plugin();
    virtual EVENT_RET OnEvent(sp<Event> &event __UNUSED) override;
    virtual bool CanProcessEvent(sp<Event> event __UNUSED) override;


    /* before load plugin, check whether the plugin can be loaded.
     * return true, the plugin can be loaded, Otherwise, false will not be loaded */
    virtual bool PreToLoad()
    {
        return true;
    };

    /* platform load plugin interface, plugin initial env in this function */
    virtual void OnLoad(){};

    /* platform unload plugin interface, plugin clear env in this function */
    virtual void OnUnload(){};

    /* dump plugin info from dump command line */
    virtual void Dump(int fd __UNUSED, const std::vector<std::string>& cmds __UNUSED) {};

    const std::string& GetName() const;
    const std::string& GetVersion() const;
    PluginContext* GetPluginContext() const;

    /* plugin name, default config name */
    void SetName(const std::string& name);
    /* plugin version, set by bussiness plugin */
    void SetVersion(const std::string& version);
    void SetPluginContext(PluginContext* context);
    /* plugin work looper */
    void SetLooper(sp<EventLoop> loop);
    sp<EventLoop> GetLooper();

protected:
    std::string name_;
    std::string version_;
    sp<EventLoop> looper_;

private:
    /* plugin platform interface */
    PluginContext* context_;
    std::once_flag context_flag_;
};
class PluginContext {
public:
    virtual ~PluginContext(){};

    /* post event to broadcast queue, the event will be delivered to all plugin that concern this event */
    virtual void PostUnorderedEvent(sp<Plugin> plugin __UNUSED, sp<Event> event __UNUSED) {};

    /* register listener to unordered broadcast queue */
    virtual void RegisterUnorderedEventListener(sp<EventListener> listener __UNUSED) {};

    /* send a event to a specific plugin and wait the return of the OnEvent */
    virtual bool PostSyncEventToTarget(sp<Plugin> caller __UNUSED, const std::string& callee __UNUSED,
                                       sp<Event> event __UNUSED)
    {
        return true;
    }

    /* send a event to a specific plugin and return at once */
    virtual void PostAsyncEventToTarget(sp<Plugin> caller __UNUSED, const std::string& callee __UNUSED,
                                        sp<Event> event __UNUSED) {};
    virtual void RegisterPipelineListener(const std::string& triggerName __UNUSED, sp<Plugin> plugin __UNUSED) {}

    /* get the shared event loop reference */
    virtual sp<EventLoop> GetSharedWorkLoop()
    {
        return nullptr;
    }

    /* start pipeline */
    virtual void InsertEventToPipeline(const std::string& triggerName __UNUSED, sp<Event> event __UNUSED) {};

    /* check if all non-pending loaded plugins are loaded */
    virtual bool IsReady()
    {
        return false;
    }


    virtual std::string GetSysProperity(const std::string& key __UNUSED, const std::string& defVal __UNUSED)
    {
        return defVal;
    }

    virtual bool SetSysProperity(const std::string& key __UNUSED, const std::string& val __UNUSED, bool update __UNUSED)
    {
        return true;
    }
    virtual const std::string GetWorkDir()
    {
        return "";
    }
    virtual const std::string GetConfigDir()
    {
        return "";
    }

    virtual const std::string GetCloudConfigDir()
    {
        return "";
    }
    virtual const std::string GetConfigDir(const std::string configName)
    {
        return "";
    }

};
} // namespace MiSight
} // namespace android
#endif
