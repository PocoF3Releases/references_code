/*
 * Copyright (C) 2013 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * author: Wenqiang zhang 2022/01/04
 */
#ifndef __TOUCH_SENSOR_H__
#define __TOUCH_SENSOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <vector>
#include <sys/mman.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>
#include <getopt.h>
#include <pthread.h>
#include <dirent.h>
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
#include <time.h>
#include <sys/time.h>
#include <cutils/properties.h>
#include <utils/threads.h>
#include <utils/Log.h>
#include <utils/String8.h>
#include <log/log.h>

#include  <tensorflow/lite/interpreter.h>
#include  <tensorflow/lite/kernels/register.h>
#include  <tensorflow/lite/model.h>
#include  <tensorflow/lite/optional_debug_tools.h>

#undef  LOG_TAG
#define LOG_TAG                    "TouchFilmSensor"

#define GLOVE_CHECK_TIME 3
#define GLOVE_CHECK_PERD 3
#define GLOVE_MAX_VALUE  450

#define FILM_RECHECK_TIME_GAP 86400.0
#define FILM_CHECK_CNT 5
#define FILM_CHECK_ROUND 5
#define QUIT_FILM_CHECK 4
#define FILM_KIND 3
#define FILM_QUIR_CHECK_CNT 5

/********************************** PLATFORM RELATED ************************************/
#define L2_RX 18
#define L2_TX 40
#define RX L2_RX
#define TX L2_TX

/********************************** DEBUG OPERATION *************************************
 * FRAME_DEBUG:  Which decided to display the model's input frame or not in logcat.
 * DATA_COLLECT: To Record the touch frame.
 * MODEL_HANDLE: To Invoke the model.
 ***************************************************************************************/
//#define DATA_COLLECT
#define MODEL_HANDLE

//#define FRAME_DEBUG
#define TOUCH_SENSOR_LOG_DEBUG  0

/********************************** SAMPLE SETUP ****************************************
 * SAMPLE_NUM:         which must greater than or equal to GLOVE_END_FRAME.
 * GLOVE_START_FRAME:  which represent the Glove model's start frame of input, must smaller than or equal to GLOVE_END_FRAME
 * GLOVE_END_FRAME:    which represent the Glove model's end frame of input.
 *****************************************************************************************/
#define SAMPLE_NUM        5
#define GLOVE_START_FRAME 2
#define GLOVE_END_FRAME   3

#define FILM_START_FRAME  4
#define FILM_END_FRAME  5
/************************************* GLOBAL PARAMS *************************************/
using namespace tflite;
int raw_buf[RX * TX * SAMPLE_NUM];
char data_vaild_bit;

/*********************************** DEBUG MACRO START ***********************************/
#ifdef FRAME_DEBUG
static inline void show_raw(int *data, int rx, int tx, bool show_label)
{
    int i, j;
    std::string str = "";

    for (i = 0; i < rx; i++) {
        string str = "";
        for (j = 0; j < tx; j++) {
            char arr[20] = {0};
            snprintf(arr, 20, "%5d,", data[i * tx + j]);
            str += std::string(arr);
        }
        const char* data = str.data();
        ALOGI("%s", data);
    }
}
#else
static inline void show_raw(int *data, int rx, int tx, bool show_label)
{
    ;
}
#endif

#define TOUCH_DEBUG(fmt, args...) do {                              \
    if (TOUCH_SENSOR_LOG_DEBUG) {                                   \
       __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args); \
    }                                                               \
} while (0)

#define TFLITE_MINIMAL_CHECK(x)                                     \
if (!(x)) {                                                         \
    ALOGE("Error at %s:%d", __FILE__, __LINE__);                    \
    return -1;                                                      \
}
/*********************************** DEBUG MACRO END **********************************/
#define SUSPEND_STATE              "/sys/class/touch/touch_dev/suspend_state"
#define TOUCH_FINGER_STATUS_PATH   "/sys/class/touch/touch_dev/touch_finger_status"
#define TOUCH_MEM_NOTIFY_PATCH     "/sys/class/touch/touch_dev/touch_thp_mem_notify"
#define FILM_CMD_PATH              "/sys/class/touch/touch_dev/touch_sensor"
#define FILM_DATA_PATH             "/dev/xiaomi-touch"
#define DEVICE_NODE_NUM (sizeof(param) / sizeof(char *))
const char *param[] = {
    FILM_DATA_PATH,
    SUSPEND_STATE,
    TOUCH_FINGER_STATUS_PATH,
    TOUCH_MEM_NOTIFY_PATCH,
    FILM_CMD_PATH,
};

#define MODEL_PATH_GLOVE           "/odm/firmware/glove_model.tflite"
#define MODEL_PATH_FILM            "/odm/firmware/film_model.tflite"
#define MODEL_NUM (int)(sizeof(model_name) / sizeof(char *))
const char *model_name[] = {
    "model_glove",
    "model_film",
};

const char *model_path[] = {
    MODEL_PATH_GLOVE,
    MODEL_PATH_FILM,
};

enum FILE_CMD {
    WRITE_GLOVE_ENTER = 0,
    WRITE_GLOVE_SUMMARY = 1,
};

class FileOperation {
public:
    int mfdTouchStatus;
    int mfdSuspend;
    int mfdMemNotify;
    int mfdFilmData;
    int mfdListen;
    void *mdataBaseVa;

    int check_device_node_ready(const char *node_path[])
    {
        int cnt = 0;
        int fd;
        bool is_ok;
        struct pollfd ufds;

        while (1) {
            is_ok = true;
            for (size_t i = 0; i < DEVICE_NODE_NUM; i++) {
                if (access(node_path[i], R_OK) != -1) {
                } else {
                    is_ok = false;
                }
            }
            if (is_ok)
                break;
            // 100s
            usleep(100000);
            cnt++;
            if (cnt >= 1000)
                break;
        }

        if (cnt >= 1000) {
            ALOGE("can't find /sys/class/touch/touch_dev/*\n");
            return -1;
        }

        return 0;
    }

    void *getGlobalValue(void) {
        return mdataBaseVa;
    }

    int touchsensor_fop_init(void) {
        mfdTouchStatus = open(TOUCH_FINGER_STATUS_PATH, O_RDONLY);
        if (mfdTouchStatus < 0) {
            ALOGE("Can't open fd_touch_status: %d", mfdTouchStatus);
            return -1;
        }

        mfdSuspend = open(SUSPEND_STATE, O_RDONLY);
        if (mfdSuspend < 0) {
            ALOGE("Can't open fd_suspend_notify: %d", mfdSuspend);
            return -1;
        }

        mfdMemNotify = open(TOUCH_MEM_NOTIFY_PATCH, O_RDONLY);
        if (mfdMemNotify < 0) {
            ALOGE("Can't open fd_mem_notify: %d", mfdMemNotify);
            return -1;
        }

        mfdFilmData = open(FILM_DATA_PATH, O_RDONLY);
        if (mfdFilmData < 0) {
            ALOGE("Can't open fd_film_data: %d", mfdFilmData);
            return -1;
        }

        mdataBaseVa = mmap(0, RX * TX * sizeof(int),  PROT_READ, MAP_SHARED, mfdFilmData, 4096);
        if (mdataBaseVa == MAP_FAILED) {
            ALOGE("MAP film_data_base failed!");
            return -1;
        }

        return 0;
    }

    void fdSetup(struct pollfd *fds) {
        char temp_buf[1];

        fds[0].fd = mfdMemNotify;
        fds[0].events = POLLPRI | POLLERR;
        fds[1].fd = mfdTouchStatus;
        fds[1].events = POLLPRI | POLLERR;
        fds[2].fd = mfdSuspend;
        fds[2].events = POLLPRI | POLLERR;
        pread(fds[0].fd, temp_buf, 1, 0);
        pread(fds[1].fd, temp_buf, 1, 0);
        pread(fds[2].fd, temp_buf, 1, 0);
        //fd: listen num
        mfdListen = 3;
    }
};

// Time Handle
class TimeHandle {
public:
    struct rtctime {
        int tm_sec;
        int tm_min;
        int tm_hour;
        int tm_mday;
        int tm_mon;
        int tm_year;
        int tm_wday;
        int tm_yday;
        int tm_isdst;
    };

    constexpr static const unsigned char rtc_days_in_month[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    constexpr static const unsigned short rtc_ydays[2][13] = {
        // Normal years
        { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
        // Leap years
        { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
    };

    #define LEAPS_THRU_END_OF(y) ((y) / 4 - (y) / 100 + (y) / 400)
    inline bool is_leap_year(unsigned int year) {
        return (!(year % 4) && (year % 100)) || !(year % 400);
    }

    int rtc_month_days(unsigned int month, unsigned int year) {
        return rtc_days_in_month[month] + (is_leap_year(year) && month == 1);
    }

    /*
    * The number of days since January 1. (0 to 365)
    */
    int rtc_year_days(unsigned int day, unsigned int month, unsigned int year) {
        return rtc_ydays[is_leap_year(year)][month] + day - 1;
    }

    /*
    * rtc_time64_to_tm - Converts time64_t to rtc_time.
    * Convert seconds since 01-01-1970 00:00:00 to Gregorian date.
    */
    void rtc_time_to_tm(long time, struct rtctime *tm) {
        unsigned int month, year, secs;
        int days;

        // Time Must Be Positive
        days = time / 86400;
        secs = time % 86400;

        // Day Of The Week, 1970-01-01 Was A Thursday
        tm->tm_wday = (days + 4) % 7;

        year = 1970 + days / 365;
        days -= (year - 1970) * 365
                + LEAPS_THRU_END_OF(year - 1)
                - LEAPS_THRU_END_OF(1970 - 1);
        while (days < 0) {
            year -= 1;
            days += 365 + is_leap_year(year);
        }
        tm->tm_year = year - 1900;
        tm->tm_yday = days + 1;

        for (month = 0; month < 11; month++) {
            int newdays;

            newdays = days - rtc_month_days(month, year);
            if (newdays < 0)
                break;
            days = newdays;
        }
        tm->tm_mon = month;
        tm->tm_mday = days + 1;

        tm->tm_hour = secs / 3600;
        secs -= tm->tm_hour * 3600;
        tm->tm_min = secs / 60;
        tm->tm_sec = secs - tm->tm_min * 60;

        tm->tm_isdst = 0;
    }

    void get_current_time_str(string &timebuf) {
        struct rtctime tm;
        struct timeval time1;
        char buf[100];
        gettimeofday(&time1, NULL);
        rtc_time_to_tm(time1.tv_sec, &tm);

        snprintf(buf, 50, "%02d-%02d_%02d-%02d-%02d.csv",
                tm.tm_mon + 1, tm.tm_mday,
                tm.tm_hour, tm.tm_min, tm.tm_sec);

        timebuf = std::string(buf);
    }

    double tick(void) {
        struct timeval t;
        gettimeofday(&t, 0);
        return t.tv_sec + 1E-6 * t.tv_usec;
    }
};

// THP COMM
class ThpComm {
public:
    enum FRAME_OP {
        STOP_RECORD_FRAME = 0,
        START_RECORD_FRAME = 2,
    };

    int write_val(char const *path, int val) {
        char str[20] = {0,};
        int fd;
        int cnt;

        snprintf(str, 10, "%d", val);
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            ALOGE("%s can't open:%s\n", __func__, path);
            return -1;
        }
        cnt = write(fd, str, strlen(str));
        close(fd);
        if (cnt <= 0)
            return 0;
        return cnt;
    }

    int start_film_sample(int sample_num) {
        ALOGI("Start Sample!\n");
        return write_val(FILM_CMD_PATH, sample_num);
    }

    int write_film_result(int val) {
        TOUCH_DEBUG("%s val = %d!\n", __func__, val);
        return write_val(FILM_CMD_PATH, val + 100);
    }

    int write_glove_result(int val) {
        TOUCH_DEBUG("%s val = %d!\n", __func__, val);
        return write_val(FILM_CMD_PATH, val + 200);
    }

    int stop_film_sample() {
        ALOGI("Stop Sample!\n");
        return write_val(FILM_CMD_PATH, 0);
    }
};

//Data pre-handle
class DataHandle {
public:
    int frame_cnt;

    enum FRAME_PARAMS {
        FRAME_NO1 = 1,
        FRAME_NO2,
        FRAME_NO3,
        FRAME_NO4,
        FRAME_NO5,
        FRAME_NO6,
    };

    DataHandle() {
        frame_cnt = 0;
    }

    bool DataCheck(int *data) {
        for (int i = 0; i < RX; i++) {
            for (int j = 0; j < TX; j++) {
                if(data[i * TX + j] > GLOVE_MAX_VALUE) {
                    return false;
                }
            }
        }
        return true;
    }

    void inline resetFrameCount() {
        TOUCH_DEBUG("++++++++++++++++++ Touch up Frame Cnt:%d ++++++++++++++++++++\n", frame_cnt);
        frame_cnt = 0;
    }

    void inline addFrameCount() {
        frame_cnt++;
    }

    int inline getFrameCount() {
        return frame_cnt;
    }

   int FrameRecord(void *raw_data_va) {
        if (frame_cnt > SAMPLE_NUM) {
            return 0;
        }

        switch (frame_cnt) {
        case FRAME_NO1:
            memcpy((char *)raw_buf, raw_data_va, TX * RX * sizeof(int));
            show_raw(raw_buf, RX, TX, true);
            data_vaild_bit &= 0x00;
            if (this->DataCheck(raw_buf)) {
                data_vaild_bit |= (1 << FRAME_NO1);
                TOUCH_DEBUG("data_vaild_bit1: 0x%.2x!\n", data_vaild_bit);
            }
            return FRAME_NO1;
            break;
        case FRAME_NO2:
            memcpy((char *)raw_buf + TX * RX * sizeof(int), raw_data_va, TX * RX * sizeof(int));
            show_raw(raw_buf + TX * RX, RX, TX, true);
            if (this->DataCheck(raw_buf + TX * RX)) {
                data_vaild_bit |= (1 << FRAME_NO2);
                TOUCH_DEBUG("data_vaild_bit2: 0x%.2x!\n", data_vaild_bit);
            }
            return FRAME_NO2;
            break;
        case FRAME_NO3:
            memcpy((char *)raw_buf + TX * RX * sizeof(int) * 2, raw_data_va, TX * RX * sizeof(int));
            show_raw(raw_buf + TX * RX * 2, RX, TX, true);
            if (this->DataCheck(raw_buf + TX * RX * 2)) {
                data_vaild_bit |= (1 << FRAME_NO3);
                TOUCH_DEBUG("data_vaild_bit3: 0x%.2x!\n", data_vaild_bit);
            }
            return FRAME_NO3;
            break;
        case FRAME_NO4:
            memcpy((char *)raw_buf + TX * RX * sizeof(int) * 3, raw_data_va, TX * RX * sizeof(int));
            show_raw(raw_buf +  TX * RX * 3, RX, TX, true);
            if (this->DataCheck(raw_buf + TX * RX * 3)) {
                data_vaild_bit |= (1 << FRAME_NO4);
                TOUCH_DEBUG("data_vaild_bit4: 0x%.2x!\n", data_vaild_bit);
            }
            return FRAME_NO4;
            break;
        case FRAME_NO5:
            memcpy((char *)raw_buf + TX * RX * sizeof(int) * 4, raw_data_va, TX * RX * sizeof(int));
            show_raw(raw_buf +  TX * RX * 4, RX, TX, true);
            if (this->DataCheck(raw_buf + TX * RX * 4)) {
                data_vaild_bit |= (1 << FRAME_NO5);
                TOUCH_DEBUG("data_vaild_bit5: 0x%.2x!\n", data_vaild_bit);
            }
            return FRAME_NO5;
            break;
        case FRAME_NO6:
            memcpy((char *)raw_buf + TX * RX * sizeof(int) * 5, raw_data_va, TX * RX * sizeof(int));
            show_raw(raw_buf +  TX * RX * 5, RX, TX, true);
            if (this->DataCheck(raw_buf + TX * RX * 5)) {
                data_vaild_bit |= (1 << FRAME_NO6);
                TOUCH_DEBUG("data_vaild_bit6: 0x%.2x!\n", data_vaild_bit);
            }
            return FRAME_NO6;
            break;
        }
        return 0;
    }
};

class TouchTFModel {
private:
    const char *tfmodelpath;
    int model_mode;

public:
    float *mInput;
    float *mOut;
    std::unique_ptr<tflite::FlatBufferModel> mModel;
    tflite::ops::builtin::BuiltinOpResolver mResolver;
    std::unique_ptr<Interpreter> mInterpreter;
    TimeHandle *timehandle;
    static int film_mode;
    static bool stop_glove_invoke;

    enum MODEL_STATE {
        BARE_MODE = 0,
        SLIM_MODE,
        THICK_MODE,
        GLOVE_MODE = 3,
    };

    TouchTFModel() {
        model_mode = BARE_MODE;
        timehandle = new TimeHandle;
    }

    virtual ~TouchTFModel() {
        delete(timehandle);
        ALOGI("~TouchTFModel!");
    }

    void SetTfModelName(const char* ModelPath) {
        tfmodelpath = ModelPath;
    }

    inline void input_fill(int *raw_data, int frame_start, int frame_end) {
        for (int i = frame_start; i <= frame_end; i++) {
            show_raw(raw_data + TX * RX * (i - 1), RX, TX, true);
        }

        for(int i = 0; i < TX * RX * (frame_end - frame_start + 1); i++) {
            mInput[i] = raw_data[i + (TX * RX * (frame_start - 1))];
        }
    }

    void setModelMode(int value) {
        switch (value) {
        case BARE_MODE:
            ALOGI("Normal Mode!\n");
            model_mode = value;
            film_mode = value;
            break;
        case SLIM_MODE:
            ALOGI("Slim Mode!\n");
            model_mode = BARE_MODE;
            film_mode = BARE_MODE;
            break;
        case THICK_MODE:
            ALOGI("Film Mode!\n");
            model_mode = value;
            film_mode = value;
            break;
        case GLOVE_MODE:
            ALOGI("Glove Mode!\n");
            model_mode = value;
            break;
        default:
            break;
        }
    }

    int getModelMode() {
        return model_mode;
    }

    int TfModelInit() {
        mModel = tflite::FlatBufferModel::BuildFromFile(tfmodelpath);
        TFLITE_MINIMAL_CHECK(mModel != nullptr);
        InterpreterBuilder builder(*mModel, mResolver);
        builder(&mInterpreter);
        TFLITE_MINIMAL_CHECK(mInterpreter != nullptr);
        TFLITE_MINIMAL_CHECK(mInterpreter->AllocateTensors() == kTfLiteOk);
        //tflite::PrintInterpreterState(mInterpreter.get());
        TFLITE_MINIMAL_CHECK(mInterpreter->Invoke() == kTfLiteOk);
        return 0;
    }

    virtual void Init() = 0;
    virtual bool getSampleStatus() = 0;
    virtual bool ModelNeedComm() = 0;
    virtual void clearModelComm() = 0;
    virtual void Handle(int frame_num) = 0;
};

int TouchTFModel::film_mode = BARE_MODE;
bool TouchTFModel::stop_glove_invoke = false;

class TouchGlove : public TouchTFModel {
public:
    std::vector<char> glove_result;
    int mgloveCheckCnt;
    int mgloveCheckSum;
    bool mgloveStop;
    bool mgloveNeedComm;

    TouchGlove() {
       this->Init();
    }
    virtual ~TouchGlove() {
        ALOGI("~TouchGlove!");
    }

    void Init() {
        mgloveStop = false;
        mgloveNeedComm = false;
        mgloveCheckCnt = 0;
        glove_result.clear();
    }

    bool getSampleStatus() {
        return mgloveStop;
    }

    bool ModelNeedComm() {
        return mgloveNeedComm;
    }

    void clearModelComm() {
        mgloveNeedComm = false;
    }

    void TfModelInvoke(int *raw_data, int frame_start, int frame_end) {
        mInput = mInterpreter->typed_input_tensor<float>(0);
        mOut = mInterpreter->typed_output_tensor<float>(0);
        input_fill(raw_data, frame_start, frame_end);
        mInterpreter->Invoke();
    }

    void Handle(int frame_num);
};

class TouchFilm : public TouchTFModel {
public:
    double lastCheckTime;
    bool clear_result;
    int mfilmCheckCnt;
    int mfilmCheckRound;
    std::vector<int> filmResult;
    std::vector<int> finalResult;
    bool mfilmStop;
    bool mfilmNeedComm;
    int mfilmQuitCheck;
    int filmQuitResult;
    bool flag_film_quit;

    TouchFilm() {
        lastCheckTime = timehandle->tick();
        mfilmNeedComm = false;
        flag_film_quit = false;
        mfilmStop = false;
        clear_result = true;
        mfilmCheckRound = 0;
        mfilmCheckCnt = 0;
        mfilmQuitCheck = 0;
        filmQuitResult = 0;
        filmResult.clear();
        finalResult.clear();
        for (int i = 0; i < FILM_KIND; i++) {
            filmResult.push_back(0);
            finalResult.push_back(0);
        }
        ALOGI("FilmMode: TOUCH_FILM %fns\n", lastCheckTime);
    }

    virtual ~TouchFilm() {
        ALOGI("~TouchFilm!");
    }

    bool getSampleStatus() {
        return mfilmStop;
    }

    bool ModelNeedComm() {
        return mfilmNeedComm;
    }

    void clearModelComm() {
        mfilmNeedComm = false;
    }

    void Init() {
        mfilmCheckCnt = 0;
        mfilmQuitCheck = 0;
        filmQuitResult = 0;
        mfilmStop = false;
        filmResult.clear();
        for (int i = 0; i < FILM_KIND; i++) {
            filmResult.push_back(0);
        }

        if(timehandle->tick() - lastCheckTime >= FILM_RECHECK_TIME_GAP) {
            ALOGI("Recheck every day %f", timehandle->tick() - lastCheckTime);
            mfilmCheckRound = 0;
            flag_film_quit = false;
            clear_result = true;
            lastCheckTime = timehandle->tick();
        }
        if (clear_result) {
            for (int i = 0; i < FILM_KIND; i++)
                finalResult[i] = 0;
            clear_result = false;
        }
    }

    void TfModelInvoke(int *raw_data, int frame_start, int frame_end) {
        mInput = mInterpreter->typed_input_tensor<float>(0);
        mOut = mInterpreter->typed_output_tensor<float>(0);
        input_fill(raw_data, frame_start, frame_end);
        mInterpreter->Invoke();
    }

    void Handle(int frame_num);
};

class TouchSensor {
public:
    TouchTFModel *mModel[MODEL_NUM];
    std::vector<int> mMode_rcd;
    ThpComm *thpcomm;
    FileOperation *fileop;
    DataHandle *datahandle;

    TouchSensor() {
        mMode_rcd.clear();
        for (int i = 0; i < MODEL_NUM; i++) {
            if (this->ModelInit(mModel[i], i)) {
                ALOGE("[%s] Init failed!\n", model_name[i]);
            }
        }
        thpcomm = new ThpComm;
        fileop = new FileOperation;
        datahandle = new DataHandle;
    }

    ~TouchSensor() {
        for (int i = 0; i < MODEL_NUM; i++) {
            delete(mModel[i]);
        }
        delete(thpcomm);
        delete(fileop);
        delete(datahandle);
    }

    int ModelInit(TouchTFModel* &Model, int num) {
        Model = nullptr;
        if (access(model_path[num], R_OK) == -1) {
            TOUCH_DEBUG("[%s] Model_path err!", model_path[num]);
            return -1;
        }

        Model = this->ModelCreate(model_name[num]);
        if (Model == nullptr) {
            TOUCH_DEBUG("[%s] ModelCreate failed!", model_name[num]);
            return -1;
        }

        Model->SetTfModelName(model_path[num]);
        if(Model->TfModelInit() < 0) {
            TOUCH_DEBUG("[%s] TfModelInit failed!", model_name[num]);
            Model = nullptr;
            return -1;
        }

        mMode_rcd.push_back(num);
        ALOGI("[%s] Init success!", model_name[num]);
        return 0;
    }

    TouchTFModel *ModelCreate(const char *ModelName) {
        TouchTFModel *Model = nullptr;
        if (strncmp(ModelName, "model_glove", sizeof("model_glove")) == 0) {
            Model = new TouchGlove;
            return Model;
        } else if (strncmp(ModelName, "model_film", sizeof("model_film")) == 0) {
            Model = new TouchFilm;
            return Model;
        }
        return nullptr;
    }
};

#endif
