
#include <utils/Log.h>
#ifdef SDK_GREATER_THAN_KITKAT
#include <cutils/compiler.h>
#endif
#include <cutils/properties.h>
#include <GLES2/gl2.h>
#include "MiuiBlur.h"

#include "SurfaceFlinger.h"
#include "Layer.h"
#ifdef PLATFORM_VERSION_GREATER_THAN_28
#include <compositionengine/Display.h>
#include <compositionengine/OutputLayer.h>
#include <compositionengine/impl/OutputLayerCompositionState.h>
#include "renderengine/Texture.h"
#include "renderengine/Mesh.h"
#include "renderengine/RenderEngine.h"
#else
#include "RenderEngine/RenderEngine.h"
#include "RenderEngine/Texture.h"
#endif

#include "HandyModeForSF.h"

namespace android {

class State;

const int DIM_ANIMATION_DURATION = 250; //ms

int Blur::sMinification = 0;
int Blur::sBlurRadius = 0;
bool Blur::sUseHighPrecision = false;
bool Blur::sHasInitialized = false;
bool Blur::sEnable = true;
bool Blur::sScreenShot = false;
Vector< sp<Layer> >* Blur::sBlurBehindLayers = new Vector< sp<Layer> >();
size_t Blur::sPrevBlurBehindLayerCount = 0;
Vector<Layer*>* Blur::sLayersPrevFrame = new Vector<Layer*>();
uint32_t Blur::sBlurXProgram = 0;
uint32_t Blur::sBlurYProgram = 0;
uint32_t Blur::sScreenWidth = 0;
uint32_t Blur::sScreenHeight = 0;
uint32_t Blur::sBlurTextureWidth = 0;
bool Blur::sInScreenRotation = false;
uint32_t Blur::sBlurTextureHeight = 0;
bool Blur::sBlurInvalidated = false;
MiddleTexture* Blur::sMiddleTextures = NULL;
int Blur::sMiddleTexturesCount = 0;
int Blur::sSimpleProgramId = 0;
int Blur::sSaturationProgramId = 0;
#ifdef PLATFORM_VERSION_GREATER_THAN_28
renderengine::Mesh Blur::sSimpleMesh(renderengine::Mesh::TRIANGLE_FAN, 4, 2, 2);
#else
Mesh Blur::sSimpleMesh(Mesh::TRIANGLE_FAN, 4, 2, 2);
#endif

enum {
    eAttrLocPosition = 1,
    eAttrLocTexCoords = 2,
};

static GLuint buildShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLchar log[512];
        glGetShaderInfoLog(shader, sizeof(log), 0, &log[0]);
        ALOGE("Error while compiling shader: %s", log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint buildProgram(const char* vertexShaderSrc, const char* fragShaderSrc, bool hasCoordsAttri) {
    GLuint vertexShader = buildShader(vertexShaderSrc, GL_VERTEX_SHADER);
    if (vertexShader == 0) return 0;

    GLuint fragmentShader = buildShader(fragShaderSrc, GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) return 0;

    GLuint programId = glCreateProgram();

    glAttachShader(programId, vertexShader);
    glAttachShader(programId, fragmentShader);

    glBindAttribLocation(programId, eAttrLocPosition, "a_position");
    if (hasCoordsAttri) glBindAttribLocation(programId, eAttrLocTexCoords, "a_texCoords");

    glLinkProgram(programId);
    GLint status;
    glGetProgramiv(programId, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLint infoLen = 0;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            GLchar log[infoLen];
            glGetProgramInfoLog(programId, infoLen, 0, &log[0]);
            ALOGE("buildProgram fail. %s", log);
        }

        glDetachShader(programId, vertexShader);
        glDetachShader(programId, fragmentShader);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        glDeleteProgram(programId);
        return 0;
    }

    glUseProgram(programId);

    return programId;
}

static void bindTexture(uint32_t texName, bool isFilter) {
    glBindTexture(GL_TEXTURE_2D, texName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, isFilter?GL_LINEAR:GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, isFilter?GL_LINEAR:GL_NEAREST);
}

#ifdef PLATFORM_VERSION_GREATER_THAN_28
    static void drawMeshSimple(const renderengine::Mesh& mesh) {
#else
    static void drawMeshSimple(const Mesh& mesh) {
#endif
    glEnableVertexAttribArray(eAttrLocTexCoords);
    glVertexAttribPointer(eAttrLocTexCoords,
            mesh.getTexCoordsSize(),
            GL_FLOAT, GL_FALSE,
            mesh.getByteStride(),
            mesh.getTexCoords());

    glEnableVertexAttribArray(eAttrLocPosition);
    glVertexAttribPointer(eAttrLocPosition,
            mesh.getVertexSize(),
            GL_FLOAT, GL_FALSE,
            mesh.getByteStride(),
            mesh.getPositions());

    glDrawArrays(mesh.getPrimitive(), 0, mesh.getVertexCount());

    glDisableVertexAttribArray(eAttrLocTexCoords);
}

#ifdef PLATFORM_VERSION_GREATER_THAN_28
    static void setupVectors(renderengine::Mesh::VertexArray<vec2>& vectors,
        float x1, float x2, float y1, float y2) {
#else
    static void setupVectors(Mesh::VertexArray<vec2>& vectors,
        float x1, float x2, float y1, float y2) {
#endif
    vectors[0] = vec2(x1, y1);
    vectors[1] = vec2(x1, y2);
    vectors[2] = vec2(x2, y2);
    vectors[3] = vec2(x2, y1);
}

///////////////////////////////////////////
// class BlurSurface
///////////////////////////////////////////

BlurSurface::BlurSurface(Layer* layer, uint32_t layerWidth, uint32_t layerHeight)
#ifdef PLATFORM_VERSION_GREATER_THAN_28
    : mMesh(renderengine::Mesh::TRIANGLE_FAN, 4, 2, 2) {
#else
    : mMesh(Mesh::TRIANGLE_FAN, 4, 2, 2) {
#endif
    (void)(layerWidth);
    (void)(layerHeight);
    mLayer = layer;
    mAnimatingAlpha = 0;
    mFboTextureId = 0;
    mFboName = 0;
    mTempTextureId = 0;
    mBlurRatio = 0.0f;
    mBlurMode = 0;

    mVisualState = VISUAL_STATE_HIDED;
    mStepTime = 0;
    mShowHideTime = 0;

    mContentLayers = new Vector< sp<Layer> >();
}

BlurSurface::~BlurSurface() {
    releaseResource();
    delete mContentLayers;
    if (mCallback != nullptr) {
        delete mCallback;
    }

}

void BlurSurface::releaseResource() {
    mContentLayers->clear();
    if (mFboTextureId != 0) {
        Blur::deleteFbo(mFboName);
        Blur::deleteTexture(mFboTextureId);
        Blur::deleteTexture(mTempTextureId);
        mFboTextureId = 0;
    }
}

void BlurSurface::show(int64_t now) {
    if (mVisualState == VISUAL_STATE_HIDED || mVisualState == VISUAL_STATE_HIDING) {
        mShowHideTime = now;
        mVisualState = VISUAL_STATE_SHOWING;
    }
}

bool BlurSurface::step(int64_t now, uint8_t alpha) {
    mStepTime = now;
    float ratio = 1.0f;
    if (mVisualState == VISUAL_STATE_SHOWING || mVisualState == VISUAL_STATE_HIDING) {
        ratio = ((float)(now - mShowHideTime + 16))/DIM_ANIMATION_DURATION;
        if (ratio > 1) ratio = 1.0f;

        if (mVisualState == VISUAL_STATE_HIDING) {
            ratio = 1.0f - ratio;
        }

        if (mVisualState == VISUAL_STATE_SHOWING && ratio >= 1) {
            mVisualState = VISUAL_STATE_SHOWED;
        }
        if (mVisualState == VISUAL_STATE_HIDING && ratio <= 0) {
            mVisualState = VISUAL_STATE_HIDED;
        }
    }

    ratio = mBlurRatio * 2.0f;
    if (ratio >1.0f) {
        ratio = 1.0f;
    }

#ifdef PLATFORM_VERSION_GREATER_THAN_28
    const size_t blurBehindLayerCount = Blur::blurLayerCount();
    if(blurBehindLayerCount > 1){
        mAnimatingAlpha = 255;
    }else{
        mAnimatingAlpha = (uint8_t)(alpha * ratio);
    }
#else
    mAnimatingAlpha = (uint8_t)(alpha * ratio);
#endif
    return mVisualState == VISUAL_STATE_SHOWING || mVisualState == VISUAL_STATE_HIDING;
}

void BlurSurface::hide(int64_t now) {
    if (mVisualState == VISUAL_STATE_SHOWING || mVisualState == VISUAL_STATE_SHOWED) {
        mShowHideTime = now;
        mVisualState = VISUAL_STATE_HIDING;
    }
}

bool BlurSurface::coverBackgroud(const sp<const DisplayDevice>& hw) const {
    const Layer::State& s(mLayer->getDrawingState());
#ifdef PLATFORM_VERSION_GREATER_THAN_23
    #ifdef PLATFORM_VERSION_GREATER_THAN_27
    bool isLeftTop=mLayer->getTransform().tx() == 0 && mLayer->getTransform().ty() == 0;
    #else
    bool isLeftTop=s.active.transform.tx() == 0 && s.active.transform.ty() == 0;
    #endif
#else
    bool isLeftTop=s.transform.tx() == 0 && s.transform.ty() == 0;
#endif
    if (isLeftTop) {
        int orientation = hw->getOrientation();
        if (orientation == 0 || orientation == 2) {
            return s.active.w >= (uint32_t)hw->getWidth() && s.active.h >= (uint32_t)hw->getHeight();
        } else {
            return s.active.w >= (uint32_t)hw->getHeight() && s.active.h >= (uint32_t)hw->getWidth();
        }
    }
    return false;
}

void BlurSurface::draw(const sp<const DisplayDevice>& hw, const sp<SurfaceFlinger>& flinger) const {
    if (!Blur::isEnable()) return;
    if (mFboTextureId == 0) return;

    #ifdef PLATFORM_VERSION_GREATER_THAN_27
        #ifdef PLATFORM_VERSION_GREATER_THAN_28
        renderengine::RenderEngine& engine(flinger->getRenderEngine());
        #else
        RE::RenderEngine& engine(flinger->getRenderEngine());
        #endif
    #else
    RenderEngine& engine(flinger->getRenderEngine());
    #endif

    /*
     * the code below applies the display's inverse transform to the texture transform
     */

    // create a 4x4 transform matrix from the display transform flags
    const mat4 flipH(-1,0,0,0,  0,1,0,0, 0,0,1,0, 1,0,0,1);
    const mat4 flipV( 1,0,0,0, 0,-1,0,0, 0,0,1,0, 0,1,0,1);
    const mat4 rot90( 0,1,0,0, -1,0,0,0, 0,0,1,0, 1,0,0,1);

    mat4 tr;
    #ifdef PLATFORM_VERSION_GREATER_THAN_23
    uint32_t transform = hw->getPrimaryDisplayOrientationTransform();
    #else
    uint32_t transform = hw->getOrientationTransform();
    #endif
    if (transform & NATIVE_WINDOW_TRANSFORM_ROT_90)
        tr = tr * rot90;
    if (transform & NATIVE_WINDOW_TRANSFORM_FLIP_H)
        tr = tr * flipH;
    if (transform & NATIVE_WINDOW_TRANSFORM_FLIP_V)
        tr = tr * flipV;

    // calculate the inverse
    tr = inverse(tr);

    mFboTexture.setDimensions(Blur::sBlurTextureWidth, Blur::sBlurTextureHeight);
    mFboTexture.setFiltering(true);
    mFboTexture.setMatrix(tr.asArray());

    engine.setupLayerTexturing(mFboTexture);
    drawContentWithOpenGL(hw, flinger);
    engine.disableTexturing();
}

void BlurSurface::drawContentWithOpenGL(const sp<const DisplayDevice>& hw, const sp<SurfaceFlinger>& flinger) const {
    const Layer::State& s(mLayer->getDrawingState());

    // compute position
    #ifdef PLATFORM_VERSION_GREATER_THAN_28
    const ui::Transform tr(hw->getTransform());
    #else
    const Transform tr(hw->getTransform());
    #endif
    const uint32_t hw_h = hw->getHeight();
    Rect winScreen(hw->getViewport().getWidth(), hw->getViewport().getHeight());
#ifdef PLATFORM_VERSION_GREATER_THAN_28
    renderengine::Mesh::VertexArray<vec2> position(mMesh.getPositionArray<vec2>());
#else
    Mesh::VertexArray<vec2> position(mMesh.getPositionArray<vec2>());
#endif
    position[0] = tr.transform(winScreen.left,  winScreen.top);
    position[1] = tr.transform(winScreen.left,  winScreen.bottom);
    position[2] = tr.transform(winScreen.right, winScreen.bottom);
    position[3] = tr.transform(winScreen.right, winScreen.top);
#ifdef PLATFORM_VERSION_GREATER_THAN_28
#else
    for (size_t i=0 ; i<4 ; i++) {
        position[i].y = hw_h - position[i].y;
    }
#endif
    // compute texture coordinates
#ifdef PLATFORM_VERSION_GREATER_THAN_28
    const FloatRect winLayer= mLayer->getBounds();
#else
    const Rect winLayer(mLayer->computeBounds());
#endif

#ifdef PLATFORM_VERSION_GREATER_THAN_28
    float left   = float(winLayer.left)   / float(s.active_legacy.w);
    float top    = float(winLayer.top)    / float(s.active_legacy.h);
    float right  = float(winLayer.right)  / float(s.active_legacy.w);
    float bottom = float(winLayer.bottom) / float(s.active_legacy.h);
#else
    float left   = float(winLayer.left)   / float(s.active.w);
    float top    = float(winLayer.top)    / float(s.active.h);
    float right  = float(winLayer.right)  / float(s.active.w);
    float bottom = float(winLayer.bottom) / float(s.active.h);
#endif
#ifdef PLATFORM_VERSION_GREATER_THAN_28
    renderengine::Mesh::VertexArray<vec2> texCoords(mMesh.getTexCoordArray<vec2>());
    setupVectors(texCoords, left, right, top, bottom);
#else
    Mesh::VertexArray<vec2> texCoords(mMesh.getTexCoordArray<vec2>());
    setupVectors(texCoords, left, right, 1.0f - top, 1.0f - bottom);
#endif

    #ifdef PLATFORM_VERSION_GREATER_THAN_27
        #ifdef PLATFORM_VERSION_GREATER_THAN_28
        const auto outputLayer = mLayer->findOutputLayerForDisplay(hw);
            if (hw->isPrimary() && (outputLayer != nullptr)) {
                const auto& compositionState = outputLayer->getState();
                const Rect& frame = compositionState.displayFrame;
        #else
        // Only supports full screen, no scaling and blurry texture cropping without panning
        auto hwcId = hw->getHwcDisplayId();
        if (hwcId == DisplayDevice::DISPLAY_PRIMARY && mLayer->hasHwcLayer(hwcId)) {
            const LayerBE::HWCInfo& hwcInfo = mLayer->getBE().mHwcLayers.at(hwcId);
            const Rect& frame = hwcInfo.displayFrame;
        #endif
            const uint32_t hw_w = hw->getWidth();
            switch (hw->getOrientation()) {
                case DisplayState::eOrientationDefault:
                    // Coordinate origin is left bottom,swap y
                    left   = float(frame.left)   / float(hw_w);
                    top    = float(frame.top)    / float(hw_h);
                    right  = float(frame.right)  / float(hw_w);
                    bottom = float(frame.bottom) / float(hw_h);
                    #ifdef PLATFORM_VERSION_GREATER_THAN_28
                    #else
                        setupVectors(texCoords, left, right, 1.0f - top, 1.0f - bottom);
                    #endif
                    break;
                case DisplayState::eOrientation90:
                    // Coordinate origin is left top,do not swap x y
                    left = float(frame.top)    / float(hw_h);
                    top = float(frame.right)   / float(hw_w);
                    right = float(frame.bottom) / float(hw_h);
                    bottom = float(frame.left)  / float(hw_w);
                    #ifdef PLATFORM_VERSION_GREATER_THAN_28
                        setupVectors(texCoords, 1.0f - left, 1.0f - right, top, bottom);
                    #else
                        setupVectors(texCoords, left, right, top, bottom);
                    #endif
                    break;
                case DisplayState::eOrientation180:
                    // Coordinate origin is right top,swap x
                    left = float(frame.right)    / float(hw_w);
                    top = float(frame.bottom)   / float(hw_h);
                    right = float(frame.left) / float(hw_w);
                    bottom = float(frame.top)  / float(hw_h);
                    setupVectors(texCoords, 1.0f - left, 1.0f - right, top, bottom);
                    break;
                case DisplayState::eOrientation270:
                    // Coordinate origin is right bottom,swap x y
                    left = float(frame.bottom) / float(hw_h);
                    top = float(frame.left)  / float(hw_w);
                    right = float(frame.top)    / float(hw_h);
                    bottom = float(frame.right)   / float(hw_w);
                    #ifdef PLATFORM_VERSION_GREATER_THAN_28
                        setupVectors(texCoords, left, right, 1.0f - top, 1.0f - bottom);
                    #else
                        setupVectors(texCoords, 1.0f - left, 1.0f - right, 1.0f - top, 1.0f - bottom);
                    #endif
                    break;
            }
        }
    #endif

    // single finger process
    if (HandyModeForSF::getInstance()->isInHandyMode()) {
        HandyModeForSF::getInstance()->onPostLayerComputeGeometry(NULL, mMesh);
    }

    // draw
    #ifdef PLATFORM_VERSION_GREATER_THAN_27
        #ifdef PLATFORM_VERSION_GREATER_THAN_28
        renderengine::RenderEngine& engine(flinger->getRenderEngine());
        #else
        RE::RenderEngine& engine(flinger->getRenderEngine());
        #endif
    #else
    RenderEngine& engine(flinger->getRenderEngine());
    #endif

    #ifdef SDK_GREATER_THAN_KITKAT
    #ifdef PLATFORM_VERSION_GREATER_THAN_27
    const half4 color(s.color);
    #ifdef PLATFORM_VERSION_GREATER_THAN_28
    engine.setupLayerBlending(true, mLayer->isOpaque(s), false /* disableTexture */,
                              half4(color.r, color.g, color.b, mAnimatingAlpha/255.0f), 0);

    #else
        if (mCallback != nullptr) {
            mCallback->setupLayerBlending(mAnimatingAlpha/255.0f);
        } else {
            engine.setupLayerBlending(true, mLayer->isOpaque(s), false /* disableTexture */,
                            half4(color.r, color.g, color.b, mAnimatingAlpha/255.0f));
        }
    #endif
    #else
    #ifdef USE_HWC2
    engine.setupLayerBlending(true, mLayer->isOpaque(s), mAnimatingAlpha/255.0f);
    #else
    engine.setupLayerBlending(true, mLayer->isOpaque(s), mAnimatingAlpha);
    #endif
    #endif
    #else
    engine.setupLayerBlending(true, mLayer->isOpaque(), mAnimatingAlpha);
    #endif

    // show only layer rect
    bool needScissor = !coverBackgroud(hw);
    bool isScissorEnableBefore;
    int *scissorBox;
    if (needScissor) {
#ifdef PLATFORM_VERSION_GREATER_THAN_23
        #ifdef PLATFORM_VERSION_GREATER_THAN_27
            #ifdef PLATFORM_VERSION_GREATER_THAN_28
            const ui::Transform tr2(hw->getTransform() * mLayer->getTransform());
            #else
            const Transform tr2(hw->getTransform() * mLayer->getTransform());
            #endif
        #else
        const Transform tr2(hw->getTransform() * s.active.transform);
        #endif
#else
        const Transform tr2(hw->getTransform() * s.transform);
#endif
        vec2 pos;
        vec2 anotherPos;
#ifdef PLATFORM_VERSION_GREATER_THAN_25
        details::TVec2<int> size;
#else
        tvec2<int> size;
#endif

        switch (hw->getOrientation()) {
            case DisplayState::eOrientationDefault:
                if (Blur::isScreenShot()) {
                    pos = tr2.transform(winLayer.left, winLayer.top);
                    anotherPos = tr2.transform(winLayer.right, winLayer.bottom);
                } else {
                    pos = tr2.transform(winLayer.left, winLayer.bottom);
                    anotherPos = tr2.transform(winLayer.right, winLayer.top);
                }
                break;
            case DisplayState::eOrientation90:
                if (Blur::isScreenShot()) {
                    pos = tr2.transform(winLayer.left, winLayer.bottom);
                    anotherPos = tr2.transform(winLayer.right, winLayer.top);
                } else {
                    pos = tr2.transform(winLayer.right, winLayer.bottom);
                    anotherPos = tr2.transform(winLayer.left, winLayer.top);
                }
                break;
            case DisplayState::eOrientation180:
                if (Blur::isScreenShot()) {
                    pos = tr2.transform(winLayer.right, winLayer.bottom);
                    anotherPos = tr2.transform(winLayer.left, winLayer.top);
                } else {
                    pos = tr2.transform(winLayer.right, winLayer.top);
                    anotherPos = tr2.transform(winLayer.left, winLayer.bottom);
                }
                break;
            case DisplayState::eOrientation270:
                if (Blur::isScreenShot()) {
                    pos = tr2.transform(winLayer.right, winLayer.top);
                    anotherPos = tr2.transform(winLayer.left, winLayer.bottom);
                } else {
                    pos = tr2.transform(winLayer.left, winLayer.top);
                    anotherPos = tr2.transform(winLayer.right, winLayer.bottom);
                }
                break;
        }

        isScissorEnableBefore = glIsEnabled(GL_SCISSOR_TEST);
        if (isScissorEnableBefore) {
            scissorBox = new int[4];
            glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
        } else {
            glEnable(GL_SCISSOR_TEST);
        }

        if (Blur::isScreenShot()) {
            #ifdef PLATFORM_VERSION_GREATER_THAN_28
            glScissor(int(pos.x), int(pos.y), int(anotherPos.x-pos.x), int(anotherPos.y-pos.y));
            #else
            if (!mLayer->getFiltering()) {
                glScissor(int(pos.x), int(pos.y), int(anotherPos.x-pos.x), int(anotherPos.y-pos.y));
            } else {
                glScissor(0, 0, int(anotherPos.x-pos.x), int(anotherPos.y-pos.y));
            }
            #endif
        } else {
            pos.y = hw->getHeight() - pos.y;
            anotherPos.y = hw->getHeight() - anotherPos.y;
            if (HandyModeForSF::getInstance()->isInHandyMode()) {
                size.x = int(anotherPos.x - pos.x);
                size.y = int(anotherPos.y - pos.y);
                HandyModeForSF::getInstance()->onPartBlurScissor(pos, size);
                anotherPos.x = pos.x + size.x;
                anotherPos.y = pos.y + size.y;
            }
            glScissor(int(pos.x), int(pos.y), int(anotherPos.x-pos.x), int(anotherPos.y-pos.y));
        }
    }

    engine.drawMesh(mMesh);
    engine.disableBlending();

    if (needScissor) {
        if (isScissorEnableBefore) {
            glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
            delete[] scissorBox;
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
    }
}

void BlurSurface::beginDrawToFbo(const sp<const DisplayDevice>& hw) const {
    (void)(hw);
    if (mFboTextureId == 0) {
        Blur::genTexture(&mFboTextureId, Blur::sBlurTextureWidth, Blur::sBlurTextureHeight);
        Blur::genTexture(&mTempTextureId, Blur::sBlurTextureWidth, Blur::sBlurTextureHeight);
        Blur::genFbo(&mFboName);
#ifdef PLATFORM_VERSION_GREATER_THAN_28
        mFboTexture.init(renderengine::Texture::TEXTURE_2D, mFboTextureId);
#else
        mFboTexture.init(Texture::TEXTURE_2D, mFboTextureId);
#endif
    }

    uint32_t textureId, textureWidth, textureHeight;
    if (Blur::sMiddleTexturesCount > 0) {
        // draw to biggest middle texture
        textureId = Blur::sMiddleTextures->textureId;
        textureWidth = Blur::sMiddleTextures->width;
        textureHeight = Blur::sMiddleTextures->height;
    } else {
        // draw to FboTexture
        textureId = mFboTextureId;
        textureWidth = Blur::sBlurTextureWidth;
        textureHeight = Blur::sBlurTextureHeight;
    }

    Blur::bindFbo(mFboName);
    Blur::bindTextureToFbo(textureId);

    glViewport(0, 0, textureWidth, textureHeight);

    mContentLayers->clear();
    Blur::clearWithColor(0, 0, 0, 0);
}

void BlurSurface::endDrawToFbo(const sp<const DisplayDevice>& hw, int32_t initFbo) const {
    if (Blur::sMiddleTexturesCount > 0) {
        glUseProgram(Blur::sSimpleProgramId);

        // biggest middle texture draw to smallest middle texture
        MiddleTexture *currMiddleTexture = Blur::sMiddleTextures;
        MiddleTexture *nextMiddleTexture = Blur::sMiddleTextures + 1;
        const int sizeMinusOne = Blur::sMiddleTexturesCount - 1;
        for (int i=0; i<sizeMinusOne; i++) {
            if (nextMiddleTexture->height < Blur::sBlurTextureHeight) {
                break;
            }
            glViewport(0, 0, nextMiddleTexture->width, nextMiddleTexture->height);
            Blur::bindTextureToFbo(nextMiddleTexture->textureId);
            bindTexture(currMiddleTexture->textureId, true);
            drawMeshSimple(Blur::sSimpleMesh);

            currMiddleTexture++;
            nextMiddleTexture++;
        }

        glUseProgram(Blur::sSaturationProgramId);
        int mModeLoc = glGetUniformLocation(Blur::sSaturationProgramId, "mode");
        glUniform1i(mModeLoc, mBlurMode);

        // smallest middle texture draw to FboTexture
        glViewport(0, 0, Blur::sBlurTextureWidth, Blur::sBlurTextureHeight);
        Blur::bindTextureToFbo(mFboTextureId);
        //bindTexture(Blur::sMiddleTextures[Blur::sMiddleTexturesCount-1].textureId, true);
        bindTexture(currMiddleTexture->textureId, true);
        drawMeshSimple(Blur::sSimpleMesh);
    } else {
        // TODO: increase saturation use Blur::sSaturationProgramId
    }
    Blur::bindFbo((uint32_t)initFbo);
    #ifdef PLATFORM_VERSION_GREATER_THAN_28
    (void)hw;
    #else
    hw->setViewportAndProjection();
    #endif
}

void BlurSurface::makeFboBlur(const sp<const DisplayDevice>& hw) const {
    Blur::blur(mFboName, mFboTextureId, mTempTextureId, Blur::sBlurTextureWidth, Blur::sBlurTextureHeight);
    #ifdef PLATFORM_VERSION_GREATER_THAN_28
    (void)hw;
    #else
    hw->setViewportAndProjection();
    #endif
}

bool BlurSurface::computeBlurParam(float blurRatio, int blurMode) {
    mBlurRatio = blurRatio;
    mBlurMode = blurMode;
    return true;
}

///////////////////////////////////////////
// class Blur
///////////////////////////////////////////

void Blur::initialize(float density, bool unused, uint32_t screenWidth, uint32_t screenHeight)
{
    (void)(unused);
    (void)(density);
    if (!sHasInitialized) {
        sHasInitialized = true;
    } else {
        return;
    }
    sScreenWidth = screenWidth;
    sScreenHeight = screenHeight;

    char value[PROPERTY_VALUE_MAX];

    property_get("persist.sys.real_blur_minify", value, "10000");
    sMinification = atoi(value);

    property_get("persist.sys.real_blur_radius", value, "8");
    sBlurRadius = atoi(value);

    property_get("persist.sys.real_blur_sat", value, "20");
    float increaseSaturation = atoi(value)/100.0f;

    property_get("persist.sys.real_blur_black", value, "30");
    float black = atoi(value)/100.0f;

    property_get("persist.sys.real_blur_highp", value, "0");
    sUseHighPrecision = atoi(value) == 1;

    sBlurTextureWidth = sScreenWidth >> SCALE_SHIFT_BIT;
    sBlurTextureHeight = sScreenHeight >> SCALE_SHIFT_BIT;

    sMiddleTexturesCount = 0;
    for (uint32_t n = sScreenHeight/4; n > sBlurTextureHeight; n /= 4) {
        sMiddleTexturesCount++;
    }
    sMiddleTextures = new MiddleTexture[sMiddleTexturesCount];
    uint32_t currHeight = sScreenHeight;
    uint32_t currWidth = sScreenWidth;
    for (int i=0; i<sMiddleTexturesCount; i++) {
        currHeight /= 4;
        currWidth /= 4;
        sMiddleTextures[i].width = currWidth;
        sMiddleTextures[i].height = currHeight;
        Blur::genTexture(&(sMiddleTextures[i].textureId), currWidth, currHeight);
    }
    if (sMiddleTexturesCount > 0) {
        sSimpleProgramId = buildProgram(
                "attribute vec2 a_position;\n"
                "attribute vec2 a_texCoords;\n"
                "varying vec2 v_TexCoord;\n"
                "void main(void) {\n"
                "    v_TexCoord = a_texCoords;\n"
                "    gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);\n"
                "}\n",
                "precision mediump float;\n"
                "uniform sampler2D sampler1;\n"
                "varying vec2 v_TexCoord;\n"
                "void main(void) {\n"
                "    gl_FragColor = texture2D(sampler1, v_TexCoord);\n"
                "}\n",
                true);
    }

    if (sBlurXProgram == 0) {
        String8 strShader;
        Blur::getFragmentShader(true, sBlurRadius, sBlurTextureWidth, strShader);
        sBlurXProgram = buildProgram(
                Blur::getVertexShader(),
                strShader.string(),
                true);

        Blur::getFragmentShader(false, sBlurRadius, sBlurTextureHeight, strShader);
        sBlurYProgram = buildProgram(
                Blur::getVertexShader(),
                strShader.string(),
                true);
    }

    String8 saturationFragmentShader;
    saturationFragmentShader.appendFormat(
                "precision mediump float;\n"
                "uniform sampler2D sampler1;\n"
                "varying vec2 v_TexCoord;\n"
                "uniform int mode;\n"
                "void main(void) {\n"
                "    vec3 colorOri = texture2D(sampler1, v_TexCoord).xyz;\n"
                "    float opaque = %10.9f;\n"
                "    float dodge = %10.9f;\n"
                "    float burn = %10.9f;\n"
                "    if (mode == %d) {\n"
                "        vec3 colorIncrease = vec3("
                "        colorOri.r * %10.9f + colorOri.g * %10.9f + colorOri.b * %10.9f,"
                "        colorOri.r * %10.9f + colorOri.g * %10.9f + colorOri.b * %10.9f,"
                "        colorOri.r * %10.9f + colorOri.g * %10.9f + colorOri.b * %10.9f);\n"
                "        gl_FragColor = vec4(colorIncrease * %10.9f, opaque);\n"
                "    } else if (mode == %d) {\n"
                "        vec3 colorDodge = vec3(dodge, dodge, dodge);\n"
                "        vec3 colorWhite = vec3(opaque, opaque, opaque);\n"
                "        gl_FragColor = vec4(colorDodge * %10.9f + colorWhite * %10.9f + colorOri * %10.9f, 1.0);\n"
                "    } else if (mode == %d) {\n"
                "        vec3 colorIncrease = vec3("
                "        colorOri.r * %10.9f + colorOri.g * %10.9f + colorOri.b * %10.9f,"
                "        colorOri.r * %10.9f + colorOri.g * %10.9f + colorOri.b * %10.9f,"
                "        colorOri.r * %10.9f + colorOri.g * %10.9f + colorOri.b * %10.9f);\n"
                "        vec3 colorWhite = vec3(opaque, opaque, opaque);\n"
                "        gl_FragColor = vec4(colorIncrease * %10.9f + colorWhite * %10.9f, opaque);\n"
                "    } else if (mode == %d) {\n"
                "        vec3 colorBurn = vec3(burn, burn, burn);\n"
                "        gl_FragColor = vec4(colorOri * %10.9f + colorBurn * %10.9f, opaque);\n"
                "    } else {\n"
                "        gl_FragColor = vec4(colorOri, opaque);\n"
                "    }\n"
                "}\n",
                1.0, 0.75, 0.25,
                BLUR_MODE_BLACK_SATURATION, (1.0f + 2.0742f * increaseSaturation), (-1.8282f * increaseSaturation), (-0.246 * increaseSaturation),
                (-0.9258 * increaseSaturation), (1.0f + 1.1718 * increaseSaturation), (-0.246 * increaseSaturation),
                (-0.9258 * increaseSaturation), (-1.8282f * increaseSaturation), (1.0f + 2.754 * increaseSaturation),(1.0-black),
                BLUR_MODE_COLOR_DODGE, 0.1, 0.4, 0.5,
                BLUR_MODE_WHITE_SATURATION, (1.0f + 2.0742f * increaseSaturation), (-1.8282f * increaseSaturation), (-0.246 * increaseSaturation),
                (-0.9258 * increaseSaturation), (1.0f + 1.1718 * increaseSaturation), (-0.246 * increaseSaturation),
                (-0.9258 * increaseSaturation), (-1.8282f * increaseSaturation), (1.0f + 2.754 * increaseSaturation),(1.0-black), black,
                BLUR_MODE_COLOR_BURN, 0.5, 0.1);
    sSaturationProgramId = buildProgram(
            "attribute vec2 a_position;\n"
            "attribute vec2 a_texCoords;\n"
            "varying vec2 v_TexCoord;\n"
            "void main(void) {\n"
            "    v_TexCoord = a_texCoords;\n"
            "    gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);\n"
            "}\n",
            saturationFragmentShader.string(),
            true);

#ifdef PLATFORM_VERSION_GREATER_THAN_28
    renderengine::Mesh::VertexArray<vec2> position(sSimpleMesh.getPositionArray<vec2>());
#else
    Mesh::VertexArray<vec2> position(sSimpleMesh.getPositionArray<vec2>());
#endif
    setupVectors(position, -1.0f, 1.0f, 1.0f, -1.0f);

#ifdef PLATFORM_VERSION_GREATER_THAN_28
    renderengine::Mesh::VertexArray<vec2> texCoords(sSimpleMesh.getTexCoordArray<vec2>());
#else
    Mesh::VertexArray<vec2> texCoords(sSimpleMesh.getTexCoordArray<vec2>());
#endif
    setupVectors(texCoords, 0.0f, 1.0f, 1.0f, 0.0f);

    ALOGI("Blur.initialize. minification:%d, blurRadius:%d, useHighPrecision:%d",
            sMinification, sBlurRadius, sUseHighPrecision);
}

const char* Blur::getVertexShader()
{
    return "attribute vec2 a_position;\n"
            "attribute vec2 a_texCoords;\n"
            "varying vec2 texCoords;\n"
            "void main(void) {\n"
            "    texCoords = a_texCoords;\n"
            "    gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);\n"
            "}\n";
}

void Blur::getFragmentShader(bool isX, int radius, int textureSize, String8 &builder)
{
    if (radius%2 == 1) radius += 1;

    float step = 1.0f/textureSize;

    int factorAmount = 0;
    for (int i = 1; i <= radius; ++i) {
        factorAmount += i;
    }
    factorAmount = (factorAmount * 2) + (radius + 1);
    float factor = 1.0f/factorAmount;

    builder.clear();

    if (Blur::sUseHighPrecision) {
        builder +=
                "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
                "precision highp float;\n"
                "#else\n"
                "precision mediump float;\n"
                "#endif\n";
    } else {
        builder += "precision mediump float;\n";
    }

    builder +=
            "uniform sampler2D sampler;\n"
            "varying vec2 texCoords;\n";

    builder.appendFormat(
            "void main(void) {\n"
            "vec3 sumRgb = texture2D(sampler, vec2(texCoords.x, texCoords.y)).xyz * %10.9f;\n",
        (radius + 1) * factor);

    for (int n = 0; n < 2; ++n) {
        for (int i = 1; i < radius; i += 2) {
            int weight = (radius - i) * 2 + 1;
            float distance = step * (i + ((float)(radius - i))/weight);
            builder.appendFormat(
                isX ?
                    "sumRgb += texture2D(sampler, vec2(texCoords.x %s %10.9f, texCoords.y)).xyz * %10.9f;\n":
                    "sumRgb += texture2D(sampler, vec2(texCoords.x, texCoords.y %s %10.9f)).xyz * %10.9f;\n",
                n == 0 ? "+" : "-",
                distance,
                weight * factor);
        }
    }

    builder +=
        "gl_FragColor = vec4(sumRgb, 1.0);\n"
        "}\n";
}

void Blur::bindTextureToFbo(uint32_t texName) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texName, 0);
}

void Blur::genTexture(uint32_t* texName, uint32_t width, uint32_t height) {
    glGenTextures(1, texName);
    glBindTexture(GL_TEXTURE_2D, *texName);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
}

void Blur::genFbo(uint32_t* fbName) {
    glGenFramebuffers(1, fbName);
}

void Blur::bindFbo(uint32_t fbName) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbName);
}

void Blur::deleteFbo(uint32_t fbName) {
    glDeleteFramebuffers(1, &fbName);
}

void Blur::deleteTexture(uint32_t texName) {
    glDeleteTextures(1, &texName);
}

void Blur::deleteProgram(uint32_t programId, uint32_t vsId, uint32_t fsId) {
    glDeleteProgram(programId);
    glDeleteShader(vsId);
    glDeleteShader(fsId);
}

void Blur::clearWithColor(float red, float green, float blue, float alpha) {
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Blur::blur(uint32_t fbName, uint32_t texName, uint32_t tempTextName, uint32_t width, uint32_t height) {
    int32_t initFbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &initFbo);
    bindFbo(fbName);

    glViewport(0, 0, width, height);

    // make blur by x axis
    glUseProgram(sBlurXProgram);
    bindTextureToFbo(tempTextName);
    bindTexture(texName, true);

    drawMeshSimple(sSimpleMesh);

    // make blur by y axis
    glUseProgram(sBlurYProgram);
    bindTextureToFbo(texName);
    bindTexture(tempTextName, true);

    drawMeshSimple(sSimpleMesh);

    // end
    bindFbo(initFbo);
}

int indexOf(const Vector< sp<Layer> >* vector, const sp<Layer>& layer) {
    const size_t count = vector->size();
    for (size_t i = 0; i < count; ++i) {
        if (vector->itemAt(i) == layer) return i;
    }
    return -1;
}

bool Blur::checkIsNeedReBlur(const Vector< sp<Layer> >& layers) {
    const size_t prevLayerCount = sLayersPrevFrame->size();
    const size_t layerCount = layers.size();

    bool prevLayersNeedRefresh = false;
    if (prevLayerCount != layerCount) {
        prevLayersNeedRefresh = true;
    } else {
        for (size_t i = 0; i < layerCount; ++i) {
            if (layers[i].get() != (*sLayersPrevFrame)[i]) {
                prevLayersNeedRefresh = true;
                break;
            }
        }
    }
    if (prevLayersNeedRefresh) {
        sLayersPrevFrame->clear();
        for (size_t i = 0; i < layerCount; ++i) {
            sLayersPrevFrame->add(layers[i].get());
        }
        return true;
    }

    if (sBlurInvalidated) {
        sBlurInvalidated = false;
        return true;
    }

    const size_t blurBehindLayerCount = sBlurBehindLayers->size();
    if (blurBehindLayerCount != sPrevBlurBehindLayerCount) {
        sPrevBlurBehindLayerCount = blurBehindLayerCount;
        return true;
    }

    for (size_t i = 0; i < blurBehindLayerCount; ++i) {
        const sp<Layer>& blurBehindLayer((*sBlurBehindLayers)[i]);
        const Vector< sp<Layer> >* contentLayers(blurBehindLayer->blurSurface->mContentLayers);
        const size_t contentLayerCount = contentLayers->size();
        if (contentLayerCount == 0) return true;
        for (size_t j = 0; j < contentLayerCount; ++j) {
            if (android_atomic_dec(&(contentLayers->itemAt(j)->mQueuedFramesBackForBlur)) > 0) {
                return true;
            }
        }
    }
    return false;
}

#ifdef PLATFORM_VERSION_GREATER_THAN_28
bool Blur::onDoComposeSurfaces(const sp<const DisplayDevice>& hw, const sp<SurfaceFlinger>& flinger,
    const renderengine::DisplaySettings& clientCompositionDisplay, const Vector< sp<Layer> >& layers) {
#else
bool Blur::onDoComposeSurfaces(const sp<const DisplayDevice>& hw, const Vector< sp<Layer> >& layers) {
#endif
    bool needInvalidate = false;
    if (!isEnable()) return needInvalidate;

    const size_t layerCount = layers.size();
    const int64_t now = nanoseconds_to_milliseconds(systemTime(SYSTEM_TIME_MONOTONIC));

    // blur data prepare
    for (size_t i = 0; i < layerCount; ++i) {
        const sp<Layer>& layer(layers[i]);
        if (layer->blurSurface != NULL && layer->isVisible()) {
            const Layer::State& state(layer->getDrawingState());
            int index = indexOf(sBlurBehindLayers, layer);
            if (index == -1) {
                sBlurBehindLayers->add(layer);
            }
            #ifdef PLATFORM_VERSION_GREATER_THAN_27
            needInvalidate |= layer->blurSurface->step(now, state.color.a * 255);
            #else
            #ifdef USE_HWC2
                needInvalidate |= layer->blurSurface->step(now, state.alpha * 255);
            #else
                needInvalidate |= layer->blurSurface->step(now, state.alpha);
            #endif
            #endif
        }
    }

    if (sBlurBehindLayers->size() != 0) {
        const size_t N = sBlurBehindLayers->size();
        for (int i = N - 1; i >= 0; --i) {
            sp<Layer> layer = sBlurBehindLayers->itemAt(i);
            if (layer->blurSurface == NULL) {
                sBlurBehindLayers->removeAt(i);
            } else if (layer->blurSurface->mVisualState == VISUAL_STATE_HIDED) {
                layer->destroyBlurSurface();
                sBlurBehindLayers->removeAt(i);
            } else if (layer->blurSurface->stepTime() != now) {
                sBlurBehindLayers->removeAt(i);
            }
        }
    }

    if (!checkIsNeedReBlur(layers)) return needInvalidate;

    // fill blur surface and make blur
    if (sBlurBehindLayers->size() != 0) {
        for (size_t i = 0; i < layerCount; ++i) {
            const sp<Layer>& layer(layers[i]);
            layer->coveredByBlur = false;
        }

        HandyModeForSF::getInstance()->onPreScreenshotRender();

        int32_t initFbo;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &initFbo);
        const size_t blurBehindLayerCount = sBlurBehindLayers->size();
        for (size_t i=0; i<blurBehindLayerCount; ++i) {
            sp<Layer> blurBehindLayer = sBlurBehindLayers->itemAt(i);
            #ifdef PLATFORM_VERSION_GREATER_THAN_28
            size_t w = clientCompositionDisplay.physicalDisplay.getWidth();
            size_t h = clientCompositionDisplay.physicalDisplay.getHeight();
            flinger->getRenderEngine().setViewportAndProjection(w, h, clientCompositionDisplay.clip, ui::Transform::ROT_0);
            #endif
            blurBehindLayer->blurSurface->beginDrawToFbo(hw);
            #ifdef PLATFORM_VERSION_GREATER_THAN_28
            std::vector<renderengine::LayerSettings> clientCompositionLayers;
            #endif
            for (size_t j=0; j<layerCount; ++j) {
                const sp<Layer>& layer(layers[j]);
                if (isBlurLayerVisibleChild(layer)) continue;
                if (layer == blurBehindLayer) break;

                if (!layer->coveredByBlur && layer->isVisible()) {
                    layer->coveredByBlur = blurBehindLayer->blurSurface->coverBackgroud(hw);
                    blurBehindLayer->blurSurface->mContentLayers->add(layer);
                    #ifdef PLATFORM_VERSION_GREATER_THAN_27
                        const DisplayRenderArea renderArea(hw);
                        #ifdef PLATFORM_VERSION_GREATER_THAN_28
                        Region clearRegion = Region::INVALID_REGION;

                        renderengine::LayerSettings layerSettings;
                        bool prepared =
                            layer->prepareClientLayer(renderArea, false, clearRegion,
                                                      false, layerSettings);
                        if (prepared) {
                            clientCompositionLayers.push_back(layerSettings);
                        }
                        #else
                        layer->draw(renderArea);
                        #endif
                    #else
                        layer->draw(hw);
                    #endif
                }
            }
            #ifdef PLATFORM_VERSION_GREATER_THAN_28
            Blur::clearWithColor(0.0f, 0, 0, 0.0f);
            // Use an empty fence for the buffer fence, since we just created the buffer so
            // there is no need for synchronization with the GPU.
            base::unique_fd bufferFence;
            base::unique_fd drawFence;
            if(blurBehindLayerCount > 1 && i == (blurBehindLayerCount-1)){
                sp<Layer> needBlurLayer = sBlurBehindLayers->itemAt(i-1);
                needBlurLayer->drawBlurLayer(hw);
            }
            flinger->getRenderEngine().drawLayersByBlur(clientCompositionDisplay, clientCompositionLayers,
                                 std::move(bufferFence), &drawFence);
            #endif
            blurBehindLayer->blurSurface->endDrawToFbo(hw, initFbo);
            blurBehindLayer->blurSurface->makeFboBlur(hw);
        }

        HandyModeForSF::getInstance()->onPostScreenshotRender();
    }

    return needInvalidate;
}

bool Blur::isLayerCovered(const sp<Layer>& layer) {
    if (sBlurBehindLayers->size() > 0 && layer->coveredByBlur && sEnable) {
        const size_t blurBehindLayerCount = sBlurBehindLayers->size();
        for (size_t i = 0; i < blurBehindLayerCount; ++i) {
            const sp<Layer>& blurBehindLayer((*sBlurBehindLayers)[i]);
            if (blurBehindLayer->blurSurface->mAnimatingAlpha != 255) return false;
        }
        return true;
    }
    return false;
}

void Blur::addBlurLayer(const sp<Layer>& layer) {
    int index = indexOf(sBlurBehindLayers, layer);
    if (index == -1) {
        sBlurBehindLayers->add(layer);
    }
}

void Blur::removeBlurLayer(const sp<Layer>& layer) {
    int index = indexOf(sBlurBehindLayers, layer);
    if (index >= 0) {
        layer->destroyBlurSurface();
        sBlurBehindLayers->removeAt(index);
        if (sBlurBehindLayers->size() > 0) {
            sBlurInvalidated = true;
        }
    }
}

bool Blur::isBlurLayerVisibleChild(const sp<const Layer>& layer) {
    if (hasBlurLayer() && strstr(layer->getName().string(), "SurfaceView") != 0) {
        const auto& p = layer->getParent();
        if (p != nullptr && isLayerBlur(p)) {
            return true;
        }
    }
    return false;
}

#ifdef PLATFORM_VERSION_GREATER_THAN_28
void Blur::setglViewport(Rect viewport, size_t width, size_t height){
    glViewport(viewport.left, viewport.top, width, height);
}
#endif

#ifdef PLATFORM_VERSION_GREATER_THAN_28
void Blur::checkErrors() {
    do {
        // there could be more than one error flag
        GLenum error = glGetError();
        if (error == GL_NO_ERROR) break;
        ALOGE("GL error 0x%04x", int(error));
    } while (true);
}
#endif

} /* namespace android */
