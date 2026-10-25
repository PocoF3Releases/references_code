/*
 * Copyright (C) 2014 The Android Open Source Project
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
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/AMRWriter.h>
#include <cmds/stagefright/AudioPlayer.h>
#include <media/stagefright/AudioSource.h>
#include <media/stagefright/MediaCodecSource.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <media/mediaplayer.h>
#include <media/IMediaHTTPService.h>
#include <utils/threads.h>
#include <binder/ProcessState.h>



static pthread_mutex_t mp_lock;
static pthread_cond_t mp_cond;
static bool isMPlayerPrepared = false;
static bool isMPlayerstarted = false;

static bool isMPlayerCompleted = false;
char fileName[256] =  {"/vendor/etc/spk.wav"};
using namespace android;

sp<MediaPlayer> mp = NULL;

void sendFCTProfile(int fctmode)
{
    ALOGD("send profile");
    String8 str;
    if(fctmode ==1)
        str += "audio_profile=fct";
    else
        str += "audio_profile=normal";
    AudioSystem::setParameters(0, str);
}

void Msleep(int msec)
{

    int timestamp;
    int remain;
    int sec, usec;


    mp->getCurrentPosition(&timestamp);
    ALOGD("postion is %d",timestamp );

    remain = msec - timestamp;
    if(remain <0)
        return;

    sec = remain/1000;
    usec = (remain - sec*1000)*1000;
    if(sec)
        sleep(sec);
    usleep(usec);

    //in case music start has some delay
    mp->getCurrentPosition(&timestamp);
    ALOGD("postion is %d",timestamp );

    remain = msec - timestamp;
    if(remain <0)
        return;

    sec = remain/1000;
    usec = (remain - sec*1000)*1000;
    if(sec)
        sleep(sec);
    usleep(usec);

    mp->getCurrentPosition(&timestamp);

    ALOGD("postion is %d",timestamp );

}

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


int PlayMusic(char *fileName)
{
    int timestamp = 0;

    mp->reset();
    if(mp ==NULL)
        ALOGE("failed to setDataSource for %s", fileName);
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

     while(timestamp <= 0){
         mp->getCurrentPosition(&timestamp);
         usleep(500);
     }

     return 0;
}


int main()
{
   //Binder interface
    int i;
    int index;
    const int32_t kSampleRate = 48000;
    static const int channels = 1;
    audio_attributes_t attr = AUDIO_ATTRIBUTES_INITIALIZER;
    attr.source = AUDIO_SOURCE_MIC;
    android::ProcessState::self()->startThreadPool();

    mp = new MediaPlayer();
    sp<MPlayerListener> mListener = new MPlayerListener();
    if (mp == NULL) {
         ALOGE("failed to create MediaPlayer");
            return -1;
    }
    mp->setListener(mListener);

    /*set volume to max*/
    AudioSystem::getStreamVolumeIndex(AUDIO_STREAM_MUSIC, &index, AUDIO_DEVICE_OUT_SPEAKER);
#ifdef FACTORY_BUILD
    AudioSystem::setStreamVolumeIndex(AUDIO_STREAM_MUSIC, 15, AUDIO_DEVICE_OUT_SPEAKER);
#else
    AudioSystem::setStreamVolumeIndex(AUDIO_STREAM_MUSIC, 150, AUDIO_DEVICE_OUT_SPEAKER);
#endif
    sendFCTProfile(1);
    PlayMusic(fileName);

    sp<MediaSource> source;
    source = new AudioSource(
            &attr,
            AttributionSourceState(),
            kSampleRate,
            channels);
    source->start();
    MediaBufferBase *mbuf = NULL;
    for(i=0; i<5; i++){
        source->read(&mbuf);
        if (mbuf != NULL) {
            mbuf->release();
            mbuf = NULL;
        }
    }
    source->stop();


    Msleep(100);
    mp->stop();

    /*restore volume*/
    AudioSystem::setStreamVolumeIndex(AUDIO_STREAM_MUSIC, index, AUDIO_DEVICE_OUT_SPEAKER);

    sendFCTProfile(0);

    sleep(3);
    usleep(500*1000);

    ALOGI("setup_fct retrun");

    return 0;
}

