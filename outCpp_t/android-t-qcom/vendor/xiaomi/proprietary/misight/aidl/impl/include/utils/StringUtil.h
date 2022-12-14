/*
 * Copyright (c) 2022 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#include <list>
#include <string>

namespace aidl::vendor::xiaomi::hardware::misight {
namespace StringUtil {
    std::string TrimStr(const std::string& str, const char cTrim = ' ');
    std::list<std::string> SplitStr(const std::string& str, char delimiter = ' ');
    int32_t ConvertInt(const std::string& str);
    bool StartsWith(const std::string &str, const std::string &searchFor);
    bool EndsWith(const std::string &str, const std::string &searchFor);
    std::string ReplaceStr(const std::string& str, const std::string& src, const std::string& dst);

} // namespace StringUtil
} // namespace aidl::vendor::xiaomi::hardware::misight