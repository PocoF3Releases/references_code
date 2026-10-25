#ifndef __TOUCHDEVICE_H__
#define __TOUCHDEVICE_H__

#include <map>
#include <utils/threads.h>

#define MAX_SLOT    10
#define MAX_INPUT_NAME_SIZE 128

using namespace android;
using namespace std;

enum touch_state {
    TOUCH_UP = 0,
    TOUCH_DOWN = 1,
    TOUCH_MOVE = 2,
};

struct tp_event_data {
    int64_t press_time;
    int64_t release_time;
    int64_t move_time;
    int abs_x;
    int abs_y;
    int press_x;
    int press_y;
    int release_x;
    int release_y;
    int move_x;
    int move_y;
    char is_pressed;
    char is_startpoint;
    char touch_state;
};

typedef void (*TOUCH_FUNC)(int slot, int64_t time, int x, int y);
typedef int (*findPropertyFunc)(const char *device);

enum {
    TOUCH_DEVICE,
    PEN_DEVICE,
    KEYBOARD_DEVICE,
    KEY_POWER_DEVICE,
    INPUT_DEVICE_NUM,
};
#define PER_DEVICE_NUM 5

class TouchDevice : public Thread {
public:
    TouchDevice();
    ~TouchDevice();

    int input_fd[INPUT_DEVICE_NUM][PER_DEVICE_NUM];
    findPropertyFunc findProperty[INPUT_DEVICE_NUM];

    int abs_x;
    int abs_y;
    struct tp_event_data tp_event_data[MAX_SLOT];
    struct tp_event_data pen_event_data;
    int64_t first_down_time;
    int64_t last_up_time;
    int findInputDevice();

    void geteventTouch(struct input_event *event);
    void geteventPen(struct input_event *event);
    void geteventKeyPower(struct input_event *event);
    void threadRun();
    virtual void set_touchfunc_call(TOUCH_FUNC td, TOUCH_FUNC tm, TOUCH_FUNC tu);
    virtual void touchKey(int64_t, uint32_t, int);
    virtual void touchDown(int, int64_t, int, int);
    virtual void touchUp(int, int64_t, int, int);
    virtual void touchMove(int, int64_t, int, int);
    virtual void touchRelease();
    virtual void penDown(int64_t, int, int);
    virtual void penUp(int64_t, int, int);
    virtual void penMove(int64_t, int, int);
    virtual void touchCmd(int*);
    virtual void touchSuspendState(int);
    virtual void abnormalEvent(struct abnormal_event*);
private:
    const char *input_device_dir = "/dev/input";
    TOUCH_FUNC touchDownCb;
    TOUCH_FUNC touchMoveCb;
    TOUCH_FUNC touchUpCb;
    int searchDir(const char *dirname, findPropertyFunc findProperty);
    virtual bool threadLoop();
    map<string, bool> isFind;
};

#endif
