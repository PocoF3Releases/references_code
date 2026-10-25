#include "MiuiShadow.h"

namespace android {
namespace renderengine {
namespace skia {

inline SkPoint3 getSkPoint3(const vec3& vector) {
    return SkPoint3::Make(vector.x, vector.y, vector.z);
}

inline SkColor getSkColor(const vec4& color) {
    return SkColorSetARGB(color.a * 255, color.r * 255, color.g * 255, color.b * 255);
}

void drawBlurShadow(SkCanvas* canvas, const SkRRect& casterRRect,
                    const MiuiShadowSettings& settings, BlurFilter* blurFilter,
                    GrRecordingContext* grContext, const sk_sp<SkSurface> surface) {
    sk_sp<SkImage> blurInput = surface->makeImageSnapshot();
    if (blurInput == nullptr) {
        ALOGE("MakeImageSnapshot failed!");
        return;
    }
    auto rrect = casterRRect;
    float offsetX = settings.offsetX;
    float offsetY = settings.offsetY;
    float outset = settings.outset;
    rrect.offset(offsetX, offsetY);
    if (outset < 0.0) {
        outset = 0.0;
    }
    rrect.outset(outset, outset);
    auto blurRect = canvas->getTotalMatrix().mapRect(rrect.rect());
    blurFilter->drawBlurShadow(grContext, canvas, rrect, blurRect, blurInput);
}

void drawAtmosphereLamp(SkCanvas* canvas, const SkRRect& casterRRect,
                    const MiuiShadowSettings& settings, BlurFilter* blurFilter,
                    GrRecordingContext* grContext, const sk_sp<SkImage> blurInput) {
    auto tempRrect = casterRRect;
    float offsetX = settings.offsetX;
    float offsetY = settings.offsetY;
    float outset = settings.outset;
    tempRrect.outset(outset, outset);
    if (outset < 0.0) {
        outset = 0.0;
    }
    tempRrect.offset(offsetX, offsetY);
    auto blurShadowRect = canvas->getTotalMatrix().mapRect(tempRrect.rect());
    blurFilter->drawAtmosphereLamp(grContext, canvas, tempRrect, blurShadowRect, blurInput);
}

void drawMiuiShadow(SkCanvas* canvas, const SkRRect& casterRRect,
                    const MiuiShadowSettings& settings) {
    if (settings.shadowType == 1) {
        const float casterZ = settings.length / 2.f;
        const auto flags =
                settings.casterIsTranslucent ? kTransparentOccluder_ShadowFlag : kNone_ShadowFlag;

        float offsetX = settings.offsetX;
        float offsetY = settings.offsetY;
        float outset = settings.outset;
        uint32_t numOfLayers = settings.numOfLayers;
        float biasX = offsetX / numOfLayers;
        float biasY = offsetY / numOfLayers;
        auto color = settings.color;
        auto rect = casterRRect.rect();
        auto rrect = casterRRect;
        if (outset < 0) {
            rect.outset(outset, outset);
            rrect = SkRRect::MakeRectXY(rect,
                        rrect.getSimpleRadii().x(),
                        rrect.getSimpleRadii().y());
        } else {
            rrect.outset(outset, outset);
        }
        color[3] /= pow(numOfLayers, 0.5);
        for (int i = 0; i < numOfLayers; ++i) {
            auto tmpRect = rrect;
            tmpRect.offset(biasX*(i+1), biasY*(i+1));

            SkShadowUtils::DrawShadow(canvas, SkPath::RRect(tmpRect), SkPoint3::Make(0, 0, casterZ),
                                        getSkPoint3(settings.lightPos), settings.lightRadius,
                                        getSkColor(color), getSkColor(color), flags);
        }
    } else if (settings.shadowType == 2) {
        const float casterZ = settings.length / 2.f;
        const auto flags =
                settings.casterIsTranslucent ? kTransparentOccluder_ShadowFlag : kNone_ShadowFlag;
        float outset = settings.outset;
        auto rrect = casterRRect;
        auto color = settings.color;
        rrect.outset(outset, outset);
        SkShadowUtils::DrawShadow(canvas, SkPath::RRect(rrect), SkPoint3::Make(0, 0, casterZ),
                                getSkPoint3(settings.lightPos), settings.lightRadius,
                                getSkColor(color), getSkColor(color), flags);
    }
}

} // namespace skia
} // namespace renderengine
} // namespace android