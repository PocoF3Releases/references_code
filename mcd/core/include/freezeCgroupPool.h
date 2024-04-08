#ifndef OCTVM_FREEZECGROUPPOOL_H
#define OCTVM_FREEZECGROUPPOOL_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include "mc_utils.h"
#include "mc_plugintools.h"
#include <private/android_filesystem_config.h>
#include <cutils/list.h>
#include "freezeProcessApi.h"
#include <semaphore.h>

#define FROZEN 1
#define THAWED 0

struct cgroup_freezer_pool_item {
    char name[CGROUP_MAX_NAME_CHARS];   /* cgroup freezer item name */
    int task_fd;
    int state_fd;
    int root_task_fd;
    int pid[APP_MAX_PID_NUM];
    int pidCnt;
    int uid;
    int valid;
    int state;
    sem_t g_pool_item_sem;
};


#define FREEZE_CGROUPPOOL_LEN    64
#define FREEZE_CGROUPPOOL_DEBUGNAME "miui"
struct cgroup_freezer_pool {
    struct cgroup_freezer_pool_item poolArray[FREEZE_CGROUPPOOL_LEN];
    int freeItemNums;
};

void freezeCGPoolinit(void);
void dumpfpCgroupPool(void);
void freezeCGPoolinitCallbywhetstone(void);
struct cgroup_freezer_pool_item * getFreezeCgroup(void);
struct cgroup_freezer_pool_item * fp_find_cgroup_freezer_item_in_pool(int uid);
void fp_copy_app_entry_to_cgroup_entry(struct tfp_app_entity* app, struct cgroup_freezer_pool_item* pItem);
void fp_release_cgroup_freezer_pool_item(struct cgroup_freezer_pool_item * pFreezerItem);
int fp_check_cgroup_actual_state(int uid);
int fp_check_cgroup_frozen_but_thawed(int uid);
int fp_check_cgroup_state_frozen_if_thawed(int uid);
#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

#endif



