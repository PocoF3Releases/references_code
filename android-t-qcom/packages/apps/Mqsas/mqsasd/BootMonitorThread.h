/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef MQS_BOOT_MONITOR
#define MQS_BOOT_MONITOR

#include "dump.h"
#include <utils/threads.h>

namespace android {

// for safe mode
#define CACHE_SAFE_MODE_PATH        "/cache/recovery/last_safemode"
#define RESCUE_SAFE_MODE_PATH       "/mnt/rescue/recovery/last_safemode"

// for maintenace mode
#define MAINTENANCE_MODE_PATH    "/cache/recovery/last_maintenance"
#define RESCUE_MAINTENANCE_MODE_PATH "/mnt/rescue/recovery/last_maintenance"

// for trap mode
#define CACHE_TRAP_MODE_PATH        "/cache/recovery/last_trap"
#define RESCUE_TRAP_MODE_PATH       "/mnt/rescue/recovery/last_trap"

// for boot count
#define BOOT_COUNT_PATH             "/cache/boot_flag"
#define RESCUE_BOOT_COUNT_PATH      "/mnt/rescue/boot_flag"

// for log save dir
#define BOOT_FAILED_LOG_PATH        "/cache/mqsas/"
#define RESCUE_BOOT_FAILED_LOG_PATH "/mnt/rescue/mqsas/"

// for unhealthy boot dir
#define UNHEALTHY_BOOT_LOG_PATH        "/cache/mqsas/UnhealthyBootLog/"
#define RESCUE_UNHEALTHY_BOOT_LOG_PATH "/mnt/rescue/mqsas/UnhealthyBootLog/"

class BootMonitorThread : public Thread {
    private:
        void captureBugreport(std::string& topdir, int bootCount, long long wait_time=0);
    protected:
        virtual bool threadLoop();
};

} // namespace android
#endif
