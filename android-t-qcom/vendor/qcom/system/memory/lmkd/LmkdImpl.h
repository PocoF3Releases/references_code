#ifndef ANDROID_LMKD_IMPL_H
#define ANDROID_LMKD_IMPL_H

#include <ILmkdStub.h>
#define LOG_TAG "lowmemorykiller"
#define PERFD_LIB  "libqti-perfd-client_system.so"
#define IOPD_LIB  "libqti-iopd-client_system.so"

#include <dlfcn.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <pwd.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <cutils/properties.h>
#include <cutils/sched_policy.h>
#include <cutils/sockets.h>
#include <liblmkd_utils.h>
#include <lmkd.h>
#include <log/log.h>
#include <log/log_event_list.h>
#include <log/log_time.h>
#include <statslog.h>
#include <private/android_filesystem_config.h>
#include <psi/psi.h>
#include <system/thread_defs.h>
#include <dlfcn.h>

#include <set>

#ifdef MIUI_VERSION
#include <utils/String8.h>
#include "MQSServiceManager.h"
using namespace android;
#define CAMERA_APP_ID "31000000285"
#endif

/*
 * Define LMKD_TRACE_KILLS to record lmkd kills in kernel traces
 * to profile and correlate with OOM kills
 */
#ifdef LMKD_TRACE_KILLS

#define ATRACE_TAG ATRACE_TAG_ALWAYS
#include <cutils/trace.h>

#define TRACE_KILL_START(pid) ATRACE_INT(__FUNCTION__, pid);
#define TRACE_KILL_END()      ATRACE_INT(__FUNCTION__, 0);

#else /* LMKD_TRACE_KILLS */

#define TRACE_KILL_START(pid) ((void)(pid))
#define TRACE_KILL_END() ((void)0)

#endif /* LMKD_TRACE_KILLS */

#ifndef __unused
#define __unused __attribute__((__unused__))
#endif

#define MEMCG_SYSFS_PATH "/dev/memcg/"
#define MEMCG_MEMORY_USAGE "/dev/memcg/memory.usage_in_bytes"
#define MEMCG_MEMORYSW_USAGE "/dev/memcg/memory.memsw.usage_in_bytes"
#define ZONEINFO_PATH "/proc/zoneinfo"
#define MEMINFO_PATH "/proc/meminfo"
#define VMSTAT_PATH "/proc/vmstat"
#define PROC_STATUS_TGID_FIELD "Tgid:"

#define PERCEPTIBLE_APP_ADJ 200

#define TRACE_MARKER_PATH "/sys/kernel/tracing/trace_marker"

/* OOM score values used by both kernel and framework */
#define OOM_SCORE_ADJ_MIN       (-1000)
#define OOM_SCORE_ADJ_MAX       1000

#ifdef QCOM_FEATURE_ENABLE
#define EXTEND_RECLAIM_SYS_PATH             "/sys/kernel/extend_reclaim/reclaim"
#define ZRAM_COMPACTION_RATIO               0.25
#define RECLAIM_GAIN_RATIO                  1.1
#define MEMORY_50M_IN_PAGES                 12800
#define MEMORY_300M_IN_PAGES                76800
#define MEMORY_800M_IN_PAGES                216370
#define CAMERA_ADAPTIVE_LMK_ADJ_LIMIT       2

enum EXTEND_RECLAIM_TYPE {
    EXTEND_RECLAIM_TYPE_BASE = 0,
    EXTEND_RECLAIM_TYPE_FILE = EXTEND_RECLAIM_TYPE_BASE,
    EXTEND_RECLAIM_TYPE_ANON,
    EXTEND_RECLAIM_TYPE_ALL,
    EXTEND_RECLAIM_TYPE_COUNT
};
#endif

#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
#define VISIBLE_APP_ADJ 100
#define STUB_PERCEPTIBLE_APP_ADJ 250
#define PREVIOUS_APP_ADJ 700
#define PRE_PREVIOUS_APP_ADJ 701
#define SERVICE_B_ADJ 800
#define CACHED_APP_ADJ 900
#define CACHED_APP_LMK_FIRST_ADJ 950
#define WMARK_NUMS (WMARK_NONE + 1)
#define BUF_MAX 256
#define DATA_INVALID_TIME_MS 2000
#define MI_UPDATE_KILL_REASON_BASE 200
#define MI_NEW_KILL_REASON_BASE 50

/* memory pressure policy*/
enum mi_vmpressure_policy {
    VMPRESS_POLICY_NONE = -1,
    VMPRESS_POLICY_GRECLAIM = 0, /*global reclaim*/
    VMPRESS_POLICY_GSRECLAIM, /*global strong reclaim*/
    VMPRESS_POLICY_PRECLAIM, /*process reclaim*/
    VMPRESS_POLICY_GRECLAIM_AND_PM, /*global reclaim and pm*/
    VMPRESS_POLICY_CHECK_IO, /*transfer to mi mms to check io*/
    VMPRESS_POLICY_CHECK_MEM, /*transfer to mi mms to check mem*/
    VMPRESS_POLICY_TO_MI_PM = 17, /*transfer to mi pm*/
    VMPRESS_POLICY_COUNT
};

enum mi_to_mms_event_type {
    NO_EVENT_TO_PM = 0,
    SWAP_LOW_AND_MEMNOAVAILABLE,
    SWAP_OK_MEMNOAVAILABLE,
    CONT_LOW_WMARK,
    WILL_THROTTLED,
};

enum mi_reclaim_type {
    GRECLAIM = 1,
    PRECLAIM = 2
};

/* sync with lmkd.cpp*/
static const char *ilevel_name[] = {
    "low",
    "medium",
    "critical",
#ifdef QCOM_FEATURE_ENABLE
    "super critical",
#endif
};

static const char *mms_event_name[] = {
    "no event to pm",
    "swap is low and mem isn't ok", /*swap is low and mem isn't ok*/
    "swap ok and mem isn't ok", /*swap ok and mem isn't ok*/
    "clwm", /*cont low wmark*/
    "cw", /* will be throttled*/
};

enum poll_window {
    IN_WINDOW = 0,
    BETWEEN_WINDOW,
    WINDOW_COUNT
};

enum reclaim_state {
    NO_RECLAIM = 0,
    KSWAPD_RECLAIM,
    DIRECT_RECLAIM,
    DIRECT_RECLAIM_THROTTLE,
    RECLAIM_TYPE_COUNT
};

enum vm_events {
    PSWAPIN_DELTAS = 0,
    PSWAPOUT_DELTAS,
    PGPGIN_DELTAS,
    PGPGOUT_DELTAS,
    PGPGOUTCLEAN_DELTAS,
    NR_DIRTIED_DELTAS,
    NR_WRITTEN_DELTAS,
    NR_DIRTY,
    NR_WRITEBACK,
    VM_EVENTS_TYPE_COUNT
};

struct memory_pressure_policy_pc {
    bool in_compaction;
    bool swap_is_low;
    bool cycle_after_kill;
    bool file_thrashing;
    bool file_critical_thrashing;
    int reclaim;
    int thrashing_limit;
    int thrashing_limit_pct;
    int thrashing_critical_pct;
    uint32_t events;
    int64_t thrashing;
    int64_t swap_low_threshold;
    int *min_score_adj;
    struct timespec *curr_tm;
    char *kill_desc;
    enum kill_reasons kill_reason;
    enum zone_watermark wmark;
    enum vmpressure_level level;
    union meminfo mi;
    struct zoneinfo zi;
    struct zone_meminfo zone_mem_info;
    struct psi_data psi_info;
};

struct mi_vmpressure_params {
    bool mi_polling_check;
    int cont_polling_count_in_window[VMPRESS_LEVEL_COUNT];
    int cont_triggered_count_between_window[VMPRESS_LEVEL_COUNT];
    int kswapd_reclaim_ratio[WINDOW_COUNT];
    int direct_reclaim_ratio[WINDOW_COUNT];
    int total_reclaim_ratio;
    int cont_reclaim_type_count_between_window[RECLAIM_TYPE_COUNT];
    int cont_wmark_count_between_window[WMARK_NUMS];
    int cont_wmark_count_in_window[WMARK_NUMS];
    int64_t kswapd_reclaim_deltas[WINDOW_COUNT];
    int64_t direct_reclaim_deltas[WINDOW_COUNT];
    int64_t vm_events[VM_EVENTS_TYPE_COUNT];
};

#endif


class LmkdImpl : public ILmkdStub {
#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
private:
        int     mi_reclaim_fd;
        bool    spt_mode;
        bool    game_mode;
        bool    mi_mms_switch;
        bool    mi_mms_log_switch;
        long    page_k;
        int32_t reclaim_switch;
        int32_t mi_mms_pre_previous_anonpages_thread;
        int32_t mi_mms_lowmem_pre_previous_anonpages_thread;
        int32_t mi_mms_mediummem_pre_previous_anonpages_thread;
        int32_t mi_mms_previous_anonpages_thread;
        int32_t mi_mms_lowermem_previous_anonpages_thread;
        int32_t mi_mms_lowmem_previous_anonpages_thread;
        int32_t mi_mms_mediummem_previous_anonpages_thread;
        int32_t mi_mms_perceptible_anonpages_thread;
        int32_t mi_mms_lowermem_perceptible_anonpages_thread;
        int32_t mi_mms_lowmem_perceptible_anonpages_thread;
        int32_t mi_mms_mediummem_perceptible_anonpages_thread;
        int32_t mi_mms_visible_anonpages_thread;
        int32_t mi_mms_lowermem_visible_anonpages_thread;
        int32_t mi_mms_lowmem_visible_anonpages_thread;
        int32_t mi_mms_mediummem_visible_anonpages_thread;
        int32_t mi_mms_critical_anonpages_thread;
        int32_t mi_mms_freemem_throttled_thread;
        int32_t mi_mms_cont_low_wmark_reclaim_thread;
        int32_t mi_mms_cached_memavailable_thread;
        int32_t mi_mms_lowmem_cached_memavailable_thread;
        int32_t mi_mms_mediummem_cached_memavailable_thread;
        int32_t mi_mms_lower_pre_previous_memavailable_thread;
        int32_t mi_mms_pre_previous_memavailable_thread;
        int32_t mi_mms_previous_memavailable_thread;
        int32_t mi_mms_lower_previous_memavailable_thread;
        int32_t mi_mms_perceptible_memavailable_thread;
        int32_t mi_mms_lower_perceptible_memavailable_thread;
        int32_t mi_mms_visible_memavailable_thread;
        int32_t mi_mms_visible_lowermem_memavailable_thread;
        int32_t mi_mms_visible_lowmem_memavailable_thread;
        int32_t mi_mms_visible_mediummem_memavailable_thread;
        int32_t mi_mms_critical_memavailable_thread;
        int32_t mi_mms_critical_lowmem_memavailable_thread;
        int32_t mi_mms_critical_mediummem_memavailable_thread;
        int32_t mi_mms_kill_subprocess_memavailable_thread;
        int32_t mi_mms_game_reclaim_memavailable_thread;
        int32_t mi_mms_cont_medium_event_thread;
        int32_t mi_mms_reclaim_ratio;
        int32_t mi_mms_system_pool_thread;
        int32_t mi_mms_system_pool_limit_thread;
        int32_t mi_mms_lowmem_system_pool_thread;
        int32_t mi_mms_lowmem_system_pool_limit_thread;
        int32_t mi_mms_filepage_thread;
        int32_t mi_mms_lowmem_filepage_thread;
        int32_t mi_mms_lowmem_wmark_boost_factor;
        int32_t mi_mms_game_wmark_boost_factor;
        int32_t mi_mms_cont_wmark_low_trigger_pm_thread;
        int32_t mi_mms_cont_wmark_low_thread;
        int32_t mi_mms_lowmem_cont_wmark_low_thread;
        int32_t mi_mms_cont_reclaim_kswapd_thread;
        int32_t mi_mms_cont_reclaim_direct_thread;
        int32_t mi_mms_cont_reclaim_direct_throttle_thread;
        int32_t mi_mms_poll_cont_critical_percentage;
        int32_t mi_mms_lowmem_swap_free_low_percentage;
        int32_t mi_mms_min_swap_free;
        int32_t mi_mms_file_thrashing_percentage;
        int32_t mi_mms_file_thrashing_critical_percentage;
        int32_t mi_mms_file_thrashing_page_thread;
        int32_t mi_mms_lowermem_thread;
        int32_t mi_mms_lowmem_thread;
        int32_t mi_mms_mediummem_thread;
        int32_t mi_mms_trigger_reclaim_mem_thread;
        union meminfo common_mi;
        struct mi_vmpressure_params mvmp;
#endif

private:
        int file_cache_to_adj(enum vmpressure_level __unused lvl, int nr_free, int nr_file);
        void updateCameraProp();
        void* find_process_from_list(int type, int adj_max, int adj_min);
#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
        int connect_mi_reclaim();
        void trigger_mi_reclaim(int pressure_level, int pressure_policy);
        void count_reclaim_rate(union vmstat *window_current, uint32_t events, int64_t *init_pgskip);
        void count_vm_events(union vmstat *vs);
        int64_t get_total_pgskip_deltas(union vmstat *vs, int64_t *init_pgskip);
        void count_cont_reclaim_type(int reclaim, struct timespec *curr_tm);
        void count_cont_wmark(uint32_t events, union meminfo *mi, struct zone_meminfo *zmi, struct timespec *curr_tm);
        bool reclaim_rate_ok(int polling_window);
        bool memavailable_is_low(union meminfo *mi, struct zoneinfo *zi, int64_t *memavailable);
        bool file_pages_is_low(union meminfo *mi);
        bool should_kill_subprocess(union meminfo *mi, struct zoneinfo *zi, int64_t *memavailable);
        int64_t get_memavailable(union meminfo *mi, struct zoneinfo *zi);
        int64_t get_anon_pages(union meminfo *mi);
        bool mem_meet_reclaim(union meminfo *mi);
        bool cont_event_memavailable_touch(struct memory_pressure_policy_pc *pc);
        bool pre_previous_memavailable_touch(struct memory_pressure_policy_pc *pc);
        bool previous_memavailable_touch(struct memory_pressure_policy_pc *pc);
        bool perceptible_memavailable_touch(struct memory_pressure_policy_pc *pc);
        bool visible_memavailable_touch(struct memory_pressure_policy_pc *pc);
        bool critical_memavailable_touch(struct memory_pressure_policy_pc *pc);
        int get_protected_adj(struct memory_pressure_policy_pc *pc);
        bool should_trigger_greclaim(uint32_t events, enum vmpressure_level level, union meminfo *mi);
        bool should_trigger_preclaim(uint32_t events, enum vmpressure_level level, union meminfo *mi, bool is_thrashing);
        void out_log_when_psi(uint32_t events, int lvl, int reclaim_type, union meminfo *mi,
                              bool swap_is_low, int64_t thrashing);
        bool cont_event_handle(uint32_t events, enum vmpressure_level level, enum mi_to_mms_event_type *event_to_mms);
        void cmd_spt_mode(LMKD_CTRL_PACKET packet);
        void cmd_game_mode(LMKD_CTRL_PACKET packet);
        void low_ram_device_config();
        void game_mode_config();
#endif
public:
        virtual ~LmkdImpl() {}
        virtual enum vmpressure_level upgrade_vmpressure_event(enum vmpressure_level level);
        virtual void log_zone_watermarks(struct zoneinfo *zi, struct zone_watermarks *wmarks);
        virtual void log_meminfo(union meminfo *mi, enum zone_watermark wmark);
        virtual void fill_log_pgskip_stats(union vmstat *vs, int64_t *init_pgskip, int64_t *pgskip_deltas);
        virtual bool have_psi_events(struct epoll_event *evt, int nevents);
        virtual void check_cont_lmkd_events(int lvl);
        virtual long proc_get_size(int pid);
        virtual int zone_watermarks_ok(enum vmpressure_level level);
        virtual long proc_get_script(void);
        virtual void update_perf_props(void);
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
        virtual bool mi_special_case(enum vmpressure_level level, int *min_score_adj,union meminfo mi, struct zoneinfo zi);
        virtual void mi_count_cont_psi_event(uint32_t events, enum vmpressure_level lvl, struct timespec *curr_tm);
        virtual void mi_do_early(union vmstat vs, uint32_t events, int64_t *pgskip_deltas, union meminfo mi,
                           enum vmpressure_level level);
        virtual void mi_update_props();
        virtual void poll_params_reset(struct polling_params *poll_params);
        virtual void mi_memory_pressure_policy(struct memory_pressure_policy_pc *pc);
#endif
};

enum check_cmd {
    START_CHECK_LIST = 0,   /* Start to check list for white list and black list */
    STOP_CHECK_LIST,        /* Stop to check all list */
    CLEAR_WHITE_LIST,       /* Clear white list */
    CLEAR_BLACK_LIST,       /* Clear black list */
};

enum CAMERA_LIST_TYPE {
    SET_BALCK_LIST = 0,
    SET_PROTECT_LIST,
    SET_PERCEPTIBLE_LIST,
    SET_WHITE_LIST,
    CAM_LIST_MAX_COUNT,
};

extern "C" ILmkdStub* create();
extern "C" void destroy(ILmkdStub* impl);

#endif
