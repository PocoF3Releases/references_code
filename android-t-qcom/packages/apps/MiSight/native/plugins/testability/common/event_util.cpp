/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event utils head file
 *
 */
#include "event_util.h"

namespace android {
namespace MiSight {
namespace EventUtil {
FaultLevel GetFaultLevel(const std::string& faultLevel)
{

    FaultLevel level = FaultLevel::DEBUG;
    if (faultLevel == "INFORMATIVE") {
        level = FaultLevel::INFORMATIVE;
    }
    if (faultLevel == "GENERAL") {
        level = FaultLevel::GENERAL;
    }
    if (faultLevel == "IMPORTANT") {
        level = FaultLevel::IMPORTANT;
    }
    if (faultLevel == "CRITICAL") {
        level = FaultLevel::CRITICAL;
    }
    return level;
}

PrivacyLevel GetPrivacyLevel(const std::string& privacyLevel)
{
    PrivacyLevel level = PrivacyLevel::MAX_LEVEL;
    if (privacyLevel == "L1") {
        level = PrivacyLevel::L1;
    }
    if (privacyLevel == "L3") {
        level = PrivacyLevel::L3;
    }
    if (privacyLevel == "L4") {
        level = PrivacyLevel::L4;
    }
    if (privacyLevel == "L2") {
        level = PrivacyLevel::L2;
    }
    return level;
}
}
} // namespace MiSight
} // namespace android

