#ifndef SF_RENDER_ENGINE_MIUI_BLUR_H
#define SF_RENDER_ENGINE_MIUI_BLUR_H

#include <stdint.h>
#include <system/window.h>
#include <sys/types.h>
#include <ui/Region.h>
#include <ui/Rotation.h>
#include <utils/String8.h>
#include <utils/RefBase.h>
#include <renderengine/RenderEngine.h>
#include <renderengine/Mesh.h>
#include <renderengine/LayerSettings.h>
// #include "Layer.h"
#include <MiuiRenderEngineLowEffect.h>

#define VISUAL_STATE_HIDED 0
#define VISUAL_STATE_SHOWING 1
#define VISUAL_STATE_SHOWED 2
#define VISUAL_STATE_HIDING 3

#define SCALE_STARTTING 8.0f/16.0f
#define SCALE_SPEED 7.0f/16.0f

namespace android {

class DisplayDevice;
class Blur;


class BlurCallback {
public:
    virtual ~BlurCallback(){};
    virtual void setupLayerBlending(float alpha) = 0;
};

namespace renderengine{
struct LayerSettings;
class RenderEngine;


class BlurSurface {
private:
    /*Layer* mLayer;*/

    LayerObject mLayerDrawInfo;

    mutable Rect mBlurAbsoluteCrop;
    mutable Rect mBlurRelativeCrop;

    int64_t mShowHideTime;
    int64_t mStepTime;
    int mVisualState;

    bool mBlurCurrentLayer;
    mutable renderengine::Mesh mMesh;
    mutable renderengine::Texture mFboTexture;

    // Vector< sp<Layer> >* mContentLayers;
    std::vector<renderengine::LayerSettings> mBlurContentLayerSettings;
    BlurCallback* mCallback = nullptr;  // Need check

    void drawContentWithOpenGL(const renderengine::DisplaySettings& display, renderengine::RenderEngine* renderEngine, const renderengine::LayerSettings* layer);
    void releaseResource();

public:
    BlurSurface(/*Layer* layer, */uint32_t layerWidth, uint32_t layerHeight);
    ~BlurSurface();

    void show(int64_t now);
    bool step(int64_t now, uint8_t alpha);
    void hide(int64_t now);
    inline int64_t stepTime() { return mStepTime; };
    bool coverBackgroud(const sp<const DisplayDevice>& hw) const;
    void beginDrawToFbo();
    void endDrawToFbo();
    void makeFboBlur();
    bool setCrop(Rect relative,Rect abs);
    void draw(const renderengine::DisplaySettings& display, renderengine::RenderEngine* renderEngine, const renderengine::LayerSettings* layer);
    bool computeBlurParam(float blurRatio, int blurMode);
    void setCallback(BlurCallback* callback) { mCallback = callback; }
    void setupBlurCurrentState(bool mBlurCurrent);

    void prepareSurfaceSettingsinfo() const;

    uint8_t mAnimatingAlpha;

    friend class Blur;
};
}

class Blur {
private:
    static bool sEnable;
    static bool sHasInitialized;
    static bool sBlurInvalidated;
    static bool sBlurSurfaceContentChanged;
    static bool sYswap;
    static size_t sPrevBlurBehindLayerCount;

    static vector< sp<renderengine::LayerSettings> >* sBlurBehindLayerSettings;
    // static Vector< sp<Layer*> >* sBlurBehindLayers;
    // static Vector<Layer*>* sLayersPrevFrame;

    // static Vector< sp<Layer> >* sCoveredByBlurLayers;

    static bool checkIsNeedReBlur(/*const Vector< sp<Layer> >& layers*/);
    static bool checkIsContentLayerChanges(/*const Vector< sp<Layer> >* contentLayer*/);

public:
    static void initialize(float density, bool useHighPrecision, uint32_t screenWidth, uint32_t screenHeight);
    static void release();
    static bool onDoComposeSurfaces(renderengine::RenderEngine* renderengine,
                                    const renderengine::DisplaySettings& clientCompositionDisplay,
                                    std::vector<const renderengine::LayerSettings *> layerSettings);

    static void addBlurLayer(/*const sp<Layer>& layerSetting*/);
    static void removeBlurLayer(/*const sp<Layer>& layer*/);
    static bool isBlurLayerVisibleChild(/*const sp<const Layer>& layer*/);
    static bool isLayerCovered(/*const sp<Layer>& layer*/);
    static void clearBlurLayer(void);

    inline static bool isLayerBlur(/*const sp<Layer>& layer*/) {
        return false; //layer->blurSurface != NULL;
    };

    inline static bool needLayerSkip(/*const sp<Layer>& layer*/) {
        return true;/*isEnable() && (isLayerBlur(layer) || isLayerCovered(layer) || isBlurLayerVisibleChild(layer));*///set for gpu composer
    };

    inline static void invalidateBlur() { sBlurInvalidated = true; };
    inline static void setEnable(bool value) { sEnable = value; };
    inline static bool isEnable() { return sEnable; };

    inline static size_t blurLayerCount() { return 0;} //sBlurBehindLayers->size(); };
    inline static bool hasBlurLayer() {
        return false;}//isEnable(); }//&& sBlurBehindLayers->size() > 0; };
    inline static bool isYswap() { return sYswap; };
    inline static void setYswap(bool value) { sYswap = value; };

    static void setGLViewport(Rect viewport, int width, int height);
    static void checkErrors();

    static uint32_t sScreenWidth;
    static uint32_t sScreenHeight;

    friend class BlurSurface;
};

} /* namespace android */
#endif /* SF_RENDER_ENGINE_MIUI_BLUR_H */
