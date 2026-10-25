#ifndef _MIUI_LOG_UTILS_H
#define _MIUI_LOG_UTILS_H

#include <string>

#define MQSASD_SOCKET "mqsasd"

#define CACHE_LOG_DIR    "/cache/mqsas"
#define RESUCE_LOG_DIR   "/mnt/rescue/mqsas"
#define KMSG_FILE        "android-kmsg.txt"
#define TAG              "miui_log_utils"
#define MAX_KMSG_SIZE    (2 * 1024 * 1024)
#define CACHE_TIME_FILE  "/cache/recovery/last_extension_parameters"
#define RESCUE_TIME_FILE "/mnt/rescue/recovery/last_extension_parameters"
#define EXTENSION_PARAMETERS_SYSTEM_TIME "get_system_time"

namespace android {
    namespace init {
        void save_reboot_log();
        void save_system_time_file();
        void monitor_boot_failed(const std::string& reboot_reason);
    }
}

#endif  /* _MIUI_LOG_UTILS_H */
