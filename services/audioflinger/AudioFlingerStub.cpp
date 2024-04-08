#include <iostream>
#include <dlfcn.h>
#include "AudioFlingerStub.h"
void* AudioFlingerStub::LibAudioFlingerImpl = NULL;
IAudioFlingerStub* AudioFlingerStub::ImplInstance = NULL;
std::mutex AudioFlingerStub::StubLock;
IAudioFlingerStub* AudioFlingerStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!ImplInstance) {
        LibAudioFlingerImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibAudioFlingerImpl) {
            Create* create = (Create *)dlsym(LibAudioFlingerImpl, "create");
            ImplInstance = create();
        }
    }
    return ImplInstance;
}

void AudioFlingerStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibAudioFlingerImpl) {
        Destroy* destroy = (Destroy *)dlsym(LibAudioFlingerImpl, "destroy");
        destroy(ImplInstance);
        dlclose(LibAudioFlingerImpl);
        LibAudioFlingerImpl = NULL;
        ImplInstance = NULL;
    }
}
// MIUI ADD: start MIAUDIO_GLOBAL_AUDIO_EFFECT
int AudioFlingerStub::audioEffectSetStreamVolume(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains,
        audio_stream_type_t stream, float value) {
    int ret = AUDIO_IMPL_OK;
    IAudioFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->audioEffectSetStreamVolume(effectChains, stream, value);
    return ret;
}
int AudioFlingerStub::audioEffectSetStandby(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains) {
    int ret = AUDIO_IMPL_OK;
    IAudioFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->audioEffectSetStandby(effectChains);
    return ret;
}
int AudioFlingerStub::audioEffectSetAppName(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains,
        audio_stream_type_t stream, bool active) {
    int ret = AUDIO_IMPL_OK;
    IAudioFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->audioEffectSetAppName(effectChains, stream, active);
    return ret;
}
//MIUI ADD: end  MIAUDIO_GLOBAL_AUDIO_EFFECT

//MIUI ADD: start  MIAUDIO_DUMP_PCM
int AudioFlingerStub::dumpAudioFlingerDataToFile(
        uint8_t *buffer,
        size_t size,
        audio_format_t format,
        uint32_t sampleRate,
        uint32_t channelCount,
        const char *name) {
    int ret = AUDIO_IMPL_OK;
    IAudioFlingerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->dumpAudioFlingerDataToFile(buffer, size, format, sampleRate,
            channelCount, name);
    return ret;
}
//MIUI ADD: end MIAUDIO_DUMP_PCM

//MIUI ADD: start MIAUDIO_VOICE_CHANGE
void AudioFlingerStub::setGameParameters(const String8& keyValuePairs,
        const DefaultKeyedVector< audio_io_handle_t, sp<AudioFlinger::RecordThread> >& RecordThreads) {
    IAudioFlingerStub* Istub = GetImplInstance();
    if(Istub)
        Istub->setGameParameters(keyValuePairs,RecordThreads);
}
void AudioFlingerStub::getGameMode(String8& keyValuePair) {
    IAudioFlingerStub* Istub = GetImplInstance();
    if(Istub)
        Istub->getGameMode(keyValuePair);
}
void AudioFlingerStub::deleteVocoderStereo(void* &stereo) {
    IAudioFlingerStub* Istub = GetImplInstance();
    if(Istub)
        Istub->deleteVocoderStereo(stereo);
}

bool AudioFlingerStub::voiceProcess(
        const sp<AudioFlinger::RecordThread::RecordTrack>& activeTrack,
        size_t framesOut, size_t frameSize, uint32_t sampleRate,
        uint32_t channelCount, void* &voiceChanger, void* &stereo) {
    int ret = true;
    IAudioFlingerStub* Istub = GetImplInstance();
    if(Istub)
        ret = Istub->voiceProcess(activeTrack, framesOut, frameSize, sampleRate,
            channelCount, voiceChanger, stereo);
    return ret;
}
void AudioFlingerStub::checkForGameParameter_l(const String8& param,
        const SortedVector<sp<AudioFlinger::RecordThread::RecordTrack>>& Tracks,
        uint32_t sampleRate, uint32_t channelCount, void* &voiceChanger, void* &stereo) {
    IAudioFlingerStub* Istub = GetImplInstance();
    if(Istub)
        Istub->checkForGameParameter_l(param, Tracks, sampleRate,channelCount,voiceChanger, stereo);
}
void AudioFlingerStub::YouMeMagicVoice(void* &voiceChanger) {
    IAudioFlingerStub* Istub = GetImplInstance();
    if(Istub)
        Istub->YouMeMagicVoice(voiceChanger);
}
void AudioFlingerStub::stopYouMeMagicVoice(void* &voiceChanger) {
    IAudioFlingerStub* Istub = GetImplInstance();
    if(Istub)
        Istub->stopYouMeMagicVoice(voiceChanger);
}
//MIUI ADD:end

// AudioCloudCtrl
void AudioFlingerStub::updateCloudCtrlData(const String8& keyValuePairs){
    IAudioFlingerStub* Istub = GetImplInstance();
    if(Istub)
        Istub->updateCloudCtrlData(keyValuePairs);
}

//MIUI ADD: start MIAUDIO_SPITAL_AUDIO
String8 AudioFlingerStub::getSpatialDevice(
        const String8& keys) {
    IAudioFlingerStub* Istub = GetImplInstance();
    if (Istub)
        return Istub->getSpatialDevice(keys);
    return String8("");
}
//MIUI ADD: end

//MIUI ADD: start MIPERF_PRELOAD_MUTE
bool AudioFlingerStub::shouldMutedProcess(uid_t uid) {
    int ret = false;
    IAudioFlingerStub* Istub = GetImplInstance();
    if(Istub)
        ret = Istub->shouldMutedProcess(uid);
    return ret;
}

bool AudioFlingerStub::checkAndUpdatePreloadProcessVolume(const String8& keyValuePairs) {
    int ret = false;
    IAudioFlingerStub* Istub = GetImplInstance();
    if(Istub)
        ret = Istub->checkAndUpdatePreloadProcessVolume(keyValuePairs);
    return ret;
}
//MIUI ADD: end