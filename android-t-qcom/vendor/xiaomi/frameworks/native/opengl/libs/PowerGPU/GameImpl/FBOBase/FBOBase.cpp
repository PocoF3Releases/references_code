#include "FBOBase.h"
#include <GLES/glext.h>
#include <stdlib.h>
namespace android {
using namespace std;
FboBase::~FboBase() {
    MIDEBUGI("~FboBase");
    releaseResourcesAll();
}
void FboBase::Fbosetup(float factor, const gl_hooks_t *gl,
                       const platform_impl_t *egl, int width, int height) {
    glApi = gl;
    eglApi = egl;
    scaleFactor = factor;
    scaleFramebufferWidth = width * scaleFactor;
    scaleFramebufferHeight = height * scaleFactor;
    MIDEBUGI("fbo factor=%f width=%d height=%d ", factor, scaleFramebufferWidth,
             scaleFramebufferHeight);
}
void FboBase::releaseFBO(GLuint &fboHandle, GLuint &textureHandle,
                         GLuint &depthStencilHandle) {
    glApi->gl.glDeleteTextures(1, &depthStencilHandle);
    checkGlError();
    glApi->gl.glDeleteRenderbuffers(1, &depthStencilHandle);
    checkGlError();
    glApi->gl.glDeleteTextures(1, &textureHandle);
    checkGlError();
    glApi->gl.glDeleteFramebuffers(1, &fboHandle);
    checkGlError();
}
void FboBase::createFbogMultisampleEXT(GLuint &fboHandle, GLuint &textureHandle,
                                       GLuint &depthStencilHandle, int width,
                                       int height, int levels) {
    // clear the pervious gl errors..just in case....
    // glApi->gl.glGetError();
    checkGlError();
    glApi->gl.glGenFramebuffers(1, &fboHandle);
    checkGlError();
    glApi->gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboHandle);
    // glApi->gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ScaleFbo);
    checkGlError();
    /*attach color buffer*/
    glApi->gl.glGenTextures(1, &textureHandle);
    checkGlError();
//#ifdef USE_AHARDWAREBUFFER
#if 0
    struct FBOBufferInfo HardwareInfo;
    MIDEBUGI("start to create a khr imamge");
    createImageByAHardwareBuffer(&HardwareInfo.EGLImage, width, height, HardwareInfo.hwbuffer);
    glApi->gl.glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureHandle);
    checkGlError();
    glApi->gl.glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, HardwareInfo.EGLImage);
    checkGlError();
   // glApi->gl.glTexStorage2D(GL_TEXTURE_2D, levels, GL_RGBA8, width, height);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_BASE_LEVEL, 0);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAX_LEVEL, 0);
    checkGlError();
    glApi->gl.glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_EXTERNAL_OES, textureHandle, 0, 4);
    checkGlError();
    glApi->gl.glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    checkGlError();
#else
    glApi->gl.glBindTexture(GL_TEXTURE_2D, textureHandle);
    checkGlError();
    glApi->gl.glTexStorage2D(GL_TEXTURE_2D, levels, GL_RGB8, width, height);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                              GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                              GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R,
                              GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT,
                              GL_DECODE_EXT);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    checkGlError();
    glApi->gl.glFramebufferTexture2DMultisampleEXT(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureHandle, 0,
        4);
    checkGlError();
    glApi->gl.glBindTexture(GL_TEXTURE_2D, 0);
    checkGlError();
#endif
    /*depth and stencial buffer*/
    glApi->gl.glGenRenderbuffers(1, &depthStencilHandle);
    checkGlError();
    glApi->gl.glBindRenderbuffer(GL_RENDERBUFFER, depthStencilHandle);
    checkGlError();
    glApi->gl.glRenderbufferStorageMultisampleEXT(
        GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);
    checkGlError();
    glApi->gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                        GL_RENDERBUFFER, depthStencilHandle);
    checkGlError();
    glApi->gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                        GL_RENDERBUFFER, depthStencilHandle);
    checkGlError();
    glApi->gl.glBindTexture(GL_TEXTURE_2D, 0);
    checkGlError();
    glApi->gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    checkGlError();
    MIDEBUGI(
        "create new fbo(%d) with multisample done, texture=%d depthStencil=%d "
        "w=%d h=%d levels=%d format=0x%x.",
        fboHandle, textureHandle, depthStencilHandle, width, height, levels,
        GL_RGB8);
}
void FboBase::createFBO(GLuint &fboHandle, GLuint &textureHandle,
                        GLuint &depthStencilHandle, int width, int height,
                        int levels) {
    // clear the pervious gl errors..just in case....
    // glApi->gl.glGetError();
    checkGlError();
    glApi->gl.glGenFramebuffers(1, &fboHandle);
    checkGlError();
    glApi->gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboHandle);
    // glApi->gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ScaleFbo);
    checkGlError();
    /*attach color buffer*/
    glApi->gl.glGenTextures(1, &textureHandle);
    checkGlError();
//#ifdef USE_AHARDWAREBUFFER
#if 0
    struct FBOBufferInfo HardwareInfo;
    MIDEBUGI("start to create a khr imamge");
    createImageByAHardwareBuffer(&HardwareInfo.EGLImage, width, height, HardwareInfo.hwbuffer);
    glApi->gl.glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureHandle);
    checkGlError();
    glApi->gl.glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, HardwareInfo.EGLImage);
    checkGlError();
   // glApi->gl.glTexStorage2D(GL_TEXTURE_2D, levels, GL_RGBA8, width, height);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT);
    checkGlError();
   // glApi->gl.glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_EXTERNAL_OES, textureHandle, 0, 4);
    glApi->gl.glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_EXTERNAL_OES, textureHandle, 0);
    checkGlError();
    glApi->gl.glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    checkGlError();
#else
    glApi->gl.glBindTexture(GL_TEXTURE_2D, textureHandle);
    checkGlError();
    glApi->gl.glTexStorage2D(GL_TEXTURE_2D, levels, GL_RGB8, width, height);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                              GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                              GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R,
                              GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT,
                              GL_DECODE_EXT);
    checkGlError();
    glApi->gl.glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, textureHandle, 0);
    checkGlError();
    glApi->gl.glBindTexture(GL_TEXTURE_2D, 0);
    checkGlError();
#endif
    /*depth and stencial buffer*/
    glApi->gl.glGenTextures(1, &depthStencilHandle);
    checkGlError();
    glApi->gl.glBindTexture(GL_TEXTURE_2D, depthStencilHandle);
    checkGlError();
    glApi->gl.glTexStorage2D(GL_TEXTURE_2D, levels, GL_DEPTH24_STENCIL8, width,
                             height);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                              GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                              GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R,
                              GL_CLAMP_TO_EDGE);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT,
                              GL_DECODE_EXT);
    checkGlError();
    glApi->gl.glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                     GL_TEXTURE_2D, depthStencilHandle, 0);
    checkGlError();
    glApi->gl.glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                     GL_TEXTURE_2D, depthStencilHandle, 0);
    checkGlError();
    glApi->gl.glBindTexture(GL_TEXTURE_2D, 0);
    checkGlError();
    glApi->gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    checkGlError();
    MIDEBUGI(
        "create new fbo(%d) done, texture=%d depthStencil=%d w=%d h=%d "
        "levels=%d.",
        fboHandle, textureHandle, depthStencilHandle, width, height, levels);
}
void FboBase::renderTextureToFbo() {
    // clear the pervious gl errors..just in case....
    // glApi->gl.glGetError();
    checkGlError();
    if (fbo_init) {
        MIDEBUGV("skip init!");
        goto REBIND;
    }
    // check the platform extensions
    queryGlFunctions();
#ifdef USE_AHARDWAREBUFFER
    if (!EGLImage)  // first time here
    {
        MIDEBUGI("start to create a khr imamge");
        createImageByAHardwareBuffer(&EGLImage, scaleFramebufferWidth,
                                     scaleFramebufferHeight, hwbuffer);
    }
#endif
    glApi->gl.glGenFramebuffers(1, &ScaleFbo);
    checkGlError();
    MIDEBUGI("create fbo(%d) done.", ScaleFbo);
    glApi->gl.glBindFramebuffer(GL_FRAMEBUFFER, ScaleFbo);
    // glApi->gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ScaleFbo);
    checkGlError();
    MIDEBUGI("get current window size [0 0 %d %d] ", scaleFramebufferWidth,
             scaleFramebufferHeight);
    /*attach color buffer*/
    glApi->gl.glGenTextures(1, &ScaleTexture);
    checkGlError();
    // glApi->gl.glActiveTexture(GL_TEXTURE0);
    checkGlError();
#ifdef USE_AHARDWAREBUFFER
    glApi->gl.glBindTexture(GL_TEXTURE_EXTERNAL_OES, ScaleTexture);
    checkGlError();
    glApi->gl.glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, EGLImage);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER,
                              GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER,
                              GL_LINEAR);
    checkGlError();
    glApi->gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_EXTERNAL_OES, ScaleTexture, 0);
    checkGlError();
    glApi->gl.glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    checkGlError();
#else  // use internal texture clor buffer
    glApi->gl.glBindTexture(GL_TEXTURE_2D, ScaleTexture);
    checkGlError();
    glApi->gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, scaleFramebufferWidth,
                           scaleFramebufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE,
                           NULL);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_TEXTURE_2D, ScaleTexture, 0);
    checkGlError();
    glApi->gl.glBindTexture(GL_TEXTURE_2D, 0);
    checkGlError();
#endif
    /*attach depth and stencil buffer*/
    glApi->gl.glGenRenderbuffers(1, &fboRbo);
    checkGlError();
    MIDEBUGI("create a new rbo target(%d).", fboRbo);
    glApi->gl.glBindRenderbuffer(GL_RENDERBUFFER, fboRbo);
    checkGlError();
    // glApi->gl.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
    // scaleFramebufferWidth, scaleFramebufferHeight);
    glApi->gl.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                                    scaleFramebufferWidth,
                                    scaleFramebufferHeight);
    checkGlError();
    // glApi->gl.glBindRenderbuffer(GL_RENDERBUFFER, 0);
    // checkGlError();
    glApi->gl.glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fboRbo);
    checkGlError();
    fbo_init = true;
REBIND:
    // attach color buufer  to fbo
    glApi->gl.glBindFramebuffer(GL_FRAMEBUFFER, ScaleFbo);
    checkGlError();
    uint attachments[] = {GL_DEPTH_STENCIL_ATTACHMENT};
    glApi->gl.glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, attachments);
    checkGlError() glApi->gl.glClearColor(0, 0, 0, 0);
    checkGlError();
    glApi->gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                      GL_STENCIL_BUFFER_BIT);
    checkGlError();
#ifdef MIDEBUG
    if (glApi->gl.glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
        GL_FRAMEBUFFER_COMPLETE) {
        MIDEBUGE("framebuffer is not complete!");
    }
#endif
    // queryFramebufferParameter();
    glApi->gl.glViewport(0, 0, scaleFramebufferWidth, scaleFramebufferHeight);
    // MIDEBUGI("glViewport =(%d %d)",scaleFramebufferWidth,
    // scaleFramebufferHeight);
    checkGlError();
}
GLuint FboBase::loadShader(GLenum shaderType, const char *pSource) {
    GLuint shader = glApi->gl.glCreateShader(shaderType);
    checkGlError();
    if (shader) {
        glApi->gl.glShaderSource(shader, 1, &pSource, NULL);
        checkGlError();
        glApi->gl.glCompileShader(shader);
        checkGlError();
        GLint compiled = 0;
        glApi->gl.glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        checkGlError();
        if (!compiled) {
            GLint infoLen = 0;
            glApi->gl.glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            checkGlError();
            if (infoLen) {
                char *buf = (char *)malloc(infoLen);
                if (buf) {
                    glApi->gl.glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    MIDEBUGE("Could not compile shader %d:\n%s\n", shaderType,
                             buf);
                    free(buf);
                }
            } else {
                MIDEBUGE("Guessing at GL_INFO_LOG_LENGTH size\n");
                char *buf = (char *)malloc(0x1000);
                if (buf) {
                    glApi->gl.glGetShaderInfoLog(shader, 0x1000, NULL, buf);
                    MIDEBUGE("Could not compile shader %d:\n%s\n", shaderType,
                             buf);
                    free(buf);
                }
            }
            glApi->gl.glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}
GLuint FboBase::createProgram(const char *pVertexSource,
                              const char *pFragmentSource) {
    vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }
    pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }
    GLuint program = glApi->gl.glCreateProgram();
    if (program) {
        glApi->gl.glAttachShader(program, vertexShader);
        checkGlError();
        glApi->gl.glAttachShader(program, pixelShader);
        checkGlError();
        glApi->gl.glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glApi->gl.glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glApi->gl.glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char *buf = (char *)malloc(bufLength);
                if (buf) {
                    glApi->gl.glGetProgramInfoLog(program, bufLength, NULL,
                                                  buf);
                    MIDEBUGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glApi->gl.glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}
bool FboBase::setupGraphics() {
    // GLint preProgram;
    //  glApi->gl.glGetIntegerv(GL_CURRENT_PROGRAM,&preProgram);
    // checkGlError();
    FboProgram = createProgram(gVertexShader, gFragmentShader);
    if (!FboProgram) {
        MIDEBUGE("failed to createProgram.");
        return false;
    }
    glApi->gl.glUseProgram(FboProgram);
    checkGlError();
    MIDEBUGI("create fbo programe(%d) done.", FboProgram);
    fboPositionHandle =
        glApi->gl.glGetAttribLocation(FboProgram, "fboPosition");
    checkGlError();
    MIDEBUGI("glGetAttribLocation(\"fboPositionHandle\") = %d\n",
             fboPositionHandle);
    fboTexCoordsHandle =
        glApi->gl.glGetAttribLocation(FboProgram, "fboTexCoords");
    checkGlError();
    MIDEBUGI("glGetAttribLocation(\"fboTexCoordsHandle\") = %d\n",
             fboTexCoordsHandle);
    fboTexSamplerHandle =
        glApi->gl.glGetUniformLocation(FboProgram, "fboTexSampler");
    checkGlError();
    MIDEBUGI("glGetUniformLocation(\"fboTexSamplerHandle\") = %d\n",
             fboTexSamplerHandle);
    glApi->gl.glGenBuffers(1, &FboVBO);
    checkGlError();
    glApi->gl.glBindBuffer(GL_ARRAY_BUFFER, FboVBO);
    checkGlError();
    glApi->gl.glBufferData(GL_ARRAY_BUFFER, sizeof(fboFullScreenVertices),
                           fboFullScreenVertices, GL_STATIC_DRAW);
    checkGlError();
    //  glApi->gl.glUseProgram(preProgram);
    // checkGlError();
    return true;
}
void FboBase::collectParametersState() {
    // for BlendFuncSeparate
    glApi->gl.glGetIntegerv(GL_BLEND_SRC_RGB,
                            &ParametersState.blendSeparate.srcRgb);
    checkGlError();
    glApi->gl.glGetIntegerv(GL_BLEND_SRC_ALPHA,
                            &ParametersState.blendSeparate.srcAlpha);
    checkGlError();
    glApi->gl.glGetIntegerv(GL_BLEND_DST_RGB,
                            &ParametersState.blendSeparate.dstRgb);
    checkGlError();
    glApi->gl.glGetIntegerv(GL_BLEND_DST_ALPHA,
                            &ParametersState.blendSeparate.dstAlpha);
    checkGlError();
    // for view size
    //   glApi->gl.glGetIntegerv(GL_VIEWPORT,ParametersState.view.size);
    //  checkGlError();
    // for color mask
    glApi->gl.glGetIntegerv(GL_COLOR_WRITEMASK, ParametersState.ColorMask.mask);
    checkGlError();
    // for some gl functions
    glApi->gl.glGetBooleanv(GL_DEPTH_TEST, &ParametersState.isEnableDepth);
    checkGlError();
    glApi->gl.glGetBooleanv(GL_STENCIL_TEST, &ParametersState.isEnableStencil);
    checkGlError();
    glApi->gl.glGetBooleanv(GL_SCISSOR_TEST, &ParametersState.isEnableScissor);
    checkGlError();
    glApi->gl.glGetBooleanv(GL_BLEND, &ParametersState.isEnableBlend);
    checkGlError();

    glApi->gl.glEnable(GL_BINNING_CONTROL_HINT_QCOM);
    checkGlError();

    // glApi->gl.glHint(GL_BINNING_CONTROL_HINT_QCOM, GL_BINNING_QCOM); //  Hint
    // to SW Binning rendering glApi->gl.glHint(GL_BINNING_CONTROL_HINT_QCOM,
    // GL_VISIBILITY_OPTIMIZED_BINNING_QCOM);     // Hint to HW Binning
    // rendering
    glApi->gl.glHint(
        GL_BINNING_CONTROL_HINT_QCOM,
        GL_RENDER_DIRECT_TO_FRAMEBUFFER_QCOM);  // Hint to Direct rendering
    checkGlError();
}
void FboBase::reinstallParametersState() const {
    glApi->gl.glBlendFuncSeparate(ParametersState.blendSeparate.srcRgb,
                                  ParametersState.blendSeparate.dstRgb,
                                  ParametersState.blendSeparate.srcAlpha,
                                  ParametersState.blendSeparate.dstAlpha);
    checkGlError();
    glApi->gl.glColorMask(
        ParametersState.ColorMask.mask[0], ParametersState.ColorMask.mask[1],
        ParametersState.ColorMask.mask[2], ParametersState.ColorMask.mask[3]);
    checkGlError();
    if (ParametersState.isEnableStencil) glApi->gl.glEnable(GL_STENCIL_TEST);
    if (ParametersState.isEnableBlend) glApi->gl.glEnable(GL_BLEND);
    if (ParametersState.isEnableDepth) glApi->gl.glEnable(GL_DEPTH_TEST);
    if (ParametersState.isEnableScissor) glApi->gl.glEnable(GL_SCISSOR_TEST);
    checkGlError();

    glApi->gl.glHint(GL_BINNING_CONTROL_HINT_QCOM, GL_DONT_CARE);  // Default
    checkGlError();
    glApi->gl.glDisable(GL_BINNING_CONTROL_HINT_QCOM);
    checkGlError();
}
void FboBase::renderfboTodisplay(EGLDisplay dpy, EGLSurface surface,
                                 int screenWidth, int screenHeight) {
    // clear the pervious gl errors..just in case....
    glApi->gl.glGetError();
    if (render_init) goto INITDONE;
    if (!setupGraphics()) {
        MIDEBUGE("failed to init fbo texture!");
    } else {
        MIDEBUGI("setupGraphics successed!");
    }
    render_init = true;
INITDONE:
    // glApi->gl.glFinish();
    // dumpHardwareBuffer( 1, hwbuffer);
    // MIDEBUGI("screenWidth=%d screenHeight=%d FboProgram=%d ScaleTexture=%d
    // FboVBO=%d.", screenWidth, screenHeight,FboProgram,ScaleTexture,FboVBO);
    collectParametersState();
    glApi->gl.glUseProgram(FboProgram);
    checkGlError();
    // CheckProgram(FboProgram);
#ifdef USE_AHARDWAREBUFFER
    glApi->gl.glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
#else
    glApi->gl.glBindTexture(GL_TEXTURE_2D, 0);
#endif
    checkGlError();
    glApi->gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // glDeleteFrameBufferEXT(1,&ScaleFbo);
    checkGlError();
    // uint	attachments[] = {GL_COLOR};
    // glApi->gl.glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, attachments);
    // checkGlError();
    glApi->gl.glUniform1i(fboTexSamplerHandle, 0);
    checkGlError();
#ifdef USE_AHARDWAREBUFFER
    glApi->gl.glBindTexture(GL_TEXTURE_EXTERNAL_OES, ScaleTexture);
#else
    glApi->gl.glBindTexture(GL_TEXTURE_2D, ScaleTexture);
#endif
    checkGlError();
    glApi->gl.glActiveTexture(GL_TEXTURE0);
    checkGlError();
#ifdef USE_AHARDWAREBUFFER
    glApi->gl.glBindTexture(GL_TEXTURE_EXTERNAL_OES, ScaleTexture);
#else
    glApi->gl.glBindTexture(GL_TEXTURE_2D, ScaleTexture);
#endif
    checkGlError();
    // MIDEBUGI("glViewport =(%d %d)",screenWidth, screenHeight);
    glApi->gl.glViewport(0, 0, screenWidth, screenHeight);
    checkGlError();
    glApi->gl.glClearColor(0, 0, 0, 0);
    checkGlError();
    glApi->gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                      GL_STENCIL_BUFFER_BIT);
    checkGlError();
    // set color and texture Position
    glApi->gl.glBindBuffer(GL_ARRAY_BUFFER, FboVBO);
    checkGlError();
    glApi->gl.glVertexAttribPointer(fboPositionHandle, 2, GL_FLOAT, GL_FALSE,
                                    4 * sizeof(GLfloat), (void *)0);
    checkGlError();
    glApi->gl.glEnableVertexAttribArray(fboPositionHandle);
    checkGlError();
    glApi->gl.glVertexAttribPointer(fboTexCoordsHandle, 2, GL_FLOAT, GL_FALSE,
                                    4 * sizeof(GLfloat),
                                    (void *)(2 * sizeof(float)));
    checkGlError();
    glApi->gl.glEnableVertexAttribArray(fboTexCoordsHandle);
    checkGlError();
    glApi->gl.glDisable(GL_STENCIL_TEST);
    checkGlError();
    glApi->gl.glDisable(GL_DEPTH_TEST);
    checkGlError();
    glApi->gl.glDisable(GL_BLEND);
    checkGlError();
    glApi->gl.glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
    checkGlError() glApi->gl.glColorMask(1, 1, 1, 0);
    checkGlError();
    glApi->gl.glDisable(GL_BLEND);
    checkGlError();
    glApi->gl.glDisable(GL_SAMPLE_SHADING);
    checkGlError();
    // set render parameteri
    /*
  #ifdef USE_AHARDWAREBUFFER
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER,
  GL_LINEAR); checkGlError(); glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES,
  GL_TEXTURE_MAG_FILTER, GL_LINEAR); checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S,
  GL_CLAMP_TO_EDGE); checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
  GL_CLAMP_TO_EDGE); checkGlError(); #else
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    checkGlError();
    glApi->gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
  GL_CLAMP_TO_EDGE); checkGlError(); glApi->gl.glTexParameteri(GL_TEXTURE_2D,
  GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); checkGlError(); #endif
  */
    glApi->gl.glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    checkGlError();
    // glApi->gl.glDisableVertexAttribArray(fboPositionHandle);
    // checkGlError();
    // glApi->gl.glDisableVertexAttribArray(fboTexCoordsHandle);
    // checkGlError();
    reinstallParametersState();
    // render_init = true;
}
void FboBase::releaseResourcesAll() {
    // Add by shuaishuai, delete resoreces
    if (fbo_init) {
#ifdef USE_AHARDWAREBUFFER
        if (hwbuffer) AHardwareBuffer_release(hwbuffer);
        if (EGLImage) {
            eglApi->eglDestroyImageKHR(eglApi->eglGetCurrentDisplay(),
                                       EGLImage);
            EGLImage = nullptr;
        }
#endif
        MIDEBUGI("start release all of the resources");
        glApi->gl.glDeleteFramebuffers(1, &ScaleFbo);
        checkGlError();
        glApi->gl.glDeleteTextures(1, &ScaleTexture);
        checkGlError();
        glApi->gl.glDeleteRenderbuffers(1, &fboRbo);
        checkGlError();
        fbo_init = false;
    }
    if (render_init)  // that means we have alread enabled this function
    {
        glApi->gl.glDeleteShader(vertexShader);
        checkGlError();
        glApi->gl.glDeleteShader(pixelShader);
        checkGlError();
        glApi->gl.glDeleteProgram(FboProgram);
        checkGlError();
        glApi->gl.glDeleteBuffers(1, &FboVBO);
        checkGlError();
        // reinit some flags
        render_init = false;
    }
}
void FboBase::queryGlFunctions() {
    if (stoi(base::GetProperty("vendor.debug.disable.vrs", "1"))) return;
    // check if the platform support VRS function
    const char *ext = (char *)glApi->gl.glGetString(GL_EXTENSIONS);
    if (strstr(ext, "GL_QCOM_shading_rate") != nullptr) {
        isEnableQCOM_shading_rate = true;
        MIDEBUGI("support GL_QCOM_shading_rate extension");
    }
}
void FboBase::FboEnableVRS(bool isNeedQuery) {
    if (isEnableQCOM_shading_rate && isNeedQuery) {
        //ParametersState.ext.isEnableShadingRate =
        //    glApi->gl.glIsEnabled(GL_PRESERVE_SHADING_RATE_ASPECT_RATIO_QCOM);
        //checkGlError();
        if (ParametersState.ext.isEnableShadingRate) {
        //    glApi->gl.glGetIntegerv(GL_SHADING_RATE_QCOM,
        //                            &ParametersState.ext.shadingRateQCOM);
        //    checkGlError();
        }
    }

    if (isEnableQCOM_shading_rate) {
        //glApi->gl.glEnable(GL_PRESERVE_SHADING_RATE_ASPECT_RATIO_QCOM);
        //checkGlError();
        //glApi->gl.glShadingRateQCOM(GL_SHADING_RATE_1X2_PIXELS_QCOM);
        //checkGlError();
    }
}
void FboBase::FboDisableVRS() {
    if (isEnableQCOM_shading_rate) {
        if (ParametersState.ext.isEnableShadingRate) {
            //glApi->gl.glShadingRateQCOM(ParametersState.ext.shadingRateQCOM);
            //checkGlError();
        } else {
            //glApi->gl.glShadingRateQCOM(GL_SHADING_RATE_1X1_PIXELS_QCOM);
            //checkGlError();
            //glApi->gl.glDisable(GL_PRESERVE_SHADING_RATE_ASPECT_RATIO_QCOM);
            //checkGlError();
        }
    }
}
#ifdef USE_AHARDWAREBUFFER
int FboBase::createImageByAHardwareBuffer(EGLImageKHR *khrImage, uint width,
                                          uint height,
                                          AHardwareBuffer *&buffer) {
    int err;
    EGLint attrs[] = {EGL_NONE};
    AHardwareBuffer_Desc desc{
        .width = width,
        .height = height,
        .layers = 1,
        .format =
            AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,  // AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM,
                                                    // //format,
                                                    // AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN
                                                    // |
                                                    // AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN
                                                    // , //shuaishuai: both of
                                                    // two usages should be
                                                    // removed in the official
                                                    // version
                                                    // AHARDWAREBUFFER_USAGE_VENDOR_0
                                                    // will enbale ubwc for qcom
                                                    // it is equal to
                                                    // GRALLOC_USAGE_PRIVATE_ALLOC_UBWC,
                                                    // but not sure for MALI
                                                    // platform
        .usage = AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER |
                 AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                 AHARDWAREBUFFER_USAGE_VENDOR_0,
        .stride = 1,  // shuaishuai: The 'stride' field will be ignored by
                      // AHardwareBuffer_allocate,
    };
    err = AHardwareBuffer_allocate(&desc, &buffer);
    if (err != 0) {
        MIDEBUGE("failed to allocate AHardwareBuffer, error=%d.", err);
        return err;
    }
    EGLClientBuffer native_buffer =
        eglApi->eglGetNativeClientBufferANDROID(buffer);
    if (!native_buffer) {
        MIDEBUGE("failed to get EGLClientBuffer!");
        goto error;
    }
    *khrImage = eglApi->eglCreateImageKHR(
        eglApi->eglGetCurrentDisplay(), EGL_NO_CONTEXT,
        EGL_NATIVE_BUFFER_ANDROID, native_buffer, attrs);
    if (*khrImage == EGL_NO_IMAGE_KHR) {
        MIDEBUGE("failed to creat egl image!");
        goto error;
    }
    MIDEBUGE("create AHardwareBuffer done. width=%d hight=%d format=0x%x",
             width, height, desc.format);
    return 0;
error:
    if (buffer) AHardwareBuffer_release(buffer);
    return -1;
}
int FboBase::dumpHardwareBuffer(int times, AHardwareBuffer *buffer) {
    FILE *fp = NULL;
    if (!buffer) {
        // MIDEBUGE("hardware buffer handle is invalid!");
        return -1;
    }
    static int dump_times = 0;
    if (std::stoi(base::GetProperty("vendor.debug.egl.fbo.dump", "0")) &&
        (dump_times < times)) {
        AHardwareBuffer_Desc outDesc;
        AHardwareBuffer_describe(buffer, &outDesc);
        int dataSize =
            outDesc.width * outDesc.height * 2;  // shuaishuai: for RGB
        void *data = NULL;
        AHardwareBuffer_lock(buffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1,
                             0, &data);
        using namespace std;
        string fileName = "/data/raw/" + to_string(outDesc.width) + "x" +
                          to_string(outDesc.height) + "_" +
                          to_string(dump_times) + ".raw";
        char *p = (char *)data;
        if ((fp = fopen(fileName.data(), "wb+")) == NULL) {
            MIDEBUGE("Failed to open file!");
        } else {
            int write_length = fwrite(p, 1, dataSize, fp);
            fclose(fp);
            MIDEBUGI("dump image width=%d height=%d write_length =%d %s",
                     outDesc.width, outDesc.height, write_length,
                     fileName.data());
        }
        AHardwareBuffer_unlock(buffer, NULL);
        dump_times++;
    }
    return 0;
}
#endif
}  // namespace android
