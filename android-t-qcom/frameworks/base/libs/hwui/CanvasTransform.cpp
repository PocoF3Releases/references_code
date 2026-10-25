/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CanvasTransform.h"
#include "Properties.h"
#include "utils/Color.h"

#include <SkColorFilter.h>
#include <SkGradientShader.h>
#include <SkPaint.h>
#include <SkShader.h>

#include <algorithm>
#include <cmath>

#include <log/log.h>
#include <SkHighContrastFilter.h>

namespace android::uirenderer {

SkColor makeLight(SkColor color) {
    Lab lab = sRGBToLab(color);
    float invertedL = std::min(110 - lab.L, 100.0f);
    if (invertedL > lab.L) {
        lab.L = invertedL;
        return LabToSRGB(lab, SkColorGetA(color));
    } else {
        return color;
    }
}

SkColor makeDark(SkColor color) {
    Lab lab = sRGBToLab(color);
    float invertedL = std::min(110 - lab.L, 100.0f);
    if (invertedL < lab.L) {
        lab.L = invertedL;
        return LabToSRGB(lab, SkColorGetA(color));
    } else {
        return color;
    }
}

// MIUI ADD: START
static SkColor makeInvert(SkColor color) {
    Lab lab = sRGBToLab(color);
    lab.L = std::min(110 - lab.L, 100.0f);
    return LabToSRGB(lab, SkColorGetA(color));
}
// END

SkColor transformColor(ColorTransform transform, SkColor color) {
    switch (transform) {
        case ColorTransform::Light:
            return makeLight(color);
        case ColorTransform::Dark:
            return makeDark(color);
        // MIUI ADD START:
        case ColorTransform::ForceInvert:
        case ColorTransform::InvertExcludeBitmap:
            return makeInvert(color);
        case ColorTransform::BigView:
            return makeLight(color);
        // END
        default:
            return color;
    }
}

SkColor transformColorInverse(ColorTransform transform, SkColor color) {
    switch (transform) {
        case ColorTransform::Dark:
            return makeLight(color);
        case ColorTransform::Light:
            return makeDark(color);
        default:
            return color;
    }
}

// MIUI ADD: START
class MinMaxAverage {
public:
    void add(float sample) {
        if (mCount == 0) {
            mMin = sample;
            mMax = sample;
        } else {
            mMin = std::min(mMin, sample);
            mMax = std::max(mMax, sample);
        }
        mTotal += sample;
        mCount++;
    }

    float average() { return mTotal / mCount; }

    float min() { return mMin; }

    float max() { return mMax; }

    float delta() { return mMax - mMin; }

private:
    float mMin = 0.0f;
    float mMax = 0.0f;
    float mTotal = 0.0f;
    int mCount = 0;
};
// END

static void applyColorTransform(ColorTransform transform, SkPaint& paint) {
    if (transform == ColorTransform::None) return;
    // MIUI ADD: START
    if (transform == ColorTransform::ForceTransparent) {
        float matrix[20] = { 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0 };
        paint.setColorFilter(SkColorFilters::Matrix(matrix));
        return;
    }
    // END
    SkColor newColor = transformColor(transform, paint.getColor());
    paint.setColor(newColor);

    if (paint.getShader()) {
        SkShader::GradientInfo info;
        std::array<SkColor, 10> _colorStorage;
        std::array<SkScalar, _colorStorage.size()> _offsetStorage;
        info.fColorCount = _colorStorage.size();
        info.fColors = _colorStorage.data();
        info.fColorOffsets = _offsetStorage.data();
        SkShader::GradientType type = paint.getShader()->asAGradient(&info);

        if (info.fColorCount <= 10) {
            // MIUI ADD
            bool shouldInvert = false;
            switch (type) {
                case SkShader::kLinear_GradientType: {
                    for (int i = 0; i < info.fColorCount; i++) {
                        info.fColors[i] = transformColor(transform, info.fColors[i]);
                    }
                    paint.setShader(SkGradientShader::MakeLinear(info.fPoint, info.fColors,
                                                                 info.fColorOffsets, info.fColorCount,
                                                                 info.fTileMode, info.fGradientFlags, nullptr));
                    break;
                }
                // MIUI MOD: START
                // default:break;
                case SkShader::kNone_GradientType: {
                    if (MiuiForceDarkConfigStub::getMainRule() & MiuiForceDarkConfigStub::RULE_DEPRECATED) {
                        if (SkImage* image = paint.getShader()->isAImage(nullptr, nullptr)) {
                            // BitmapShader
                            SkPixmap pixmap;
                            image->peekPixels(&pixmap);
                            MinMaxAverage hue, saturation, value;
                            const int x_step = std::max(1, pixmap.width() / 10);
                            const int y_step = std::max(1, pixmap.height() / 10);
                            for (int x = 0; x < pixmap.width(); x += x_step) {
                                for (int y = 0; y < pixmap.height(); y += y_step) {
                                    SkColor color = pixmap.getColor(x, y);
                                    if (SkColorGetA(color) < 75) {
                                        continue;
                                    }
                                    float hsv[3];
                                    SkColorToHSV(color, hsv);
                                    hue.add(hsv[0]);
                                    saturation.add(hsv[1]);
                                    value.add(hsv[2]);
                                }
                            }
                            if (hue.delta() <= 20 && saturation.delta() <= .1f){
                                if (value.average() >= .5f && transform == ColorTransform::Dark){
                                    shouldInvert = true;
                                }
                            }
                        }
                    }
                }
                default: {
                    if (transform == ColorTransform::ForceInvert) {
                        shouldInvert = true;
                    }
                    if (shouldInvert) {
                        SkHighContrastConfig config;
                        config.fInvertStyle = SkHighContrastConfig::InvertStyle::kInvertLightness;
                        paint.setColorFilter(SkHighContrastFilter::Make(config)->
                                             makeComposed(paint.refColorFilter()));
                    }
                    break;
                }
                // END
            }
        }
    }

    if (paint.getColorFilter()) {
        SkBlendMode mode;
        SkColor color;
        // TODO: LRU this or something to avoid spamming new color mode filters
        if (paint.getColorFilter()->asAColorMode(&color, &mode)) {
            color = transformColor(transform, color);
            paint.setColorFilter(SkColorFilters::Blend(color, mode));
        }
    }
}

static BitmapPalette paletteForColorHSV(SkColor color) {
    float hsv[3];
    SkColorToHSV(color, hsv);
    return hsv[2] >= .5f ? BitmapPalette::Light : BitmapPalette::Dark;
}

static BitmapPalette filterPalette(const SkPaint* paint, BitmapPalette palette) {
    // MIUI ADD: START
    if (palette == BitmapPalette::NoInvert) {
        return palette;
    }
    // END
    if (palette == BitmapPalette::Unknown || !paint || !paint->getColorFilter()) {
        return palette;
    }

    SkColor color = palette == BitmapPalette::Light ? SK_ColorWHITE : SK_ColorBLACK;
    color = paint->getColorFilter()->filterColor(color);
    return paletteForColorHSV(color);
}

bool transformPaint(ColorTransform transform, SkPaint* paint) {
    // TODO
    applyColorTransform(transform, *paint);
    return true;
}

bool transformPaint(ColorTransform transform, SkPaint* paint, BitmapPalette palette) {
    palette = filterPalette(paint, palette);
    bool shouldInvert = false;
    if (palette == BitmapPalette::Light && transform == ColorTransform::Dark) {
        shouldInvert = true;
    }
    if (palette == BitmapPalette::Dark && transform == ColorTransform::Light) {
        shouldInvert = true;
    }
    // MIUI ADD: START
    if(palette == BitmapPalette::NoInvert) {
        shouldInvert = false;
    }
    if (transform == ColorTransform::ForceInvert) {
        shouldInvert = true;
    }

    if (transform == ColorTransform::ForceTransparent) {
        float matrix[20] = { 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0 };
        paint->setColorFilter(SkColorFilters::Matrix(matrix));
        return true;
    }
    // END
    if (shouldInvert) {
        SkHighContrastConfig config;
        config.fInvertStyle = SkHighContrastConfig::InvertStyle::kInvertLightness;
        paint->setColorFilter(SkHighContrastFilter::Make(config)->makeComposed(paint->refColorFilter()));
    }
    return shouldInvert;
}

}  // namespace android::uirenderer
