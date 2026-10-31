
#include "init.h"

static inline struct dex2oat_config *get_dex2oat_config()
{
    struct dex2oat_config *config = get_loaded_dex2oat_config();

    ALOGE("     dex2oat threads: %d", config->threads);
    ALOGE("boot dex2oat threads: %d", config->boot_threads);
    ALOGE("  bg dex2oat threads: %d", config->bg_threads);

    return config;
}

int dex2oat_threads_init(struct sys_info *si)
{
    if (si->sdk_version == SDK_VERSION_UNKNOWN ||
        si->malloc_type == MALLOC_UNKNOWN ||
        si->nr_bg_cpus == -1 ||
        si->nr_cpus == -1)
        return -1;

    if (!loaded_config_available()) {
        ALOGE("loaded config not available");
        return -1;
    }

    struct dex2oat_config *config = get_dex2oat_config();

    if (si->sdk_version >= SDK_VERSION_26) {
        set_int_prop_config_item("persist.dalvik.vm.dex2oat-threads",
                        config->threads);

        set_int_prop_config_item("system_perf_init.dex2oat-threads",
                        config->threads);

        set_int_prop_config_item("system_perf_init.bg-dex2oat-threads",
                        config->bg_threads);

        set_int_prop_config_item("system_perf_init.boot-dex2oat-threads",
                        config->boot_threads);
    }

    return 0;
}
