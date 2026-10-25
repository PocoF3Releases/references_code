#include <iostream>
#include <dlfcn.h>
#include "AudioClientStub.h"

void* AudioClientStub::LibAudioClientImpl = NULL;
IAudioClientStub* AudioClientStub::ImplInstance = NULL;
std::mutex AudioClientStub::StubLock;

IAudioClientStub* AudioClientStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!ImplInstance) {
        LibAudioClientImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibAudioClientImpl) {
            Create* create = (Create *)dlsym(LibAudioClientImpl, "create");
            ImplInstance = create();
        }else {
            ALOGE("AudioClientStub::GetImplInstance dlerror %s", dlerror());
        }
    }
    return ImplInstance;
}

void AudioClientStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibAudioClientImpl) {
        Destroy* destroy = (Destroy *)dlsym(LibAudioClientImpl, "destroy");
        destroy(ImplInstance);
        dlclose(LibAudioClientImpl);
        LibAudioClientImpl = NULL;
        ImplInstance = NULL;
    }
}
//MIUI ADD: start MIAUDIO_GAME_EFFECT
int AudioClientStub::gameEffectAudioRecordChangeInputSource (
        pid_t pid,
        audio_format_t format,
        audio_source_t inputSource,
        uint32_t sampleRate,
        audio_channel_mask_t channelMask,
        audio_source_t &output,
        audio_flags_mask_t &flags) {
    int ret = AUDIOCLIENT_IMPL_ERROR;
    IAudioClientStub* Istub = GetImplInstance();
    if (Istub)
        ret = Istub->gameEffectAudioRecordChangeInputSource(pid, format, inputSource, sampleRate, channelMask, output,flags);
    return ret;
}
int AudioClientStub::gameEffectAudioTrackChangeStreamType (
        pid_t pid,
        audio_format_t format,
        audio_stream_type_t streamType,
        uint32_t sampleRate,
        audio_channel_mask_t channelMask,
        audio_stream_type_t &output) {
    int ret = AUDIOCLIENT_IMPL_ERROR;
    IAudioClientStub* Istub = GetImplInstance();
    if (Istub)
        ret = Istub->gameEffectAudioTrackChangeStreamType(pid, format, streamType, sampleRate, channelMask, output);
    return ret;
}
//MIUI ADD: end

//MIUI ADD: start MIAUDIO_DUMP_PCM
int AudioClientStub::dumpAudioDataToFile(
        void *buffer,
        size_t size,
        audio_format_t format,
        uint32_t sampleRate,
        uint32_t channelCount,
        int dumpSwitch,
        pid_t pid,
        int sessionId,
        const char *packageName,
        const sp<StreamOutHalInterface>& streamOut) {
    int ret = AUDIOCLIENT_IMPL_OK;
    IAudioClientStub* Istub = GetImplInstance();
    if (Istub)
        Istub->dumpAudioDataToFile(buffer, size, format, sampleRate,
                        channelCount, dumpSwitch, pid, sessionId, packageName, streamOut);
    return ret;
}
//MIUI ADD: end

//MIUI ADD: start MIAUDIO_ULL_CHECK
bool AudioClientStub::isUllRecordSupport(uid_t uid)
{
    bool supported = true;
    IAudioClientStub* Istub = GetImplInstance();
    if (Istub)
        supported = Istub->isUllRecordSupport(uid);
    return supported;
}
//MIUI ADD: end

//MIUI ADD: onetrack rbis start
int AudioClientStub::reportAudiotrackParameters(int playbackTime, const char* clientName, const char* usage, const char* flags, int channelMask, const char* format, int sampleRate)
{
    int ret = 0;
    if(clientName == NULL || usage == NULL || flags == NULL || format == NULL){
        ALOGE("AudioClientStub::reportAudiotrackParameters: invalid parameters!");
        return -1;
    }
    IAudioClientStub* Istub = GetImplInstance();
    if (Istub)
        Istub->reportAudiotrackParameters(playbackTime, clientName, usage, flags, channelMask, format, sampleRate);
    return ret;
}
//MIUI ADD: onetrack rbis end
