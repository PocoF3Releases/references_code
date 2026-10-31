#ifndef __PERFORMANCE_INIT_H__
#define __PERFORMANCE_INIT_H__

#define LOG_TAG "PerfInit"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/swap.h>

#include <cutils/properties.h>
#include <linux/loop.h>
#include <log/log.h>

#include <logwrap/logwrap.h>
#include <utils/Log.h>

#include <chrono>
#include <thread>
#include <android-base/chrono_utils.h>

#include "cJSON/cJSON.h"


/* helper marco */
#define BYTE_TO_MB(s) ((s) >> 20)
#define MB_TO_BYTE(s) ((s) << 20)

#define write_format_to_path(path, fmt, ...) ({ \
    int _ret = 0;                               \
    int _fd = 0;                                \
    do {                                        \
        _fd = open(path, O_WRONLY);             \
    } while(_fd == -1 && errno == EINTR);       \
    if (_fd != -1) {                            \
       _ret = dprintf(_fd, fmt, ##__VA_ARGS__);   \
    } else {                                    \
        _ret = 1;                               \
    }                                           \
    _ret; })

#define roundup(x, y) (                 \
{                                       \
    const typeof(y) __y = y;            \
    (((x) + (__y - 1)) / __y) * __y;    \
}                                       \
)

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(*(x)))


/* device/sys info */
#define MALLOC_UNKNOWN -1
enum {
    MALLOC_JEMALLOC = 0,
    MALLOC_TYPE_NR
};

#define SDK_VERSION_UNKNOWN -1
#define SDK_VERSION_23 23 /* M */
#define SDK_VERSION_25 25 /* N 7.1 */
#define SDK_VERSION_26 26 /* O 8.0 */
#define SDK_VERSION_27 27 /* O 8.1 */
#define SDK_VERSION_28 28 /* P 9.0 */
#define SDK_VERSION_29 29 /* Q 10.0 */
#define SDK_VERSION_30 30 /* R 11.0 */

#define DEVICE_MODEL_UNKNOWN "unknown"
struct sys_info {
    char* product_name;
    int sdk_version;

    int malloc_type;
    int total_mem; /* GB */
    int total_data_size; /* MB */

    int nr_cpus;
    int nr_bg_cpus;
};


/* config file */
#define CONFIG_FILE_PATH "/system/etc/perfinit.conf"

enum {
    CONFIG_NON_AVAILABLE = 0,
    CONFIG_AVAILABLE = 1,
};

/* mem config */
enum {
    MEM_1G = 1,
    MEM_2G,
    MEM_3G,
    MEM_4G,
    MEM_5G,
    MEM_6G,
    MEM_7G,
    MEM_8G,
    MEM_9G,
    MEM_10G,
    MEM_11G,
    MEM_12G,
    MEM_NR = MEM_12G,
};

struct mem_config {
    int reclaim_on_start;
    int swappiness_on_start;

    int reclaim_on_launcher;
    int swappiness_on_launcher ;
};

/* dex2oat threads config */
enum {
    CPU_NR_1 = 1,
    CPU_NR_2,
    CPU_NR_3,
    CPU_NR_4,
    CPU_NR_5,
    CPU_NR_6,
    CPU_NR_7,
    CPU_NR_8,
    CPU_NR_9,
    CPU_NR_10,
    CPU_NRS = CPU_NR_10,
};

struct dex2oat_config {
    int threads ;
    int boot_threads ;
    int bg_threads ;
};

/* zram config */
struct zram_config {
    int swap_on;
    int disksize; /* MB */
    char* comp_algorithm;
    int swappiness;
    int max_comp_streams;
    int page_cluster;

    // extendable memory
    int extm_on;
    char* extm_file; /* we can use a file or a block dev */
    char* extm_dev;
    int extm_size; /* MB */
};

struct sys_config {
    struct mem_config mem;
    struct dex2oat_config dex2oat;
    struct zram_config zram;
};

struct sys_info *get_sysinfo();

int mem_init(struct sys_info *si);
int dex2oat_threads_init(struct sys_info *si);
int zram_init();

void load_config(const char* config_path, struct sys_info *si);
int loaded_config_available();
void set_int_prop_config_item(const char *prop, int config);
void set_int_prop_config_item_forcing(const char *prop, int config);
int get_int_prop_config_item(const char *prop, int default_val);
long long get_long_long_prop_config_item(const char *prop, long long default_val);
int write_fmt_data_to_file(const char *filename, const char* fmt, ...);
int wait_for_file(const char* filename, std::chrono::milliseconds timeout);

struct mem_config *get_loaded_mem_config();
struct dex2oat_config *get_loaded_dex2oat_config();
struct zram_config *get_loaded_zram_config();

#endif
