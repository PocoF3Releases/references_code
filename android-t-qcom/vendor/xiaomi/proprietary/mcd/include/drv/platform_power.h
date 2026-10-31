#ifndef OCTVM_PLATFORM_POWER_DRV_H
#define OCTVM_PLATFORM_POWER_DRV_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int32_t sysfile_read_int32_t(const char *sysfile)
{
    int32_t val = -1;
    FILE *fp = fopen(sysfile, "r");
    if (fp == NULL) {
        fprintf(stderr, "open sys file %s for read error, %s\n", sysfile, strerror(errno));
    }
    else {
        fscanf(fp, "%d", &val);
        fclose(fp);
    }
    return val;
}

static inline int32_t sysfile_write_int32_t(const char *sysfile, int32_t val)
{
    int32_t ret = -1;
    FILE *fp = fopen(sysfile, "w");
    if (fp == NULL) {
        fprintf(stderr, "open sys file %s for write error, %s\n", sysfile, strerror(errno));
    }
    else {
        fprintf(fp, "%d", val);
        fclose(fp);
        ret = 0;
    }
    return ret;
}

/*
 * need ensure the out put length is small than buf_string length
 */
static inline int32_t sysfile_read_string(const char *sysfile, char *buf_string) {
    int32_t ret = -1;
    FILE *fp = fopen(sysfile, "r");
    if (fp == NULL) {
        fprintf(stderr, "open sys file %s for read error, %s\n", sysfile, strerror(errno));
    }
    else {
        fscanf(fp, "%s", buf_string);
        fclose(fp);
        ret = 0;
    }
    return ret;
}

static inline int32_t sysfile_write_string(const char *sysfile, char *buf_string) {
    int32_t ret = -1;
    FILE *fp = fopen(sysfile, "w");
    if (fp == NULL) {
        fprintf(stderr, "open sys file %s for write error, %s\n", sysfile, strerror(errno));
    }
    else {
        fprintf(fp, "%s", buf_string);
        fclose(fp);
        ret = 0;
    }
    return ret;
}

static inline int32_t sysfile_find_nodepath(char *full_path, char *dir_path, char *match_pattern, char * base_name)
{
    int32_t find = -1;
    DIR *dir;
    struct dirent *entry;
    dir = opendir(dir_path);
    if (dir == NULL) {
        return find;
    }

    find = 0;
    while((entry = readdir(dir)) != NULL) {
        if(strstr(entry->d_name, match_pattern) == entry->d_name) {
            strcpy(full_path, dir_path);
            strcat(full_path, entry->d_name);
            strcat(full_path, "/");
            strcat(full_path, base_name);
            find = 1;
        }
    }
    closedir(dir);
    return find;
}

// ----------------------------------------------------------------------------
/*
 * will allocate the array internal, need free array after use
 */
int32_t get_available_governors(char ***governors, const char *av_gov_path);
int32_t get_available_frequencies(int32_t **frequencies, const char *av_gov_path);

#define WAKE_LOCK      "/sys/power/wake_lock"
#define WAKE_UNLOCK    "/sys/power/wake_unlock"
#define BRIGHTNESS     "/sys/class/leds/lcd-backlight/brightness"
#define MAX_BRIGHTNESS "/sys/class/leds/lcd-backlight/max_brightness"

int32_t get_lcd_backlight_brightness();
int32_t get_lcd_backlight_maxbrightness();
int32_t set_lcd_backlight_brightness(int brightness);
int32_t acquire_kernel_wakelock(const char* wakelock);
int32_t release_kernel_wakelock(const char* wakelock);

#define GPU_LOAD_PATH "/sys/class/kgsl/kgsl-3d0/devfreq/gpu_load"
int32_t get_gpu_load(void);

// ----------------------------------------------------------------------------
#define CPUFREQ_NAME_LEN  16
#define NODE_PATH_MAX     128
#define USER_PROFILE_PATH "/data/system/mcd/"

#define CPU_STATE_NORMAL  0
#define CPU_STATE_IDLE    1
#define CPU_STATE_BUSY    2

struct power_profile_struct {
    char* value;
    char* node;
};

struct power_tunable_params {
    //indicating which of below param is valid, can combined
    int valid_param_flag;

    //----cpu/gpu freq policy-----
    /*flag:0x1. freq in kHz */
    unsigned int cpufreq_min;
    /*flag:0x2. freq in kHz */
    unsigned int cpufreq_max;
    /*flag:0x4. governor string */
    char cpufreq_gov[CPUFREQ_NAME_LEN];
    /*flag:0x8. freq in Hz */
    unsigned int gpufreq_max;
    /*flag:0x10. governor string */
    char gpufreq_gov[CPUFREQ_NAME_LEN];

    //----some interactive tunables----
    /*flag:0x20. time in us */
    unsigned long min_sample_time;
    /*flag:0x40. time in us */
    unsigned long timer_rate;

    //----some core_ctrl_tunables----
    /*flag:0x80. min online cpus */
    unsigned int min_cpus;
    /*flag:0x100. max online cpus */
    unsigned int max_cpus;
    /*flag:0x200. time in ms */
    unsigned int offline_delay_ms;
    /*flag:0x400. tasks num should not less then core num */
    unsigned int task_thres;
};

struct autosave_params {
    unsigned long windowLength; // in ms
    int forceIdleOffPct;        // 1~100
    int forceBusyOffPct;        // 1~100
};

int32_t init_device_powerstate();
int32_t platform_set_cpu_powermode(const char *profile);
int32_t platform_set_cpu_custprofile(const char *profile_name);
int32_t platform_autosave_handle(unsigned long window_ms, unsigned int *cpu_states, int cpu_num);

int32_t platform_set_lcd_mode(const char *lcd_mode);
int32_t platform_set_thermal_mode(const char *thermal_mode);
int32_t platform_set_bcbc_cust_state(const char * cust_state);
int32_t platform_set_dfps_cust_state(const char * cust_state);

int32_t set_cpufreq_max(const char *freq_max);
int32_t set_gpufreq_max(const char *freq_max);
const char* get_cpu_param_path(char *param_name);

int32_t set_sysconfig_custprofile(const char *profile_path);

// ----------------------------------------------------------------------------

int32_t get_current_battery_uAh();
extern bool has_accuracy_battery_counter;

// ----------------------------------------------------------------------------

struct dfps_info_t {
    uint32_t min_fps;
    uint32_t max_fps;
};

extern struct dfps_info_t *s_dfps_info;

extern int32_t platform_set_dfps_mode(const char *dfps_mode);
extern void init_device_dfps_state(struct dfps_info_t **dfps_info);
extern void configure_dfps_enable(bool enable);
extern void configure_dfps_rate(uint32_t rate);

// ----------------------------------------------------------------------------

extern bool s_dfps_feature;

static inline bool get_dyn_refresh_rate_feature( ) {
    return s_dfps_feature;
}

static inline void set_dyn_refresh_rate_feature(bool enable) {
    if (s_dfps_feature == enable) {
        return;
    }
    if (enable) {
        s_dfps_feature = enable;
        platform_set_dfps_mode("low");
    } else {
        platform_set_dfps_mode("off");
        s_dfps_feature = enable;
    }
}

#ifdef __cplusplus
}
#endif

#endif
