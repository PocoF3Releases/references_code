/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event utils head file
 *
 */

#ifndef MISIGHT_PLUGIN_EVENT_UTIL_H
#define MISIGHT_PLUGIN_EVENT_UTIL_H
#include <cstdint>
#include <string>

namespace android {
namespace MiSight {
namespace EventUtil {
// read from miev format
const std::string DEV_FIELD_EVENT = "EventId";
const std::string DEV_FIELD_TIMESTAMP = "-t";
const std::string DEV_FIELD_PARMLIST = "-paraList";
const std::string DEV_SAVE_EVENT = "param_list";

// fault event
constexpr uint32_t FAULT_EVENT_MAX = 999999999;
constexpr uint32_t FAULT_EVENT_MIN = 900000000;
constexpr uint32_t FAULT_EVENT_SIX_RANGE = 1000000;
const std::string FAULT_EVENT_DEFAULT_DIR = "0";
// raw event
constexpr uint32_t RAW_EVENT_MAX = 199999;
constexpr uint32_t RAW_EVENT_MIN = 100000;
//raw event
const std::string RAW_EVENT_DB_ID = "rawEventDbID";

// event drop reason
const std::string EVENT_DROP_REASON  = "dropReason";
const std::string EVENT_NOCATCH_LOG  = "noCatchLog";
const std::string EVENT_LOG_ZIP_NAME = "zipLogName";

enum FaultLevel {
    DEBUG,
    INFORMATIVE,
    GENERAL,
    IMPORTANT,
    CRITICAL
};
enum PrivacyLevel {
     L1 = 1,
     L2,
     L3,
     L4,
     MAX_LEVEL
};
FaultLevel GetFaultLevel(const std::string& faultLevel);
PrivacyLevel GetPrivacyLevel(const std::string& privacyLevel);
}
} // namespace MiSight
} // namespace android
#endif
