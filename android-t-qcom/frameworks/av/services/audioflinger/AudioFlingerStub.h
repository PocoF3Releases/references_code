#ifndef ANDROID_AUDIOFLINGER_STUB_H
#define ANDROID_AUDIOFLINGER_STUB_H
#include "IAudioFlingerStub.h"
#include <mutex>
#define LIBPATH "libaudioflingerimpl.so"
//using namespace android::AudioRecord;
using namespace android;
class AudioFlingerStub {
private:
    AudioFlingerStub() {}
    static void *LibAudioFlingerImpl;
    static IAudioFlingerStub *ImplInstance;
    static IAudioFlingerStub *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;
public:
    virtual ~AudioFlingerStub() {}
//MIUI ADD: start MIAUDIO_GLOBAL_AUDIO_EFFECT
    static int audioEffectSetStreamVolume(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains,
        audio_stream_type_t stream, float value);
    static int audioEffectSetStandby(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains);
    static int audioEffectSetAppName(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains,
        audio_stream_type_t stream, bool active);
//MIUI ADD: end

//MIUI ADD: start MIAUDIO_DUMP_PCM
    static int dumpAudioFlingerDataToFile(
                   uint8_t *buffer,
                   size_t size,
                   audio_format_t format,
                   uint32_t sampleRate,
                   uint32_t channelCount,
                   const char *name);
//MIUI ADD: end

//MIUI ADD: start MIAUDIO_VOICE_CHANGE
    static void setGameParameters(const String8& keyValuePairs,
        const DefaultKeyedVector< audio_io_handle_t, sp<AudioFlinger::RecordThread> >& RecordThreads);
    static void getGameMode(String8& keyValuePair);
    static void checkForGameParameter_l(const String8& param,
        const SortedVector<sp<AudioFlinger::RecordThread::RecordTrack>>& Tracks,
        uint32_t sampleRate, uint32_t channelCount, void* &voiceChanger, void* &stereo);
    static bool voiceProcess(
        const sp<AudioFlinger::RecordThread::RecordTrack>& activeTrack,
        size_t framesOut, size_t frameSize, uint32_t sampleRate,
        uint32_t channelCount, void* &voiceChanger, void* &stereo);
    static void deleteVocoderStereo(void* &stereo);
    static void        YouMeMagicVoice(void* &voiceChanger);
    static void        stopYouMeMagicVoice(void* &voiceChanger);
//MIUI ADD: end

// AudioCloudCtrl
    static void updateCloudCtrlData(const String8& keyValuePairs);

//MIUI ADD: start MIAUDIO_SPITAL_AUDIO
    static String8 getSpatialDevice(const String8& keys);
//MIUI ADD: end

//MIUI ADD: start MIPERF_PRELOAD_MUTE
    static bool shouldMutedProcess(uid_t uid);
    static bool checkAndUpdatePreloadProcessVolume(const String8& keyValuePairs);
//MIUI ADD: end
};
#endif
