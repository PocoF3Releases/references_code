#define LOG_TAG "AudioLoopbackSource"

#include <utils/Log.h>
#include <selinux/selinux.h>
#include <media/AudioParameter.h>
#include <media/AudioSystem.h>

extern "C" {
#include <alsa_audio.h>
}

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

    status_t AudioDevice::initDevice() {

        struct snd_pcm_hw_params *hparams;
        struct snd_pcm_sw_params *sparams;

        hparams = (struct snd_pcm_hw_params*)malloc(sizeof(struct snd_pcm_hw_params));

        param_init(hparams);

        param_set_mask(hparams, SNDRV_PCM_HW_PARAM_ACCESS,
                (mPcmHandle->flags & PCM_MMAP)? SNDRV_PCM_ACCESS_MMAP_INTERLEAVED
                : SNDRV_PCM_ACCESS_RW_INTERLEAVED);
        param_set_mask(hparams, SNDRV_PCM_HW_PARAM_FORMAT,
                SNDRV_PCM_FORMAT_S16_LE);
        param_set_mask(hparams, SNDRV_PCM_HW_PARAM_SUBFORMAT,
                SNDRV_PCM_SUBFORMAT_STD);
        param_set_min(hparams, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, mPcmHandle->period_size);
        param_set_int(hparams, SNDRV_PCM_HW_PARAM_SAMPLE_BITS, 16);
        param_set_int(hparams, SNDRV_PCM_HW_PARAM_FRAME_BITS,
                mPcmHandle->channels - 1 ? 32 : 16);
        param_set_int(hparams, SNDRV_PCM_HW_PARAM_CHANNELS,
                mPcmHandle->channels);
        param_set_int(hparams, SNDRV_PCM_HW_PARAM_RATE, mPcmHandle->rate);

        param_set_hw_refine(mPcmHandle, hparams);

        if (param_set_hw_params(mPcmHandle, hparams))
        {
            ALOGE("AudioSource::AudioSource:_set_params cannot set hw params");
            return INVALID_OPERATION;
        }

        mPcmHandle->buffer_size = pcm_buffer_size(hparams);
        mPcmHandle->period_size = pcm_period_size(hparams);

        mPcmHandle->period_cnt = mPcmHandle->buffer_size/mPcmHandle->period_size;

        sparams = (struct snd_pcm_sw_params*)malloc(sizeof(struct snd_pcm_sw_params));

        sparams->tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
        sparams->period_step = 1;
        sparams->avail_min = (mPcmHandle->flags & PCM_MONO) ?
            mPcmHandle->period_size/2 : mPcmHandle->period_size/4;
        sparams->start_threshold = 1;
        sparams->stop_threshold = (mPcmHandle->flags & PCM_MONO) ?
            mPcmHandle->buffer_size/2 : mPcmHandle->buffer_size/4;
        sparams->xfer_align = (mPcmHandle->flags & PCM_MONO) ?
            mPcmHandle->period_size/2 : mPcmHandle->period_size/4; /* needed for old kernels */
        sparams->silence_size = 0;
        sparams->silence_threshold = 0;

        if (param_set_sw_params(mPcmHandle, sparams))
        {
            ALOGE("AudioSource::AudioSource:_set_params cannot set sw params");
            return INVALID_OPERATION;
        }

        return OK;
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

        mPcmHandle = pcm_open(PCM_IN | PCM_NMMAP | PCM_STEREO, audiodevice);
        if (!mPcmHandle || !pcm_ready(mPcmHandle)) {
            ALOGE("Unable to open PCM device, %d\n", pcm_error(mPcmHandle), errno);
            mInitialized = false;
            return INVALID_OPERATION;
        }

        mPcmHandle->channels = mChannels;
        mPcmHandle->rate = mSampleRate;
        mPcmHandle->flags = PCM_IN | PCM_NMMAP | PCM_STEREO;
        mPcmHandle->period_size = 480;

        if (initDevice()) {
            ALOGE("initDevice failed\n");
            pcm_close(mPcmHandle);
            mPcmHandle = NULL;
            mInitialized = false;
            return INVALID_OPERATION;
        }

        if (pcm_prepare(mPcmHandle))
        {
            ALOGE("pcm_prepare failed\n");
            pcm_close(mPcmHandle);
            mPcmHandle = NULL;
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
        return pcm_read(mPcmHandle, buffer, bufferSize);
    }

    size_t AudioDevice::getFrameSize() {
        return 2 * mChannels;
    }

    size_t AudioDevice::getBufferSize() {
        return mPcmHandle->buffer_size;
    }

    status_t AudioDevice::enableAudioLoopback(bool enable) {
        android::AudioParameter param = android::AudioParameter();
        param.add(android::String8("audio_loopback"), android::String8(enable ? "on" : "off"));
        android::AudioSystem::setParameters(0, param.toString());
        return OK;
    }
}
