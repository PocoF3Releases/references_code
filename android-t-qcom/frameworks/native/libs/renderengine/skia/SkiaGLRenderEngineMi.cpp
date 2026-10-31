#ifdef MI_FEATURE_ENABLE
// MIUI ADD: This file is added just for Global HBM Dither (J2S-T)
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <utils/Trace.h>
#include "SkiaGLRenderEngine.h"

namespace android {
namespace renderengine {
namespace skia {

void SkiaGLRenderEngine::getMiHBMEffectParams(const std::vector<LayerSettings>& layers,
                                              bool& needApplyHBMEffect,
                                              uint32_t& layersInfo,
                                              float& hbmLayerAlpha) {
    for (const auto& layer : layers) {
        if (layer.name.find("hbm_overlay") != std::string::npos) {
            needApplyHBMEffect = true;
            hbmLayerAlpha = 1.0f - (float)layer.alpha;
            continue;
        }

        if (layer.name.find("ColorFade") != std::string::npos) {
            layersInfo = layersInfo | COLORFADE_LAYER_MASK;
        }

        if (layer.name.find("AOD") != std::string::npos || layer.backgroundBlurRadius) {
            layersInfo = layersInfo | NO_HBM_EFFECT_MASK;
        }
    }
}

bool SkiaGLRenderEngine::skiaHBMLayerDraw(const LayerSettings& layer,
                                          MiHBMEffect* miHBMEffect) {
    bool ret = false;

    if (miHBMEffect != nullptr &&
        (layer.name.find("hbm_overlay") != std::string::npos)) {
        ret = true;
    }

    return ret;
}

bool SkiaGLRenderEngine::applyMiHBMRuntimeEffect(SkPaint& paint,
                                                 sk_sp<SkShader> shader,
                                                 const LayerSettings& layer,
                                                 bool needApplyHBMEffect,
                                                 float hbmLayerAlpha,
                                                 MiHBMEffect* miHBMEffect) {
    bool ret = false;

    if (miHBMEffect != nullptr &&
        needApplyHBMEffect &&
        !(layer.name.find("hbm_overlay") != std::string::npos) &&
        !(layer.name.find("gxzw_icon") != std::string::npos)
        ) {
        shader = miHBMEffect->createMiRuntimeEffectShader(layer, shader, hbmLayerAlpha);

        if (shader != nullptr) {
            ATRACE_NAME("DrawMiHBMEffect");
            paint.setShader(shader);
            ret = true;
        } else {
            ALOGE("%s failed!", __func__);
        }
    }

    return ret;
}

} // namespace skia
} // namespace renderengine
} // namespace Android
// MIUI ADD: END
#endif
