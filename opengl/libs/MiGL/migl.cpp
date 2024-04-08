#include <stdlib.h>

#include "migl.h"
#include "migl_gl.h"
#include "MiGLSettings.h"

#include <unistd.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <cutils/log.h>

namespace android {
namespace migl {

EGLAPI pthread_key_t gMiGLKey = -1;

void setThreadSpecific(gl_hooks_t const *value) {
    pthread_setspecific(gMiGLKey, value);
}

gl_hooks_t const* getThreadSpecific() {
    return static_cast<gl_hooks_t*>(pthread_getspecific(gMiGLKey));
}

void createThreadSpecificKey() {
    pthread_key_create(&gMiGLKey, NULL);
}

void deleteThreadSpecificKey() {
    pthread_key_delete(gMiGLKey);
}

bool isMiGLEnabled() {
    return MiGLSettings::getInstance().isMiGLEnabled();
}

gl_hooks_t* getGLHooks() {
    return getMiGLHooks();
}

void hookEglCreateContext(int version) {
    //MiGLContext::getInstance().createContext(version);
}

void hookEglDestroyContext() {
    //MiGLContext::getInstance().clearContext();
}

}
}
