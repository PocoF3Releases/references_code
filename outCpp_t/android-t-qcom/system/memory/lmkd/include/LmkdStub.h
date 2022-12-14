#ifndef ANDROID_LMKD_LMKDSTUB_H
#define ANDROID_LMKD_LMKDSTUB_H
#include <LmkdImpl.h>
#include <mutex>

#define LIBPATH "/system_ext/lib64/liblmkdimpl.so"

class LmkdStub {
private:
        LmkdStub() {}
        static void *LibLmkdImpl;
        static ILmkdStub *ImplInstance;
        static ILmkdStub *GetImplInstance();
        static void DestroyImplInstance();
        static std::mutex StubLock;
        static bool inited;
public:
        virtual ~LmkdStub() {}
        static enum vmpressure_level upgrade_vmpressure_event(enum vmpressure_level level);
        static void log_zone_watermarks(struct zoneinfo *zi, struct zone_watermarks *wmarks);
        static void log_meminfo(union meminfo *mi, enum zone_watermark wmark);
        static void fill_log_pgskip_stats(union vmstat *vs, int64_t *init_pgskip, int64_t *pgskip_deltas);
        static bool have_psi_events(struct epoll_event *evt, int nevents);
        static void check_cont_lmkd_events(int lvl);
        static long proc_get_size(int pid);
        static int zone_watermarks_ok(enum vmpressure_level level);
        static long proc_get_script(void);
        static void update_perf_props();
        static void* proc_get_heaviest_extend(int oomadj);
        static bool ext_command_handler(int cmd, LMKD_CTRL_PACKET packet, int nargs);
        static bool is_protected_task(int oomadj, char* taskname);
        static void mem_report(int oomadj, struct sock_event_handler_info data_sock[]);

#ifdef QCOM_FEATURE_ENABLE
        static void extend_reclaim_init();
        static void extend_reclaim_check(union meminfo *mi);
#endif

#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
        static void mi_init(long page_size, union meminfo mi);
        static bool mi_special_case(enum vmpressure_level level, int *min_score_adj,union meminfo mi, struct zoneinfo zi);
        static void mi_count_cont_psi_event(uint32_t events, enum vmpressure_level lvl, struct timespec *curr_tm);
        static void mi_do_early(union vmstat vs, uint32_t events, int64_t *pgskip_deltas, union meminfo mi,
                           enum vmpressure_level level);
        static void mi_update_props();
        static void poll_params_reset(struct polling_params *poll_params);
        static void mi_feature_cmd_handler(enum lmk_cmd, LMKD_CTRL_PACKET packet);
        static void mi_memory_pressure_policy(struct memory_pressure_policy_pc *pc);
#endif

};

#endif
