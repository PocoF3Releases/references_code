//#define DEBUG

#include "TouchMain.h"
#include "TouchDevice.h"
#include <list>
#include <map>
#include <json/json.h>
#include <time.h>

static vector <touch_point> down_point_list;
static vector <touch_point> pen_down_point_list;
static vector <touch_check_func_struct*> touch_func_list;
static struct touch_point_state point_state[MAX_SLOT];
static struct touch_point_state pen_point_state;
static Timer slot_check_timer[MAX_SLOT];
static Json::Value event_json;

enum {
    CHECK_DOWN_FUNC,
    CHECK_MOVE_FUNC,
    CHECK_UP_FUNC,
    CHECK_RELEASE_FUNC,
    CHECK_KEY_FUNC,
    CHECK_CMD_FUNC,
    CHECK_SUSPEND_STATE_FUNC,
    CHECK_ABNORMAL_EVENT_FUNC,
    CHECK_PEN_DOWN_FUNC,
    CHECK_PEN_MOVE_FUNC,
    CHECK_PEN_UP_FUNC,
    CHECK_FUNC_SIZE,
};

static map<string, list<touch_check_func_struct*>> touch_event_map;
static vector<vector<touch_check_func_struct*>> checkpoints(CHECK_FUNC_SIZE);

long read_val(char const* path)
{
    char str[200];
    int fd;
    int cnt;

    fd=open(path, O_RDONLY);
    cnt = read(fd, str, 200);
    close(fd);
    if (cnt <= 0)
        return 0;
    return atol(str);
}

char *skip_space(char *str) {
    while (*str != '\0' && *str == ' ')
        ++str;
    return str;
}
char *skip_to_space(char *str) {
    while (*str != '\0' && *str != ' ')
        ++str;
    return str;
}

void inc_count(struct count_type *count) {
    if (count) {
        count->cnt++;
    }
}

void TouchDevice::touchKey(int64_t, uint32_t code, int value)
{
    call_check_key_func(code, value);
}

void TouchDevice::touchDown(int slot, int64_t time, int x, int y)
{
    touch_point tp_point;
    tp_point.slot = slot;
    tp_point.press_time = time;
    tp_point.release_time = 0;
    tp_point.press_x = x;
    tp_point.press_y = y;

    point_state[slot].press_time = time;
    point_state[slot].press_x = x;
    point_state[slot].press_y = y;
    point_state[slot].has_moved = false;
    point_state[slot].is_pressed = true;
    point_state[slot].move_x = x;
    point_state[slot].move_y = y;
    point_state[slot].release_x = 0;
    point_state[slot].release_y = 0;
    point_state[slot].release_time = 0;

    if (tp_point.press_time) {
        if (down_point_list.size() == RECORD_POINT_NUM)
            down_point_list.erase(down_point_list.begin());
        down_point_list.push_back(tp_point);
    }
    call_check_down_func(&down_point_list);
    //slot_check_timer[slot].set(LONG_PRESS, slot_check_longpress_timer_func);
    //ALOGE("%s, slot:%2d, time:%ld, X:%2d, Y:%2d", __func__, slot, time, x, y);
}

void TouchDevice::touchUp(int slot, int64_t time, int x, int y)
{
    point_state[slot].release_x = x;
    point_state[slot].release_y = y;
    point_state[slot].is_pressed = false;
    point_state[slot].release_time = time;
    vector <touch_point> :: iterator v = down_point_list.begin();
    while (v != down_point_list.end()) {
        if (v->slot == slot && !v->release_time && v->press_time) {
            v->release_time = time;
            break;
        }
        v++;
    }
    //slot_check_timer[slot].stop();
    call_check_up_func(&down_point_list);
    //ALOGE("%s, slot:%2d, time:%ld, X:%2d, Y:%2d", __func__, slot, time, x, y);
}

void TouchDevice::touchMove(int slot, int64_t, int x, int y)
{
    //ALOGE("%s", __func__);
    point_state[slot].move_x = x;
    point_state[slot].move_y = y;
    point_state[slot].has_moved = true;
    //slot_check_timer[slot].stop();
    call_check_move_func(&down_point_list);
}

void TouchDevice::touchRelease()
{
    /*int i = 0;
    vector <touch_point> :: iterator v = down_point_list.begin();
    while (v != down_point_list.end()) {
        ALOGE("%s, Point:%2d, slot:%2d, X:%4d,Y:%4d, P:%ld,R:%ld", __func__,
        i++, v->slot, v->press_x, v->press_y, v->press_time, v->release_time);
        v++;
    }*/
    call_check_release_func(&down_point_list);
}

void TouchDevice::touchCmd(int *cmd_buf)
{
    call_check_cmd_func(cmd_buf);
}

void TouchDevice::touchSuspendState(int suspend_state)
{
    call_check_suspend_state_func(suspend_state);
}

void TouchDevice::abnormalEvent(struct abnormal_event *event)
{
    call_check_abnormal_event_func(event);
}

void TouchDevice::penDown(int64_t time, int x, int y) {
    touch_point tp_point;
    tp_point.slot = 0;
    tp_point.press_time = time;
    tp_point.release_time = 0;
    tp_point.press_x = x;
    tp_point.press_y = y;

    pen_point_state.press_time = time;
    pen_point_state.press_x = x;
    pen_point_state.press_y = y;
    pen_point_state.has_moved = false;
    pen_point_state.is_pressed = true;
    pen_point_state.move_x = x;
    pen_point_state.move_y = y;
    pen_point_state.release_x = 0;
    pen_point_state.release_y = 0;
    pen_point_state.release_time = 0;

    if (tp_point.press_time) {
        if (pen_down_point_list.size() == RECORD_POINT_NUM)
            pen_down_point_list.erase(pen_down_point_list.begin());
        pen_down_point_list.push_back(tp_point);
    }
    call_check_pen_down_func(&pen_down_point_list);
}
void TouchDevice::penUp(int64_t time, int x, int y) {
    call_check_pen_up_func(&pen_down_point_list);
}
void TouchDevice::penMove(int64_t, int x, int y) {
    call_check_pen_move_func(&pen_down_point_list);
}


void register_check_func(struct touch_check_func_struct *check_struct)
{
    if (!check_struct)
        return;

    if (check_struct->report_immediately == false) {
        if (check_struct->alloc_data == NULL) {
            ALOGE("%s: Failed to register %s, alloc_data=NULL", __func__, check_struct->type_name);
            return;
        }
        check_struct->alloc_data(check_struct);
        if (check_struct->data == NULL) {
            ALOGE("%s: Failed to register %s, alloc_data failed", __func__, check_struct->type_name);
            return;
        }
    }

    pthread_mutex_init(&check_struct->lock, NULL);
    touch_func_list.push_back(check_struct);

    map<string, list<touch_check_func_struct*>> *event_map;
    event_map = &touch_event_map;
    if (check_struct->parent) {
        (*event_map)[string(check_struct->parent)].push_back(check_struct);
    } else {
        (*event_map)[string(check_struct->type_name)].push_back(check_struct);
    }
    if (check_struct->check_down_func)
        checkpoints[CHECK_DOWN_FUNC].push_back(check_struct);
    if (check_struct->check_up_func)
        checkpoints[CHECK_UP_FUNC].push_back(check_struct);
    if (check_struct->check_move_func)
        checkpoints[CHECK_MOVE_FUNC].push_back(check_struct);
    if (check_struct->check_release_func)
        checkpoints[CHECK_RELEASE_FUNC].push_back(check_struct);
    if (check_struct->check_key_func)
        checkpoints[CHECK_KEY_FUNC].push_back(check_struct);
    if (check_struct->check_cmd_func)
        checkpoints[CHECK_CMD_FUNC].push_back(check_struct);
    if (check_struct->check_suspend_state_func)
        checkpoints[CHECK_SUSPEND_STATE_FUNC].push_back(check_struct);
    if (check_struct->check_abnormal_event_func)
        checkpoints[CHECK_ABNORMAL_EVENT_FUNC].push_back(check_struct);
    if (check_struct->check_pen_down_func)
        checkpoints[CHECK_PEN_DOWN_FUNC].push_back(check_struct);
    if (check_struct->check_pen_up_func)
        checkpoints[CHECK_PEN_UP_FUNC].push_back(check_struct);
    if (check_struct->check_pen_move_func)
        checkpoints[CHECK_PEN_MOVE_FUNC].push_back(check_struct);

    ALOGI("%s: Success to register %s", __func__, check_struct->type_name);
}

void call_check_down_func(vector <touch_point> *point_list)
{
    for (auto check_struct : checkpoints[CHECK_DOWN_FUNC]) {
        check_struct->check_down_func(check_struct, point_list);
    }
}
void call_check_up_func(vector <touch_point> *point_list)
{
    for (auto check_struct : checkpoints[CHECK_UP_FUNC]) {
        check_struct->check_up_func(check_struct, point_list);
    }
}
void call_check_move_func(vector <touch_point> *point_list)
{
    for (auto check_struct : checkpoints[CHECK_MOVE_FUNC]) {
        check_struct->check_move_func(check_struct, point_list);
    }
}
void call_check_release_func(vector <touch_point> *point_list)
{
    for (auto check_struct : checkpoints[CHECK_RELEASE_FUNC]) {
        check_struct->check_release_func(check_struct, point_list);
    }
}
void call_check_key_func(uint32_t code, int value) {
    for (auto check_struct : checkpoints[CHECK_KEY_FUNC]) {
        check_struct->check_key_func(check_struct, code, value);
    }
}
void call_check_cmd_func(int* cmd_buf) {
    for (auto check_struct : checkpoints[CHECK_CMD_FUNC]) {
        check_struct->check_cmd_func(check_struct, cmd_buf);
    }
}
void call_check_suspend_state_func(int suspend_state) {
    for (auto check_struct : checkpoints[CHECK_SUSPEND_STATE_FUNC]) {
        check_struct->check_suspend_state_func(check_struct, suspend_state);
    }
}
void call_check_abnormal_event_func(struct abnormal_event *event) {
    for (auto check_struct : checkpoints[CHECK_ABNORMAL_EVENT_FUNC]) {
        check_struct->check_abnormal_event_func(check_struct, event);
    }
}
void call_check_pen_down_func(vector <touch_point> *point_list)
{
    for (auto check_struct : checkpoints[CHECK_PEN_DOWN_FUNC]) {
        check_struct->check_pen_down_func(check_struct, NULL);
    }
}
void call_check_pen_up_func(vector <touch_point> *point_list)
{
    for (auto check_struct : checkpoints[CHECK_PEN_UP_FUNC]) {
        check_struct->check_pen_up_func(check_struct, NULL);
    }
}
void call_check_pen_move_func(vector <touch_point> *point_list)
{
    for (auto check_struct : checkpoints[CHECK_PEN_MOVE_FUNC]) {
        check_struct->check_pen_move_func(check_struct, NULL);
    }
}

/* TouchUseData */
bool check_touch_click(touch_check_func_struct *check_struct, vector <touch_point> *)
{
    inc_count((struct count_type *)check_struct->data);
    return true;
}

bool check_double_wakeup(touch_check_func_struct *check_struct, uint32_t code, int)
{
    if (code == KEY_WAKEUP) {
        ALOGD("%s true", __func__);
        inc_count((struct count_type*)check_struct->data);
        return true;
    }
    return false;
}

bool check_fod_single(touch_check_func_struct *check_struct, uint32_t code, int)
{
    if (code == KEY_GOTO) {
        ALOGD("%s true", __func__);
        inc_count((struct count_type*)check_struct->data);
        return true;
    }
    return false;
}

bool check_fod_unlock(touch_check_func_struct *check_struct, uint32_t code, int)
{
    if (code == BTN_INFO) {
        ALOGD("%s true", __func__);
        return true;
    }
    return false;
}

bool check_water_mode_func(touch_check_func_struct *check_struct, int *cmd_buf)
{
    if (cmd_buf[0] == SET_CUR_VALUE && cmd_buf[2] == THP_HAL_TOUCH_SENSOR) {
        if (cmd_buf[3] > 300 && cmd_buf[3] < 400) { /* water */
            LOGD("%s", __FUNCTION__);
            inc_count((struct count_type*)check_struct->data);
            return true;
        }
    }
    return false;
}

bool check_glove_mode_func(touch_check_func_struct *check_struct, int *cmd_buf)
{
    if (cmd_buf[0] == SET_CUR_VALUE && cmd_buf[2] == THP_HAL_TOUCH_SENSOR) {
        if (cmd_buf[3] > 200 && cmd_buf[3] < 300) { /* glove */
            LOGD("%s ", __FUNCTION__);
            inc_count((struct count_type*)check_struct->data);
            return true;
        }
    }
    return false;
}

bool check_film_mode_func(touch_check_func_struct *check_struct, int *cmd_buf)
{
    if (cmd_buf[0] == SET_CUR_VALUE && cmd_buf[2] == THP_HAL_TOUCH_SENSOR) {
        if (cmd_buf[3] >= 100 && cmd_buf[3] < 200) { /* film */
            LOGD("%s =%d", __FUNCTION__, cmd_buf[3] - 100);
            Json::Value value;
            value["event"] = check_struct->parent;
            value[check_struct->type_name] = (Json::Int64)(cmd_buf[3] - 100);
            event_json.append(value);
            return true;
        }
    }
    return false;
}
/* TouchException */
/* frozen */
static Timer frozen_timer;
static bool resume = false, power_key = false;
static uint64_t irq_count_after_resume = 0;
static time_t time_prev = 0;

uint64_t read_touch_interrupts() {
    static long file_offset = 0;
    static int line_offset = 0;
    uint64_t irq_count = 0;
    char *p;
    int len = 0;
    char buf[512];
    bool found = false;
    FILE *file = fopen(PROC_INTERRUPTS_PATH, "r");
    if (!file) {
        ALOGE("%s: Failed to open %s, error=%s\n", __FUNCTION__, PROC_INTERRUPTS_PATH, strerror(errno));
        return -1;
    }
    while (!feof(file)) {
        if (file_offset > 0) {
            fseek(file, file_offset, SEEK_SET);
        }
        fgets(buf, 512, file);
        p = buf + line_offset;
        len = line_offset;
        while (*p != '\n') {
            if ((!found) && (!strncmp(IRQ_NAME, p, strlen(IRQ_NAME)))) {
                found = true;
                LOGD("%s\n", buf);
            }
            ++p;
            ++len;
        }
        if (found) {
            line_offset = p - buf;
            file_offset = ftell(file) - (len + 1);
            break;
        } else if (file_offset) {
            fseek(file, 0, SEEK_SET);
            file_offset = 0;
            line_offset = 0;
        }
    }
    if (found) {
        p = buf;
        p = skip_space(p);
        p = skip_to_space(p);
        p = skip_space(p);
        while (!isalpha(*p)) {
            uint64_t tmp;
            sscanf(p, "%lu ", &tmp);
            irq_count += tmp;
            p = skip_to_space(p);
            p = skip_space(p);
        }
    }
    LOGD("irq_count=%lu\n", irq_count);
    fclose(file);
    return irq_count;
}

static void check_frozen_timer_func(union sigval)
{
    static touch_check_func_struct *frozen_check = NULL;
    uint64_t irq_count_cur = read_touch_interrupts();
    time_t time_cur = time(NULL);

    if (frozen_check == NULL) {
        for (auto check : touch_func_list) {
            if (!strncmp(check->type_name, "frozen", sizeof("frozen"))) {
                frozen_check = check;
            }
        }
    }
    if (frozen_check != NULL) {
        if ((irq_count_cur != 0) &&
            (irq_count_cur - irq_count_after_resume == 0) &&
            (time_prev == 0 || (time_cur - time_prev > FROZEN_DELAY_S))) {
            time_prev = time_cur;
            pthread_mutex_lock(&frozen_check->lock);
            inc_count((struct count_type *)(frozen_check->data));
            pthread_mutex_unlock(&frozen_check->lock);
            ALOGE("%s: maybe frozen", __FUNCTION__);
        }
    }
    resume = power_key = false;
}

static bool frozen_suspend_state(touch_check_func_struct *check_struct, int suspend_state) {
    LOGD("%s", __FUNCTION__);
    if (suspend_state == 0) {
        if (power_key == true) {
            if (frozen_timer.isRunning() == false) {
                LOGD("%s: start frozen timer", __FUNCTION__);
                irq_count_after_resume = read_touch_interrupts();
                frozen_timer.set(FROZEN_INTERVAL_US, check_frozen_timer_func);
            }
        }
        resume = true;
    } else {
        resume = power_key = false;
        frozen_timer.stop();
    }
    return true;
}

static bool frozen_power_key(touch_check_func_struct *check_struct, uint32_t code, int value) {
    LOGD("%s", __FUNCTION__);
    if (code == KEY_POWER && value == 1) {
        if (resume == true) {
            if (frozen_timer.isRunning() == false) {
                LOGD("%s: start frozen timer", __FUNCTION__);
                irq_count_after_resume = read_touch_interrupts();
                frozen_timer.set(FROZEN_INTERVAL_US, check_frozen_timer_func);
            }
        }
        power_key = true;
    }
    return true;
}

/* ghost */
static bool check_ghost_func(touch_check_func_struct *check_struct, struct abnormal_event *event) {
    LOGD("%s: %d %d %d", __FUNCTION__, event->type, event->code, event->value);
    if (event->type == ABNORMAL_EVENT_TYPE_ABNORMAL &&
        event->code == ABNORMAL_EVENT_CODE_GHOST) {
        inc_count((struct count_type *)(check_struct->data));
        return true;
    }
    return false;
}
/* Pen */
/* use time */
static Timer pen_use_timer;
static time_t start_time, end_time;

static void check_pen_use_timer_func(union sigval)
{
    static touch_check_func_struct *pen_use_time_check = NULL;
    if (pen_use_time_check == NULL) {
        for (auto check : touch_func_list) {
            if (!strncmp(check->type_name, "use_time", sizeof("use_time"))) {
                pen_use_time_check = check;
            }
        }
    }
    if (pen_use_time_check != NULL) {
        pthread_mutex_lock(&pen_use_time_check->lock);
        Json::Value value;
        value["event"] = pen_use_time_check->parent;
        value[pen_use_time_check->type_name] = (Json::Int64)(end_time - start_time);
        LOGD("%s: start %lld", __FUNCTION__, end_time - start_time);
        event_json.append(value);
        start_time = 0;
        end_time = 0;
        pthread_mutex_unlock(&pen_use_time_check->lock);
    }
}
bool check_use_time_down_func(touch_check_func_struct *check_struct, vector <touch_point> *) {
    if (start_time == 0) {
        start_time = time(0);
        LOGD("%s: start %lld", __FUNCTION__, start_time);
    }
    return true;
}
bool check_use_time_up_func(touch_check_func_struct *check_struct, vector <touch_point> *) {
    end_time = time(0) ;
    LOGD("%s: end %lld", __FUNCTION__, end_time);
    pen_use_timer.stop();
    pen_use_timer.set(PEN_USE_TIME_INTERVAL, check_pen_use_timer_func);
    return true;
}
/* active_num */
bool check_pen_active_num_func(touch_check_func_struct *check_struct, vector <touch_point> *) {
    static bool reported = false;
    if (reported == false) {
        inc_count((struct count_type *)(check_struct->data));
        reported = true;
    }
    return true;
}

template <typename T>
void alloc_data(touch_check_func_struct *check_struct) {
    check_struct->data = new T(check_struct->type_name);
    if (check_struct->data == NULL) {
        ALOGE("Failed to alloc data for %s", check_struct->type_name);
    }
}

const char *event_touch_use_data = "TouchUseData";
const char *event_touch_exception = "TouchException";
const char *event_game_mode = "GameMode";
const char *event_pen = "Pen";
const char *event_keyboard = "KeyBoard";

struct touch_check_func_struct init_check_list[] = {
    {
        .type_name = "click_num",
        .parent = event_touch_use_data,
        .alloc_data = alloc_data<count_type>,
        .check_down_func = check_touch_click,
    },
    {
        .type_name = "doubletap_wakeup",
        .parent = event_touch_use_data,
        .alloc_data = alloc_data<count_type>,
        .check_key_func = check_double_wakeup,
    },
    {
        .type_name = "fod_unlock",
        .parent = event_touch_use_data,
        .alloc_data = alloc_data<count_type>,
        .check_key_func = check_fod_unlock,
    },
    {
        .type_name = "frozen",
        .parent = event_touch_exception,
        .alloc_data = alloc_data<count_type>,
        .check_key_func = frozen_power_key,
        .check_suspend_state_func = frozen_suspend_state,
    },
    {
        .type_name = "active_num",
        .parent = "Pen",
        .alloc_data = alloc_data<count_type>,
        .check_pen_down_func = check_pen_active_num_func,
    },
    {
        .type_name = "use_time",
        .parent = "Pen",
        .report_immediately = true,
        .check_pen_down_func = check_use_time_down_func,
        .check_pen_up_func = check_use_time_up_func,
    },
    {
        .type_name = "water_mode_num",
        .parent = "TouchUseData",
        .alloc_data = alloc_data<count_type>,
        .check_cmd_func = check_water_mode_func,
    },
    {
        .type_name = "glove_mode_num",
        .parent = "TouchUseData",
        .alloc_data = alloc_data<count_type>,
        .check_cmd_func = check_glove_mode_func,
    },
    {
        .type_name = "film_num",
        .parent = "TouchUseData",
        .report_immediately = true,
        .check_cmd_func = check_film_mode_func,
    },
    {
        .type_name = "ghost",
        .parent = event_touch_exception,
        .alloc_data = alloc_data<count_type>,
        .check_abnormal_event_func = check_ghost_func,
    },
    {
        .type_name = "GameMode",
        .alloc_data = alloc_data<game_mode_type>,
    },
    {
        .type_name = NULL,
    }
};

void register_check_func_list()
{
    struct touch_check_func_struct *p = init_check_list;

    while (p->type_name != NULL) {
        register_check_func(p);
        p++;
    }
}

void unregister_check_func_list() {
    for (auto check : touch_func_list) {
        if (check->data) {
            delete check->data;
        }
    }
    touch_func_list.clear();
    touch_event_map.clear();
    checkpoints.clear();
}

void print_func_list()
{
    for (auto event : touch_event_map) {
        ALOGD("event: %s, param:", event.first.c_str());
        for (auto param : event.second) {
            if (param->parent) {
                ALOGD("%s ", param->type_name);
            } else {
                if (param->data) {
                    string str;
                    param->data->toStr(str);
                    ALOGD("%s", str.c_str());
                }
            }
        }
    }
}

static void clear_data()
{
    for (auto v : touch_func_list) {
        if (v->type_name && v->data) {
            pthread_mutex_lock(&v->lock);
            v->data->clear();
            pthread_mutex_unlock(&v->lock);
        }
        v++;
    }
}

const char* get_event_string() {
    static string result;

    if (touch_func_list.empty())
        return NULL;

    for (auto event : touch_event_map) {
        bool update = false;
        Json::Value value;
        for (auto param : event.second) {
            if (param->data) {
                update |= param->data->toJson(value);
            }
        }
        if (update) {
            value["event"] = event.first;
            event_json.append(value);
        }
    }
    if (event_json.size() > 0) {
        result = event_json.toStyledString();
        LOGD("%s: %s", __FUNCTION__, result.c_str());
        event_json.clear();
        clear_data();
        return result.c_str();
    }
    return NULL;
}
