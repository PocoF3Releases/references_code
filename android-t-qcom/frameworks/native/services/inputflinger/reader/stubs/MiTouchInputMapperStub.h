#ifndef ANDROID_TOUCH_INPUT_MAPPER_IMPL_H
#define ANDROID_TOUCH_INPUT_MAPPER_IMPL_H
#include <mutex>
#include "IMiTouchInputMapperStub.h"
namespace android {

class IMiTouchInputMapperStub;
class MiTouchInputMapperStub {
private:
    static void *LibMiTouchInputMapperImpl;
    static IMiTouchInputMapperStub *ImplInstance;
    static std::mutex StubLock;
    static bool inited;
    static IMiTouchInputMapperStub *GetImplInstance();
    static void DestroyImplInstance();
    static constexpr const char* LIBPATH = "/system_ext/lib64/libmiinputflinger.so";

public:
    MiTouchInputMapperStub();
    virtual ~MiTouchInputMapperStub();
    static void init(TouchInputMapper* touchInputMapper);
    static bool dispatchMotionIntercept(NotifyMotionArgs* args, int surfaceOrientation);
};
}//namespace android
#endif