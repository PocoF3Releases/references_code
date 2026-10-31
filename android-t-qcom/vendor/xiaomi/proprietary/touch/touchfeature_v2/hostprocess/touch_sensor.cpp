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
 */

#include "touch_sensor.h"

/******************************* Add Model Handle Func *************************************/
#ifdef MODEL_HANDLE
void TouchFilm::Handle(int frame_num) {
    if (mfilmStop || !stop_glove_invoke || flag_film_quit) {
        TOUCH_DEBUG("FilmMode:Skip invoke mfilmStop: %d, glove_status: %d， flag_film_quit %d\n", mfilmStop, !stop_glove_invoke, flag_film_quit);
        return;
    }

    if (frame_num == FILM_END_FRAME && mfilmCheckCnt < FILM_CHECK_CNT && mfilmCheckRound < FILM_CHECK_ROUND) {
        mfilmCheckCnt++;
        this->TfModelInvoke(raw_buf, FILM_START_FRAME, FILM_END_FRAME);
        TOUCH_DEBUG("[Film Invoke] Bare:%f, Slim:%f, Thick:%f\n", this->mOut[0], this->mOut[1], this->mOut[2]);

        for(int i = 0; i < FILM_KIND; i++)
            filmResult[i] += this->mOut[i] > 0.5;
        TOUCH_DEBUG("FilmMode[Count] Bare:%d, Slim:%d, Thick:%d\n", filmResult[0], filmResult[1], filmResult[2]);
    }

    if (film_mode == THICK_MODE && frame_num == FILM_END_FRAME && mfilmQuitCheck < FILM_QUIR_CHECK_CNT && !flag_film_quit) {
        mfilmQuitCheck++;
        this->TfModelInvoke(raw_buf, FILM_START_FRAME, FILM_END_FRAME);
        TOUCH_DEBUG("FilmMode[Film Invoke] Bare:%f, Slim:%f, Thick:%f\n", this->mOut[0], this->mOut[1], this->mOut[2]);

        for(int i = 0; i < FILM_KIND; i++)
            filmQuitResult += ((i - 1) * this->mOut[i] > 0.5);
        TOUCH_DEBUG("FilmMode[Quit]: %d\n", filmQuitResult);
    }

    if (mfilmCheckCnt >= FILM_CHECK_CNT && mfilmCheckRound < FILM_CHECK_ROUND) {
        for (int i = 0; i < FILM_KIND; i++)
            finalResult[i] += filmResult[i];
        mfilmCheckRound++;
        ALOGI("FilmMode[Result] Bare:%d, Slim:%d, Thick:%d\n", finalResult[0], finalResult[1], finalResult[2]);
        mfilmStop = true;
    }

    if (mfilmCheckRound >= FILM_CHECK_ROUND && film_mode != THICK_MODE && !flag_film_quit) {
        int tmp_film_mode = std::distance(std::begin(finalResult), std::max_element(std::begin(finalResult),
                                                        std::end(finalResult)));
        ALOGI("Change to film mode: %d", tmp_film_mode);
        mfilmNeedComm = true;
        this->setModelMode(tmp_film_mode);

        if (tmp_film_mode != THICK_MODE) {
            mfilmStop = true;
            flag_film_quit = true;
        } else
            flag_film_quit = false;
    }

    if (film_mode == THICK_MODE && mfilmQuitCheck >= FILM_QUIR_CHECK_CNT && !flag_film_quit) {
        if (mfilmNeedComm)
            TOUCH_DEBUG("FilmMode[Quit]: %d\n", filmQuitResult);
        mfilmNeedComm = true;
        if (filmQuitResult < QUIT_FILM_CHECK) {
            flag_film_quit = true;
            mfilmStop = true;
            ALOGI("Return normal mode");
            this->setModelMode(BARE_MODE);
        }
    }
};

void TouchGlove::Handle(int frame_num) {
    if (mgloveStop)
        return;

    stop_glove_invoke = false;
    if (frame_num == GLOVE_END_FRAME) {
        mgloveCheckCnt++;
 
        this->TfModelInvoke(raw_buf, GLOVE_START_FRAME, GLOVE_END_FRAME);
        float glove_rate = this->mOut[0];
        float finger_rate = this->mOut[1];

        // 1.Model Result Output
        char result_buf[1];
        if (glove_rate > finger_rate) {
            result_buf[0] = 1;
        } else {
            result_buf[0] = 0;
        }
        TOUCH_DEBUG("[Glove Invoke] GLove:%f, Finger:%f\n", glove_rate, finger_rate);

        // 2.Handle Model Result
        glove_result.push_back(result_buf[0]);
        if (mgloveCheckCnt <= GLOVE_CHECK_TIME && (getModelMode() != GLOVE_MODE)) {
            if (result_buf[0] == 1 && data_vaild_bit == 0x0e) {
                setModelMode(GLOVE_MODE);
                mgloveCheckCnt = 0;
                mgloveNeedComm = true;
                glove_result.clear();
            }
        }

        if (mgloveCheckCnt >= GLOVE_CHECK_PERD && (getModelMode() == GLOVE_MODE)) {
            int res_sum = 0;
            for (size_t i = 0; i < glove_result.size(); i++) {
                res_sum += glove_result[i];
            }
            if (res_sum >= GLOVE_CHECK_PERD - 1) {
                mgloveCheckCnt = 0;
                mgloveNeedComm = false;
                glove_result.clear();
                TOUCH_DEBUG("[Glove Check] Glove/Finger: (%d / %d)\n", res_sum, GLOVE_CHECK_PERD - res_sum);
            } else {
                mgloveStop = true;
                mgloveNeedComm = true;
                stop_glove_invoke = true;
                setModelMode(film_mode);
                ALOGI("[Glove Mode: Exit] Glove/Finger: (%d / %d)\n", res_sum, GLOVE_CHECK_PERD - res_sum);
            }
        } else if (mgloveCheckCnt >= GLOVE_CHECK_PERD && (getModelMode() != GLOVE_MODE)) {
            mgloveStop = true;
            mgloveNeedComm = true;
            stop_glove_invoke = true;
            setModelMode(film_mode);
        }
    }
}


int main(int argc, char* argv[])
{
    struct pollfd fds[3];
    char temp_buf[1];
    bool flag_start = true;
    bool flag_suspend = false;
    std::unique_ptr<TouchSensor> touchsensor(new TouchSensor);

    if (touchsensor->fileop->check_device_node_ready(param) < 0) {
        ALOGE("Touch Device Not Ready, Time Out!\n");
        return -1;
    }

    if (touchsensor->fileop->touchsensor_fop_init() < 0) {
        ALOGE("Touchsensor Fop Init Failed!\n");
        return -1;
    }
    touchsensor->fileop->fdSetup(fds);
    touchsensor->thpcomm->start_film_sample(SAMPLE_NUM);

    while(1) {
        poll(fds, touchsensor->fileop->mfdListen, -1);
        if (fds[0].revents & (POLLPRI | POLLIN)) {
            pread(fds[0].fd, temp_buf, 1, 0);
            if (!flag_suspend && flag_start) {
                bool flag_sample = true;
                touchsensor->datahandle->addFrameCount();
                int frame_num = touchsensor->datahandle->FrameRecord(touchsensor->fileop->getGlobalValue());

                if (frame_num > 0 && !touchsensor->mMode_rcd.empty()) {
                    for (size_t i = 0; i < touchsensor->mMode_rcd.size(); i++) {
                        int model_num = touchsensor->mMode_rcd[i];
                        touchsensor->mModel[model_num]->Handle(frame_num);

                        flag_sample = (flag_sample && touchsensor->mModel[model_num]->getSampleStatus());

                        bool need_comm = touchsensor->mModel[model_num]->ModelNeedComm();
                        if (need_comm) {
                            TOUCH_DEBUG("[Need Comm] model_mode = %d!\n", touchsensor->mModel[model_num]->getModelMode());
                            int model_mode = touchsensor->mModel[model_num]->getModelMode();
                            if (model_mode == TouchFilm::GLOVE_MODE)
                                touchsensor->thpcomm->write_glove_result(model_mode);
                            else
                                touchsensor->thpcomm->write_film_result(model_mode);
                            touchsensor->mModel[model_num]->clearModelComm();
                        }
                    }
                    //sample control
                    if (flag_sample) {
                        TOUCH_DEBUG("stop_film_sample: flag_sample = %d!\n", flag_sample);
                        flag_start = false;
                        touchsensor->thpcomm->stop_film_sample();
                    }
                }
            }
        }

        if (fds[1].revents & (POLLPRI | POLLIN)) {
            pread(fds[1].fd, temp_buf, 1, 0);
            if (!flag_suspend && flag_start) {
                //TOUCH_DEBUG("Touch Status: %c\n", temp_buf[0]);
                if (temp_buf[0] == '0') {
                    touchsensor->datahandle->resetFrameCount();
                }
            }
        }

        if (fds[2].revents & (POLLPRI | POLLIN)) {
            pread(fds[2].fd, temp_buf, 1, 0);
            TOUCH_DEBUG("Suspend Status: %c\n", temp_buf[0]);
            if (temp_buf[0] == '1') {
                flag_suspend = true;
                flag_start = false;
                touchsensor->thpcomm->stop_film_sample();
            } else {
                flag_suspend = false;
                flag_start = true;
                touchsensor->datahandle->resetFrameCount();
                touchsensor->thpcomm->start_film_sample(SAMPLE_NUM);

                if (!touchsensor->mMode_rcd.empty()) {
                    for (size_t i = 0; i < touchsensor->mMode_rcd.size(); i++) {
                        int model_num = touchsensor->mMode_rcd[i];
                        TOUCH_DEBUG("Model Init: %s\n", model_name[model_num]);
                        touchsensor->mModel[model_num]->Init();
                    }
                }
            }
        }
    }
}
#endif

#ifdef DATA_COLLECT
static char data_buf[4096 * 2000];
FILE *fp_sm;
const char *fp_path;
std::unique_ptr<TimeHandle> timehandle(new TimeHandle);

class DataCollet {
public:
    DataCollet() {
        ALOGI("DataCollet: construct");
        mframeSum = 0;
        mcnt = 0;
        mframeRecordCnt = 0;
        mflagData = false;
        mfpClosed = true;
    }

    ~DataCollet() {
        ALOGI("DataCollet: deconstruct");
    }

    int mframeSum;
    FILE *mfp;
    int mcnt;
    int mframeRecordCnt;
    bool mflagData;
    bool mfpClosed;

    void Init() {
        mframeSum = 0;
        mcnt = 0;
        mframeRecordCnt = 0;
        mflagData = false;
        mfpClosed = true;

        string cur_time = "";
        timehandle->get_current_time_str(cur_time);
        string tmp = "/data/vendor/touch/" + cur_time;
        const char* path = tmp.data();
        ALOGI("[%s] %s!\n", __func__, path);
        this->FileClosed();
        mfp = fopen(path, "w");
        if (mfp == NULL) {
            ALOGE("can't open file: %s\n", path);
            mfpClosed = true;
        } else
            mfpClosed = false;
    }

    void DataFormat(void *raw_data_va, int frame_cnt) {
        if (frame_cnt > SAMPLE_NUM) {
            //TOUCH_DEBUG("NO need Record frame = %d", frame_cnt);
            this->FileWrite(frame_cnt);
            this->FileClosed();
            return;
        }

        if (!mfpClosed) {
            mflagData = true;

            for (int i = 0; i < RX; i++) {
                for (int j = 0; j < TX; j++) {
                    if (j < TX - 1)
                        mcnt += snprintf(data_buf + mcnt, 20, "%5d,", ((int *)raw_data_va)[i * TX + j]);
                    else
                        mcnt += snprintf(data_buf + mcnt, 20, "%5d", ((int *)raw_data_va)[i * TX + j]);
                }
                data_buf[mcnt++] = '\n';
            }
        }
    }

    void FileWrite(int frame_cnt) {
        if (mflagData && !mfpClosed) {
            mframeRecordCnt++;
            data_buf[mcnt] = '\0';
            fwrite(data_buf, mcnt, 1, mfp);
            TOUCH_DEBUG("Record End! cnt: %d, Record frame = %d\n", mcnt, frame_cnt - 1);
        }
    }

    void FileClosed() {
        if (!mfpClosed) {
            fclose(mfp);
            mfpClosed = true;
        }
    }
};

FILE *file_create(string name)
{
    FILE *file = NULL;

    if (!file) {
        string cur_time = "";
        timehandle->get_current_time_str(cur_time);
        string tmp = "/data/vendor/touch/" + name + cur_time;
        const char* path = tmp.data();
        fp_path = tmp.data();
        file = fopen(path, "w");
        if (file == NULL) {
            ALOGE("can't open file: %s\n", path);
            return NULL;
        }
        ALOGI("%s!\n", path);
        return file;
    }

    ALOGI("File %s has already created!\n", name.data());
    return file;
}

void file_close(FILE *file)
{
    if (file) {
        fclose(file);
        file = NULL;
    }
}

inline void file_data(FILE *file, int cmd, int *data)
{
    if (file) {
        int cnt = 0;
        switch (cmd) {
        case WRITE_GLOVE_ENTER:
            for (int i = 0; i < RX; i++) {
                for (int j = 0; j < TX; j++) {
                    cnt += snprintf(data_buf + cnt, 20, "%5d", data[i * TX + j]);
                }
                data_buf[cnt++] = '\n';
            }
            break;
        case WRITE_GLOVE_SUMMARY:
            file = fopen(fp_path, "w+");
            cnt += snprintf(data_buf + cnt, 4096, "[Summary: %d] Glove/Finger: (%d / %d)\n",  data[0], data[1], data[0] - data[1]);
            TOUCH_DEBUG("[Summary: %d] Glove/Finger: (%d / %d)\n", data[0], data[1], data[0] - data[1]);
            break;        
        default:
            break;
        }
        data_buf[cnt++] = '\0';
        fwrite(data_buf, cnt, 1, file);
        file_close(file);
    }
}

void file_write_patten(FILE *fp, int cmd, int *data)
{
    if (fp) {
        switch (cmd) {
        case WRITE_GLOVE_ENTER:
            file_data(fp, WRITE_GLOVE_ENTER, data);
            file_data(fp, WRITE_GLOVE_ENTER, data + RX * TX);
            file_data(fp, WRITE_GLOVE_ENTER, data + RX * TX * 2);
            break;
        case WRITE_GLOVE_SUMMARY:
            file_data(fp, WRITE_GLOVE_SUMMARY, data);
            break;
        default:
            break;
        }
    }
}

void TouchFilm::Handle(int frame_num) {
    if (mfilmStop && glove_mode)
        return;
};

void TouchGlove::Handle(int frame_num) {
    if (frame_num == GLOVE_END_FRAME) {
        this->TfModelInvoke(raw_buf, GLOVE_START_FRAME, GLOVE_END_FRAME);
        float glove_rate = this->mOut[0];
        float finger_rate = this->mOut[1];

        // 1.Model Result Output
        char result_buf[1];
        if (glove_rate > finger_rate) {
            result_buf[0] = GLOVE_MODE;
        } else {
            result_buf[0] = NO_GLOVE_MODE;
        }
        TOUCH_DEBUG("[Single] GLove:%f, Finger:%f\n", glove_rate, finger_rate);

        // 2.Handle Model Result
        glove_result.push_back(result_buf[0]);

        if (fp_sm) {
            int tmp_data[2] = {0};
            int res_sum = 0;
            int cnt_num = (int)glove_result.size();

            for (int i = 0; i < cnt_num; i++) {
                res_sum += glove_result[i];
            }
            tmp_data[0] = cnt_num;
            tmp_data[1] = res_sum;
            file_write_patten(fp_sm, WRITE_GLOVE_SUMMARY, tmp_data);
        }

    }
}

int main(int argc, char* argv[])
{
    struct pollfd fds[3];
    char temp_buf[1];
    bool flag_start = true;
    bool flag_suspend = false;
    std::unique_ptr<TouchSensor> touchsensor(new TouchSensor);
    std::unique_ptr<DataHandle> thpdatahandle(new DataHandle);
    std::unique_ptr<DataCollet> datacollect(new DataCollet);

    if (touchsensor->check_device_node_ready(param) < 0) {
        ALOGE("Touch Device Not Ready, Time Out!\n");
        return -1;
    }

    if (touchsensor->touchsensor_fop_init() < 0) {
        ALOGE("Touchsensor Fop Init Failed!\n");
        return -1;
    }
    touchsensor->fdSetup(fds);

    while(1) {
        poll(fds, touchsensor->mfdListen, -1);
        if (fds[0].revents & (POLLPRI | POLLIN)) {
            pread(fds[0].fd, temp_buf, 1, 0);
            if (!flag_suspend && flag_start) {
                thpdatahandle->addFrameCount();
                if (thpdatahandle->getFrameCount() == 1)
                    datacollect->Init();
                datacollect->DataFormat(touchsensor->mdataBaseVa, thpdatahandle->getFrameCount());

                int frame_num = thpdatahandle->FrameRecord(touchsensor->getGlobalValue());
                if (frame_num > 0 && !touchsensor->mMode_rcd.empty()) {
                    for (size_t i = 0; i < touchsensor->mMode_rcd.size(); i++) {
                        int model_num = touchsensor->mMode_rcd[i];
                        touchsensor->mModel[model_num]->Handle(frame_num);
                    }
                }
            }
        }

        if (fds[1].revents & (POLLPRI | POLLIN)) {
            pread(fds[1].fd, temp_buf, 1, 0);
            if (!flag_suspend && flag_start) {
                //TOUCH_DEBUG("Touch Status: %c\n", temp_buf[0]);
                if (temp_buf[0] == '0') {
                    TOUCH_DEBUG("++++++++++++++++++ Touch up Frame Cnt:%d ++++++++++++++++++++\n", thpdatahandle->getFrameCount());
                    thpdatahandle->resetFrameCount();
                    datacollect->FileWrite(thpdatahandle->getFrameCount());
                    datacollect->FileClosed();
                }
            }
        }

        if (fds[2].revents & (POLLPRI | POLLIN)) {
            pread(fds[2].fd, temp_buf, 1, 0);
            TOUCH_DEBUG("Suspend Status: %c\n", temp_buf[0]);
            if (temp_buf[0] == '1') {
                flag_suspend = true;
                flag_start = false;
                file_close(fp_sm);
                datacollect->FileClosed();
            } else {
                flag_suspend = false;
                flag_start = true;
                thpdatahandle->resetFrameCount();
                fp_sm = file_create("Glove-Summary-");

                if (!touchsensor->mMode_rcd.empty()) {
                    touchsensor->mModel[0]->Init();
                }
            }
        }
    }
}
#endif