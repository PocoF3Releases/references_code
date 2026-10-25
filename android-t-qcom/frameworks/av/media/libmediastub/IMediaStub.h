#ifndef ANDROID_IMEDIASTUB_H
#define ANDROID_IMEDIASTUB_H

#include <android-base/macros.h>
#include <cutils/atomic.h>
#include <cutils/compiler.h>
#include <cutils/properties.h>
#include <utils/Errors.h>
#include <utils/Vector.h>

#include <media/stagefright/foundation/AString.h>
#include <media/IMediaRecorder.h>
#include <media/mediarecorder.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaCodecSource.h>
#include <media/stagefright/AudioSource.h>

using namespace android;

#define AUDIO_IMPL_OK (0)
#define AUDIO_IMPL_ERROR (-8)
#define HW_NOT_SUPPORT (-9)

class IMediaStub {
public:
    virtual ~IMediaStub() {}
    virtual void parseOzoOrientations(KeyedVector<size_t, AString> & events,
            const char *data);
    virtual status_t setOzoAudioTuneFile( sp<IMediaRecorder> &mMediaRecorder,
            media_recorder_states &mCurrentState, int fd );
    virtual status_t setOzoRunTimeParametersMedia(
            sp<IMediaRecorder> &mMediaRecorder,
            media_recorder_states &mCurrentState,
            const String8& params );
    virtual void createOzoAudioParamsStagefright( void* &ozoAudioParams );
    virtual void destroyOzoAudioParamsStagefright( void * &ozoAudioParams );
    virtual void closeOzoAudioTuneFile(
            IMediaCodecEventListener* &ozoTuneWriter);
    virtual bool ozoAudioSetParameterStagefrightImpl(void *params,
            bool &ozoFileSourceEnable, const String8 &key, const String8 &value);
    virtual void ozoAudioInitStagefrightImpl(void* ozoAudioParams);
    virtual bool setOzoAudioTuneFileStagefrightRecorder(
            IMediaCodecEventListener* &ozoTuneWriter, int fd);
    virtual status_t setOzoRunTimeParametersStagefright(void *ozoAudioParams,
            sp<MediaCodecSource> audioEncoderSource, const String8 &params);
    virtual status_t createOzoAudioSource(
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
    virtual void setOzoAudioFormat(
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
    virtual void setOzoAudioCodecInfo(
            sp<MediaCodecSource> audioEncoderSource,
            IMediaCodecEventListener *ozoTuneWriter,
            IMediaCodecEventListener* codecEventListener);
    virtual void tryOzoAudioConfigureImpl( const sp<AMessage> &format,
            AString &mime, void* &oZoPlayCtr);
    virtual void ozoPlayCtrlSignal( sp<MediaCodec> &codec, void* &oZoPlayCtr);
    virtual void destroyOzoPlayCtrl( void* &oZoPlayCtr );
    //MIUI ADD FOR ONETRACK RBIS START
    virtual void sendFrameRate(int frameRate);
    virtual void sendWidth(int32_t mVideoWidth);
    virtual void sendHeight(int32_t mVideoHeight);
    virtual void sendPackageName(const char* packageName);
    virtual void sendDolbyVision(const char* name);
    virtual void sendMine(const char* mime);
    virtual void sendHdrInfo(int32_t fHdr);
    virtual void sendFrameRateFloatCal(int frameRateFloatCal);
    virtual void updateFrcAieAisState();
    virtual void sendVideoPlayData();
    //MIUI ADD FOR ONETRACK RBIS END
};

typedef IMediaStub* Create();
typedef void Destroy(IMediaStub *);

#endif
