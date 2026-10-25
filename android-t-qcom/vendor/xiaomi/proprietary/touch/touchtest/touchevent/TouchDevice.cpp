#include "TouchMain.h"
#include "TouchDevice.h"

const char *input_device_name[INPUT_DEVICE_NUM] = {
    "touch", "pen", "keyboard", "power"
};

static int findTouchProperty(const char *device) {
    int fd;
    unsigned long ev_bits[BITS_TO_LONGS(EV_MAX)];
    unsigned long absBitmask[BITS_TO_LONGS(ABS_MAX)];
    int clkid = SYSTEM_TIME_MONOTONIC;
    char buffer[MAX_INPUT_NAME_SIZE];
    struct input_absinfo abs_x;
    struct input_absinfo abs_y;

    fd = open(device, O_RDONLY | O_NONBLOCK);
    if(fd < 0) {
        return -1;
    }
    /* read the evbits of the input device */
    if(ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
        close(fd);
        return -1;
    }

    if(!test_bit(EV_ABS, ev_bits)) {
        close(fd);
        return -1;
    }

    if(ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absBitmask)), absBitmask) < 0) {
        close(fd);
        return -1;
    }

    if(test_bit(ABS_MT_POSITION_X, absBitmask)
       && test_bit(ABS_MT_POSITION_Y, absBitmask)) {

        if(ioctl(fd, EVIOCGNAME(sizeof(buffer) - 1), &buffer) < 1) {
            close(fd);
            return -1;
        } else {
            buffer[sizeof(buffer) - 1] = '\0';
        }

        if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &abs_x) < 0
           || ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &abs_y) < 0) {
            close(fd);
            return -1;
        }
    } else {
        close(fd);
        return -1;
    }

    if (!strncmp(buffer, "Xiaomi Touch", sizeof("Xiaomi Touch"))) {
        close(fd);
        return -1;
    }

    return fd;
}

static int findPenProperty(const char *device) {
    int fd;
    unsigned long absBitmask[BITS_TO_LONGS(ABS_MAX)];
    unsigned long ev_bits[BITS_TO_LONGS(EV_MAX)];
    int clkid = SYSTEM_TIME_MONOTONIC;
    char buffer[MAX_INPUT_NAME_SIZE];
    struct input_absinfo abs_x;
    struct input_absinfo abs_y;

    fd = open(device, O_RDONLY | O_NONBLOCK);
    if(fd < 0) {
        return -1;
    }
    /* read the evbits of the input device */
    if(ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
        close(fd);
        return -1;
    }

    if(!test_bit(EV_ABS, ev_bits)) {
        close(fd);
        return -1;
    }

    if(ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absBitmask)), absBitmask) < 0) {
        close(fd);
        return -1;
    }

    if(test_bit(ABS_X, absBitmask)
       && test_bit(ABS_Y, absBitmask)) {

        if(ioctl(fd, EVIOCGNAME(sizeof(buffer) - 1), &buffer) < 1) {
            close(fd);
            return -1;
        } else {
            buffer[sizeof(buffer) - 1] = '\0';
        }

        if(ioctl(fd, EVIOCGABS(ABS_X), &abs_x) < 0
           || ioctl(fd, EVIOCGABS(ABS_Y), &abs_y) < 0) {
            close(fd);
            return -1;
        }
    } else {
        close(fd);
        return -1;
    }

    return fd;
}

static int findKeyBoardProperty(const char *device) {
    return -1;
}

static int findKeyPowerProperty(const char *device) {
    int fd;
    unsigned long ev_bits[BITS_TO_LONGS(EV_MAX)];
    unsigned long keyBitmask[BITS_TO_LONGS(KEY_MAX)];

    fd = open(device, O_RDONLY | O_NONBLOCK);
    if(fd < 0) {
        return -1;
    }
    /* read the evbits of the input device */
    if(ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
        close(fd);
        return -1;
    }

    if(!test_bit(EV_KEY, ev_bits)) {
        close(fd);
        return -1;
    }

    if(ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBitmask)), keyBitmask) < 0) {
        close(fd);
        return -1;
    }

    if(!test_bit(KEY_POWER, keyBitmask)) {
        close(fd);
        return -1;
    }

    return fd;
}

TouchDevice::TouchDevice() {
    findProperty[TOUCH_DEVICE] = findTouchProperty;
    findProperty[PEN_DEVICE] = findPenProperty;
    findProperty[KEYBOARD_DEVICE] = findKeyBoardProperty;
    findProperty[KEY_POWER_DEVICE] = findKeyPowerProperty;

    for (int i = 0; i < INPUT_DEVICE_NUM; ++i) {
        for (int j = 0; j < PER_DEVICE_NUM; ++j) {
            input_fd[i][j] = -1;
        }
    }
}

TouchDevice::~TouchDevice() {
    for (int i = 0; i < INPUT_DEVICE_NUM; ++i) {
        for (int j = 0; j < PER_DEVICE_NUM; ++j) {
            if (input_fd[i][j] > 0) {
                close(input_fd[i][j]);
            }
        }
    }
}

int TouchDevice::searchDir(const char *dirname, findPropertyFunc findProperty) {
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    int fd = 0;
    struct dirent *de;

    if (findProperty == NULL) {
        ALOGE("Invalid findProperty");
        return -1;
    }
    dir = opendir(dirname);
    if (dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        if (isFind[string(filename)] == false) {
            fd = findProperty(devname);
            if (fd > 0) {
                ALOGI("%s: %s", __FUNCTION__, filename);
                isFind[string(filename)] = true;
                break;
            }
        }
    }
    closedir(dir);
    return fd;
}

int TouchDevice::findInputDevice() {
    int retval;
    int cnt = 0, res = 0;
    struct pollfd ufds;

    for (int i = 0; i < INPUT_DEVICE_NUM; ++i) {
        for (int j = 0; j < PER_DEVICE_NUM; ++j) {
            input_fd[i][j] = searchDir(input_device_dir, findProperty[i]);
            if (input_fd[i][j] > 0) {
                ALOGE("Find %s device, fd = %d", input_device_name[i], input_fd[i][j]);
                res = 1;
            }
        }
    }
    if (res > 0)
        return res;

    ufds.fd = inotify_init();
    ufds.events = POLLIN;
    retval = inotify_add_watch(ufds.fd, input_device_dir, IN_DELETE | IN_CREATE);
    if(retval < 0) {
        ALOGE("could not add watch for %s\n", input_device_dir);
        return -1;
    }
    while(1) {
        poll(&ufds, 1, -1);
        if(ufds.revents & POLLIN) {
            for (int i = 0; i < INPUT_DEVICE_NUM; ++i) {
                for (int j = 0; j < PER_DEVICE_NUM; ++j) {
                    input_fd[i][j] = searchDir(input_device_dir, findProperty[i]);
                    if (input_fd[i][j] > 0) {
                        ALOGE("Find %s device, fd = %d", input_device_name[i], input_fd[i][j]);
                        res = 1;
                    }
                }
            }
            if (res > 0)
                break;
            if (cnt++ > 10) {
                ALOGE("retry %d\n", cnt);
                return -1;
            }
        }
    }
    return res;
}

void TouchDevice::geteventTouch(struct input_event *event) {
    static int slot_num = 0;
    int slot = 0;

    if (event->code == ABS_MT_SLOT) {
        slot = event->value;
    } else if (event->code == ABS_MT_TRACKING_ID) {
        if (event->value >= 0) {
            tp_event_data[slot].press_time = event->time.tv_sec * UMECS_PER_SEC + event->time.tv_usec;
            tp_event_data[slot].is_startpoint = 1;
            slot_num++;
        }  else {
            tp_event_data[slot].release_time = event->time.tv_sec * UMECS_PER_SEC + event->time.tv_usec;
            tp_event_data[slot].release_x = tp_event_data[slot].abs_x;
            tp_event_data[slot].release_y = tp_event_data[slot].abs_y;
            tp_event_data[slot].touch_state = TOUCH_UP;
            if (slot_num)
                slot_num--;
        }
    } else if (event->code == BTN_TOUCH) {
        if (event->value) {
            first_down_time = event->time.tv_sec * UMECS_PER_SEC + event->time.tv_usec;
        } else {
            last_up_time = event->time.tv_sec * UMECS_PER_SEC + event->time.tv_usec;
        }
    } else if (event->code == ABS_MT_POSITION_X) {
        tp_event_data[slot].abs_x = event->value;
    } else if (event->code == ABS_MT_POSITION_Y) {
        tp_event_data[slot].abs_y = event->value;
    } else if (event->code == SYN_REPORT && tp_event_data[slot].abs_x != -1
               && tp_event_data[slot].abs_y != -1) {
        if (tp_event_data[slot].is_startpoint) {
            tp_event_data[slot].press_x = tp_event_data[slot].abs_x;
            tp_event_data[slot].press_y = tp_event_data[slot].abs_y;
            tp_event_data[slot].touch_state = TOUCH_DOWN;
            tp_event_data[slot].is_startpoint = 0;
            touchDown(slot, tp_event_data[slot].press_time, tp_event_data[slot].press_x, tp_event_data[slot].press_y);
            if (touchDownCb)
                touchDownCb(slot, tp_event_data[slot].press_time, tp_event_data[slot].press_x, tp_event_data[slot].press_y);
            //ALOGE("Touch Down:slot:%d, x:%04d,y::%04d, Time:%ld", slot, tp_event_data[slot].press_x, tp_event_data[slot].press_y, tp_event_data[slot].press_time);
        } else if (tp_event_data[slot].touch_state == TOUCH_UP) {
            //tp_event_data[slot].abs_x = -1;
            //tp_event_data[slot].abs_y = -1;
            touchUp(slot, tp_event_data[slot].release_time, tp_event_data[slot].release_x, tp_event_data[slot].release_y);
            if (touchUpCb)
                touchUpCb(slot, tp_event_data[slot].release_time, tp_event_data[slot].release_x, tp_event_data[slot].release_y);
            if (!slot_num)
                touchRelease();
            //ALOGE("Touch   Up:slot:%d, x:%04d,y::%04d, Time:%ld", slot, tp_event_data[slot].release_x, tp_event_data[slot].release_y, tp_event_data[slot].release_time);
        } else if (tp_event_data[slot].touch_state == TOUCH_DOWN || tp_event_data[slot].touch_state == TOUCH_MOVE) {
            tp_event_data[slot].touch_state = TOUCH_MOVE;
            tp_event_data[slot].move_x = tp_event_data[slot].abs_x;
            tp_event_data[slot].move_y = tp_event_data[slot].abs_y;
            tp_event_data[slot].move_time = event->time.tv_sec * UMECS_PER_SEC + event->time.tv_usec;
            touchMove(slot, tp_event_data[slot].move_time, tp_event_data[slot].move_x, tp_event_data[slot].move_y);
            if (touchMoveCb)
                touchMoveCb(slot, tp_event_data[slot].move_time, tp_event_data[slot].move_x, tp_event_data[slot].move_y);
            //ALOGE("Touch Move:slot:%d, x:%04d,y::%04d, Time:%ld", slot, tp_event_data[slot].move_x, tp_event_data[slot].move_y, tp_event_data[slot].move_time);
        }
    } else if (event->type == EV_KEY) {
        touchKey(event->time.tv_sec * UMECS_PER_SEC + event->time.tv_usec, event->code, event->value);
    }
}

void TouchDevice::geteventPen(struct input_event *event) {
    int slot = -1;
    if (event->type == EV_ABS) {
        if (event->code == ABS_X) {
            pen_event_data.abs_x = event->value;
        } else if (event->code == ABS_Y) {
            pen_event_data.abs_y = event->value;
        }
    } else if (event->type == EV_SYN) {
        if (event->code == SYN_REPORT &&
            pen_event_data.abs_x != 0 && pen_event_data.abs_y != 0) {
            if (pen_event_data.is_startpoint) {
                pen_event_data.press_x = pen_event_data.abs_x;
                pen_event_data.press_y = pen_event_data.abs_y;
                pen_event_data.touch_state = TOUCH_DOWN;
                pen_event_data.is_startpoint = 0;

                penDown(pen_event_data.press_time, pen_event_data.press_x, pen_event_data.press_y);
                //ALOGE("Pen Down:slot:%d, x:%04d,y::%04d, Time:%ld", 0, pen_event_data.press_x, pen_event_data.press_y, pen_event_data.press_time);
            } else if (pen_event_data.touch_state == TOUCH_DOWN || pen_event_data.touch_state == TOUCH_MOVE) {
                pen_event_data.move_time = event->time.tv_sec * UMECS_PER_SEC + event->time.tv_usec;
                pen_event_data.move_x = pen_event_data.abs_x;
                pen_event_data.move_y = pen_event_data.abs_y;
                pen_event_data.touch_state = TOUCH_MOVE;

                penMove(pen_event_data.move_time, pen_event_data.move_x, pen_event_data.move_y);
                //ALOGE("Pen Move:slot:%d, x:%04d,y::%04d, Time:%ld", 0, pen_event_data.move_x, pen_event_data.move_y, pen_event_data.move_time);
            } else if (pen_event_data.touch_state == TOUCH_UP) {
                penUp(pen_event_data.release_time, pen_event_data.release_x, pen_event_data.release_y);
                //ALOGE("Pen Up:slot:%d, x:%04d,y::%04d, Time:%ld", 0, pen_event_data.release_x, pen_event_data.release_y, pen_event_data.release_time);
            }
        }
    } else if (event->type == EV_KEY) {
        if (event->code == BTN_DIGI) {
            if (event->value == 1) {
                pen_event_data.press_time = event->time.tv_sec * UMECS_PER_SEC + event->time.tv_usec;
                pen_event_data.is_startpoint = 1;
                //ALOGD("Pen Key: BTN_DIGI DOWN");
            } else if (event->value == 0) {
                pen_event_data.release_time = event->time.tv_sec * UMECS_PER_SEC + event->time.tv_usec;
                pen_event_data.release_x = pen_event_data.abs_x;
                pen_event_data.release_y = pen_event_data.abs_y;
                pen_event_data.touch_state = TOUCH_UP;
                //ALOGD("Pen Key: BTN_DIGI UP");
            }
        }
    }
}

void TouchDevice::geteventKeyPower(struct input_event *event) {
    if (event->type == EV_KEY && event->code == KEY_POWER) {
        touchKey(event->time.tv_sec * UMECS_PER_SEC + event->time.tv_usec, event->code, event->value);
    }
}

bool TouchDevice::threadLoop()
{
    struct pollfd ufds[INPUT_DEVICE_NUM * PER_DEVICE_NUM + 3];
    int nfds = sizeof(ufds)/sizeof(struct pollfd);
    struct input_event event;
    struct abnormal_event abnormal_event;
    int thp_cmd_buf[MAX_BUF_SIZE] = { 0 };
    int res = 0;
    char suspend_state = 0;

    memset(&ufds, 0x00, sizeof(ufds));
    memset(&event, 0x00, sizeof(event));
    memset(thp_cmd_buf, 0x00, MAX_BUF_SIZE);

    for (int i = 0; i < INPUT_DEVICE_NUM; ++i) {
        for (int j = 0; j < PER_DEVICE_NUM; ++j) {
            if (input_fd[i][j] > 0) {
                ufds[i * PER_DEVICE_NUM + j].fd = input_fd[i][j];
                ufds[i * PER_DEVICE_NUM + j].events = POLLIN;
                res = 1;
            } else {
                ufds[i * PER_DEVICE_NUM + j].fd = -1;
            }
        }
    }
    if (res == 0) {
        ALOGE("Invalid touch_fd\n");
        return false;
    }
    int suspend_state_fd = open(SUSPEND_STATE_PATH, O_RDONLY);
    if (suspend_state_fd < 0) {
        ALOGE("Failed to open %s, error=%s\n", SUSPEND_STATE_PATH, strerror(errno));
    } else {
        ALOGI("Success to open %s, fd=%d\n", SUSPEND_STATE_PATH, suspend_state_fd);
        ufds[INPUT_DEVICE_NUM * PER_DEVICE_NUM].fd = suspend_state_fd;
        ufds[INPUT_DEVICE_NUM * PER_DEVICE_NUM].events = POLLPRI;
        pread(suspend_state_fd, &suspend_state, 1, 0);
    }
    int thp_cmd_fd = open(TOUCH_THP_CMD_PATH, O_RDONLY);
    if (thp_cmd_fd < 0) {
        ALOGE("Failed to open %s, error=%s\n", TOUCH_THP_CMD_PATH, strerror(errno));
    } else {
        ALOGI("Success to open %s, fd=%d\n", TOUCH_THP_CMD_PATH, thp_cmd_fd);
        ufds[INPUT_DEVICE_NUM * PER_DEVICE_NUM + 1].fd = thp_cmd_fd;
        ufds[INPUT_DEVICE_NUM * PER_DEVICE_NUM + 1].events = POLLPRI;
        read(thp_cmd_fd, thp_cmd_buf, MAX_BUF_SIZE);
    }
    int abnormal_event_fd = open(ABNORMAL_EVENT_PATH, O_RDONLY);
    if (abnormal_event_fd < 0) {
        ALOGE("Failed to open %s, error=%s\n", ABNORMAL_EVENT_PATH, strerror(errno));
    } else {
        ALOGI("Success to open %s, fd=%d\n", ABNORMAL_EVENT_PATH, abnormal_event_fd);
        ufds[INPUT_DEVICE_NUM * PER_DEVICE_NUM + 2].fd = abnormal_event_fd;
        ufds[INPUT_DEVICE_NUM * PER_DEVICE_NUM + 2].events = POLLPRI;
        pread(abnormal_event_fd, &abnormal_event, sizeof(abnormal_event), 0);
    }

    for (int i = 0; i < MAX_SLOT; i++) {
        tp_event_data[i].abs_x = -1;
        tp_event_data[i].abs_y = -1;
    }
    memset(&abnormal_event, 0, sizeof(abnormal_event));

    ALOGI("TouchDevice threadLoop\n");
    while(1) {
        poll(ufds, nfds, -1);
        for (int j = 0; j < PER_DEVICE_NUM; ++j) {
            if(ufds[TOUCH_DEVICE * PER_DEVICE_NUM + j].revents & POLLIN) {
                res = read(ufds[TOUCH_DEVICE * PER_DEVICE_NUM + j].fd, &event, sizeof(event));
                if (res < (int)sizeof(event)) {
                    ALOGE("could not get event\n");
                    continue;
                }
                geteventTouch(&event);
            }
        }
        for (int j = 0; j < PER_DEVICE_NUM; ++j) {
            if(ufds[PEN_DEVICE * PER_DEVICE_NUM + j].revents & POLLIN) {
                res = read(ufds[PEN_DEVICE * PER_DEVICE_NUM + j].fd, &event, sizeof(event));
                if (res < (int)sizeof(event)) {
                    ALOGE("could not get event\n");
                    continue;
                }
                geteventPen(&event);
            }
        }
        for (int j = 0; j < PER_DEVICE_NUM; ++j) {
            if(ufds[KEY_POWER_DEVICE * PER_DEVICE_NUM + j].revents & POLLIN) {
                res = read(ufds[KEY_POWER_DEVICE * PER_DEVICE_NUM + j].fd, &event, sizeof(event));
                if (res < (int)sizeof(event)) {
                    ALOGE("could not get event\n");
                    continue;
                }
                geteventKeyPower(&event);
            }
        }

        if (suspend_state_fd > 0 && (ufds[INPUT_DEVICE_NUM * PER_DEVICE_NUM].revents & POLLPRI)) {
            res = pread(suspend_state_fd, &suspend_state, 1, 0);
            if (res < 0) {
                ALOGE("Failed to read, error=%s", strerror(errno));
                continue;
            }
            LOGD("suspend_state=%d", suspend_state - '0');
            touchSuspendState(suspend_state - '0');
        }
        if (thp_cmd_fd > 0 && (ufds[INPUT_DEVICE_NUM * PER_DEVICE_NUM + 1].revents & POLLPRI)) {
            pread(thp_cmd_fd, thp_cmd_buf, MAX_BUF_SIZE, 0);
            touchCmd(thp_cmd_buf);
        }
        if (abnormal_event_fd > 0 && (ufds[INPUT_DEVICE_NUM * PER_DEVICE_NUM + 2].revents & POLLPRI)) {
            pread(abnormal_event_fd, &abnormal_event, sizeof(abnormal_event), 0);
            abnormalEvent(&abnormal_event);
        }
    }
    close(suspend_state_fd);
    close(thp_cmd_fd);
    close(abnormal_event_fd);
}

void TouchDevice::threadRun()
{
    this->run("touchevent-dump-thread", PRIORITY_BACKGROUND);
}

void TouchDevice::set_touchfunc_call(TOUCH_FUNC td, TOUCH_FUNC tm, TOUCH_FUNC tu)
{
    if (td)
        touchDownCb = td;
    if (tm)
        touchMoveCb = tm;
    if (tu)
        touchUpCb = tu;
}
