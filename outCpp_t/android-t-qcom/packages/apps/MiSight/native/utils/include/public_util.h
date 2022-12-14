/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight define common head file
 *
 */
#ifndef MISIGHT_INCLUDE_PUBLIC_UTIL_H
#define MISIGHT_INCLUDE_PUBLIC_UTIL_H

#include <string>
namespace android {
namespace MiSight {

constexpr int EVENT_SERVICE_UPLOAD_ID = 101;
constexpr int EVENT_SERVICE_RUNMODE_ID = EVENT_SERVICE_UPLOAD_ID +1;
constexpr int EVENT_SERVICE_UE_UPDATE_ID = EVENT_SERVICE_UPLOAD_ID +2;
constexpr int EVENT_SERVICE_CONFIG_UPDATE_ID = EVENT_SERVICE_UPLOAD_ID +3;
constexpr int EVENT_SERVICE_USER_ACTIVATE_ID = EVENT_SERVICE_UPLOAD_ID +4;


const std::string EVENT_SERVICE_UPLOAD_RET = "EVENT_SERVICE_UPLOAD_RET";
const std::string EVENT_SERVICE_UE_UPDATE = "EVENT_UE_UPDATE";
} // namespace MiSight
} // namespace android
#endif

