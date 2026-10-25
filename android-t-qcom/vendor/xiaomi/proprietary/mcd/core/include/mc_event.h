#ifndef MC_EVENT_H
#define MC_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdbool.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#ifdef OCTVM_USES_LOGD
#if (PLATFORM_SDK_VERSION >= 29)
#include <log/log.h>
#else
#include <log/logger.h>
#endif
#endif

#define STDLOG_EVENT_PATH "/dev/log/events"
#define INPUT_DEVICE_PATH "/dev/input/"

#define DEFAULT_EVENT_TIMEOUT      500
#define DEFAULT_MAX_EVENT_TIMEOUT  5*1000
#define UEVENT_MSG_LEN             1024
#define MAX_INPUT_DEVICE           16

#define EMPTY_MESSAGE    0
#define BAD_MESSAGE      1
#define IGNORED_MESSAGE  2
#define CORRUPT_MESSAGE  3

/*
 * module android standard events structure and apis
 */
struct stdlog_event {
    int pid;
    int tag_id;
    time_t timestamp;
    char msg_body[1024];
};
#ifdef OCTVM_USES_LOGD
struct logger_list *mc_event_stdlog_event_init(void);
int mc_event_poll_stdlog_event(struct logger_list *logger_list, struct stdlog_event *target_event);
bool check_logd_is_alive(struct logger_list *logger_list);
#else
int mc_event_stdlog_event_init(void);
int mc_event_poll_stdlog_event(int stdlog_fd, struct stdlog_event *target_event);
#endif

typedef struct stdlog_event_tagseg {
    int tag_id_base;
    int continuous_count;
}Tag_seg;
//add a not overlapped segment
int mc_event_stdlog_event_add_observe(Tag_seg *tag_seg);
//del which base and count all matched
int mc_event_stdlog_event_del_observe(Tag_seg *tag_seg);
int mc_event_stdlog_event_in_observe(int tag_id);

/*
 * module uevents structure and apis
 */
struct mc_uevent {
    int count;
    char **messages;
};
int mc_event_uevent_init(void);
int mc_event_poll_uevent(int uevent_fd, struct mc_uevent *target_event);
int mc_event_uevent_add_observe(char *subsystem_devpath);
int mc_event_uevent_del_observe(char *subsystem_devpath);
int mc_event_uevent_in_observe(struct mc_uevent *target_event);

/*
 * module input events structure and apis
 */
struct mc_input_watch_set {
    struct pollfd *ufds;
    int nfds;
    char **device;
};
int mc_event_input_event_init(struct mc_input_watch_set *watch_set);
void mc_event_input_event_deinit(struct mc_input_watch_set *watch_set);
int mc_event_poll_input_event(struct mc_input_watch_set *watch_set,struct input_event *target_event);
int mc_event_input_event_add_observe(uint16_t type);
int mc_event_input_event_del_observe(uint16_t type);
int mc_event_input_event_in_observe(uint16_t type);

#define MAX_WSET_PATTERN_NUM 10
#define MAX_FILE_NAME_LEN    128
struct mc_fs_watchset {
    int nfds;
    struct pollfd ufds[1];
    char *watch_path;
    int watch_mask;
    int pattern_chk; //default not check
    char *event_pattern[MAX_WSET_PATTERN_NUM];
    char revent_name[MAX_FILE_NAME_LEN];
    int revent_mask;
};
struct mc_fs_watchset *mc_event_creat_fs_watchset(char * path, int watch_mask);
void mc_event_destroy_fs_watchset(struct mc_fs_watchset *wset);
int mc_event_fs_watchset_add_observe(struct mc_fs_watchset *wset, char *pattern);
void mc_event_fs_watchset_del_observe(struct mc_fs_watchset *wset, char *pattern);
int mc_event_poll_fs_watchset(struct mc_fs_watchset *wset, int time_out);

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */

#endif
