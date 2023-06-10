#include "MiGLStubImpl.h"
#include "migl.h"

using namespace android::migl;

namespace android {

MiGLStubImpl::MiGLStubImpl() {}

MiGLStubImpl::~MiGLStubImpl() {}

gl_hooks_t const* MiGLStubImpl::getGLThreadSpecific(gl_hooks_t const *value) {
    setThreadSpecific(value);
    return getGLHooks();
}

void MiGLStubImpl::eglCreateContext(int version) {
    hookEglCreateContext(version);
}

void MiGLStubImpl::eglDestroyContext() {
    hookEglDestroyContext();
}

bool MiGLStubImpl::isEnabled() {
    return isMiGLEnabled();
}

} // namespace android
