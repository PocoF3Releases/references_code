#define LOG_TAG "AudioPolicyServiceStub"
#define LOG_NDEBUG 0

#include <iostream>
#include <dlfcn.h>
#include "AudioPolicyServiceStub.h"

void* AudioPolicyServiceStub::LibAudioPolicyServiceImpl = NULL;
IAudioPolicyServiceStub* AudioPolicyServiceStub::ImplInstance = NULL;
std::mutex AudioPolicyServiceStub::StubLock;

IAudioPolicyServiceStub* AudioPolicyServiceStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!ImplInstance) {
        ALOGV("AudioPolicyStub::GetImplInstance for debug LIBPATH %s", LIBPATH);
        LibAudioPolicyServiceImpl = dlopen(LIBPATH, RTLD_GLOBAL);
        if (LibAudioPolicyServiceImpl) {
            Create* create = (Create*)dlsym(LibAudioPolicyServiceImpl,"create");
            ALOGV("AudioPolicyServiceStub::GetImplInstance for debug create,%p",create);
            if (dlerror())
                ALOGE("AudioPolicyServiceStub::GetImplInstance dlerror %s",dlerror());
            else
                ImplInstance = create();
        }
    }
    return ImplInstance;
}

void AudioPolicyServiceStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibAudioPolicyServiceImpl) {
        Destroy* destroy =
            (Destroy *)dlsym(LibAudioPolicyServiceImpl, "destroy");
        destroy(ImplInstance);
        dlclose(LibAudioPolicyServiceImpl);
        LibAudioPolicyServiceImpl = NULL;
        ImplInstance = NULL;
    }
}
int AudioPolicyServiceStub::loadGlobalEffects(const sp<AudioPolicyEffects>& ape,
        cnode *root, const Vector <AudioPolicyEffects::EffectDesc *>& effects) {
    int ret = AUDIO_IMPL_OK;
    IAudioPolicyServiceStub* Istub = GetImplInstance();
    if(Istub)
        Istub->loadGlobalEffects(ape, root, effects);
    return ret;
}
int AudioPolicyServiceStub::createGlobalEffects(
        const sp<AudioPolicyEffects>& ape) {
    int ret = AUDIO_IMPL_OK;
    IAudioPolicyServiceStub* Istub = GetImplInstance();
    if(Istub)
        Istub->createGlobalEffects(ape);
    return ret;
}
int AudioPolicyServiceStub::loadGlobalAudioEffectXmlConfig(
        Vector< AudioPolicyEffects::EffectDesc > &descs,
        struct ParsingResult &result) {
    int ret = AUDIO_IMPL_OK;
    IAudioPolicyServiceStub* Istub = GetImplInstance();
    if(Istub)
        Istub->loadGlobalAudioEffectXmlConfig(descs, result);
    return ret;
}

bool AudioPolicyServiceStub::isAllowedAPP(String16 ClientName)
{
    bool ret = false;
    IAudioPolicyServiceStub* Istub = GetImplInstance();
    if(Istub)
        ret = Istub->isAllowedAPP(ClientName);
    return ret;
}
