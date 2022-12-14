/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <xf86drmMode.h>

#include "../graphics.h"
#include "minui/minui.h"

#define NUM_PLANES 2

struct Crtc {
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
    uint32_t mode_blob_id;
};

struct Connector {
    drmModeObjectProperties *props;
    drmModePropertyRes **props_info;
};

struct Plane {
    drmModePlane *plane;
    drmModeObjectProperties *props;
    drmModePropertyRes ** props_info;
};

class GRSurfaceDrmMi : public GRSurface {
public:
    ~GRSurfaceDrmMi() override;

    // Creates a GRSurfaceDrmMi instance.
    static std::unique_ptr<GRSurfaceDrmMi> Create(int drm_fd, int width, int height);

    uint8_t* data() override {
        return mmapped_buffer_;
    }

private:
    friend class MinuiBackendDrmMi;

    GRSurfaceDrmMi(size_t width, size_t height, size_t row_bytes, size_t pixel_bytes, int drm_fd,
                 uint32_t handle)
        : GRSurface(width, height, row_bytes, pixel_bytes), drm_fd_(drm_fd), handle(handle) {}

    const int drm_fd_;

    uint32_t fb_id{ 0 };
    uint32_t handle{ 0 };
    uint8_t* mmapped_buffer_{ nullptr };
};

class MinuiBackendDrmMi : public MinuiBackend {
public:
    MinuiBackendDrmMi() {mDisplayId = DISP_PRIMARY;}
    MinuiBackendDrmMi(int displayId);
    ~MinuiBackendDrmMi() override;

    GRSurface* Init() override;
    GRSurface* Flip() override;
    void Blank(bool) override;
    void ReleaseMemory();
    static void Create();
    static void Destroy();

private:
    enum screen_side{Left, Right};
    drmModeConnector* FindMainMonitor(int fd, int displayId, drmModeRes* resources, uint32_t* mode_index);
    int SetupPipeline(drmModeAtomicReqPtr atomic_req);
    int TeardownPipeline(drmModeAtomicReqPtr atomic_req);
    void UpdatePlaneFB();
    int AtomicPopulatePlane(int plane, drmModeAtomicReqPtr atomic_req);

    static int mDrmFd;
    int mDisplayId;
    int mFBPropId;
    uint32_t mLMs;
    drmModeCrtc* mMonitorcrtc{ nullptr };
    drmModeConnector* mMonitorconnector{ nullptr };

    std::unique_ptr<GRSurfaceDrmMi> mGRSurfaceDrms[2];
    int mCurrentBufferIndex{ 0 };

    struct Crtc mCrtcRes;
    struct Connector mConnRes;
    struct Plane mPlaneRes[NUM_PLANES];

    bool mCurrentBlankState = true;
};
