#define ATRACE_TAG ATRACE_TAG_ALWAYS
#include <utils/Log.h>
#include <cutils/compiler.h>
#include <cutils/properties.h>
#include <GLES2/gl2.h>

#include <compositionengine/Display.h>
#include <compositionengine/OutputLayer.h>
#include <compositionengine/impl/OutputLayerCompositionState.h>

#include "renderengine/Texture.h"
#include "renderengine/Mesh.h"
#include "renderengine/RenderEngine.h"
#include <utils/Trace.h>

#include "SurfaceFlinger.h"
#include "Layer.h"

#include "MiuiBlur.h"
#include "HandyModeForSF.h"
#include "MiuiRenderEngine.h"

namespace android {

class State;

const int DIM_ANIMATION_DURATION = 250; //ms

bool Blur::sHasInitialized = false;
bool Blur::sEnable = true;
bool Blur::sYswap = true;
size_t Blur::sPrevBlurBehindLayerCount = 0;
bool Blur::sBlurInvalidated = false;
bool Blur::sBlurSurfaceContentChanged = false;
uint32_t Blur::sScreenWidth = 0;
uint32_t Blur::sScreenHeight = 0;

Vector< sp<Layer> >* Blur::sBlurBehindLayers = new Vector< sp<Layer> >();
Vector< sp<Layer> >* Blur::sCoveredByBlurLayers = new Vector< sp<Layer> >();
Vector<Layer*>* Blur::sLayersPrevFrame = new Vector<Layer*>();

static std::unique_ptr<MiuiRenderEngine> sBlurRender = nullptr;

static void setupVectors(renderengine::Mesh::VertexArray<vec2>& vectors,
        float x1, float x2, float y1, float y2) {
    vectors[0] = vec2(x1, y1);
    vectors[1] = vec2(x1, y2);
    vectors[2] = vec2(x2, y2);
    vectors[3] = vec2(x2, y1);
}

///////////////////////////////////////////
// class BlurSurface
///////////////////////////////////////////
BlurSurface::BlurSurface(Layer* layer, uint32_t layerWidth, uint32_t layerHeight)
    : mMesh(renderengine::Mesh::TRIANGLE_FAN, 4, 2, 2) {
    (void)(layerWidth);
    (void)(layerHeight);
    mLayer = layer;
    mAnimatingAlpha = 0;

    memset(&mLayerDrawInfo, 0x0, sizeof(LayerObject));

    mVisualState = VISUAL_STATE_HIDED;
    mStepTime = 0;
    mShowHideTime = 0;

    mContentLayers = new Vector< sp<Layer> >();
}

BlurSurface::~BlurSurface() {
    releaseResource();
    if (mCallback != nullptr) {
        delete mCallback;
        mCallback = nullptr;
    }
}

void BlurSurface::releaseResource() {
    if (mContentLayers != nullptr) {
        mContentLayers->clear();
        delete mContentLayers;
        mContentLayers = nullptr;
    }

    if (mBlurContentLayerSettings.size() != 0) {
        mBlurContentLayerSettings.clear();
    }

    std::vector<renderengine::LayerSettings>().swap(mBlurContentLayerSettings);
    sBlurRender->releaseResource(mLayerDrawInfo.mDrawInfo);
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

    ratio = mLayerDrawInfo.mDrawInfo.mBlurRatio * 5.0f;
    if (ratio >1.0f) {
        ratio = 1.0f;
    }

    mAnimatingAlpha = (uint8_t)(alpha * ratio);

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
    if (mBlurAbsoluteCrop.isValid() || Rect::EMPTY_RECT != mBlurRelativeCrop) {
        return false;
    }
    bool isLeftTop = mLayer->getTransform().tx() == 0 && mLayer->getTransform().ty() ==  0;
    if (mLayer->getTransform().getOrientation() == ui::Transform::ROT_INVALID) {
        isLeftTop = true;
    }
    if (isLeftTop) {
        int orientation = hw->getOrientation();
        if (orientation == 0 || orientation == 2) {
            return (s.active_legacy.w >= (uint32_t)(hw->getWidth())) && (s.active_legacy.h >= (uint32_t)(hw->getHeight()));
        } else {
            return (s.active_legacy.w >= (uint32_t)(hw->getHeight())) && (s.active_legacy.h >= (uint32_t)(hw->getWidth()));
        }
    }
    return false;
}

void BlurSurface::draw(const sp<const DisplayDevice>& hw, const sp<SurfaceFlinger>& flinger) const {
    if (!Blur::isEnable()) return;
    if (mLayerDrawInfo.mDrawInfo.mFboTexture.textureId == 0) {
        return;
    }

    renderengine::RenderEngine& engine(flinger->getRenderEngine());

    /*
     * the code below applies the display's inverse transform to the texture transform
     */
    // create a 4x4 transform matrix from the display transform flags
    const mat4 flipH(-1,0,0,0,  0,1,0,0, 0,0,1,0, 1,0,0,1);
    const mat4 flipV( 1,0,0,0, 0,-1,0,0, 0,0,1,0, 0,1,0,1);
    const mat4 rot90( 0,1,0,0, -1,0,0,0, 0,0,1,0, 1,0,0,1);

    mat4 tr;
    uint32_t transform = hw->getPrimaryDisplayOrientationTransform();

    if (transform & NATIVE_WINDOW_TRANSFORM_ROT_90)
        tr = tr * rot90;
    if (transform & NATIVE_WINDOW_TRANSFORM_FLIP_H)
        tr = tr * flipH;
    if (transform & NATIVE_WINDOW_TRANSFORM_FLIP_V)
        tr = tr * flipV;

    // calculate the inverse
    tr = inverse(tr);

    mFboTexture.init(renderengine::Texture::TEXTURE_2D, mLayerDrawInfo.mDrawInfo.mFboTexture.textureId);
    mFboTexture.setDimensions(mLayerDrawInfo.mDrawInfo.mBlurTextureWidth, mLayerDrawInfo.mDrawInfo.mBlurTextureHeight);
    mFboTexture.setFiltering(true);
    mFboTexture.setMatrix(tr.asArray());

    engine.setupLayerTexturing(mFboTexture);
    drawContentWithOpenGL(hw, flinger);
    engine.disableTexturing();
}

void BlurSurface::drawContentWithOpenGL(const sp<const DisplayDevice>& hw, const sp<SurfaceFlinger>& flinger) const {
    const Layer::State& s(mLayer->getDrawingState());

    // compute position
    const ui::Transform tr(hw->getTransform());
    const uint32_t hw_h = hw->getHeight();
    Rect winScreen(hw->getViewport().getWidth(), hw->getViewport().getHeight());
    renderengine::Mesh::VertexArray<vec2> position(mMesh.getPositionArray<vec2>());

    position[0] = tr.transform(winScreen.left,  winScreen.top);
    position[1] = tr.transform(winScreen.left,  winScreen.bottom);
    position[2] = tr.transform(winScreen.right, winScreen.bottom);
    position[3] = tr.transform(winScreen.right, winScreen.top);

    // compute texture coordinates
    const FloatRect winLayer= mLayer->getBounds();

    float left   = float(winLayer.left)   / float(s.active_legacy.w);
    float top    = float(winLayer.top)    / float(s.active_legacy.h);
    float right  = float(winLayer.right)  / float(s.active_legacy.w);
    float bottom = float(winLayer.bottom) / float(s.active_legacy.h);

    renderengine::Mesh::VertexArray<vec2> texCoords(mMesh.getTexCoordArray<vec2>());
    setupVectors(texCoords, left, right, top, bottom);

    const auto outputLayer = mLayer->findOutputLayerForDisplay(hw);
    if (hw->isPrimary() && (outputLayer != nullptr)) {
        const auto& compositionState = outputLayer->getState();
        const Rect& frame = compositionState.displayFrame;

        const uint32_t hw_w = hw->getWidth();
        switch (hw->getOrientation()) {
            case DisplayState::eOrientationDefault:
                // Coordinate origin is left bottom,swap y
                left   = float(frame.left)   / float(hw_w);
                top    = float(frame.top)    / float(hw_h);
                right  = float(frame.right)  / float(hw_w);
                bottom = float(frame.bottom) / float(hw_h);
                break;
            case DisplayState::eOrientation90:
                // Coordinate origin is left top,do not swap x y
                left = float(frame.top)    / float(hw_h);
                top = float(frame.right)   / float(hw_w);
                right = float(frame.bottom) / float(hw_h);
                bottom = float(frame.left)  / float(hw_w);
                setupVectors(texCoords, 1.0f - left, 1.0f - right, top, bottom);
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
                setupVectors(texCoords, left, right, 1.0f - top, 1.0f - bottom);
                break;
        }
    }

    // single finger process
    if (HandyModeForSF::getInstance()->isInHandyMode()) {
        HandyModeForSF::getInstance()->onPostLayerComputeGeometry(NULL, mMesh);
    }

    // draw
    renderengine::RenderEngine& engine(flinger->getRenderEngine());

    const half4 color(s.color);

    if (mCallback != nullptr) {
        mCallback->setupLayerBlending(1.0);
    } else {
        engine.setupLayerBlending(true, mLayer->isOpaque(s), false /* disableTexture */,
                                  half4(color.r, color.g, color.b, 1.0 /*mAnimatingAlpha/255.0f*/), 0);
    }

    // show only layer rect
    bool needScissor = !coverBackgroud(hw);
    bool isScissorEnableBefore;
    int *scissorBox;
    if (needScissor) {
        const ui::Transform tr2(hw->getTransform() * mLayer->getTransform());
        vec2 pos;
        vec2 anotherPos;
        details::TVec2<int> size;

        switch (hw->getOrientation()) {
            case DisplayState::eOrientationDefault:
                if (Blur::isYswap()) {
                    pos = tr2.transform(winLayer.left, winLayer.top);
                    anotherPos = tr2.transform(winLayer.right, winLayer.bottom);
                } else {
                    pos = tr2.transform(winLayer.left, winLayer.bottom);
                    anotherPos = tr2.transform(winLayer.right, winLayer.top);
                }
                break;
            case DisplayState::eOrientation90:
                if (Blur::isYswap()) {
                    pos = tr2.transform(winLayer.left, winLayer.bottom);
                    anotherPos = tr2.transform(winLayer.right, winLayer.top);
                } else {
                    pos = tr2.transform(winLayer.right, winLayer.bottom);
                    anotherPos = tr2.transform(winLayer.left, winLayer.top);
                }
                break;
            case DisplayState::eOrientation180:
                if (Blur::isYswap()) {
                    pos = tr2.transform(winLayer.right, winLayer.bottom);
                    anotherPos = tr2.transform(winLayer.left, winLayer.top);
                } else {
                    pos = tr2.transform(winLayer.right, winLayer.top);
                    anotherPos = tr2.transform(winLayer.left, winLayer.bottom);
                }
                break;
            case DisplayState::eOrientation270:
                if (Blur::isYswap()) {
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

        if (mBlurAbsoluteCrop.isValid()) {
            pos.x = mBlurAbsoluteCrop.left;
            pos.y = mBlurAbsoluteCrop.top;
            anotherPos.x = mBlurAbsoluteCrop.right;
            anotherPos.y = mBlurAbsoluteCrop.bottom;
        } else if (Rect::EMPTY_RECT != mBlurRelativeCrop) {
            pos.x = pos.x + mBlurRelativeCrop.left;
            pos.y = pos.y + mBlurRelativeCrop.top;
            anotherPos.x = anotherPos.x + mBlurRelativeCrop.right;
            anotherPos.y = anotherPos.y + mBlurRelativeCrop.bottom;
        }
        if (Blur::isYswap()) {
            glScissor(int(pos.x), int(pos.y), int(anotherPos.x-pos.x), int(anotherPos.y-pos.y));
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

void BlurSurface::beginDrawToFbo(const sp<const DisplayDevice>& hw) {
    ATRACE_CALL();
    (void)hw;
    sBlurRender->beginDrawToFbo(mLayerDrawInfo);
}

void BlurSurface::endDrawToFbo(const sp<const DisplayDevice>& hw) {
    ATRACE_CALL();
    sBlurRender->endDrawToFbo(mLayerDrawInfo);
    (void)hw;
}

void BlurSurface::makeFboBlur(const sp<const DisplayDevice>& hw) {
    ATRACE_CALL();
    sBlurRender->makeFboBlur(mLayerDrawInfo);
    sBlurRender->onPostBlur();
    (void)hw;
}

bool BlurSurface::setCrop(Rect relative,Rect abs) {
    mBlurAbsoluteCrop = abs;
    mBlurRelativeCrop = relative;
    return true;
}

bool BlurSurface::computeBlurParam(float blurRatio, int blurMode) {
    uint32_t width = mLayerDrawInfo.mDrawInfo.mBlurTextureWidth;
    uint32_t height = mLayerDrawInfo.mDrawInfo.mBlurTextureHeight;
    int radius = mLayerDrawInfo.mDrawInfo.mBlurRadius;

    mLayerDrawInfo.mDrawInfo.mBlurRatio = blurRatio;
    mLayerDrawInfo.mDrawInfo.mBlurMode = blurMode;

    if (blurRatio > 0 && blurRatio <= 1.0f) {
        mLayerDrawInfo.mDrawInfo.mBlurRadius = (int)(blurRatio * sBlurRender->getMaxBlurRadius());
        mLayerDrawInfo.mDrawInfo.mBlurTextureHeight =
                        Blur::sScreenHeight * (SCALE_STARTTING - blurRatio * SCALE_SPEED);
        if (mLayerDrawInfo.mDrawInfo.mBlurTextureHeight % 2 != 0) {
            mLayerDrawInfo.mDrawInfo.mBlurTextureHeight =
                        mLayerDrawInfo.mDrawInfo.mBlurTextureHeight -
                        mLayerDrawInfo.mDrawInfo.mBlurTextureHeight % 2;
        }
        mLayerDrawInfo.mDrawInfo.mBlurTextureWidth =
                        mLayerDrawInfo.mDrawInfo.mBlurTextureHeight * Blur::sScreenWidth/Blur::sScreenHeight;
        if (mLayerDrawInfo.mDrawInfo.mBlurTextureWidth % 2 != 0) {
            mLayerDrawInfo.mDrawInfo.mBlurTextureWidth =
                        mLayerDrawInfo.mDrawInfo.mBlurTextureWidth -
                        mLayerDrawInfo.mDrawInfo.mBlurTextureWidth % 2;
        }
    } else if (blurRatio < 0.001) {
        mLayerDrawInfo.mDrawInfo.mBlurTextureWidth = Blur::sScreenWidth;
        mLayerDrawInfo.mDrawInfo.mBlurTextureHeight = Blur::sScreenHeight;
        mLayerDrawInfo.mDrawInfo.mBlurRadius = 0;
    }

    if (radius != mLayerDrawInfo.mDrawInfo.mBlurRadius) {
        mLayerDrawInfo.mDrawInfo.mCurrWeightIndex =
                    ((mLayerDrawInfo.mDrawInfo.mBlurRadius - 1) + 1) *
                    ((mLayerDrawInfo.mDrawInfo.mBlurRadius - 1) + 2) / 2;
        mLayerDrawInfo.mDrawInfo.mNextWeightIndex =
                    mLayerDrawInfo.mDrawInfo.mCurrWeightIndex +
                    mLayerDrawInfo.mDrawInfo.mBlurRadius + 1;
    }

    if (width != mLayerDrawInfo.mDrawInfo.mBlurTextureWidth ||
            height != mLayerDrawInfo.mDrawInfo.mBlurTextureHeight) {
        mLayerDrawInfo.mDrawInfo.mFboTexture.width = mLayerDrawInfo.mDrawInfo.mBlurTextureWidth;
        mLayerDrawInfo.mDrawInfo.mFboTexture.height = mLayerDrawInfo.mDrawInfo.mBlurTextureHeight;
        mLayerDrawInfo.mDrawInfo.mTempTexture.width = mLayerDrawInfo.mDrawInfo.mBlurTextureWidth;
        mLayerDrawInfo.mDrawInfo.mTempTexture.height = mLayerDrawInfo.mDrawInfo.mBlurTextureHeight;
        mLayerDrawInfo.mDrawInfo.mChangeTextureSize = true;
        return true;
    }
    if (radius != mLayerDrawInfo.mDrawInfo.mBlurRadius) {
        return true;
    }
    return false;
}

///////////////////////////////////////////
// class Blur
///////////////////////////////////////////
int indexOf(const Vector< sp<Layer> >* vector, const sp<Layer>& layer) {
    const size_t count = vector->size();
    for (size_t i = 0; i < count; ++i) {
        if (vector->itemAt(i) == layer) return i;
    }
    return -1;
}

bool Blur::checkIsContentLayerChanges(const Vector< sp<Layer> >* contentLayer) {
    const size_t contentLayerCount = contentLayer->size();
    if (contentLayerCount == 0) {
        return true;
    }

   for (size_t j = 0; j < contentLayerCount; ++j) {
        if (android_atomic_dec(&(contentLayer->itemAt(j)->mQueuedFramesBackForBlur)) > 0) {
            return true;
        }
    }

   return false;
}

bool Blur::checkIsNeedReBlur(const Vector<sp<Layer>>& layers) {
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
            if (layers[i]->blurSurface != NULL) {
                if (layers[i]->getTransform().getOrientation() == ui::Transform::ROT_INVALID) {
                    return true;
                }
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

    if (sBlurSurfaceContentChanged) {
        sBlurSurfaceContentChanged = false;
        return true;
    }
    return false;
}

void Blur::initialize(float density, bool unused, uint32_t screenWidth, uint32_t screenHeight) {
    (void)(unused);
    if (!sHasInitialized) {
        sHasInitialized = true;
    } else {
        return;
    }
    sScreenWidth = screenWidth;
    sScreenHeight = screenHeight;
    sBlurRender = MiuiRenderEngine::create(density, screenWidth, screenHeight);
}

void Blur::release() {
    if (sHasInitialized) {
        if (sBlurBehindLayers != nullptr) {
            sBlurBehindLayers->clear();
            delete sBlurBehindLayers;
            sBlurBehindLayers = nullptr;
        }

        if (sCoveredByBlurLayers != nullptr) {
            sCoveredByBlurLayers->clear();
            delete sCoveredByBlurLayers;
            sCoveredByBlurLayers = nullptr;
        }

        if (sLayersPrevFrame != nullptr) {
            sLayersPrevFrame->clear();
            delete sLayersPrevFrame;
            sLayersPrevFrame = nullptr;
        }

        if (sBlurRender != nullptr) {
            sBlurRender = nullptr;
        }
        sHasInitialized = false;
    }
}

bool Blur::onDoComposeSurfaces(const sp<const DisplayDevice>& hw, const sp<SurfaceFlinger>& flinger,
                                const renderengine::DisplaySettings& clientCompositionDisplay, const Vector< sp<Layer> >& layers) {
    bool needInvalidate = false;

    std::vector<renderengine::LayerSettings> clientCompositionLayers;
    Vector<sp<Layer>> contentLayers;
    Vector<sp<Layer>> coveredByBlurLayers;

    if (!isEnable()) return needInvalidate;

    const size_t layerCount = layers.size();
    const int64_t now = nanoseconds_to_milliseconds(systemTime(SYSTEM_TIME_MONOTONIC));

    clearBlurLayer();

    for (size_t i = 0; i < layerCount; ++i) {
        const sp<Layer>& layer(layers[i]);
        layer->coveredByBlur = false;

        const DisplayRenderArea renderArea(hw);
        renderengine::LayerSettings layerSettings;
        Region clearRegion = Region::INVALID_REGION;

        if (layer->blurSurface == NULL  && layer->isVisible()) {
            coveredByBlurLayers.add(layer);
            contentLayers.push_back(layer);
            bool prepared =
                        layer->prepareClientLayer(renderArea, false, clearRegion,
                                                  false, layerSettings);
            if (prepared) {
                clientCompositionLayers.push_back(layerSettings);
            }
        }

        if (layer->blurSurface != NULL && layer->isVisible() &&
                    layer->blurSurface->mVisualState != VISUAL_STATE_HIDED) {
            const Layer::State& state(layer->getDrawingState());
            needInvalidate |= layer->blurSurface->step(now, state.color.a * 255);
            addBlurLayer(layer);

            bool coveredByBlur = layer->blurSurface->coverBackgroud(hw);

            if (layer->getCurrentLayerBlurState()) {
                contentLayers.push_back(layer);
                bool prepared =
                        layer->prepareClientLayer(renderArea, false, clearRegion,
                                                  false, layerSettings);
                if (prepared) {
                    clientCompositionLayers.push_back(layerSettings);
                }
            }

            layer->blurSurface->mBlurContentLayerSettings.clear();
            layer->blurSurface->mBlurContentLayerSettings.assign(clientCompositionLayers.begin(), clientCompositionLayers.end());

            sBlurSurfaceContentChanged |= checkIsContentLayerChanges(layer->blurSurface->mContentLayers);
            layer->blurSurface->mContentLayers->clear();
            layer->blurSurface->mContentLayers->appendVector(contentLayers);

            if (coveredByBlur) {
                clientCompositionLayers.clear();
                contentLayers.clear();
                if (layer->getCurrentLayerBlurState()) {
                    coveredByBlurLayers.add(layer);
                }
                sCoveredByBlurLayers->appendVector(coveredByBlurLayers);
                coveredByBlurLayers.clear();
            }

            if (!layer->getCurrentLayerBlurState()) {
                bool prepared =
                        layer->prepareClientLayer(renderArea, false, clearRegion,
                                                  false, layerSettings);
                if (prepared) {
                    clientCompositionLayers.push_back(layerSettings);
                }
                contentLayers.push_back(layer);
                coveredByBlurLayers.add(layer);
            }
        }
    }

    if (!checkIsNeedReBlur(layers)) {
        return needInvalidate;
    }

    if (sBlurBehindLayers->size() != 0) {
        Vector<sp<Layer>> blurredBackgroundLayers;

        HandyModeForSF::getInstance()->onPreScreenshotRender();

        const size_t blurBehindLayerCount = sBlurBehindLayers->size();
        size_t w = clientCompositionDisplay.physicalDisplay.getWidth();
        size_t h = clientCompositionDisplay.physicalDisplay.getHeight();

        for (size_t i=0; i<blurBehindLayerCount; ++i) {
            flinger->getRenderEngine().setViewportAndProjection(w, h, clientCompositionDisplay.clip, ui::Transform::ROT_0);
            sp<Layer> blurBehindLayer = sBlurBehindLayers->itemAt(i);

            sBlurRender->computeBlurParam(blurBehindLayer->blurSurface->mLayerDrawInfo.mDrawInfo.mBlurRatio);
            blurBehindLayer->blurSurface->beginDrawToFbo(hw);

            // Use an empty fence for the buffer fence, since we just created the buffer so
            // there is no need for synchronization with the GPU.
            base::unique_fd bufferFence;
            base::unique_fd drawFence;

            for (auto& layer : blurredBackgroundLayers) {
                layer->drawBlurLayer(hw);
            }

            flinger->getRenderEngine().drawLayersByBlur(clientCompositionDisplay, blurBehindLayer->blurSurface->mBlurContentLayerSettings,
                                 std::move(bufferFence), &drawFence);

            blurBehindLayer->blurSurface->endDrawToFbo(hw);
            blurBehindLayer->blurSurface->makeFboBlur(hw);

            blurredBackgroundLayers.add(blurBehindLayer);
        }
        HandyModeForSF::getInstance()->onPostScreenshotRender();
    }

    return needInvalidate;
}

void Blur::addBlurLayer(const sp<Layer>& layer) {
    int index = indexOf(sBlurBehindLayers, layer);
    if (index == -1) {
        sBlurBehindLayers->add(layer);
    }
}

void Blur::removeBlurLayer(const sp<Layer>& layer) {
    if (layer->blurSurface != NULL) {
        layer->destroyBlurSurface();
        int index = indexOf(sBlurBehindLayers, layer);
        if (index >= 0) {
            sBlurBehindLayers->removeAt(index);
            if (sBlurBehindLayers->size() > 0) {
                sBlurInvalidated = true;
            }
        }

        /*if ((sCoveredByBlurLayers != NULL) && (indexOf(sCoveredByBlurLayers, layer) >= 0)) {
            sCoveredByBlurLayers->removeAt(indexOf(sCoveredByBlurLayers, layer));
        }*/
    }
}

void Blur::clearBlurLayer(void) {
    if (sBlurBehindLayers != NULL) {
        if (sBlurBehindLayers->size() > 0) {
            sBlurBehindLayers->clear();
        }
    }

    if (sCoveredByBlurLayers != NULL) {
        if (sCoveredByBlurLayers->size() > 0) {
            sCoveredByBlurLayers->clear();
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

bool Blur::isLayerCovered(const sp<Layer>& layer) {
    if (indexOf(sCoveredByBlurLayers, layer) >= 0) {
        if (sBlurBehindLayers->size() > 0 && sEnable) {
            const size_t blurBehindLayerCount = sBlurBehindLayers->size();
            for (size_t i = 0; i < blurBehindLayerCount; ++i) {
                const sp<Layer>& blurBehindLayer((*sBlurBehindLayers)[i]);
                if (blurBehindLayer->blurSurface->mAnimatingAlpha != 255) {
                    return false;
                }
            }
            return true;
        }
        return false;
    } else {
        return false;
    }
}

void Blur::setglViewport(Rect viewport, size_t width, size_t height) {
    glViewport(viewport.left, viewport.top, width, height);
}

void Blur::checkErrors() {
    do {
        // there could be more than one error flag
        GLenum error = glGetError();
        if (error == GL_NO_ERROR) break;
        ALOGE("GL error 0x%04x", int(error));
    } while (true);
}

} /* namespace android */
