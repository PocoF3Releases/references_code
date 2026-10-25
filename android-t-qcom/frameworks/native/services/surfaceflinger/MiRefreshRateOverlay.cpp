/*
 * Copyright 2019 The Android Open Source Project
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

#include <algorithm>

#include "MiRefreshRateOverlay.h"
#include "Client.h"
#include "Layer.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#include <SkCanvas.h>
#include <SkPaint.h>
#pragma clang diagnostic pop
#include <SkBlendMode.h>
#include <SkRect.h>
#include <SkSurface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceControl.h>

#include "MiSurfaceFlingerStub.h"

#undef LOG_TAG
#define LOG_TAG "MiRefreshRateOverlay"

namespace android {
namespace {

constexpr int kMiDigitWidth = 64;
constexpr int kMiDigitHeight = 100;
constexpr int kMiDigitSpace = 16;

// Layout is A, space, U, space, T, space, O.
constexpr int kMiBufferWidth = 4 * kMiDigitWidth + 3 * kMiDigitSpace;
constexpr int kMiBufferHeight = kMiDigitHeight;

SurfaceComposerClient::Transaction miCreateTransaction(const sp<SurfaceControl>& surface) {
    constexpr float kFrameRate = 0.f;
    constexpr int8_t kCompatibility = ANATIVEWINDOW_FRAME_RATE_NO_VOTE;
    constexpr int8_t kSeamlessness = ANATIVEWINDOW_CHANGE_FRAME_RATE_ONLY_IF_SEAMLESS;

    return SurfaceComposerClient::Transaction().setFrameRate(surface, kFrameRate, kCompatibility,
                                                             kSeamlessness);
}

} // namespace

void MiRefreshRateOverlay::MiSevenSegmentDrawer::drawSegment(Segment segment, int left, SkColor color,
                                                         SkCanvas& canvas) {
    const SkRect rect = [&]() {
        switch (segment) {
            case Segment::Upper:
                return SkRect::MakeLTRB(left, 0, left + kMiDigitWidth, kMiDigitSpace);
            case Segment::UpperLeft:
                return SkRect::MakeLTRB(left, 0, left + kMiDigitSpace, kMiDigitHeight / 2);
            case Segment::UpperRight:
                return SkRect::MakeLTRB(left + kMiDigitWidth - kMiDigitSpace, 0, left + kMiDigitWidth,
                                        kMiDigitHeight / 2);
            case Segment::UpperMiddle:
                return SkRect::MakeLTRB(left + kMiDigitWidth / 2 - kMiDigitSpace / 2, 0,
                                        left + kMiDigitWidth / 2 + kMiDigitSpace / 2, kMiDigitHeight / 2);
            case Segment::Middle:
                return SkRect::MakeLTRB(left, kMiDigitHeight / 2 - kMiDigitSpace / 2,
                                        left + kMiDigitWidth, kMiDigitHeight / 2 + kMiDigitSpace / 2);
            case Segment::LowerLeft:
                return SkRect::MakeLTRB(left, kMiDigitHeight / 2, left + kMiDigitSpace, kMiDigitHeight);
            case Segment::LowerRight:
                return SkRect::MakeLTRB(left + kMiDigitWidth - kMiDigitSpace, kMiDigitHeight / 2,
                                        left + kMiDigitWidth, kMiDigitHeight);
            case Segment::LowerMiddle:
                return SkRect::MakeLTRB(left + kMiDigitWidth / 2 - kMiDigitSpace / 2, kMiDigitHeight / 2,
                                        left + kMiDigitWidth / 2 + kMiDigitSpace / 2, kMiDigitHeight);
            case Segment::Bottom:
                return SkRect::MakeLTRB(left, kMiDigitHeight - kMiDigitSpace, left + kMiDigitWidth,
                                        kMiDigitHeight);
        }
    }();

    SkPaint paint;
    paint.setColor(color);
    paint.setBlendMode(SkBlendMode::kSrc);
    canvas.drawRect(rect, paint);
}

void MiRefreshRateOverlay::MiSevenSegmentDrawer::drawDigit(int digit, int left, SkColor color,
                                                       SkCanvas& canvas) {
    if (digit < 0 || digit > 9) return;

    if (digit == 0 || digit == 2 || digit == 3 || digit == 5 || digit == 6 || digit == 7 ||
        digit == 8 || digit == 9)
        drawSegment(Segment::Upper, left, color, canvas);
    if (digit == 0 || digit == 4 || digit == 5 || digit == 6 || digit == 8 || digit == 9)
        drawSegment(Segment::UpperLeft, left, color, canvas);
    if (digit == 0 || digit == 1 || digit == 2 || digit == 3 || digit == 4 || digit == 7 ||
        digit == 8 || digit == 9)
        drawSegment(Segment::UpperRight, left, color, canvas);
    if (digit == 2 || digit == 3 || digit == 4 || digit == 5 || digit == 6 || digit == 8 ||
        digit == 9)
        drawSegment(Segment::Middle, left, color, canvas);
    if (digit == 0 || digit == 2 || digit == 6 || digit == 8)
        drawSegment(Segment::LowerLeft, left, color, canvas);
    if (digit == 0 || digit == 1 || digit == 3 || digit == 4 || digit == 5 || digit == 6 ||
        digit == 7 || digit == 8 || digit == 9)
        drawSegment(Segment::LowerRight, left, color, canvas);
    if (digit == 0 || digit == 2 || digit == 3 || digit == 5 || digit == 6 || digit == 8 ||
        digit == 9)
        drawSegment(Segment::Bottom, left, color, canvas);
}

auto MiRefreshRateOverlay::MiSevenSegmentDrawer::draw(int number, SkColor color,
                                 ui::Transform::RotationFlags rotation) -> Buffers {
    if (number < 0 || number > 1000) return {};

    const auto hundreds = number / 100;
    const auto tens = (number / 10) % 10;
    const auto ones = number % 10;

    const size_t loopCount = 1;

    Buffers buffers;
    buffers.reserve(loopCount);

    for (size_t i = 0; i < loopCount; i++) {
        // Pre-rotate the buffer before it reaches SurfaceFlinger.
        SkMatrix canvasTransform = SkMatrix();
        const auto [bufferWidth, bufferHeight] = [&]() -> std::pair<int, int> {
            switch (rotation) {
                case ui::Transform::ROT_90:
                    canvasTransform.setTranslate(kMiBufferHeight, 0);
                    canvasTransform.preRotate(90.f);
                    return {kMiBufferHeight, kMiBufferWidth};
                case ui::Transform::ROT_270:
                    canvasTransform.setRotate(270.f, kMiBufferWidth / 2.f, kMiBufferWidth / 2.f);
                    return {kMiBufferHeight, kMiBufferWidth};
                default:
                    return {kMiBufferWidth, kMiBufferHeight};
            }
        }();

        sp<GraphicBuffer> buffer =
                new GraphicBuffer(static_cast<uint32_t>(bufferWidth),
                                  static_cast<uint32_t>(bufferHeight), HAL_PIXEL_FORMAT_RGBA_8888,
                                  1,
                                  GRALLOC_USAGE_SW_WRITE_RARELY | GRALLOC_USAGE_HW_COMPOSER |
                                          GRALLOC_USAGE_HW_TEXTURE,
                                  "MiRefreshRateOverlayBuffer");

        const status_t bufferStatus = buffer->initCheck();
        LOG_ALWAYS_FATAL_IF(bufferStatus != OK, "RefreshRateOverlay: Buffer failed to allocate: %d",
                            bufferStatus);

        sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(bufferWidth, bufferHeight);
        SkCanvas* canvas = surface->getCanvas();
        canvas->setMatrix(canvasTransform);

        int left = 0;
        if (hundreds != 0) {
            drawDigit(hundreds, left, color, *canvas);
        }
        left += kMiDigitWidth + kMiDigitSpace;

        if (hundreds != 0 || tens != 0) {
            drawDigit(tens, left, color, *canvas);
        }
        left += kMiDigitWidth + kMiDigitSpace;

        drawDigit(ones, left, color, *canvas);
        left += kMiDigitWidth + kMiDigitSpace;

        void* pixels = nullptr;
        buffer->lock(GRALLOC_USAGE_SW_WRITE_RARELY, reinterpret_cast<void**>(&pixels));

        const SkImageInfo& imageInfo = surface->imageInfo();
        const size_t dstRowBytes =
                buffer->getStride() * static_cast<size_t>(imageInfo.bytesPerPixel());

        canvas->readPixels(imageInfo, pixels, dstRowBytes, 0, 0);
        buffer->unlock();
        buffers.push_back(std::move(buffer));
    }
    return buffers;
}

auto MiRefreshRateOverlay::MiSevenSegmentDrawer::drawBuffer(Buffers buffers, int number, SkColor color,
                                 ui::Transform::RotationFlags rotation) -> Buffers {
    if (number < 0 || number > 1000) return {};

    const auto hundreds = number / 100;
    const auto tens = (number / 10) % 10;
    const auto ones = number % 10;

    const size_t loopCount = 1;

    buffers.reserve(loopCount);

    for (size_t i = 0; i < loopCount; i++) {
        // Pre-rotate the buffer before it reaches SurfaceFlinger.
        SkMatrix canvasTransform = SkMatrix();
        const auto [bufferWidth, bufferHeight] = [&]() -> std::pair<int, int> {
            switch (rotation) {
                case ui::Transform::ROT_90:
                    canvasTransform.setTranslate(kMiBufferHeight, 0);
                    canvasTransform.preRotate(90.f);
                    return {kMiBufferHeight, kMiBufferWidth};
                case ui::Transform::ROT_270:
                    canvasTransform.setRotate(270.f, kMiBufferWidth / 2.f, kMiBufferWidth / 2.f);
                    return {kMiBufferHeight, kMiBufferWidth};
                default:
                    return {kMiBufferWidth, kMiBufferHeight};
            }
        }();

        sp<GraphicBuffer> buffer = buffers[i];

        sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(bufferWidth, bufferHeight);
        SkCanvas* canvas = surface->getCanvas();
        canvas->setMatrix(canvasTransform);

        int left = 0;
        if (hundreds != 0) {
            drawDigit(hundreds, left, color, *canvas);
        }
        left += kMiDigitWidth + kMiDigitSpace;

        if (hundreds != 0 || tens != 0) {
            drawDigit(tens, left, color, *canvas);
        }
        left += kMiDigitWidth + kMiDigitSpace;

        drawDigit(ones, left, color, *canvas);
        left += kMiDigitWidth + kMiDigitSpace;

        void* pixels = nullptr;
        buffer->lock(GRALLOC_USAGE_SW_WRITE_RARELY, reinterpret_cast<void**>(&pixels));

        const SkImageInfo& imageInfo = surface->imageInfo();
        const size_t dstRowBytes =
                buffer->getStride() * static_cast<size_t>(imageInfo.bytesPerPixel());

        canvas->readPixels(imageInfo, pixels, dstRowBytes, 0, 0);
        buffer->unlock();
    }
    return buffers;
}

MiRefreshRateOverlay::MiRefreshRateOverlay(FpsRange fpsRange)
      : mFpsRange(fpsRange),
        mSurfaceControl(SurfaceComposerClient::getDefault()
                                ->createSurface(String8("MiRefreshRateOverlay"), kMiBufferWidth,
                                                kMiBufferHeight, PIXEL_FORMAT_RGBA_8888,
                                                ISurfaceComposerClient::eFXSurfaceBufferState)) {
    if (!mSurfaceControl) {
        ALOGE("%s: Failed to create buffer state layer", __func__);
        return;
    }

    miCreateTransaction(mSurfaceControl)
            .setLayer(mSurfaceControl, INT32_MAX - 1)
            .setTrustedOverlay(mSurfaceControl, true)
            .apply();
}

auto MiRefreshRateOverlay::getOrCreateBuffers(int index, Fps fps) -> const Buffers& {
    static const Buffers kNoBuffers;
    if (!mSurfaceControl) return kNoBuffers;

    const auto transformHint =
            static_cast<ui::Transform::RotationFlags>(mSurfaceControl->getTransformHint());

    // Tell SurfaceFlinger about the pre-rotation on the buffer.
    const auto transform = [&] {
        switch (transformHint) {
            case ui::Transform::ROT_90:
                return ui::Transform::ROT_270;
            case ui::Transform::ROT_270:
                return ui::Transform::ROT_90;
            default:
                return ui::Transform::ROT_0;
        }
    }();

    miCreateTransaction(mSurfaceControl).setTransform(mSurfaceControl, transform).apply();

    const int minFps = mFpsRange.min.getIntValue();
    const int maxFps = mFpsRange.max.getIntValue();

    // Clamp to the range. The current fps may be outside of this range if the display has
    // changed its set of supported refresh rates.
    const int intFps = std::clamp(fps.getIntValue(), minFps, maxFps);

    // Ensure non-zero range to avoid division by zero.
    const float fpsScale = static_cast<float>(intFps - minFps) / std::max(1, maxFps - minFps);

    constexpr SkColor kMinFpsColor = SK_ColorRED;
    constexpr SkColor kMaxFpsColor = SK_ColorGREEN;
    constexpr float kAlpha = 0.8f;

    SkColor4f colorBase = SkColor4f::FromColor(kMaxFpsColor) * fpsScale;
    const SkColor4f minFpsColor = SkColor4f::FromColor(kMinFpsColor) * (1 - fpsScale);

    colorBase.fR = colorBase.fR + minFpsColor.fR;
    colorBase.fG = colorBase.fG + minFpsColor.fG;
    colorBase.fB = colorBase.fB + minFpsColor.fB;
    colorBase.fA = kAlpha;

    const SkColor color = colorBase.toSkColor();

    BufferCache::const_iterator it = mBufferCache.find({index, transformHint});
    if (it == mBufferCache.end()) {
        auto buffers = MiSevenSegmentDrawer::draw(intFps, color, transformHint);
        it = mBufferCache.try_emplace({index, transformHint}, std::move(buffers)).first;
    } else {
        MiSevenSegmentDrawer::drawBuffer(it->second, intFps, color, transformHint);
    }

    return it->second;
}

void MiRefreshRateOverlay::setViewport(ui::Size viewport) {
    constexpr int32_t kMaxWidth = 1000;
    const auto width = std::min({kMaxWidth, viewport.width, viewport.height});
    const auto height = 2 * width;
    Rect frame((3 * width) >> 4, height >> 5);
    frame.offsetBy(width >> 5, height >> 3);

    miCreateTransaction(mSurfaceControl)
            .setMatrix(mSurfaceControl, frame.getWidth() / static_cast<float>(kMiBufferWidth), 0, 0,
                       frame.getHeight() / static_cast<float>(kMiBufferHeight))
            .setPosition(mSurfaceControl, frame.left, frame.top)
            .apply();
}

void MiRefreshRateOverlay::setLayerStack(ui::LayerStack stack) {
    miCreateTransaction(mSurfaceControl).setLayerStack(mSurfaceControl, stack).apply();
}

void MiRefreshRateOverlay::changeRefreshRate(Fps fps, bool needShow) {
    mCurrentFps = fps;
    const auto& buffers = getOrCreateBuffers(mCurrentBufferIndex, fps);
    mCurrentBufferIndex = 1 - mCurrentBufferIndex;
    mFrame = (mFrame + 1) % buffers.size();
    const auto buffer = buffers[mFrame];
    bool isOpaque = needShow ? true : false;
    if (isOpaque) {
        miCreateTransaction(mSurfaceControl).setBuffer(mSurfaceControl, buffer).setAlpha(mSurfaceControl, 1).apply();
    } else if (mLastShowStatus != needShow && !isOpaque) {
        miCreateTransaction(mSurfaceControl).setBuffer(mSurfaceControl, buffer).setAlpha(mSurfaceControl, 0).apply();
    }
    mLastShowStatus = needShow;
}

} // namespace android
