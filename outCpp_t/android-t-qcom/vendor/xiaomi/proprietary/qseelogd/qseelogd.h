#ifndef _QSEELOG_H_
#define _QSEELOG_H_

#define KERNEL_QSEELOG_NODE       "/proc/tzdbg/qsee_log"
#define KERNEL_TZLOG_NODE         "/proc/tzdbg/log"
#define SAVE_FILE_DIR             "/cache/qseelog"

#define TZLOG_PREFLX              "tzlog_"
#define TZLOG_FIRST_SAVE_FILE     SAVE_FILE_DIR "/tzlog_00"
#define TZLOG_SAVE_FILE_PREFIX    SAVE_FILE_DIR "/tzlog_"

#define QSEELOG_PREFIX            "qsee_log_"
#define QSEELOG_FIRST_SAVE_FILE   SAVE_FILE_DIR "/qsee_log_00"
#define QSEELOG_SAVE_FILE_PREFIX  SAVE_FILE_DIR "/qsee_log_"
#define PATH_MAX_LENGTH           128

#define MAX_FILE_NUM              10
#define SIZE_10MB                 0xA00000
#define MAX_FILE_SIZE             SIZE_10MB
#define MALL0C_SIZE               4096

#define PROP_SECUREBOOT           "ro.boot.secureboot"
#define PROP_DBGPOLICY            "ro.boot.dp"

/* secboot AP debug policy enable logs*/
#define SEC_DBG_ENABLE_LOGGING    (1 << 3)

#endif /* _QSEELOG_H_ */
