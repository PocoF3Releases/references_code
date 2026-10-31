#define LOG_TAG "AudioPolicyManagerStub"
#define LOG_NDEBUG 0

#include <iostream>
#include <dlfcn.h>
#include "AudioPolicyManagerStub.h"

void* AudioPolicyManagerStub::LibAudioPolicyManagerImpl = NULL;
IAudioPolicyManagerStub* AudioPolicyManagerStub::ImplInstance = NULL;
std::mutex AudioPolicyManagerStub::StubLock;

IAudioPolicyManagerStub* AudioPolicyManagerStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!ImplInstance) {
        ALOGD("GetImplInstance for debug LIBPATH %s", LIBPATH);
        LibAudioPolicyManagerImpl = dlopen(LIBPATH, RTLD_GLOBAL);
        if (LibAudioPolicyManagerImpl) {
            Create* create = (Create *)dlsym(LibAudioPolicyManagerImpl, "create");
            ALOGV("GetImplInstance for debug create,%p", create);
            if (dlerror())
                ALOGE("GetImplInstance dlerror %s", dlerror());
            else
                ImplInstance = create();
        }
    }
    return ImplInstance;
}

void AudioPolicyManagerStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibAudioPolicyManagerImpl) {
        Destroy* destroy = (Destroy *)dlsym(LibAudioPolicyManagerImpl, "destroy");
        destroy(ImplInstance);
        dlclose(LibAudioPolicyManagerImpl);
        LibAudioPolicyManagerImpl = NULL;
        ImplInstance = NULL;
    }
}

//#ifdef MIAUDIO_NOTIFICATION_FILTER
int AudioPolicyManagerStub::notificationFilterGetDevicesForStrategyInt(
    legacy_strategy strategy, DeviceVector &devices,
    const SwAudioOutputCollection &outputs,
    DeviceVector availableOutputDevices, const Engine *engine ) {
    int ret = AUDIO_IMPL_OK;
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->notificationFilterGetDevicesForStrategyInt( strategy, devices,
            outputs, availableOutputDevices, engine);
    return ret;
}

int AudioPolicyManagerStub::setNotificationFilter(
    audio_stream_type_t stream) {
    int ret = AUDIO_IMPL_OK;
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if(Istub)
        Istub->setNotificationFilter(stream);
    return ret;
}
//#endif

//#ifdef MIAUDIO_MULTI_ROUTE
bool AudioPolicyManagerStub::multiRouteSetForceSkipDevice(
    audio_skip_device_t skipDevive, Engine * engine){
    bool ret = false;
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if(Istub)
        ret = Istub->multiRouteSetForceSkipDevice( skipDevive, engine);
    return ret;
}

status_t AudioPolicyManagerStub::multiRouteGetOutputForAttrInt(
                        audio_attributes_t *resultAttr,
                        audio_io_handle_t *output,
                        DeviceVector&  availableOutputDevices,
                        audio_output_flags_t *flags,
                        audio_port_handle_t *selectedDeviceId,
                        audio_session_t session,
                        audio_stream_type_t *stream,
                        const audio_config_t *config,
                        uid_t uid,
                        AudioPolicyInterface::output_type_t *outputType,
                        DeviceVector& outputDevices,
                        AudioPolicyManager* apm,
                        EngineInstance& engine,
                        DefaultKeyedVector< uid_t, deviceType >&  multiRoutes) {
    int ret = HW_NOT_SUPPORT;
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if (Istub)
        ret = Istub->multiRouteGetOutputForAttrInt( resultAttr, output,
            availableOutputDevices, flags, selectedDeviceId, session, stream,
            config, uid, outputType, outputDevices, apm, engine, multiRoutes );
    return ret;
}

status_t AudioPolicyManagerStub:: multiRouteSetProParameters(
                        const String8& keyValuePairs,
                        AudioPolicyManager* apm,
                        EngineInstance& engine,
                        DefaultKeyedVector< uid_t, deviceType >&  multiRoutes) {
    int ret = AUDIO_IMPL_OK;
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if(Istub)
        Istub->multiRouteSetProParameters( keyValuePairs, apm, engine,
            multiRoutes );
    return ret;
}
//#endif

void AudioPolicyManagerStub::dumpAudioMixer(int fd) {
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->dumpAudioMixer(fd);
}

void AudioPolicyManagerStub::setDumpMixerEnable() {
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->setDumpMixerEnable();
}

void AudioPolicyManagerStub::audioDynamicLogInit() {
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->audioDynamicLogInit( );

}

//MIUI ADD: start MIAUDIO_SOUND_TRANSMIT
bool AudioPolicyManagerStub::soundTransmitEnable() {
    bool ret = false;
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if (Istub)
        ret = Istub->soundTransmitEnable();
    return ret;
}
//MIUI ADD: end

//MIUI ADD: start MIAUDIO_EFFECT_SWITCH
void AudioPolicyManagerStub::initOldVolumeCurveSupport(){
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->initOldVolumeCurveSupport();
}

bool AudioPolicyManagerStub::isOldVolumeCurveSupport(){
    bool ret = false;
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if (Istub)
        ret = Istub->isOldVolumeCurveSupport();
    return ret;
}

void AudioPolicyManagerStub::setCurrSpeakerMusicStreamVolumeIndex(int index){
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if (Istub)
        Istub->setCurrSpeakerMusicStreamVolumeIndex(index);
}

bool AudioPolicyManagerStub::isSmallGame( String8 ClientName ) {
    bool ret = false;
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if(Istub)
        ret = Istub->isSmallGame( ClientName );
    return ret;
}

void AudioPolicyManagerStub::set_special_region() {
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if(Istub)
        Istub->set_special_region( );
}

bool AudioPolicyManagerStub::get_special_region() {
    bool ret = false;
    IAudioPolicyManagerStub* Istub = GetImplInstance();
    if(Istub)
        ret = Istub->get_special_region( );
    return ret;
}


