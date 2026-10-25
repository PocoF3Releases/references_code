#ifndef OCTVM_STABILITY_H
#define OCTVM_STABILITY_H

#include <stdint.h>
#include <cutils/list.h>

#include "mc_utils.h"
#include "klo_task.h"

#define MAX_KLO_LOG_LINE_LEN 1024
#define MAX_KLO_HEADER_SIZE  1024
#define MAX_KERNEL_BUF_SIZE  (1 << CONFIG_LOG_BUF_SHIFT)

#define MAX_STACK_BUF_SIZE   (5*1024)
#define MAX_ANR_TRACE_SIZE   (10*1024)
#define MAX_AMS_TRACE_SIZEA  (8*1024)
#define MAX_AMS_TRACE_SIZEC  (6*1024)
#define MAX_PSTORE_LOG_SIZE  (2*1024)
#define MAX_KLO_BUF_SIZE     MAX_ANR_TRACE_SIZE

#define MAX_KLO_LOGGING_TIME 10 // second

#define ANDROID_ANR_TRACE_PROP  "dalvik.vm.stack-trace-file"
#define KLO_RECORD_START_PROP   "persist.sys.klo.rec_start"

enum KLORecordType {
    RECORD_NONE = 0,
    RECORD_FOR_ANR = 1,
    RECORD_FOR_CRASH = 2,
    RECORD_FOR_KILLED = 4,
    //RECORD_FOR_WTF = 8,
    RECORD_FOR_WATCHDOG = 32,
};

enum KLOFileOP {
    KLO_FILE_CREAT = 0,
    KLO_FILE_WRITE = 1,
    KLO_FILE_TEST  = 2,
    KLO_FILE_READ  = 3,
};

#define MAX_RECENT_KLO_RECORD_NUM   5

typedef struct {
    uint8_t fail_type;
    char * proc_name;
    time_t timestamp;
    char * stack_trace;
    mc_ring_buffer *logcat_trace;
}KLORecord;

typedef struct {
    struct listnode sub_list; //should be the first member
    char *subtype_sumary;
    uint32_t total_count;
    uint32_t records_count;
    uint32_t records_tail;
    KLORecord *recent_records[MAX_RECENT_KLO_RECORD_NUM];
}KLORecordSubtypeEntry;

typedef struct {
    struct listnode list; //should be the first member
    uint8_t record_type;
    char * package_name;
    char * package_version;
    char * package_install_path;
    uint32_t total_count;
    struct listnode records_list;
}KLORecordEntry;

/*the list head for all klo entry*/
extern struct listnode sys_klo_list;

#endif
