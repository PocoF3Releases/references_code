/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight time common implements
 *
 */

#include <utils/TimeUtil.h>

#include <chrono>
#include <cinttypes>
#include <ctime>
#include <sys/time.h>
#include <unistd.h>

namespace aidl::vendor::xiaomi::hardware::misight {
namespace TimeUtil {

uint64_t GetTimestampUS()
{
    struct timeval now = {.tv_sec = 0, .tv_usec = 0 };

    if (gettimeofday(&now, nullptr) == -1) {
        return 0;
    }
    return (now.tv_sec * S_TO_US + now.tv_usec);
}

uint64_t GetTimestampSecond()
{
    return (GetTimestampUS()/ S_TO_US);
}


uint64_t GetNanoSecondSinceSystemStart()
{
    auto nanoNow = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(nanoNow.count());
}

int64_t GetZeroTimestampMs()
{
    time_t now = std::time(nullptr);
    int64_t zero = now;
    struct tm *l = std::localtime(&now);
    if (l != nullptr) {
        l->tm_hour = 0;
        l->tm_min = 0;
        l->tm_sec = 0;
        zero = std::mktime(l) * TIME_UNIT;  // time is 00:00:00
    }
    return zero;
}

std::string GetTimeDateStr(long timestamp, const std::string& pattern)
{
    char timeChar[20]; // 20:time str len;
    if (std::strftime(timeChar, sizeof(timeChar), pattern.c_str(), std::localtime(&timestamp))) {
        return std::string(timeChar);
    }

    return "";
}

time_t DateStrToTime(const std::string& ATime, const std::string& AFormat) {
    struct tm tm_Temp;
    time_t time_Ret;
    sscanf(ATime.c_str(), AFormat.c_str(),// "%d/%d/%d-%d:%d:%d"
            &(tm_Temp.tm_year),
            &(tm_Temp.tm_mon),
            &(tm_Temp.tm_mday),
            &(tm_Temp.tm_hour),
            &(tm_Temp.tm_min),
            &(tm_Temp.tm_sec),
            &(tm_Temp.tm_wday),
            &(tm_Temp.tm_yday));

    tm_Temp.tm_year -= 1900;
    tm_Temp.tm_mon --;
    tm_Temp.tm_isdst = 0;
    time_Ret = mktime(&tm_Temp);
    return time_Ret;
}

time_t GetCurrentTime() {
    time_t t_Now = time(0);
    struct tm* tm_Now = localtime(&t_Now);

    return mktime(tm_Now);
}

} // namespace TimeUtil
} // namespace aidl::vendor::xiaomi::hardware::misight
