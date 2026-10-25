#ifndef HANDY_MODE_FOR_SF_H
#define HANDY_MODE_FOR_SF_H

#include <renderengine/Texture.h>
#include <renderengine/Mesh.h>
#include <renderengine/RenderEngine.h>

namespace android {

enum {
    MODE_NONE = 0,
    MODE_LEFT = 1,
    MODE_RIGHT = 2,
    MODE_DOWN = 3
};

enum {
    ANIMATE_STATE_OPENING,
    ANIMATE_STATE_OPENED,
    ANIMATE_STATE_CLOSING,
    ANIMATE_STATE_CLOSED,
    ANIMATE_STATE_MOVING,
};

class IRepaintCallback {
public:
    virtual ~IRepaintCallback() = default;
    virtual void repaintCallback() = 0;
};

class HandyModeForSF
{
private:
    static HandyModeForSF* sInstance;

    int mMode;
    float mScale;
    int mScreenWidth;
    int mScreenHeight;
    float mScaleforHandyMode;

    int mAnimateState;
    long long mAnimateStartTime;
    // Animate from 0 to 1 while opening or moving, animate from 1 to 0 while closing.
    float mAnimateCurrRate;
    int mBackMode;

    bool mTexturesIsLoaded;
    renderengine::Texture* mTitleTexture;
    renderengine::Texture* mBackgroundTexture;
    renderengine::Texture* mSettingIconTexture;
    // The transform applyed by handy mode. This is used to correct positionTransform of layers.
    ui::Transform mTransform = ui::Transform();

    int32_t mSettingIconMargin;
    renderengine::Mesh mSimpleMesh;
    IRepaintCallback* mReCallBack = nullptr;

    HandyModeForSF();
    ~HandyModeForSF();

    void loadTextures();
    void destroyTextures();

public:
    static void init(/*SurfaceFlinger* sf*/);
    static inline HandyModeForSF* getInstance() { return sInstance; }

    void setCallBack(IRepaintCallback *callBack) {mReCallBack = callBack;}

    inline float getScale() { return mScale; }
    inline void setScale(float scale) { mScale = scale; }
    inline float getHandyModeScale() { return mScaleforHandyMode; }
    inline ui::Transform getTransform() {return mTransform;}
    inline int getMode() { return mMode; }
    inline void setMode(int mode) { mMode = mode; }
    inline bool isInHandyMode() { return mMode != MODE_NONE; }
    inline bool isInScaleMode() { return mMode == MODE_LEFT || mMode == MODE_RIGHT; }

    inline void setSetingTexture(renderengine::Texture* texture) {
        mSettingIconTexture = texture;
    }
    inline void setBackgroundTexture(renderengine::Texture* texture) {
        mBackgroundTexture = texture;
    }
    inline void setTitleTexture(renderengine::Texture* texture) {
        mTitleTexture = texture;
    }
    inline void setTexturesIsLoaded(bool isLoaded) {
        mTexturesIsLoaded = isLoaded;
    }
    inline bool isTexturesIsLoaded() {
        return mTexturesIsLoaded;
    }

    void setupScreenSize(int w, int h);

    void onDoComposeSurfaces(renderengine::RenderEngine* renderEngine);
    void onPreLayerDraw();
    void onPostLayerDraw();
    void onPreScreenshotRender();
    void onPostScreenshotRender();
    void onPostLayerComputeGeometry(renderengine::Mesh& mesh);

    void onPartBlurScissor(vec2& pos, details::TVec2<int>& size);

    inline void changeMode(int mode, float scale) {
        if (mode == MODE_NONE) {
            mAnimateState = ANIMATE_STATE_CLOSING;
            mAnimateCurrRate = 1.0f;
        } else {
            if (mAnimateState == ANIMATE_STATE_OPENED) mAnimateState = ANIMATE_STATE_MOVING;
            else mAnimateState = ANIMATE_STATE_OPENING;
            mAnimateCurrRate = 0;
            mScale = scale;
            mMode = mode;
        }
        mAnimateStartTime = systemTime()/1000000;
    }

    static inline float processCoordinate(float coordValue, float offset, float scale, float scalePivot) {
        return scalePivot - (scalePivot - coordValue) * scale - offset;
    }
};

}

#endif /* HANDY_MODE_FOR_SF_H */
