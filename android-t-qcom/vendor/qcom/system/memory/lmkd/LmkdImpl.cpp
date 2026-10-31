#include "LmkdImpl.h"
#include <pthread.h>
#define BPF_FD_JUST_USE_INT
#include "BpfSyscallWrappers.h"

extern void trace_kernel_buffer(const char *fmt, ...);
#define ULMK_LOG(X, fmt...) ({ \
    ALOG##X(fmt);              \
    trace_kernel_buffer(fmt);            \
})

#ifdef QCOM_FEATURE_ENABLE
// Be same with ModeConstant.java
#define MODE_CAMERA_OPEN            0x9F
#define MODE_RECORD_VIDEO           0xA2
#define EXTEND_RECLAIM_2G_PAGES     524288
#define EXTEND_RECLAIM_100M_PAGE    25000
#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(*(x)))
static float lowmem_psi[] =     {30.0, 20.0, 13.0, 8.0};
static int   lowmem_psi_adj[] = {51,  101,  250, 700};
static int lowmem_adj_free[] =     {701,    800,    900};
static int lowmem_minfree_free[] = {12800,  25600,  51200};
static bool double_watermark_enable = false;
static bool camera_adaptive_lmk_enable = false;
static int extend_reclaim_type = 0;
static int extend_reclaim_pages = 0;
static int extend_reclaim_fd = 0;
static bool extend_reclaim_enable = false;
static pthread_cond_t extend_reclaim_cond;
static pthread_mutex_t extend_reclaim_mutex;
enum EXTEND_RECLAIM_EVENT {
    EXTEND_RECLAIM_EVENT_CHECK,
    EXTEND_RECLAIM_EVENT_OPEN,
    EXTEND_RECLAIM_EVENT_VIDEO,
    EXTEND_RECLAIM_EVENT_COUNT
};
static int extend_reclaim_current_event;
static int extend_reclaim_event_count[EXTEND_RECLAIM_EVENT_COUNT];
typedef struct {
    int reclaim_type;
    int reclaim_pages;
    bool reclaim_request;
} extend_reclaim_event_t;
extend_reclaim_event_t extend_reclaim_event[EXTEND_RECLAIM_EVENT_COUNT];
static bool have_been_handle_first_request = false;

#endif
enum field_match_result {
    NO_MATCH,
    PARSE_FAIL,
    PARSE_SUCCESS
};
/*
 * Initialize this as we decide the window size based on ram size for
 * lowram targets on old strategy.
 */
#ifdef QCOM_FEATURE_ENABLE
/* PAGE_SIZE / 1024 */
static long page_k = PAGE_SIZE / 1024;
#else
static long page_k;
#endif

extern int level_oomadj[];
extern int lowmem_adj[];
extern int lowmem_minfree[];
extern int lowmem_targets_size;
extern bool s_crit_event;
static bool s_crit_event_upgraded = false;
static bool debug_process_killing;

static bool parse_int64(const char* str, int64_t* ret) {
    char* endptr;
    long long val = strtoll(str, &endptr, 10);
    if (str == endptr) {
        return false;
    }
    *ret = (int64_t)val;
    return true;
}

static int find_field(const char* name, const char* const field_names[], int field_count) {
    for (int i = 0; i < field_count; i++) {
        if (!strcmp(name, field_names[i])) {
            return i;
        }
    }
    return -1;
}

int LmkdImpl::file_cache_to_adj(enum vmpressure_level __unused lvl, int nr_free, int nr_file)
{
    int min_score_adj = OOM_SCORE_ADJ_MAX + 1;
    int minfree;
    int i;
    int crit_minfree;
    int s_crit_adj_level = level_oomadj[VMPRESS_LEVEL_SUPER_CRITICAL];

    /*
     * Below condition is to catch the zones where the file pages
     * are not allowed to, eg: Movable zone.
     * A corner case is where file_cache = 0 in the allowed zones
     * which is a very rare scenario.
     */
    if (!nr_file)
        goto out;

    for (i = 0; i < lowmem_targets_size; i++) {
        minfree = lowmem_minfree[i];
        if (nr_file < minfree) {
            min_score_adj = lowmem_adj[i];
            break;
        }
    }

    crit_minfree = lowmem_minfree[lowmem_targets_size - 1];
    if (lowmem_targets_size >= 2) {
        crit_minfree = lowmem_minfree[lowmem_targets_size - 1] +
                    (lowmem_minfree[lowmem_targets_size - 1] -
                    lowmem_minfree[lowmem_targets_size - 2]);
    }

    /* Adjust the selected adj in accordance with pressure. */
    if (s_crit_event && !s_crit_event_upgraded && (min_score_adj > s_crit_adj_level)) {
        min_score_adj = s_crit_adj_level;
    } else {
        if (s_crit_event_upgraded &&
                nr_free < lowmem_minfree[lowmem_targets_size -1] &&
                nr_file < crit_minfree &&
                min_score_adj > s_crit_adj_level) {
            min_score_adj = s_crit_adj_level;
        }
    }

out:
    /*
     * If event is upgraded, just allow one kill in that window. This
     * is to avoid the aggressiveness of kills by upgrading the event.
     */
    if (s_crit_event_upgraded)
           s_crit_event_upgraded = s_crit_event = false;
    ULMK_LOG(E, "adj:%d file_cache: %d\n", min_score_adj, nr_file);
    return min_score_adj;
}


extern int direct_reclaim_pressure;
extern union vmstat s_crit_base;
extern bool s_crit_event;
extern int vmstat_parse(union vmstat *vs);
extern int reclaim_scan_threshold;
extern bool last_event_upgraded;
static int count_upgraded_event;
enum vmpressure_level LmkdImpl::upgrade_vmpressure_event(enum vmpressure_level level)
{
    static union vmstat base;
    union vmstat current;
    int64_t throttle;
    static int64_t sync, async, pressure;

    switch (level) {
        case VMPRESS_LEVEL_LOW:
            if (vmstat_parse(&base) < 0) {
                ULMK_LOG(E, "Failed to parse vmstat!");
                goto out;
            }
            break;
        case VMPRESS_LEVEL_MEDIUM:
        case VMPRESS_LEVEL_CRITICAL:
            if (vmstat_parse(&current) < 0) {
                ULMK_LOG(E, "Failed to parse vmstat!");
                goto out;
            }
            throttle = current.field.pgscan_direct_throttle -
                    base.field.pgscan_direct_throttle;
           sync += (current.field.pgscan_direct -
                    base.field.pgscan_direct);
           async += (current.field.pgscan_kswapd -
                     base.field.pgscan_kswapd);
           /* Here scan window size is put at default 4MB(=1024 pages). */
           if (throttle || (sync + async) >= reclaim_scan_threshold) {
                   pressure = ((100 * sync)/(sync + async + 1));
                   if (throttle || (pressure >= direct_reclaim_pressure)) {
                           last_event_upgraded = true;
                           if (count_upgraded_event >= 4) {
                                   count_upgraded_event = 0;
                                   s_crit_event = true;
                                   ULMK_LOG(D, "Medium/Critical is permanently upgraded to Supercritical event\n");
                           } else {
                                  s_crit_event = s_crit_event_upgraded = true;
                                  ULMK_LOG(D, "Medium/Critical is upgraded to Supercritical event\n");
                           }
                           s_crit_base = current;
                   }
                   sync = async = 0;
             }

            base = current;
            break;
        default:
            ;
    }
out:
    return level;
}

void LmkdImpl::log_zone_watermarks(struct zoneinfo *zi, struct zone_watermarks *wmarks)
{
    int i, j;
    struct zoneinfo_node *node;
    union zoneinfo_zone_fields *zone_fields;

    for (i = 0; i < zi->node_count; i++) {
        node = &zi->nodes[i];

        for (j = 0; j < node->zone_count; j++) {
            zone_fields = &node->zones[j].fields;

            if (debug_process_killing) {
                ULMK_LOG(D, "Zone: %d nr_free_pages: %" PRId64 " min: %" PRId64
                         " low: %" PRId64 " high: %" PRId64 " present: %" PRId64
                         " nr_cma_free: %" PRId64 " max_protection: %" PRId64,
                         j, zone_fields->field.nr_free_pages,
                         zone_fields->field.min, zone_fields->field.low,
                         zone_fields->field.high, zone_fields->field.present,
                         zone_fields->field.nr_free_cma,
                         node->zones[j].max_protection);
            }
        }
    }

    if (debug_process_killing) {
        ULMK_LOG(D, "Aggregate wmarks: min: %ld low: %ld high: %ld",
                 wmarks->min_wmark, wmarks->low_wmark, wmarks->high_wmark);
    }
}

void LmkdImpl::log_meminfo(union meminfo *mi, enum zone_watermark wmark)
{
    char wmark_str[LINE_MAX];

    if (wmark == WMARK_MIN) {
        strlcpy(wmark_str, "min", LINE_MAX);
    } else if (wmark == WMARK_LOW) {
        strlcpy(wmark_str, "low", LINE_MAX);
    } else if (wmark == WMARK_HIGH) {
        strlcpy(wmark_str, "high", LINE_MAX);
    } else {
        strlcpy(wmark_str, "none", LINE_MAX);
    }

    if (debug_process_killing) {
        ULMK_LOG(D, "smallest wmark breached: %s nr_free_pages: %" PRId64
                 " active_anon: %" PRId64 " inactive_anon: %" PRId64
                 " cma_free: %" PRId64, wmark_str, mi->field.nr_free_pages,
                 mi->field.active_anon, mi->field.inactive_anon,
                 mi->field.cma_free);
    }
}

void LmkdImpl::fill_log_pgskip_stats(union vmstat *vs, int64_t *init_pgskip, int64_t *pgskip_deltas)
{
    unsigned int i;

    for (i = VS_PGSKIP_FIRST_ZONE; i <= VS_PGSKIP_LAST_ZONE; i++) {
        if (vs->arr[i] >= 0) {
            pgskip_deltas[PGSKIP_IDX(i)] = vs->arr[i] -
                                           init_pgskip[PGSKIP_IDX(i)];
        } else {
            pgskip_deltas[PGSKIP_IDX(i)] = -1;
        }
    }

    if (debug_process_killing) {
        ULMK_LOG(D, "pgskip deltas: DMA: %" PRId64 "DMA32: %" PRId64 " Normal: %" PRId64 " High: %"
                 PRId64 " Movable: %" PRId64,
                 pgskip_deltas[PGSKIP_IDX(VS_PGSKIP_DMA)],
                 pgskip_deltas[PGSKIP_IDX(VS_PGSKIP_DMA32)],
                 pgskip_deltas[PGSKIP_IDX(VS_PGSKIP_NORMAL)],
                 pgskip_deltas[PGSKIP_IDX(VS_PGSKIP_HIGH)],
                 pgskip_deltas[PGSKIP_IDX(VS_PGSKIP_MOVABLE)]);
    }

}

extern void mp_event_common(int data, uint32_t events, struct polling_params *poll_params);
bool LmkdImpl::have_psi_events(struct epoll_event *evt, int nevents)
{
       int i;
       struct event_handler_info* handler_info;

       for (i = 0; i < nevents; i++, evt++) {
               if (evt->events & (EPOLLERR | EPOLLHUP))
                       continue;
               if (evt->data.ptr) {
                       handler_info = (struct event_handler_info*)evt->data.ptr;
                       if (handler_info->handler == mp_event_common)
                               return true;
               }
       }

       return false;
}

extern long get_time_diff_ms(struct timespec *from, struct timespec *to);
extern int psi_window_size_ms;
void LmkdImpl::check_cont_lmkd_events(int lvl)
{
       static struct timespec tmed, tcrit, tupgrad;
       struct timespec now, prev;

       clock_gettime(CLOCK_MONOTONIC_COARSE, &now);

       if (lvl == VMPRESS_LEVEL_MEDIUM) {
               prev = tmed;
               tmed = now;
       }else {
               prev = tcrit;
               tcrit = now;
       }

       /*
        * Consider it as contiguous if two successive medium/critical events fall
        * in window + 1/2(window) period.
        */
       if (get_time_diff_ms(&prev, &now) < ((psi_window_size_ms * 3) >> 1)) {
               if (get_time_diff_ms(&tupgrad, &now) > psi_window_size_ms) {
                       if (last_event_upgraded) {
                               count_upgraded_event++;
                               last_event_upgraded = false;
                               tupgrad = now;
                       } else {
                               count_upgraded_event = 0;
                       }
               }
       } else {
               count_upgraded_event = 0;
       }
}

extern size_t read_all(int fd, char *buf, size_t max_len);

static bool parse_vmswap(char *buf, long *data) {

    if(sscanf(buf, "VmSwap: %ld", data) == 1)
        return 1;

    return 0;
}

static long proc_get_swap(int pid) {
    static char buf[PAGE_SIZE] = {0, };
    static char path[PATH_MAX] = {0, };
    ssize_t ret;
    char *c, *save_ptr;
    int fd;
    long data;

    snprintf(path, PATH_MAX, "/proc/%d/status", pid);
    fd = open(path,  O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return 0;

    ret = read_all(fd, buf, sizeof(buf) - 1);
    if (ret < 0) {
        ALOGE("unable to read Vm status");
        data = 0;
        goto out;
    }

    for(c = strtok_r(buf, "\n", &save_ptr); c;
        c = strtok_r(NULL, "\n", &save_ptr)) {
        if (parse_vmswap(c, &data))
            goto out;
    }

    ALOGE("Couldn't get Swap info. Is it kthread?");
    data = 0;
out:
    close(fd);
    /* Vmswap is in Kb. Convert to page size. */
    return (data >> 2);
}

extern long proc_get_rss(int pid);
long LmkdImpl::proc_get_size(int pid)
{
    long size;

    return (size = proc_get_rss(pid)) ? size : proc_get_swap(pid);
}

static long proc_get_vm(int pid)
{
    static char path[PATH_MAX];
    static char line[LINE_MAX];
    int fd;
    long total;
    ssize_t ret;

    /* gid containing AID_READPROC required */
    snprintf(path, PATH_MAX, "/proc/%d/statm", pid);
    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1)
        return -1;

    ret = read_all(fd, line, sizeof(line) - 1);
    if (ret < 0) {
        close(fd);
        return -1;
    }

    sscanf(line, "%ld", &total);
    close(fd);
    return total;
}

/*
 * no strtok_r since that modifies buffer and we want to use multiline sscanf
 */
static char *nextln(char *buf)
{
    char *x;

    x = static_cast<char*>(memchr(buf, '\n', strlen(buf)));
    if (!x)
        return buf + strlen(buf);
    return x + 1;
}

#define STRINGIFY(x) STRINGIFY_INTERNAL(x)
#define STRINGIFY_INTERNAL(x) #x

static int parse_one_zone_watermark(char *buf, struct watermark_info *w)
{
    char *start = buf;
    int nargs;
    int ret = 0;

    while (*buf) {
        nargs = sscanf(buf, "Node %*u, zone %" STRINGIFY(LINE_MAX) "s", w->name);
        buf = nextln(buf);
        if (nargs == 1) {
            break;
        }
    }

    while(*buf) {
        nargs = sscanf(buf,
                    " pages free %d"
                    " min %*d"
                    " low %*d"
                    " high %d"
                    " spanned %*d"
                    " present %d"
                    " managed %*d",
                    &w->free, &w->high, &w->present);
        buf = nextln(buf);
        if (nargs == 3) {
            break;
        }
    }

    while(*buf) {
        nargs = sscanf(buf,
                    " protection: (%d, %d, %d, %d, %d, %d)",
                    &w->lowmem_reserve[0], &w->lowmem_reserve[1],
                    &w->lowmem_reserve[2], &w->lowmem_reserve[3],
                    &w->lowmem_reserve[4], &w->lowmem_reserve[5]);
        buf = nextln(buf);
        if (nargs >= 1) {
            break;
        }
    }
    while(*buf) {
        nargs = sscanf(buf,
                    " nr_zone_inactive_anon %d"
                    " nr_zone_active_anon %d"
                    " nr_zone_inactive_file %d"
                    " nr_zone_active_file %d",
                    &w->inactive_anon, &w->active_anon,
                    &w->inactive_file, &w->active_file);
        buf = nextln(buf);
        if (nargs == 4) {
            break;
        }
    }

    while (*buf) {
        nargs = sscanf(buf, " nr_free_cma %u", &w->cma);
        buf = nextln(buf);
        if (nargs == 1) {
            ret = buf - start;
            break;
        }
    }

    return ret;
}

/*
 * Returns OOM_XCORE_ADJ_MAX + 1  on parsing error.
 */
extern char *reread_file(struct reread_data *data);
int LmkdImpl::zone_watermarks_ok(enum vmpressure_level level)
{
    static struct reread_data file_data = {
        .filename = ZONEINFO_PATH,
        .fd = -1,
    };
    char *buf;
    char *offset;
    struct watermark_info w[MAX_NR_ZONES];
    static union vmstat vs1, vs2;
    int zone_id, i, nr, present_zones = 0;
    bool lowmem_reserve_ok[MAX_NR_ZONES];
    int nr_file = 0;
    int min_score_adj = OOM_SCORE_ADJ_MAX + 1;

    if ((buf = reread_file(&file_data)) == NULL) {
        return min_score_adj;
    }

    memset(&w, 0, sizeof(w));
    memset(&lowmem_reserve_ok, 0, sizeof(lowmem_reserve_ok));
    offset = buf;
    /* Parse complete zone info. */
    for (zone_id = 0; zone_id < MAX_NR_ZONES; zone_id++, present_zones++) {
        nr = parse_one_zone_watermark(offset, &w[zone_id]);
        if (!nr)
            break;
        offset += nr;
    }
    if (!present_zones)
        goto out;

    if (vmstat_parse(&vs1) < 0) {
        ULMK_LOG(E, "Failed to parse vmstat!");
        goto out;
    }

    for (zone_id = 0, i = VS_PGSKIP_FIRST_ZONE;
            i <= VS_PGSKIP_LAST_ZONE && zone_id < present_zones; ++i) {
        if (vs1.arr[i] == -EINVAL)
            continue;
        /*
         * If no page is skipped while reclaiming, then consider this
         * zone file cache stats.
         */
        if (!(vs1.arr[i] - vs2.arr[i]))
            nr_file += w[zone_id].inactive_file + w[zone_id].active_file;

        ++zone_id;
    }

    vs2 = vs1;
    for (zone_id = 0; zone_id < present_zones; zone_id++) {
        int margin;

        ULMK_LOG(D, "Zone %s: free:%d high:%d cma:%d reserve:(%d %d %d) anon:(%d %d) file:(%d %d)\n",
                w[zone_id].name, w[zone_id].free, w[zone_id].high, w[zone_id].cma,
                w[zone_id].lowmem_reserve[0], w[zone_id].lowmem_reserve[1],
                w[zone_id].lowmem_reserve[2],
                w[zone_id].inactive_anon, w[zone_id].active_anon,
                w[zone_id].inactive_file, w[zone_id].active_file);

        /* Zone is empty */
        if (!w[zone_id].present)
            continue;

        margin = w[zone_id].free - w[zone_id].cma - w[zone_id].high;
        for (i = 0; i < present_zones; i++)
            if (w[zone_id].lowmem_reserve[i] && (margin > w[zone_id].lowmem_reserve[i]))
                lowmem_reserve_ok[i] = true;
        if (!s_crit_event && (margin >= 0 || lowmem_reserve_ok[zone_id]))
            continue;
        return file_cache_to_adj(level, w[zone_id].free, nr_file);
    }

out:
    if (offset == buf)
        ALOGE("Parsing watermarks failed in %s", file_data.filename);

    return min_score_adj;
}


static enum field_match_result match_field(const char* cp, const char* ap,
                                   const char* const field_names[],
                                   int field_count, int64_t* field,
                                   int *field_idx) {
    int i = find_field(cp, field_names, field_count);
    if (i < 0) {
        return NO_MATCH;
    }
    *field_idx = i;
    return parse_int64(ap, field) ? PARSE_SUCCESS : PARSE_FAIL;
}

/* /proc/meminfo parsing routines */
static bool meminfo_parse_line(char *line, union meminfo *mi) {
    char *cp = line;
    char *ap;
    char *save_ptr;
    int64_t val;
    int field_idx;
    enum field_match_result match_res;

    cp = strtok_r(line, " ", &save_ptr);
    if (!cp) {
        return false;
    }

    ap = strtok_r(NULL, " ", &save_ptr);
    if (!ap) {
        return false;
    }

    match_res = match_field(cp, ap, meminfo_field_names, MI_FIELD_COUNT,
        &val, &field_idx);
    if (match_res == PARSE_SUCCESS) {
        mi->arr[field_idx] = val / page_k;
    }
    return (match_res != PARSE_FAIL);
}

static int64_t read_gpu_total_kb() {
    static int fd = android::bpf::bpfFdGet(
            "/sys/fs/bpf/map_gpu_mem_gpu_mem_total_map", BPF_F_RDONLY);
    static constexpr uint64_t kBpfKeyGpuTotalUsage = 0;
    uint64_t value;

    if (fd < 0) {
        return 0;
    }

    return android::bpf::findMapEntry(fd, &kBpfKeyGpuTotalUsage, &value)
            ? 0
            : (int32_t)(value / 1024);
}

static int meminfo_parse(union meminfo *mi) {
    static struct reread_data file_data = {
        .filename = MEMINFO_PATH,
        .fd = -1,
    };
    char *buf;
    char *save_ptr;
    char *line;

    memset(mi, 0, sizeof(union meminfo));

    if ((buf = reread_file(&file_data)) == NULL) {
        return -1;
    }

    for (line = strtok_r(buf, "\n", &save_ptr); line;
         line = strtok_r(NULL, "\n", &save_ptr)) {
        if (!meminfo_parse_line(line, mi)) {
            ALOGE("%s parse error", file_data.filename);
            return -1;
        }
    }
    mi->field.nr_file_pages = mi->field.cached + mi->field.swap_cached +
        mi->field.buffers;
    mi->field.total_gpu_kb = read_gpu_total_kb();

    return 0;
}

/*
 * Allow lmkd to "find" shell scripts with oom_score_adj >= 0
 * Since we are not informed when a shell script exit, the generated
 * list may be obsolete. This case is handled by the loop in
 * find_and_kill_processes.
 */
#define PSI_PROC_TRAVERSE_DELAY_MS 200
extern struct proc *pid_lookup(int pid);
extern void proc_insert(struct proc *procp);
extern char *proc_get_name(int pid, char *buf, size_t buf_size);
long LmkdImpl::proc_get_script(void)
{
    static DIR* d = NULL;
    struct dirent* de;
    static char path[PATH_MAX];
    static char line[LINE_MAX];
    ssize_t len;
    int fd, oomadj = OOM_SCORE_ADJ_MIN;
    int r;
    uint32_t pid;
    long total_vm;
    long tasksize = 0;
    static bool retry_eligible = false;
    struct timespec curr_tm;
    static struct timespec last_traverse_time;
    static bool check_time = false;

    if(check_time) {
        clock_gettime(CLOCK_MONOTONIC_COARSE, &curr_tm);
        if (get_time_diff_ms(&last_traverse_time, &curr_tm) <
                PSI_PROC_TRAVERSE_DELAY_MS)
            return 0;
    }
repeat:
    if (!d && !(d = opendir("/proc"))) {
        ALOGE("Failed to open /proc");
        return 0;
    }

    while ((de = readdir(d))) {
        if (sscanf(de->d_name, "%u", &pid) != 1)
            continue;

        /* Don't attempt to kill init */
        if (pid == 1)
            continue;

        /*
        * Don't attempt to kill kthreads. Rely on total_vm for this.
        */
        total_vm = proc_get_vm(pid);
        if (total_vm <= 0)
            continue;

        snprintf(path, sizeof(path), "/proc/%u/oom_score_adj", pid);
        fd = open(path, O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            continue;

        len = read_all(fd, line, sizeof(line) - 1);
        close(fd);

        if (len < 0)
            continue;

        line[LINE_MAX - 1] = '\0';

        if (sscanf(line, "%d", &oomadj) != 1) {
            ALOGE("Parsing oomadj %s failed", line);
            continue;
        }

        if (oomadj < 0)
            continue;

        tasksize = proc_get_size(pid);
        if (tasksize <= 0)
            continue;

        retry_eligible = true;
        check_time = false;
        r = kill(pid, SIGKILL);
        if (r) {
            ALOGE("kill(%d): errno=%d", pid, errno);
            tasksize = 0;
        } else {
            ULMK_LOG(I, "Kill native with pid %u, oom_adj %d, to free %ld pages",
                            pid, oomadj, tasksize);
        }
        return tasksize;
    }
    closedir(d);
    d = NULL;
    if (retry_eligible) {
        retry_eligible = false;
        goto repeat;
    }
    check_time = true;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &last_traverse_time);
    ALOGI("proc_get_script: No tasks are found to kill");

    return 0;
}

#define PREFERRED_OUT_LENGTH 12288
extern char *preferred_apps;
extern void (*perf_ux_engine_trigger)(int, char *);
extern bool enable_preferred_apps;
static void init_PreferredApps() {
    void *handle = NULL;
    handle = dlopen(IOPD_LIB, RTLD_NOW);
    if (handle != NULL) {
        perf_ux_engine_trigger = (void (*)(int, char *))dlsym(handle, "perf_ux_engine_trigger");

        if (!perf_ux_engine_trigger) {
            ALOGE("Couldn't obtain perf_ux_engine_trigger");
            enable_preferred_apps = false;
        } else {
            // Initialize preferred_apps
            preferred_apps = (char *) malloc ( PREFERRED_OUT_LENGTH * sizeof(char));
            if (preferred_apps == NULL) {
                enable_preferred_apps = false;
            } else {
                memset(preferred_apps, 0, PREFERRED_OUT_LENGTH);
                preferred_apps[0] = '\0';
            }
        }
    }
}

#define PSI_POLL_PERIOD_SHORT_MS 10
#define DEF_THRASHING 30
#define DEF_THRASHING_DECAY 10
#define DEF_LOW_SWAP 10
extern int psi_cont_event_thresh;
extern bool enable_watermark_check;
extern bool kill_heaviest_task;
extern int psi_poll_period_scrit_ms;
extern int thrashing_limit_pct;
extern int thrashing_limit_decay_pct;
extern int swap_free_low_percentage;
extern int psi_partial_stall_ms;
extern bool use_minfree_levels;
extern bool force_use_old_strategy;
extern bool enhance_batch_kill;
extern bool enable_adaptive_lmk;
extern int wmark_boost_factor;
extern int wbf_effective;
extern int psi_complete_stall_ms;
extern bool enable_userspace_lmk;
extern bool use_inkernel_interface;
extern bool has_inkernel_module;
unsigned long kill_timeout_ms;
typedef struct {
     char value[PROPERTY_VALUE_MAX];
} PropVal;
void LmkdImpl::update_perf_props() {

    enable_watermark_check =
        property_get_bool("ro.lmk.enable_watermark_check", false);
    enable_preferred_apps =
        property_get_bool("ro.lmk.enable_preferred_apps", false);

     /* Loading the vendor library at runtime to access property value */
     PropVal (*perf_get_prop)(const char *, const char *) = NULL;
     void *handle = NULL;
     handle = dlopen(PERFD_LIB, RTLD_NOW);
     if (handle != NULL)
         perf_get_prop = (PropVal (*)(const char *, const char *))dlsym(handle, "perf_get_prop");

     if(!perf_get_prop) {
          ALOGE("Couldn't get perf_get_prop function handle.");
     } else {
          char property[PROPERTY_VALUE_MAX];
          char default_value[PROPERTY_VALUE_MAX];

          /*Currently only the following properties introduced by Google
          *are used outside. Hence their names are mirrored to _dup
          *If it doesnot get value via get_prop it will use the value
          *set by Google by default. To use the properties mentioned
          *above, same can be followed*/
          strlcpy(default_value, (kill_heaviest_task)? "true" : "false", PROPERTY_VALUE_MAX);
          strlcpy(property, perf_get_prop("ro.lmk.kill_heaviest_task_dup", default_value).value, PROPERTY_VALUE_MAX);
          kill_heaviest_task = (!strncmp(property,"false",PROPERTY_VALUE_MAX))? false : true;

          snprintf(default_value, PROPERTY_VALUE_MAX, "%lu", (kill_timeout_ms));
          strlcpy(property, perf_get_prop("ro.lmk.kill_timeout_ms_dup", default_value).value, PROPERTY_VALUE_MAX);
          kill_timeout_ms =  strtod(property, NULL);

          snprintf(default_value, PROPERTY_VALUE_MAX, "%d",
                    level_oomadj[VMPRESS_LEVEL_SUPER_CRITICAL]);
          strlcpy(property, perf_get_prop("ro.lmk.super_critical", default_value).value, PROPERTY_VALUE_MAX);
          level_oomadj[VMPRESS_LEVEL_SUPER_CRITICAL] = strtod(property, NULL);

          snprintf(default_value, PROPERTY_VALUE_MAX, "%d", PSI_POLL_PERIOD_SHORT_MS);
          strlcpy(property, perf_get_prop("ro.lmk.psi_poll_period_scrit_ms", default_value).value, PROPERTY_VALUE_MAX);
          psi_poll_period_scrit_ms = strtod(property, NULL);

          snprintf(default_value, PROPERTY_VALUE_MAX, "%d", reclaim_scan_threshold);
          strlcpy(property, perf_get_prop("ro.lmk.reclaim_scan_threshold", default_value).value, PROPERTY_VALUE_MAX);
          reclaim_scan_threshold = strtod(property, NULL);

          snprintf(default_value, PROPERTY_VALUE_MAX, "%d", DEF_THRASHING);
          strlcpy(property, perf_get_prop("ro.lmk.thrashing_threshold", default_value).value, PROPERTY_VALUE_MAX);
          thrashing_limit_pct = strtod(property, NULL);

          snprintf(default_value, PROPERTY_VALUE_MAX, "%d", DEF_THRASHING_DECAY);
          strlcpy(property, perf_get_prop("ro.lmk.thrashing_decay", default_value).value, PROPERTY_VALUE_MAX);
          thrashing_limit_decay_pct = strtod(property, NULL);

          snprintf(default_value, PROPERTY_VALUE_MAX, "%d", DEF_LOW_SWAP);
          strlcpy(property, perf_get_prop("ro.lmk.nstrat_low_swap", default_value).value, PROPERTY_VALUE_MAX);
          swap_free_low_percentage = strtod(property, NULL);

          snprintf(default_value, PROPERTY_VALUE_MAX, "%d", psi_partial_stall_ms);
          strlcpy(property, perf_get_prop("ro.lmk.nstrat_psi_partial_ms", default_value).value, PROPERTY_VALUE_MAX);
          psi_partial_stall_ms = strtod(property, NULL);

          snprintf(default_value, PROPERTY_VALUE_MAX, "%d", psi_complete_stall_ms);
          strlcpy(property, perf_get_prop("ro.lmk.nstrat_psi_complete_ms", default_value).value, PROPERTY_VALUE_MAX);
          psi_complete_stall_ms = strtod(property, NULL);

          strlcpy(default_value, (use_minfree_levels)? "true" : "false", PROPERTY_VALUE_MAX);
          strlcpy(property, perf_get_prop("ro.lmk.use_minfree_levels_dup", default_value).value, PROPERTY_VALUE_MAX);
          use_minfree_levels = (!strncmp(property,"false",PROPERTY_VALUE_MAX))? false : true;

          strlcpy(default_value, (force_use_old_strategy)? "true" : "false", PROPERTY_VALUE_MAX);
          strlcpy(property, perf_get_prop("ro.lmk.use_new_strategy_dup", default_value).value, PROPERTY_VALUE_MAX);
          force_use_old_strategy = (!strncmp(property,"false",PROPERTY_VALUE_MAX))? false : true;

          snprintf(default_value, PROPERTY_VALUE_MAX, "%d", PSI_CONT_EVENT_THRESH);
          strlcpy(property, perf_get_prop("ro.lmk.psi_cont_event_thresh", default_value).value,
                  PROPERTY_VALUE_MAX);
          psi_cont_event_thresh = strtod(property, NULL);

          /*The following properties are not intoduced by Google
           *hence kept as it is */
          strlcpy(property, perf_get_prop("ro.lmk.enhance_batch_kill", "true").value, PROPERTY_VALUE_MAX);
          enhance_batch_kill = (!strncmp(property,"false",PROPERTY_VALUE_MAX))? false : true;
          strlcpy(property, perf_get_prop("ro.lmk.enable_adaptive_lmk", "false").value, PROPERTY_VALUE_MAX);
          enable_adaptive_lmk = (!strncmp(property,"false",PROPERTY_VALUE_MAX))? false : true;
          strlcpy(property, perf_get_prop("ro.lmk.enable_userspace_lmk", "false").value, PROPERTY_VALUE_MAX);
          enable_userspace_lmk = (!strncmp(property,"false",PROPERTY_VALUE_MAX))? false : true;
          strlcpy(property, perf_get_prop("ro.lmk.enable_watermark_check", "false").value, PROPERTY_VALUE_MAX);
          enable_watermark_check = (!strncmp(property,"false",PROPERTY_VALUE_MAX))? false : true;
          strlcpy(property, perf_get_prop("ro.lmk.enable_preferred_apps", "false").value, PROPERTY_VALUE_MAX);
          enable_preferred_apps = (!strncmp(property,"false",PROPERTY_VALUE_MAX))? false : true;

          snprintf(default_value, PROPERTY_VALUE_MAX, "%d", wmark_boost_factor);
          strlcpy(property, perf_get_prop("ro.lmk.nstrat_wmark_boost_factor", default_value).value, PROPERTY_VALUE_MAX);
          wmark_boost_factor = strtod(property, NULL);
          wbf_effective = wmark_boost_factor;

          //Update kernel interface during re-init.
          use_inkernel_interface = has_inkernel_module && !enable_userspace_lmk;

          debug_process_killing = property_get_bool("ro.lmk.debug", false);

         updateCameraProp();
    }

    /* Load IOP library for PApps */
    if (enable_preferred_apps) {
        init_PreferredApps();
    }
}

#define ADJTOSLOT(adj) ((adj) + -OOM_SCORE_ADJ_MIN)

static bool camera_in_foreground;
extern struct adjslot_list procadjslot_list[];
extern int pid_remove(int);

static bool force_check_list = false;
static bool is_perceptible_support = false;
static std::set<std::string> camera_list[4];
static int perceptible_adj_thres[4] = {800, 800, 800, 800};

static unsigned long mi_mms_mem_report_interval_ms = 0;
static unsigned long mi_mms_onetrack_report_interval_ms = 0;
static unsigned long mi_mms_onetrack_report_nums = 0;
static int mi_mms_mem_report_adj_selected = 0;
static unsigned long mem_reclaim_interval_ms = 0;
static long mem_reclaim_threhold = 0;
static bool cam_mem_reclaim_switch = false;

void LmkdImpl::updateCameraProp() {
    char property[PROPERTY_VALUE_MAX];
    property_get("persist.sys.miui.camera.boost.killAdj_threshold", property, "800");
    const char *pattern  = ":";
    char *tmp= strtok(property, pattern);
    int index = 0;
    while(tmp && index < CAM_LIST_MAX_COUNT) {
        perceptible_adj_thres[index] = atoi(tmp);
        tmp = strtok(NULL, pattern);
        index++;
    }

    double_watermark_enable = property_get_bool("persist.sys.lmkd.double_watermark.enable", false);
    camera_adaptive_lmk_enable = property_get_bool("persist.sys.lmkd.camera_adaptive_lmk.enable", false);

}

/*
 * For LMK_FORCE_WHITELIST and LMK_FORCE_BLACKLIST  packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static void lmkd_pack_set_list(LMKD_CTRL_PACKET packet, enum CAMERA_LIST_TYPE type) {
    std::string strs((char*)(packet + 1));
    switch(type) {
        case SET_BALCK_LIST:
            camera_list[SET_BALCK_LIST].insert(strs);
            break;
        case SET_PROTECT_LIST:
            camera_list[SET_PROTECT_LIST].insert(strs);
            break;
        case SET_PERCEPTIBLE_LIST:
            camera_list[SET_PERCEPTIBLE_LIST].insert(strs);
            break;
        case SET_WHITE_LIST:
            camera_list[SET_WHITE_LIST].insert(strs);
            break;
        default :
            break;
    }

    if (debug_process_killing) {
        ALOGI("add %s to list type(%d)", strs.c_str(), type);
    }
}

/*
 * For LMK_CAMERA_MODE  packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static void cmd_camera_mode(LMKD_CTRL_PACKET packet) {
    is_perceptible_support = ntohl(packet[1]) == 2 ? true : false;
    camera_in_foreground = (ntohl(packet[1]) == 1 || ntohl(packet[1]) == 2) ? true : false;
    if (!camera_in_foreground) {
        have_been_handle_first_request = false;
    }
    if(debug_process_killing) {
        ALOGI("is_perceptible_support %d, camera_in_foreground %d", is_perceptible_support, camera_in_foreground);
    }
}

/*
 * set psi threshold for lmkd kill when camera is foreground
*/
static void lmkd_pack_set_psi(LMKD_CTRL_PACKET packet, int nargs) {
    int lens = (int)ARRAY_SIZE(lowmem_psi);
    if(nargs < lens) {
        ALOGW("%s,  cmd lens(%d) less than %d", __func__, nargs, lens);
    }
    for(int i = 0; i < lens; i++) {
        lowmem_psi[i] = ntohl(packet[i + 1]);
        if(debug_process_killing) {
            ALOGI("%s,  lowmem_psi[%d] = %f", __func__, i, lowmem_psi[i]);
        }
    }
}

#ifdef QCOM_FEATURE_ENABLE
/*
 * For LMK_RECLAIM_FOR_CAMERA  packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static void reclaim_mem_for_camera(LMKD_CTRL_PACKET packet) {
    if (!extend_reclaim_enable || extend_reclaim_fd < 0) {
        ALOGI("extend_reclaim disable! enable = %d,fd = %d", extend_reclaim_enable, extend_reclaim_fd);
        return;
    }

    union meminfo mi;
    int camera_mode = ntohl(packet[1]);
    int reclaim_type = -1, reclaim_pages = -1, reclaim_event = -1;
    if (meminfo_parse(&mi) < 0) {
        ALOGE("Failed to parse meminfo!");
        return;
    }

    ALOGI("camera_mode = %d, Active(anon) = %ld, Inactive(anon) = %ld, Active(file) = %ld, Inactive(file) = %ld", camera_mode, mi.field.active_anon, mi.field.inactive_anon, mi.field.active_file, mi.field.inactive_file);
    ALOGI("MemFree = %ld, Cached = %ld", mi.field.nr_free_pages, mi.field.cached);
    switch(camera_mode) {
        case MODE_CAMERA_OPEN:
            if (mi.field.nr_free_pages < EXTEND_RECLAIM_2G_PAGES) {
                reclaim_type = EXTEND_RECLAIM_TYPE_ALL;
                reclaim_pages = mi.field.inactive_anon + mi.field.inactive_file;
                reclaim_event = EXTEND_RECLAIM_EVENT_OPEN;
            }
            have_been_handle_first_request = true;
            break;
        case MODE_RECORD_VIDEO:
                reclaim_type = EXTEND_RECLAIM_TYPE_FILE;
                reclaim_pages = mi.field.inactive_file;
                reclaim_event = EXTEND_RECLAIM_EVENT_VIDEO;
            break;
        default:
            break;
        }

    if (reclaim_pages > 0) {
        pthread_mutex_lock(&extend_reclaim_mutex);
        extend_reclaim_event[reclaim_event].reclaim_type = reclaim_type;
        extend_reclaim_event[reclaim_event].reclaim_pages = reclaim_pages;
        extend_reclaim_event[reclaim_event].reclaim_request = true;
        pthread_mutex_unlock(&extend_reclaim_mutex);

        pthread_cond_signal(&extend_reclaim_cond);
    }
}
#endif

/*
 * For LMK_FORCE_BLACKLIST packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_force_check_list(LMKD_CTRL_PACKET packet) {
    enum check_cmd state = (enum check_cmd) ntohl(packet[1]);
    bool clearBlackList = false;
    bool clearWhiteList = false;
    switch(state) {
        case START_CHECK_LIST:
            force_check_list = true;
            break;
        case STOP_CHECK_LIST:
            force_check_list = false;
            break;
        case CLEAR_WHITE_LIST:
            clearWhiteList = true;
            break;
        case CLEAR_BLACK_LIST:
            clearBlackList = true;
            break;
        default:
            break;
    }
    if (clearWhiteList) {
        camera_list[SET_WHITE_LIST].clear();
        if(is_perceptible_support) {
            camera_list[SET_PROTECT_LIST].clear();
            camera_list[SET_PERCEPTIBLE_LIST].clear();
        }
    }
    if (clearBlackList) {
        camera_list[SET_BALCK_LIST].clear();
    }
}

extern struct sock_event_handler_info data_sock[];
static int ctrl_data_write(int dsock_idx, char* buf, size_t bufsz, struct sock_event_handler_info data_sock[]) {
    int ret = 0;

    ret = TEMP_FAILURE_RETRY(write(data_sock[dsock_idx].sock, buf, bufsz));

    if (ret == -1) {
        ALOGE("control data socket write failed; errno=%d", errno);
    } else if (ret == 0) {
        ALOGE("Got EOF on control data socket");
        ret = -1;
    }

    return ret;
}

static void ctrl_data_write_low_mem_occurred(struct sock_event_handler_info data_sock[]) {
    LMKD_CTRL_PACKET packet;
    size_t len = lmkd_pack_set_memreport(packet);

    for (int i = 0; i < MAX_DATA_CONN; i++) {
        if (data_sock[i].sock >= 0 && data_sock[i].async_event_mask & 1 << LMK_ASYNC_EVENT_KILL) {
            ALOGW("send memreport event to ams, i:%d", i);
            ctrl_data_write(i, (char*)packet, len, data_sock);
        }
    }
}
#define DMAINFO_PATH "/sys/kernel/debug/dma_buf/bufinfo"

/* bufinfo parsing routines */
static bool dmainfo_parse_line(char *line, uint64_t *camera_mem) {
    char *cp = line;
    char *save_ptr;
    int64_t field;

    cp = strtok_r(line, "\t", &save_ptr);
    if (!cp) {
        ALOGI("strtok_r error");
        return false;
    }

    if (parse_int64(cp, &field) == true) {
        *camera_mem += (uint64_t)(field/1024);
        return true;
    } else {
        ALOGI("camera parse fail");
        return false;
    }
}

static int dmainfo_parse() {
    uint64_t camera_app_mem, camera_server_mem, camera_provider_mem;
    uint64_t media_codec_mem, mediaserver_mem;
    static struct reread_data file_data = {
        .filename = DMAINFO_PATH,
        .fd = -1,
    };
    char *buf;
    char *save_ptr;
    char *line;

    camera_app_mem = camera_server_mem = camera_provider_mem = 0;
    media_codec_mem = mediaserver_mem = 0;

    if ((buf = reread_file(&file_data)) == NULL) {
        ALOGI("fail to read file data");
        return -1;
    }

    for (line = strtok_r(buf, "\n", &save_ptr); line;
         line = strtok_r(NULL, "\n", &save_ptr)) {
        if (strstr(line, "-.android.camera")) {
            if (!dmainfo_parse_line(line, &camera_app_mem)) {
                ALOGE("%s parse app error", file_data.filename);
                return -1;
            }
        }

        if (strstr(line, "cameraserver")) {
            if (!dmainfo_parse_line(line, &camera_server_mem)) {
                ALOGE("%s parse server error", file_data.filename);
                return -1;
            }
        }

        if (strstr(line, "provider@2.4-se")) {
            if (!dmainfo_parse_line(line, &camera_provider_mem)) {
                ALOGE("%s parse provider error", file_data.filename);
                return -1;
            }
        }

        if (strstr(line, "omx@1.0-service")) {
            if (!dmainfo_parse_line(line, &media_codec_mem)) {
                ALOGE("%s parse provider error", file_data.filename);
                return -1;
            }
        }

        if (strstr(line, "mediaserver")) {
            if (!dmainfo_parse_line(line, &mediaserver_mem)) {
                ALOGE("%s parse provider error", file_data.filename);
                return -1;
            }
        }
    }

    ALOGI("ion totalmem:%lu, camera app:%lu, provider:%lu, cameraserver:%lu, omx@1.0-service:%lu, mediaserver:%lu", camera_app_mem+camera_server_mem+camera_provider_mem+media_codec_mem+mediaserver_mem, camera_app_mem, camera_provider_mem, camera_server_mem, media_codec_mem, mediaserver_mem);

    return 0;
}

#if defined(MIUI_VERSION) && defined(ONETRACK_PRODUCT_FLAG)
void *pthread_onetrack_report(void *arg) {
    int arg_oomadj = *((int*)arg);
    ALOGE("lmkd_onetrack_report[2/3]: camera_in_foreground:%d, kill_oomadj:%d",camera_in_foreground, arg_oomadj);
    char jsonData[50];
    int jsonlen = 0;
    jsonlen  = sprintf(jsonData,"{\"EVENT_NAME\":lmkd_kill_pid_adj,\"oomadj\":%d}",arg_oomadj);
    int onetrackFlag = MQSServiceManager::getInstance().reportOneTrackEvent(String8(CAMERA_APP_ID),String8("lmkd"),String8(jsonData),0);
    if(0 != onetrackFlag) {
        ALOGE("lmkd_onetrack_report: reportOneTrackEvent failed");
    }
    else{
        ALOGE("lmkd_onetrack_report[3/3]: onetrack_report_nums:%d",++mi_mms_onetrack_report_nums);
    }
    return NULL;
}

void onetrack_report(int oomadj){
    ALOGE("lmkd_onetrack_report[1/3]: camera_in_foreground:%d, oomadj:%d",camera_in_foreground, oomadj);
    struct timespec onetrack_curr_tm;
    static struct timespec last_onetrack_report_tm;

    if(camera_in_foreground == true &&
    oomadj <= 700 &&
    clock_gettime(CLOCK_MONOTONIC_COARSE, &onetrack_curr_tm)==0 &&
    get_time_diff_ms(&last_onetrack_report_tm, &onetrack_curr_tm) > mi_mms_onetrack_report_interval_ms) {
        pthread_t Pid_onetrack;
        int ret = -1;
        ret = pthread_create(&Pid_onetrack, NULL, pthread_onetrack_report, &oomadj);
        if (ret < 0) {
            ALOGE("lmkd_onetrack_report: create_pthread failed");
            return;
        }
        pthread_join(Pid_onetrack,NULL);
        last_onetrack_report_tm = onetrack_curr_tm;
    }
}
#endif

void LmkdImpl::mem_report(int oomadj, struct sock_event_handler_info data_sock[]){
    struct timespec curr_tm;
    static struct timespec last_mem_report_tm;

    if (camera_in_foreground == true &&
    oomadj <= mi_mms_mem_report_adj_selected &&
    mi_mms_mem_report_interval_ms &&
    clock_gettime(CLOCK_MONOTONIC_COARSE, &curr_tm) == 0 &&
    get_time_diff_ms(&last_mem_report_tm, &curr_tm) > static_cast<long>(mi_mms_mem_report_interval_ms)){
        ALOGI("report mem time interval is %ld, mem_report_adj_selected is %d", static_cast<long>(mi_mms_mem_report_interval_ms), mi_mms_mem_report_adj_selected);
        ctrl_data_write_low_mem_occurred(data_sock);
        if (dmainfo_parse() < 0) {
            ALOGE("Failed to parse dmainfo!");
        }
        last_mem_report_tm = curr_tm;
        sleep(3);
    }
#if defined(MIUI_VERSION) && defined(ONETRACK_PRODUCT_FLAG)
    onetrack_report(oomadj);
#endif
}

#ifdef QCOM_FEATURE_ENABLE
void *extend_reclaim_work(void *arg) {
    int ret;
    cpu_set_t mask;
    int reclaim_pages = 0, remain_reclaim_pages = 0;
    int reclaim_type = EXTEND_RECLAIM_TYPE_ALL;
    char nr_to_reclaim_str[PROPERTY_VALUE_MAX] = {0};

    pthread_detach(pthread_self());

    CPU_ZERO(&mask);
    for (int i = 0; i < 4; i++) {
        CPU_SET(i, &mask);
    }

    pthread_setname_np(pthread_self(), "extend_reclaim");
    sched_setaffinity(0, sizeof(mask), &mask);

    while(1) {
        pthread_mutex_lock(&extend_reclaim_mutex);

        ALOGI("extend_reclaim wait!!!!");
        pthread_cond_wait(&extend_reclaim_cond, &extend_reclaim_mutex);

        if (extend_reclaim_fd == 0) {
            // Open at this time cause extend_reclaim.ko install after lmkd sometime.
            extend_reclaim_fd = open(EXTEND_RECLAIM_SYS_PATH, O_WRONLY);
            if (extend_reclaim_fd < 0) {
                ALOGE("Error opening %s; errno=%d", EXTEND_RECLAIM_SYS_PATH, errno);
                pthread_mutex_unlock(&extend_reclaim_mutex);
                goto exit;
            }
        }
        // Find highest priority
        for (int i = EXTEND_RECLAIM_EVENT_COUNT - 1; i >= 0; i--) {
            if(extend_reclaim_event[i].reclaim_request) {
                extend_reclaim_current_event = i;
            }
        }

do_reclaim:
        reclaim_type = extend_reclaim_event[extend_reclaim_current_event].reclaim_type;
        remain_reclaim_pages = extend_reclaim_event[extend_reclaim_current_event].reclaim_pages;
        // reset event request
        memset(extend_reclaim_event, 0, sizeof(extend_reclaim_event));
        pthread_mutex_unlock(&extend_reclaim_mutex);

        ALOGI("trigger extend_reclaim:  %d, %d", reclaim_type, remain_reclaim_pages);
        // Just reclaim 100M each time, then we can interrupt reclaim for higher request.
        do {
            reclaim_pages = remain_reclaim_pages > EXTEND_RECLAIM_100M_PAGE ? EXTEND_RECLAIM_100M_PAGE : remain_reclaim_pages;
            memset(nr_to_reclaim_str, 0, sizeof(nr_to_reclaim_str));
            snprintf(nr_to_reclaim_str, sizeof(nr_to_reclaim_str), "%d %d", reclaim_type, reclaim_pages);

            ret = write(extend_reclaim_fd, nr_to_reclaim_str, strlen(nr_to_reclaim_str));
            if (ret < 0) {
                ALOGE("Error write %s: \"%s\"", EXTEND_RECLAIM_SYS_PATH, nr_to_reclaim_str);
                // Will try open again next time.
                close(extend_reclaim_fd);
                extend_reclaim_fd = 0;
                break;
            }
            remain_reclaim_pages = remain_reclaim_pages - EXTEND_RECLAIM_100M_PAGE;

            // Higher priority request
            pthread_mutex_lock(&extend_reclaim_mutex);
            for (int i = EXTEND_RECLAIM_EVENT_COUNT - 1; i > extend_reclaim_current_event; i--) {
                if(extend_reclaim_event[i].reclaim_request) {
                    ALOGI("Higher priority request: %d, interrupt current request: %d", i, extend_reclaim_current_event);
                    extend_reclaim_current_event = i;
                    goto do_reclaim;
                }
            }
            pthread_mutex_unlock(&extend_reclaim_mutex);
        } while(remain_reclaim_pages > 0 && camera_in_foreground);

    }

exit:
    pthread_cond_destroy(&extend_reclaim_cond);
    pthread_mutex_destroy(&extend_reclaim_mutex);
    ALOGI("extend_reclaim thread exit!");
    return NULL;
}


void LmkdImpl::extend_reclaim_init() {
    int ret;
    pthread_t pth;

    extend_reclaim_enable = property_get_bool("persist.sys.lmkd.extend_reclaim.enable", false);
    ALOGI("extend_reclaim_enable = %d", extend_reclaim_enable);
    if (!extend_reclaim_enable) {
        return;
    }

    pthread_cond_init(&extend_reclaim_cond, NULL);
    pthread_mutex_init(&extend_reclaim_mutex, NULL);

    ret = pthread_create(&pth, NULL, extend_reclaim_work, NULL);
    if (ret < 0) {
        ALOGE("Error pthread_create; errno=%d", errno);
        return;
    }
    ALOGI("%s: successfully!", __func__);
}

void LmkdImpl::extend_reclaim_check(union meminfo *mi) {
    int nr_to_reclaim = 0;
    if (!extend_reclaim_enable || !camera_in_foreground || mi->field.free_swap < MEMORY_300M_IN_PAGES || extend_reclaim_fd < 0 ||
        !have_been_handle_first_request) {
        return;
    }

    ALOGI("%s: MemFree = %ld, SwapFree: %ld, Active(anon): = %ld, Inactive(anon): %ld, Active(file): = %ld, Inactive(file): %ld", __func__,
            mi->field.nr_free_pages, mi->field.free_swap,
            mi->field.active_anon, mi->field.inactive_anon,
            mi->field.active_file, mi->field.inactive_file);

    nr_to_reclaim = (lowmem_minfree[lowmem_targets_size - 1] - mi->field.nr_free_pages) / (1 - ZRAM_COMPACTION_RATIO) * RECLAIM_GAIN_RATIO;
    if ((mi->field.active_anon + mi->field.inactive_anon) < nr_to_reclaim || nr_to_reclaim <= 0) {
        return;
    }

    pthread_mutex_lock(&extend_reclaim_mutex);
    extend_reclaim_event[EXTEND_RECLAIM_EVENT_CHECK].reclaim_type = EXTEND_RECLAIM_TYPE_ANON;
    extend_reclaim_event[EXTEND_RECLAIM_EVENT_CHECK].reclaim_pages = nr_to_reclaim;
    extend_reclaim_event[EXTEND_RECLAIM_EVENT_CHECK].reclaim_request = true;
    pthread_mutex_unlock(&extend_reclaim_mutex);

    if (extend_reclaim_fd >= 0) {
        ALOGI("%s: trigger reclaim action! nr_to_reclaim = %d", __func__, nr_to_reclaim);
        pthread_cond_signal(&extend_reclaim_cond);
    }

}
#endif

bool LmkdImpl::ext_command_handler(int cmd, LMKD_CTRL_PACKET packet, int nargs) {
    bool ret = true;
    switch(cmd) {
        case LMK_FORCE_CHECK_LIST:
            lmkd_pack_get_force_check_list(packet);
            break;
        case LMK_SET_BLACKLIST:
            lmkd_pack_set_list(packet, SET_BALCK_LIST);
            break;
        case LMK_SET_PROTECT_LIST:
            lmkd_pack_set_list(packet, SET_PROTECT_LIST);
            break;
        case LMK_SET_PERCEPTIBLE_LIST:
            lmkd_pack_set_list(packet, SET_PERCEPTIBLE_LIST);
            break;
        case LMK_SET_WHITELIST:
            lmkd_pack_set_list(packet, SET_WHITE_LIST);
            break;
        case LMK_CAMERA_MODE:
            cmd_camera_mode(packet);
            break;
        case LMK_SET_PSI_LEVEL:
            lmkd_pack_set_psi(packet, nargs);
            break;
#ifdef QCOM_FEATURE_ENABLE
        case LMK_RECLAIM_FOR_CAMERA:
            reclaim_mem_for_camera(packet);
            break;
#endif
#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
        case LMK_SPT_MODE:
            cmd_spt_mode(packet);
            break;
        case LMK_GAME_MODE:
            cmd_game_mode(packet);
            break;
#endif
        default:
            ret = false;
            break;
    }
    return ret;
}

void* LmkdImpl::find_process_from_list(int type, int adj_max, int adj_min) {
    struct proc *maxprocp = NULL;
    int maxsize = 0;
    char buf[LINE_MAX] = {0};

    if (type > CAM_LIST_MAX_COUNT || type < 0) return (void *)maxprocp;
    std::set<std::string> list = camera_list[type];
    if(list.empty()) return (void *)maxprocp;

    for (int i = adj_max; i >= adj_min; i--) {
        struct adjslot_list *head = &procadjslot_list[ADJTOSLOT(i)];
        struct adjslot_list *curr = head->next;
        while (curr != head) {
            int pid = ((struct proc *)curr)->pid;
            long tasksize = proc_get_size(pid);
            if (tasksize <= 0) {
                struct adjslot_list *next = curr->next;
                pid_remove(pid);
                curr = next;
            } else {
                char *tmp_taskname = proc_get_name(pid, buf, sizeof(buf));
                if ((list.count(tmp_taskname) > 0) && (tasksize > maxsize)) {
                    maxsize = tasksize;
                    maxprocp = (struct proc *)curr;
                }
                curr = curr->next;
            }
        }
    }
    if (maxsize > 0) {
        ALOGW("%s: get heaviest pid(%d) in list(type %d)", __func__, maxprocp->pid, type);
    }
    return (void *)maxprocp;
}

void* LmkdImpl::proc_get_heaviest_extend(int oomadj) {
    void *maxprocp = NULL;
    if (!force_check_list) {
        return maxprocp;
    }
    if(is_perceptible_support) {
        int index = SET_BALCK_LIST;
        while(index < CAM_LIST_MAX_COUNT) {
            if(oomadj == perceptible_adj_thres[index]) {
                int adj_max =  (index == SET_BALCK_LIST) ? oomadj - 1 : OOM_SCORE_ADJ_MAX;
                int adj_min = (index == SET_BALCK_LIST) ? 100 : oomadj;
                maxprocp = find_process_from_list(index, adj_max, adj_min);
            }
            index++;
            if(maxprocp != NULL) return maxprocp;
        }
    } else {
        if(oomadj == perceptible_adj_thres[0]) {
            maxprocp = find_process_from_list(SET_BALCK_LIST, oomadj - 1, 100);
        }
        if(maxprocp != NULL) return maxprocp;

        if(oomadj == perceptible_adj_thres[0]) {
            maxprocp = find_process_from_list(SET_WHITE_LIST, OOM_SCORE_ADJ_MAX, oomadj);
        }
    }
    return (void *)maxprocp;
}

bool LmkdImpl::is_protected_task(int oomadj, char* taskname) {
    // Any task should not be protected if force_check is not enable.
    // Also when oomadj < 800 which means memory is low should not apply protect.
    if (!force_check_list) {
        return false;
    }

    if(is_perceptible_support) {
        int index = SET_PROTECT_LIST;
        while(index < CAM_LIST_MAX_COUNT) {
            if(camera_list[index].count(taskname) > 0 &&
                    oomadj >= perceptible_adj_thres[index]) {
                return true;
            }
            index++;
        }
    } else if(oomadj >= perceptible_adj_thres[0] &&
                camera_list[SET_WHITE_LIST].count(taskname) > 0) {
        return true;
    }
    return false;
}

#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
extern enum zone_watermark get_lowest_watermark(union meminfo *mi __unused,
                                                struct zone_meminfo *zmi);
void LmkdImpl::mi_init(long page_size, union meminfo mi) {
    mi_reclaim_fd = -1;
    spt_mode = false;
    game_mode = false;
    page_k = page_size;
    common_mi = mi;
    if (mi_mms_switch) {
        low_ram_device_config();
    }
}

void LmkdImpl::cmd_spt_mode(LMKD_CTRL_PACKET packet) {
    spt_mode = ntohl(packet[1]) == 1 ? true : false;
}

void LmkdImpl::cmd_game_mode(LMKD_CTRL_PACKET packet) {
    game_mode = ntohl(packet[1]) == 1 ? true : false;
    game_mode_config();
}

int LmkdImpl::connect_mi_reclaim() {
    int fd = socket_local_client("mi_reclaim",
                  ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if(fd < 0) {
        ALOGE("Fail to connect to socket mi mms. return code: %d", fd);
        return -1;
    }
    return fd;
}

void LmkdImpl::trigger_mi_reclaim(int pressure_level, int pressure_policy) {
    if (mi_reclaim_fd < 0) {
        mi_reclaim_fd = connect_mi_reclaim();
        if (mi_reclaim_fd < 0) {
            return;
        }
    }

    char buf[256];
    int ret = snprintf(buf, sizeof(buf), "%d:%d\r\n", pressure_level, pressure_policy);
    if (ret < 0 || ret >= (ssize_t)sizeof(buf)) {
        ALOGE ("trigger mi reclaim Error %d", ret);
        return;
    }

    ssize_t written;
    written = write(mi_reclaim_fd, buf, strlen(buf) + 1);
    if (written < 0) {
        ALOGE ("trigger mi reclaim written:%zu", written);
        close(mi_reclaim_fd);
        mi_reclaim_fd = -1;
        return;
    }
}

// MIUI ADD: START
/*
 * Write a string to a file.
 * Returns false if the file does not exist.
 */
static bool writefilestring(const char *path, const char *s,
                            bool err_if_missing) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    ssize_t len = strlen(s);
    ssize_t ret;

    if (fd < 0) {
        if (err_if_missing) {
            ALOGE("Error opening %s; errno=%d", path, errno);
        }
        return false;
    }

    ret = TEMP_FAILURE_RETRY(write(fd, s, len));
    if (ret < 0) {
        ALOGE("Error writing %s; errno=%d", path, errno);
    } else if (ret < len) {
        ALOGE("Short write on %s; length=%zd", path, ret);
    }

    close(fd);
    return true;
}

static void cam_mem_reclaim(long other_free, long other_file)
{
    char path[LINE_MAX];
    static struct timespec last_mem_reclaim_tm;
    struct timespec curr_tm;

    if (other_free < mem_reclaim_threhold && other_file < mem_reclaim_threhold &&
            mem_reclaim_interval_ms &&
            clock_gettime(CLOCK_MONOTONIC_COARSE, &curr_tm) == 0 &&
            get_time_diff_ms(&last_mem_reclaim_tm, &curr_tm) > static_cast<long>(mem_reclaim_interval_ms)) {
            ALOGI("reclaim mem time interval is %ld", static_cast<long>(mem_reclaim_interval_ms));

            snprintf(path, sizeof(path), "/sys/kernel/cam_reclaim/event");
            if (!writefilestring(path, "1", false)) {
                 ALOGW("Failed to open %s; errno=%d", path, errno);
            } else {
                 last_mem_reclaim_tm = curr_tm;
            }
    }
}

inline static bool cam_is_perceptible_support()
{
    return (camera_in_foreground && is_perceptible_support);
}

static float cam_get_memory_psiavg10()
{
    const char* psi_mem_path = "/proc/pressure/memory";
    const char* psi_mem_key = "full avg10=";
    const char* psi_info_str = NULL;
    float       psi_value = -1;
    char buf[1024];
    FILE* fp = fopen(psi_mem_path,"r");
    if (fp == NULL)
    {
        return -1;
    }
    while (fgets(buf, sizeof(buf), fp))
    {
        psi_info_str = strstr(buf, psi_mem_key);
        if (psi_info_str != NULL)
        {
            psi_value = strtof(psi_info_str + strlen(psi_mem_key), NULL);
            break;
        }
    }
    fclose(fp);
    return psi_value;
}

static bool cam_perceptible_policy(struct memory_pressure_policy_pc *pc, int *min_score_adj)
{
    union meminfo mi = pc->mi;
    struct zoneinfo zi = pc->zi;
    long other_free = 0, other_file = 0;
    int minfree = 0;
    bool use_minfree_levels = cam_is_perceptible_support();
    float psi_avg10 = pc->psi_info.mem_stats[PSI_FULL].avg10;

    if (use_minfree_levels) {
        int i;

        other_free = mi.field.nr_free_pages - zi.totalreserve_pages;
        if (mi.field.nr_file_pages > (mi.field.shmem + mi.field.unevictable + mi.field.swap_cached)) {
            other_file = (mi.field.nr_file_pages - mi.field.shmem -
                          mi.field.unevictable - mi.field.swap_cached);
            if (camera_in_foreground == true) {
                other_file += mi.field.kreclaimable;
            }
        } else {
            other_file = 0;
        }

        *min_score_adj = OOM_SCORE_ADJ_MAX + 1;
        for (i = 0; i < lowmem_targets_size; i++) {
            minfree = lowmem_minfree[i];
            if ((other_free + other_file) < minfree) {
                *min_score_adj = lowmem_adj[i];
                break;
            }
        }

        if ( pc->level >= VMPRESS_LEVEL_CRITICAL) {
            for (i = 0; i < (int)ARRAY_SIZE(lowmem_psi_adj) - 1; i++) {
              if( psi_avg10 >= lowmem_psi[i] && lowmem_psi[i] > 0) {
                  *min_score_adj = std::min(lowmem_psi_adj[i], *min_score_adj);
                  break;
              }
            }
        }
    }

    if (*min_score_adj == OOM_SCORE_ADJ_MAX + 1) {
        if (debug_process_killing) {
            ALOGI("Ignore %s memory pressure event "
                    "(free memory=%ldkB, cache=%ldkB, limit=%ldkB)",
                    ilevel_name[pc->level], other_free * page_k, other_file * page_k,
                    (long)lowmem_minfree[lowmem_targets_size - 1] * page_k);
        }
    } else {
        ALOGI("cache(%ldkB) and "
                "free(%" PRId64 "kB)-reserved(%" PRId64 "kB) below min(%ldkB) for oom_adj %d,(%d:%.2f)",
                other_file * page_k, mi.field.nr_free_pages * page_k,
                zi.totalreserve_pages * page_k,
                minfree * page_k, *min_score_adj, pc->level, psi_avg10);
        return true;
    }
    return false;
}

bool LmkdImpl::mi_special_case(enum vmpressure_level level, int *min_score_adj,
                                   union meminfo mi, struct zoneinfo zi) {
    long other_free = 0, other_file = 0;
    int minfree = 0;
    bool use_minfree_levels = false;

    use_minfree_levels = (camera_in_foreground || spt_mode);

    if (use_minfree_levels) {
        int i;

        other_free = mi.field.nr_free_pages - zi.totalreserve_pages;
        if (mi.field.nr_file_pages > (mi.field.shmem + mi.field.unevictable + mi.field.swap_cached)) {
            other_file = (mi.field.nr_file_pages - mi.field.shmem -
                          mi.field.unevictable - mi.field.swap_cached);
	    if (camera_in_foreground == true) {
		other_file += mi.field.kreclaimable;
	    }
        } else {
            other_file = 0;
        }

        if (!double_watermark_enable || spt_mode) {
            *min_score_adj = OOM_SCORE_ADJ_MAX + 1;
            for (i = 0; i < lowmem_targets_size; i++) {
                minfree = lowmem_minfree[i];
                if (other_free < minfree && other_file < minfree) {
                    *min_score_adj = lowmem_adj[i];
                    break;
                }
            }
        } else {
            //cache && free watermark
            *min_score_adj = OOM_SCORE_ADJ_MAX + 1;
            for (i = 0; i < lowmem_targets_size; i++) {
                minfree = lowmem_minfree[i];
                if (other_free < minfree && other_file < minfree) {
                    *min_score_adj = lowmem_adj[i];
                   // Adaptive LMK
                    if (camera_adaptive_lmk_enable) {
                        if(level >= VMPRESS_LEVEL_CRITICAL && i >= CAMERA_ADAPTIVE_LMK_ADJ_LIMIT
                                && other_free < MEMORY_50M_IN_PAGES) {
                            *min_score_adj = lowmem_adj[i-1] + 1;
                            ALOGI("Adaptive lmk level:%d,min_score_adj:%d ", level, *min_score_adj);
                        }
                    }
                    break;
                }
            }
            // free watermark
            int lowmem_targets_size_free = sizeof(lowmem_minfree_free) / sizeof(int);
            int min_score_adj_free = OOM_SCORE_ADJ_MAX + 1;
            int minfree_free = 0;
            for (i = 0; i < lowmem_targets_size_free; i++) {
                minfree_free = lowmem_minfree_free[i];
                if (other_free < minfree_free) {
                    min_score_adj_free = lowmem_adj_free[i];
                    break;
                }
            }
            // min
            minfree = *min_score_adj < min_score_adj_free ? minfree : minfree_free;
            *min_score_adj = *min_score_adj < min_score_adj_free ? *min_score_adj : min_score_adj_free;
        }

        if (cam_mem_reclaim_switch == true && camera_in_foreground == true)
            cam_mem_reclaim(other_free, other_file);

        if (*min_score_adj == OOM_SCORE_ADJ_MAX + 1) {
            if (debug_process_killing) {
                ALOGI("Ignore %s memory pressure event "
                      "(free memory=%ldkB, cache=%ldkB, limit=%ldkB)",
                      ilevel_name[level], other_free * page_k, other_file * page_k,
                      (long)lowmem_minfree[lowmem_targets_size - 1] * page_k);
            }
        } else {
            ALOGI("cache(%ldkB) and "
                  "free(%" PRId64 "kB)-reserved(%" PRId64 "kB) below min(%ldkB) for oom_adj %d",
                  other_file * page_k, mi.field.nr_free_pages * page_k,
                  zi.totalreserve_pages * page_k,
                  minfree * page_k, *min_score_adj);
        }
        return true;
    }
    return false;
}

void LmkdImpl::low_ram_device_config() {
    if (page_k != 0) {
        if ((common_mi.field.nr_total_pages * page_k) < mi_mms_lowermem_thread) {
            mi_mms_system_pool_thread = mi_mms_lowmem_system_pool_thread;
            mi_mms_system_pool_limit_thread = mi_mms_lowmem_system_pool_limit_thread;
            mi_mms_cont_wmark_low_thread = mi_mms_lowmem_cont_wmark_low_thread;
            mi_mms_pre_previous_anonpages_thread = mi_mms_lowmem_pre_previous_anonpages_thread;
            mi_mms_previous_anonpages_thread = mi_mms_lowermem_previous_anonpages_thread;
            mi_mms_perceptible_anonpages_thread = mi_mms_lowermem_perceptible_anonpages_thread;
            mi_mms_visible_anonpages_thread = mi_mms_lowermem_visible_anonpages_thread;
            mi_mms_filepage_thread = mi_mms_lowmem_filepage_thread;
            swap_free_low_percentage = mi_mms_lowmem_swap_free_low_percentage;
            mi_mms_cached_memavailable_thread = mi_mms_lowmem_cached_memavailable_thread;
            mi_mms_pre_previous_memavailable_thread = mi_mms_lower_pre_previous_memavailable_thread;
            mi_mms_previous_memavailable_thread = mi_mms_lower_previous_memavailable_thread;
            mi_mms_perceptible_memavailable_thread = mi_mms_lower_perceptible_memavailable_thread;
            mi_mms_visible_memavailable_thread = mi_mms_visible_lowermem_memavailable_thread;
            mi_mms_critical_memavailable_thread = mi_mms_critical_lowmem_memavailable_thread;
            wmark_boost_factor = mi_mms_lowmem_wmark_boost_factor;
	} else if ((common_mi.field.nr_total_pages * page_k) < mi_mms_lowmem_thread) {
            mi_mms_system_pool_thread = mi_mms_lowmem_system_pool_thread;
            mi_mms_system_pool_limit_thread = mi_mms_lowmem_system_pool_limit_thread;
            mi_mms_cont_wmark_low_thread = mi_mms_lowmem_cont_wmark_low_thread;
            mi_mms_pre_previous_anonpages_thread = mi_mms_lowmem_pre_previous_anonpages_thread;
            mi_mms_previous_anonpages_thread = mi_mms_lowmem_previous_anonpages_thread;
            mi_mms_perceptible_anonpages_thread = mi_mms_lowmem_perceptible_anonpages_thread;
            mi_mms_visible_anonpages_thread = mi_mms_lowmem_visible_anonpages_thread;
            mi_mms_filepage_thread = mi_mms_lowmem_filepage_thread;
            swap_free_low_percentage = mi_mms_lowmem_swap_free_low_percentage;
            mi_mms_cached_memavailable_thread = mi_mms_lowmem_cached_memavailable_thread;
            mi_mms_pre_previous_memavailable_thread = mi_mms_lower_pre_previous_memavailable_thread;
            mi_mms_visible_memavailable_thread = mi_mms_visible_lowmem_memavailable_thread;
            mi_mms_critical_memavailable_thread = mi_mms_critical_lowmem_memavailable_thread;
            wmark_boost_factor = mi_mms_lowmem_wmark_boost_factor;
        } else if ((common_mi.field.nr_total_pages * page_k) < mi_mms_mediummem_thread) {
            mi_mms_system_pool_thread = mi_mms_lowmem_system_pool_thread;
            mi_mms_system_pool_limit_thread = mi_mms_lowmem_system_pool_limit_thread;
            mi_mms_cont_wmark_low_thread = mi_mms_lowmem_cont_wmark_low_thread;
            mi_mms_pre_previous_anonpages_thread = mi_mms_mediummem_pre_previous_anonpages_thread;
            mi_mms_previous_anonpages_thread = mi_mms_mediummem_previous_anonpages_thread;
            mi_mms_perceptible_anonpages_thread = mi_mms_mediummem_perceptible_anonpages_thread;
            mi_mms_visible_anonpages_thread = mi_mms_mediummem_visible_anonpages_thread;
            mi_mms_filepage_thread = mi_mms_lowmem_filepage_thread;
            swap_free_low_percentage = mi_mms_lowmem_swap_free_low_percentage;
            mi_mms_cached_memavailable_thread = mi_mms_mediummem_cached_memavailable_thread;
            mi_mms_visible_memavailable_thread = mi_mms_visible_mediummem_memavailable_thread;
            mi_mms_critical_memavailable_thread = mi_mms_critical_mediummem_memavailable_thread;
            wmark_boost_factor = mi_mms_lowmem_wmark_boost_factor;
        }
    }

    if (mi_mms_log_switch) {
        ALOGI("filepage_thread %" PRId32 " system_pool_thread %" PRId32
              " lowmem_thread %" PRId32 "kB medium_mem_thread %" PRId32 "kB"
              " wmark_boost_factor %d"
              " pre_previous_anonpages_thread %" PRId32
              " previous_anonpages_thread %" PRId32
              " perceptible_anonpages_thread %" PRId32
              " visible_anonpages_thread %" PRId32
              " critical_anonpages_thread %" PRId32
              " swap_free_low_percentage %d"
              " cached_memavailable_thread %" PRId32
              " pre_previous_memavailable_thread %" PRId32
              " previous_memavailable_thread %" PRId32
              " perceptible_memavailable_thread %" PRId32
              " visible_memavailable_thread %" PRId32
              " critical_memavailable_thread %" PRId32
              " mi_mms_cont_wmark_low_thread %" PRId32,
              mi_mms_filepage_thread, mi_mms_system_pool_thread,
              mi_mms_lowmem_thread, mi_mms_mediummem_thread,
              wmark_boost_factor,
              mi_mms_pre_previous_anonpages_thread,
              mi_mms_previous_anonpages_thread,
              mi_mms_perceptible_anonpages_thread,
              mi_mms_visible_anonpages_thread,
              mi_mms_critical_anonpages_thread,
              swap_free_low_percentage,
              mi_mms_cached_memavailable_thread,
              mi_mms_pre_previous_memavailable_thread,
              mi_mms_previous_memavailable_thread,
              mi_mms_perceptible_memavailable_thread,
              mi_mms_visible_memavailable_thread,
              mi_mms_critical_memavailable_thread,
              mi_mms_cont_wmark_low_thread);
    }
}

void LmkdImpl::game_mode_config() {
    if (page_k != 0) {
        /* < 8GB */
        if ((common_mi.field.nr_total_pages * page_k) < mi_mms_mediummem_thread) {
            if (game_mode) {
                mi_mms_cached_memavailable_thread = mi_mms_game_reclaim_memavailable_thread;
                wmark_boost_factor = mi_mms_game_wmark_boost_factor;
            } else {
                low_ram_device_config();
	    }
	    ALOGI("game state %d %d %" PRId32,
			    game_mode, wmark_boost_factor, mi_mms_cached_memavailable_thread);
        }
    }

    if (mi_mms_log_switch) {
        ALOGI("filepage_thread %" PRId32 " system_pool_thread %" PRId32
              " lowmem_thread %" PRId32 "kB medium_mem_thread %" PRId32 "kB"
              " wmark_boost_factor %d perceptible_anonpages_thread %" PRId32
              " swap_free_low_percentage %d" " cached_memavailable_thread %" PRId32,
              mi_mms_filepage_thread, mi_mms_system_pool_thread,
              mi_mms_lowmem_thread, mi_mms_mediummem_thread,
              wmark_boost_factor, mi_mms_perceptible_anonpages_thread,
              swap_free_low_percentage, mi_mms_cached_memavailable_thread);
    }
}

#ifdef TEMP_DEBUG
void LmkdImpl::count_vm_events(union vmstat *vs) {
    static union vmstat base_vs;
    int64_t pswpin_deltas, pswpout_deltas;
    int64_t pgpgin_deltas, pgpgout_deltas, pgpgoutclean_deltas;
    int64_t nr_dirtied_deltas, nr_written_deltas;

    if (0 == base_vs.field.pswpin) {
        base_vs = *vs;
        return;
    }

    pswpin_deltas = vs->field.pswpin - base_vs.field.pswpin;
    pswpout_deltas = vs->field.pswpout - base_vs.field.pswpout;

    pgpgin_deltas = vs->field.pgpgin - base_vs.field.pgpgin;
    pgpgout_deltas = vs->field.pgpgout - base_vs.field.pgpgout;
    pgpgoutclean_deltas = vs->field.pgpgoutclean - base_vs.field.pgpgoutclean;

    nr_dirtied_deltas = vs->field.nr_dirtied - base_vs.field.nr_dirtied;
    nr_written_deltas = vs->field.nr_written - base_vs.field.nr_written;

    mvmp.vm_events[PSWAPIN_DELTAS] = pswpin_deltas;
    mvmp.vm_events[PSWAPOUT_DELTAS] = pswpout_deltas;
    mvmp.vm_events[PGPGIN_DELTAS] = pgpgin_deltas;
    mvmp.vm_events[PGPGOUT_DELTAS] = pgpgout_deltas;
    mvmp.vm_events[PGPGOUTCLEAN_DELTAS] = pgpgoutclean_deltas;
    mvmp.vm_events[NR_DIRTIED_DELTAS] = nr_dirtied_deltas;
    mvmp.vm_events[NR_WRITTEN_DELTAS] = nr_written_deltas;
    mvmp.vm_events[NR_DIRTY] = vs->field.nr_dirty;
    mvmp.vm_events[NR_WRITEBACK] = vs->field.nr_writeback;

    if (mi_mms_log_switch) {
        ALOGD("pswpin c:%" PRId64 " b:%" PRId64 " delta %" PRId64 "kB\n"
              "pswpout c:%" PRId64 " b:%" PRId64 " delta %" PRId64 "kB\n"
              "pgpgin c:%" PRId64 " b:%" PRId64 " delta %" PRId64 "kB\n"
              "pgpgout c:%" PRId64 " b:%" PRId64 " delta %" PRId64 "kB\n"
              "pgpgoutclean c:%" PRId64 " b:%" PRId64 " delta %" PRId64 "kB\n"
              "nr_dirtied c:%" PRId64 " b:%" PRId64 " delta %" PRId64 "kB\n"
              "nr_written c:%" PRId64 " b:%" PRId64 " delta %" PRId64 "kB\n"
              "nr_dirty %"  PRId64 "kB nr_writeback %" PRId64 "kB",
              vs->field.pswpin, base_vs.field.pswpin, pswpin_deltas * page_k,
              vs->field.pswpout, base_vs.field.pswpout, pswpout_deltas * page_k,
              vs->field.pgpgin, base_vs.field.pgpgin, pgpgin_deltas * page_k,
              vs->field.pgpgout, base_vs.field.pgpgout, pgpgout_deltas * page_k,
              vs->field.pgpgoutclean, base_vs.field.pgpgoutclean, pgpgoutclean_deltas * page_k,
              vs->field.nr_dirtied, base_vs.field.nr_dirtied, nr_dirtied_deltas * page_k,
              vs->field.nr_written, base_vs.field.nr_written, nr_written_deltas * page_k,
              vs->field.nr_dirty * page_k, vs->field.nr_writeback * page_k);
    }
    base_vs = *vs;
}
#endif

int64_t LmkdImpl::get_total_pgskip_deltas(union vmstat *vs, int64_t *init_pgskip) {
    unsigned int i;
    int64_t total_pgskip = 0;

    for (i = VS_PGSKIP_FIRST_ZONE; i <= VS_PGSKIP_LAST_ZONE; i++) {
        if (vs->arr[i] >= 0) {
            total_pgskip += vs->arr[i] - init_pgskip[PGSKIP_IDX(i)];
        }
    }
    return total_pgskip;
}

void LmkdImpl::count_cont_reclaim_type(int reclaim,
            struct timespec *curr_tm) {
    int index;
    static struct timespec last_tm;

    if (0 == last_tm.tv_sec) {
        last_tm = *curr_tm;
        return;
    }

    if (get_time_diff_ms(&last_tm, curr_tm) > DATA_INVALID_TIME_MS) {
        for (index = NO_RECLAIM; index < RECLAIM_TYPE_COUNT; index++) {
            mvmp.cont_reclaim_type_count_between_window[index] = 0;
        }
    }

    for (index = NO_RECLAIM; index < RECLAIM_TYPE_COUNT; index++) {
        if (index == reclaim) {
            mvmp.cont_reclaim_type_count_between_window[index]++;
        } else {
            mvmp.cont_reclaim_type_count_between_window[index] = 0;
        }
    }

    if (mi_mms_log_switch) {
        ALOGD("reclaim count: no_reclaim=%d kswapd_reclaim=%d direct_reclaim=%d "
              "direct_reclaim_throttle=%d",
              mvmp.cont_reclaim_type_count_between_window[NO_RECLAIM],
              mvmp.cont_reclaim_type_count_between_window[KSWAPD_RECLAIM],
              mvmp.cont_reclaim_type_count_between_window[DIRECT_RECLAIM],
              mvmp.cont_reclaim_type_count_between_window[DIRECT_RECLAIM_THROTTLE]);
    }
    last_tm = *curr_tm;
}

void LmkdImpl::count_cont_wmark(uint32_t events, union meminfo *mi,
            struct zone_meminfo *zmi, struct timespec *curr_tm) {
    int index;
    char buf[BUF_MAX] = {'\0'};
    static struct timespec last_tm;
    enum zone_watermark current_lowest_watermark;

    if (0 == last_tm.tv_sec) {
        last_tm = *curr_tm;
        return;
    }

    if (get_time_diff_ms(&last_tm, curr_tm) > DATA_INVALID_TIME_MS) {
        for (index = WMARK_MIN; index <= WMARK_NONE; index++) {
            mvmp.cont_wmark_count_between_window[index] = 0;
            mvmp.cont_wmark_count_in_window[index] = 0;
        }
    }

    current_lowest_watermark = get_lowest_watermark(mi, zmi);
    if (events > 0) {
        for (index = WMARK_MIN; index <= WMARK_NONE; index++) {
            if (index == current_lowest_watermark) {
                mvmp.cont_wmark_count_between_window[index]++;
                mvmp.cont_wmark_count_in_window[index] = 1;
            } else {
                mvmp.cont_wmark_count_between_window[index] = 0;
                mvmp.cont_wmark_count_in_window[index] = 0;
            }
        }
    } else {
        if (mvmp.cont_wmark_count_between_window[current_lowest_watermark] == 0) {
            for (index = WMARK_MIN; index <= WMARK_NONE; index++) {
                mvmp.cont_wmark_count_between_window[index] = 0;
                mvmp.cont_wmark_count_in_window[index] = 0;
            }
            mvmp.cont_wmark_count_between_window[current_lowest_watermark] = 1;
	}
        mvmp.cont_wmark_count_in_window[current_lowest_watermark]++;
    }

    if (mi_mms_log_switch) {
        snprintf(buf, BUF_MAX - 1, "events %d watermark %d ", events, current_lowest_watermark);
        for (index = WMARK_MIN; index <= WMARK_NONE; index++) {
            if ((int)(BUF_MAX - strlen(buf) - 1) > 0) {
                snprintf(buf + strlen(buf), BUF_MAX - strlen(buf) - 1, "%d:%d ",
                mvmp.cont_wmark_count_in_window[index],
                mvmp.cont_wmark_count_between_window[index]);
            } else {
                break;
            }
        }
        ALOGD("%s", buf);
    }

    last_tm = *curr_tm;
}

void LmkdImpl::mi_count_cont_psi_event(uint32_t events, enum vmpressure_level lvl,
                                            struct timespec *curr_tm) {
    int lvl_index;
    char buf[BUF_MAX] = {'\0'};
    static struct timespec last_tm;

    if (mi_mms_switch) {
        if (0 == last_tm.tv_sec) {
            last_tm = *curr_tm;
            return;
        }

        if (get_time_diff_ms(&last_tm, curr_tm) > DATA_INVALID_TIME_MS) {
            for (lvl_index = VMPRESS_LEVEL_LOW; lvl_index < VMPRESS_LEVEL_COUNT; lvl_index++) {
                mvmp.cont_triggered_count_between_window[lvl_index] = 0;
                mvmp.cont_polling_count_in_window[lvl_index] = 0;
            }
        }

        if (events > 0) {
            for (lvl_index = VMPRESS_LEVEL_LOW; lvl_index < VMPRESS_LEVEL_COUNT; lvl_index++) {
                if (lvl_index == lvl) {
                    mvmp.cont_triggered_count_between_window[lvl_index]++;
                } else {
                    mvmp.cont_triggered_count_between_window[lvl_index] = 0;
                }
                mvmp.cont_polling_count_in_window[lvl_index] = 0;
            }
        } else {
            if (mvmp.cont_triggered_count_between_window[lvl] == 0) {
                for (lvl_index = VMPRESS_LEVEL_LOW; lvl_index < VMPRESS_LEVEL_COUNT; lvl_index++) {
                    if (0 != mvmp.cont_triggered_count_between_window[lvl_index]) {
                        mvmp.cont_triggered_count_between_window[lvl_index] = 0;
                        mvmp.cont_polling_count_in_window[lvl_index] = 0;
                    }
                }
                mvmp.cont_triggered_count_between_window[lvl] = 1;
            }
            mvmp.cont_polling_count_in_window[lvl]++;
        }

        if (mi_mms_log_switch) {
            snprintf(buf, BUF_MAX - 1, "events %d lvl %d ", events, lvl);
            for (lvl_index = VMPRESS_LEVEL_LOW; lvl_index < VMPRESS_LEVEL_COUNT; lvl_index++) {
                if ((int)(BUF_MAX - strlen(buf) - 1) > 0) {
                    snprintf(buf + strlen(buf), BUF_MAX - strlen(buf) - 1, "%d:%d ",
                             mvmp.cont_polling_count_in_window[lvl_index],
                             mvmp.cont_triggered_count_between_window[lvl_index]);
                } else {
                    break;
                }
            }
            ALOGD("%s", buf);
        }

        last_tm = *curr_tm;
    }
}

void LmkdImpl::count_reclaim_rate(union vmstat *window_current, uint32_t events,
                                     int64_t *init_pgskip) {
    static union vmstat window_base[WINDOW_COUNT];
    int polling_window;
    int64_t pgscan_kswapd_deltas, pgscan_direct_deltas;
    int64_t pgsteal_kswapd_deltas, pgsteal_direct_deltas;
    int64_t real_total_pgscan_deltas, total_pgskip_deltas;

    /* events > 0: triggered
       events = 0: polling check
    */
    polling_window = events == 0 ? 0 : 1;

    if (window_base[polling_window].field.pgscan_kswapd == 0) {
        window_base[polling_window] = *window_current;
        return;
    }

    total_pgskip_deltas = get_total_pgskip_deltas(window_current, init_pgskip);
    pgscan_kswapd_deltas = window_current->field.pgscan_kswapd -
            window_base[polling_window].field.pgscan_kswapd;
    pgscan_direct_deltas = window_current->field.pgscan_direct -
            window_base[polling_window].field.pgscan_direct;

    pgsteal_kswapd_deltas = window_current->field.pgsteal_kswapd -
            window_base[polling_window].field.pgsteal_kswapd;
    pgsteal_direct_deltas = window_current->field.pgsteal_direct -
            window_base[polling_window].field.pgsteal_direct;

    mvmp.kswapd_reclaim_deltas[polling_window] = pgsteal_kswapd_deltas;
    mvmp.direct_reclaim_deltas[polling_window] = pgsteal_direct_deltas;
    real_total_pgscan_deltas = pgscan_kswapd_deltas + pgscan_direct_deltas - total_pgskip_deltas;

    if (pgscan_kswapd_deltas > 0) {
        mvmp.kswapd_reclaim_ratio[polling_window] = (int)((pgsteal_kswapd_deltas * 100) / pgscan_kswapd_deltas);
    } else {
        mvmp.kswapd_reclaim_ratio[polling_window] = 0;
    }

    if (pgscan_direct_deltas > 0) {
        mvmp.direct_reclaim_ratio[polling_window] = (int)((pgsteal_direct_deltas * 100) / pgscan_direct_deltas);
    } else {
        mvmp.direct_reclaim_ratio[polling_window] = 0;
    }

    if (real_total_pgscan_deltas > 0) {
        mvmp.total_reclaim_ratio = (int)(((pgsteal_kswapd_deltas + pgsteal_direct_deltas) * 100) / real_total_pgscan_deltas);
    } else {
        mvmp.total_reclaim_ratio = 0;
    }

    if (mi_mms_log_switch) {
        ALOGD("%s"
            " kswapd pgscan: current %" PRId64
            " base %" PRId64 " deltas %" PRId64 "(%" PRId64 "kB)"
            " ;direct pgscan: current %" PRId64
            " base %" PRId64 " deltas %" PRId64 "(%" PRId64 "kB)"
            " ;kswapd pgsteal: current %" PRId64
            " base %" PRId64 " deltas %" PRId64 "(%" PRId64 "kB)"
            " ;direct pgsteal: current %" PRId64
            " base %" PRId64 " deltas %" PRId64 "(%" PRId64 "kB)"
            " ;total_pgskip_deltas %" PRId64 "(%" PRId64 "kB)"
            " kswapd_reclaim_ratio %d direct_reclaim_ratio %d"
            " total_reclaim_ratio %d",
            polling_window == IN_WINDOW ? "[polling check]" : "[triggered]",
            window_current->field.pgscan_kswapd,
            window_base[polling_window].field.pgscan_kswapd,
            pgscan_kswapd_deltas, pgscan_kswapd_deltas * page_k,
            window_current->field.pgscan_direct,
            window_base[polling_window].field.pgscan_direct,
            pgscan_direct_deltas, pgscan_direct_deltas * page_k,
            window_current->field.pgsteal_kswapd,
            window_base[polling_window].field.pgsteal_kswapd,
            pgsteal_kswapd_deltas, pgsteal_kswapd_deltas * page_k,
            window_current->field.pgsteal_direct,
            window_base[polling_window].field.pgsteal_direct,
            pgsteal_direct_deltas, pgsteal_direct_deltas * page_k,
            total_pgskip_deltas, total_pgskip_deltas *page_k,
            mvmp.kswapd_reclaim_ratio[polling_window],
            mvmp.direct_reclaim_ratio[polling_window],
            mvmp.total_reclaim_ratio);
    }

    window_base[polling_window] = *window_current;
}

bool LmkdImpl::reclaim_rate_ok(int polling_window) {
    if ((mvmp.kswapd_reclaim_ratio[polling_window] != 0 &&
            mvmp.kswapd_reclaim_ratio[polling_window] >= mi_mms_reclaim_ratio) ||
            (mvmp.direct_reclaim_ratio[polling_window] != 0 &&
            mvmp.direct_reclaim_ratio[polling_window] >= mi_mms_reclaim_ratio)) {
        return true;
    } else {
        return false;
    }
}

int64_t LmkdImpl::get_memavailable(union meminfo *mi, struct zoneinfo *zi) {
    int64_t other_free = 0, other_file = 0;
    int64_t memavailable;

    other_free = mi->field.nr_free_pages - zi->totalreserve_pages;
    other_file = mi->field.active_file + mi->field.inactive_file +  mi->field.swap_cached;

    memavailable = other_free + other_file;
    if (mi_mms_log_switch) {
        ALOGD("MI_LOG_MEMAVA| other_free %" PRId64 "kB other_file %" PRId64
              "kB memavailable %" PRId64 "kB nr_file_pages %" PRId64
              "kB nr_free_pages %" PRId64 "kB totalreserve_pages %" PRId64
              "kB cached_memavailable_thread %" PRId32
              "kB previous_memavailable_thread %" PRId32
              "kB perceptible_memavailable_thread %" PRId32 "kB"
              "kB visible_memavailable_thread %" PRId32 "kB"
              "kB critical_memavailable_thread %" PRId32 "kB",
             other_free * page_k, other_file * page_k,
             memavailable * page_k, mi->field.nr_file_pages * page_k,
             mi->field.nr_free_pages * page_k, zi->totalreserve_pages * page_k,
             mi_mms_cached_memavailable_thread * (int32_t)page_k,
             mi_mms_previous_memavailable_thread * (int32_t)page_k,
             mi_mms_perceptible_memavailable_thread * (int32_t)page_k,
             mi_mms_visible_memavailable_thread * (int32_t)page_k,
             mi_mms_critical_memavailable_thread * (int32_t)page_k);
    }
    return memavailable;
}

int64_t LmkdImpl::get_anon_pages(union meminfo *mi) {
    int64_t anon_pages;

    anon_pages = mi->field.active_anon + mi->field.inactive_anon;
    return anon_pages;
}

bool LmkdImpl::memavailable_is_low(union meminfo *mi, struct zoneinfo *zi,
                                       int64_t *memavailable) {
    int64_t in_memavailable;

    in_memavailable = get_memavailable(mi, zi);
    if (memavailable != NULL) {
        *memavailable = in_memavailable;
    }

    if (in_memavailable <= (int64_t)mi_mms_cached_memavailable_thread) {
        return true;
    } else {
        return false;
    }
}

bool LmkdImpl::file_pages_is_low(union meminfo *mi) {
    int64_t file_pages;

    file_pages = mi->field.active_file + mi->field.inactive_file;
    if (file_pages <= (int64_t)mi_mms_file_thrashing_page_thread) {
        return true;
    } else {
        return false;
    }
}

bool LmkdImpl::should_kill_subprocess(union meminfo *mi, struct zoneinfo *zi,
                                       int64_t *memavailable) {
    int64_t in_memavailable;

    in_memavailable = get_memavailable(mi, zi);
    if (memavailable != NULL) {
        *memavailable = in_memavailable;
    }

    if (in_memavailable <= (int64_t)mi_mms_kill_subprocess_memavailable_thread) {
        return true;
    } else {
        return false;
    }
}

bool LmkdImpl::cont_event_memavailable_touch(struct memory_pressure_policy_pc *pc) {
    int64_t memavailable;
    int64_t anon_pages;

    memavailable = get_memavailable(&pc->mi, &pc->zi);
    if (memavailable < (int64_t)mi_mms_cached_memavailable_thread) {
        return true;
    } else {
        return false;
    }
}

bool LmkdImpl::pre_previous_memavailable_touch(struct memory_pressure_policy_pc *pc) {
    int64_t memavailable;

    memavailable = get_memavailable(&pc->mi, &pc->zi);
    if (memavailable < (int64_t)mi_mms_pre_previous_memavailable_thread) {
        return true;
    } else {
        return false;
    }
}

bool LmkdImpl::previous_memavailable_touch(struct memory_pressure_policy_pc *pc) {
    int64_t memavailable;

    memavailable = get_memavailable(&pc->mi, &pc->zi);
    if (memavailable < (int64_t)mi_mms_previous_memavailable_thread) {
        return true;
    } else {
        return false;
    }
}

bool LmkdImpl::perceptible_memavailable_touch(struct memory_pressure_policy_pc *pc) {
    int64_t memavailable;
    int64_t anon_pages;

    anon_pages = get_anon_pages(&pc->mi);
    memavailable = get_memavailable(&pc->mi, &pc->zi);
    if (pc->swap_is_low || pc->file_thrashing || game_mode) {
        if (memavailable < (int64_t)mi_mms_perceptible_memavailable_thread) {
            return true;
	} else {
            return false;
        }
    } else {
        if (memavailable < (int64_t)mi_mms_perceptible_memavailable_thread &&
                anon_pages < mi_mms_perceptible_anonpages_thread) {
            return true;
	} else {
            return false;
	}
    }
}

bool LmkdImpl::visible_memavailable_touch(struct memory_pressure_policy_pc *pc) {
    int64_t memavailable;
    int64_t anon_pages;

    anon_pages = get_anon_pages(&pc->mi);
    memavailable = get_memavailable(&pc->mi, &pc->zi);
    if (pc->swap_is_low || pc->file_thrashing || game_mode) {
        if (memavailable < (int64_t)mi_mms_visible_memavailable_thread) {
            return true;
	} else {
            return false;
        }
    } else {
        if (memavailable < (int64_t)mi_mms_visible_memavailable_thread &&
                anon_pages < mi_mms_visible_anonpages_thread) {
            return true;
	} else {
            return false;
	}
    }
}

bool LmkdImpl::critical_memavailable_touch(struct memory_pressure_policy_pc *pc) {
    int64_t memavailable;
    int64_t anon_pages;

    anon_pages = get_anon_pages(&pc->mi);
    memavailable = get_memavailable(&pc->mi, &pc->zi);
    if (pc->swap_is_low || pc->file_thrashing || game_mode) {
        if (memavailable < (int64_t)mi_mms_critical_memavailable_thread) {
            return true;
        } else {
            return false;
        }
    } else {
        if (memavailable < (int64_t)mi_mms_critical_memavailable_thread &&
                anon_pages < mi_mms_critical_anonpages_thread) {
            return true;
        } else {
            return false;
        }
    }
}

int LmkdImpl::get_protected_adj(struct memory_pressure_policy_pc *pc) {
    int64_t memavailable;
    int64_t anon_pages;
    int min_adj = 0;

    anon_pages = get_anon_pages(&pc->mi);
    if (pc->events == 0) {
        if (anon_pages > (int64_t)mi_mms_pre_previous_anonpages_thread) {
            pc->kill_reason = NONE;
            return min_adj;
        }
    }

    memavailable = get_memavailable(&pc->mi, &pc->zi);
    if (memavailable < mi_mms_critical_memavailable_thread) {
        min_adj = 0;
    } else if (memavailable < mi_mms_visible_memavailable_thread) {
        min_adj = VISIBLE_APP_ADJ;
    } else if (memavailable < (int64_t)mi_mms_perceptible_memavailable_thread) {
        min_adj = STUB_PERCEPTIBLE_APP_ADJ + 1;
    } else if (memavailable < (int64_t)mi_mms_previous_memavailable_thread) {
        min_adj = PREVIOUS_APP_ADJ;
    } else if (memavailable < (int64_t)mi_mms_pre_previous_memavailable_thread) {
        min_adj = PRE_PREVIOUS_APP_ADJ;
    } else if (memavailable < (int64_t)mi_mms_cached_memavailable_thread) {
        min_adj = SERVICE_B_ADJ;
    } else {
        min_adj = CACHED_APP_LMK_FIRST_ADJ;
    }

    return min_adj;
}

bool LmkdImpl::mem_meet_reclaim(union meminfo *mi) {
    bool swap_is_low;
    bool anon_is_low;
    bool low_mem;

    low_mem = ((mi->field.nr_total_pages * page_k) < mi_mms_trigger_reclaim_mem_thread);
    swap_is_low = mi->field.free_swap < (mi->field.total_swap * swap_free_low_percentage / 100);
    anon_is_low =  (mi->field.active_anon + mi->field.inactive_anon) < mi_mms_perceptible_anonpages_thread;
    if (!swap_is_low && !anon_is_low && low_mem) {
        return true;
    } else {
        return false;
    }
}

bool LmkdImpl::should_trigger_preclaim(uint32_t events, enum vmpressure_level level,
                                          union meminfo *mi, bool is_thrashing) {
    if (reclaim_switch & PRECLAIM && events > 0 && mem_meet_reclaim(mi) &&
            (level >= VMPRESS_LEVEL_CRITICAL || is_thrashing) && !camera_in_foreground) {
        return true;
    } else {
        return false;
    }
}

bool LmkdImpl::should_trigger_greclaim(uint32_t events, enum vmpressure_level level,
                                          union meminfo *mi) {
    if (reclaim_switch & GRECLAIM && events > 0 && mem_meet_reclaim(mi) &&
            level >= VMPRESS_LEVEL_MEDIUM) {
        return true;
    } else {
        return false;
    }
}

void LmkdImpl::out_log_when_psi(uint32_t events, int lvl, int reclaim_type,
		union meminfo *mi, bool swap_is_low, int64_t thrashing) {
    int polling_window;
    int lvl_index;
    char buf[BUF_MAX] = {'\0'};

    polling_window = events == 0 ? 0 : 1;

    for (lvl_index = VMPRESS_LEVEL_LOW; lvl_index < VMPRESS_LEVEL_COUNT; lvl_index++) {
        if ((int)(BUF_MAX - strlen(buf) - 1) > 0) {
            snprintf(buf + strlen(buf), BUF_MAX - strlen(buf) - 1, "%d:%d,",
                    mvmp.cont_polling_count_in_window[lvl_index],
                    mvmp.cont_triggered_count_between_window[lvl_index]);
        } else {
            break;
        }
    }
    buf[strlen(buf) - 1] = '\0';

    ALOGD("MI_LOG_WHEN_PSI|"
             " window=%s level=%d level_count=%s count_upgraded_event=%d|"
             " total_mem=%" PRId64 "kB free_mem=%" PRId64 "kB"
             " active_file=%" PRId64 "kB inactive_file=%" PRId64 "kB"
             " active_anon=%" PRId64 "kB inactive_anon=%" PRId64 "kB"
             " kreclaimable=%" PRId64 "kB"
             " total_swap=%" PRId64 "kB free_swap=%" PRId64 "kB"
             " swap_free_percentage=%" PRId64 "%%|"
             " kswapd_pgsteal_deltas=%" PRId64 "kB kswapd_reclaim_ratio=%d%%"
             " direct_pgsteal_deltas=%" PRId64 "kB direct_reclaim_ratio=%d%%|"
             " reclaim_type=%d"
             " reclaim_count=no:%d,kswapd:%d,direct:%d,direct_throttle:%d|"
#ifdef TEMP_DEBUG
             " vm_event=pswpin_deltas:%" PRId64 "kB,pswpout_deltas:%" PRId64 "kB"
             ",pgpgin_deltas:%" PRId64 "kB,pgpgout_deltas:%" PRId64 "kB"
             ",pgpgoutclean_deltas:%" PRId64 "kB,nr_dirtied_delta:%" PRId64 "kB"
             ",nr_written_deltas:%" PRId64 "kB,nr_dirty:%" PRId64 "kB"
             ",nr_writeback:%" PRId64 "kB|"
#endif
             " wmark=min:%d low:%d high:%d"
             " none:%d lower_than_high:%d |"
             " other=swap_is_low:%d,thrashing:%" PRId64 "%%",
             polling_window == IN_WINDOW ? "polling_check" : "triggered",
             lvl, buf, count_upgraded_event,
             mi->field.nr_total_pages * page_k, mi->field.nr_free_pages * page_k,
             mi->field.active_file * page_k, mi->field.inactive_file * page_k,
             mi->field.active_anon * page_k, mi->field.inactive_anon * page_k,
             mi->field.kreclaimable * page_k,
             mi->field.total_swap * page_k, mi->field.free_swap * page_k,
             (mi->field.free_swap * 100) / (mi->field.total_swap + 1),
             mvmp.kswapd_reclaim_deltas[polling_window] * page_k,
             mvmp.kswapd_reclaim_ratio[polling_window],
             mvmp.direct_reclaim_deltas[polling_window] * page_k,
             mvmp.direct_reclaim_ratio[polling_window],
             reclaim_type,
             mvmp.cont_reclaim_type_count_between_window[NO_RECLAIM],
             mvmp.cont_reclaim_type_count_between_window[KSWAPD_RECLAIM],
             mvmp.cont_reclaim_type_count_between_window[DIRECT_RECLAIM],
             mvmp.cont_reclaim_type_count_between_window[DIRECT_RECLAIM_THROTTLE],
#ifdef TEMP_DEBUG
             mvmp.vm_events[PSWAPIN_DELTAS] * page_k,
             mvmp.vm_events[PSWAPOUT_DELTAS] * page_k,
             mvmp.vm_events[PGPGIN_DELTAS] * page_k,
             mvmp.vm_events[PGPGOUT_DELTAS] * page_k,
             mvmp.vm_events[PGPGOUTCLEAN_DELTAS] * page_k,
             mvmp.vm_events[NR_DIRTIED_DELTAS] * page_k,
             mvmp.vm_events[NR_WRITTEN_DELTAS] * page_k,
             mvmp.vm_events[NR_DIRTY] * page_k,
             mvmp.vm_events[NR_WRITEBACK] * page_k,
#endif
             mvmp.cont_wmark_count_between_window[WMARK_MIN],
             mvmp.cont_wmark_count_between_window[WMARK_LOW],
             mvmp.cont_wmark_count_between_window[WMARK_HIGH],
             mvmp.cont_wmark_count_between_window[WMARK_NONE],
             mvmp.cont_wmark_count_between_window[WMARK_NUMS - 1],
             swap_is_low, thrashing);
}

bool LmkdImpl::cont_event_handle(uint32_t events, enum vmpressure_level level,
                                       enum mi_to_mms_event_type *event_to_mms) {
    int cont_medium_pressure_count = 0;
    int cont_low_wmark_count = 0;
    bool trigger_pm_reclaim = false;

    cont_medium_pressure_count = mvmp.cont_triggered_count_between_window[VMPRESS_LEVEL_MEDIUM];
    cont_low_wmark_count = mvmp.cont_wmark_count_in_window[WMARK_LOW];
    if (events > 0 && 0 != cont_medium_pressure_count &&
              (0 == cont_medium_pressure_count % mi_mms_cont_medium_event_thread)) {
        trigger_pm_reclaim = true;
    }

    /*if (0 != cont_low_wmark_count &&
              (0 == cont_low_wmark_count % mi_mms_cont_wmark_low_thread)) {
        *event_to_mms = CONT_LOW_WMARK;
    }*/

    if (0 != cont_low_wmark_count &&
              (0 == cont_low_wmark_count % mi_mms_cont_wmark_low_trigger_pm_thread)) {
        trigger_pm_reclaim = true;
    }

    if (trigger_pm_reclaim)
        trigger_mi_reclaim(level, VMPRESS_POLICY_TO_MI_PM);

    return(*event_to_mms == NO_EVENT_TO_PM ? false : true);
}

void LmkdImpl::mi_do_early(union vmstat vs, uint32_t events, int64_t *pgskip_deltas, union meminfo mi,
                           enum vmpressure_level level) {
    if (mi_mms_switch) {
        count_reclaim_rate(&vs, events, pgskip_deltas);
#ifdef TEMP_DEBUG
        if (events > 0)
            count_vm_events(&vs);
#endif
        if (should_trigger_greclaim(events, level, &mi)) {
            trigger_mi_reclaim(level, VMPRESS_POLICY_GRECLAIM);
            if (mi_mms_log_switch) {
                ALOGD("MI_LOG_TO_MMS| events=%d level=%d policy=%d", events, level, VMPRESS_POLICY_GRECLAIM);
            }
        }
    }
}

void LmkdImpl::mi_update_props() {
    mi_mms_switch = property_get_bool("persist.sys.mmms.switch", false);
    if (mi_mms_switch) {
        mi_mms_pre_previous_anonpages_thread = property_get_int32("ro.mmms.pre.previous.anonpages.thread", 524288);
        mi_mms_lowmem_pre_previous_anonpages_thread = property_get_int32("persist.sys.mmms.lowmem.pre.previous.anonpages.thread", 288358);
        mi_mms_mediummem_pre_previous_anonpages_thread = property_get_int32("persist.sys.mmms.mediummem.pre.previous.anonpages.thread", 288358);
        mi_mms_previous_anonpages_thread = property_get_int32("ro.mmms.previous.anonpages.thread", 524288);
        mi_mms_lowermem_previous_anonpages_thread = property_get_int32("persist.sys.mmms.loerwmem.previous.anonpages.thread", 128000); /* 500MB */
        mi_mms_lowmem_previous_anonpages_thread = property_get_int32("persist.sys.mmms.lowmem.previous.anonpages.thread", 262144);
        mi_mms_mediummem_previous_anonpages_thread = property_get_int32("persist.sys.mmms.mediummem.previous.anonpages.thread", 262144);
        mi_mms_perceptible_anonpages_thread = property_get_int32("ro.mmms.perceptible.anonpages.thread", 393216);
        mi_mms_lowermem_perceptible_anonpages_thread = property_get_int32("persist.sys.mmms.lowermem.perceptible.anonpages.thread", 128000); /* 500MB*/
        mi_mms_lowmem_perceptible_anonpages_thread = property_get_int32("persist.sys.mmms.lowmem.perceptible.anonpages.thread", 204800);
        mi_mms_mediummem_perceptible_anonpages_thread = property_get_int32("persist.sys.mmms.mediummem.perceptible.anonpages.thread", 204800);
        mi_mms_visible_anonpages_thread = property_get_int32("ro.mmms.visible.anonpages.thread", 314572);
        mi_mms_lowermem_visible_anonpages_thread = property_get_int32("persist.sys.mmms.lowermem.visible.anonpages.thread", 128000); /* 500MB */
        mi_mms_lowmem_visible_anonpages_thread = property_get_int32("persist.sys.mmms.lowmem.visible.anonpages.thread", 153600);
        mi_mms_mediummem_visible_anonpages_thread = property_get_int32("persist.sys.mmms.mediummem.visible.anonpages.thread", 153600);
        mi_mms_critical_anonpages_thread = property_get_int32("persist.sys.mmms.critical.anonpages.thread", 128000); /* 500MB */
        mi_mms_freemem_throttled_thread = property_get_int32("persist.sys.mmms.throttled.thread", 6400);
        mi_mms_cont_low_wmark_reclaim_thread = property_get_int32("persist.sys.mmms.low.wmark.reclaim.thread", 524288);
        mi_mms_cached_memavailable_thread = property_get_int32("ro.mmms.reclaim.memavailable.thread", 524288);
        mi_mms_lowmem_cached_memavailable_thread = property_get_int32("persist.sys.mmms.lowmem.reclaim.memavailable.thread", 393216);
        mi_mms_mediummem_cached_memavailable_thread = property_get_int32("persist.sys.mmms.mediummem.reclaim.memavailable.thread", 445644);
        mi_mms_lower_pre_previous_memavailable_thread = property_get_int32("persist.sys.mmms.lower.pre.previous.memavailable.thread", 262144);
        mi_mms_pre_previous_memavailable_thread = property_get_int32("persist.sys.mmms.pre.previous.memavailable.thread", 262144);
        mi_mms_lower_previous_memavailable_thread = property_get_int32("persist.sys.mmms.lower.previous.memavailable.thread", 102400); /* 400MB */
        mi_mms_previous_memavailable_thread = property_get_int32("persist.sys.mmms.previous.memavailable.thread", 166400); /* 650MB */
        mi_mms_lower_perceptible_memavailable_thread = property_get_int32("persist.sys.mmms.lower.perceptible.memavailable.thread", 76800); /* 300MB */
        mi_mms_perceptible_memavailable_thread = property_get_int32("persist.sys.mmms.perceptible.memavailable.thread", 128000); /* 500MB */
        mi_mms_visible_memavailable_thread = property_get_int32("ro.mmms.visible.memavailable.thread", 102400); /* 400MB */
        mi_mms_visible_lowermem_memavailable_thread = property_get_int32("persist.sys.mmms.visible.lowermem.memavailable.thread", 64000); /* 250MB*/
        mi_mms_visible_lowmem_memavailable_thread = property_get_int32("persist.sys.mmms.visible.lowmem.memavailable.thread", 102400);
        mi_mms_visible_mediummem_memavailable_thread = property_get_int32("persist.sys.mmms.visible.mediummem.memavailable.thread", 102400);
        mi_mms_critical_memavailable_thread = property_get_int32("ro.mmms.critical.memavailable.thread", 51200); /* 200 MB */
        mi_mms_critical_lowmem_memavailable_thread = property_get_int32("persist.sys.mmms.critical.lowmem.memavailable.thread", 51200);
        mi_mms_critical_mediummem_memavailable_thread = property_get_int32("persist.sys.mmms.critical.mediummem.memavailable.thread", 51200);
        mi_mms_kill_subprocess_memavailable_thread = property_get_int32("ro.mmms.kill.subprocess.memavailable.thread", 524288);
        mi_mms_game_reclaim_memavailable_thread = property_get_int32("persist.sys.mmms.game.reclaim.memavailable.thread", 524288);
        mi_mms_cont_medium_event_thread = property_get_int32("ro.mmms.cont.medium.event.thread", 5);
        mi_mms_reclaim_ratio = property_get_int32("ro.mmms.reclaim.ratio", 60);
        mi_mms_system_pool_thread = property_get_int32("ro.mmms.reclaim.pool.thread", 76800);
        mi_mms_system_pool_limit_thread = property_get_int32("ro.mmms.reclaim.pool.limit.thread", 262144);
        mi_mms_lowmem_system_pool_thread = property_get_int32("ro.mmms.lowmem.reclaim.pool.thread", 51200);
        mi_mms_lowmem_system_pool_limit_thread = property_get_int32("ro.mmms.lowmem.reclaim.pool.limit.thread", 262144);
        mi_mms_filepage_thread = property_get_int32("ro.mmms.reclaim.filepage.thread", 524288);
        mi_mms_lowmem_filepage_thread = property_get_int32("persist.sys.mmms.lowmem.reclaim.filepage.thread", 524288);
        wmark_boost_factor = property_get_int32("persist.sys.mmms.wmark.boost.factor", 1);
        mi_mms_lowmem_wmark_boost_factor = property_get_int32("persist.sys.mmms.lowmem.wmark.boost.factor", 1);
        mi_mms_game_wmark_boost_factor = property_get_int32("persist.sys.mmms.game.wmark.boost.factor", 2);
        mi_mms_cont_wmark_low_trigger_pm_thread = property_get_int32("ro.mmms.cont.wmark.low.trigger.pm.thread", 10);
        mi_mms_cont_wmark_low_thread = property_get_int32("ro.mmms.cont.wmark.low.thread", 4);
        mi_mms_lowmem_cont_wmark_low_thread = property_get_int32("persist.sys.mmms.lowmem.cont.wmark.low.thread", 10);
        mi_mms_cont_reclaim_kswapd_thread = property_get_int32("ro.mmms.cont.reclaim.kswapd.thread", 6);
        mi_mms_cont_reclaim_direct_thread = property_get_int32("ro.mmms.cont.reclaim.direct.thread", 4);
        mi_mms_cont_reclaim_direct_throttle_thread = property_get_int32("ro.mmms.cont.reclaim.direct.throttle.thread", 1);
        mi_mms_poll_cont_critical_percentage = property_get_int32("ro.mmms.cont.critical.percentage", 4);
        mi_mms_lowmem_swap_free_low_percentage = property_get_int32("ro.mmms.lowmem.swap.free.low.percentage", 10);
        mi_mms_min_swap_free = property_get_int32("persist.sys.min.swap.free", 76800);
        mi_mms_file_thrashing_percentage = property_get_int32("persist.sys.mmms.file.thrashing.percentage", 10);
        mi_mms_file_thrashing_critical_percentage = property_get_int32("persist.sys.mmms.file.thrashing.critical.percentage", 40);
        mi_mms_file_thrashing_page_thread = property_get_int32("persist.sys.mmms.file.thrashing.page.thread", 153600);
        mi_mms_lowermem_thread = property_get_int32("ro.mmms.lowermem.thread", 4194304);
        mi_mms_lowmem_thread = property_get_int32("ro.mmms.lowmem.thread", 6291456);
        mi_mms_mediummem_thread = property_get_int32("ro.mmms.medium.wmem.thread", 8388608);
        mi_mms_trigger_reclaim_mem_thread = property_get_int32("ro.mmms.trigger.reclaim.mem.thread", 8388608);
        reclaim_switch = property_get_int32("persist.sys.mmms.reclaim_switch", 0);
        mi_mms_log_switch = property_get_bool("ro.mmms.log.switch", false);
        mi_mms_onetrack_report_interval_ms = (unsigned long)property_get_int32("persist.sys.onetrack_report_interval_ms", 60 * 1000);
        mi_mms_mem_report_interval_ms = (unsigned long)property_get_int32("persist.sys.mem_report_interval_ms", 5 * 1000);
        mi_mms_mem_report_adj_selected = property_get_int32("persist.sys.mem_report_adj_selected", 0);
        mem_reclaim_interval_ms = (unsigned long)property_get_int32("persist.sys.mem_reclaim_interval_ms", 2 * 1000);
        mem_reclaim_threhold = (long)property_get_int32("persist.sys.mem_reclaim_threhold", 150000);
        cam_mem_reclaim_switch = property_get_bool("persist.sys.lmk.camera.mem_reclaim", false);
        if (property_get_bool("persist.sys.use_mi_new_strategy", false)) {
            use_minfree_levels = false;
            force_use_old_strategy = false;
        }
        low_ram_device_config();
    }
}

void LmkdImpl::poll_params_reset(struct polling_params __unused *poll_params) {
    if (mi_mms_switch) {
        if (!game_mode) {
            //poll_params->poll_handler = NULL;
            //poll_params->paused_handler = NULL;
        }
    }
    return;
}

void LmkdImpl::mi_memory_pressure_policy(struct memory_pressure_policy_pc *pc) {
    enum mi_to_mms_event_type event_to_mms = NO_EVENT_TO_PM;
    int mms_min_score_adj;
    enum kill_reasons old_kill_reason;

    if (mi_mms_switch) {
        *pc->min_score_adj = 0;
        pc->swap_low_threshold = std::max(mi_mms_min_swap_free, (int32_t)pc->swap_low_threshold);
        pc->kill_reason = NONE;
        pc->file_thrashing = pc->thrashing > std::min(mi_mms_file_thrashing_percentage, pc->thrashing_limit_pct);
        pc->file_critical_thrashing = pc->thrashing > std::min(mi_mms_file_thrashing_critical_percentage, pc->thrashing_critical_pct);
       /*
        * TODO: move this logic into a separate function
        * Decide if killing a process is necessary and record the reason
        */
        if (pc->cycle_after_kill && pc->wmark < WMARK_LOW) {
           /*
            * Prevent kills not freeing enough memory which might lead to OOM kill.
            * This might happen when a process is consuming memory faster than reclaim can
            * free even after a kill. Mostly happens when running memory stress tests.
            */
            pc->kill_reason = PRESSURE_AFTER_KILL;
            strlcpy(pc->kill_desc, "min watermark is breached even after kill", LINE_MAX);
        } else if (pc->reclaim == DIRECT_RECLAIM_THROTTLE && pc->wmark < WMARK_LOW) {
            pc->kill_reason = DIRECT_RECL_AND_THROT;
            strlcpy(pc->kill_desc, "system processes are being throttled", LINE_MAX);
        } else if (pc->level >= VMPRESS_LEVEL_CRITICAL && pc->events != 0 && pc->wmark < WMARK_LOW) {
           /*
            * Device is too busy reclaiming memory which might lead to ANR.
            * Critical level is triggered when PSI complete stall (all tasks are blocked because
            * of the memory congestion) breaches the configured threshold.
            */
            pc->kill_reason = CRITICAL_KILL;
            strlcpy(pc->kill_desc, "critical pressure and device is low on memory", LINE_MAX);
        } else if (pc->swap_is_low && pc->file_thrashing) {
            /* Page cache is thrashing while swap is low */
            pc->kill_reason = LOW_SWAP_AND_THRASHING;
            snprintf(pc->kill_desc, LINE_MAX, "device is low on swap (%" PRId64 "kB < %" PRId64 "kB) and thrashing (%" PRId64 "%%)",
                     pc->mi.field.free_swap * page_k, pc->swap_low_threshold * page_k, pc->thrashing);
            /* Do not kill perceptible apps unless below min watermark or heavily thrashing */
            if (pc->wmark > WMARK_MIN && !pc->file_critical_thrashing) {
                *pc->min_score_adj = STUB_PERCEPTIBLE_APP_ADJ + 1;
            }
        } else if (pc->reclaim == DIRECT_RECLAIM && pc->file_thrashing) {
            /* Page cache is thrashing while in direct reclaim (mostly happens on lowram devices) */
            pc->kill_reason = DIRECT_RECL_AND_THRASHING;
            snprintf(pc->kill_desc, LINE_MAX, "device is in direct reclaim and thrashing (%" PRId64 "%%)", pc->thrashing);
            /* Do not kill perceptible apps unless below min watermark or heavily thrashing */
            if (pc->wmark > WMARK_MIN && !pc->file_critical_thrashing) {
                *pc->min_score_adj = STUB_PERCEPTIBLE_APP_ADJ + 1;
            }
        } else if (pc->mi.field.nr_free_pages < mi_mms_freemem_throttled_thread) {
            pc->kill_reason = (enum kill_reasons)(MI_NEW_KILL_REASON_BASE + WILL_THROTTLED);
            strlcpy(pc->kill_desc, "cw", LINE_MAX);
            *pc->min_score_adj = VISIBLE_APP_ADJ;
        } else if (pc->reclaim == DIRECT_RECLAIM && pc->wmark < WMARK_HIGH && memavailable_is_low(&pc->mi, &pc->zi, NULL)) {
            pc->kill_reason = DIRECT_RECL_AND_LOW_MEM;
            strlcpy(pc->kill_desc, "device is in direct reclaim and low on memory", LINE_MAX);
            *pc->min_score_adj = get_protected_adj(pc);
        } else if (pc->in_compaction && pc->wmark < WMARK_HIGH && memavailable_is_low(&pc->mi, &pc->zi, NULL)) {
            pc->kill_reason = COMPACTION;
            strlcpy(pc->kill_desc, "device is in compaction and low on memory", LINE_MAX);
            *pc->min_score_adj = SERVICE_B_ADJ;
        }

        if (pc->kill_reason != NONE) {
            int old_min_score_adj = *pc->min_score_adj;
            pc->kill_reason = (enum kill_reasons)(pc->kill_reason + MI_UPDATE_KILL_REASON_BASE);
            if (*pc->min_score_adj <= PRE_PREVIOUS_APP_ADJ) {
                if (critical_memavailable_touch(pc)) {
                    if (*pc->min_score_adj < VISIBLE_APP_ADJ) {
                        *pc->min_score_adj = 0;
                    }
                } else if (visible_memavailable_touch(pc)) {
                    if (*pc->min_score_adj < VISIBLE_APP_ADJ) {
                        *pc->min_score_adj = VISIBLE_APP_ADJ;
                    }
                } else if (perceptible_memavailable_touch(pc)) {
                    if (*pc->min_score_adj <= STUB_PERCEPTIBLE_APP_ADJ) {
                        *pc->min_score_adj = STUB_PERCEPTIBLE_APP_ADJ + 1;
                    }
                } else if (previous_memavailable_touch(pc)) {
                    if (*pc->min_score_adj < PREVIOUS_APP_ADJ) {
                        *pc->min_score_adj = PREVIOUS_APP_ADJ;
                    }
                } else if (pre_previous_memavailable_touch(pc)) {
                    if (*pc->min_score_adj < PRE_PREVIOUS_APP_ADJ) {
                        *pc->min_score_adj = PRE_PREVIOUS_APP_ADJ;
                    }
                } else {
                    *pc->min_score_adj = PRE_PREVIOUS_APP_ADJ + 1;
                }

                if (pc->file_critical_thrashing && file_pages_is_low(&pc->mi)) {
                    int new_min_score_adj = *pc->min_score_adj;
                    *pc->min_score_adj = std::min(VISIBLE_APP_ADJ, new_min_score_adj);
                    ALOGD("thrashing (%" PRId64 "%%) old_min_score_adj %d new_min_score_adj %d cur_min_score_adj %d",
                            pc->thrashing, old_min_score_adj, new_min_score_adj, *pc->min_score_adj);
                }
            }
        }

        count_cont_wmark(pc->events, &pc->mi, &pc->zone_mem_info, pc->curr_tm);
        if (pc->events > 0) {
            count_cont_reclaim_type(pc->reclaim, pc->curr_tm);
            if (mi_mms_log_switch) {
                out_log_when_psi(pc->events, pc->level, pc->reclaim, &pc->mi, pc->swap_is_low, pc->thrashing);
            }
        }

        if (should_trigger_preclaim(pc->events, pc->level, &pc->mi, pc->thrashing > pc->thrashing_limit)) {
            trigger_mi_reclaim(pc->level, VMPRESS_POLICY_PRECLAIM);
            if (mi_mms_log_switch) {
                ALOGD("MI_LOG_TO_MMS| events=%d level=%d policy=%d", pc->events, pc->level, VMPRESS_POLICY_PRECLAIM);
            }
        }

        old_kill_reason = pc->kill_reason;
        if (!cam_is_perceptible_support() && mi_special_case(pc->level, &mms_min_score_adj, pc->mi, pc->zi)) {
            if (mms_min_score_adj != OOM_SCORE_ADJ_MAX + 1) {
                *pc->min_score_adj = mms_min_score_adj;
                pc->kill_reason = MINFREE_MODE;
                strlcpy(pc->kill_desc, "special case: using minfree mode", LINE_MAX);
            } else {
                pc->kill_reason = NONE;
            }
            goto source_kill;
        } else if (cam_is_perceptible_support() && cam_perceptible_policy(pc, &mms_min_score_adj)) {
            if (mms_min_score_adj != OOM_SCORE_ADJ_MAX + 1) {
                *pc->min_score_adj = mms_min_score_adj;
                pc->kill_reason = MINFREE_MODE;
                snprintf(pc->kill_desc, LINE_MAX, "special case: using minfree mode,old_reason:%d", pc->kill_reason );
            } else {
                pc->kill_reason = NONE;
            }

             goto source_kill;
         }

        if (pc->kill_reason != NONE &&
                pc->mi.field.kreclaimable > 0 && pc->mi.field.sreclaimable > 0 &&
                ((pc->mi.field.kreclaimable - pc->mi.field.sreclaimable) > mi_mms_system_pool_limit_thread)) {
            if (pc->events != 0) {
                ALOGD("no killing reason %d for pool", pc->kill_reason);
            }
            pc->kill_reason = NONE;
            goto source_kill;
        }

        if (pc->kill_reason == NONE && cont_event_memavailable_touch(pc)
                && cont_event_handle(pc->events, pc->level, &event_to_mms)) {
            pc->kill_reason = (enum kill_reasons)(MI_UPDATE_KILL_REASON_BASE + MI_NEW_KILL_REASON_BASE + event_to_mms);
            snprintf(pc->kill_desc, LINE_MAX, "%s", mms_event_name[event_to_mms]);
            *pc->min_score_adj = PRE_PREVIOUS_APP_ADJ;
            if (event_to_mms == CONT_LOW_WMARK &&
                    previous_memavailable_touch(pc)) {
                *pc->min_score_adj = PREVIOUS_APP_ADJ;
            }
            goto source_kill;
        }

        if (pc->kill_reason != NONE || mvmp.mi_polling_check || pc->reclaim > KSWAPD_RECLAIM ||
            pc->level > VMPRESS_LEVEL_MEDIUM || should_kill_subprocess(&pc->mi, &pc->zi, NULL)) {
            int polling_window;

            polling_window = pc->events == 0 ? 0 : 1;
            if (pc->level <= VMPRESS_LEVEL_MEDIUM &&
                (pc->mi.field.kreclaimable != 0 && pc->mi.field.kreclaimable > mi_mms_system_pool_thread) &&
                reclaim_rate_ok(polling_window) && pc->thrashing < pc->thrashing_limit) {
                pc->kill_reason = NONE;
                goto mi_no_kill;
            }

            if (pc->swap_is_low) {
                if (old_kill_reason != NONE && !polling_window) {
                    event_to_mms = SWAP_LOW_AND_MEMNOAVAILABLE;
                    trigger_mi_reclaim(pc->level, VMPRESS_POLICY_TO_MI_PM);
                }
            } else {
                if ((pc->mi.field.kreclaimable != 0 && pc->mi.field.kreclaimable < mi_mms_system_pool_thread) ||
                    (!polling_window && should_kill_subprocess(&pc->mi, &pc->zi, NULL))) {
                    event_to_mms = SWAP_OK_MEMNOAVAILABLE;
                    trigger_mi_reclaim(pc->level, VMPRESS_POLICY_TO_MI_PM);
                }
            }

        mi_no_kill:
            if (mi_mms_log_switch &&
                (old_kill_reason != NONE && pc->kill_reason == NONE)) {
                ALOGD("MI_LOG_FILTER_KILL|"
                      " skill_desc=%s"
                      " events=%d level=%d event_to_mms=%d(%s) swap_is_low=%d"
                      " reclaim_type=%d wmarklow=%d"
                      " kreclaimable=%" PRId64 "kB"
                      " mi_polling_check=%d"
                      " cont_reclaim_count=kswapd:%d,direct:%d,direct_throttle:%d"
                      " reclaim_ratio=kswapd:%d,direct:%d"
                      " thrashing=%" PRId64 " thrashing_limit=%d"
                      " in_compaction=%d",
                      pc->kill_desc,
                      pc->events, pc->level, event_to_mms, mms_event_name[event_to_mms],
                      pc->swap_is_low, pc->reclaim, pc->wmark <= WMARK_LOW,
                      pc->mi.field.kreclaimable * page_k,
                      mvmp.mi_polling_check,
                      mvmp.cont_reclaim_type_count_between_window[KSWAPD_RECLAIM],
                      mvmp.cont_reclaim_type_count_between_window[DIRECT_RECLAIM],
                      mvmp.cont_reclaim_type_count_between_window[DIRECT_RECLAIM_THROTTLE],
                      mvmp.kswapd_reclaim_ratio[polling_window],
                      mvmp.direct_reclaim_ratio[polling_window],
                      pc->thrashing, pc->thrashing_limit,
                      pc->in_compaction);
            }
        } else {
            int64_t memavailable = 0;
            if (pc->swap_is_low && memavailable_is_low(&pc->mi, &pc->zi, &memavailable)) {
                pc->kill_reason = LOW_SWAP_AND_LOW_FILE;
                snprintf(pc->kill_desc, LINE_MAX, "swap is low and filepage is also low "
                                                  "free_swap=%" PRId64 " memavailable=%" PRId64,
                         pc->mi.field.free_swap, memavailable);
            }
        }

    source_kill:
        if (mi_mms_log_switch && event_to_mms != NO_EVENT_TO_PM) {
            ALOGD("MI_LOG_TO_PM|"
                  " skill_desc=%s"
                  " events=%d level=%d event_to_mms=%d(%s) swap_is_low=%d"
                  " reclaim_type=%d wmarklow=%d"
                  " kreclaimable=%" PRId64 "kB"
                  " mi_polling_check=%d"
                  " con_reclaim_count=kswapd:%d,direct:%d,direct_throttle:%d"
                  " thrashing=%" PRId64 " thrashing_limit=%d"
                  " in_compaction=%d",
                  pc->kill_desc,
                  pc->events, pc->level, event_to_mms, mms_event_name[event_to_mms],
                  pc->swap_is_low, pc->reclaim, pc->wmark <= WMARK_LOW,
                  pc->mi.field.kreclaimable * page_k,
                  mvmp.mi_polling_check,
                  mvmp.cont_reclaim_type_count_between_window[KSWAPD_RECLAIM],
                  mvmp.cont_reclaim_type_count_between_window[DIRECT_RECLAIM],
                  mvmp.cont_reclaim_type_count_between_window[DIRECT_RECLAIM_THROTTLE],
                  pc->thrashing, pc->thrashing_limit,
                  pc->in_compaction);
        }

        mvmp.mi_polling_check = false;
    }
}
#endif

extern "C" ILmkdStub* create() {
    return new LmkdImpl;
}

extern "C" void destroy(ILmkdStub* impl) {
    delete impl;
}
