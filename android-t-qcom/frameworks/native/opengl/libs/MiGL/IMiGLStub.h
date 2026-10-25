#ifndef ANDROID_I_MiGL_Stub_H
#define ANDROID_I_MiGL_Stub_H
#include "../hooks.h"
#include "../EGL/egldefs.h"
#include "EGL/egl.h"

namespace android {

class IMiGLStub {
private:
public:
    IMiGLStub() {};
    virtual ~IMiGLStub() {};

    virtual gl_hooks_t const* getGLThreadSpecific(gl_hooks_t const *value);
    virtual void eglCreateContext(int version);
    virtual void eglDestroyContext();
    virtual bool isEnabled();
};
typedef IMiGLStub* createMiGL();
typedef void destroyMiGL(IMiGLStub*);
} // namespace android

#endif
