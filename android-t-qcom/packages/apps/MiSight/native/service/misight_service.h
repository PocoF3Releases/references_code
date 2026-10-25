/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight service head file
 *
 */
#ifndef MISIGHT_SERVICE_MISIGHT_SERVICE_H
#define MISIGHT_SERVICE_MISIGHT_SERVICE_H

#include <string>

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

#include "plugin_platform.h"

namespace android {
namespace MiSight {
class MiSightService : public RefBase {
public:
    explicit MiSightService(PluginPlatform *platform);
    ~MiSightService();
    void StartService();
    int NotifyPackLog();
    void SetRunMode(const std::string& runName, const std::string& filePath);
    void NotifyUploadSwitch(bool isOn);
    void NotifyUserActivated();
    void NotifyConfigUpdate(const std::string& folderName);
    void Dump(int fd, const std::vector<std::string>& args);
private:
    PluginPlatform *platform_;

    void DumpPluginSummaryInfo(int fd);
    void DumpPluginInfo(int fd, const std::string& pluginName, const std::vector<std::string>& args);
};
} // namespace MiSight
} // namespace android
#endif
