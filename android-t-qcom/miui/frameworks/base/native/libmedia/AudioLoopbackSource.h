#ifndef __AUDIO_LOOP_BACK_SOURCE

#define __AUDIO_LOOP_BACK_SOURCE

#include <media/stagefright/MediaBuffer.h>
#include <utils/List.h>
#include <utils/threads.h>

#include "AudioDevice.h"

namespace android {

struct AudioLoopbackSource {

    AudioLoopbackSource(int Fd, size_t size);
    ~AudioLoopbackSource();

    bool initCheck();

    status_t start(int64_t startTimeUs);
    status_t stop() ;
    status_t fillBuffer(size_t off, size_t size, int64_t* timeUs, size_t *len);
    status_t getMetadata(int32_t *sample, int32_t *channel);
    int64_t calcPresentationTimeUs(size_t size);

private:

    status_t queueInputBuffer(void* buffer, size_t bufferSize);
    void releaseQueuedFrames_l();

    class AudioLoopbackRecord: public Thread {

    public:
        AudioLoopbackSource& mAudiosource;
        AudioLoopbackRecord(AudioLoopbackSource& audiosource);
        virtual bool threadLoop();
    };

    Mutex mLock;
    Mutex mStopLock;
    Condition mFrameAvailableCondition;

    bool mStarted;
    bool mInited;
    int32_t mChannelCount;
    int32_t mSampleRate;
    int mFd;
    size_t mSize;
    void* mAddress;
    int64_t mStartTimeUs;
    int64_t mPrevSampleTimeUs;

    List<MediaBuffer * > mBuffersReceived;

    sp<AudioLoopbackRecord>  mLoopbackRecorder;
    sp<AudioDevice> mAudioDevice;
};
}

#endif
