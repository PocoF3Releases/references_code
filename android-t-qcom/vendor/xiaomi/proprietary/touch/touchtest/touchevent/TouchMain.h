#ifndef __TOUCHMAIN_H__
#define __TOUCHMAIN_H__
#define LOG_TAG "TouchEventCheck"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <time.h>
#include <inttypes.h>
#include <math.h>
#include <linux/input.h>
#include <sys/resource.h>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <queue>
#include <vector>
#include <list>
#include <map>
#include <utils/Log.h>
#include <utils/threads.h>
#include "Data.h"
#include "Timer.h"
#include "XiaomiTouch.h"

#define BITS_PER_LONG  (sizeof(unsigned long) * 8)
#define BITS_TO_LONGS(x)  (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define set_bit(bit, array)   (array)[(bit)/BITS_PER_LONG] |=(1L << ((bit) % BITS_PER_LONG))
#define clear_bit(bit, array)   (array)[(bit)/BITS_PER_LONG] &=(~ (1L << ((bit) % BITS_PER_LONG)))
#define test_bit(bit, array)   ((array)[(bit)/BITS_PER_LONG] & (1L << ((bit) % BITS_PER_LONG)))

#define MAX_SLOT   10
#define UMECS_PER_SEC 1000000
#define RECORD_POINT_NUM 10

#define BTN_INFO 0x152

#define CHARGE_STATUS "/sys/class/power_supply/usb/online"
#define TOUCH_THP_CMD_PATH "/sys/class/touch/touch_dev/touch_thp_cmd_ready"
#define SUSPEND_STATE_PATH "/sys/class/touch/touch_dev/suspend_state"
#define ABNORMAL_EVENT_PATH "/sys/class/touch/touch_dev/abnormal_event"
#define PROC_INTERRUPTS_PATH "/proc/interrupts"

#define FROZEN_INTERVAL_US (2 * 1000 * 1000)
#define FROZEN_DELAY_S (3 * 60)
#define PEN_USE_TIME_INTERVAL 2 * 60 * 1000 * 1000

#ifdef DEBUG
#define LOGD(fmt, ...) ALOGD(fmt, ##__VA_ARGS__)
#else
#define LOGD(fmt, ...)
#endif

#define IRQ_NAME "xiaomi_tp"

using namespace android;
using namespace std;

struct touch_point {
    int64_t press_time;
    int64_t release_time;
    int slot;
    int press_x;
    int press_y;
};

struct touch_point_state {
    int64_t press_time;
    int64_t release_time;
    int press_x;
    int press_y;
    int move_x;
    int move_y;
    int release_x;
    int release_y;
    bool is_pressed;
    bool has_moved;
};

struct touch_slot_state {
    int64_t first_down_time;
    int64_t all_release_time;
    int64_t bit_map;
    int point_cnt;
};

struct touch_check_func_struct {
    const char *type_name;
    const char *parent;
    bool report_immediately = false;
    data_interface *data;  /* protected by lock */
    pthread_mutex_t lock;

    void (*alloc_data)(touch_check_func_struct *check_struct);
    bool (*check_down_func)(touch_check_func_struct *check_struct, vector <touch_point> *point_list);
    bool (*check_up_func)(touch_check_func_struct *check_struct, vector <touch_point> *point_list);
    bool (*check_move_func)(touch_check_func_struct *check_struct, vector <touch_point> *point_list);
    bool (*check_release_func)(touch_check_func_struct *check_struct, vector <touch_point> *point_list);
    bool (*check_key_func)(touch_check_func_struct *check_struct, uint32_t code, int value);
    bool (*check_cmd_func)(touch_check_func_struct *check_struct, int* cmd_buf);
    bool (*check_suspend_state_func)(touch_check_func_struct *check_struct, int suspend_state);
    bool (*check_abnormal_event_func)(touch_check_func_struct *check_struct, struct abnormal_event *event);
    bool (*check_pen_down_func)(touch_check_func_struct *check_struct, vector <touch_point> *point_list);
    bool (*check_pen_up_func)(touch_check_func_struct *check_struct, vector <touch_point> *point_list);
    bool (*check_pen_move_func)(touch_check_func_struct *check_struct, vector <touch_point> *point_list);

};

void register_check_func(struct touch_check_func_struct *ts);
void call_check_down_func(vector <touch_point> *point_list);
void call_check_up_func(vector <touch_point> *point_list);
void call_check_move_func(vector <touch_point> *point_list);
void call_check_release_func(vector <touch_point> *point_list);
void call_check_key_func(uint32_t code, int value);
void call_check_pen_down_func(vector <touch_point> *point_list);
void call_check_pen_up_func(vector <touch_point> *point_list);
void call_check_pen_move_func(vector <touch_point> *point_list);
void call_check_cmd_func(int *cmd_buf);
void call_check_suspend_state_func(int suspend_state);
void call_check_abnormal_event_func(struct abnormal_event*);
void register_check_func_list();
void unregister_check_func_list();
void print_func_list();
const char* get_event_string();

#endif
