/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight json utils head file
 *
 */

#ifndef MISIGHT_TESTABLE_PLUGIN_JSON_UTIL_H
#define MISIGHT_TESTABLE_PLUGIN_JSON_UTIL_H
#include <cstdint>
#include <string>

#include <json/json.h>

namespace android {
namespace MiSight {
namespace JsonUtil {
bool ConvertStrToJson(const std::string& jsonStr, Json::Value &jsonObj);
std::string ConvertJsonToStr(Json::Value paramJson);
}
} // namespace MiSight
} // namespace android
#endif
