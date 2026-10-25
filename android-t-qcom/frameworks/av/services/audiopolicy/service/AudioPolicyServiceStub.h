#ifndef ANDROID_AUDIOPOLICYSERVICE_STUB_H
#define ANDROID_AUDIOPOLICYSERVICE_STUB_H
#include <mutex>
#include "IAudioPolicyServiceStub.h"
#define LIBPATH "/system_ext/lib64/libaudiopolicyserviceimpl.so"
using namespace android;
using namespace effectsConfig;
class AudioPolicyServiceStub {
    AudioPolicyServiceStub() {}
    static void *LibAudioPolicyServiceImpl;
    static IAudioPolicyServiceStub *ImplInstance;
    static IAudioPolicyServiceStub *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;
public:
    virtual ~AudioPolicyServiceStub() {}
    static int loadGlobalEffects(const sp<AudioPolicyEffects>& ape,
        cnode *root, const Vector <AudioPolicyEffects::EffectDesc *>& effects);
    static int createGlobalEffects(const sp<AudioPolicyEffects>& ape);
    static int loadGlobalAudioEffectXmlConfig(
        Vector< AudioPolicyEffects::EffectDesc > &descs,
        struct ParsingResult &result);
    static bool isAllowedAPP(String16 ClientName);
};
#endif
