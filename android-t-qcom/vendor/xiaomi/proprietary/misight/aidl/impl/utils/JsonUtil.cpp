/*
 * Copyright (c) 2022 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#include <utils/JsonUtil.h>
#include <android-base/logging.h>

namespace aidl::vendor::xiaomi::hardware::misight {
namespace JsonUtil {
bool ConvertStrToJson(const std::string& jsonStr, Json::Value &jsonObj)
{
#ifdef JSONCPP_VERSION_STRING
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    JSONCPP_STRING errs;
    bool ret = reader->parse(jsonStr.c_str(), jsonStr.c_str() + jsonStr.size(), &jsonObj, &errs);
    if (errs.size() != 0) {
        LOG(ERROR) << "json convert failed " << errs;
    }
    return (ret && errs.size() == 0);
#else
    Json::Reader reader;
    return reader.parse(jsonStr, jsonObj, false);
#endif
}

std::string ConvertJsonToStr(Json::Value paramJson)
{
#ifdef JSONCPP_VERSION_STRING
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = ""; // If you want whitespace-less output
    builder.settings_["precision"] = 3;
    return Json::writeString(builder, paramJson);
#else
    return Json::FastWriter().write(paramJson);
#endif
}
}
} // namespace aidl::vendor::xiaomi::hardware::misight