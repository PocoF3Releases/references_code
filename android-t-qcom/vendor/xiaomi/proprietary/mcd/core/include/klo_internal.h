#ifndef OCTVM_KLO_INTERNAL_H
#define OCTVM_KLO_INTERNAL_H

#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#include <cutils/list.h>
#include <private/android_filesystem_config.h>

#include "octvm.h"
#include "mc_event.h"
#include "klo_task.h"
#include "klo_power.h"
#include "klo_stability.h"
#include "android_helper.h"

#define KLO_DATA_PATH     "/data/system/mcd/klo"
#define KLO_DATA_VERSION  "/data/system/mcd/klo/data_version"
#define KLO_DUMP_FILES    "/data/system/mcd/klo/dump_files"
#define BOOT_COMPLETE_PS  "/data/system/mcd/klo/boot_complete_ps"

/*
 * this maybe different for some OEM
 */
#define MIUI_VERSION_PROP "ro.build.version.incremental"

#define MAX_RECORD_FILE_LEN       256
#define MAX_EPOLL_EVENTS          64
#define KLO_TASK_EPOLL_TIMEOUT    60*1000
#define MAX_KLO_RECORD_FILE_NUM   10
#define MAX_KLO_RECORD_KEEP_TIME  (48*60*60)

//periodic klo task intervals in seconds
#define DEFAULT_KLO_TASK_INTERVAL_MIN (60 * 60 * 1)
#define DEFAULT_KLO_TASK_INTERVAL_MAX (60 * 60 * 12)

struct klo_event {
    struct listnode list;
    int event_type;
    void *event;
};

struct klo_mm {
    int objsize;
    int blocks;
    void *pool;
    void *freelist;
    char *pool_name;
    struct listnode blklist;
    pthread_mutex_t mutex;
};

enum {
    EVENT_TYPE_START = -1,
    EVENT_TYPE_END = 0,
    EVENT_TYPE_STD = 1,
    EVENT_TYPE_SIMPLE = 2,
};

enum {
    SOURCE_TYPE_MIN = 1,
    SOURCE_DROPBOX  = SOURCE_TYPE_MIN,
    SOURCE_TOMESTONE,
    SOURCE_LOGD_LOGGER,
    SOURCE_KERNEL_LOGGER,
    SOURCE_TYPE_MAX = SOURCE_KERNEL_LOGGER,
};

struct _klo_task_internal {
    struct listnode list;
    uint32_t task_id;
    klo_task_t *task;
    time_t time_add_task;
    time_t time_set_alarm;
    uint32_t time_to_alarm;
};
typedef struct _klo_task_internal klo_task_internal_t;

enum {
    KLO_DUMP_FILE_CLEAR = 0,
    KLO_DUMP_FILE_ADD = 1,
    KLO_DUMP_FILE_REMOVE = 2,
    KLO_DUMP_FILE_NOP = 3,
};

struct _klo_file_internal {
    struct listnode list;
    KLOFileDesc file_desc;
};
typedef struct _klo_file_internal klo_file_internal_t;

/*klo memory pool*/
struct klo_mm *klo_mm_init(int objsize, char *pool_name);
void *klo_mm_alloc(struct klo_mm *km);
void klo_mm_free(struct klo_mm *km, void *addr);
void klo_mm_deinit(struct klo_mm *km);
/*free interface for outter compoment*/
void klo_do_free(int, void *);

/*klo ipc interface*/
int start_processing_thread();
void stop_processing_thread();
int klo_prepare_observer(int sourceType);
int klo_destroy_observer(int sourceType);
int klo_poll_events(void **event, int sourceType);
void klo_notify_event(void *event, int eventType);
void klo_dump(int dump_fd, int argc, const char **argv);
void klo_set_dump_files(int num_files, const char **file_descs);
void redirect_event_2klo(const char *event);
void klo_load_dump_files();
void record_boot_complete_info();

/*klo stability*/
void klo_stability_cleanup();
void klo_check_abnormal_reboot();
void klo_check_missed_native_crash(time_t rec_start_time);
void klo_process_activity_manager_event(struct stdlog_event *target_event);
void klo_add_native_crash_record(FILE *record_fp, time_t rec_time);
void klo_clear_entry_list(struct listnode *list);
void klo_clear_subtype_entry_list(struct listnode *list);
void klo_free_listnode_list(struct listnode *list);
void klo_lock_sys_klo_list();
void klo_unlock_sys_klo_list();

/*klo activity launch time*/
void klo_activity_profiles_reset();
void klo_dump_activity_launch_time(int dump_fd);
void klo_process_launch_time_event(struct stdlog_event *target_event);

/*klo power&battery*/
void klo_power_state_global_init();
void klo_record_sys_power_event(int8_t event_type, time_t event_ts);
void klo_process_powermanager_event(struct stdlog_event *target_event);
void klo_dump_system_power_profile(int fd);

/*klo dumper*/
int klo_get_klo_fd(char *fn_pattern, int flag);
void klo_file_clear_old(time_t time);
void klo_dump_for_klo_task(klo_task_t *task);
void klo_dump_kernel_summary(int fd, bool clear_records);
void klo_dump_android_summary(int fd, bool clear_records);
void klo_dump_file(int fd, KLOFileDesc *file_desc);

/*klo reporter*/
void klo_reporter_cleanup();
void klo_reporter_mainloop();
int klo_register_klo_task(klo_task_t *task);
int klo_unregister_klo_task(int32_t task_id);

/*klo monitor input rate*/
int32_t notify_IME_change(const char *event);
void klo_inputrate_thread_loop(void);
void klo_dump_inputrate(int fd);

#endif
