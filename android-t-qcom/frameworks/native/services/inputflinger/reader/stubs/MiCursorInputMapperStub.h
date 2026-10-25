#ifndef ANDROID_CURSORINPUTMAPPER_IMPL_H
#define ANDROID_CURSORINPUTMAPPER_IMPL_H
#include <mutex>
#include "IMiCursorInputMapperStub.h"
namespace android {

class MiCursorInputMapperStub {
private:
    static void *LibMiCursorInputMapperImpl;
    static IMiCursorInputMapperStub *ImplInstance;
    static std::mutex StubLock;
    static bool inited;
    static IMiCursorInputMapperStub *GetImplInstance();
    static void DestroyImplInstance();
    static constexpr const char* LIBPATH = "/system_ext/lib64/libmiinputflinger.so";

public:
    MiCursorInputMapperStub();
    virtual ~MiCursorInputMapperStub();
    static void init();
    static void sync(float* vscroll, float* hscroll);
};
}//namespace android
#endif