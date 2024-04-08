#include "HdrDimmer.h"
#include <utils/Log.h>
#include <sstream>
#include <cmath>
#include <android-base/properties.h>

namespace android {
namespace renderengine {
namespace skia {

HdrDimmer::HdrDimmer() {
    bool supported = android::base::GetBoolProperty("persist.sys.hdr_dimmer_supported", false);
    if (!supported) {
        ALOGD("hdr-dim: HdrDimmer is not supported.");
        return;
    }
    ALOGD("hdr-dim: HdrDimmer init...");

    mNoiseCoeff = std::atof(
            android::base::GetProperty("persist.sys.hdr_noise_coeff", "0.0075").c_str());

    kMaxDimmerFactor = std::atof(
            android::base::GetProperty("persist.sys.gallery_hdr_boost_max_factor", "2.25").c_str());

    // LUT Shader.
    SkString lutsksl(R"(
        uniform shader in_image;
        uniform shader in_lut;
        uniform shader in_noise;
        uniform float in_lut_ratio;
        uniform float in_dim_ratio;
        uniform float in_factor;
        uniform float in_coeff;

        half3 powColor(half3 c, half n) {
            return half3(pow(c.r, n), pow(c.g, n), pow(c.b, n));
        }

        half4 lut(half4 c) {
            float blueSeq = c.b * 63.0;

            half2 blueCube1;
            blueCube1.y = floor(floor(blueSeq) / 8.0);
            blueCube1.x = floor(blueSeq) - (blueCube1.y * 8.0);

            half2 blueCube2;
            blueCube2.y = floor(ceil(blueSeq) / 8.0);
            blueCube2.x = ceil(blueSeq) - (blueCube2.y * 8.0);

            float2 texPos1;
            texPos1.y = (blueCube1.y * 64.0) + (63.0 * c.g);
            texPos1.x = (blueCube1.x * 64.0) + (63.0 * c.r);

            float2 texPos2;
            texPos2.y = (blueCube2.y * 64.0) + (63.0 * c.g);
            texPos2.x = (blueCube2.x * 64.0) + (63.0 * c.r);

            // Sample 8 points' rgb around the current pixel.
            half3 newColor111 = powColor(in_lut.eval(float2(floor(texPos1.x), floor(texPos1.y))).rgb, 2.2);
            half3 newColor112 = powColor(in_lut.eval(float2(floor(texPos1.x), ceil(texPos1.y))).rgb, 2.2);
            half3 newColor121 = powColor(in_lut.eval(float2(ceil(texPos1.x), floor(texPos1.y))).rgb, 2.2);
            half3 newColor122 = powColor(in_lut.eval(float2(ceil(texPos1.x), ceil(texPos1.y))).rgb, 2.2);
            half3 newColor211 = powColor(in_lut.eval(float2(floor(texPos2.x), floor(texPos2.y))).rgb, 2.2);
            half3 newColor212 = powColor(in_lut.eval(float2(floor(texPos2.x), ceil(texPos2.y))).rgb, 2.2);
            half3 newColor221 = powColor(in_lut.eval(float2(ceil(texPos2.x), floor(texPos2.y))).rgb, 2.2);
            half3 newColor222 = powColor(in_lut.eval(float2(ceil(texPos2.x), ceil(texPos2.y))).rgb, 2.2);

            // First iteration of interpolation: Green.
            half3 newColor11 = mix(newColor111, newColor112, fract(texPos1.y));
            half3 newColor12 = mix(newColor121, newColor122, fract(texPos1.y));
            half3 newColor21 = mix(newColor211, newColor212, fract(texPos2.y));
            half3 newColor22 = mix(newColor221, newColor222, fract(texPos2.y));

            // Second iteration of interpolation: Red.
            half3 newColor1 = mix(newColor11, newColor12, fract(texPos1.x));
            half3 newColor2 = mix(newColor21, newColor22, fract(texPos2.x));

            // Final iteration of interpolation: Blue.
            half3 newColor = mix(newColor1, newColor2, fract(blueSeq));

            // Blend the original color and the new color in a ratio.
            newColor = mix(c.rgb, powColor(newColor, 0.454545455), in_lut_ratio);
            return half4(newColor, c.a);
        }

        half3 dim(half3 c, half3 noise) {
            c.r = pow(pow(c.r, 2.2)/in_factor, 1/2.2);
            c.g = pow(pow(c.g, 2.2)/in_factor, 1/2.2);
            c.b = pow(pow(c.b, 2.2)/in_factor, 1/2.2);
            return half3(c + noise * in_coeff);
        }

        half4 main(float2 coord) {
            half4 c = in_image.eval(coord);
            float dimRatio = smoothstep(0.0, 1.0, in_dim_ratio);
            return mix(half4(dim(c.rgb, in_noise.eval(coord).rgb - 0.5), c.a), lut(c), dimRatio);
        }
    )");

    auto [hdrLUTEffect, lutError] = SkRuntimeEffect::MakeForShader(lutsksl);
    if (!hdrLUTEffect) {
        LOG_ALWAYS_FATAL("RuntimeShader error: %s", lutError.c_str());
    }
    mHdrLUTEffect = std::move(hdrLUTEffect);

    // Dim shader.
    SkString dimsksl(R"(
        uniform shader in_image;
        uniform shader in_noise;
        uniform float in_factor;
        uniform float in_coeff;

        half4 main(float2 coord) {
            half4 c = in_image.eval(coord);
            c.r = pow(pow(c.r, 2.2)/in_factor, 1/2.2);
            c.g = pow(pow(c.g, 2.2)/in_factor, 1/2.2);
            c.b = pow(pow(c.b, 2.2)/in_factor, 1/2.2);
            half3 noise = in_noise.eval(coord).rgb - 0.5;
            return half4(c.rgb + noise * in_coeff, c.a);
        }
    )");

    auto [hdrDimEffect, dimError] = SkRuntimeEffect::MakeForShader(dimsksl);
    if (!hdrDimEffect) {
        LOG_ALWAYS_FATAL("RuntimeShader error: %s", dimError.c_str());
    }
    mHdrDimEffect = std::move(hdrDimEffect);


    uint32_t* pixels = new uint32_t[512 * 512];
    std::ifstream file;
    file.open("/product/etc/gpulut.txt", std::ios::in);

    // Reads pixels from gpulut.txt to a bitmap.
    if (file.is_open()) {
        uint32_t data;
        int i = 0;
        while (file >> std::hex >> data) {
            pixels[i++] = data;
        }
        file.close();
        SkImageInfo info = SkImageInfo::Make(512, 512, kRGB_101010x_SkColorType,
                                             kOpaque_SkAlphaType);
        SkBitmap bitmap;
        bitmap.installPixels(info, pixels, info.minRowBytes());
        mLutShader = bitmap.makeShader(SkSamplingOptions());
    } else {
        ALOGE("hdr-dim: open lut file failed!");
    }
    uint32_t* noisePixels = new uint32_t[256 * 256];
    file.open("/product/etc/gpunoise.txt", std::ios::in);

    // Reads pixels from gpunoise.txt to a bitmap.
    if (file.is_open()) {
        uint32_t data;
        int i = 0;
        while (file >> std::hex >> data) {
            noisePixels[i++] = data;
        }
        file.close();
        SkImageInfo info = SkImageInfo::Make(250, 250, kRGBA_8888_SkColorType,
                                             kOpaque_SkAlphaType);
        SkBitmap bitmap;
        bitmap.installPixels(info, noisePixels, info.minRowBytes());
        mNoiseShader = bitmap.makeShader(SkTileMode::kRepeat, SkTileMode::kRepeat,
                       SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone));

    } else {
        ALOGE("hdr-dim: open noise file failed!");
    }
    delete[] pixels;
    delete[] noisePixels;
}

void HdrDimmer::drawHdrDim(SkCanvas* canvas, SkPaint* paint, const SkRRect& bounds, bool layerEnabled,
                float factor, float dimRatio, const std::vector<std::vector<float>>& brightRegions,
                const std::vector<std::vector<float>>& dimRegions) {
    if (!layerEnabled) {    // Draw SDR layers.
        sk_sp<SkShader> dimImage =  paint->refShader();
        if (mNoiseShader && dimImage) {
            SkRuntimeShaderBuilder hdrDimBuilder(mHdrDimEffect);
            hdrDimBuilder.child("in_image") = dimImage;
            hdrDimBuilder.child("in_noise") = mNoiseShader;
            hdrDimBuilder.uniform("in_factor") = factor;
            hdrDimBuilder.uniform("in_coeff") = mNoiseCoeff;
            paint->setShader(hdrDimBuilder.makeShader());
        }
        if (!bounds.isRect()) {
            paint->setAntiAlias(true);
            canvas->drawRRect(bounds, *paint);
        } else {
            canvas->drawRect(bounds.rect(), *paint);
        }
    } else {    // Draw HDR layers.
        sk_sp<SkShader> dimImage =  paint->refShader();
        SkPaint dimPaint = *paint;
        if (mNoiseShader && dimImage) {
            SkRuntimeShaderBuilder hdrDimBuilder(mHdrDimEffect);
            hdrDimBuilder.child("in_image") = dimImage;
            hdrDimBuilder.child("in_noise") = mNoiseShader;
            hdrDimBuilder.uniform("in_factor") = factor;
            hdrDimBuilder.uniform("in_coeff") = mNoiseCoeff;
            dimPaint.setShader(hdrDimBuilder.makeShader());
        }

        // Clip (SkClipOp::kIntersect) canvas into the layer bounds.
        canvas->save();
        if (!bounds.isRect()) {
            canvas->clipRRect(bounds, SkClipOp::kIntersect, true);
        } else {
            canvas->clipRect(bounds.rect(), SkClipOp::kIntersect);
        }
        canvas->save();

        // Clip (SkClipOp::kDifference) the HDR areas.
        for (auto rect : brightRegions) {
            auto tmpBounds = SkRect({rect[0], rect[1], rect[2], rect[3]});
            SkMatrix matrix;
            matrix.set9(&rect[4]);
            if (rect[15] > 0.5f || rect[16] > 0.5f || rect[17] > 0.5f || rect[18] > 0.5f ) {
                matrix.mapRect(&tmpBounds);
                tmpBounds.inset(rect[15], rect[16]);
                tmpBounds.offset(rect[13], rect[14]);
                canvas->clipRect(tmpBounds, SkClipOp::kDifference);
            } else {
                SkPath path;
                path.addRect(tmpBounds);
                auto tmpPath = path.makeTransform(matrix);
                tmpPath.offset(rect[13], rect[14]);
                canvas->clipPath(tmpPath, SkClipOp::kDifference);
            }
        }

        // Draw the most SDR areas.
        if (!bounds.isRect()) {
            dimPaint.setAntiAlias(true);
            canvas->drawRRect(bounds, dimPaint);
        } else {
            canvas->drawRect(bounds.rect(), dimPaint);
        }
        canvas->restore();

        // Draw the HDR areas.
        SkPaint brightPaint = *paint;
        sk_sp<SkShader> brightImage =  brightPaint.refShader();
        if (mLutShader && brightImage) {
            SkRuntimeShaderBuilder hdrLUTBuilder(mHdrLUTEffect);
            hdrLUTBuilder.child("in_image") = brightImage;
            hdrLUTBuilder.child("in_lut") = mLutShader;
            hdrLUTBuilder.child("in_noise") = mNoiseShader;
            hdrLUTBuilder.uniform("in_lut_ratio") = std::min(1.0f, factor / kMaxDimmerFactor);
            hdrLUTBuilder.uniform("in_dim_ratio") = dimRatio;
            hdrLUTBuilder.uniform("in_factor") = factor;
            hdrLUTBuilder.uniform("in_coeff") = mNoiseCoeff;
            brightPaint.setShader(hdrLUTBuilder.makeShader());
        }
        for (auto rect : brightRegions) {
            auto tmpBounds = SkRect({rect[0], rect[1], rect[2], rect[3]});
            SkMatrix matrix;
            matrix.set9(&rect[4]);
            if (rect[15] > 0.5f || rect[16] > 0.5f || rect[17] > 0.5f || rect[18] > 0.5f ) {
                matrix.mapRect(&tmpBounds);
                tmpBounds.inset(rect[15], rect[16]);
                tmpBounds.offset(rect[13], rect[14]);
                canvas->drawRect(tmpBounds, brightPaint);
            } else {
                SkPath path;
                path.addRect(tmpBounds);
                auto tmpPath = path.makeTransform(matrix);
                tmpPath.offset(rect[13], rect[14]);
                canvas->drawPath(tmpPath, brightPaint);
            }
        }

        // Draw the extra SDR areas.
        if (brightRegions.size() > 0) {
            for (auto dimRect : dimRegions) {
                SkRRect tmpBounds;
                SkVector corners[] = {{dimRect[4], dimRect[4]}, {dimRect[5], dimRect[5]},
                    {dimRect[7], dimRect[7]}, {dimRect[6], dimRect[6]}};
                tmpBounds.setRectRadii(SkRect({dimRect[0], dimRect[1], dimRect[2], dimRect[3]}), corners);
                if (!tmpBounds.isRect()) {
                    dimPaint.setAntiAlias(true);
                    canvas->drawRRect(tmpBounds, dimPaint);
                } else {
                    canvas->drawRect(tmpBounds.rect(), dimPaint);
                }
            }
        }
        canvas->restore();
    }
}

}
}
}