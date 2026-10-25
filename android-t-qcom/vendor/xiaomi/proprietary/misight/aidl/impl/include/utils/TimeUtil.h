/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight time common head file
 *
 */

#include <string>
#include <ctime>

namespace aidl::vendor::xiaomi::hardware::misight {

constexpr uint64_t TIME_UNIT = 1000;
constexpr uint64_t US_TO_NS = TIME_UNIT;
constexpr uint64_t MS_TO_US = TIME_UNIT;
constexpr uint64_t MS_TO_NS = MS_TO_US * US_TO_NS;
constexpr uint64_t S_TO_MS = TIME_UNIT;
constexpr uint64_t S_TO_US = S_TO_MS * MS_TO_US;
constexpr uint64_t S_TO_NS = S_TO_US * US_TO_NS;
constexpr uint64_t DAY_SECONDS = 24 * 60 * 60;
constexpr uint64_t HOUR_MILLISECONDS = 60 * 60 * TIME_UNIT;
constexpr uint64_t HOUR_SECONDS = 60 * 60;
constexpr uint64_t DAY_HOUR = 24;
constexpr uint64_t MONTH_DAY = 30;
constexpr uint64_t MINUTE_SECONDS = 60;
constexpr uint64_t YEAR_DAYS = 365;


namespace TimeUtil {
uint64_t GetTimestampUS();
uint64_t GetTimestampSecond();

uint64_t GetNanoSecondSinceSystemStart();
int64_t GetZeroTimestampMs();
std::string GetTimeDateStr(long timestamp, const std::string& pattern);
time_t DateStrToTime(const std::string& ATime, const std::string& AFormat="%d-%d-%d-%d:%d:%d");
time_t GetCurrentTime();


} // namespace TimeUtil
} // namespace aidl::vendor::xiaomi::hardware::misight

