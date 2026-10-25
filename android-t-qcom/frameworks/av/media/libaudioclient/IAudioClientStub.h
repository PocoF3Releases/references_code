#ifndef ANDROID_IAUDIOCLIENTSTUB_H
#define ANDROID_IAUDIOCLIENTSTUB_H
#include <system/audio.h>
#include <system/audio_policy.h>
#include <system/audio-hal-enums.h>
#include <media/AudioTrack.h>

//MIUI ADD: start  MIAUDIO_DUMP_PCM
#include <media/audiohal/StreamHalInterface.h>
//MIUI ADD: end

#define AUDIOCLIENT_IMPL_OK (0)
#define AUDIOCLIENT_IMPL_ERROR (-8)
#define HW_NOT_SUPPORT (-9)
//using namespace android::AudioRecord;
using namespace android;

class IAudioClientStub {
public:
        virtual ~IAudioClientStub() {}
//MIUI ADD: start MIAUDIO_GAME_EFFECT
        virtual int gameEffectAudioRecordChangeInputSource(
                                pid_t pid,
                                audio_format_t format,
                                audio_source_t inputSource,
                                uint32_t sampleRate,
                                audio_channel_mask_t channelMask,
                                audio_source_t &output,
                                audio_flags_mask_t &flags);
        virtual int gameEffectAudioTrackChangeStreamType(
                                pid_t pid,
                                audio_format_t format,
                                audio_stream_type_t streamType,
                                uint32_t sampleRate,
                                audio_channel_mask_t channelMask,
                                audio_stream_type_t &output);
//MIUI ADD: end

//MIUI ADD: start MIAUDIO_DUMP_PCM
        virtual void dumpAudioDataToFile(
                void *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                int dumpSwitch,
                pid_t pid,
                int sessionId,
                const char *packageName,
                const sp<StreamOutHalInterface>& streamOut);

//MIUI ADD: start MIAUDIO_ULL_CHECK
    virtual bool isUllRecordSupport(uid_t uid);
//MIUI ADD: end

//MIUI ADD: onetrack rbis start
    virtual int reportAudiotrackParameters(int playbackTime, const char* clientName, const char* usage, const char* flags, int channelMask, const char* format, int sampleRate);
//MIUI ADD: onetrack rbis end
};
typedef IAudioClientStub* Create();
typedef void Destroy(IAudioClientStub *);
#endif

