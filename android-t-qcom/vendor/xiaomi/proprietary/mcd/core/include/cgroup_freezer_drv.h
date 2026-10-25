#ifndef OCTVM_CGROUP_FREEZER_DRV_H
#define OCTVM_CGROUP_FREEZER_DRV_H

#include <stdint.h>
#include "freezeCgroupPool.h"

int fp_echo_app_to_cgroup_task(struct cgroup_freezer_pool_item * pitem);
int fp_set_cgroup_freezer_state(struct cgroup_freezer_pool_item * pCgroup, int state);
int fp_get_cgroup_freezer_item_actual_state(struct cgroup_freezer_pool_item* pCgroup, int *pState);
int fp_cgroup_new_group(char * groupname);
int fp_clear_cgroup_task(struct cgroup_freezer_pool_item * pItem);

#endif
