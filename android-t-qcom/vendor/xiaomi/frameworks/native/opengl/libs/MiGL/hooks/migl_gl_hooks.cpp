#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <cutils/log.h>

#include "migl_gl.h"

#include "migl_modify_params.h"

using namespace android::migl;

namespace android {

static void hookGlViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    modifyRenderPrecisionYuanshen(width, height, 0);
    CALL_GL_API(glViewport, (x,y,width,height));
}

static void hookGlTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat,
        GLsizei width, GLsizei height) {
    modifyRenderPrecisionYuanshen(width, height, 1);
    CALL_GL_API(glTexStorage2D, (target,levels,internalformat,width,height));
}

static void hookGlTexParameteri(GLenum target, GLenum pname, GLint param) {
    modifyAFValuePubg(pname, param);
    CALL_GL_API(glTexParameteri, (target, pname, param));
}

static void hookGlDrawArrays(GLenum mode, GLint first, GLsizei count) {
    modifyShadingRate();
    CALL_GL_API(glDrawArrays, (mode, first, count));
}

static void hookGlBindTexture(GLenum target, GLuint texture) {
    updateTexSizeArr();
    CALL_GL_API(glBindTexture, (target,texture));
}


static void hookGlDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) {
    modifyShadingRate();

    checkBossPattern();
    updateEleCountArr(count);

    CALL_GL_API(glDrawElements, (mode, count, type, indices));
}

// MIGL_ENTRY_VOID(void, glScissor, (GLint x, GLint y, GLsizei width, GLsizei height), (x,y,width,height))
static void hookGlScissor(GLint x, GLint y, GLsizei width, GLsizei height){
    modifyRenderPrecisionYuanshen(width, height, 0);
    CALL_GL_API(glScissor, (x,y,width,height));
}
// MIGL_ENTRY_VOID(void, glBlitFramebuffer, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter), (srcX0,srcY0,srcX1,srcY1,dstX0,dstY0,dstX1,dstY1,mask,filter))
static void hookGlBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    modifyRenderPrecisionYuanshen(srcX1, srcY1, 0);
    modifyRenderPrecisionYuanshen(dstX1, dstY1, 0);
    CALL_GL_API(glBlitFramebuffer, (srcX0,srcY0,srcX1,srcY1,dstX0,dstY0,dstX1,dstY1,mask,filter));
}

// MIGL_ENTRY_VOID(void, glUniform4fv, (GLint location, GLsizei count, const GLfloat *value), (location,count,value))
static void hookGlUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
    if (count == 1) {
        GLfloat newValue[] = { value[0], value[1], value[2], value[3] };
        modifyRenderPrecisionYuanshenShaderUniform(count, newValue);
        CALL_GL_API(glUniform4fv, (location,count,newValue));
        return;
    }
    CALL_GL_API(glUniform4fv, (location,count,value));
}

void installMiGLHooks(gl_hooks_t* hooks) {
    hooks->gl.glTexParameteri = hookGlTexParameteri;
    hooks->gl.glTexStorage2D = hookGlTexStorage2D;
    hooks->gl.glViewport = hookGlViewport;
    hooks->gl.glDrawArrays = hookGlDrawArrays;
    hooks->gl.glBindTexture = hookGlBindTexture;
    hooks->gl.glDrawElements = hookGlDrawElements;
    hooks->gl.glScissor = hookGlScissor;
    hooks->gl.glBlitFramebuffer = hookGlBlitFramebuffer;
    hooks->gl.glUniform4fv = hookGlUniform4fv;
}

};
