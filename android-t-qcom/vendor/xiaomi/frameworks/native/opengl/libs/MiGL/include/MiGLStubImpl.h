#ifndef __MIGL_STUB_IMPL_H__
#define __MIGL_STUB_IMPL_H__

#include "MiGL/IMiGLStub.h"

namespace android {

class MiGLStubImpl : public IMiGLStub {
private:

public:
    MiGLStubImpl();
    ~MiGLStubImpl();

    gl_hooks_t const* getGLThreadSpecific(gl_hooks_t const *value);
    void eglCreateContext(int version);
    void eglDestroyContext();
    bool isEnabled();
};

} // namespace android

#endif
