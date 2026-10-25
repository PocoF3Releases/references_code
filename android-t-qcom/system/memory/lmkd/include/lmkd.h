/*
 *  Copyright 2018 Google, Inc
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _LMKD_H_
#define _LMKD_H_

#include <arpa/inet.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <time.h>

__BEGIN_DECLS

/*
 * Supported LMKD commands
 */
enum lmk_cmd {
    LMK_TARGET = 0,         /* Associate minfree with oom_adj_score */
    LMK_PROCPRIO,           /* Register a process and set its oom_adj_score */
    LMK_PROCREMOVE,         /* Unregister a process */
    LMK_PROCPURGE,          /* Purge all registered processes */
    LMK_GETKILLCNT,         /* Get number of kills */
    LMK_SUBSCRIBE,          /* Subscribe for asynchronous events */
    LMK_PROCKILL,           /* Unsolicited msg to subscribed clients on proc kills */
    LMK_UPDATE_PROPS,       /* Reinit properties */
    LMK_STAT_KILL_OCCURRED, /* Unsolicited msg to subscribed clients on proc kills for statsd log */
    LMK_STAT_STATE_CHANGED, /* Unsolicited msg to subscribed clients on state changed */
#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
    LMK_SPT_MODE = 16, /*Get Speed Test Mode flag*/
    LMK_GAME_MODE = 17,
#endif
    LMK_FORCE_CHECK_LIST = 20, /*Force check blacklist*/
    LMK_SET_WHITELIST,      /*Force check whitelist*/
    LMK_SET_BLACKLIST,      /*Force check whitelist*/
    //TODO support swap low percent for mtk platform
    LMK_ADJ_SWAP_PERCENT,   /* Adjust swap free low percentage */
    //TODO support switch to minfree in camera mode
    LMK_CAMERA_MODE,        /* Update camera foreground state*/
#ifdef QCOM_FEATURE_ENABLE
    LMK_RECLAIM_FOR_CAMERA = 25, /*Reclaim page for camera*/
#endif
    LMK_SET_PROTECT_LIST = 26,
    LMK_SET_PERCEPTIBLE_LIST = 27,
    LMK_SET_PSI_LEVEL = 28,
    LMK_MEMREPORT = 30,
    LMK_CLEAN_CACHED = 40,     /* Clean all cached processes */
};

/*
 * Max number of targets in LMK_TARGET command.
 */
#define MAX_TARGETS 6

/*
 * Max packet length in bytes.
 * Longest packet is LMK_TARGET followed by MAX_TARGETS
 * of minfree and oom_adj_score values
 */
#define CTRL_PACKET_MAX_SIZE (sizeof(int) * (MAX_TARGETS * 2 + 1))

#define LINE_MAX 128

/* LMKD packet - first int is lmk_cmd followed by payload */
typedef int LMKD_CTRL_PACKET[CTRL_PACKET_MAX_SIZE / sizeof(int)];

/* Get LMKD packet command */
static inline enum lmk_cmd lmkd_pack_get_cmd(LMKD_CTRL_PACKET pack) {
    return (enum lmk_cmd)ntohl(pack[0]);
}

/* LMK_TARGET packet payload */
struct lmk_target {
    int minfree;
    int oom_adj_score;
};

/* memory pressure levels */
enum vmpressure_level {
    VMPRESS_LEVEL_LOW = 0,
    VMPRESS_LEVEL_MEDIUM,
    VMPRESS_LEVEL_CRITICAL,
    VMPRESS_LEVEL_SUPER_CRITICAL,
    VMPRESS_LEVEL_COUNT
};

/* Fields to parse in /proc/vmstat */
enum vmstat_field {
    VS_FREE_PAGES,
    VS_INACTIVE_FILE,
    VS_ACTIVE_FILE,
    VS_WORKINGSET_REFAULT,
    VS_WORKINGSET_REFAULT_FILE,
#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
#ifdef TEMP_DEBUG
    VS_DIRTY,
    VS_WRITEBACK,
    VS_DIRTIED,
    VS_WRITTEN,
    VS_PGPGIN,
    VS_PGPGOUT,
    VS_PGPGOUTCLEAN,
    VS_PSWPIN,
    VS_PSWPOUT,
#endif
    VS_PGSTEAL_KSWAPD,
    VS_PGSTEAL_DIRECT,
#endif
    VS_PGSCAN_KSWAPD,
    VS_PGSCAN_DIRECT,
    VS_PGSCAN_DIRECT_THROTTLE,
    VS_PGSKIP_FIRST_ZONE,
    VS_PGSKIP_DMA = VS_PGSKIP_FIRST_ZONE,
#ifdef QCOM_FEATURE_ENABLE
    VS_PGSKIP_DMA32,
#endif
    VS_PGSKIP_NORMAL,
    VS_PGSKIP_HIGH,
    VS_PGSKIP_MOVABLE,
    VS_PGSKIP_LAST_ZONE = VS_PGSKIP_MOVABLE,
    VS_COMPACT_STALL,
    VS_FIELD_COUNT
};

union vmstat {
    struct {
        int64_t nr_free_pages;
        int64_t nr_inactive_file;
        int64_t nr_active_file;
        int64_t workingset_refault;
        int64_t workingset_refault_file;
#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
#ifdef TEMP_DEBUG
        int64_t nr_dirty;
        int64_t nr_writeback;
        int64_t nr_dirtied;
        int64_t nr_written;
        int64_t pgpgin;
        int64_t pgpgout;
        int64_t pgpgoutclean;
        int64_t pswpin;
        int64_t pswpout;
#endif
        int64_t pgsteal_kswapd;
        int64_t pgsteal_direct;
#endif
        int64_t pgscan_kswapd;
        int64_t pgscan_direct;
        int64_t pgscan_direct_throttle;
        int64_t pgskip_dma;
#ifdef QCOM_FEATURE_ENABLE
        int64_t pgskip_dma32;
#endif
        int64_t pgskip_normal;
        int64_t pgskip_high;
        int64_t pgskip_movable;
        int64_t compact_stall;
    } field;
    int64_t arr[VS_FIELD_COUNT];
};

/* Fields to parse in /proc/zoneinfo */
/* zoneinfo per-zone fields */
enum zoneinfo_zone_field {
    ZI_ZONE_NR_FREE_PAGES = 0,
    ZI_ZONE_MIN,
    ZI_ZONE_LOW,
    ZI_ZONE_HIGH,
    ZI_ZONE_PRESENT,
    ZI_ZONE_NR_FREE_CMA,
    ZI_ZONE_FIELD_COUNT
};

/* see __MAX_NR_ZONES definition in kernel mmzone.h */
#define MAX_NR_ZONES 6

union zoneinfo_zone_fields {
    struct {
        int64_t nr_free_pages;
        int64_t min;
        int64_t low;
        int64_t high;
        int64_t present;
        int64_t nr_free_cma;
    } field;
    int64_t arr[ZI_ZONE_FIELD_COUNT];
};

struct zoneinfo_zone {
    union zoneinfo_zone_fields fields;
    int64_t protection[MAX_NR_ZONES];
    int64_t max_protection;
    char name[LINE_MAX];
};

/* zoneinfo per-node fields */
enum zoneinfo_node_field {
    ZI_NODE_NR_INACTIVE_FILE = 0,
    ZI_NODE_NR_ACTIVE_FILE,
    ZI_NODE_FIELD_COUNT
};

union zoneinfo_node_fields {
    struct {
        int64_t nr_inactive_file;
        int64_t nr_active_file;
    } field;
    int64_t arr[ZI_NODE_FIELD_COUNT];
};

struct zoneinfo_node {
    int id;
    int zone_count;
    struct zoneinfo_zone zones[MAX_NR_ZONES];
    union zoneinfo_node_fields fields;
};

struct reread_data {
    const char* const filename;
    int fd;
};

/* for now two memory nodes is more than enough */
#define MAX_NR_NODES 2

struct zoneinfo {
    int node_count;
    struct zoneinfo_node nodes[MAX_NR_NODES];
    int64_t totalreserve_pages;
    int64_t total_inactive_file;
    int64_t total_active_file;
};

enum zone_watermark {
    WMARK_MIN = 0,
    WMARK_LOW,
    WMARK_HIGH,
    WMARK_NONE
};

static const char *wmark_str[WMARK_NONE + 1] {
    "min",
    "low",
    "high",
    "none"
};

struct zone_watermarks {
    long high_wmark;
    long low_wmark;
    long min_wmark;
};

struct zone_meminfo {
    int64_t nr_free_pages;
    int64_t cma_free;
    struct zone_watermarks watermarks;

};

/* Fields to parse in /proc/meminfo */
enum meminfo_field {
    MI_NR_TOTAL_PAGES = 0,
    MI_NR_FREE_PAGES,
    MI_CACHED,
    MI_SWAP_CACHED,
    MI_BUFFERS,
    MI_SHMEM,
    MI_UNEVICTABLE,
    MI_TOTAL_SWAP,
    MI_FREE_SWAP,
    MI_ACTIVE_ANON,
    MI_INACTIVE_ANON,
    MI_ACTIVE_FILE,
    MI_INACTIVE_FILE,
#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
    MI_KRECLAIMABLE,
#endif
    MI_SRECLAIMABLE,
    MI_SUNRECLAIM,
    MI_KERNEL_STACK,
    MI_PAGE_TABLES,
    MI_ION_HELP,
    MI_ION_HELP_POOL,
    MI_CMA_FREE,
    MI_FIELD_COUNT
};

static const char* const meminfo_field_names[MI_FIELD_COUNT] = {
    "MemTotal:",
    "MemFree:",
    "Cached:",
    "SwapCached:",
    "Buffers:",
    "Shmem:",
    "Unevictable:",
    "SwapTotal:",
    "SwapFree:",
    "Active(anon):",
    "Inactive(anon):",
    "Active(file):",
    "Inactive(file):",
#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
    "KReclaimable:",
#endif
    "SReclaimable:",
    "SUnreclaim:",
    "KernelStack:",
    "PageTables:",
    "ION_heap:",
    "ION_heap_pool:",
    "CmaFree:",
};

union meminfo {
    struct {
        int64_t nr_total_pages;
        int64_t nr_free_pages;
        int64_t cached;
        int64_t swap_cached;
        int64_t buffers;
        int64_t shmem;
        int64_t unevictable;
        int64_t total_swap;
        int64_t free_swap;
        int64_t active_anon;
        int64_t inactive_anon;
        int64_t active_file;
        int64_t inactive_file;
#if defined(QCOM_FEATURE_ENABLE) && defined(MI_PERF_FEATURE)
        int64_t kreclaimable;
#endif
        int64_t sreclaimable;
        int64_t sunreclaimable;
        int64_t kernel_stack;
        int64_t page_tables;
        int64_t ion_heap;
        int64_t ion_heap_pool;
        int64_t cma_free;
        /* fields below are calculated rather than read from the file */
        int64_t nr_file_pages;
        int64_t total_gpu_kb;
    } field;
    int64_t arr[MI_FIELD_COUNT];
};

#define PGSKIP_IDX(x) (x - VS_PGSKIP_FIRST_ZONE)

/*
 * For LMK_TARGET packet get target_idx-th payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_target(LMKD_CTRL_PACKET packet, int target_idx,
                                        struct lmk_target* target) {
    target->minfree = ntohl(packet[target_idx * 2 + 1]);
    target->oom_adj_score = ntohl(packet[target_idx * 2 + 2]);
}

/*
 * Prepare LMK_TARGET packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_target(LMKD_CTRL_PACKET packet, struct lmk_target* targets,
                                          size_t target_cnt) {
    int idx = 0;
    packet[idx++] = htonl(LMK_TARGET);
    while (target_cnt) {
        packet[idx++] = htonl(targets->minfree);
        packet[idx++] = htonl(targets->oom_adj_score);
        targets++;
        target_cnt--;
    }
    return idx * sizeof(int);
}

/* Process types for lmk_procprio.ptype */
enum proc_type {
    PROC_TYPE_FIRST,
    PROC_TYPE_APP = PROC_TYPE_FIRST,
    PROC_TYPE_SERVICE,
    PROC_TYPE_COUNT,
};

/* LMK_PROCPRIO packet payload */
struct lmk_procprio {
    pid_t pid;
    uid_t uid;
    int oomadj;
    enum proc_type ptype;
};

enum polling_update {
    POLLING_DO_NOT_CHANGE,
    POLLING_START,
#ifdef QCOM_FEATURE_ENABLE
    POLLING_CRIT_UPGRADE,
#endif
    POLLING_PAUSE,
    POLLING_RESUME,
};

/*
 * Data used for periodic polling for the memory state of the device.
 * Note that when system is not polling poll_handler is set to NULL,
 * when polling starts poll_handler gets set and is reset back to
 * NULL when polling stops.
 */
struct polling_params {
    struct event_handler_info* poll_handler;
    struct event_handler_info* paused_handler;
    struct timespec poll_start_tm;
    struct timespec last_poll_tm;
    int polling_interval_ms;
    enum polling_update update;
};

/* data required to handle events */
struct event_handler_info {
    int data;
    void (*handler)(int data, uint32_t events, struct polling_params *poll_params);
};

/* data required to handle socket events */
struct sock_event_handler_info {
    int sock;
    pid_t pid;
    uint32_t async_event_mask;
    struct event_handler_info handler_info;
};

/* max supported number of data connections (AMS, init, tests) */
#define MAX_DATA_CONN 3

/* socket event handler data */
static struct sock_event_handler_info ctrl_sock;
static struct sock_event_handler_info data_sock[MAX_DATA_CONN];

/* vmpressure event handler data */
static struct event_handler_info vmpressure_hinfo[VMPRESS_LEVEL_COUNT];

#define PSI_CONT_EVENT_THRESH (4)

struct watermark_info {
    char name[LINE_MAX];
    int free;
    int high;
    int cma;
    int present;
    int lowmem_reserve[MAX_NR_ZONES];
    int inactive_anon;
    int active_anon;
    int inactive_file;
    int active_file;
};

/*
 * For LMK_PROCPRIO packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_procprio(LMKD_CTRL_PACKET packet, int field_count,
                                          struct lmk_procprio* params) {
    params->pid = (pid_t)ntohl(packet[1]);
    params->uid = (uid_t)ntohl(packet[2]);
    params->oomadj = ntohl(packet[3]);
    /* if field is missing assume PROC_TYPE_APP for backward compatibility */
    params->ptype = field_count > 3 ? (enum proc_type)ntohl(packet[4]) : PROC_TYPE_APP;
}

/*
 * Prepare LMK_PROCPRIO packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_procprio(LMKD_CTRL_PACKET packet, struct lmk_procprio* params) {
    packet[0] = htonl(LMK_PROCPRIO);
    packet[1] = htonl(params->pid);
    packet[2] = htonl(params->uid);
    packet[3] = htonl(params->oomadj);
    packet[4] = htonl((int)params->ptype);
    return 5 * sizeof(int);
}

/* LMK_PROCREMOVE packet payload */
struct lmk_procremove {
    pid_t pid;
};

/*
 * For LMK_PROCREMOVE packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_procremove(LMKD_CTRL_PACKET packet,
                                            struct lmk_procremove* params) {
    params->pid = (pid_t)ntohl(packet[1]);
}

/*
 * Prepare LMK_PROCREMOVE packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_procremove(LMKD_CTRL_PACKET packet,
                                              struct lmk_procremove* params) {
    packet[0] = htonl(LMK_PROCREMOVE);
    packet[1] = htonl(params->pid);
    return 2 * sizeof(int);
}

/*
 * Prepare LMK_PROCPURGE packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_procpurge(LMKD_CTRL_PACKET packet) {
    packet[0] = htonl(LMK_PROCPURGE);
    return sizeof(int);
}

/* LMK_GETKILLCNT packet payload */
struct lmk_getkillcnt {
    int min_oomadj;
    int max_oomadj;
};

/*
 * For LMK_GETKILLCNT packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_getkillcnt(LMKD_CTRL_PACKET packet,
                                            struct lmk_getkillcnt* params) {
    params->min_oomadj = ntohl(packet[1]);
    params->max_oomadj = ntohl(packet[2]);
}

/*
 * Prepare LMK_GETKILLCNT packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_getkillcnt(LMKD_CTRL_PACKET packet,
                                              struct lmk_getkillcnt* params) {
    packet[0] = htonl(LMK_GETKILLCNT);
    packet[1] = htonl(params->min_oomadj);
    packet[2] = htonl(params->max_oomadj);
    return 3 * sizeof(int);
}

/*
 * Prepare LMK_GETKILLCNT reply packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_getkillcnt_repl(LMKD_CTRL_PACKET packet, int kill_cnt) {
    packet[0] = htonl(LMK_GETKILLCNT);
    packet[1] = htonl(kill_cnt);
    return 2 * sizeof(int);
}

/* Types of asynchronous events sent from lmkd to its clients */
enum async_event_type {
    LMK_ASYNC_EVENT_FIRST,
    LMK_ASYNC_EVENT_KILL = LMK_ASYNC_EVENT_FIRST,
    LMK_ASYNC_EVENT_STAT,
    LMK_ASYNC_EVENT_COUNT,
};

/* LMK_SUBSCRIBE packet payload */
struct lmk_subscribe {
    enum async_event_type evt_type;
};

/*
 * For LMK_SUBSCRIBE packet get its payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_subscribe(LMKD_CTRL_PACKET packet, struct lmk_subscribe* params) {
    params->evt_type = (enum async_event_type)ntohl(packet[1]);
}

/**
 * Prepare LMK_SUBSCRIBE packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_subscribe(LMKD_CTRL_PACKET packet, enum async_event_type evt_type) {
    packet[0] = htonl(LMK_SUBSCRIBE);
    packet[1] = htonl((int)evt_type);
    return 2 * sizeof(int);
}

/**
 * Prepare LMK_PROCKILL unsolicited packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_prockills(LMKD_CTRL_PACKET packet, pid_t pid, uid_t uid) {
    packet[0] = htonl(LMK_PROCKILL);
    packet[1] = htonl(pid);
    packet[2] = htonl(uid);
    return 3 * sizeof(int);
}

static inline size_t lmkd_pack_set_memreport(LMKD_CTRL_PACKET packet) {
    packet[0] = htonl(LMK_MEMREPORT);
    return sizeof(int);
}

/*
 * Prepare LMK_UPDATE_PROPS packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_update_props(LMKD_CTRL_PACKET packet) {
    packet[0] = htonl(LMK_UPDATE_PROPS);
    return sizeof(int);
}

/*
 * Prepare LMK_UPDATE_PROPS reply packet and return packet size in bytes.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline size_t lmkd_pack_set_update_props_repl(LMKD_CTRL_PACKET packet, int result) {
    packet[0] = htonl(LMK_UPDATE_PROPS);
    packet[1] = htonl(result);
    return 2 * sizeof(int);
}

/* LMK_PROCPRIO reply payload */
struct lmk_update_props_reply {
    int result;
};

/*
 * For LMK_UPDATE_PROPS reply payload.
 * Warning: no checks performed, caller should ensure valid parameters.
 */
static inline void lmkd_pack_get_update_props_repl(LMKD_CTRL_PACKET packet,
                                          struct lmk_update_props_reply* params) {
    params->result = ntohl(packet[1]);
}

struct adjslot_list {
    struct adjslot_list *next;
    struct adjslot_list *prev;
};

struct proc {
    struct adjslot_list asl;
    int pid;
    int pidfd;
    uid_t uid;
    int oomadj;
    pid_t reg_pid; /* PID of the process that registered this record */
    struct proc *pidhash_next;
};

__END_DECLS

#endif /* _LMKD_H_ */
