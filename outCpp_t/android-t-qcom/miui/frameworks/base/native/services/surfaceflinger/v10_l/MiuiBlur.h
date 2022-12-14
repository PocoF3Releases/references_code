#ifndef SF_RENDER_ENGINE_MIUI_BLUR_H
#define SF_RENDER_ENGINE_MIUI_BLUR_H

#include <stdint.h>
#include <system/window.h>
#include <sys/types.h>
#include <ui/Region.h>
#include <utils/String8.h>
#include <utils/RefBase.h>
#ifdef PLATFORM_VERSION_GREATER_THAN_28
#include "renderengine/Texture.h"
#include "renderengine/Mesh.h"
#else
#include "RenderEngine/Texture.h"
#include "RenderEngine/Mesh.h"
#endif
#include "Layer.h"

#define VISUAL_STATE_HIDED 0
#define VISUAL_STATE_SHOWING 1
#define VISUAL_STATE_SHOWED 2
#define VISUAL_STATE_HIDING 3
#define SCALE_SHIFT_BIT 5
#define BLUR_MODE_BLACK_SATURATION 0
#define BLUR_MODE_COLOR_DODGE 1
#define BLUR_MODE_WHITE_SATURATION 2
#define BLUR_MODE_COLOR_BURN 3
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
    mutable uint32_t mFboName;
    mutable uint32_t mFboTextureId;
    mutable uint32_t mTempTextureId;
#ifdef PLATFORM_VERSION_GREATER_THAN_28
    mutable renderengine::Texture mFboTexture;
    mutable renderengine::Mesh mMesh;
#else
    mutable Texture mFboTexture;
    mutable Mesh mMesh;
#endif
    mutable float mBlurRatio;
    mutable int mBlurMode;

    int64_t mShowHideTime;
    int64_t mStepTime;
    uint8_t mAnimatingAlpha;
    int mVisualState;

    Vector< sp<Layer> >* mContentLayers;
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

    void beginDrawToFbo(const sp<const DisplayDevice>& hw) const;
    void endDrawToFbo(const sp<const DisplayDevice>& hw, int32_t initFbo) const;
    void makeFboBlur(const sp<const DisplayDevice>& hw) const;
    void draw(const sp<const DisplayDevice>& hw, const sp<SurfaceFlinger>& flinger) const;
    bool computeBlurParam(float blurRatio, int blurMode);
    void setCallback(BlurCallback* callback) { mCallback = callback; }

    friend class Blur;
};

struct MiddleTexture {
    uint32_t textureId;
    uint32_t width;
    uint32_t height;
};

class Blur {
private:
    static bool sEnable;
    static int sMinification;
    static int sBlurRadius;
    static bool sUseHighPrecision;
    static bool sHasInitialized;
    static Vector< sp<Layer> >* sBlurBehindLayers;
    static size_t sPrevBlurBehindLayerCount;
    static Vector<Layer*>* sLayersPrevFrame;
    static uint32_t sBlurXProgram;
    static uint32_t sBlurYProgram;
    static bool sBlurInvalidated;
    static MiddleTexture *sMiddleTextures;
    static int sMiddleTexturesCount;
    static int sSimpleProgramId;
    static int sSaturationProgramId;
#ifdef PLATFORM_VERSION_GREATER_THAN_28
    static renderengine::Mesh sSimpleMesh;
#else
    static Mesh sSimpleMesh;
#endif
    static bool sScreenShot;

    static const char* getVertexShader();
    static void getFragmentShader(bool isX, int radius, int textureSize, String8& builder);
    static bool checkIsNeedReBlur(const Vector< sp<Layer> >& layers);

public:
    static uint32_t sScreenWidth;
    static uint32_t sScreenHeight;
    static uint32_t sBlurTextureWidth;
    static bool sInScreenRotation;
    static uint32_t sBlurTextureHeight;

    static void initialize(float density, bool useHighPrecision, uint32_t screenWidth, uint32_t screenHeight);

    static void genFbo(uint32_t* fbName);
    static void genTexture(uint32_t* texName, uint32_t width, uint32_t height);
    static void bindFbo(uint32_t fbName);
    static void bindTextureToFbo(uint32_t texName);
    static void deleteFbo(uint32_t fbName);
    static void deleteTexture(uint32_t texName);
    static void deleteProgram(uint32_t programId, uint32_t vsId, uint32_t fsId);
    static void clearWithColor(float red, float green, float blue, float alpha);
    static void blur(uint32_t fbName, uint32_t texName, uint32_t tempTextName, uint32_t width, uint32_t height);

#ifdef PLATFORM_VERSION_GREATER_THAN_28
    static bool onDoComposeSurfaces(const sp<const DisplayDevice>& hw, const sp<SurfaceFlinger>& flinger,
            const renderengine::DisplaySettings& clientCompositionDisplay, const Vector< sp<Layer> >& layers);
#endif

    static bool onDoComposeSurfaces(const sp<const DisplayDevice>& hw, const Vector< sp<Layer> >& layers);
    static void addBlurLayer(const sp<Layer>& layer);
    static void removeBlurLayer(const sp<Layer>& layer);
    #ifdef PLATFORM_VERSION_GREATER_THAN_28
    static void setglViewport(Rect viewport, size_t width, size_t height);
    #endif

    inline static bool isLayerBlur(const sp<Layer>& layer) {
        return layer->blurSurface != NULL;
    };
    inline static bool needLayerSkip(const sp<Layer>& layer) {
        return isLayerBlur(layer) || isLayerCovered(layer) || isBlurLayerVisibleChild(layer);
    };
    static bool isBlurLayerVisibleChild(const sp<const Layer>& layer);
    static bool isLayerCovered(const sp<Layer>& layer);

#ifdef PLATFORM_VERSION_GREATER_THAN_28
    static void checkErrors();
#endif

    inline static void invalidateBlur(Layer* layer) { (void)(layer); sBlurInvalidated = true; };
    inline static void setEnable(bool value) { sEnable = value; };
    inline static bool isEnable() { return sEnable; };
#ifdef PLATFORM_VERSION_GREATER_THAN_28
    inline static size_t blurLayerCount() { return sBlurBehindLayers->size(); };
#endif
    inline static bool hasBlurLayer() { return isEnable() && sBlurBehindLayers->size() > 0; };
    inline static int getMinification() { return sMinification; };
    inline static bool isScreenShot() { return sScreenShot; };
    inline static void setScreenShot(bool value) { sScreenShot = value; };

    friend class BlurSurface;
};

} /* namespace android */
#endif /* SF_RENDER_ENGINE_MIUI_BLUR_H */
