#define LOG_NDEBUG 0
#define LOG_TAG "AudioLoopbackSource"

#include <utils/Log.h>
#include <sys/mman.h>
#include "AudioLoopbackSource.h"

namespace android {

AudioLoopbackSource::AudioLoopbackSource(
        int Fd, size_t size)
    : mStarted(false),
    mInited(false),
    mChannelCount(2),
    mSampleRate(48000),
    mFd(Fd),
    mSize(size),
    mAddress(NULL),
    mStartTimeUs(0),
    mPrevSampleTimeUs(0)
{
    if (Fd > 0 && size > 0) {
        mFd = Fd;
        mAddress = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mFd, 0);
    }
}

AudioLoopbackSource::~AudioLoopbackSource()
{
    stop();
}

bool AudioLoopbackSource::initCheck()
{
    mAudioDevice = new AudioDevice(mSampleRate, mChannelCount);
    if (mAudioDevice->openAudioDevice()) {
        ALOGE("initCheck fail! Unable to open audio device");
        mStarted = false;
        return false;
    }

    ALOGV("init success");
    mInited = true;
    return true;
}

status_t AudioLoopbackSource::start(int64_t startTimeUs)
{
    ALOGV("start");

    Mutex::Autolock autoLock(mLock);
    if (!mInited){
        return OK;
    }
    if (mStarted) {
        return OK;
    }
    mStarted = true;
    mStartTimeUs = startTimeUs;
    mPrevSampleTimeUs = mStartTimeUs;

    mLoopbackRecorder = new AudioLoopbackRecord(*this);
    mLoopbackRecorder->run("AudioLoopBackRecorderThread", ANDROID_PRIORITY_AUDIO);
    return OK;
}

void AudioLoopbackSource::releaseQueuedFrames_l()
{
    Mutex::Autolock autoLock(mLock);
    List<MediaBuffer *>::iterator it;
    while (!mBuffersReceived.empty()) {
        it = mBuffersReceived.begin();
        (*it)->release();
        mBuffersReceived.erase(it);
    }
}

status_t AudioLoopbackSource::stop()
{
    ALOGV("stop");
    Mutex::Autolock autoLock(mStopLock);
    {
        Mutex::Autolock autoLock(mLock);
        if (!mStarted) {
            return OK;
        }
        mStarted = false;
        mInited = false;
    }

    if (mLoopbackRecorder != 0) {
        mLoopbackRecorder->requestExitAndWait();
        mLoopbackRecorder.clear();
    }

    mAudioDevice->closeAudioDevice();
    mFrameAvailableCondition.signal();
    releaseQueuedFrames_l();

    return OK;
}

status_t AudioLoopbackSource::fillBuffer(size_t off, size_t size, int64_t* timeUs, size_t *len)
{
    Mutex::Autolock autoLock(mLock);

    if (!mStarted) {
        ALOGE("fillBuffer err! (Have not start)");
        return INVALID_OPERATION;
    }
    if (mSize < off + size) {
        ALOGE("Buffer overflow will happen if we read %zu data from %zu", size, off);
        return INVALID_OPERATION;
    }
    while (mBuffersReceived.empty()) {
        mFrameAvailableCondition.wait(mLock);
        if (!mStarted) {
            return INVALID_OPERATION;
        }
    }

    size_t readsize  = 0;
    size_t leftsize  = size;
    while (readsize < size && !mBuffersReceived.empty()) {
        MediaBuffer *buffer = *mBuffersReceived.begin();
        size_t offset = buffer->range_offset();
        size_t tryReadSize = buffer->range_length() > leftsize ? leftsize : buffer->range_length();
        memcpy((uint8_t *)mAddress + readsize,(uint8_t *)buffer->data() + offset, tryReadSize);
        readsize += tryReadSize;
        leftsize = size - readsize;
        buffer->set_range(offset + readsize, buffer->range_length() - readsize);
        if (!buffer->range_length()) {
            mBuffersReceived.erase(mBuffersReceived.begin());
            buffer->release();
        }
    }
    *timeUs = calcPresentationTimeUs(readsize);
    *len = readsize;
    return OK;
}

int64_t AudioLoopbackSource::calcPresentationTimeUs(size_t size)
{
    const size_t frameSize = mAudioDevice->getFrameSize();
    const int64_t timestampUs =
        mPrevSampleTimeUs +
        ((1000000LL * (size / frameSize)) +
         (mSampleRate >> 1)) / mSampleRate;

    int64_t ret = mPrevSampleTimeUs;
    mPrevSampleTimeUs = timestampUs;
    return ret;
}

status_t AudioLoopbackSource::queueInputBuffer(void* buf, size_t bufferSize)
{
    Mutex::Autolock autoLock(mLock);
    if (!mStarted) {
        ALOGW("Spurious queueInputBuffer. Drop the audio data.");
        return OK;
    }

    if (bufferSize == 0) {
        ALOGW("Nothing is available from AudioRecord callback buffer");
        return OK;
    }

    MediaBuffer *buffer = new MediaBuffer(bufferSize);
    memcpy((void *) buffer->data(), buf, bufferSize);
    buffer->set_range(0, bufferSize);
    mBuffersReceived.push_back(buffer);
    mFrameAvailableCondition.signal();
    return OK;
}

status_t AudioLoopbackSource::getMetadata(int32_t *sample, int32_t *channel)
{
    Mutex::Autolock autoLock(mLock);
    *sample = mSampleRate;
    *channel = mChannelCount;
    return OK;
}

AudioLoopbackSource::AudioLoopbackRecord::AudioLoopbackRecord(AudioLoopbackSource& audiosource)
    :Thread(false),
    mAudiosource(audiosource)
{
}

bool AudioLoopbackSource::AudioLoopbackRecord::threadLoop()
{

    size_t bufferSize = mAudiosource.mAudioDevice->getBufferSize();
    void *buffer = malloc(bufferSize);

    while (!exitPending()) {
        if (!mAudiosource.mAudioDevice->readData(buffer, bufferSize)) {
            mAudiosource.queueInputBuffer(buffer, bufferSize);
        }
    }

    free(buffer);
    return true;
}

}  // namespace android

