#ifndef ANDROID_AUDIOCLIENT_STUB_H
#define ANDROID_AUDIOCLIENT_STUB_H
#include "IAudioClientStub.h"
#include <mutex>
#define LIBPATH "libaudioclientimpl.so"
//using namespace android::AudioRecord;
//using namespace android;

class AudioClientStub {
private:
    AudioClientStub() {}
    static void *LibAudioClientImpl;
    static IAudioClientStub *ImplInstance;
    static IAudioClientStub *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;

public:
    virtual ~AudioClientStub() {}
//MIUI ADD: start MIAUDIO_GAME_EFFECT
    static int gameEffectAudioRecordChangeInputSource(
                pid_t pid,
                audio_format_t format,
                audio_source_t inputSource,
                uint32_t sampleRate,
                audio_channel_mask_t channelMask,
                audio_source_t &output,
                audio_flags_mask_t &flags);
    static int gameEffectAudioTrackChangeStreamType(
                pid_t pid,
                audio_format_t format,
                audio_stream_type_t streamType,
                uint32_t sampleRate,
                audio_channel_mask_t channelMask,
                audio_stream_type_t &output);
//MIUI ADD: end

//MIUI ADD: start MIAUDIO_DUMP_PCM
    static int dumpAudioDataToFile(
                void *buffer,
                size_t size,
                audio_format_t format,
                uint32_t sampleRate,
                uint32_t channelCount,
                int dumpSwitch,
                pid_t pid = 0,
                int sessionId = -1,
                const char *packageName = "",
                const sp<StreamOutHalInterface>& streamOut = NULL);
//MIUI ADD: end

//MIUI ADD: start MIAUDIO_ULL_CHECK
    static bool isUllRecordSupport(uid_t uid);
//MIUI ADD: end

//MIUI ADD: onetrack rbis start
    static int reportAudiotrackParameters(int playbackTime, const char* clientName, const char* usage, const char* flags, int channelMask, const char* format, int sampleRate);
//MIUI ADD: onetrack rbis end
};

#endif
