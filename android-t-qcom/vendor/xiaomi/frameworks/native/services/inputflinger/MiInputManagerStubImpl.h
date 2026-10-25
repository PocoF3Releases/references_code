#ifndef ANDROID_MI_INPUTMANAGER_IMPL_H
#define ANDROID_MI_INPUTMANAGER_IMPL_H

#include <android-base/thread_annotations.h>
#include <cinttypes>
#include <InputListener.h>
#include "IMiInputManagerStub.h"
#include "knock/InputKnockBase.h"
#include "reader/MiuiInputMapper.h"
#include "MiInputManager.h"

namespace android {

class MiInputManagerStubImpl : public IMiInputManagerStub {
public:
    MiInputManagerStubImpl();
    void init(InputManager* InputManager) override;
    InputListenerInterface* createMiInputMapper(InputListenerInterface& listener);
    status_t start();
    status_t stop();
    sp<KnockServiceInterface> createKnockService(InputListenerInterface* listener);
    static MiInputManagerStubImpl* getInstance();
    sp<KnockServiceInterface> getKnockService();
    MiuiInputMapper* getMiuiInputMapper();
    void dump(std::string&);

protected:
    virtual ~MiInputManagerStubImpl();

private:
    InputManager *mInputManager = nullptr;
    sp<KnockServiceInterface> mKnockService;
    static MiInputManagerStubImpl* sImplInstance;
    static std::mutex mInstLock;
    static constexpr const char* LIBPATH = "/system_ext/lib64/libknockservice.so";
    MiuiInputMapper *mMiuiInputMapper = nullptr;
};

} //namespace:android
#endif