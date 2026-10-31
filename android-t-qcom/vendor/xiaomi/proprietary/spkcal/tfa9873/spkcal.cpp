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
#include <stdint.h>
#include <sys/types.h>
#include <math.h>
#include <fcntl.h>
#include <utils/misc.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/select.h>


#include <media/AudioSystem.h>
#include <media/mediaplayer.h>
#include <media/IMediaHTTPService.h>
#include <utils/threads.h>
#include <binder/ProcessState.h>
#include <media/AudioSystem.h>
#include <cutils/properties.h>

#ifdef __cplusplus
extern "C" {
#endif


extern void exTfa98xx_start_device(const char* profilename);
extern int exTfa98xx_cal(int bMusicFlag, void* pImpedanceList, unsigned short iListLen);
extern double exTfa98xx_f0(int bMusicFlag);
extern double exTfa98xx_temperature(int bMusicFlag);
extern int exTfa98xx_get_spkid(char *spk_name);
extern int exTfa98xx_setProfile(const char* profilename);
extern void exTfa98xx_clear_calibration_flag(void);
extern void exTfa98xx_reset(void);

#ifdef __cplusplus
}
#endif

#define SPK_CAL_SILENCE_FILE "/vendor/etc/spk_cal_silence.wav"
#define SPK_CAL_PINKNOISE_FILE "/vendor/etc/spk_cal_pinknoise.wav"
#define SPK_CAL_WHITENOISE_FILE "/vendor/etc/spk_cal_whitenoise.wav"

#ifdef FACTORY_BUILD
#define TFA9894_CHK_PERSIST_FILE "/mnt/vendor/persist/audio/tfa9894_chk.txt"
#else
#define TFA9894_CHK_PERSIST_FILE "/data/vendor/cit/tfa9894_chk.txt"
#endif
#define TFA98XX_PROFILE_CALIBRTE  "calibrate.cal"
#define TFA98XX_PROFILE_MUSIC     "MUSIC_48000"
#define TFA98XX_PROFILE_FRESFAST  "fresfast"

#define TFA98XX_MAX_DEVICES    4
typedef struct{
	char address[10];
	double impedance;
}tfa98xx_RDC;

static pthread_mutex_t mp_lock;
static pthread_cond_t mp_cond;

static bool isMPlayerPrepared = false;
static bool isMPlayerstarted = false;
static bool isMPlayerCompleted = false;

double F0;
double Temp;

double F0_min = 600;
double F0_max = 990;
double Temp_min = 0;
double Temp_max = 60;
double Spk_imp_min = 6;
double Spk_imp_max = 8;

double F0_min_aac = 600;
double F0_max_aac = 990;
double Temp_min_aac = 0;
double Temp_max_aac = 60;
double Spk_imp_min_aac = 6;
double Spk_imp_max_aac = 8;

double F0_min_goer = 600;
double F0_max_goer = 990;
double Temp_min_goer = 0;
double Temp_max_goer = 60;
double Spk_imp_min_goer = 6;
double Spk_imp_max_goer = 8;

enum{
	IOCTL_CMD_GET_MEMTRACK_DATA = 0,
	IOCTL_CMD_GET_CNT_VERSION,
};

using namespace android;

sp<MediaPlayer> mp = NULL;
float paramVal[8] = {0};

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

	if(mp == NULL)
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
	printf("usage: spkcal [-t temperature] [-l L channel config file] [-r R channel config file] [-c Re calitrating] [-m measure F0 Q] \n\r");
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
	int index;
	char boot_id[128] = {0};
	bool spkc_result = true;
	bool spkm_fresult = true;
	bool spkm_tresult = true;
	bool spkid_match = true;
	const char* speaker_id;
	int fd = 0, ret = 0;
	char version[30] = {0};
	tfa98xx_RDC rdclist[TFA98XX_MAX_DEVICES];
	String8 spk_id("speaker_id");

	property_get("audio.speaker.id", boot_id, NULL);
	exTfa98xx_get_spkid(boot_id);
	speaker_id = AudioSystem::getParameters(spk_id).string();
	ALOGD("speaker id %s, boot is %s", speaker_id, boot_id);
	memset(&rdclist[0], 0x00, sizeof(rdclist));

	if(!speaker_id || !strstr(speaker_id, boot_id )) {
		ALOGE("Speaker ID mismatch, please reboot the phone");
		printf("SPK ID: fail!\n");
		printf("Speaker ID mismatch, please reboot the phone\n");
		spkid_match = false;
    }

    fd = open("/dev/tfa_control", O_RDWR|O_NONBLOCK);
    if (fd < 0) {
        ALOGE("unable to open device\n");
    } else {
        memset(&version[0], 0x00, sizeof(version));
        ret = ioctl(fd, IOCTL_CMD_GET_CNT_VERSION, &version[0]);
        printf("PA version is :%s\n", version);
        close(fd);
    }

	android::ProcessState::self()->startThreadPool();

	//InitFTCC(&sFTCC);
	if (!strcmp(boot_id, "AAC")) {
		ALOGD("Speaker ID value use AAC\n");
		F0_min = F0_min_aac;
		F0_max = F0_max_aac;
		Temp_min = Temp_min_aac;
		Temp_max = Temp_max_aac;
		Spk_imp_min = Spk_imp_min_aac;
		Spk_imp_max = Spk_imp_max_aac;
	}

	if (!strcmp(boot_id, "GOER")) {
		ALOGD("Speaker ID value use GOER\n");
		F0_min = F0_min_goer;
		F0_max = F0_max_goer;
		Temp_min = Temp_min_goer;
		Temp_max = Temp_max_goer;
		Spk_imp_min = Spk_imp_min_goer;
		Spk_imp_max = Spk_imp_max_goer;
	}


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
			//LoadFTCC(argv[nArg + 1], &sFTCC, &(sFTCC.nTSpkCharDevA));
				nArg++;
				bValidArg = true;
		}

		if (!strncmp(argv[nArg], "-r", 2)) {
			if (argc <= (nArg + 1))
				exitWithHint("configuration file name is missing");
			//LoadFTCC(argv[nArg + 1], &sFTCC, &(sFTCC.nTSpkCharDevB));
				nArg++;
				bValidArg = true;
		}

		if (!strncmp(argv[nArg], "-c", 2)) {
			//sFTCC.bLoadCalibration = true;
			bSpkCal = true;
			bValidArg = true;
		}

		if (!strncmp(argv[nArg], "-m", 2)) {
			bSpkChk = true;
			bValidArg = true;
		}

		if (!strcmp(argv[nArg], "-u")) {
			//sFTCC.bLoadCalibration = true;
			bValidArg = true;
		}

		if (!strcmp(argv[nArg], "-v")) {
			//sFTCC.bVerbose = true;
			bValidArg = true;
		}

		if (!bValidArg) {
			sprintf(pHint, "don't know argument %s", argv[nArg]);
			exitWithHint(pHint);
		}

		nArg++;
	}

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
		int retry = 0;
		int ret = 0;
		if (false == spkid_match) {
			printf("spkcal -c fail!\n");
			return 0;
		}

		do {
			retry++;
			PlayMusic(SPK_CAL_SILENCE_FILE);
			MPlayerSleep(1000);

			/* do calibration, and read result */
			ret = exTfa98xx_cal(0, rdclist, TFA98XX_MAX_DEVICES);
			if (0 != ret) {
				exTfa98xx_clear_calibration_flag();
				MPlayerSleep(10);
				mp->stop();
				MPlayerSleep(200);
				continue;
			}

			/* check impedance range */
			spkc_result = false;
			for (index=0; index< TFA98XX_MAX_DEVICES; index++) {
				ALOGD("%s PA%d: {address:%s, impedance:%1.2f}\n", __func__, index+1,
					rdclist[index].address,
					rdclist[index].impedance);
				/* if address is not empty, will be checking value range */
				if (strlen(rdclist[index].address) != 0) {
					if ((rdclist[index].impedance > Spk_imp_min) && (rdclist[index].impedance < Spk_imp_max)) {
						spkc_result = true;
					} else {
						spkc_result = false;
						break;
					}
				}
			}

			if (spkc_result) {
				mp->stop();
				break;
			} else {
				/* clear calibration value if the valuse out of range. */
				exTfa98xx_clear_calibration_flag();
				MPlayerSleep(10);
				mp->stop();
				MPlayerSleep(200);
			}
		} while (retry < 2);

		printf("SPK: %s\n\r", boot_id);
		for (index=0; index< TFA98XX_MAX_DEVICES; index++) {
			if (strlen(rdclist[index].address) != 0) {
				printf("PA%d %s calibration=%1.2f\n", index+1,
					rdclist[index].address, rdclist[index].impedance);
			}
		}

		if (spkc_result) {
			printf("SPK: Calibration sucess!\n");
		} else {
			printf("SPK: Calibration fail! out of bound.\n");
		}
	}

	if (bSpkChk) {

		if (false == spkid_match) {
			printf("spkcal -m fail!\n");
			return 0;
		}

		PlayMusic(SPK_CAL_PINKNOISE_FILE);
		/* switching profile to fresfast. */
		MPlayerSleep(100);
		exTfa98xx_start_device(TFA98XX_PROFILE_FRESFAST);
		MPlayerSleep(3000);
		F0 = exTfa98xx_f0(0);
		printf("SPK F0  = %f\n", F0);
		/* switching profile to default. */
		exTfa98xx_start_device(TFA98XX_PROFILE_MUSIC);
		MPlayerSleep(100);
		mp->stop();

		if (F0 > F0_max || F0 < F0_min) {
			spkm_fresult = false;
			printf("SPK: F0 fail! out of bound.\n");
		} else {
			printf("SPK: F0 success!\n");
		}

		printf("\n");
		PlayMusic(SPK_CAL_WHITENOISE_FILE);
		MPlayerSleep(2000);
		Temp = exTfa98xx_temperature(0);
		printf("SPK temperature = %f\n", Temp);
		mp->stop();

		if (Temp > Temp_max || Temp < Temp_min) {
			spkm_tresult = false;
			printf("SPK: Temperature fail! out of bound.\n");
		} else {
			printf("SPK: Temperature success!\n");
		}
		printf("\n");
	}

	/* reset tfa device after implemented all command.. */
	exTfa98xx_reset();

	if (bSpkCal) {
		FILE *pFile = fopen(TFA9894_CHK_PERSIST_FILE, "ab+");
		//fprintf(pFile, "SPK: %s\n\r", boot_id);
		fprintf(pFile, "PA Version = %s\n\r", version);
		for (index=0; index< TFA98XX_MAX_DEVICES; index++) {
			if (strlen(rdclist[index].address) != 0) {
				fprintf(pFile, "PA%d %s speaker impedance=%1.2f\n", index+1,
					rdclist[index].address, rdclist[index].impedance);
			}
		}
		if (spkc_result) {
			fprintf(pFile, "SPK: Calibration sucess!\n");
		} else {
			fprintf(pFile, "SPK: Calibration fail! out of bound.\n");
		}
		fclose(pFile);

		if (access(TFA9894_CHK_PERSIST_FILE, F_OK) == 0){
			printf("TFA9894_CHK_PERSIST_FILE is exist success!\n");
			printf("\n");
			if (false == spkc_result) {
				printf("spkcal -c fail!\n");
			} else {
				printf("spkcal -c success!\n");
			}
		} else {
			printf("TFA9894_CHK_PERSIST_FILE is not exist fail!\n");
			printf("\n");
			printf("spkcal -c fail!\n");
		}
	}

	if (bSpkChk) {
		FILE *pFile = fopen(TFA9894_CHK_PERSIST_FILE, "ab+");
		fprintf(pFile, "SPK: F0 = %1.2f\n\r", F0);

		if (F0 > F0_max || F0 < F0_min) {
			fprintf(pFile,"SPK: F0 fail! out of bound.\n");
		} else {
			fprintf(pFile,"SPK: F0 success!\n");
		}

		fprintf(pFile, "SPK: T = %1.2f\n\r", Temp);
		if (Temp > Temp_max || Temp < Temp_min) {
			fprintf(pFile,"SPK: Temperature fail! out of bound.\n");
		} else {
			fprintf(pFile,"SPK: Temperature success!\n");
		}
		fclose(pFile);

		if (access(TFA9894_CHK_PERSIST_FILE, F_OK) == 0){
			printf("TFA9894_CHK_PERSIST_FILE is exist sucess!\n");
			printf("\n");
			if (false == spkm_fresult || false == spkm_tresult) {
				printf("spkcal -m fail!\n");
			} else {
				printf("spkcal -m success!\n");
			}
		} else {
			printf("TFA9894_CHK_PERSIST_FILE is not exist fail!\n");
			printf("\n");
			printf("spkcal -m fail!\n");
		}
	}

	//restore volume
	AudioSystem::setStreamVolumeIndex(AUDIO_STREAM_MUSIC, index, AUDIO_DEVICE_OUT_SPEAKER);
	return 0;
}

