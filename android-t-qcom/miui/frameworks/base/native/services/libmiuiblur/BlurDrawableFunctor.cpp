#include <android/log.h>
#include <jni.h>
#include <string>
#include "BlurDrawableFunctor.h"

#define  MYLOGE(...)  __android_log_print(ANDROID_LOG_ERROR,"BLUR_DEBUG",__VA_ARGS__)

using namespace android;
using namespace uirenderer;

status_t BlurDrawableFunctor::operator()(int mode, void *info) {
    operate(mode, info);
    return 0;
}

void BlurDrawableFunctor::operate(int mode, void *info) {
    if (info == NULL) {
        return;
    }

    DrawGlInfo *c_drawGlInfo = reinterpret_cast<DrawGlInfo *>(info);
    renderFrame(c_drawGlInfo);
    return;
}

void BlurDrawableFunctor::renderFrame(DrawGlInfo *c_drawGlInfo) {
    std::unique_lock<mutex> lock(mStateLock);
    if (nullptr == mBlurRender) {
        MYLOGE("enderEngine::create");
        mBlurRender = MiuiRenderEngine::create(1.0f, 0, 0);
    }
    mBlurRender->computeBlurParam(mFunctor, c_drawGlInfo);
    mBlurRender->prepareBlurDrawable();
    mBlurRender->beginDrawToFbo();
    if (mFunctor.mBlurRatio != 0.0F) {
        mBlurRender->makeFboBlur();
    }
    if (mFunctor.mMixMode.size() > 0) {
        for(size_t i = 0; i<mFunctor.mMixMode.size(); i++) {
            mBlurRender->drawBlendColor(mFunctor.mMixColor[i], mFunctor.mMixMode[i], i % 2 != 0);
        }

    }
    mBlurRender->drawRoundCorner(mFunctor.mMixMode.size() % 2 == 0);
    mBlurRender->onPostBlur();
}

BlurDrawableFunctor::BlurDrawableFunctor() {
    mFunctor.isEnable = true;
    mFunctor.mBlurRadius = 0;
    mFunctor.mBlurMode = 0;
    mFunctor.mBlurRatio = 1.0f;
    mFunctor.mMixMode.clear();
    mFunctor.mMixColor.clear();
    float defaultCornerRadii[4] = {0, 0,0,0};
    memcpy(&mFunctor.mBlurCornerRadii, defaultCornerRadii, sizeof(defaultCornerRadii));
}

BlurDrawableFunctor::~BlurDrawableFunctor() {
    std::unique_lock<mutex> lock(mStateLock);
    if (mBlurRender != nullptr) {
        mBlurRender->releaseResource();
        delete mBlurRender;
        mBlurRender = nullptr;
    }
}

