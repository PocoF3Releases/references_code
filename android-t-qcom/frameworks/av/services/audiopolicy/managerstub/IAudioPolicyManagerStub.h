#ifndef ANDROID_IAUDIOPOLICYMANAGERSTUB_H
#define ANDROID_IAUDIOPOLICYMANAGERSTUB_H

#include <android-base/macros.h>
#include <cutils/atomic.h>
#include <cutils/compiler.h>
#include <cutils/properties.h>
#include <utils/Errors.h>
#include <utils/Vector.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <system/audio-hal-enums.h>
#include <system/audio_effects/audio_effects_conf.h>
#include <common/managerdefinitions/include/DeviceDescriptor.h>
#include <enginedefault/src/Engine.h>
#include <AudioPolicyManager.h>

using namespace android;
using namespace audio_policy;

#define AUDIO_IMPL_OK (0)
#define AUDIO_IMPL_ERROR (-8)
#define HW_NOT_SUPPORT (-9)
#define NOT_MULTI_ROUTES_UID (-10)

typedef int deviceType;

class IAudioPolicyManagerStub {
public:
    virtual ~IAudioPolicyManagerStub() {}

//#ifdef MIAUDIO_NOTIFICATION_FILTER
    virtual void notificationFilterGetDevicesForStrategyInt(
        legacy_strategy strategy, DeviceVector &devices,
        const SwAudioOutputCollection &outputs,
        DeviceVector availableOutputDevices, const Engine *engine);
    virtual void setNotificationFilter(audio_stream_type_t stream);
//#endif

//#ifdef MIAUDIO_MULTI_ROUTE
    virtual bool multiRouteSetForceSkipDevice(
        audio_skip_device_t skipDevive, Engine* engine);
    virtual status_t multiRouteGetOutputForAttrInt(
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
    virtual status_t multiRouteSetProParameters(
        const String8& keyValuePairs,
        AudioPolicyManager* apm,
        EngineInstance& engine,
        DefaultKeyedVector< uid_t, deviceType >&  multiRoutes);
//#endif


    virtual void dumpAudioMixer(int fd);

    virtual void setDumpMixerEnable();

    virtual void audioDynamicLogInit();

//MIUI ADD: start MIAUDIO_SOUND_TRANSMIT
    virtual bool soundTransmitEnable();

//MIUI ADD: start MIAUDIO_EFFECT_SWITCH
    virtual void initOldVolumeCurveSupport();
    virtual bool isOldVolumeCurveSupport();
    virtual void setCurrSpeakerMusicStreamVolumeIndex(int index);
//MIUI ADD: end

    virtual int isSmallGame(String8 ClientName);
    virtual void set_special_region();
    virtual bool get_special_region();
};

typedef IAudioPolicyManagerStub* Create();
typedef void Destroy(IAudioPolicyManagerStub *);

#endif
