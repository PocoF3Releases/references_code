#ifndef __AUDIO_DEVICE

#define __AUDIO_DEVICE

#include <utils/Errors.h>

namespace android {

class AudioDevice : public RefBase {

public:
    AudioDevice(
        int32_t sampleRate,
        int32_t channels);

    ~AudioDevice();

    status_t openAudioDevice();
    status_t closeAudioDevice();
    int readData(void* buffer, size_t bufferSize);
    size_t getFrameSize();
    size_t getBufferSize();

private:
    status_t initDevice();
    status_t enableAudioLoopback(bool enable);

    int kCard = 0;
    int kDevice = 8;

    struct pcm *mPcmHandle;
    int32_t mSampleRate;
    int32_t mChannels;
    int32_t mDeviceId;
    bool mInitialized;
};

}

#endif
