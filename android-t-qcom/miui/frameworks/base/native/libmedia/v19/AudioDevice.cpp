#define LOG_TAG "AudioLoopbackSource"

#include <utils/Log.h>
#include <selinux/selinux.h>
#include <tinyalsa/asoundlib.h>
#include <media/AudioParameter.h>
#include <media/AudioSystem.h>

#include "AudioDevice.h"

namespace android {

    AudioDevice::AudioDevice(
            int32_t sampleRate,
            int32_t channels)
        : mSampleRate(sampleRate),
          mChannels(channels),
          mInitialized(false) {
        ALOGV("AudioDevice sampleRate: %u, channelCount: %u",
                sampleRate, mChannels);
    }


    AudioDevice::~AudioDevice() {
        if (mInitialized) {
            closeAudioDevice();
        }
    }

    status_t AudioDevice::openAudioDevice() {
        ALOGI("AudioDevice::openAudioDevice()");

        char audiodevice[256];
        snprintf(audiodevice, sizeof(audiodevice), "hw:%d,%d", kCard, kDevice);
        mInitialized = true;

        if (enableAudioLoopback(true)) {
            ALOGE("enableAudioLoopback failed");
            mInitialized = false;
            return INVALID_OPERATION;
        }

        struct pcm_config config;
        memset(&config, 0, sizeof(config));
        config.channels = mChannels;
        config.rate = mSampleRate;
        config.period_size = 480;
        config.period_count = 32;
        config.format = PCM_FORMAT_S16_LE;

        mPcmHandle = pcm_open(kCard, kDevice, PCM_IN | PCM_MMAP | PCM_NOIRQ, &config);
        if (!mPcmHandle || !pcm_is_ready(mPcmHandle)) {
            ALOGE("Unable to open PCM device (%s), %d\n", pcm_get_error(mPcmHandle), errno);
            enableAudioLoopback(false);
            mInitialized = false;
            return INVALID_OPERATION;
        }

        return OK;
    }

    status_t AudioDevice::closeAudioDevice() {
        ALOGI("AudioDevice::closeAudioDevice()");
        mInitialized = false;

        pcm_close(mPcmHandle);
        mPcmHandle = NULL;
        enableAudioLoopback(false);
        return OK;
    }

    int AudioDevice::readData(void* buffer, size_t bufferSize) {
        return pcm_mmap_read(mPcmHandle, buffer, bufferSize);
    }

    size_t AudioDevice::getFrameSize() {
        return pcm_frames_to_bytes(mPcmHandle, 1);
    }

    size_t AudioDevice::getBufferSize() {
        return pcm_frames_to_bytes(mPcmHandle, pcm_get_buffer_size(mPcmHandle));
    }

    status_t AudioDevice::enableAudioLoopback(bool enable) {

        android::AudioParameter param = android::AudioParameter();
        param.add(android::String8("audio_loopback"), android::String8(enable ? "on" : "off"));
        android::AudioSystem::setParameters(0, param.toString());
        return OK;
    }
}
