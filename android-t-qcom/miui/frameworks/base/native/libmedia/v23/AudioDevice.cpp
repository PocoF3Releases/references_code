#define LOG_TAG "AudioLoopbackSource"

#define AUDIO_PLATFORM_INFO_PATH "/etc/audio_platform_info.xml"
#define AUDIO_PLATFORM_INFO_PATH_IN_VENDOR "/system/vendor/etc/audio_platform_info.xml"

#include <utils/Log.h>
#include <tinyalsa/asoundlib.h>
#include <media/AudioParameter.h>
#include <media/AudioSystem.h>
#include <stdio.h>

#include "AudioDevice.h"

#ifdef USE_VIRTUAL_PROXY_DEV
#include <cutils/config_utils.h>
#include <cutils/misc.h>
#endif

namespace android {

    AudioDevice::AudioDevice(
            int32_t sampleRate,
            int32_t channels)
        : mSampleRate(sampleRate),
          mChannels(channels),
          mDeviceId(kDevice),
          mInitialized(false) {
        ALOGV("AudioDevice sampleRate: %u, channelCount: %u",
                sampleRate, mChannels);

#ifdef RECHECK_AFE_PROXY_DEVICE
        char fn[256];
        FILE * file;
        file = fopen(AUDIO_PLATFORM_INFO_PATH, "r");
        if (file == NULL) {
            file = fopen(AUDIO_PLATFORM_INFO_PATH_IN_VENDOR, "r");
        }
        if (file != NULL) {
            while(fgets(fn, 256, file)) {
                if (strstr(fn, "USECASE_AUDIO_RECORD_AFE_PROXY")) {
                    char* id = strstr(fn, "id=");
                    char* value_start = id + 4;
                    char* value_end;
                    if (id) {
                        value_end = strstr(value_start, "\"");
                        int length = value_end - value_start;
                        char value[length + 1];
                        memcpy(value, value_start, length);
                        value[length] = '\0';
                        snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%sc", kCard, value);
                        mDeviceId = atoi(value);
                        ALOGD("proxy device update to %s", fn);
                    }
                    break;
                }
            }
        }
#endif
    }


    AudioDevice::~AudioDevice() {
        if (mInitialized) {
            closeAudioDevice();
        }
    }

    status_t AudioDevice::openAudioDevice() {
        ALOGI("AudioDevice::openAudioDevice()");

        char audiodevice[256];
        snprintf(audiodevice, sizeof(audiodevice), "hw:%d,%d", kCard, mDeviceId);
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

        mPcmHandle = pcm_open(kCard, mDeviceId, PCM_IN | PCM_MMAP | PCM_NOIRQ, &config);
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

#ifdef USE_VIRTUAL_PROXY_DEV
        android::AudioSystem::setDeviceConnectionState(
                AUDIO_DEVICE_OUT_PROXY,
                enable ? AUDIO_POLICY_DEVICE_STATE_AVAILABLE : AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE,
                "",
                "virtual_proxy_device");
#endif

        android::AudioParameter param = android::AudioParameter();
        param.add(android::String8("audio_loopback"), android::String8(enable ? "on" : "off"));
        android::AudioSystem::setParameters(0, param.toString());
        return OK;
    }
}
