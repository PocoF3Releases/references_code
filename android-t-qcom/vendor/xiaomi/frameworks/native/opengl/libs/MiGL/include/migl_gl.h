#ifndef __MIGL_HOOKS_H__
#define __MIGL_HOOKS_H__

#include "migl.h"

namespace android {

using ::android::gl_hooks_t;

#define CALL_GL_API(_api, _argList)                                             \
    do {                                                                        \
        gl_hooks_t::gl_t const * const _c = &migl::getThreadSpecific()->gl;     \
        _c->_api _argList;                                                      \
    } while (0)

#define CALL_GL_API_RET(_api, _argList, _ret)                                   \
    do {                                                                        \
        gl_hooks_t::gl_t const * const _c = &migl::getThreadSpecific()->gl;     \
        _ret = _c->_api _argList;                                               \
    } while (0)

gl_hooks_t *getMiGLHooks();
void installMiGLHooks(gl_hooks_t* hooks);

};

#endif