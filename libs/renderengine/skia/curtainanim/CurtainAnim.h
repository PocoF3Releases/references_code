#pragma once

#include <SkCanvas.h>
#include <SkData.h>
#include <SkPaint.h>
#include <SkRRect.h>
#include <SkSize.h>
#include <SkString.h>
#include <SkSurface.h>
#include <SkImage.h>
#include <SkRuntimeEffect.h>
#include <SkImageFilters.h>
#include <SkMaskFilter.h>
#include <cutils/properties.h>

namespace android {
namespace renderengine {
namespace skia {

class CurtainAnim {
public:
    explicit CurtainAnim();
    virtual ~CurtainAnim(){};
    sk_sp<SkShader> makeShader(SkCanvas* canvas, const SkRect& effectRegion,
                               sk_sp<SkImage> input,float progress);
    void drawCurtainAnim(SkCanvas* canvas, float rate, const sk_sp<SkSurface> surface);
private:
    sk_sp<SkRuntimeEffect> mCurtainAnimEffect;
    float mPushDeep = 0.f;
    float mFoldDeep = 0.f;
    float mScreenRadius = 0.f;
    bool mDebug = false;
};

} // namespace skia
} // namespace renderengine
} // namespace android