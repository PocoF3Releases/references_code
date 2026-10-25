#ifndef ANDROID_INPUTMANAGER_IMPL_H
#define ANDROID_INPUTMANAGER_IMPL_H
#include <mutex>
#include "IMiInputManagerStub.h"
namespace android {

class IMiInputManagerStub;
class MiInputManagerStub {
private:
    static void *LibMiInputManagerImpl;
    static IMiInputManagerStub *ImplInstance;
    static std::mutex StubLock;
    static bool inited;
    static IMiInputManagerStub *GetImplInstance();
    static void DestroyImplInstance();
    static constexpr const char* LIBPATH = "/system_ext/lib64/libmiinputflinger.so";

public:
    MiInputManagerStub();
    virtual ~MiInputManagerStub();
    static void init(InputManager* inputManager);
    static InputListenerInterface* createMiInputMapper(InputListenerInterface& listener);
    static status_t start();
    static status_t stop();
};
}//namespace android
#endif