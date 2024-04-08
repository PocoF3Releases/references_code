#define LOG_TAG "MediaStub"
#define LOG_NDEBUG 0

#include <iostream>
#include <dlfcn.h>
#include "MediaStub.h"

void* MediaStub::LibMediaImpl = NULL;
IMediaStub* MediaStub::ImplInstance = NULL;
std::mutex MediaStub::StubLock;

IMediaStub* MediaStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!ImplInstance) {
        LibMediaImpl = dlopen(LIBPATH, RTLD_GLOBAL);
        if (LibMediaImpl) {
            Create* create = (Create *)dlsym(LibMediaImpl, "create");
            ALOGV("MediaStub::GetImplInstance for debug create,%p", create);
            if (dlerror())
                ALOGE("MediaStub::GetImplInstance dlerror %s", dlerror());
            else
                ImplInstance = create();
        }
    }
    return ImplInstance;
}

void MediaStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibMediaImpl) {
        Destroy* destroy = (Destroy *)dlsym(LibMediaImpl, "destroy");
        destroy(ImplInstance);
        dlclose(LibMediaImpl);
        LibMediaImpl = NULL;
        ImplInstance = NULL;
    }
}

//MIUI ADD: start MIAUDIO_OZO
void MediaStub::parseOzoOrientations(
        KeyedVector<size_t, AString> & events,
        const char *data){
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->parseOzoOrientations(events, data);
    return;
}

status_t MediaStub::setOzoAudioTuneFile(
        sp<IMediaRecorder> &mediaRecorder,
        media_recorder_states &currentState, int fd) {
    int ret = AUDIO_IMPL_OK;
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        ret = Istub->setOzoAudioTuneFile(mediaRecorder, currentState, fd);
    return ret;
}

status_t MediaStub::setOzoRunTimeParametersMedia(
            sp<IMediaRecorder> &mediaRecorder,
            media_recorder_states &currentState,
            const String8& params) {
    int ret = AUDIO_IMPL_OK;
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        ret = Istub->setOzoRunTimeParametersMedia(mediaRecorder, currentState,
            params);
    return ret;
}

void MediaStub::createOzoAudioParamsStagefright(void* &ozoAudioParams){
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->createOzoAudioParamsStagefright(ozoAudioParams);
}

void MediaStub::destroyOzoAudioParamsStagefright(
        void * &ozoAudioParams) {
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->destroyOzoAudioParamsStagefright(ozoAudioParams);
    return;
}

void MediaStub::closeOzoAudioTuneFile(
        IMediaCodecEventListener* &ozoTuneWriter) {
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->closeOzoAudioTuneFile(ozoTuneWriter);
    return;
}

bool MediaStub::ozoAudioSetParameterStagefrightImpl(void *params,
        bool &ozoFileSourceEnable, const String8 &key, const String8 &value) {
    bool ret = AUDIO_IMPL_OK;
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        ret = Istub->ozoAudioSetParameterStagefrightImpl(params,
            ozoFileSourceEnable, key, value);
    return ret;
}

void MediaStub::ozoAudioInitStagefrightImpl(void* ozoAudioParams) {
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->ozoAudioInitStagefrightImpl(ozoAudioParams);
    return;
}

bool MediaStub::setOzoAudioTuneFileStagefrightRecorder(
        IMediaCodecEventListener* &ozoTuneWriter, int fd){
    bool ret = AUDIO_IMPL_OK;
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        ret = Istub->setOzoAudioTuneFileStagefrightRecorder(ozoTuneWriter, fd);
    return ret;
}

status_t MediaStub::setOzoRunTimeParametersStagefright(void *ozoAudioParams,
        sp<MediaCodecSource> audioEncoderSource, const String8 &params) {
    int ret = AUDIO_IMPL_OK;
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        ret = Istub->setOzoRunTimeParametersStagefright(ozoAudioParams,
            audioEncoderSource, params);
    return ret;
}

status_t MediaStub::createOzoAudioSource(
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
        bool &use_ozo_capture) {
    int ret = HW_NOT_SUPPORT;
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        ret = Istub->createOzoAudioSource(attr, clientIdentity,
            sourceSampleRate, channels, outSampleRate, selectedDeviceId,
            selectedMicDirection, selectedMicFieldDimension, audioEncoder,
            outputFormat, ozoAudioParams, audioSource, audioSourceNode,
            ozoFileSourceEnable, use_ozo_capture);
    return ret;
}

void MediaStub::setOzoAudioFormat(
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
        sp<AMessage> &format) {
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->setOzoAudioFormat(channels, samplerate, ozoAudioParams,
            codecEventListener, use_ozo_capture, ozoBrandEnabled, audioBitRate,
            audioSource, ozoTuneWriter, listener, format);
}

void MediaStub::setOzoAudioCodecInfo(
        sp<MediaCodecSource> audioEncoderSource,
        IMediaCodecEventListener* ozoTuneWriter,
        IMediaCodecEventListener* codecEventListener) {
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->setOzoAudioCodecInfo(audioEncoderSource, ozoTuneWriter,
            codecEventListener);
}

void MediaStub::tryOzoAudioConfigureImpl(const sp<AMessage> &format,
        AString &mime, void* &oZoPlayCtr) {
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->tryOzoAudioConfigureImpl(format, mime, oZoPlayCtr);
}

void MediaStub::ozoPlayCtrlSignal(sp<MediaCodec> &codec,
        void* &oZoPlayCtr) {
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->ozoPlayCtrlSignal(codec, oZoPlayCtr);
}

void MediaStub::destroyOzoPlayCtrl(void* &oZoPlayCtr) {
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->destroyOzoPlayCtrl(oZoPlayCtr);
}

//MIUI ADD: end

//MIUI ADD FOR ONETRACK RBIS START
void MediaStub::sendFrameRate(int frameRate){
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->sendFrameRate(frameRate);
}

void MediaStub::sendWidth(int32_t mVideoWidth){
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->sendWidth(mVideoWidth);
}

void MediaStub::sendHeight(int32_t mVideoHeight){
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->sendHeight(mVideoHeight);
}

void MediaStub::sendPackageName(const char* packageName){
    IMediaStub* Istub = GetImplInstance();
    if (Istub){
        Istub->sendPackageName(packageName);
        ALOGD("MediaStub::sendPackageName %s", packageName);
    }
}

void MediaStub::sendDolbyVision(const char* name){
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->sendDolbyVision(name);
}

void MediaStub::sendMine(const char* mime){
    IMediaStub* Istub = GetImplInstance();
    if (Istub){
        Istub->sendMine(mime);
        ALOGD("MediaStub::sendMine %s", mime);
    }

}

void MediaStub::sendHdrInfo(int32_t fHdr){
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->sendHdrInfo(fHdr);
}

void MediaStub::sendFrameRateFloatCal(int frameRateFloatCal){
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->sendFrameRateFloatCal(frameRateFloatCal);
}

void MediaStub::updateFrcAieAisState(){
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->updateFrcAieAisState();
}

void MediaStub::sendVideoPlayData(){
    IMediaStub* Istub = GetImplInstance();
    if (Istub)
        Istub->sendVideoPlayData();
}
//MIUI ADD FOR ONETRACK RBIS END