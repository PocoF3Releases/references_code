// MIUI ADD: This file is added just for Global HBM Dither (J2S-T)
#ifndef __MI_HBM_EFFECT__H__
#define __MI_HBM_EFFECT__H__

#include <SkImage.h>
#include <SkRuntimeEffect.h>
#include <utils/Trace.h>
#include <log/log.h>

#include "renderengine/LayerSettings.h"

using namespace std;

namespace android {
namespace renderengine {
namespace skia {

class MiHBMEffect {
public:
    void initPatternSkImage();
    sk_sp<SkRuntimeEffect> buildMiRuntimeEffect(bool workAroundChange);
    sk_sp<SkShader> createMiRuntimeEffectShader(const LayerSettings& layer,
                                                sk_sp<SkShader> shader,
                                                const float hbmLayerAlpha);

private:
    bool mCanApplyPattern = false;
    float factorCoeff = 0.015625f;
    sk_sp<SkImage> mPatternSkImage = nullptr;
};

} // namespace skia
} // namespace renderengine
} // namespace android

#endif // __MI_HBM_EFFECT__H__
// MIUI ADD: END
