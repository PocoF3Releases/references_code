#ifndef ANDROID_MEDIA_STUB_H
#define ANDROID_MEDIA_STUB_H
#include <mutex>
#include "IMediaStub.h"

#define LIBPATH "libmediaimpl.so"

using namespace android;

class MediaStub {
    MediaStub() {}
    static void *LibMediaImpl;
    static IMediaStub *ImplInstance;
    static IMediaStub *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;

public:
    virtual ~MediaStub() {}

//#ifdef MIAUDIO_OZO
    static void parseOzoOrientations(KeyedVector<size_t, AString> & events,
            const char *data);
    static status_t setOzoAudioTuneFile( sp<IMediaRecorder> &mMediaRecorder,
            media_recorder_states &mCurrentState, int fd );
    static status_t setOzoRunTimeParametersMedia(
            sp<IMediaRecorder> &mMediaRecorder,
            media_recorder_states &mCurrentState,
            const String8& params );
    static void createOzoAudioParamsStagefright( void* &ozoAudioParams );
    static void destroyOzoAudioParamsStagefright( void * &ozoAudioParams );
    static void closeOzoAudioTuneFile(
            IMediaCodecEventListener* &ozoTuneWriter);
    static bool ozoAudioSetParameterStagefrightImpl(void *params,
            bool &ozoFileSourceEnable, const String8 &key,
            const String8 &value);
    static void ozoAudioInitStagefrightImpl(void* ozoAudioParams);
    static bool setOzoAudioTuneFileStagefrightRecorder(
            IMediaCodecEventListener* &ozoTuneWriter, int fd);
    static status_t setOzoRunTimeParametersStagefright(void *ozoAudioParams,
            sp<MediaCodecSource> audioEncoderSource, const String8 &params);
    static status_t createOzoAudioSource(
            const audio_attributes_t &attr,
            const android::content::AttributionSourceState& clientIdentity,
            uint32_t sourceSampleRate,
            uint32_t channels,
            uint32_t outSampleRate,
            audio_port_handle_t selectedDeviceId,
            audio_microphone_direction_t selectedMicDirection,
            float selectedMicFieldDimension,
            audio_encoder audioEncoder,
            output_format outputFormat,
            void *ozoAudioParams,
            sp<AudioSource> &audioSource,
            sp<AudioSource> &audioSourceNode,
            bool ozoFileSourceEnable,
            bool &use_ozo_capture );
    static void setOzoAudioFormat(
            int32_t &channels,
            int32_t samplerate,
            void *ozoAudioParams,
            IMediaCodecEventListener* &codecEventListener,
            bool use_ozo_capture,
            bool &ozoBrandEnabled,
            int32_t &audioBitRate,
            sp<AudioSource> &audioSource,
            IMediaCodecEventListener *ozoTuneWriter,
            sp<IMediaRecorderClient> &listener,
            sp<AMessage> &format );
    static void setOzoAudioCodecInfo(
            sp<MediaCodecSource> audioEncoderSource,
            IMediaCodecEventListener *ozoTuneWriter,
            IMediaCodecEventListener* codecEventListener );
    static void tryOzoAudioConfigureImpl( const sp<AMessage> &format,
            AString &mime, void* &oZoPlayCtr );
    static void ozoPlayCtrlSignal( sp<MediaCodec> &codec, void* &oZoPlayCtr );
    static void destroyOzoPlayCtrl( void* &oZoPlayCtr );
//#endif
    //MIUI ADD FOR ONETRACK RBIS START
    static void sendFrameRate(int frameRate);
    static void sendWidth(int32_t mVideoWidth);
    static void sendHeight(int32_t mVideoHeight);
    static void sendPackageName(const char* packageName);
    static void sendDolbyVision(const char* name);
    static void sendMine(const char* mime);
    static void sendHdrInfo(int32_t fHdr);
    static void sendFrameRateFloatCal(int frameRateFloatCal);
    static void updateFrcAieAisState();
    static void sendVideoPlayData();
    //MIUI ADD FOR ONETRACK RBIS END
};

#endif
