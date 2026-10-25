
#include "SGAMEImpl.h"
#include "MonitorThread.h"

namespace android {
SGAMEImpl::SGAMEImpl() {
    HDRscaleFactor = 0.70;
    FB0scaleFactor = 0.75;
};
SGAMEImpl::SGAMEImpl(gl_hooks_t *glImpl) : DefaultImplV2(glImpl) {
    HDRscaleFactor = 0.70;
    FB0scaleFactor = 0.75;
}
SGAMEImpl::~SGAMEImpl() { releaseResourcesAll(); }

bool SGAMEImpl::checkIfEnableFunc() {
    if (gameInfo[INTERNALENABLE]) {
        HDRscaleFactor = (float)gameInfo[GAMEINTERNALFBOFACTOR] / 100;
        FB0scaleFactor = (float)gameInfo[FBOFACTOR] / 100;
        return true;
    }

    return false;
}

EGLBoolean SGAMEImpl::eglMakeCurrent(EGLDisplay display, EGLSurface draw,
                                     EGLSurface read, EGLContext context) {
    releaseResourcesAll();
    return getEGLPlatformImpl()->eglMakeCurrent(display, draw, read, context);
}
EGLBoolean SGAMEImpl::eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    renderfboTodisplay(dpy, surface);
    bool ret = getEGLPlatformImpl()->eglSwapBuffers(dpy, surface);
    //if (std::stoi(base::GetProperty("vendor.debug.egl.fbo", "0")))
    if(checkIfEnableFunc())
    {
        MIDEBUGV("start to render to fbo");
        renderTextureToFbo();
    } else {
        if (!needInitialization) {
            releaseResourcesAll();
            getGLDriverImpl()->gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            getGLDriverImpl()->gl.glViewport(
                0, 0, screenWidth,
                screenHeight);  // we need to restore the size of the fb0
            getGLDriverImpl()->gl.glScissor(0, 0, screenWidth, screenHeight);
            // cap GL_SCISSOR_TEST
        }
    }
    return ret;
}
void SGAMEImpl::glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    if ((hasRendertoFbo || hasRendertoHdrFbo) &&
        width > scaleFramebufferHeight) {
        // MIDEBUGI("hasRendertoFbo=%d,  hasRendertoHdrFbo =%d, size(%d
        // %d)",hasRendertoFbo, hasRendertoHdrFbo, (int)(width * HDRscaleFactor),
        // (int)(height * HDRscaleFactor));
        if (hasRendertoFbo)
            getGLDriverImpl()->gl.glViewport(
                x * FB0scaleFactor, y * FB0scaleFactor, width * FB0scaleFactor,
                height * FB0scaleFactor);
        else
            getGLDriverImpl()->gl.glViewport(
                x * HDRscaleFactor, y * HDRscaleFactor, width * HDRscaleFactor,
                height * HDRscaleFactor);
    } else {
        // MIDEBUGI("glViewport =(%d %d)",width, height);
        getGLDriverImpl()->gl.glViewport(x, y, width, height);

        if (std::stoi(base::GetProperty("vendor.debug.resize_view", "0"))) {
            getGLDriverImpl()->gl.glViewport(x, y, width * HDRscaleFactor,
                                             height * HDRscaleFactor);
        } else
            getGLDriverImpl()->gl.glViewport(x, y, width, height);
    }
}
void SGAMEImpl::glBindTexture(GLenum target, GLuint texture) {
    if ((!needInitialization) && (texture == GameHdrTexture) &&
        (!needCreareHdrFbo)) {
//#ifdef USE_AHARDWAREBUFFER
#if 0  // oes buffer was not be supported afbc/ubwc format, we should use
       // internal tmp fbo.
        getGLDriverImpl()->gl.glBindTexture(GL_TEXTURE_EXTERNAL_OES, hdrTexture);
#else
        getGLDriverImpl()->gl.glBindTexture(target, hdrTexture);
#endif
        MIDEBUGV("rebind HDR texture =(%d --> %d)", texture, hdrTexture);
    } else {
        getGLDriverImpl()->gl.glBindTexture(target, texture);
    }
}
void SGAMEImpl::glBindFramebuffer(GLenum target, GLuint framebuffer) {
    hasRendertoHdrFbo = false;
    if ((!needInitialization) && (framebuffer == 0)) {
        MIDEBUGV("start to render to fbo");
        renderTextureToFbo();
    } else {
        getGLDriverImpl()->gl.glBindFramebuffer(target, framebuffer);
        hasRendertoFbo = false;
        if ((!needInitialization) && needCreareHdrFbo) {
            checkIfHdrFbo(target, framebuffer);
        }
    }
    if (!needCreareHdrFbo && (!!hdrFbo) && (framebuffer == GameHdrFbo) &&
        (!hasRendertoHdrFbo)) {
        getGLDriverImpl()->gl.glBindFramebuffer(target, hdrFbo);
        hasRendertoHdrFbo = true;
        getGLDriverImpl()->gl.glViewport(0, 0, scaleFramebufferWidth,
                                         scaleFramebufferHeight);
        FboEnableVRS(false);
    } else {
        FboDisableVRS();
    }
    //  if(hasRendertoHdrFbo)
    //      MIDEBUGI("start to render to HDR fbo");
}
void SGAMEImpl::checkIfHdrFbo(GLenum target, GLuint framebuffer) {
    int sample;
    int obj;
    int tex;
    int format;
    int blueSize, alphaSize, depthSize, StencilSize;
    // MIDEBUGI("checkIfHdrFbo");
    getGLDriverImpl()->gl.glGetFramebufferAttachmentParameteriv(
        target, GL_DEPTH_STENCIL_ATTACHMENT,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &obj);
    if (obj == GL_INVALID_ENUM || obj == GL_INVALID_OPERATION || obj == GL_NONE)
        return;
    getGLDriverImpl()->gl.glGetFramebufferAttachmentParameteriv(
        target, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
        &obj);
    if (obj == GL_TEXTURE)
        getGLDriverImpl()->gl.glGetFramebufferAttachmentParameteriv(
            target, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
            &tex);
    else
        return;
    getGLDriverImpl()->gl.glGetFramebufferAttachmentParameteriv(
        target, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,
        &blueSize);
    getGLDriverImpl()->gl.glGetFramebufferAttachmentParameteriv(
        target, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE,
        &alphaSize);
    getGLDriverImpl()->gl.glGetFramebufferAttachmentParameteriv(
        target, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,
        &depthSize);
    getGLDriverImpl()->gl.glGetFramebufferAttachmentParameteriv(
        target, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
        &StencilSize);
    getGLDriverImpl()->gl.glGetIntegerv(GL_SAMPLE_BUFFERS, &sample);
    // MIDEBUGI(" (%d  %d %d %d) for fd(%d) obj=%x  sample=%d name=%d ",
    // blueSize, alphaSize,depthSize, StencilSize,framebuffer,obj,sample,tex);
    if (blueSize & alphaSize & depthSize & StencilSize) {
        scaleFramebufferWidth = screenWidth * HDRscaleFactor;
        scaleFramebufferHeight = screenHeight * HDRscaleFactor;
        createHdrFbo(scaleFramebufferWidth, scaleFramebufferHeight, 1, sample);
        GameHdrFbo = framebuffer;
        GameHdrTexture = tex;
        MIDEBUGI(
            " (%d  %d %d %d) for fd(%d) obj=%x  sample=%d name=%d "
            "HDRscaleFactor=%f",
            blueSize, alphaSize, depthSize, StencilSize, framebuffer, obj,
            sample, tex, HDRscaleFactor);
        getGLDriverImpl()->gl.glBindFramebuffer(target, hdrFbo);
        hasRendertoHdrFbo = true;
        getGLDriverImpl()->gl.glViewport(0, 0, scaleFramebufferWidth,
                                         scaleFramebufferHeight);
        FboEnableVRS(true);  // need query if the game enable the vrs extension
    }
}
void SGAMEImpl::glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    if ((hasRendertoFbo || hasRendertoHdrFbo) &&
        width > scaleFramebufferHeight) {
        if (hasRendertoFbo)
            getGLDriverImpl()->gl.glScissor(
                x * FB0scaleFactor, y * FB0scaleFactor, width * FB0scaleFactor,
                height * FB0scaleFactor);
        else
            getGLDriverImpl()->gl.glScissor(
                x * HDRscaleFactor, y * HDRscaleFactor, width * HDRscaleFactor,
                height * HDRscaleFactor);
    } else {
        getGLDriverImpl()->gl.glScissor(x, y, width, height);
    }
}
void SGAMEImpl::glShaderSource(GLuint shader, GLsizei count,
                               const GLchar *const *string,
                               const GLint *length) {
    // using namespace std;
    std::string keyString = "precision highp";
    if (needReplacePrecision && strstr(*string, keyString.c_str())) {
        std::string replaceString = "precision mediump";
        std::string customizedBuf(*string);
        int position = 0;
        while ((position = customizedBuf.find(keyString, position)) !=
               std::string::npos) {
            customizedBuf.replace(position, keyString.length(), replaceString);
            position++;
        }
        const GLchar *p = customizedBuf.c_str();
        if (length) {
            int newLength =
                *length + replaceString.length() - keyString.length();
            getGLDriverImpl()->gl.glShaderSource(shader, count, &p, &newLength);
            MIDEBUGI("source shader : %s \n length=%d count =%d", *string,
                     *length, count);
        } else {
            getGLDriverImpl()->gl.glShaderSource(shader, count, &p, nullptr);
            MIDEBUGI("source shader : %s \n length=null count =%d", *string,
                     count);
        }
        MIDEBUGI("customized shader : %s", customizedBuf.c_str());
    } else {
        // MIDEBUGI("customized shader : %s",*string);
        getGLDriverImpl()->gl.glShaderSource(shader, count, string, length);
    }
}
void SGAMEImpl::renderTextureToFbo() {
    if (needInitialization) {
        Fbosetup(FB0scaleFactor);
        needInitialization = false;
    }
    FboBase::renderTextureToFbo();
    hasRendertoFbo = true;
}
void SGAMEImpl::renderfboTodisplay(EGLDisplay dpy, EGLSurface surface) {
    if (!hasRendertoFbo) {
        MIDEBUGV("skip draw fbo image.");
        return;
    }
    FboBase::renderfboTodisplay(dpy, surface, screenWidth, screenHeight);
    hasRendertoFbo = false;
}
void SGAMEImpl::Fbosetup(float factor) {
    getEGLPlatformImpl()->eglQuerySurface(
        getEGLPlatformImpl()->eglGetCurrentDisplay(),
        getEGLPlatformImpl()->eglGetCurrentSurface(EGL_DRAW), EGL_WIDTH,
        &screenWidth);
    getEGLPlatformImpl()->eglQuerySurface(
        getEGLPlatformImpl()->eglGetCurrentDisplay(),
        getEGLPlatformImpl()->eglGetCurrentSurface(EGL_DRAW), EGL_HEIGHT,
        &screenHeight);
    FboBase::Fbosetup(factor, getGLDriverImpl(), getEGLPlatformImpl(),
                      screenWidth, screenHeight);
}
void SGAMEImpl::releaseResourcesAll() {
    // shauishuai: clear up the logical flags
    if (!needInitialization)  // that means we have alread enabled this function
    {
        hasRendertoFbo = false;
        needInitialization = true;
        releaseHdrFbo();
        FboBase::releaseResourcesAll();
    }
}
void SGAMEImpl::createHdrFbo(int width, int hight, int levels, int sample) {
    if (needCreareHdrFbo) {
        if (sample)
            FboBase::createFbogMultisampleEXT(
                hdrFbo, hdrTexture, hdrDepthStencil, width, hight, levels);
        else
            FboBase::createFBO(hdrFbo, hdrTexture, hdrDepthStencil, width,
                               hight, levels);
        needCreareHdrFbo = false;
    }
}
void SGAMEImpl::releaseHdrFbo() {
    if (!needCreareHdrFbo) {
        FboBase::releaseFBO(hdrFbo, hdrTexture, hdrDepthStencil);
        needCreareHdrFbo = true;
        hdrFbo = 0;
        hdrTexture = 0;
        hdrDepthStencil = 0;
        GameHdrFbo = 0;
        GameHdrTexture = 0;
    }
}
// VRS setting
void SGAMEImpl::FboEnableVRS(bool isNeedQuery) {
    FboBase::FboEnableVRS(isNeedQuery);
}
void SGAMEImpl::FboDisableVRS() { FboBase::FboDisableVRS(); }
}  // namespace android
