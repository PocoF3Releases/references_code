#ifndef OCTVM_ANDROID_EVENTS_H
#define OCTVM_ANDROID_EVENTS_H

#if (PLATFORM_SDK_VERSION >= 29)
#include <log/log.h>
#elif (PLATFORM_SDK_VERSION >= 19)
#include <log/log.h>
#include <log/logger.h>
#else
#include <cutils/log.h>
#include <cutils/logger.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* should sync with event-log-tags */
#define BATTERY_LEVEL                2722
#define BATTERY_STATUS               2723
#define POWER_SLEEP_REQUESTED        2724
#define POWER_SCREEN_BROADCAST_SEND  2725
#define POWER_SCREEN_BROADCAST_DONE  2726
#define POWER_SCREEN_BROADCAST_STOP  2727
#define POWER_SCREEN_STATE           2728
#define POWER_PARTIAL_WAKE_STATE     2729
#define BATTERY_DISCHARGE            2730
#define MIN_BATTERY_EVENT      BATTERY_LEVEL
#define MAX_BATTERY_EVENT      BATTERY_DISCHARGE

#define WATCHDOG                     2802

#define AM_FINISH_ACTIVITY           30001
#define AM_ANR                       30008
#define AM_ACTIVITY_LAUNCH_TIME      30009
#define AM_PROC_DIE                  30011
#define AM_DESTROY_ACTIVITY          30018
#define AM_KILL                      30023
#define AM_CRASH                     30039

#define SCREEN_TOGGLED               70000

#define MAX_VERSION_LEN              128
#define MAX_PACKAGE_NAME_LEN         128
#define MAX_INSTALL_PATH_LEN         256
#define ANDROID_PACKAGE_FILE         "/data/system/packages.xml"

typedef struct {
    char version_code[MAX_VERSION_LEN];
    char install_path[MAX_INSTALL_PATH_LEN];
} AndroidPackageInfo;

int get_package_info(const char* package_name, AndroidPackageInfo *info);

static inline uint32_t get4LE(const uint8_t* src)
{
    return src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
}
static inline uint64_t get8LE(const uint8_t* src)
{
    uint32_t low = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
    uint32_t high = src[4] | (src[5] << 8) | (src[6] << 16) | (src[7] << 24);
    return ((long long) high << 32) | (long long) low;
}

/* format: <priority:1><tag:N>\0<message:N>\0 *
 * tag str starts at buf->msg+1               */
#define plain_log_entry_tag_offset  1

int32_t get_event_pid(const unsigned char* entry_buffer);
int32_t get_event_tid(const unsigned char* entry_buffer);
int32_t get_event_tag_id(const unsigned char* entry_buffer);
int64_t get_event_timestamp(const unsigned char* entry_buffer);
int get_plain_log_entry_msg_offset(const unsigned char* entry_buffer);
int parse_plain_log_entry(const unsigned char* entry_buffer, char *msg_body);
int parse_binary_log_entry(const unsigned char* entry_buffer, char *msg_body);

char *get_event_name(int event_id);
int is_powermanager_event(int event_id);
int is_activity_manager_event(int event_id);
int is_activity_launchtime_event(int event_id);

int get_event_target_id(int event_id, char *event_msg);
int get_event_target_name(int event_id, char *event_msg, char *p_name_buf, int size);
char *get_event_desc_summary(int event_id, char *event_msg);
int get_event_target_state(int event_id, char *event_msg);

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

#endif
