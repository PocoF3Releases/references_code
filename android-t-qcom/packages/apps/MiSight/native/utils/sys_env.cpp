/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight system environment common implement file
 *
 */
#include "sys_env.h"

#include <cutils/properties.h>

namespace android {
namespace MiSight {
namespace SysEnv {
std::string GetSysProperity (const std::string& key, const std::string& defVal)
{
    char getStr[PROPERTY_VALUE_MAX] = {0};
    property_get(key.c_str(), getStr, "");
    if (strlen(getStr) == 0) {
        return defVal;
    }
    return std::string(getStr);
}
bool SetSysProperity(const std::string& key, const std::string& val)
{
     return (property_set(key.c_str(), val.c_str()) == 0);
}
} // namespace SysEnv
} // namespace MiSight
} // namespace android
