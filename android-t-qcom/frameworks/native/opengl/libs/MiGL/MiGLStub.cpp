#include "MiGLStub.h"

#include <dlfcn.h>
#include <unistd.h>

namespace android {

static bool loadOnce = false;
void* MiGLStub::libMiGL = nullptr;
IMiGLStub* MiGLStub::sInstance = nullptr;

MiGLStub::MiGLStub() {}
MiGLStub::~MiGLStub() {}

IMiGLStub* MiGLStub::loadInstance() {
    if(getuid() <= 10000) return sInstance;
    if (!loadOnce) {
        if (!libMiGL) {
            libMiGL = dlopen(LIBPATH, RTLD_LAZY);
            if (libMiGL) {
                createMiGL* c = (createMiGL*)dlsym(libMiGL, "createInstance");
                sInstance = c();
            } else {
                ALOGE("open %s failed! dlopen - %s", LIBPATH, dlerror());
            }
        }
        loadOnce = true;
    }
    return sInstance;
}

void MiGLStub::releaseInstance() {
    if (libMiGL) {
        dlclose(libMiGL);
        libMiGL = nullptr;
    }
}

gl_hooks_t const* MiGLStub::getGLThreadSpecific(gl_hooks_t const *value) {
    IMiGLStub* instance = loadInstance();
    if (instance && instance->isEnabled()) {
        return instance->getGLThreadSpecific(value);
    }
    return value;
}

void MiGLStub::eglCreateContext(int version) {
    IMiGLStub* instance = loadInstance();
    if (instance && instance->isEnabled()) {
        instance->eglCreateContext(version);
    }
}

void MiGLStub::eglDestroyContext() {
    IMiGLStub* instance = loadInstance();
    if (instance && instance->isEnabled()) {
        instance->eglDestroyContext();
    }
}

bool MiGLStub::isEnabled() {
    IMiGLStub* instance = loadInstance();
    if (instance) {
        return instance->isEnabled();
    }
    return false;
}

}
