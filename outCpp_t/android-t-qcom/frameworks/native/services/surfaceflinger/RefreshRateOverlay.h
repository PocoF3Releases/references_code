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

#pragma once

#include <SkColor.h>
#include <vector>

#include <ftl/small_map.h>
#include <ui/LayerStack.h>
#include <ui/Size.h>
#include <ui/Transform.h>
#include <utils/StrongPointer.h>

#include <scheduler/Fps.h>

class SkCanvas;

namespace android {

class GraphicBuffer;
class SurfaceControl;

class RefreshRateOverlay {
public:
    RefreshRateOverlay(FpsRange, bool showSpinner);

    void setLayerStack(ui::LayerStack);
    void setViewport(ui::Size);
    void changeRefreshRate(Fps, bool showAuto = 0);
    void animate();

private:
    using Buffers = std::vector<sp<GraphicBuffer>>;

    class SevenSegmentDrawer {
    public:
        static Buffers draw(int number, SkColor, ui::Transform::RotationFlags, bool showSpinner, bool showAuto);

    private:
        enum class Segment { Upper, UpperLeft, UpperRight, UpperMiddle, Middle, LowerLeft, LowerRight, LowerMiddle, Bottom };

        static void drawSegment(Segment, int left, SkColor, SkCanvas&);
        static void drawDigit(int digit, int left, SkColor, SkCanvas&);
#ifdef MI_FEATURE_ENABLE
        static void drawAuto(int left, SkColor color, SkCanvas& canvas);
#endif
    };

    const Buffers& getOrCreateBuffers(Fps);

    struct Key {
        int fps;
        ui::Transform::RotationFlags flags;
        bool showAuto;

        bool operator==(Key other) const { return fps == other.fps && flags == other.flags && showAuto == other.showAuto; }
    };

    using BufferCache = ftl::SmallMap<Key, Buffers, 9>;
    BufferCache mBufferCache;

    std::optional<Fps> mCurrentFps;
    size_t mFrame = 0;

    const FpsRange mFpsRange; // For color interpolation.
    const bool mShowSpinner;
    bool mShowAuto;

    const sp<SurfaceControl> mSurfaceControl;
};

} // namespace android
