#ifndef ANDROID_FBOBASE_H
#define ANDROID_FBOBASE_H
#include <android-base/properties.h>
#include <log/log.h>
#include <string.h>
#include <map>
#include "EGL/egldefs.h"
//#include "GLES3/gl3extQCOM.h"
#include "hooks.h"
//#include <ui/GraphicBuffer.h>
#include <GLES2/gl2.h>
#include <android/hardware_buffer.h>
#include <math.h>
#include <GLES/glext.h>

//#define USE_AHARDWAREBUFFER
#pragma clang diagnostic ignored "-Wunused-private-field"
#define MIDEBUG

#undef LOG_TAG
#define LOG_TAG "EGL_HOOK"

#define MIDEBUGD(fmt, ...) \
    ALOGD("[%s][%d][%s]: " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define MIDEBUGI(fmt, ...) \
    ALOGI("[%s][%d][%s]: " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define MIDEBUGW(fmt, ...) \
    ALOGW("[%s][%d][%s]: " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define MIDEBUGE(fmt, ...) \
    ALOGE("[%s][%d][%s]: " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define MIDEBUGV(fmt, ...) \
    ALOGV("[%s][%d][%s]: " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#ifdef MIDEBUG
#define checkGlError()                                    \
    if (glApi)                                            \
        for (GLint error = glApi->gl.glGetError(); error; \
             error = glApi->gl.glGetError()) {            \
            MIDEBUGE("glError (0x%x)\n", error);          \
        }
#define checkEglError()                                          \
    for (EGLint error = gEGLImpl.default_platform.eglGetError(); \
         error != EGL_SUCCESS;                                   \
         error = gEGLImpl.default_platform.eglGetError()) {      \
        MIDEBUGE("eglError 0x%x\n", error);                      \
    }
#else
#define checkGlError()
#define checkEglError()
#endif

#ifndef GL_QCOM_BINNING_CONTROL2
#define GL_QCOM_BINNING_CONTROL2 1
#define GL_BINNING_CONTROL_HINT_QCOM 0x8FB0
#define GL_BINNING_QCOM 0x8FB1
#define GL_VISIBILITY_OPTIMIZED_BINNING_QCOM 0x8FB2
#define GL_RENDER_DIRECT_TO_FRAMEBUFFER_QCOM 0x8FB3
#endif

namespace android {
class FboBase {
   public:
    /*
    FboBase(float factor = 0.8,
            const gl_hooks_t  *gl = nullptr,
            const platform_impl_t* egl= nullptr,
            EGLDisplay dpy = nullptr ,
            EGLSurface suf = nullptr) :
            glApi(gl), eglApi(egl), scaleFactor(factor), display(dpy),
    surface(suf) { MIDEBUGI("FboBase");};
    */
    FboBase() { MIDEBUGI("FboBase"); };
    ~FboBase();
    void renderTextureToFbo();
    void renderfboTodisplay(EGLDisplay dpy, EGLSurface surface, int screenWidth,
                            int screenHeight);
    // setup some parameters, it is needed to be called once when enable this
    // function
    void Fbosetup(float factor, const gl_hooks_t *gl,
                  const platform_impl_t *egl, int width, int height);
    void releaseResourcesAll();
    void releaseFBO(GLuint &fboHandle, GLuint &textureHandle,
                    GLuint &depthStencilHandle);
    void createFBO(GLuint &fboHandle, GLuint &textureHandle,
                   GLuint &depthStencilHandle, int width, int hight,
                   int levels);
    void createFbogMultisampleEXT(GLuint &fboHandle, GLuint &textureHandle,
                                  GLuint &depthStencilHandle, int width,
                                  int hight, int levels);
    void FboEnableVRS(bool isNeedQuery = true);
    void FboDisableVRS();

   private:
    const gl_hooks_t *glApi;
    const platform_impl_t *eglApi;
    GLuint ScaleFbo;
    GLuint ScaleTexture;
    GLuint fboRbo;
    GLuint FboProgram;
    GLint fboPositionHandle;
    GLint fboTexCoordsHandle;
    GLint fboTexSamplerHandle;
    GLuint FboVBO;
    GLuint vertexShader;
    GLuint pixelShader;
    int scaleFramebufferWidth = 1716;
    int scaleFramebufferHeight = 772;
    float scaleFactor = 0.8;  // default value is 0.8
    bool fbo_init = false;
    bool render_init = false;
    static constexpr char gVertexShader[] =
        "attribute vec4 fboPosition;\n"
        "attribute vec2 fboTexCoords;\n"
        "varying vec2 texCoords;\n"
        "void main() {\n"
        "  texCoords = fboTexCoords.xy;\n"
        "  gl_Position = fboPosition;\n"
        "}\n";
#ifdef USE_AHARDWAREBUFFER
    static constexpr char gFragmentShader[] =
        "#extension GL_OES_EGL_image_external : require\n"
        "precision mediump float;\n"
        "uniform samplerExternalOES fboTexSampler;\n"
        "varying vec2 texCoords;\n"
        "void main() {\n"
        "  gl_FragColor = texture2D(fboTexSampler, texCoords);\n"
        "}\n";
#else
    static constexpr char gFragmentShader[] =
        "precision mediump float;\n"
        "uniform sampler2D fboTexSampler;\n"
        "varying vec2 texCoords;\n"
        "void main() {\n"
        "  gl_FragColor = texture2D(fboTexSampler, texCoords);\n"
        "}\n";
#endif
    /*****************
    top    left
    buttom left
    buttom right
    top    right
    *****************/
    static constexpr GLfloat fboFullScreenVertices[] = {
        -1.0f, 1.0f,  0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f,  -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f, 1.0f,
    };
    struct blendFuncSeparate {
        int srcRgb;
        int srcAlpha;
        int dstRgb;
        int dstAlpha;
    };
    struct blendColorMask {
        int mask[4];  // the mask of R G B A
    };
    struct viewSize {
        int size[4];  // the x and y window coordinates of the viewport,
                      // followed by its width and height.
    };
    struct platformExt {
        bool isEnableShadingRate = false;
        int shadingRateQCOM = 0;
    };
    // state machine
    struct FSM {
        // for blending
        blendFuncSeparate blendSeparate;
        blendColorMask ColorMask;
        // end blend;
        viewSize view;
        unsigned char isEnableBlend;
        unsigned char isEnableStencil;
        unsigned char isEnableDepth;
        unsigned char isEnableScissor;
        bool isReset;  // the hint to check if these parameters is already set
        // platform special
        platformExt ext;
    } ParametersState;
    GLuint loadShader(GLenum shaderType, const char *pSource);
    GLuint createProgram(const char *pVertexSource,
                         const char *pFragmentSource);
    bool setupGraphics();
    void collectParametersState();
    void reinstallParametersState() const;
#ifdef USE_AHARDWAREBUFFER
   private:
    EGLImageKHR EGLImage = nullptr;
    AHardwareBuffer *hwbuffer = nullptr;
    int createImageByAHardwareBuffer(EGLImageKHR *khrImage, uint width,
                                     uint height, AHardwareBuffer *&buffer);
    int dumpHardwareBuffer(int times, AHardwareBuffer *buffer);
    struct FBOBufferInfo {
        EGLImageKHR EGLImage = nullptr;
        AHardwareBuffer *hwbuffer = nullptr;
    };
    std::map<int, FBOBufferInfo> fboBufferMap;
#endif
   protected:
    EGLDisplay display;
    EGLSurface surface;
    bool isEnableQCOM_binning_control = false;
    bool isEnableQCOM_shading_rate = false;
    void queryGlFunctions();
};  // class FboBase
}  // namespace android
#endif
