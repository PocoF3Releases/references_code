
#include "init.h"

int main(void)
{
    struct sys_info *si;

    si = get_sysinfo();

    load_config(CONFIG_FILE_PATH, si);
    ALOGE("  load_config_ok");

    mem_init(si);
    ALOGE("  mem_init_ok");

    dex2oat_threads_init(si);
    ALOGE("  dex2oat_threads_init_ok");

    zram_init();
    ALOGE("  zram_init_ok");

    return 0;
}
