#ifndef OCTVM_MEMCONTROL_PRIV_H
#define OCTVM_MEMCONTROL_PRIV_H

#include <stdint.h>

#include "octvm.h"
#include "memcontrol.h"
#include "drv/memctrl_drv.h"

#define CGROUP_BASE_PATH             "/sys/fs/cgroup"
#define CGROUP_MEMCTL_BASE_PATH      "/sys/fs/cgroup/memory"
#define CGROUP_MEMCTL_BASE_PATH_LEN  21

struct mc_cgroup_memory_stat {
    long long cache;
    long long rss;
    long long mapped_file;
    long long pgpgin;
    long long pgpgout;
    long long swap;
    long long pgfault;
    long long pgmajfault;
    long long inactive_anon;
    long long active_anon;
    long long inactive_file;
    long long active_file;
    long long memory_limit;
};
typedef struct mc_cgroup_memory_stat *cgroup_memory_stat;

int cgroup_new_group(struct task_cgroup *groupinfo);
int cgroup_add_tasks(char* groupname, int pids[], int cnt);
int cgroup_remove_tasks(char* groupname, int pids[], int cnt);
int cgroup_move_task(char* groupname, int pid);
int cgroup_remove_group(char* groupname);
int cgroup_get_tasks(char* groupname, int* pids, int* count);
int cgroup_set_anon_mem_limit(char* groupname, long long size_in_bytes);
int cgroup_set_anon_mem_softlimit(char* groupname, long long size_in_bytes);
int cgroup_get_group_stat(char* groupname, cgroup_memory_stat cgroup_stat);
long long cgroup_get_anon_mem_limit(char* groupname);
long long cgroup_get_anon_mem_softlimit(char* groupname);
long long cgroup_get_max_mem_usage(char* groupname);
long long cgroup_get_max_memsw_usage(char* groupname);
int cgroup_get_swappiness(char* groupname);
int cgroup_set_swappiness(char* groupname, int swappiness);
int cgroup_add_task_by_name(char *user_name, char *task_name, char *groupname);
void cgroup_dump_print(int fd, char *groupname);

#endif
