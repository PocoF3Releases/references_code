#pragma once

#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkMatrix.h>
#include <SkColorMatrix.h>
#include <SkColorFilter.h>
#include <SkData.h>
#include <SkImageGenerator.h>
#include <SkImage.h>
#include <SkShader.h>
#include <SkRuntimeEffect.h>
#include <cutils/properties.h>
#include <SkRRect.h>
#include <SkPath.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <renderengine/LayerSettings.h>

namespace android {
namespace renderengine {
namespace skia {

class HdrDimmer
{
private:
    sk_sp<SkRuntimeEffect> mHdrLUTEffect;
    sk_sp<SkRuntimeEffect> mHdrDimEffect;
    sk_sp<SkShader> mLutShader;
    sk_sp<SkShader> mNoiseShader;
    float mNoiseCoeff;
    float kMaxDimmerFactor;
public:
    explicit HdrDimmer();
    virtual ~HdrDimmer(){};
    void drawHdrDim(SkCanvas* canvas, SkPaint* paint, const SkRRect& bounds, bool layerEnabled,
                    float factor, float dimRatio, const std::vector<std::vector<float>>& brightRegions,
                    const std::vector<std::vector<float>>& dimRegions);
};

}
}
}