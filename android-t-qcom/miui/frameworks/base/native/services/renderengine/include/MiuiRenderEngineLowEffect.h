/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SF_BLURRENDERENGINE_H_
#define SF_BLURRENDERENGINE_H_

#include <stdint.h>
#include <sys/types.h>
#include <condition_variable>
#include <deque>
#include <queue>
#include <unordered_map>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <sys/system_properties.h>
#include <utils/String8.h>

#include "vec2.h"
#include "MMesh.h"
#include "Math.h"
#include "MiuiProgram.h"
#include "MiuiShaderUtilLowEffect.h"
#include "DrawGlInfo.h"

#define  LOG(...)  __android_log_print(ANDROID_LOG_ERROR,"MiuiRenderEngineLowEffect",__VA_ARGS__)

#define COORDS_PER_VERTEX 3
#define VERTEX_STRIDE (COORDS_PER_VERTEX * 4)

#define MPROPERTY_VALUE_MAX 92

#define SCALE_STARTTING 8.0f/16.0f
#define SCALE_SPEED 7.0f/16.0f
#define SCALE_SHIFT_BIT 5

#define MIDDLE_TEXTURE_MAX_SCALE 1/16
#define MIDDLE_TEXTURE_SCALE_STARTTING 10/16
#define MIDDLE_TEXTURE_SCALE_INTERVAL 1/16

namespace android {

using namespace uirenderer;

class DisplayDevice;
class MiuiProgram;
class MMesh;

enum {
    eAttrLocPosition = 1,
    eAttrLocTexCoords = 2,
};

static float squareCoords[] = {
        0.0f, 0.0f, 0.0f, // top left
        1.0f, 0.0f, 0.0f, // bottom left
        0.0f, 1.0f, 0.0f, // bottom right
        1.0f, 1.0f, 0.0f, // top right
};

static float mTexHorizontalCoords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
};

static short drawOrder[] = {
        0, 1, 2, 2, 3, 1
};

static const float colormix[] = {
        0.0f, 0.0f, 0.0f, 0.0f
};

struct TextureObject {
    uint32_t width;
    uint32_t height;
    uint32_t textureId;
};

struct FrameBufferObject {
    uint32_t framebufferId;
    uint32_t textureId;
};

struct DrawObject {
    float mBlurRatio;
    int mBlurRadius;
    int mBlurMode;
    int mCurrWeightIndex;
    int mNextWeightIndex;
    uint32_t mBlurTextureWidth;
    uint32_t mBlurTextureHeight;

    FrameBufferObject mFbo;
    TextureObject mFboTexture;
    TextureObject mTempTexture;
    TextureObject mBlendTexture;
    TextureObject mBlendTwoTexture;

    bool mChangeTextureSize;
};

struct ViewObject {
    bool mNeedRelink;
    bool mChildRedraw;
    int mMixMode;
    int mMixColor;
    float mModelMatrix[16];
    float mViewMatrix[16];
    float mProjMatrix[16];
    float mScreenMVPMatrix[16];
    float mMVPMatrix[16];

    float mTexMatrix[16];
    float mBlurCornerRadii[4];
    DrawGlInfo mGlInfo;
    DrawGlInfo mParentGlInfo;
    TextureObject mDisplayTexture;
    DrawObject mDrawInfo;
};

struct FunctorData {
    int isEnable;
    int mBlurMode;
    int mBlurRadius;
    float mBlurRatio;
    vector<int> mMixMode;
    vector<int> mMixColor;
    float mBlurCornerRadii[4];
};

struct DrawableFunctorData {
    int isEnable;
    ViewObject mViewDrawInfo;
};

struct LayerObject {
    float blurCornerRadii[4] = {0,0,0,0};
    DrawObject mDrawInfo;
};

class MiuiRenderEngineLowEffect {
public:
    uint32_t mVpWidth;
    uint32_t mVpHeight;

    MiuiShaderUtilLowEffect mShaderUtil;

    mutable float mRatio;
    mutable float mBlack;
    mutable float mMode;
    mutable float mIncreaseSaturation;

    uint32_t mMiddleTexturesCount;
    TextureObject* mMiddleTextures;

    DrawableFunctorData mViewFunctor;

    std::unique_ptr<MiuiProgram> mSimpleProgram = nullptr;
    std::unique_ptr<MiuiProgram> mSaturationProgram = nullptr;
    std::unique_ptr<MiuiProgram> mBlurXProgram = nullptr;
    std::unique_ptr<MiuiProgram> mBlurYProgram = nullptr;
    std::unique_ptr<MiuiProgram> mCopyProgram = nullptr;
    std::unique_ptr<MiuiProgram> mBlendColorProgram = nullptr;
    std::unique_ptr<MiuiProgram> mRoundCornerProgram = nullptr;

    static MiuiRenderEngineLowEffect* create(float density, uint32_t screenWidth, uint32_t screenHeight);

    MiuiRenderEngineLowEffect(float density, uint32_t screenWidth, uint32_t screenHeight);
    ~MiuiRenderEngineLowEffect();

    void checkErrors(const char *msg);
    void writePPM(const char* basename, uint32_t width, uint32_t height);
    void enableDumpPPM();
    void disableDumpPPM();

    void prepareFbo();
    void onPostBlur();
    void createFbo(FrameBufferObject &mFrameBufferObject);
    void createTexture(TextureObject &mTextureObject);
    void resizeTexture(TextureObject &mTextureObject);
    void bindFbo(FrameBufferObject &mFrameBufferObject);
    void bindTextureToFbo(FrameBufferObject &mFrameBufferObject, TextureObject &mTextureObject);
    void deleteFbo(uint32_t fbName);
    void deleteTexture(uint32_t texName);

    void setViewport(size_t left, size_t top, size_t width, size_t height);
    void drawMeshSimple(const MMesh& mesh);
    void clearWithColor(float red, float green, float blue, float alpha);

    void computeBlurParam(float blurRatio, float blurMode);
    int getMaxBlurRadius();

    void beginDrawToFbo();
    void beginDrawToFbo(LayerObject &mLayerDrawInfo);
    void endDrawToFbo();
    void endDrawToFbo(LayerObject &mLayerDrawInfo);
    void makeFboBlur();
    void makeFboBlur(LayerObject &mLayerDrawInfo);

    void deletePrograms();
    void releaseResource(DrawObject &mDrawInfo);

    void prepareViewFunctor();
    void releaseResource();
    void computeBlurParam(FunctorData &mFunctor, DrawGlInfo *c_drawGlInfo);
    bool prepareBlurDrawable();
    void prepareTexture(ViewObject &mViewDrawInfo);
    void initMVPMatrix(ViewObject &mViewDrawInfo);
    void copyTextureFromScreen(ViewObject mViewDrawInfo);
    void checkClipSize(DrawGlInfo& info);
    void selectDisplayTexture(ViewObject &mViewDrawInfo);
    bool upscaleWithMixColor();
    bool drawBlendColor(int color, int mode, bool isTwo);
    bool drawRoundCorner(bool isTwo);
    bool isChildrenGLInfo();

private:
    float* mWeight;
    int mMaxBlurRadius;
    float mMaxBlack;
    float mMaxIncreaseSaturation;
    bool mUseHighPrecision;
    static MMesh sSimpleMesh;
    LayerObject mLayerDrawInfo;
};

} // namespace android

#endif /* SF_GLESRENDERENGINE_H_ */
