/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin factory  head file
 *
 */

#ifndef MISIGHT_BASE_PLUGIN_FACTORY_H
#define MISIGHT_BASE_PLUGIN_FACTORY_H
#include <functional>
#include <map>

#include <utils/StrongPointer.h>

#include "plugin.h"

namespace android {
namespace MiSight {

class PluginFactory {
public:
    typedef std::function<sp<Plugin>()> PluginPtrFunc;
    template<typename T>
    class Register {
    public:
        Register(const std::string& name)
        {
            PluginFactory::GetInstance().GetPluginMap().emplace(name, &Register<T>::ConstructPlugin);
        }
        inline static sp<Plugin> ConstructPlugin()
        {
            return new T;
        }
    };
    inline sp<Plugin> GetPlugin(const std::string& name)
    {
        auto it = pluginInitFuncMap_.find(name);
        if (it != pluginInitFuncMap_.end()) {
            return it->second();
        } else {
            return nullptr;
    }
    }

    inline static PluginFactory& GetInstance() {
        static PluginFactory instance;
        return instance;
    }

    inline std::map<std::string, PluginPtrFunc>& GetPluginMap() {
        return pluginInitFuncMap_;
    }
private:
    std::map<std::string, PluginPtrFunc> pluginInitFuncMap_;
};

#define REGISTER_PLUGIN_VNAME(T) reg_plugin_##T##_
#define REGISTER_PLUGIN(T) static PluginFactory::Register<T> REGISTER_PLUGIN_VNAME(T)(#T);

} // namespace MiSight
} // namespace android
#endif
