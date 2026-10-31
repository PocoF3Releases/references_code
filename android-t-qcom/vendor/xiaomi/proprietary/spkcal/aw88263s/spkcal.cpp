/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "awinic_calibiration"
/*#define LOG_NDEBUG 0*/

#include <stdint.h>
#include <sys/types.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <unistd.h>
#include <math.h>
#include <sys/wait.h>
#include <errno.h>

#include <media/AudioSystem.h>
#include <media/mediaplayer.h>
#include <media/IMediaHTTPService.h>
#include <utils/threads.h>
#include <binder/ProcessState.h>
#include <media/AudioSystem.h>
#include <cutils/properties.h>
#include <log/log.h>

using namespace android;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#define AWINIC_SMARTPA_CALI_RE   "/sys/class/smartpa/re25_calib"
#define AWINIC_SMARTPA_CALI_F0    "/sys/class/smartpa/f0_calib"
#define AWINIC_SMARTPA_CALI_TIEM "/sys/class/smartpa/calib_time"

#define SPK_CAL_SILENCE_FILE "/vendor/etc/spk_cal_silence.wav"
#define SPK_CAL_PINKNOISE_FILE "/vendor/etc/spk_cal_pinknoise.wav"

#ifdef FACTORY_BUILD
#define AW88263_CHK_PERSIST_FILE "/mnt/vendor/persist/audio/aw88263_chk.txt"
#else
#define AW88263_CHK_PERSIST_FILE "/data/vendor/cit/aw88263_chk.txt"
#endif

enum {
    AW_DEV_CH_PRI_L = 0,
    AW_DEV_CH_PRI_R = 1,
    AW_DEV_CH_SEC_L = 2,
    AW_DEV_CH_SEC_R = 3,
    AW_DEV_CH_MAX,
};

static char ch_name[AW_DEV_CH_MAX][6] = {"pri_l", "pri_r", "sec_l", "sec_r"};

double F0;
double Temp;

double F0_min = 600;
double F0_max = 990;
double Temp_min = 0;
double Temp_max = 60;
double Spk_imp_min = 4000;
double Spk_imp_max = 9000;

static int aw882xx_cali_re(int *buf)
{
    int fd_re = 0;
    int ret, i;
    char read_buf[100] = { 0 };
    int cali_re[AW_DEV_CH_MAX] = {0};

    fd_re = open(AWINIC_SMARTPA_CALI_RE, O_RDWR);
    if (fd_re < 0) {
        ALOGE("wusc_debug %s: can not open :%s err:%s", __func__, AWINIC_SMARTPA_CALI_RE, strerror(errno));
        return -1;
    }

    ret = read(fd_re, read_buf, 100);
    if (ret <= 0) {
        ALOGE("%s: read re failed :%s", __func__, strerror(errno));
        ret = -1;
        goto exit;
    }

    ret = sscanf(read_buf, "pri_l:%d mOhms pri_r:%d mOhms sec_l:%d mOhms sec_r:%d mOhms ",
                &cali_re[AW_DEV_CH_PRI_L], &cali_re[AW_DEV_CH_PRI_R],
                &cali_re[AW_DEV_CH_SEC_L], &cali_re[AW_DEV_CH_SEC_R]);
    if (!ret) {
        ALOGD("%s:get cali_re failed ,[%s]", __func__, read_buf);
        ret = -1;
        goto exit;
    }

    for (i = 0; i < ret; i++) {
        buf[i] = cali_re[i];
        //printf("cali_re[%d]:%d \n", i, cali_re[i]);
    }

exit:
    if (fd_re) {
        close(fd_re);
    }
    return ret;
}

static int aw882xx_write_cali_re_to_dirver(int *cali_re, int buf_size)
{
    int fd_re = 0, len = 0;
    int ret, i;
    char write_buf[256] = { 0 };

    fd_re = open(AWINIC_SMARTPA_CALI_RE, O_RDWR);
    if (fd_re < 0) {
        ALOGE("%s:can not open :%s err:%s", __func__, AWINIC_SMARTPA_CALI_RE, strerror(errno));
        return -1;
    }

    for (i = 0; i < buf_size; i ++) {
        len += snprintf(write_buf+len, 100-len, "%s:%d ", ch_name[i], cali_re[i]);
    }

    ret = write(fd_re, write_buf, len);
    if (ret < 0) {
        ALOGE("%s: write [%s] failed :%s", __func__, write_buf, strerror(errno));
        ret = -1;
        goto exit;
    }

    //printf("write [%s] success \n", write_buf);
exit:
    if (fd_re) {
        close(fd_re);
    }

    return ret;
}

static int aw882xx_cali_f0(int *buf)
{
    int fd_f0 = 0;
    int ret, i;
    char read_buf[100] = { 0 };
    int cali_f0[AW_DEV_CH_MAX] = {0};

    fd_f0 = open(AWINIC_SMARTPA_CALI_F0, O_RDWR);
    if (fd_f0 < 0) {
        printf("%s:can not open : %s \n", __func__, AWINIC_SMARTPA_CALI_F0);
        return -1;
    }

    ret = read(fd_f0, read_buf, 100);
    if (ret <= 0) {
        printf("%s: read f0 failed \n", __func__);
        ret = -1;
        goto exit;
    }

    ret = sscanf(read_buf, "pri_l:%d pri_r:%d sec_l:%d  sec_r:%d ",
                &cali_f0[AW_DEV_CH_PRI_L], &cali_f0[AW_DEV_CH_PRI_R],
                &cali_f0[AW_DEV_CH_SEC_L], &cali_f0[AW_DEV_CH_SEC_R]);
    if (!ret) {
        printf("%s:get cali_f0 failed ,[%s]", __func__, read_buf);
        ret = -1;
        goto exit;
    }

    for (i = 0; i < ret; i++) {
        buf[i] = cali_f0[i];
    }

exit:
    if (fd_f0) {
        close(fd_f0);
    }

    return ret;
}

/*****************************************************************************
 * play media music
 *****************************************************************************/
static pthread_mutex_t mp_lock;
static pthread_cond_t mp_cond;
sp<MediaPlayer> mp = NULL;

static bool isMPlayerPrepared = false;
static bool isMPlayerstarted = false;
static bool isMPlayerCompleted = false;

class MPlayerListener : public MediaPlayerListener
{
    void notify(int msg, int ext1, int ext2, const Parcel *obj)
    {
        ALOGD("message received msg=%d, ext1=%d, ext2=%d obj %p",
                msg, ext1, ext2, obj);
        switch (msg) {
            case MEDIA_NOP: // interface test message
                break;

            case MEDIA_PREPARED:
                pthread_mutex_lock(&mp_lock);
                isMPlayerPrepared = true;
                pthread_cond_signal(&mp_cond);
                pthread_mutex_unlock(&mp_lock);
                break;

            case MEDIA_STARTED:
                pthread_mutex_lock(&mp_lock);
                isMPlayerstarted = true;
                pthread_cond_signal(&mp_cond);
                pthread_mutex_unlock(&mp_lock);
                break;

            case MEDIA_PLAYBACK_COMPLETE:
                pthread_mutex_lock(&mp_lock);
                isMPlayerCompleted = true;
                pthread_cond_signal(&mp_cond);
                pthread_mutex_unlock(&mp_lock);
                break;

            case MEDIA_ERROR:
                isMPlayerCompleted = true;
                isMPlayerPrepared = true;
                break;

            default:
                break;
        }
    }
};

void MPlayerSleep(int msec)
{
    int timestamp;
    int remain;
    int sec, usec;

    mp->getCurrentPosition(&timestamp);
    ALOGD("postion is %d",timestamp );

    remain = msec - timestamp;
    //printf("%s: msec:%d, remain:%d \n", __func__, msec, remain);
    if (remain < 0)
        return;

    sec = remain / 1000;
    usec = (remain - sec * 1000) * 1000;
    if (sec)
        sleep(sec);
    usleep(usec);

    //in case music start has some delay
    mp->getCurrentPosition(&timestamp);
    ALOGD("postion is %d",timestamp );

    remain = msec - timestamp;
    if (remain < 0)
        return;

    sec = remain / 1000;
    usec = (remain - sec * 1000) * 1000;
    if(sec)
        sleep(sec);
    usleep(usec);

    mp->getCurrentPosition(&timestamp);
    ALOGD("postion is %d",timestamp );
}

int PlayMusic(const char *fileName)
{
    int timestamp = 0;

    if (mp == NULL)
        ALOGE("failed to setDataSource for %s", fileName);

    mp->reset();
    if (mp->setDataSource(NULL, fileName, NULL) == NO_ERROR) {
        mp->prepare();
    } else {
        ALOGE("failed to setDataSource for %s", fileName);
        return -1;
    }

    ALOGD("starting to play %s", fileName);
    //waiting for media player is prepared.
    pthread_mutex_lock(&mp_lock);
    while (!isMPlayerPrepared) {
        pthread_cond_wait(&mp_cond, &mp_lock);
    }
    pthread_mutex_unlock(&mp_lock);

    mp->seekTo(0);
    mp->start();

    //waiting for media player is started.
    pthread_mutex_lock(&mp_lock);
    while (!isMPlayerstarted) {
        pthread_cond_wait(&mp_cond, &mp_lock);
    }
    pthread_mutex_unlock(&mp_lock);

    while (timestamp <= 0) {
        mp->getCurrentPosition(&timestamp);
        usleep(500);
    }

    return 0;
}

void exitWithHint(const char *pHint)
{
    printf("spkcal: invalid command line: %s\n\r\n\r", pHint);
    //printf("usage: spkcal [-t temperature] [-l L channel config file] [-r R channel config file] [-c Re calitrating] [-m measure F0 Q] \n\r");
    exit(-1);
}

int main(int argc, char *argv[])
{
    int nArg = 1;
    bool bValidArg = false;
    bool bSpkChk = false;
    bool bSpkCal = false;
    double temp = 27.0;
    char pHint[256];
    bool spkc_result = true;
    int cali_re[AW_DEV_CH_MAX] = {0};
    int cali_f0[AW_DEV_CH_MAX] = {0};
    int ret = 0, i;
    int index;

    while (nArg < argc) {
        bValidArg = false;

        if (!strncmp(argv[nArg], "-t", 2)) {
            printf("nArg = %d, argc = %d\n\r", nArg, argc);
            if (argc <= (nArg + 1))
                exitWithHint("temperature parameter is missing");
                temp = atof(argv[nArg + 1]);
                nArg++;
                bValidArg = true;
        }

        if (!strncmp(argv[nArg], "-l", 2)) {
            if (argc <= (nArg + 1))
            exitWithHint("configuration file name is missing");
            nArg++;
            bValidArg = true;
        }

        if (!strncmp(argv[nArg], "-r", 2)) {
            if (argc <= (nArg + 1))
                exitWithHint("configuration file name is missing");
                nArg++;
                bValidArg = true;
        }

        if (!strncmp(argv[nArg], "-c", 2)) {
            bSpkCal = true;
            bValidArg = true;
        }

        if (!strncmp(argv[nArg], "-m", 2)) {
            bSpkChk = false;
            bValidArg = true;
        }

        if (!strncmp(argv[nArg], "-d", 2)) {
            bValidArg = true;
        }

        if (!strcmp(argv[nArg], "-u")) {
            bValidArg = true;
        }

        if (!strcmp(argv[nArg], "-v")) {
            bValidArg = true;
        }

        if (!bValidArg) {
            sprintf(pHint, "don't know argument %s", argv[nArg]);
            exitWithHint(pHint);
        }

        nArg++;
    }

    // play mute wav file
    mp = new MediaPlayer();
    sp<MPlayerListener> mListener = new MPlayerListener();
    if (mp == NULL) {
        ALOGE("failed to create MediaPlayer");
        return -1;
    }
    mp->setListener(mListener);

    //set max volume
    AudioSystem::getStreamVolumeIndex(AUDIO_STREAM_MUSIC, &index, AUDIO_DEVICE_OUT_SPEAKER);
    AudioSystem::setStreamVolumeIndex(AUDIO_STREAM_MUSIC, 150, AUDIO_DEVICE_OUT_SPEAKER);

    if (bSpkCal) {
        // play music
        PlayMusic(SPK_CAL_SILENCE_FILE);
        MPlayerSleep(500);

        // start cali re
        ret = aw882xx_cali_re(cali_re);
            if (ret < 0) {
            printf("cali_re failed \n");
            return -1;
        }

        // print cali result
        for (i = 0; i < ret; i++) {
            printf("SPK: %s: %d mOhms ", ch_name[i], cali_re[i]);
            if ((cali_re[i] > Spk_imp_min) && (cali_re[i] < Spk_imp_max)) {
                spkc_result = true;
                mp->stop();
                printf("Calibration sucess!\n");
            } else {
                /* clear calibration value if the valuse out of range. */
                spkc_result = false;
                MPlayerSleep(10);
                mp->stop();
                printf("Calibration fail! out of bound.\n");
                MPlayerSleep(200);
            }
        }

        // store cali re to driver
        aw882xx_write_cali_re_to_dirver(cali_re, ret);
    }

    if (bSpkCal) {
        FILE *pFile = fopen(AW88263_CHK_PERSIST_FILE, "w+");
        if (pFile == NULL) {
            ALOGE("fopen can not open :%s err:%s", AWINIC_SMARTPA_CALI_RE, strerror(errno));
            printf("spkcal -c fail!\n");
            return -1;
        }
        for (i = 0; i < ret; i++) {
            fprintf(pFile, "%s:%d mOhms ", ch_name[i], cali_re[i]);
            if (cali_re[i] > Spk_imp_max ||  cali_re[i] < Spk_imp_min) {
                fprintf(pFile,"SPK: %s Calibration fail! out of bound.\n", ch_name[i]);
            }
        }
        fprintf(pFile," Calibration sucess!\n");

        fclose(pFile);

        if (access(AW88263_CHK_PERSIST_FILE, F_OK) == 0) {
            printf("AW88263_CHK_PERSIST_FILE is exist success!\n");
            printf("\n");
            if (false == spkc_result) {
                printf("spkcal -c fail!\n");
            } else {
                printf("spkcal -c success!\n");
            }
        } else {
            printf("AW88263_CHK_PERSIST_FILE is not exist fail!\n");
            printf("\n");
            printf("spkcal -c fail!\n");
        }
    }

    if (bSpkChk) {
        PlayMusic(SPK_CAL_PINKNOISE_FILE);
        MPlayerSleep(100);
        ret = aw882xx_cali_f0(cali_f0);
        if (ret < 0) {
            printf("cali_f0 failed");
            return -1;
        }
        mp->stop();

        for (i = 0; i < ret; i++) {
            printf("%s: %d Hz \n", ch_name[i], cali_f0[i]);
        }
    }

    //restore volume
    AudioSystem::setStreamVolumeIndex(AUDIO_STREAM_MUSIC, index, AUDIO_DEVICE_OUT_SPEAKER);

    return 0;
}
