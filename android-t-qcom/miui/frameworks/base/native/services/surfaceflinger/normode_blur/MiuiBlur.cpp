#define ATRACE_TAG ATRACE_TAG_ALWAYS
#include <utils/Log.h>
#include <cutils/compiler.h>
#include <cutils/properties.h>
#include <GLES2/gl2.h>

//#include <compositionengine/Display.h>
//#include <compositionengine/OutputLayer.h>
//#include <compositionengine/impl/OutputLayerCompositionState.h>

#include <renderengine/Texture.h>
#include <renderengine/Mesh.h>
#include <renderengine/RenderEngine.h>
#include <utils/Trace.h>

//#include "SurfaceFlinger.h"
//#include "Layer.h"

#include "MiuiBlur.h"
//#include "HandyModeForSF.h"

namespace android {
using namespace renderengine;
using namespace ui;

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

// Vector< sp<renderengine::LayerSettings> >* Blur::sBlurBehindLayerSettings = new Vector< sp<renderengine::LayerSettings> >();
// Vector< sp<Layer> >* Blur::sBlurBehindLayers = new Vector< sp<Layer> >();
// Vector< sp<Layer> >* Blur::sCoveredByBlurLayers = new Vector< sp<Layer> >();
// Vector<Layer*>* Blur::sLayersPrevFrame = new Vector<Layer*>();

static MiuiRenderEngine* sBlurRender = nullptr;

// static void setupVectors(renderengine::Mesh::VertexArray<vec2>& vectors,
//         float x1, float x2, float y1, float y2) {
//     vectors[0] = vec2(x1, y1);
//     vectors[1] = vec2(x1, y2);
//     vectors[2] = vec2(x2, y2);
//     vectors[3] = vec2(x2, y1);
// }

///////////////////////////////////////////
// class BlurSurface
///////////////////////////////////////////
BlurSurface::BlurSurface(/*Layer* layer,*/ uint32_t layerWidth, uint32_t layerHeight)
    : mMesh(Mesh::Builder()
                        .setPrimitive(Mesh::TRIANGLE_FAN)
                        .setVertices(4 /* count */, 2 /* size */)
                        .setTexCoords(2 /* size */)
                        .setCropCoords(2 /* size */)
                        .build()) {
    (void)(layerWidth);
    (void)(layerHeight);
    // mLayer = layer;
    mAnimatingAlpha = 0;

    memset(&mLayerDrawInfo, 0x0, sizeof(LayerObject));

    mVisualState = VISUAL_STATE_HIDED;
    mStepTime = 0;
    mShowHideTime = 0;

    // mContentLayers = new Vector< sp<Layer> >();
    // mSurfaceSettings = new renderengine::MiuiBlurSurfaceSettings();
}

BlurSurface::~BlurSurface() {
    releaseResource();
    if (mCallback != nullptr) {
        delete mCallback;
        mCallback = nullptr;
    }
  /*  if (mSurfaceSettings != nullptr) {
        delete mSurfaceSettings;
        mSurfaceSettings = nullptr;
    }*/
}

void BlurSurface::releaseResource() {
    // if (mContentLayers != nullptr) {
    //     mContentLayers->clear();
    //     delete mContentLayers;
    //     mContentLayers = nullptr;
    // }

    if (mBlurContentLayerSettings.size() != 0) {
        mBlurContentLayerSettings.clear();
    }

    std::vector<LayerSettings>().swap(mBlurContentLayerSettings);
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

void BlurSurface::setupBlurCurrentState(bool mBlurCurrent) {
    mBlurCurrentLayer = mBlurCurrent;
}

bool BlurSurface::coverBackgroud(const sp<const DisplayDevice>& hw) const {
    (void)hw;
/*    const Layer::State& s(mLayer->getDrawingState());
    if (mBlurAbsoluteCrop.isValid() || Rect::EMPTY_RECT != mBlurRelativeCrop) {
        return false;
    }
    bool isLeftTop = mLayer->getTransform().tx() == 0 && mLayer->getTransform().ty() ==  0;
    if (mLayer->getTransform().getOrientation() == ui::Transform::ROT_INVALID) {
        isLeftTop = true;
    }
    if (isLeftTop) {
        ui::Rotation rot = hw->getOrientation();
        if (rot == ui::ROTATION_0 || rot == ui::ROTATION_180) {  // need check
            return (s.active_legacy.w >= (uint32_t)(hw->getWidth())) && (s.active_legacy.h >= (uint32_t)(hw->getHeight()));
        } else {
            return (s.active_legacy.w >= (uint32_t)(hw->getHeight())) && (s.active_legacy.h >= (uint32_t)(hw->getWidth()));
        }
    }*/
    return false;
}

void BlurSurface::draw(const renderengine::DisplaySettings& display,
                        renderengine::RenderEngine* renderEngine,
                        const renderengine::LayerSettings* layer){

    if (!Blur::isEnable()) return;
    if (mLayerDrawInfo.mDrawInfo.mFboTexture.textureId == 0) {
        return;
    }

    mFboTexture.init(renderengine::Texture::TEXTURE_2D, mLayerDrawInfo.mDrawInfo.mFboTexture.textureId);
    mFboTexture.setDimensions(mLayerDrawInfo.mDrawInfo.mBlurTextureWidth, mLayerDrawInfo.mDrawInfo.mBlurTextureHeight);
    mFboTexture.setFiltering(true);
    if (layer->source.buffer.buffer != nullptr) {
        mat4 texMatrix = layer->source.buffer.textureTransform;
        mFboTexture.setMatrix(texMatrix.asArray());
    }

    renderEngine->setupLayerTexturing(mFboTexture);
    drawContentWithOpenGL(display, renderEngine, layer);
    renderEngine->disableTexturing();

}

void BlurSurface::drawContentWithOpenGL(const renderengine::DisplaySettings& display,
                                        renderengine::RenderEngine* renderengine,
                                         const renderengine::LayerSettings* layer) {

    // compute position
    Rect displayBounds = display.clip;
    renderengine::Mesh::VertexArray<vec2> position(mMesh.getPositionArray<vec2>());
    position[0] = vec2(displayBounds.left, displayBounds.top);
    position[1] = vec2(displayBounds.left, displayBounds.bottom);
    position[2] = vec2(displayBounds.right, displayBounds.bottom);
    position[3] = vec2(displayBounds.right, displayBounds.top);

    // compute texture coordinates
    // const FloatRect winLayer = layer->geometry.boundaries;

    renderengine::Mesh::VertexArray<vec2> texCoords(mMesh.getTexCoordArray<vec2>());
    texCoords[0] = vec2(0.0, 0.0);
    texCoords[1] = vec2(0.0, 1.0);
    texCoords[2] = vec2(1.0, 1.0);
    texCoords[3] = vec2(1.0, 0.0);

    const half3 solidColor = layer->source.solidColor;
    const half4 color = half4(solidColor.r, solidColor.g, solidColor.b, layer->alpha);
    bool isOpaque = false;
    if (layer->source.buffer.buffer != nullptr) {
        isOpaque = layer->source.buffer.isOpaque;
    }

    renderengine->setupLayerBlending(true, isOpaque, false /* disableTexture */,
                              half4(color.r, color.g, color.b, 1.0 /*mAnimatingAlpha/255.0f*/), 0);

    renderengine->drawMesh(mMesh);
    renderengine->disableBlending();


}

void BlurSurface::beginDrawToFbo() {
    ATRACE_CALL();
    sBlurRender->beginDrawToFbo(mLayerDrawInfo);
}

void BlurSurface::endDrawToFbo() {
    ATRACE_CALL();
    sBlurRender->endDrawToFbo(mLayerDrawInfo);
}

void BlurSurface::makeFboBlur() {
    ATRACE_CALL();
    sBlurRender->makeFboBlur(mLayerDrawInfo);
    sBlurRender->onPostBlur();
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
                        (uint32_t)(Blur::sScreenHeight * (SCALE_STARTTING - blurRatio * SCALE_SPEED));
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
int indexOf(/*const Vector< sp<Layer> >* vector,  const sp<Layer>& layer*/) {

/*    const size_t count = vector->size();
    for (size_t i = 0; i < count; ++i) {
        if (vector->itemAt(i) == layer) return (int)i;
    }*/
    return -1;
}

bool Blur::checkIsContentLayerChanges(/*const Vector< sp<Layer> >* contentLayer*/) {
    // (void)contentLayer;
/*    const size_t contentLayerCount = contentLayer->size();
    if (contentLayerCount == 0) {
        return true;
    }

   for (size_t j = 0; j < contentLayerCount; ++j) {
        if (android_atomic_dec(&(contentLayer->itemAt(j)->mQueuedFramesBackForBlur)) > 0) {
            return true;
        }
    }*/

   return false;
}

bool Blur::checkIsNeedReBlur(/*const Vector<sp<Layer>>& layers*/) {
    // (void)layers;
    return true;
    /*
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
    */
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
     /*   if (sBlurBehindLayerSettings != nullptr) {
            sBlurBehindLayerSettings->clear();
            delete sBlurBehindLayerSettings;
            sBlurBehindLayerSettings = nullptr;
        }*/

        /*if (sCoveredByBlurLayers != nullptr) {
            sCoveredByBlurLayers->clear();
            delete sCoveredByBlurLayers;
            sCoveredByBlurLayers = nullptr;
        }*/

       /* if (sLayersPrevFrame != nullptr) {
            sLayersPrevFrame->clear();
            delete sLayersPrevFrame;
            sLayersPrevFrame = nullptr;
        }*/

        if (sBlurRender != nullptr) {
            delete sBlurRender;
            sBlurRender = nullptr;
        }
        sHasInitialized = false;
    }
}


bool Blur::onDoComposeSurfaces(renderengine::RenderEngine* renderengine,
                               const renderengine::DisplaySettings& clientCompositionDisplay,
                               std::vector<const renderengine::LayerSettings *> layerSettings) {

    renderengine->setViewportAndProjection(clientCompositionDisplay.physicalDisplay, clientCompositionDisplay.clip);
    int w = clientCompositionDisplay.physicalDisplay.getWidth();
    int h = clientCompositionDisplay.physicalDisplay.getHeight();
    Blur::setGLViewport(clientCompositionDisplay.physicalDisplay, w, h);

    std::vector<const renderengine::LayerSettings *> blurLayers;
    std::vector<int> blurIndexInLayerSettings;

    int index = -1;
    for (auto const layer : layerSettings) {
        index++;
        if (layer->miuiBlurRatio > 0.0f) {
            blurLayers.push_back(layer);
            blurIndexInLayerSettings.push_back(index);
        }
    }

    int firstBlurIndex = 1;
    if (blurIndexInLayerSettings.size() > 0) {
        firstBlurIndex = blurIndexInLayerSettings[0];
    }

    std::vector<const renderengine::LayerSettings *> coveredByBlurLayers(layerSettings.begin(),layerSettings.begin() + firstBlurIndex);
/*
     ALOGD(" Blur::onDoComposeSurfaces coveredByBlurLayers size:%zu  layerSettings :%zu blurLayers.size:%zu  blurIndexInLayerSettings:%zu w: %d  h:%d",
                                                                    coveredByBlurLayers.size(),
                                                                    layerSettings.size(),
                                                                    blurLayers.size(),
                                                                    blurIndexInLayerSettings.size(),
                                                                    w, h); */
    std::vector<const renderengine::LayerSettings *> blurredBackgroundLayers;
    renderengine->setViewportAndProjection(clientCompositionDisplay.physicalDisplay, clientCompositionDisplay.clip);
    renderengine->clearWithColor(0.0, 0.0, 0.0, 0.0);
    for (size_t i=0; i<blurLayers.size(); i++) {
        const renderengine::LayerSettings* blurLayer = blurLayers[i];

        sBlurRender->computeBlurParam(blurLayer->miuiBlurRatio);
        blurLayer->blurSurface->beginDrawToFbo();

        // Use an empty fence for the buffer fence, since we just created the buffer so
        // there is no need for synchronization with the GPU.
        base::unique_fd bufferFence;
        base::unique_fd drawFence;

        for (auto const layer : blurredBackgroundLayers) {
            layer->blurSurface->draw(clientCompositionDisplay, renderengine, layer);
        }

        if (i < 1) {
            renderengine->drawLayersByBlur(clientCompositionDisplay, coveredByBlurLayers,
                                 std::move(bufferFence), &drawFence);
        } else {
            std::vector<const renderengine::LayerSettings *> needDrawLayers;
            needDrawLayers.push_back(blurLayers[i-1]);
            renderengine->drawLayersByBlur(clientCompositionDisplay, needDrawLayers,
                                 std::move(bufferFence), &drawFence);
        }

        blurLayer->blurSurface->endDrawToFbo();
        blurLayer->blurSurface->makeFboBlur();

        blurredBackgroundLayers.push_back(blurLayer);
        /*
        static int managedColorFrameCount  = 0;
        std::ostringstream out;
        out << "/data/texture_out/ppm" << managedColorFrameCount++;
        sBlurRender->writePPM(out.str().c_str(), 674, 1462);
        */

    }
    return true;
}

void Blur::addBlurLayer(/*const sp<Layer>& layerSetting*/) {
    // (void)layerSetting;
    /*
    int index = indexOf(sBlurBehindLayerSettings, layerSetting);
    if (index == -1) {
        sBlurBehindLayerSettings->add(layerSetting);
    }
    */
}

void Blur::removeBlurLayer(/*const sp<Layer>& layer*/) {
     // (void)layer;
    /*
    if (layer->blurSurface != NULL) {
        layer->destroyBlurSurface();
        int index = indexOf(sBlurBehindLayers, layer);
        if (index >= 0) {
            sBlurBehindLayers->removeAt(static_cast<size_t>(index));
            if (sBlurBehindLayers->size() > 0) {
                sBlurInvalidated = true;
            }
        }
    }
    */
}

void Blur::clearBlurLayer(void) {
    /*
    if (sBlurBehindLayerSettings != NULL) {
        if (sBlurBehindLayerSettings->size() > 0) {
            sBlurBehindLayerSettings->clear();
        }
    }

    if (sBlurBehindLayerSettings != NULL) {
        if (sBlurBehindLayerSettings->size() > 0) {
            sBlurBehindLayerSettings->clear();
        }
    }
    */
}

bool Blur::isBlurLayerVisibleChild(/*const sp<const Layer>& layer*/) {
    // (void)layer;
    /*
    if (hasBlurLayer() && strstr(layer->getName().c_str(), "SurfaceView") != 0) {
        const auto& p = layer->getParent();
        if (p != nullptr && isLayerBlur(p)) {
            return true;
        }
    }
    */
    return false;
}

bool Blur::isLayerCovered(/*const sp<Layer>& layer*/) {
    // (void)layer;
    /*
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
    */
    return false;
}

void Blur::setGLViewport(Rect viewport, int width, int height) {
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
