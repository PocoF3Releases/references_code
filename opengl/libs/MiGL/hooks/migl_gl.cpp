#include <stdarg.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <cutils/log.h>

#include "migl_gl.h"

// ----------------------------------------------------------------------------
namespace android {
// ----------------------------------------------------------------------------

gl_hooks_t gMiGLHooks;

gl_hooks_t *getMiGLHooks() {
    const gl_hooks_t* real_gl = migl::getThreadSpecific();
    gMiGLHooks = *real_gl;
    installMiGLHooks(&gMiGLHooks);
    return &gMiGLHooks;
}

// ----------------------------------------------------------------------------
}; // namespace android
// ----------------------------------------------------------------------------

