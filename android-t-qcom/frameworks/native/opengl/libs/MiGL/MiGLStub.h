#ifndef ANDROID_MiGL_Stub_H
#define ANDROID_MiGL_Stub_H

#include "IMiGLStub.h"
namespace android {

class MiGLStub {
private:
    static void* libMiGL;
    static IMiGLStub* sInstance;
    static constexpr const char* LIBPATH = "/system_ext/lib64/libmigl.so";
    static IMiGLStub* loadInstance();

public:
    MiGLStub();
    ~MiGLStub();

    static void releaseInstance();
    static gl_hooks_t const* getGLThreadSpecific(gl_hooks_t const *value);
    static void eglCreateContext(int version);
    static void eglDestroyContext();
    static bool isEnabled();

};
} // namespace android

#endif
