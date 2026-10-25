/*
 * Copyright (C) 2015 The Android Open Source Project
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

/*
 * DRM based mode setting test program
 * Copyright 2008 Tungsten Graphics
 *   Jakob Bornecrantz <jakob@tungstengraphics.com>
 * Copyright 2008 Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>

#include <android-base/macros.h>
#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <string>
#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <sstream>

#include "graphics_drm_mi.h"

#define find_prop_id(_res, type, Type, obj_id, prop_name, prop_id)    \
    do {                                                                \
        int j = 0;                                                        \
        int prop_count = 0;                                               \
        struct Type *obj = NULL;                                          \
        obj = (_res);                                                     \
        if (!obj || mMonitor##type->type##_id != (obj_id)){          \
            prop_id = 0;                                                    \
            break;                                                          \
        }                                                                 \
        prop_count = (int)obj->props->count_props;                        \
        for (j = 0; j < prop_count; ++j)                                  \
            if (!strcmp(obj->props_info[j]->name, (prop_name)))             \
                break;                                                        \
        (prop_id) = (j == prop_count)?                                    \
          0 : obj->props_info[j]->prop_id;                                \
    } while (0)

#define add_prop(res, type, Type, id, id_name, id_val) \
    find_prop_id(res, type, Type, id, id_name, prop_id); \
    if (prop_id)                                         \
        drmModeAtomicAddProperty(atomic_req, id, prop_id, id_val);

/**
 * enum sde_rm_topology_name - HW resource use case in use by connector
 * @SDE_RM_TOPOLOGY_NONE:                 No topology in use currently
 * @SDE_RM_TOPOLOGY_SINGLEPIPE:           1 LM, 1 PP, 1 INTF/WB
 * @SDE_RM_TOPOLOGY_SINGLEPIPE_DSC:       1 LM, 1 DSC, 1 PP, 1 INTF/WB
 * @SDE_RM_TOPOLOGY_DUALPIPE:             2 LM, 2 PP, 2 INTF/WB
 * @SDE_RM_TOPOLOGY_DUALPIPE_DSC:         2 LM, 2 DSC, 2 PP, 2 INTF/WB
 * @SDE_RM_TOPOLOGY_DUALPIPE_3DMERGE:     2 LM, 2 PP, 3DMux, 1 INTF/WB
 * @SDE_RM_TOPOLOGY_DUALPIPE_3DMERGE_DSC: 2 LM, 2 PP, 3DMux, 1 DSC, 1 INTF/WB
 * @SDE_RM_TOPOLOGY_DUALPIPE_DSCMERGE:    2 LM, 2 PP, 2 DSC Merge, 1 INTF/WB
 * @SDE_RM_TOPOLOGY_PPSPLIT:              1 LM, 2 PPs, 2 INTF/WB
 */

static uint32_t get_lm_number(const std::string &topology) {
    if (topology == "sde_singlepipe") return 1;
    if (topology == "sde_singlepipe_dsc") return 1;
    if (topology == "sde_dualpipe") return 2;
    if (topology == "sde_dualpipe_dsc") return 2;
    if (topology == "sde_dualpipemerge") return 2;
    if (topology == "sde_dualpipemerge_dsc") return 2;
    if (topology == "sde_dualpipe_dscmerge") return 2;
    if (topology == "sde_ppsplit") return 1;
    return 2;
}

static uint32_t get_topology_lm_number(int fd, uint32_t blob_id) {
    uint32_t num_lm = 2;

    drmModePropertyBlobRes *blob = drmModeGetPropertyBlob(fd, blob_id);
    if (!blob) {
      return num_lm;
    }

    const char *fmt_str = (const char *)(blob->data);
    std::stringstream stream(fmt_str);
    std::string line = {};
    const std::string topology = "topology=";

    while (std::getline(stream, line)) {
      if (line.find(topology) != std::string::npos) {
          num_lm = get_lm_number(std::string(line, topology.length()));
      }
    }

    drmModeFreePropertyBlob(blob);
    return num_lm;
}

static int find_plane_prop_id(uint32_t obj_id, const char *prop_name,
                              Plane *plane_res) {
    int i, j = 0;
    int prop_count = 0;
    struct Plane *obj = NULL;

    for (i = 0; i < NUM_PLANES; ++i) {
        obj = &plane_res[i];
        if (!obj || obj->plane->plane_id != obj_id)
            continue;
        prop_count = (int)obj->props->count_props;
        for (j = 0; j < prop_count; ++j)
            if (!strcmp(obj->props_info[j]->name, prop_name))
               return obj->props_info[j]->prop_id;
        break;
    }

  return 0;
}

static int atomic_add_prop_to_plane(Plane *plane_res, drmModeAtomicReq *req,
                                    uint32_t obj_id, const char *prop_name,
                                    uint64_t value) {
    uint32_t prop_id;

    prop_id = find_plane_prop_id(obj_id, prop_name, plane_res);
    if (prop_id == 0) {
        printf("Could not find obj_id = %d\n", obj_id);
        return -EINVAL;
    }

    if (drmModeAtomicAddProperty(req, obj_id, prop_id, value) < 0) {
        printf("Could not add prop_id = %d for obj_id %d\n",
                prop_id, obj_id);
        return -EINVAL;
    }

    return 0;
}

static drmModeCrtc* find_crtc_for_connector(int fd, int displayId,
                drmModeRes* resources, drmModeConnector* connector) {
    int32_t crtc = -1;
    int32_t step = 0;

    // Find the encoder. If we already have one, just use it.
    drmModeEncoder* encoder;
    if (connector->encoder_id) {
        encoder = drmModeGetEncoder(fd, connector->encoder_id);
    } else {
        encoder = nullptr;
    }

    if (encoder && encoder->crtc_id) {
        crtc = encoder->crtc_id;
        drmModeFreeEncoder(encoder);
        return drmModeGetCrtc(fd, crtc);
    }

    // Didn't find anything, try to find a crtc and encoder combo.
    for (int i = 0; i < connector->count_encoders; i++) {
        encoder = drmModeGetEncoder(fd, connector->encoders[i]);

        if (encoder) {
            step = displayId ? 1 : 0;
            for (int j = step; j < resources->count_crtcs; j++) {
				/* Now the qualcom's possible_crtcs is compatible for all crtcs*/
                if (!(encoder->possible_crtcs & (1 << j))) continue;
                crtc = resources->crtcs[j];
                break;
            }
            if (crtc >= 0) {
                drmModeFreeEncoder(encoder);
                return drmModeGetCrtc(fd, crtc);
            }
        }
    }

    return nullptr;
}

static drmModeConnector* find_used_connector_by_type(int fd, drmModeRes* resources, unsigned type) {
    for (int i = 0; i < resources->count_connectors; i++) {
        drmModeConnector* connector = drmModeGetConnector(fd, resources->connectors[i]);
        if (connector) {
            if ((connector->connector_type == type) && (connector->connection == DRM_MODE_CONNECTED) &&
                (connector->count_modes > 0)) {
                return connector;
            }
            drmModeFreeConnector(connector);
        }
    }

    return nullptr;
}

static drmModeConnector* find_first_connected_connector(int fd, drmModeRes* resources) {
    for (int i = 0; i < resources->count_connectors; i++) {
        drmModeConnector* connector;

        connector = drmModeGetConnector(fd, resources->connectors[i]);
        if (connector) {
            if ((connector->count_modes > 0) && (connector->connection == DRM_MODE_CONNECTED))
                return connector;

            drmModeFreeConnector(connector);
        }
    }

    return nullptr;
}

static drmModeConnector* find_buidin_connected_connector(int fd, int displayId,
                drmModeRes* resources) {
    int conn_connected_num = 0;

    for (int i = 0; i < resources->count_connectors; i++) {
        drmModeConnector* connector;

        connector = drmModeGetConnector(fd, resources->connectors[i]);
        if (connector) {
            if ((connector->count_modes > 0) && (connector->connection == DRM_MODE_CONNECTED)
                && connector->connector_type == DRM_MODE_CONNECTOR_DSI) {
                conn_connected_num++;
            }

            if ((!displayId && conn_connected_num > 0)
                || (displayId && conn_connected_num > 1))
                return connector;

            drmModeFreeConnector(connector);
        }
    }

    return nullptr;
}

static int drm_format_to_bpp(uint32_t format) {
    switch (format) {
        case DRM_FORMAT_ABGR8888:
        case DRM_FORMAT_BGRA8888:
        case DRM_FORMAT_RGBX8888:
        case DRM_FORMAT_RGBA8888:
        case DRM_FORMAT_ARGB8888:
        case DRM_FORMAT_BGRX8888:
        case DRM_FORMAT_XBGR8888:
        case DRM_FORMAT_XRGB8888:
            return 32;
        case DRM_FORMAT_RGB565:
            return 16;
        default:
            printf("Unknown format %d\n", format);
            return 32;
    }
}

GRSurfaceDrmMi::~GRSurfaceDrmMi() {
    if (mmapped_buffer_) {
        munmap(mmapped_buffer_, row_bytes * height);
    }

    if (fb_id) {
        if (drmModeRmFB(drm_fd_, fb_id) != 0) {
            perror("Failed to drmModeRmFB");
            // Falling through to free other resources.
        }
    }

    if (handle) {
        drm_gem_close gem_close = {};
        gem_close.handle = handle;

        if (drmIoctl(drm_fd_, DRM_IOCTL_GEM_CLOSE, &gem_close) != 0) {
            perror("Failed to DRM_IOCTL_GEM_CLOSE");
        }
    }
}

std::unique_ptr<GRSurfaceDrmMi> GRSurfaceDrmMi::Create(int drm_fd, int width, int height) {
    uint32_t format;
    PixelFormat pixel_format = gr_pixel_format();
    // PixelFormat comes in byte order, whereas DRM_FORMAT_* uses little-endian
    // (external/libdrm/include/drm/drm_fourcc.h). Note that although drm_fourcc.h also defines a
    // macro of DRM_FORMAT_BIG_ENDIAN, it doesn't seem to be actually supported (see the discussion
    // in https://lists.freedesktop.org/archives/amd-gfx/2017-May/008560.html).
    if (pixel_format == PixelFormat::ABGR) {
        format = DRM_FORMAT_RGBA8888;
    } else if (pixel_format == PixelFormat::BGRA) {
        format = DRM_FORMAT_ARGB8888;
    } else if (pixel_format == PixelFormat::RGBX) {
        format = DRM_FORMAT_XBGR8888;
    } else if (pixel_format == PixelFormat::ARGB) {
        format = DRM_FORMAT_BGRA8888;
    } else {
        // MIUI MOD: START
        printf("warning: drm default pixel format: RGBX\n");
        format = DRM_FORMAT_XBGR8888;
        // END
    }

    drm_mode_create_dumb create_dumb = {};
    create_dumb.height = height;
    create_dumb.width = width;
    create_dumb.bpp = drm_format_to_bpp(format);
    create_dumb.flags = 0;

    if (drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb) != 0) {
        perror("Failed to DRM_IOCTL_MODE_CREATE_DUMB");
        return nullptr;
    }

    // Cannot use std::make_unique to access non-public ctor.
    auto surface = std::unique_ptr<GRSurfaceDrmMi>(new GRSurfaceDrmMi(
        width, height, create_dumb.pitch, create_dumb.bpp / 8, drm_fd, create_dumb.handle));

    uint32_t handles[4], pitches[4], offsets[4];

    handles[0] = surface->handle;
    pitches[0] = create_dumb.pitch;
    offsets[0] = 0;
    if (drmModeAddFB2(drm_fd, width, height, format, handles, pitches, offsets, &surface->fb_id, 0) !=
        0) {
        perror("Failed to drmModeAddFB2");
        return nullptr;
    }

    drm_mode_map_dumb map_dumb = {};
    map_dumb.handle = create_dumb.handle;
    if (drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb) != 0) {
        perror("Failed to DRM_IOCTL_MODE_MAP_DUMB");
        return nullptr;
    }

    auto mmapped = mmap(nullptr, surface->height * surface->row_bytes, PROT_READ | PROT_WRITE,
                        MAP_SHARED, drm_fd, map_dumb.offset);
    if (mmapped == MAP_FAILED) {
        perror("Failed to mmap()");
        return nullptr;
    }
    surface->mmapped_buffer_ = static_cast<uint8_t*>(mmapped);

    return surface;
}

int MinuiBackendDrmMi::mDrmFd = -1;
void MinuiBackendDrmMi::Create() {
    if (mDrmFd >= 0) {
        printf("Create, drm device has been created!\n");
        return;
    }

    /* open drm device*/
    for (int i = 0; i < DRM_MAX_MINOR; i++) {
         auto dev_name = android::base::StringPrintf(DRM_DEV_NAME, DRM_DIR_NAME, i);
         android::base::unique_fd fd(open(dev_name.c_str(), O_RDWR | O_CLOEXEC));
         if (fd == -1) continue;

         /* We need dumb buffers. */
         if (uint64_t cap = 0; drmGetCap(fd.get(), DRM_CAP_DUMB_BUFFER, &cap) != 0 || cap == 0) {
             continue;
         }

         auto res = drmModeGetResources(fd.get());
         if (!res) {
             continue;
         }

         printf("create, crtc_cnts:%d, connector_cnt:%d\n", res->count_crtcs, res->count_connectors);
         /* Use this device if it has at least one connected monitor. */
         if (res->count_crtcs > 0 && res->count_connectors > 0) {
             if (find_first_connected_connector(fd.get(), res)) {
                 printf("MinuiBackendDrmMi::Create, find connector\n");
                 mDrmFd = fd.release();
				 drmModeFreeResources(res);
                 break;
             }
         }

         drmModeFreeResources(res);
     }
}

MinuiBackendDrmMi::MinuiBackendDrmMi(int displayId)
   : mDisplayId(displayId) {}

MinuiBackendDrmMi::~MinuiBackendDrmMi() {
    ReleaseMemory();
}

GRSurface* MinuiBackendDrmMi::Init() {
    uint32_t selected_mode;
	uint32_t offset = 0;
    mLMs = 2;

    printf("Init, displayId:%d\n", mDisplayId);
    if (mDrmFd == -1) {
        perror("Failed to find/open a drm device");
        return nullptr;
    }

    auto res = drmModeGetResources(mDrmFd);
    if (res == nullptr) {
        perror("Failed to get drm resources");
        return nullptr;
    }

    mMonitorconnector = FindMainMonitor(mDrmFd, mDisplayId, res, &selected_mode);
    if (!mMonitorconnector) {
        fprintf(stderr, "Failed to find mMonitorconnector\n");
        drmModeFreeResources(res);
        return nullptr;
    }

    mMonitorcrtc = find_crtc_for_connector(mDrmFd, mDisplayId, res, mMonitorconnector);
    if (!mMonitorcrtc) {
        fprintf(stderr, "Failed to find mMonitorcrtc\n");
        drmModeFreeResources(res);
        return nullptr;
    }

    mMonitorcrtc->mode = mMonitorconnector->modes[selected_mode];

    drmModeFreeResources(res);

    mGRSurfaceDrms[0] = GRSurfaceDrmMi::Create(mDrmFd,
                                          mMonitorcrtc->mode.hdisplay, mMonitorcrtc->mode.vdisplay);
    mGRSurfaceDrms[1] = GRSurfaceDrmMi::Create(mDrmFd,
                                          mMonitorcrtc->mode.hdisplay, mMonitorcrtc->mode.vdisplay);
    if (!mGRSurfaceDrms[0] || !mGRSurfaceDrms[1]) {
        return nullptr;
    }

    mCurrentBufferIndex = 0;

    drmSetClientCap(mDrmFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    drmSetClientCap(mDrmFd, DRM_CLIENT_CAP_ATOMIC, 1);

    /* Get possible plane_ids */
    drmModePlaneRes *plane_options = drmModeGetPlaneResources(mDrmFd);
    if (!plane_options || !plane_options->planes || (plane_options->count_planes < mLMs)) {
        return nullptr;
    }

    /* Set crtc resources */
    mCrtcRes.props = drmModeObjectGetProperties(mDrmFd,
                        mMonitorcrtc->crtc_id,
                        DRM_MODE_OBJECT_CRTC);
    if (!mCrtcRes.props) {
        return nullptr;
    }

    mCrtcRes.props_info = static_cast<drmModePropertyRes **>
                             (calloc(mCrtcRes.props->count_props,
                             sizeof(mCrtcRes.props_info)));
    if (!mCrtcRes.props_info) {
        return nullptr;
    } else {
        for (int j = 0; j < (int)mCrtcRes.props->count_props; ++j)
            mCrtcRes.props_info[j] = drmModeGetProperty(mDrmFd,
                                         mCrtcRes.props->props[j]);
    }

    /* Set connector resources */
    mConnRes.props = drmModeObjectGetProperties(mDrmFd,
                       mMonitorconnector->connector_id,
                       DRM_MODE_OBJECT_CONNECTOR);
    if (!mConnRes.props) {
        return nullptr;
    }

    mConnRes.props_info = static_cast<drmModePropertyRes **>
                           (calloc(mConnRes.props->count_props,
                           sizeof(mConnRes.props_info)));
    if (!mConnRes.props_info) {
        return nullptr;
    } else {
        for (int j = 0; j < (int)mConnRes.props->count_props; ++j) {
            mConnRes.props_info[j] = drmModeGetProperty(mDrmFd,
                                   mConnRes.props->props[j]);

            if (!strcmp(mConnRes.props_info[j]->name, "mode_properties")) {
                mLMs = get_topology_lm_number(mDrmFd, mConnRes.props->prop_values[j]);
                printf("number of lms in topology %d\n", mLMs);
            }
        }
    }

    if (mDisplayId) {
        /* 2 is the planes reserved for primary display */
        if (plane_options->count_planes < (mLMs + 2)) {
            return nullptr;
        }
        offset = 2;
    }

    /* Set plane resources */
    for(uint32_t i = 0; i < mLMs; ++i) {
        mPlaneRes[i].plane = drmModeGetPlane(mDrmFd, plane_options->planes[i + offset]);
        if (!mPlaneRes[i].plane) {
            return nullptr;
        }
    }

    for (uint32_t i = 0; i < mLMs; ++i) {
        struct Plane *obj = &mPlaneRes[i];
        unsigned int j;
        obj->props = drmModeObjectGetProperties(mDrmFd, obj->plane->plane_id,
                        DRM_MODE_OBJECT_PLANE);
        if (!obj->props) {
            continue;
        }

        obj->props_info = static_cast<drmModePropertyRes **>
                             (calloc(obj->props->count_props, sizeof(*obj->props_info)));
        if (!obj->props_info) {
            continue;
        }

        for (j = 0; j < obj->props->count_props; ++j) {
            obj->props_info[j] = drmModeGetProperty(mDrmFd, obj->props->props[j]);
        }
    }

    drmModeFreePlaneResources(plane_options);
    plane_options = nullptr;

    /* Setup pipe and blob_id */
    if (drmModeCreatePropertyBlob(mDrmFd, &mMonitorcrtc->mode, sizeof(drmModeModeInfo),
        &mCrtcRes.mode_blob_id)) {
        printf("failed to create mode blob\n");
        return nullptr;
    }

    /* Save mFBPropId*/
    mFBPropId = find_plane_prop_id(mPlaneRes[0].plane->plane_id, "FB_ID", mPlaneRes);

    Blank(false);

    return mGRSurfaceDrms[0].get();
}

GRSurface* MinuiBackendDrmMi::Flip() {
    UpdatePlaneFB();

    mCurrentBufferIndex = 1 - mCurrentBufferIndex;
    return mGRSurfaceDrms[mCurrentBufferIndex].get();
}

void MinuiBackendDrmMi::Blank(bool blank) {
    int ret = 0;

    printf("Blank blank:%d, displayId:%d\n", blank, mDisplayId);
    if (blank == mCurrentBlankState) {
        return;
    }

    drmModeAtomicReqPtr atomic_req = drmModeAtomicAlloc();
    if (!atomic_req) {
       printf("Atomic Alloc failed\n");
       return;
    }

    if (blank) {
        ret = TeardownPipeline(atomic_req);
    } else {
        ret = SetupPipeline(atomic_req);
    }

    if (!ret) {
        ret = drmModeAtomicCommit(mDrmFd, atomic_req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
    }

    if (!ret) {
       printf("Atomic Commit successed\n");
       mCurrentBlankState = blank;
    }

    drmModeAtomicFree(atomic_req);
}

int MinuiBackendDrmMi::AtomicPopulatePlane(int plane, drmModeAtomicReqPtr atomic_req) {
    uint32_t src_x, src_y, src_w, src_h;
    uint32_t crtc_x, crtc_y, crtc_w, crtc_h;
    int width = mMonitorcrtc->mode.hdisplay;
    int height = mMonitorcrtc->mode.vdisplay;

    src_y = 0;
    src_w = width/mLMs;
    src_h =  height;
    crtc_y = 0;
    crtc_w = width/mLMs;
    crtc_h = height;

    if (plane == Left) {
        src_x = 0;
        crtc_x = 0;
    } else {
        src_x = width/mLMs;
        crtc_x = width/mLMs;
    }

    if (atomic_add_prop_to_plane(mPlaneRes, atomic_req,
                                 mPlaneRes[plane].plane->plane_id, "FB_ID",
                                 mGRSurfaceDrms[mCurrentBufferIndex]->fb_id))
        return -EINVAL;

    if (atomic_add_prop_to_plane(mPlaneRes, atomic_req,
                                 mPlaneRes[plane].plane->plane_id, "SRC_X", src_x << 16))
        return -EINVAL;

    if (atomic_add_prop_to_plane(mPlaneRes, atomic_req,
                                 mPlaneRes[plane].plane->plane_id, "SRC_Y", src_y << 16))
        return -EINVAL;

    if (atomic_add_prop_to_plane(mPlaneRes, atomic_req,
                                 mPlaneRes[plane].plane->plane_id, "SRC_W", src_w << 16))
        return -EINVAL;

    if (atomic_add_prop_to_plane(mPlaneRes, atomic_req,
                                 mPlaneRes[plane].plane->plane_id, "SRC_H", src_h << 16))
        return -EINVAL;

    if (atomic_add_prop_to_plane(mPlaneRes, atomic_req,
                                 mPlaneRes[plane].plane->plane_id, "CRTC_X", crtc_x))
        return -EINVAL;

    if (atomic_add_prop_to_plane(mPlaneRes, atomic_req,
                                 mPlaneRes[plane].plane->plane_id, "CRTC_Y", crtc_y))
        return -EINVAL;

    if (atomic_add_prop_to_plane(mPlaneRes, atomic_req,
                                 mPlaneRes[plane].plane->plane_id, "CRTC_W", crtc_w))
        return -EINVAL;

    if (atomic_add_prop_to_plane(mPlaneRes, atomic_req,
                                 mPlaneRes[plane].plane->plane_id, "CRTC_H", crtc_h))
        return -EINVAL;

    if (atomic_add_prop_to_plane(mPlaneRes, atomic_req,
                                 mPlaneRes[plane].plane->plane_id, "CRTC_ID",
                                 mMonitorcrtc->crtc_id))
        return -EINVAL;

    return 0;
}

int MinuiBackendDrmMi::TeardownPipeline(drmModeAtomicReqPtr atomic_req) {
    uint32_t i, prop_id;
    int ret;

    /* During suspend, tear down pipeline */
    add_prop(&mConnRes, connector, Connector, mMonitorconnector->connector_id, "CRTC_ID", 0);
    add_prop(&mCrtcRes, crtc, Crtc, mMonitorcrtc->crtc_id, "MODE_ID", 0);
    add_prop(&mConnRes, connector, Connector, mMonitorconnector->connector_id, "LP", 0);
    add_prop(&mCrtcRes, crtc, Crtc, mMonitorcrtc->crtc_id, "ACTIVE", 0);

    for(i = 0; i < mLMs; i++) {
        ret = atomic_add_prop_to_plane(mPlaneRes, atomic_req,
                                       mPlaneRes[i].plane->plane_id, "CRTC_ID", 0);
        if (ret < 0) {
            printf("Failed to tear down plane %d\n", i);
            return ret;
        }

        if (drmModeAtomicAddProperty(atomic_req, mPlaneRes[i].plane->plane_id, mFBPropId, 0) < 0) {
            printf("Failed to add property for plane_id=%d\n", mPlaneRes[i].plane->plane_id);
            return -EINVAL;
        }
    }

    return 0;
}

int MinuiBackendDrmMi::SetupPipeline(drmModeAtomicReqPtr atomic_req) {
    uint32_t i, prop_id;
    int ret;

    for(i = 0; i < mLMs; i++) {
        add_prop(&mConnRes, connector, Connector, mMonitorconnector->connector_id,
             "CRTC_ID", mMonitorcrtc->crtc_id);
        add_prop(&mCrtcRes, crtc, Crtc, mMonitorcrtc->crtc_id, "MODE_ID", mCrtcRes.mode_blob_id);
        add_prop(&mCrtcRes, crtc, Crtc, mMonitorcrtc->crtc_id, "ACTIVE", 1);
    }

    /* Setup planes */
    for(i = 0; i < mLMs; i++) {
        ret = AtomicPopulatePlane(i, atomic_req);
        if (ret < 0) {
            printf("Error populating plane_id = %d\n", mPlaneRes[i].plane->plane_id);
            return ret;
        }
    }

    return 0;
}

drmModeConnector* MinuiBackendDrmMi::FindMainMonitor(int fd, int displayId,
                                                   drmModeRes* resources, uint32_t* mode_index) {
    /* Look for LVDS/eDP/DSI connectors. Those are the main screens. */
    static constexpr unsigned kConnectorPriority[] = {
      DRM_MODE_CONNECTOR_LVDS,
      DRM_MODE_CONNECTOR_eDP,
      DRM_MODE_CONNECTOR_DSI,
    };

    unsigned i = 0;
    drmModeConnector* main_monitor_connector = nullptr;

    if (!displayId) {
        do {
            main_monitor_connector = find_used_connector_by_type(fd, resources, kConnectorPriority[i]);
            i++;
        } while (!main_monitor_connector && i < arraysize(kConnectorPriority));

        /* If we didn't find a connector, grab the first one that is connected. */
        if (!main_monitor_connector) {
            main_monitor_connector = find_first_connected_connector(fd, resources);
        }
    } else {
        main_monitor_connector = find_buidin_connected_connector(fd, displayId, resources);
    }

    /* If we still didn't find a connector, give up and return. */
    if (!main_monitor_connector) return nullptr;

    *mode_index = 0;
    for (int modes = 0; modes < main_monitor_connector->count_modes; modes++) {
        if (main_monitor_connector->modes[modes].type & DRM_MODE_TYPE_PREFERRED) {
            *mode_index = modes;
            break;
        }
    }

    return main_monitor_connector;
}

void MinuiBackendDrmMi::UpdatePlaneFB() {
    uint32_t i;
    int32_t ret;

    /* Set atomic req */
    drmModeAtomicReqPtr atomic_req = drmModeAtomicAlloc();
    if (!atomic_req) {
         printf("Atomic Alloc failed. Could not update fb_id\n");
         return;
    }

    /* Add property */
    for(i = 0; i < mLMs; i++) {
        drmModeAtomicAddProperty(atomic_req, mPlaneRes[i].plane->plane_id,
                                 mFBPropId, mGRSurfaceDrms[mCurrentBufferIndex]->fb_id);
    }

    /* Commit changes */
    ret = drmModeAtomicCommit(mDrmFd, atomic_req,
                   DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);

    drmModeAtomicFree(atomic_req);

    if (ret) {
        printf("Atomic commit failed ret=%d\n", ret);
    }
}

void MinuiBackendDrmMi::ReleaseMemory() {
    Blank(true);
    drmModeDestroyPropertyBlob(mDrmFd, mCrtcRes.mode_blob_id);
    drmModeFreeCrtc(mMonitorcrtc);
    drmModeFreeConnector(mMonitorconnector);
}

void MinuiBackendDrmMi::Destroy() {
    if (mDrmFd >= 0) {
        close(mDrmFd);
        mDrmFd = -1;
    }
}
