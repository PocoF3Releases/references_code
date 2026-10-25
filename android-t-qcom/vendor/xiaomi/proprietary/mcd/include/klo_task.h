#ifndef OCTVM_KLO_TASK_H
#define OCTVM_KLO_TASK_H

#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define MAX_FILE_PATH_LEN        256
#define MAX_FILE_LINE_LEN        1024
#define MAX_PACKAGE_NAME_LEN     128
#define MAX_PACKAGE_VERSION_LEN  128

typedef enum {
    APP_FAIL_NONE = 0,
    APP_FAIL_ANR = 1,
    APP_FAIL_CRASH = 2,
    //APP_FAIL_WTF = 4,
    NATIVE_FAIL_CRASH = 8,
    APP_FAIL_WATCHDOG = 16,
} AppFailType;

typedef enum {
    LOG_NONE = 0,
    LOG_FROM_LOGCAT_AMS = 1,
    LOG_FROM_VM_TRACE = 2,
    LOG_FROM_CRASH_STACK = 4,
} AppRequestLogType;

typedef struct _klo_task {
    /*magic 'K''L''O'*/
    int32_t magic;
    /*specific package name*/
    char *package_name;
    /*which kind of app fail to care*/
    AppFailType fail_types;
    /*which kind of log to obtain*/
    AppRequestLogType req_log_types;
    /*record save path*/
    char *record_file_path;
    /*record file name pattern*/
    char *record_fn_pattern;
    /*frequentness of report, unit:second*/
    int32_t report_interval;
    /*the smallest report size, unit:Byte*/
    int32_t report_size;
    /*the time klo module start to monitor*/
    time_t start_time;
} klo_task_t;

typedef enum {
    FILE_PLAIN_TEXT = 0,
    FILE_PLAIN_DATABASE = 1,
    FILE_BASE64 = 2,
} KLOFileType;

typedef enum {
    DUMP_RAW = 0,
    DUMP_DATABASE = 1,
    TRACK_RAW = 2,
    TRACK_DATABASE = 3,
} KLOFileAction;

typedef struct _klo_file_desc {
    char *absolute_path;
    KLOFileType type;
    KLOFileAction action;
    char *file_description;
} KLOFileDesc;

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

#endif
