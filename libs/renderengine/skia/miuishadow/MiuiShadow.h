#pragma once

#include <SkShadowUtils.h>
#include <SkCanvas.h>
#include <SkRRect.h>
#include <SkColor.h>
#include <SkPath.h>
#include <renderengine/RenderEngine.h>
#include "GrContextOptions.h"
#include "../filters/BlurFilter.h"

namespace android {
namespace renderengine {
namespace skia {
void drawBlurShadow(SkCanvas* canvas, const SkRRect& casterRRect,
                    const MiuiShadowSettings& settings, BlurFilter* blurFilter,
                    GrRecordingContext* grContext, const sk_sp<SkSurface> surface);
void drawAtmosphereLamp(SkCanvas* canvas, const SkRRect& casterRRect,
                        const MiuiShadowSettings& settings, BlurFilter* blurFilter,
                        GrRecordingContext* grContext, const sk_sp<SkImage> blurInput);
void drawMiuiShadow(SkCanvas* canvas, const SkRRect& casterRRect,
                    const MiuiShadowSettings& settings);

} // namespace skia
} // namespace renderengine
} // namespace android