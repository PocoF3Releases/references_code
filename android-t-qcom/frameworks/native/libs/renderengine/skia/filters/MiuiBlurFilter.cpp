#include "BlurFilter.h"
#include <SkCanvas.h>
#include <SkData.h>
#include <SkPaint.h>
#include <SkRRect.h>
#include <SkRuntimeEffect.h>
#include <SkSize.h>
#include <SkString.h>
#include <SkSurface.h>
#include <log/log.h>
#include <utils/Trace.h>
#include <cutils/properties.h>
#include <fstream>
#include <sstream>
#include <android-base/properties.h>

namespace android {
namespace renderengine {
namespace skia {

void BlurFilter::setSaturation() {
    SkString saturationString(R"(
        uniform shader saturationInput;
        uniform float saturationRatio;

        half4 main(float2 xy) {
            half3 temp = saturationInput.eval(xy).xyz;
            half3 colorIncrease = half3(
                temp.r * (1.0 + 2.0742 * saturationRatio) + temp.g * (-1.8282 * saturationRatio) + temp.b * (-0.246 * saturationRatio),
                temp.r * (-0.9258 * saturationRatio) + temp.g * (1.0 + 1.1718 * saturationRatio) + temp.b * (-0.246 * saturationRatio),
                temp.r * (-0.9258 * saturationRatio) + temp.g * (-1.8282 * saturationRatio) + temp.b * (1.0 + 2.754 * saturationRatio));
            return half4(colorIncrease, 1.0);
        }
    )");
    auto [saturationEffect, saturationError] = SkRuntimeEffect::MakeForShader(saturationString);
    if (!saturationEffect) {
        LOG_ALWAYS_FATAL("RuntimeShader error: %s", saturationError.c_str());
    }
    mSaturationEffect = std::move(saturationEffect);
}

void BlurFilter::setFrost() {
    mNoiseCoeff = std::atof(
            android::base::GetProperty("persist.sys.blur_noise_coeff", "0.009").c_str());
    uint32_t* noisePixels = new uint32_t[256 * 256];
    std::ifstream file;
    file.open("/product/etc/gpunoise.txt", std::ios::in);

    // Reads pixels from gpunoise.txt to a bitmap and generates the shader.
    if (file.is_open()) {
        uint32_t data;
        int i = 0;
        while (file >> std::hex >> data) {
            noisePixels[i++] = data;
        }
        file.close();
        SkImageInfo info = SkImageInfo::Make(256, 256, kRGBA_8888_SkColorType,
                                             kOpaque_SkAlphaType);
        SkBitmap bitmap;
        bitmap.installPixels(info, noisePixels, info.minRowBytes());
        mNoiseShader = bitmap.makeShader(SkTileMode::kRepeat, SkTileMode::kRepeat,
                       SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone));

    } else {
        ALOGE("hdr-dim: open noise file failed!");
    }
    delete[] noisePixels;
}

bool BlurFilter::isCTS() const{
    return !property_get_bool("persist.sys.miui_optimization",true);
}

sk_sp<SkImage> BlurFilter::makeSaturation(GrRecordingContext* context, sk_sp<SkImage> tmpBlur,
        SkSamplingOptions linear, SkImageInfo scaledInfo, uint32_t blurRadius) const {
    SkRuntimeShaderBuilder saturationBuilder(mSaturationEffect);
    saturationBuilder.child("saturationInput") =
            tmpBlur->makeShader(SkTileMode::kClamp, SkTileMode::kClamp, linear);
    saturationBuilder.uniform("saturationRatio") = blurRadius >= 100 ? 0.2f : (blurRadius / 100.0f) * 0.2f;
    tmpBlur = saturationBuilder.makeImage(context, nullptr, scaledInfo, false);
    return tmpBlur;
}

static SkMatrix getShaderTransform(const SkCanvas* canvas, const SkRect& blurRect, float scale) {
    // 1. Apply the blur shader matrix, which scales up the blured surface to its real size
    auto matrix = SkMatrix::Scale(scale, scale);
    // 2. Since the blurred surface has the size of the layer, we align it with the
    // top left corner of the layer position.
    matrix.postConcat(SkMatrix::Translate(blurRect.fLeft, blurRect.fTop));
    // 3. Finally, apply the inverse canvas matrix. The snapshot made in the BlurFilter is in the
    // original surface orientation. The inverse matrix has to be applied to align the blur
    // surface with the current orientation/position of the canvas.
    SkMatrix drawInverse;
    if (canvas != nullptr && canvas->getTotalMatrix().invert(&drawInverse)) {
        matrix.postConcat(drawInverse);
    }
    return matrix;
}

void BlurFilter::drawBlurShadow(GrRecordingContext* context, SkCanvas* canvas,
        const SkRRect& effectRegion, SkRect& blurShadowRect, sk_sp<SkImage> input) {

    auto blurredImage = generate(context, 100, input, blurShadowRect);
    SkPaint paint;
    const auto blurMatrix = getShaderTransform(canvas, blurShadowRect, kInverseInputScale);
    SkSamplingOptions linearSampling(SkFilterMode::kLinear, SkMipmapMode::kNone);
    const auto blurShadowShader = blurredImage->makeShader(SkTileMode::kClamp, SkTileMode::kClamp,
                                                     linearSampling, &blurMatrix);
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStrokeAndFill_Style);
    paint.setShader(blurShadowShader);
    paint.setImageFilter(SkImageFilters::Blur(8, 8, nullptr, nullptr));
    canvas->drawRRect(effectRegion, paint);
    bool isDarkMode = property_get_bool("debug.hwui.force_dark", false);
    SkPaint paintColor;
    if(isDarkMode){
        paintColor.setARGB(30, 250, 250, 250);
    } else {
        paintColor.setARGB(30, 100, 100, 100);
    }
    paintColor.setImageFilter(SkImageFilters::Blur(8, 8, nullptr, nullptr));
    canvas->drawRRect(effectRegion, paintColor);
}

void BlurFilter::drawAtmosphereLamp(GrRecordingContext* context, SkCanvas* canvas,
        const SkRRect& effectRegion, SkRect& blurShadowRect, sk_sp<SkImage> input) {

    auto blurredImage = generate(context, 100, input, blurShadowRect);
    SkPaint paint;

    const auto blurMatrix = getShaderTransform(canvas, blurShadowRect, kInverseInputScale);
    SkSamplingOptions linearSampling(SkFilterMode::kLinear, SkMipmapMode::kNone);
    const auto blurShadowShader = blurredImage->makeShader(SkTileMode::kClamp, SkTileMode::kClamp,
                                                     linearSampling, &blurMatrix);
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStrokeAndFill_Style);
    paint.setShader(blurShadowShader);
    paint.setImageFilter(SkImageFilters::Blur(8, 8, nullptr, nullptr));
    canvas->drawRRect(effectRegion, paint);
}

} // skia
} // renderengine
} // android