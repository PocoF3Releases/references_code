#ifndef OCTVM_BATTERY_UTILS_H
#define OCTVM_BATTERY_UTILS_H

#include <stdint.h>

#define POWER_SUPPLY_SYSFS_PATH "/sys/class/power_supply"
#define BATTERY_SYSFS_PATH  "/sys/class/power_supply/battery"

// must in sync with definitions in BatteryManager.java
enum BatteryStatusType {
    BATTERY_STATUS_ENUM_START = 0,
    BATTERY_STATUS_UNKNOWN = 1,
    BATTERY_STATUS_CHARGING = 2,
    BATTERY_STATUS_DISCHARGING = 3,
    BATTERY_STATUS_NOT_CHARGING = 4,
    BATTERY_STATUS_FULL = 5,
    BATTERY_STATUS_ENUM_END = 6,
};

// must in sync with definitions in BatteryManager.java
enum BatteryHealthType {
    BATTERY_HEALTH_ENUM_START = 0,
    BATTERY_HEALTH_UNKNOWN = 1,
    BATTERY_HEALTH_GOOD = 2,
    BATTERY_HEALTH_OVERHEAT = 3,
    BATTERY_HEALTH_DEAD = 4,
    BATTERY_HEALTH_OVER_VOLTAGE = 5,
    BATTERY_HEALTH_UNSPECIFIED_FAILURE = 6,
    BATTERY_HEALTH_COLD = 7,
    BATTERY_HEALTH_ENUM_END = 8,
};

//must in sync with definitions in BatteryManager.java
enum BatteryPlugType {
    BATTERY_PLUGGED_NONE = 0,
    BATTERY_PLUGGED_AC = 1,
    BATTERY_PLUGGED_USB = 2,
    BATTERY_PLUGGED_WIRELESS = 4,
    BATTERY_PLUGGED_ANY = BATTERY_PLUGGED_AC | BATTERY_PLUGGED_USB | BATTERY_PLUGGED_WIRELESS,
};

enum PowerSupplyType {
    POWER_SUPPLY_TYPE_UNKNOWN = 0,
    POWER_SUPPLY_TYPE_AC,
    POWER_SUPPLY_TYPE_USB,
    POWER_SUPPLY_TYPE_WIRELESS,
    POWER_SUPPLY_TYPE_BATTERY
};

struct battery_state {
    int32_t battery_soc;
    int32_t battery_current;
    int32_t battery_voltage;
    int32_t battery_status;
    int32_t battery_health;
    int32_t battery_temp;
};
typedef struct battery_state * Battery_State;

/* get battery temperature */
int32_t get_battery_temp(int32_t *temp);
int32_t get_battery_cool_temp(int32_t *cool_temp);
int32_t get_battery_warn_temp(int32_t *warn_temp);

/* get battery C/V/SOC */
int32_t get_battery_current_now(int32_t *current_now);
int32_t get_battery_voltage_now(int32_t *voltage_now);
int32_t get_battery_capacity(int32_t *capacity);

/* get battery status: Charging, Discharging, Full*/
int32_t get_battery_status(int32_t *status);
char * get_battery_status_text(int32_t status_enum);

/* get battery health: Good, Overheat, ...*/
int32_t get_battery_health(int32_t *health);
char * get_battery_health_text(int32_t health_enum);

/* get battery_state from uevent*/
int32_t get_battery_state(struct battery_state *battery_info);

/* get current online power supply type*/
int32_t get_online_powersupply(int32_t *ps_type);

#endif
