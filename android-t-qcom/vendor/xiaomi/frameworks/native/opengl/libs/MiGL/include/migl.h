#ifndef __MIGL_H__
#define __MIGL_H__

#include <pthread.h>
#include "hooks.h"

namespace android {
namespace migl {

void setThreadSpecific(gl_hooks_t const *value);
gl_hooks_t const* getThreadSpecific();
void createThreadSpecificKey();
void deleteThreadSpecificKey();
void hookEglCreateContext(int version);
void hookEglDestroyContext();
bool isMiGLEnabled();
gl_hooks_t* getGLHooks();

}
};

#endif
