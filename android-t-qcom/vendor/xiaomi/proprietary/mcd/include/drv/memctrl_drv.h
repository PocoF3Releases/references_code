#ifndef OCTVM_MEMCTRL_DRV_H
#define OCTVM_MEMCTRL_DRV_H

#include <stdio.h>
#include <sys/types.h>
#include <cutils/log.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define PROC_SWAPS            "/proc/swaps"
#define ZRAM_CONFIG_PREFIX    "/sys/block/zram"
#define ZRAM_DEVICE_PREFIX    "/dev/block/zram"

#define MAGIC_SWAP_HEADER     "SWAPSPACE2"
#define MAGIC_SWAP_HEADER_LEN 10
#define MIN_PAGES             10

/* These need to be obtained from kernel headers. */
#define SWAP_FLAG_PREFER        0x8000
#define SWAP_FLAG_PRIO_MASK     0x7fff
#define SWAP_FLAG_PRIO_SHIFT    0
#define SWAP_FLAG_DISCARD       0x10000

/* This needs to be obtained from kernel headers. */
struct linux_swap_header {
    char            bootbits[1024]; /* Space for disklabel etc. */
    uint32_t        version;
    uint32_t        last_page;
    uint32_t        nr_badpages;
    unsigned char   sws_uuid[16];
    unsigned char   sws_volume[16];
    uint32_t        padding[117];
    uint32_t        badpages[1];
};

/*
 * zram driver level operation
 */
int supported_zram_devices();
int zram_try_config_devices(int total_size_MB, int devices_num);
int zram_swap_set_global_swappiness(int global_swappiness);
int zram_swap_extra_sysctl(int global_swappiness);
int zram_swap_activate(char *device_name);
int zram_swap_deactivate(char *device_name);

/*
 * cgroup driver level operation
 */
int cgroup_supported();
int cgroup_try_mount_cgroup();
int cgroup_try_mount_cgroup_freezer();
int cgroup_basic_config();
void freezeCGPoolinit();
int cgroup_basic_freezer_config();
int cgroup_freezer_supported();
#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

#endif
