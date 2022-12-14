/*
 * Copyright (C) 2007 The Android Open Source Project
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
#include <gui/Surface.h>
#include <gui/LayerState.h>

namespace android {
struct MiuiDisplayState {

    enum {
        eIsLastFrameChanged  = 0x10,
        eIsScreenProjectionChanged  = 0x20,
        eIsCastProjectionChanged    = 0x40,
    };

    MiuiDisplayState();
    uint32_t what;
    sp<IBinder> token;
    sp<IGraphicBufferProducer> surface;

    uint32_t isScreenProjection;
    uint32_t isCastProjection;
    uint32_t isLastFrame;

    status_t write(Parcel& output) const;
    status_t read(const Parcel& input);
};

typedef enum {
    CAMERA_SCENE = 1,
    VIDEO_SCENE = 2,
    TP_IDLE_SCENE = 3,
    PROMOTION_SCENE = 4,
    AOD_SCENE = 5,
    SHORT_VIDEO_SCENE = 6,
    STATUSBAR_SCENE = 7,
    VSYNC_OPTIMIZE_SCENE = 8,
    FORCE_SKIP_IDLE_FOR_90HZ = 9,
    VIDEO_FULL_SCREEN_SCENE = 10,
    FORCE_SKIP_IDLE_FOR_AUTOMODE_120HZ = 11,
    MAP_QSYNC_SCENE = 12,
    FOD_SCENE = 13,
}scene_module_t;

enum {
    LAYER_REAL_UPDATE_SCENE_1 = 1,
    LAYER_REAL_UPDATE_SCENE_2 = 2,
};

enum {
    MI_CURRENT_SETTING_FPS_60HZ = 60,
    MI_CURRENT_SETTING_FPS_90HZ = 90,
    MI_CURRENT_SETTING_FPS_120HZ = 120,
};

typedef struct VideoDecoderInfo {
    int32_t state;
    uint32_t intfps;
    float floatfps;
    bool fromplayer;
    uint32_t tid;
}s_VideoDecoderInfo;

typedef struct VideoInfo {
    bool bulletchatState;
    float videofps;
    float compressionRatio;
    uint32_t width;
    uint32_t height;
}s_VideoInfo;

class IMiSurfaceComposerStub {
public:
    IMiSurfaceComposerStub() {}
    virtual ~IMiSurfaceComposerStub() {}

    virtual void setMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays,
                         uint32_t flags, int64_t desiredPresentTime,const sp<IBinder>& remote,
                         const String16& interfaceDescriptor,uint32_t code);
    virtual status_t setFrameRateVideoToDisplay(uint32_t cmd, Parcel *data,
                         const String16& interfaceDescriptor, const sp<IBinder>& remote, uint32_t code);
};

typedef IMiSurfaceComposerStub* CreateMiSurfaceComposerStub();
typedef void DestroyMiSurfaceComposerStub(IMiSurfaceComposerStub *);

} // namespace android

