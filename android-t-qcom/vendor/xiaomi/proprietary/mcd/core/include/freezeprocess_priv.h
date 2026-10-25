#ifndef OCTVM_FREEZERCONTROL_PRIV_H
#define OCTVM_FREEZERCONTROL_PRIV_H

#include <stdint.h>
#include "freezeCgroupPool.h"
#include "octvm.h"

#define CGROUP_BASE_PATH             "/sys/fs/cgroup"

#define CGROUP_FREEZER_BASE_PATH      "/sys/fs/cgroup/freezer"
#define CGROUP_FREEZER_BASE_PATH_LEN  22

#define CGROUP_FREEZER_PRINT(x...) ALOGW(x)
#define CGROUP_FREEZER_DEBUG

#define BUSY (0)
#define FREE (1)
#define INVALID_UID ((int) -1)
#define INVALID_TASKFD ((int)-1)
#define INVALID_STATEFD ((int) -1)
#define INVALID_POOL_INDEX ((int) -1)

#define FROZEN_ERROR_NUM_BASE_INNER            	                                      (-80)
#define OPEN_CGROUP_FREEZER_TASK_ERROR                                                (FROZEN_ERROR_NUM_BASE_INNER + 1)
#define CGROUP_FREEZER_TASK_NOT_EXIST_ERROR                                           (FROZEN_ERROR_NUM_BASE_INNER + 2)
#define ECHO_TO_CGROUP_FREEZER_TASKS_TOO_MANY_PIDS_ERROR                              (FROZEN_ERROR_NUM_BASE_INNER + 3)
#define ECHO_TO_CGROUP_FREEZER_TASKS_WRITE_FILE_ERROR                                 (FROZEN_ERROR_NUM_BASE_INNER + 4)
#define ECHO_TO_CGROUP_FREEZER_STATE_OPEN_FILE_ERROR                                  (FROZEN_ERROR_NUM_BASE_INNER + 5)
#define ECHO_TO_CGROUP_FREEZER_STATE_FILE_NOT_EXIST_ERROR                             (FROZEN_ERROR_NUM_BASE_INNER + 6)
#define READ_CGROUP_FREEZER_STATE_OPEN_FILE_ERROR                                     (FROZEN_ERROR_NUM_BASE_INNER + 7)
#define READ_CGROUP_FREEZER_STATE_READ_FILE_ERROR                                     (FROZEN_ERROR_NUM_BASE_INNER + 8)
#define READ_CGROUP_FREEZER_STATE_READ_INVALID_DATA_ERROR                             (FROZEN_ERROR_NUM_BASE_INNER + 9)
#define READ_CGROUP_FREEZER_STATE_FILE_NOT_EXIST_ERROR                                (FROZEN_ERROR_NUM_BASE_INNER + 10)
#define FP_UTILS_CANNOT_OPEN_PROC_FILE_ERROR                                          (FROZEN_ERROR_NUM_BASE_INNER + 11)
#define FP_UTILS_GET_PROC_OPEN_PID_STAT_ERROR                                         (FROZEN_ERROR_NUM_BASE_INNER + 12)
#define FP_UTILS_GET_PROC_READ_PID_STATLINE_ERROR                                     (FROZEN_ERROR_NUM_BASE_INNER + 13)

#endif
