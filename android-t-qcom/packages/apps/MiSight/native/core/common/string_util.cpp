/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight string process implemnets
 *
 */

#include "string_util.h"

#include <errno.h>
#include <sstream>

#include "log.h"
namespace android {
namespace MiSight {
namespace StringUtil {
std::string TrimStr(const std::string& str, const char cTrim)
{
    std::string strTmp = str;
    strTmp.erase(0, strTmp.find_first_not_of(cTrim));
    strTmp.erase(strTmp.find_last_not_of(cTrim) + sizeof(char));
    return strTmp;
}

std::list<std::string> SplitStr(const std::string& str, char delimiter)
{
    std::list<std::string> tokens;
    std::string s;
    std::stringstream  ss(str);
    while (std::getline(ss, s, delimiter)) {
        tokens.push_back(s);
    }
    return tokens;
}

int32_t ConvertInt(const std::string& str)
{
    if (str == "") {
        return 0;
    }

    const int32_t decimal = 10;
    int32_t ret = static_cast<int32_t>(std::strtol(str.c_str(), nullptr, decimal));
    return (errno == ERANGE) ? -1 : ret;
}


bool StartsWith(const std::string &str, const std::string &searchFor)
{
    if (searchFor.size() > str.size()) {
        return false;
    }

    std::string source = str.substr(0, searchFor.size());
    return source == searchFor;
}

bool EndsWith(const std::string &str, const std::string &searchFor)
{
    if (searchFor.size() > str.size()) {
        return false;
    }

    std::string source = str.substr(str.size() - searchFor.size(), searchFor.size());
    return source == searchFor;
}


std::string ReplaceStr(const std::string& str, const std::string& src, const std::string& dst)
{
    if (src.empty()) {
        return str;
    }

    std::string::size_type pos = 0;
    std::string strTmp = str;
    while ((pos = strTmp.find(src, pos)) != std::string::npos) {
        strTmp.replace(pos, src.length(), dst);
        pos += dst.length();
    }
    return strTmp;
}


std::string EscapeSqliteChar(std::string value)
{

    value = ReplaceStr(value, "'", "''");
    value = ReplaceStr(value, "/", "//");
    //value = ReplaceStr(value, "[", "/[");
    //value = ReplaceStr(value, "]", "/]");
    value = ReplaceStr(value, "%", "/%");
    value = ReplaceStr(value, "&", "/&");
    value = ReplaceStr(value, "_", "/_");
    //value = ReplaceStr(value, "(", "/(");
    //value = ReplaceStr(value, ")", "/)");
    return value;
}


} // namespace StringUtil
} // namespace MiSight
} // namespace android

