#ifndef ANDROID_SGAME_IMPL_H
#define ANDROID_SGAME_IMPL_H
#include "../FBOBase/FBOBase.h"
#include "../Impl.h"

namespace android {
class SGAMEImpl : public DefaultImplV2, public FboBase {
   private:
    /* data */
   public:
    SGAMEImpl();
    SGAMEImpl(gl_hooks_t *glImpl);
    ~SGAMEImpl();
    virtual void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
    virtual void glBindFramebuffer(GLenum target, GLuint framebuffer);
    virtual void glShaderSource(GLuint shader, GLsizei count,
                                const GLchar *const *string,
                                const GLint *length);
    virtual void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
    virtual void glBindTexture(GLenum target, GLuint texture);
    virtual EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface);
    virtual EGLBoolean eglMakeCurrent(EGLDisplay display, EGLSurface draw,
                                      EGLSurface read, EGLContext context);

   private:
    // bool needRebindToFbo = false;
    bool hasRendertoFbo = false;
    bool needInitialization = true;
    bool needReplacePrecision = false;
    int screenWidth;
    int screenHeight;
    void renderTextureToFbo();
    void renderfboTodisplay(EGLDisplay dpy, EGLSurface surface);
    void Fbosetup(float factor);
    void releaseResourcesAll();
    void checkIfHdrFbo(GLenum target, GLuint framebuffer);
    /*hdr fbo*/
    void createHdrFbo(int width, int hight, int levels, int sample);
    void releaseHdrFbo();
    void FboEnableVRS(bool isNeedQuery = true);
    void FboDisableVRS();
    /*
        struct textureInfo
        {
            int width = 0;
            int hight = 0;
        };
        std::map<int , textureInfo> FboTexInfo;
    */
    GLuint hdrFbo = 0;
    ;
    GLuint hdrTexture = 0;
    GLuint hdrDepthStencil = 0;
    GLuint GameHdrFbo = 0;
    GLuint GameHdrTexture = 0;
    bool hasRendertoHdrFbo = false;
    bool needCreareHdrFbo = true;

    bool checkIfEnableFunc();

   protected:
    int scaleFramebufferWidth;
    int scaleFramebufferHeight;
    float HDRscaleFactor = 0.8;  // default value is 0.8
    float FB0scaleFactor = 0.8;  // default value is 0.8
};
}  // namespace android
#endif
