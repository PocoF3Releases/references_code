/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight system environment common head file
 *
 */
#ifndef MISIGHT_SYS_ENV_UTIL_H
#define MISIGHT_SYS_ENV_UTIL_H
#include <string>
namespace android {
namespace MiSight {
namespace SysEnv {
std::string GetSysProperity (const std::string& key, const std::string& defVal);
bool SetSysProperity(const std::string& key, const std::string& val);
} // namespace SysEnv
} // namespace MiSight
} // namespace android
#endif

