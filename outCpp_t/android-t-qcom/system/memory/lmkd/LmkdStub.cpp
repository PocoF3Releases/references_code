#include <iostream>
#include <dlfcn.h>
#include "LmkdStub.h"
#include <log/log.h>

void* LmkdStub::LibLmkdImpl = NULL;
ILmkdStub* LmkdStub::ImplInstance = NULL;
std::mutex LmkdStub::StubLock;
bool LmkdStub::inited = false;

#define OOM_SCORE_ADJ_MAX       1000

ILmkdStub* LmkdStub::GetImplInstance() {
        std::lock_guard<std::mutex> lock(StubLock);
        if (ImplInstance == NULL && !inited) {
                if (!LibLmkdImpl) {
                    LibLmkdImpl = dlopen(LIBPATH, RTLD_LAZY);
                }
                if (LibLmkdImpl) {
                        Create* create = (Create *)dlsym(LibLmkdImpl, "create");
                        ImplInstance = create();
                }else{
                        ALOGE("dlopen %s failed,with error:%s",LIBPATH,dlerror());
               }
               inited = true;
        }

        return ImplInstance;
}

void LmkdStub::DestroyImplInstance() {
        std::lock_guard<std::mutex> lock(StubLock);
        if (LibLmkdImpl) {
                Destroy* destroy = (Destroy *)dlsym(LibLmkdImpl, "destroy");
                destroy(ImplInstance);
                dlclose(LibLmkdImpl);
                LibLmkdImpl = NULL;
                ImplInstance = NULL;
        }
}

enum vmpressure_level LmkdStub::upgrade_vmpressure_event(enum vmpressure_level level)
{
        enum vmpressure_level ret = VMPRESS_LEVEL_LOW;
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
                ret = Istub->upgrade_vmpressure_event(level);
        }
        return ret;
}

void LmkdStub::log_zone_watermarks(struct zoneinfo *zi, struct zone_watermarks *wmarks)
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
                Istub->log_zone_watermarks(zi, wmarks);
        }
}

void LmkdStub::log_meminfo(union meminfo *mi, enum zone_watermark wmark)
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
                Istub->log_meminfo(mi, wmark);
        }
}

void LmkdStub::fill_log_pgskip_stats(union vmstat *vs, int64_t *init_pgskip, int64_t *pgskip_deltas)
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
                Istub->fill_log_pgskip_stats(vs, init_pgskip, pgskip_deltas);
        }
}

bool LmkdStub::have_psi_events(struct epoll_event *evt, int nevents)
{
        bool ret = false;
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
                ret = Istub->have_psi_events(evt, nevents);
        }
        return ret;
}

void LmkdStub::check_cont_lmkd_events(int lvl)
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
                Istub->check_cont_lmkd_events(lvl);
        }
}

long LmkdStub::proc_get_size(int pid)
{
        long ret = -1;
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
                ret = Istub->proc_get_size(pid);
        }
        return ret;
}

int LmkdStub::zone_watermarks_ok(enum vmpressure_level level)
{
        int ret = OOM_SCORE_ADJ_MAX + 1;
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
                ret = Istub->zone_watermarks_ok(level);
        }
        return ret;
}

long LmkdStub::proc_get_script(void)
{
        long ret = -1;
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
                ret = Istub->proc_get_script();
        }
        return ret;
}

void LmkdStub::update_perf_props()
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
                Istub->update_perf_props();
        }
}

void* LmkdStub::proc_get_heaviest_extend(int oomadj)
{
        void* ret = NULL;
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
               ret = Istub->proc_get_heaviest_extend(oomadj);
        }
        return ret;
}

bool LmkdStub::ext_command_handler(int cmd, LMKD_CTRL_PACKET packet, int nargs)
{
        bool ret = false;
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
               ret = Istub->ext_command_handler(cmd, packet, nargs);
        }
        return ret;
}

bool LmkdStub::is_protected_task(int oomadj, char* taskname)
{
        bool ret = false;
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
               ret = Istub->is_protected_task(oomadj, taskname);
        }
        return ret;
}

void LmkdStub::mem_report(int oomadj, struct sock_event_handler_info data_sock[])
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
               Istub->mem_report(oomadj, data_sock);
        }
}

#ifdef QCOM_FEATURE_ENABLE
void LmkdStub::extend_reclaim_init()
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
                Istub->extend_reclaim_init();
        } else {
            ALOGI("%s: Istub is NULL!", __func__);
        }
}

void LmkdStub::extend_reclaim_check(union meminfo *mi)
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
               Istub->extend_reclaim_check(mi);
        } else {
            ALOGI("%s: Istub is NULL!", __func__);
        }
}
#endif

#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
void LmkdStub::mi_do_early(union vmstat vs, uint32_t events, int64_t *pgskip_deltas, union meminfo mi,
                           enum vmpressure_level level)
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
               Istub->mi_do_early(vs, events, pgskip_deltas, mi, level);
        }
}

void LmkdStub::mi_count_cont_psi_event(uint32_t events, enum vmpressure_level lvl, struct timespec *curr_tm)
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
               Istub->mi_count_cont_psi_event(events, lvl, curr_tm);
        }
}

void LmkdStub::mi_init(long page_size, union meminfo mi)
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
               Istub->mi_init(page_size, mi);
        }
}

void LmkdStub::mi_update_props()
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
               Istub->mi_update_props();
        }
}

void LmkdStub::poll_params_reset(struct polling_params *poll_params)
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
               Istub->poll_params_reset(poll_params);
        }
}

void LmkdStub::mi_memory_pressure_policy(struct memory_pressure_policy_pc *pc)
{
        ILmkdStub* Istub = GetImplInstance();
        if(Istub) {
               Istub->mi_memory_pressure_policy(pc);
        }
}
#endif
