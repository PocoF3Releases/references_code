/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight string process head file
 *
 */
#ifndef MISIGHT_STRING_UTIL_H
#define MISIGHT_STRING_UTIL_H

#include <list>
#include <string>

namespace android {
namespace MiSight {
namespace StringUtil {
    std::string TrimStr(const std::string& str, const char cTrim = ' ');
    std::list<std::string> SplitStr(const std::string& str, char delimiter = ' ');
    int32_t ConvertInt(const std::string& str);
    bool StartsWith(const std::string &str, const std::string &searchFor);
    bool EndsWith(const std::string &str, const std::string &searchFor);
    std::string ReplaceStr(const std::string& str, const std::string& src, const std::string& dst);
    std::string EscapeSqliteChar(std::string value);
} // namespace StringUtil
} // namespace MiSight
} // namespace android
#endif