#ifndef ANDROID_KEYBOARDINPUTMAPPER_IMPL_H
#define ANDROID_KEYBOARDINPUTMAPPER_IMPL_H
#include <mutex>
#include "IMiKeyboardInputMapperStub.h"
namespace android {

class IMiKeyboardInputMapperStub;
class MiKeyboardInputMapperStub {
private:
    static void *LibMiKeyboardInputMapperImpl;
    static IMiKeyboardInputMapperStub *ImplInstance;
    static std::mutex StubLock;
    static bool inited;
    static IMiKeyboardInputMapperStub *GetImplInstance();
    static void DestroyImplInstance();
    static constexpr const char* LIBPATH = "/system_ext/lib64/libmiinputflinger.so";

public:
    MiKeyboardInputMapperStub();
    virtual ~MiKeyboardInputMapperStub();
    static void init(KeyboardInputMapper* keyboardInputMapper);
    static bool processIntercept(const RawEvent* rawEvent);
};
}//namespace android
#endif