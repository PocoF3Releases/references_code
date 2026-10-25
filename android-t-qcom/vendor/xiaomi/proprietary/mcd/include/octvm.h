#ifndef __OCTVM_SW_PLATFORM__
#define __OCTVM_SW_PLATFORM__

#include <stdio.h>
#include <stdint.h>

#include <cutils/properties.h>
#include "drv/platform_power.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define PROC_MEMINFO    "/proc/meminfo"
#define TOMBSTONE       "/data/tombstones"
#define DROP_BOX        "/data/system/dropbox"
#define MCD_DATA_PATH   "/data/system/mcd"
#define RAMDUMP_PATH    "/data/system/whetstone/ramdump"
#define RAMDUMP_FLAG    "/data/system/whetstone/ramdump/ramdump.cpl"
////////////////////////////////////////////////////////////////////////////////////////////////////
// extra calling parameters for mcd
#define MCD_EXTRA_PARAMS "mcd.extra.params"
#define MAX_MCD_PARAMS_N 64

////////////////////////////////////////////////////////////////////////////////////////////////////
// VM core param settings
struct MemoryParams{
    uint32_t zram_device_num;
    uint32_t zram_total_size;
    uint32_t global_swappiness;
    uint32_t more_memory_swappiness;
};

struct PowerSaveParams{
    uint32_t default_autosave;
    struct autosave_params default_params;
};

extern unsigned long gTotalMemoryKB;
extern char gModel[PROPERTY_VALUE_MAX];
extern char gDevice[PROPERTY_VALUE_MAX];
extern char gModDevice[PROPERTY_VALUE_MAX];
extern char serialno[PROPERTY_VALUE_MAX];
extern struct MemoryParams *gVMMemoryParams;
extern struct PowerSaveParams *gVMPowerParams;

////////////////////////////////////////////////////////////////////////////////////////////////////
// CGroup Task definition

#define MAX_TASK_NAME_LEN   64
#define MAX_UID_NAME_LEN    32
#define MAX_GROUP_NAME_LEN  64

#define NOT_CHECK           0x00000
#define UID_NOTCHECK        NOT_CHECK
#define UID_CHECK           0x00001
#define TASKNAME_NOTCHECK   NOT_CHECK
#define TASKNAME_CONTAIN    0x00002
#define TASKNAME_EXACT      0x00004

struct task_item {
    uint32_t check_flag;
    char uid_name[MAX_UID_NAME_LEN];
    char task_name[MAX_TASK_NAME_LEN];
    struct task_item *next;
};
typedef struct task_item * task_ilist;

struct task_cgroup{
    char groupname[MAX_GROUP_NAME_LEN];
    uint32_t  priority;
    uint32_t  swappiness;
    uint32_t  mem_limit_in_mega;
    uint32_t  mem_soft_limit_in_mega;
    uint32_t  move_charge_at_immigrate;
    task_ilist  def_tasks;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Utils facilities

#if PLATFORM_SDK_VERSION <= 19
    #ifdef TEMP_FAILURE_RETRY
        #undef TEMP_FAILURE_RETRY
        #define TEMP_FAILURE_RETRY(exp) ({         \
            __typeof__(exp) _rc;                   \
            do {                                   \
                _rc = (exp);                       \
            } while (_rc == -1 && errno == EINTR); \
            _rc; })
    #endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// Debugging facilities

void klogging(int level, char *tag, const char *fmt, ...);

#define MAX_KLOG_LINE_LEN 512

#define KLOG_ERROR(tag,x...)   klogging(3, tag, x)
#define KLOG_WARNING(tag,x...) klogging(4, tag, x)
#define KLOG_NOTICE(tag,x...)  klogging(5, tag, x)
#define KLOG_INFO(tag,x...)    klogging(6, tag, x)
#define KLOG_DEBUG(tag,x...)   klogging(7, tag, x)

#ifndef LOCAL_DEBUG
#include <cutils/log.h>

#define LOG_NIDEBUG   0
#define printf(...)   ALOGI(__VA_ARGS__)
#define fprintf(x, ...)                 \
    do{                                 \
        if(x==stderr)                   \
            ALOGE(__VA_ARGS__);         \
        else                            \
            fprintf(x,__VA_ARGS__);     \
    }while(0)
#define ERROR(x...)   KLOG_ERROR("octvm", x)
#define INFO(x...)    KLOG_INFO("octvm", x)
#else
#define ERROR(x...)   fprintf(stderr, x)
#define INFO(x...)    printf(x)
#endif

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

#endif
