#ifndef SF_RENDER_ENGINE_MIUI_BLUR_H
#define SF_RENDER_ENGINE_MIUI_BLUR_H

#include <stdint.h>
#include <system/window.h>
#include <sys/types.h>
#include <ui/Region.h>
#include <utils/String8.h>
#include <utils/RefBase.h>
#include "renderengine/Texture.h"
#include "renderengine/Mesh.h"
#include "Layer.h"
#include "MiuiRenderEngine.h"

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

class BlurSurface {
private:
    Layer* mLayer;

    LayerObject mLayerDrawInfo;

    mutable Rect mBlurAbsoluteCrop;
    mutable Rect mBlurRelativeCrop;

    int64_t mShowHideTime;
    int64_t mStepTime;
    int mVisualState;

    mutable renderengine::Mesh mMesh;
    mutable renderengine::Texture mFboTexture;

    Vector< sp<Layer> >* mContentLayers;
    std::vector<renderengine::LayerSettings> mBlurContentLayerSettings;
    BlurCallback* mCallback = nullptr;

    void drawContentWithOpenGL(const sp<const DisplayDevice>& hw, const sp<SurfaceFlinger>& flinger) const;
    void releaseResource();

public:
    BlurSurface(Layer* layer, uint32_t layerWidth, uint32_t layerHeight);
    ~BlurSurface();

    void show(int64_t now);
    bool step(int64_t now, uint8_t alpha);
    void hide(int64_t now);
    inline int64_t stepTime() { return mStepTime; };
    bool coverBackgroud(const sp<const DisplayDevice>& hw) const;
    void beginDrawToFbo(const sp<const DisplayDevice>& hw);
    void endDrawToFbo(const sp<const DisplayDevice>& hw);
    void makeFboBlur(const sp<const DisplayDevice>& hw);
    bool setCrop(Rect relative,Rect abs);
    void draw(const sp<const DisplayDevice>& hw, const sp<SurfaceFlinger>& flinger) const;
    bool computeBlurParam(float blurRatio, int blurMode);
    void setCallback(BlurCallback* callback) { mCallback = callback; }

    uint8_t mAnimatingAlpha;

    friend class Blur;
};

class Blur {
private:
    static bool sEnable;
    static bool sHasInitialized;
    static bool sBlurInvalidated;
    static bool sBlurSurfaceContentChanged;
    static bool sYswap;
    static size_t sPrevBlurBehindLayerCount;

    static Vector< sp<Layer> >* sBlurBehindLayers;
    static Vector<Layer*>* sLayersPrevFrame;

    static Vector< sp<Layer> >* sCoveredByBlurLayers;

    static bool checkIsNeedReBlur(const Vector< sp<Layer> >& layers);
    static bool checkIsContentLayerChanges(const Vector< sp<Layer> >* contentLayer);

public:
    static void initialize(float density, bool useHighPrecision, uint32_t screenWidth, uint32_t screenHeight);
    static void release();
    static bool onDoComposeSurfaces(const sp<const DisplayDevice>& hw, const sp<SurfaceFlinger>& flinger,
            const renderengine::DisplaySettings& clientCompositionDisplay, const Vector< sp<Layer> >& layers);

    static void addBlurLayer(const sp<Layer>& layer);
    static void removeBlurLayer(const sp<Layer>& layer);
    static bool isBlurLayerVisibleChild(const sp<const Layer>& layer);
    static bool isLayerCovered(const sp<Layer>& layer);
    static void clearBlurLayer(void);

    inline static bool isLayerBlur(const sp<Layer>& layer) {
        return layer->blurSurface != NULL;
    };

    inline static bool needLayerSkip(const sp<Layer>& layer) {
        return isEnable() && (isLayerBlur(layer) || isLayerCovered(layer) || isBlurLayerVisibleChild(layer));//set for gpu composer
    };

    inline static void invalidateBlur(Layer* layer) { (void)(layer); sBlurInvalidated = true; };
    inline static void setEnable(bool value) { sEnable = value; };
    inline static bool isEnable() { return sEnable; };

    inline static size_t blurLayerCount() { return sBlurBehindLayers->size(); };
    inline static bool hasBlurLayer() { return isEnable() && sBlurBehindLayers->size() > 0; };
    inline static bool isYswap() { return sYswap; };
    inline static void setYswap(bool value) { sYswap = value; };

    static void setglViewport(Rect viewport, size_t width, size_t height);
    static void checkErrors();

    static uint32_t sScreenWidth;
    static uint32_t sScreenHeight;

    friend class BlurSurface;
};

} /* namespace android */
#endif /* SF_RENDER_ENGINE_MIUI_BLUR_H */
