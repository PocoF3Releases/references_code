#ifndef ANDROID_AUDIOPOLICYMANAGER_STUB_H
#define ANDROID_AUDIOPOLICYMANAGER_STUB_H
#include <mutex>
#include "IAudioPolicyManagerStub.h"

#define LIBPATH "/system_ext/lib64/libaudiopolicymanagerimpl.so"

using namespace android;
using namespace audio_policy;

class AudioPolicyManagerStub {
    AudioPolicyManagerStub() {}
    static void *LibAudioPolicyManagerImpl;
    static IAudioPolicyManagerStub *ImplInstance;
    static IAudioPolicyManagerStub *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;

public:
    virtual ~AudioPolicyManagerStub() {}

//#ifdef MIAUDIO_NOTIFICATION_FILTER
    static int notificationFilterGetDevicesForStrategyInt(legacy_strategy strategy,
                DeviceVector &devices, const SwAudioOutputCollection &outputs,
                DeviceVector availableOutputDevices, const Engine *engine);
    static int setNotificationFilter(audio_stream_type_t stream);
//#endif

//#ifdef MIAUDIO_MULTI_ROUTE
    static bool multiRouteSetForceSkipDevice(audio_skip_device_t skipDevive, Engine* engine);
    static status_t multiRouteGetOutputForAttrInt(
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
                        DefaultKeyedVector< uid_t, deviceType >&  multiRoutes);
    static status_t multiRouteSetProParameters(
                        const String8& keyValuePairs,
                        AudioPolicyManager* apm,
                        EngineInstance& engine,
                        DefaultKeyedVector< uid_t, deviceType >&  multiRoutes);
//#endif //MIAUDIO_MULTI_ROUTE

    static void dumpAudioMixer(int fd);

    static void setDumpMixerEnable();

    static void audioDynamicLogInit();

//MIUI ADD: start MIAUDIO_SOUND_TRANSMIT
    static bool soundTransmitEnable();

//MIUI ADD: start MIAUDIO_EFFECT_SWITCH
    static void initOldVolumeCurveSupport();
    static bool isOldVolumeCurveSupport();
    static void setCurrSpeakerMusicStreamVolumeIndex(int index);
//MIUI ADD: end MIAUDIO_EFFECT_SWITCH

    static bool isSmallGame(String8 ClientName);
    static void set_special_region();
    static bool get_special_region();

};

#endif
