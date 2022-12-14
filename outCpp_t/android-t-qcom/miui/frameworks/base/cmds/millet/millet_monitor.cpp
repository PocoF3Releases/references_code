#include "millet.h"
#include "milletApi.h"
#include "perf/perf_millet.h"

extern void register_hook_perf_millet();
void monitor_init(void)
{
    register_mod_hook(dump_msg, O_TYPE, O_POLICY);
    register_hook_perf_millet();
}
