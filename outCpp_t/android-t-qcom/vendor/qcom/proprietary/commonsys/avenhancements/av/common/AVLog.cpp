/*
 * Copyright (c) 2015, 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#include <stdlib.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#include "AVLog.h"

namespace android {

uint32_t gAVLogLevel;

void updateLogLevel() {
    char level[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.debug.av.logs.lvl", level, "0");
    gAVLogLevel = atoi(level);
}

} // namespace android

