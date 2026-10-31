#ifndef OCTVM_POWER_H
#define OCTVM_POWER_H

#include <stdint.h>
#include "klo_cpu_utils.h"
#include "klo_battery_utils.h"

#define PERCENT_DECREASE_TIME_SON    (5*60)
#define PERCENT_DECREASE_TIME_SOFF   (30*60)

enum {
    SCREEN_STATE_OFF = 0,
    SCREEN_STATE_DIM = 1,
    SCREEN_STATE_BRIGHT = 2,
};

enum {
    POWER_EVENT_OVERHEAT = 1,
    POWER_EVENT_BATTERY_DECREASE_SON = 2,
    POWER_EVENT_BATTERY_DECREASE_SOFF = 3,
    POWER_EVENT_CURRENT_HIGH_SON = 4,
    POWER_EVENT_CURRENT_HIGH_SOFF = 5,
};

struct battery_lifecycle_stats {
    //major power timing
    int32_t uptime; //unit: s
    int32_t deepsleep_time; //unit: ms
    int32_t screen_on_time; //unit: s

    //time in state of battery
    int32_t total_usb_charge_time;
    int32_t total_ac_charge_time;
    int32_t total_connected_time;
    int32_t total_discharge_time;

    //charge property of user
    int32_t charge_times;
    int32_t avg_charge_cycle_interval; //if no charge: 0, if only charge start: bootime-startCharge
    int32_t avg_charge_start_soc;
    int32_t avg_charge_end_soc;
    int32_t poweron_soc;
};

typedef struct system_cpu_stats {
    time_t  rec_time;
    int32_t set_size;
    int64_t total_cpu_time;
    struct task_cpu_stats *cpu_stats_array;
} * System_Cpu_Stats;

typedef struct system_cpufreq_stats {
    int32_t set_size;
    struct cpufreq_stats *cpufreq_stats_array;
} * System_Cpufreq_Stats;

struct sys_power_event {
    time_t rec_time;
    int32_t event_type;
    char * summary;
    int32_t uptime;
    int32_t deepsleep_time;
    int32_t screen_on_time;
    Battery_State b_state;
    System_Cpu_Stats all_task_cpu_stats;
    System_Cpufreq_Stats all_cpufreq_stats;
};
struct lifecycle_power_events {
    struct listnode list;
    struct sys_power_event event;
};

extern int8_t gScreenState;
extern int8_t gChargeState;
extern int8_t gHealthState;
extern int8_t gPluggedState;
extern int8_t gBatteryLevel;
extern struct battery_lifecycle_stats gBatteryStastics;

#endif
