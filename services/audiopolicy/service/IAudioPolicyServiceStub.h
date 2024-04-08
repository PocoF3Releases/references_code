#ifndef ANDROID_IAUDIOPOLICYSERVICESTUB_H
#define ANDROID_IAUDIOPOLICYSERVICESTUB_H
#include <android-base/macros.h>
#include <cutils/atomic.h>
#include <cutils/compiler.h>
#include <cutils/properties.h>
#include <utils/Errors.h>
#include <utils/Vector.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <system/audio-hal-enums.h>
#include <media/EffectsConfig.h>
#include <system/audio_effects/audio_effects_conf.h>
#include <AudioPolicyService.h>
using namespace android;
using namespace effectsConfig;
#define AUDIO_IMPL_OK (0)
#define AUDIO_IMPL_ERROR (-8)
#define HW_NOT_SUPPORT (-9)
class IAudioPolicyServiceStub {
public:
    virtual ~IAudioPolicyServiceStub() {}
    virtual int loadGlobalEffects(const sp<AudioPolicyEffects>& ape,
        cnode *root, const Vector <AudioPolicyEffects::EffectDesc *>& effects);
    virtual void createGlobalEffects(const sp<AudioPolicyEffects>& ape);
    virtual void loadGlobalAudioEffectXmlConfig(
        Vector< AudioPolicyEffects::EffectDesc > &descs,
        struct ParsingResult &result);
    virtual bool isAllowedAPP(String16 ClientName);
};
typedef IAudioPolicyServiceStub* Create();
typedef void Destroy(IAudioPolicyServiceStub *);
#endif
