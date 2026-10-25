#ifndef ANDROID_LMKD_ILMKDSTUB_H
#define ANDROID_LMKD_ILMKDSTUB_H

#include <lmkd.h>


class ILmkdStub {

public:
        virtual ~ILmkdStub() {}
        virtual enum vmpressure_level upgrade_vmpressure_event(enum vmpressure_level level);
        virtual void log_zone_watermarks(struct zoneinfo *zi, struct zone_watermarks *wmarks);
        virtual void log_meminfo(union meminfo *mi, enum zone_watermark wmark);
        virtual void fill_log_pgskip_stats(union vmstat *vs, int64_t *init_pgskip, int64_t *pgskip_deltas);
        virtual bool have_psi_events(struct epoll_event *evt, int nevents);
        virtual void check_cont_lmkd_events(int lvl);
        virtual long proc_get_size(int pid);
        virtual int zone_watermarks_ok(enum vmpressure_level level);
        virtual long proc_get_script(void);
        virtual void update_perf_props();
        virtual void* proc_get_heaviest_extend(int oomadj);
        virtual bool ext_command_handler(int cmd, LMKD_CTRL_PACKET packet, int nargs);
        virtual bool is_protected_task(int oomadj, char* taskname);
        virtual void mem_report(int oomadj, struct sock_event_handler_info data_sock[]);

#ifdef QCOM_FEATURE_ENABLE
        virtual void extend_reclaim_init();
        virtual void extend_reclaim_check(union meminfo *mi);
#endif

#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
        virtual void mi_init(long page_size, union meminfo mi);
        virtual bool mi_special_case(enum vmpressure_level level, int *min_score_adj,union meminfo mi,
                                     struct zoneinfo zi);
        virtual void mi_count_cont_psi_event(uint32_t events, enum vmpressure_level lvl, struct timespec *curr_tm);
        virtual void mi_do_early(union vmstat vs, uint32_t events, int64_t *pgskip_deltas, union meminfo mi,
                                 enum vmpressure_level level);
        virtual void mi_update_props();
        virtual void poll_params_reset(struct polling_params *poll_params);
        virtual void mi_memory_pressure_policy(struct memory_pressure_policy_pc *pc);
#endif

};

typedef ILmkdStub* Create();
typedef void Destroy(ILmkdStub *);
#endif
