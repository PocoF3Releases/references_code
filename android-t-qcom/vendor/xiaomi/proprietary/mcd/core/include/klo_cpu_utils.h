#ifndef OCTVM_CPU_UTILS_H
#define OCTVM_CPU_UTILS_H

#include <stdint.h>

#define CPUFREQ_SYSFS_PATH "/sys/devices/system/cpu/cpu0/cpufreq"

struct cpufreq_stats {
    uint32_t frequency;
    uint64_t time_in_state;
};

struct task_cpu_stats {
    int32_t pid;
    int64_t task_cpu_time;
};

/* the time at which the system booted */
int32_t get_boottime();
/* milliseconds when cpu running */
int32_t get_cpu_running_time();
/* seconds since boot */
int32_t get_uptime();
/* milliseconds when cpu in deep sleep */
int32_t get_cpu_deepsleep_time();
/* get the time cpu stay in a frequency; will allocate table internal */
int32_t get_cpu_time_in_stat(struct cpufreq_stats **table, int *table_size);
/* get the total cpu time in jiffies */
int64_t get_total_cpu_time();
/* get the cpu time in jiffies for a task*/
int64_t get_task_cpu_time(pid_t pid);
/* get all task cpu time in jiffies; will allocate table internal */
int32_t get_all_task_cpu_time(struct task_cpu_stats **table, int *table_size);
/* get total cpu cores in the AP subsystem */
int32_t get_total_cpus();
/* get the cpu time for all cores */
int32_t get_all_cpu_time(unsigned long *cpu_time_array, int cpus);

#endif
