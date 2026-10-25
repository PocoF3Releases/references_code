// MIUI ADD: This file is added just for Global HBM Dither (J2S-T)
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "NoisePattern.h"
#include "MiHBMEffect.h"

namespace android {
namespace renderengine {
namespace skia {

void MiHBMEffect::initPatternSkImage() {
    SkBitmap patternBitmap;
    SkImageInfo imageInfo = SkImageInfo::Make(64, 64, kGray_8_SkColorType, kPremul_SkAlphaType);

    if (!patternBitmap.installPixels(imageInfo, (unsigned char*) &kNoisePattern + 16, 64 * 3)) {
        ALOGE("%s patternBitmap installPixels failed!", __func__);
        return;
    }

    mPatternSkImage = SkImage::MakeFromBitmap(patternBitmap);
    if (mPatternSkImage == nullptr){
        ALOGE("%s mPatternSkImage make failed!", __func__);
        return;
    }

    mCanApplyPattern = true;
}

static void generateHBMShader(SkString& shader, bool workAroundChange) {
    shader.append(R"(
        uniform shader originalInput;
        uniform shader patternInput;
        uniform float alpha;
        uniform float coeff;
        half4 main(float2 xy) {
            float4 c = float4(sample(originalInput, xy));
            float4 pattern = float4(sample(patternInput, xy));
            float3 noise = (pattern.rgb - 0.6) * coeff;
    )");

    if (workAroundChange) {
         shader.append(R"(
             return half4(clamp(c.rgb * alpha + noise, 0.0, 1.0), 1.0);
         }
         )");
    } else {
         shader.append(R"(
             return half4(clamp(c.rgb * alpha + noise, 0.0, 1.0), c.a);
         }
         )");
    }
}

sk_sp<SkRuntimeEffect> MiHBMEffect::buildMiRuntimeEffect(bool workAroundChange) {
    ATRACE_CALL();
    SkString shaderString;
    generateHBMShader(shaderString, workAroundChange);

    auto [shader, error] = SkRuntimeEffect::MakeForShader(shaderString);
    if (!shader) {
        LOG_ALWAYS_FATAL("MiShader construction error: %s", error.c_str());
    }

    return shader;
}

sk_sp<SkShader> MiHBMEffect::createMiRuntimeEffectShader(const LayerSettings& layer,
                                                         sk_sp<SkShader> shader,
                                                         const float hbmLayerAlpha) {
    bool workAroundChange = false;

    if (!mCanApplyPattern) {
        ALOGE("%s can't apply pattern effect", __func__);
        return shader;
    }

    if (layer.name.find("Wallpaper BBQ wrapper") != std::string::npos) {
        workAroundChange = true;
    }

    sk_sp<SkRuntimeEffect> runtimeEffect = nullptr;

    runtimeEffect = buildMiRuntimeEffect(workAroundChange);

    SkRuntimeShaderBuilder effectBuilder(runtimeEffect);

    SkSamplingOptions linear(SkFilterMode::kLinear, SkMipmapMode::kNone);
    effectBuilder.child("originalInput") =  shader;
    effectBuilder.child("patternInput") =
            mPatternSkImage->makeShader(SkTileMode::kRepeat, SkTileMode::kRepeat, linear);
    effectBuilder.uniform("alpha") = hbmLayerAlpha;
    effectBuilder.uniform("coeff") = factorCoeff;

    return effectBuilder.makeShader(nullptr);
}

} // namespace skia
} // namespace renderengine
} // namespace android
// MIUI ADD: END
