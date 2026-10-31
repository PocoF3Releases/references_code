#ifndef ANDROID_IAUDIOFLINGERSTUB_H
#define ANDROID_IAUDIOFLINGERSTUB_H
#include <android-base/macros.h>
#include <cutils/atomic.h>
#include <cutils/compiler.h>
#include <cutils/properties.h>
#include <media/MmapStreamInterface.h>
#include <media/MmapStreamCallback.h>
#include <utils/Errors.h>
#include <utils/threads.h>
#include <utils/SortedVector.h>
#include <utils/TypeHelpers.h>
#include <utils/Vector.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <system/audio-hal-enums.h>
#include <AudioFlinger.h>
#include <utils/KeyedVector.h>

#define AUDIO_IMPL_OK (0)
#define AUDIO_IMPL_ERROR (-8)
#define HW_NOT_SUPPORT (-9)
//using namespace android::AudioRecord;
using namespace android;
class IAudioFlingerStub {
public:
    virtual ~IAudioFlingerStub() {}
//MIUI ADD: start MIAUDIO_GLOBAL_AUDIO_EFFECT
    virtual void audioEffectSetStreamVolume(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains,
        audio_stream_type_t stream, float value);
    virtual void audioEffectSetStandby(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains);
    virtual void audioEffectSetAppName(
        const Vector< sp<AudioFlinger::EffectChain> >& effectChains,
        audio_stream_type_t stream, bool active);
//MIUI ADD: end MIAUDIO_GLOBAL_AUDIO_EFFECT

//MIUI ADD; start MIAUDIO_DUMP_PCM
    virtual void dumpAudioFlingerDataToFile(
                uint8_t *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                const char *name);
//MIUI ADD; end MIAUDIO_DUMP_PCM

//MIUI ADD: start MIAUDIO_VOICE_CHANGE
    virtual void setGameParameters(const String8& keyValuePairs,
        const DefaultKeyedVector< audio_io_handle_t, sp<AudioFlinger::RecordThread> >& RecordThreads);
    virtual void getGameMode(String8& keyValuePair);
    virtual void checkForGameParameter_l(const String8& param,
        const SortedVector<sp<AudioFlinger::RecordThread::RecordTrack>>& Tracks,
        uint32_t sampleRate, uint32_t channelCount,void* &voiceChanger, void* &stereo);
    virtual bool voiceProcess(
        const sp<AudioFlinger::RecordThread::RecordTrack>& activeTrack,
        size_t framesOut, size_t frameSize, uint32_t sampleRate,
        uint32_t channelCount,void* &voiceChanger, void* &stereo);
    virtual void deleteVocoderStereo(void* &stereo);
    virtual void  YouMeMagicVoice(void* &voiceChanger);
    virtual void  stopYouMeMagicVoice(void* &voiceChanger);
//MIUI ADD: end MIAUDIO_VOICE_CHANGE

// AudioCloudCtrl
    virtual void updateCloudCtrlData(const String8& keyValuePairs);

//MIUI ADD: start MIAUDIO_SPITAL_AUDIO
    virtual String8 getSpatialDevice(const String8& keys) const;
//MIUI ADD: end MIAUDIO_SPITAL_AUDIO

//MIUI ADD: start MIPERF_PRELOAD_MUTE
    virtual bool shouldMutedProcess(uid_t uid);
    virtual bool checkAndUpdatePreloadProcessVolume(const String8& keyValuePairs);
//MIUI ADD: end MIPERF_PRELOAD_MUTE
};
typedef IAudioFlingerStub* Create();
typedef void Destroy(IAudioFlingerStub *);
#endif
