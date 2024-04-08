#ifndef MC_UTILS_H
#define MC_UTILS_H

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include "freezeCgroupPool.h"

#define MAX_PROC_LINE_LEN  512
#define PROC_MEMINFO "/proc/meminfo"

#define append(p, ...)                    \
{                                         \
    int pr_size;                          \
    pr_size = sprintf(p, __VA_ARGS__);    \
    p += pr_size;                         \
}

#define str_equal2(str1, str2) \
        (strcmp(str1, str2) == 0)

#define str_equal3(str, tstr_start, tstr_len)  \
        ((strlen(str) == tstr_len) && (strncmp(str, tstr_start, tstr_len) == 0))

#define STRING_TMP_LEN 128
static inline char *str_add_prefix(char *prefix, char *str)
{
    int pr_size;
    static char tmp[STRING_TMP_LEN];
    pr_size = sprintf(tmp, "%s%s", prefix, str);
    tmp[pr_size] = '\0';
    return tmp;
}
static inline char *strdup2(const char *str_start, int str_len)
{
    char *new_str = (char *)malloc(str_len + 1);
    strncpy(new_str, str_start, str_len);
    *(new_str + str_len) = '\0';
    return new_str;
}
static inline char *strstr3(const char *s, const char *find, int find_len)
{
    const char *p = s;
    for (; (p=strchr(p, *find)) != NULL; p++) {
        if(strncmp(p,find,find_len)==0)
            return (char *)p;
    }
    return NULL;
}

typedef struct {
    char *buf;
    int size;
    int read_pos;
    int write_pos;
    int available;
}mc_ring_buffer;
/* init and allocate a ring buffer */
mc_ring_buffer *mc_ring_buffer_init(int size);
/* destroy and free the ring buffer */
void mc_ring_buffer_destroy(mc_ring_buffer *rb);
/* write the given data to the ring buffer */
int mc_ring_buffer_write(mc_ring_buffer *rb, void *_data, int len);
/* almost as mc_ring_buffer_write but write zero not given data */
int mc_ring_buffer_writezeros(mc_ring_buffer *rb, int len);
/* read from the ring buffer */
int mc_ring_buffer_read(mc_ring_buffer *rb, void *_data, int len);
/* read from the ring buffer, but not update the read pointer */
int mc_ring_buffer_read_persist(mc_ring_buffer *rb, void *_data, int len);
/* get the available data in the ring buffer */
int mc_ring_buffer_get_available(mc_ring_buffer *rb);
/* reallocate the ring buffer with new size, should larger */
int mc_ring_buffer_resize(mc_ring_buffer *rb, int new_size);
/* print ring buffer info*/
void mc_ring_buffer_show_info(mc_ring_buffer *rb);

#define time_msdiff(t1, t2) \
        (t2.tv_sec*1000 + t2.tv_usec/1000 - t1.tv_sec*1000 - t1.tv_usec/1000)

struct mc_sysinfo {
    struct sysinfo  sys_info;
    long cached;
    unsigned long total_swapcache_pages;
};

long long mc_get_swap_total();
long long mc_get_swap_free();
long long mc_get_mem_total();
unsigned long get_total_memory_KB();

/* print the system memory info
 * return the sysinfo structure pointer, which can be used if want detailed data
 */
struct mc_sysinfo *mc_print_system_info(int fd);

struct task_info {
    int pid;
    int ppid;
    char user[32];
    char state[2];
    unsigned long eip;
    unsigned long vsize;
    unsigned long rss;
    unsigned long rsslim;
    unsigned long wchan;
    unsigned long utime;
    unsigned long stime;
    int priority;
    int nice;
    int num_threads;
    long long starttime;
    unsigned long nswap;
    unsigned long pss;
    union {
        char cmdline[128];
        char name[128];
    }u;
    char cgroup[32];
};

int mc_childthreads(int pid, int* childpids, int* count);
/*memset taskinfo with 0 before call*/
int mc_get_task_info(int pid, int tid, struct task_info* taskinfo);
/* get task pids through user and task_name*/
int mc_get_task_pids(char* username, char* task_name, int* pids);
/* get task ppid with a give pid */
int mc_get_task_ppid(int pid);
/* get task cgroup info*/
int mc_get_task_cgroup(int pid, int tid, char *groupname);
/* get all taskinfo in a give cgroup*/
int mc_get_cgroup_tasks(char *groupname, struct task_info *tasklist);
/* get process's oom_adj by pid */
int mc_get_task_oom_adj(int pid);
/* judge the process is zygote */
int mc_is_zygote_pid(int pid);
/* get task swap used info*/
int mc_get_task_swapinfo(int pid);
/* get task pss used info*/
int mc_get_task_pssinfo(int pid);

/*not suggest this way*/
int mc_get_task_pid(char* task_name);

/* get the freezer cgroup name for pid */
int  mc_get_task_freezer_cgroup(int pid, char *groupname);
/* get the freezer cgroup state for pid */
int  mc_get_task_freezer_cgroup_state(int pid);

#define MAX_BOOT_REASON_LEN  128
#define SYS_BOOT_INFO "/sys/bootinfo/powerup_reason"
#define SYS_POWEROFF_INFO "/sys/bootinfo/poweroff_reason"

/* get power up reason to check if kernel crashed*/
int mc_get_powerup_reason(char *powerup_reason);

/* get power off reason*/
int mc_get_poweroff_reason(char *poweroff_reason);

/* get system start time
 * return the seconds from the standard time base
 */
time_t mc_get_sys_start_time();
/* get string of the localtime in short, using local tzone*/
char * mc_get_localtime_short(time_t *ltime);

/* get the kernel version, full version store in full_info if not NULL */
char * mc_get_kernel_version(char *full_info);
/* get android version identify, different OEM may use different property */
char * mc_get_android_version(const char *name);

/* get the filename by fd, store in fn_buf, buf_len is the length of fn_buf
 * return success or not
 */
int mc_get_file_name(int fd, char *fn_buf, int buf_len);
void mc_delete_file(int fd);

int mc_get_task_pids_by_uid(int uid, int* pids);
/* 0 indicate Running, 1 indicate Sleep */
int fp_get_process_state(int pid);
int fp_get_process_state_sigpend(int pid, int *pstate, int *psigpend);
#endif
