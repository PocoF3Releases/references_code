
#include "init.h"

static inline struct mem_config *get_mem_config()
{
    struct mem_config *config = get_loaded_mem_config();

    ALOGE("      reclaim_on_start: %d", config->reclaim_on_start);
    ALOGE("   swappiness_on_start: %d", config->swappiness_on_start);
    ALOGE("   reclaim_on_launcher: %d", config->reclaim_on_launcher);
    ALOGE("swappiness_on_launcher: %d", config->swappiness_on_launcher);

    return config;
}

int mem_init(struct sys_info *si)
{
    if (si->total_mem > MEM_NR) {
        ALOGE("total mem size is large than mem config array");
        return -1;
    }

    if (!loaded_config_available()) {
        ALOGE("loaded config not available");
        return -1;
    }
    struct mem_config *config = get_mem_config();

    set_int_prop_config_item("sys.mem.reclaim_on_start", config->reclaim_on_start);

    set_int_prop_config_item("sys.mem.swappiness_on_start", config->swappiness_on_start);

    set_int_prop_config_item("sys.mem.reclaim_on_launcher", config->reclaim_on_launcher);

    set_int_prop_config_item("sys.mem.swappiness_on_launcher", config->swappiness_on_launcher);

    return 0;
}


