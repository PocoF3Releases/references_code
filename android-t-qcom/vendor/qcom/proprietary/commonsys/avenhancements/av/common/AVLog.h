/*
 * Copyright (c) 2015, 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef _AV_LOGS_H_
#define _AV_LOGS_H_

#include <inttypes.h>
#include <utils/Log.h>

namespace android {

/*
 * Change logging-level at runtime with "persist.vendor.debug.av.logs.lvl"
 *
 * level     AVLOGV          AVLOGD
 * ----------------------------------
 * 0         silent          silent
 * 1         silent          printed
 * 2         printed         printed
 *
 * AVLOGI/W/E are printed always
 */

extern uint32_t gAVLogLevel;

#define AVLOGV(format, args...) ALOGD_IF((gAVLogLevel > 1), format, ##args)
#define AVLOGD(format, args...) ALOGD_IF((gAVLogLevel > 0), format, ##args)
#define AVLOGI(format, args...) ALOGI(format, ##args)
#define AVLOGW(format, args...) ALOGW(format, ##args)
#define AVLOGE(format, args...) ALOGE(format, ##args)

void updateLogLevel();

} //namespace android

#endif // _AV_LOGS_H_

