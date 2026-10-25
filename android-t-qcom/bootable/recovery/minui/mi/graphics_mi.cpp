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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <android-base/properties.h>

#include "graphics.h"
#include "graphics_drm_mi.h"
#include "minui/minui.h"

static GRFont* gr_font = nullptr;
static MinuiBackend* gr_backend[DISP_MAX] = {nullptr};

static int overscan_offset_x = 0;
static int overscan_offset_y = 0;

static uint32_t gr_current = ~0;
static constexpr uint32_t alpha_mask = 0xff000000;

// gr_draw is owned by backends.
static GRSurface* gr_draw[DISP_MAX] = {nullptr};
static GRRotation rotation[DISP_MAX] = {GRRotation::NONE};
static PixelFormat pixel_format = PixelFormat::UNKNOWN;

static bool outside(int displayId, int x, int y) {
    auto swapped = (rotation[displayId] == GRRotation::LEFT || rotation[displayId] == GRRotation::RIGHT);
    return x < 0 || x >= (swapped ? gr_draw[displayId]->height : gr_draw[displayId]->width) || y < 0 ||
           y >= (swapped ? gr_draw[displayId]->width : gr_draw[displayId]->height);
}

// Blends gr_current onto pix value, assumes alpha as most significant byte.
static inline uint32_t pixel_blend(uint8_t alpha, uint32_t pix) {
    if (alpha == 255) return gr_current;
    if (alpha == 0) return pix;
    uint32_t pix_r = pix & 0xff;
    uint32_t pix_g = pix & 0xff00;
    uint32_t pix_b = pix & 0xff0000;
    uint32_t cur_r = gr_current & 0xff;
    uint32_t cur_g = gr_current & 0xff00;
    uint32_t cur_b = gr_current & 0xff0000;

    uint32_t out_r = (pix_r * (255 - alpha) + cur_r * alpha) / 255;
    uint32_t out_g = (pix_g * (255 - alpha) + cur_g * alpha) / 255;
    uint32_t out_b = (pix_b * (255 - alpha) + cur_b * alpha) / 255;

    return (out_r & 0xff) | (out_g & 0xff00) | (out_b & 0xff0000) | (gr_current & 0xff000000);
}

// Increments pixel pointer right, with current rotation.
static void incr_x(int displayId, uint32_t** p, int row_pixels) {
    if (rotation[displayId] == GRRotation::LEFT) {
        *p = *p - row_pixels;
    } else if (rotation[displayId] == GRRotation::RIGHT) {
        *p = *p + row_pixels;
    } else if (rotation[displayId] == GRRotation::DOWN) {
        *p = *p - 1;
    } else {  // GRRotation::NONE
        *p = *p + 1;
    }
}

// Increments pixel pointer down, with current rotation.
static void incr_y(int displayId, uint32_t** p, int row_pixels) {
    if (rotation[displayId] == GRRotation::LEFT) {
        *p = *p + 1;
    } else if (rotation[displayId] == GRRotation::RIGHT) {
        *p = *p - 1;
    } else if (rotation[displayId] == GRRotation::DOWN) {
        *p = *p - row_pixels;
    } else {  // GRRotation::NONE
        *p = *p + row_pixels;
    }
}

// Returns pixel pointer at given coordinates with rotation adjustment.
static uint32_t* PixelAt(int displayId, GRSurface* surface, int x, int y, int row_pixels) {
    switch (rotation[displayId]) {
      case GRRotation::NONE:
          return reinterpret_cast<uint32_t*>(surface->data()) + y * row_pixels + x;
      case GRRotation::RIGHT:
          return reinterpret_cast<uint32_t*>(surface->data()) + x * row_pixels + (surface->width - y);
      case GRRotation::DOWN:
          return reinterpret_cast<uint32_t*>(surface->data()) + (surface->height - 1 - y) * row_pixels +
                 (surface->width - 1 - x);
      case GRRotation::LEFT:
          return reinterpret_cast<uint32_t*>(surface->data()) + (surface->height - 1 - x) * row_pixels +
                 y;
      default:
          printf("invalid rotation %d", static_cast<int>(rotation[displayId]));
    }

    return nullptr;
}

static int gr_init_font_mi(const char* name, GRFont** dest) {
    GRFont* font = static_cast<GRFont*>(calloc(1, sizeof(*gr_font)));
    if (font == nullptr) {
      return -1;
    }

    int res = res_create_alpha_surface(name, &(font->texture));
    if (res < 0) {
      free(font);
      return res;
    }

    // The font image should be a 96x2 array of character images.  The
    // columns are the printable ASCII characters 0x20 - 0x7f.  The
    // top row is regular text; the bottom row is bold.
    font->char_width = font->texture->width / 96;
    font->char_height = font->texture->height / 2;

    *dest = font;

    return 0;
}

int gr_init_mi() {
    // pixel_format needs to be set before loading any resources or initializing backends.
    std::string format = android::base::GetProperty("ro.minui.pixel_format", "");
    if (format == "ABGR_8888") {
        pixel_format = PixelFormat::ABGR;
    } else if (format == "RGBX_8888") {
        pixel_format = PixelFormat::RGBX;
    } else if (format == "ARGB_8888") {
        pixel_format = PixelFormat::ARGB;
    } else if (format == "BGRA_8888") {
        pixel_format = PixelFormat::BGRA;
    } else {
        pixel_format = PixelFormat::UNKNOWN;
    }

    std::string rotation_str =
            android::base::GetProperty("ro.minui.default_rotation", "ROTATION_NONE");
    if (rotation_str == "ROTATION_RIGHT") {
        gr_rotate_mi(DISP_PRIMARY, GRRotation::RIGHT);
    } else if (rotation_str == "ROTATION_DOWN") {
        gr_rotate_mi(DISP_PRIMARY, GRRotation::DOWN);
    } else if (rotation_str == "ROTATION_LEFT") {
        gr_rotate_mi(DISP_PRIMARY, GRRotation::LEFT);
    } else {  // "ROTATION_NONE" or unknown string
        gr_rotate_mi(DISP_PRIMARY, GRRotation::NONE);
    }

    int ret = gr_init_font_mi("font", &gr_font);
    if (ret != 0) {
        printf("Failed to init font: %d, continuing graphic backend initialization without font file\n",
               ret);
    }

    MinuiBackendDrmMi::Create();

    auto backend_prim = std::unique_ptr<MinuiBackend>{ std::make_unique<MinuiBackendDrmMi>(DISP_PRIMARY) };
    gr_draw[DISP_PRIMARY] = backend_prim->Init();
    gr_backend[DISP_PRIMARY] = backend_prim.release();

    auto backend_sec = std::unique_ptr<MinuiBackend>{ std::make_unique<MinuiBackendDrmMi>(DISP_SECONDARY) };
    gr_draw[DISP_SECONDARY] = backend_sec->Init();
    gr_backend[DISP_SECONDARY] = backend_sec.release();

    if (!gr_draw[DISP_PRIMARY] || !gr_draw[DISP_SECONDARY]) {
        printf("gr_init_mi: failed. surface is null\n");
        return -1;
    }

    if (gr_draw[DISP_PRIMARY]->pixel_bytes != 4
        || gr_draw[DISP_SECONDARY]->pixel_bytes != 4) {
        printf("gr_init_mi: Only 4-byte pixel formats supported\n");
    }

    return 0;
}

void gr_exit_mi() {
    if (gr_backend[DISP_PRIMARY]) delete gr_backend[DISP_PRIMARY];
    gr_backend[DISP_PRIMARY] = nullptr;

    if (gr_backend[DISP_SECONDARY]) delete gr_backend[DISP_SECONDARY];
    gr_backend[DISP_SECONDARY] = nullptr;

    if (gr_font) delete gr_font;
    gr_font = nullptr;

    MinuiBackendDrmMi::Destroy();
}

int gr_fb_width_mi(int displayId) {
    if (displayId < 0 || displayId >= DISP_MAX
        || !gr_draw[displayId]) {
        printf("gr_fb_width_mi: display id is invalid\n");
        return 0;
    }

    return (rotation[displayId] == GRRotation::LEFT || rotation[displayId] == GRRotation::RIGHT)
               ? gr_draw[displayId]->height - 2 * overscan_offset_y
               : gr_draw[displayId]->width - 2 * overscan_offset_x;
}

int gr_fb_height_mi(int displayId) {
    if (displayId < 0 || displayId >= DISP_MAX
        || !gr_draw[displayId]) {
        printf("gr_fb_height_mi: display id is invalid\n");
        return 0;
    }

    return (rotation[displayId] == GRRotation::LEFT || rotation[displayId] == GRRotation::RIGHT)
               ? gr_draw[displayId]->width - 2 * overscan_offset_x
               : gr_draw[displayId]->height - 2 * overscan_offset_y;
}

void gr_flip_mi(int displayId) {
    if (displayId < 0 || displayId >= DISP_MAX
        || !gr_draw[displayId] || !gr_backend[displayId]) {
        printf("gr_flip_mi: display id is invalid\n");
        return;
    }

    gr_draw[displayId] = gr_backend[displayId]->Flip();
}

void gr_fb_blank_mi(int displayId, bool blank) {
    if (displayId < 0 || displayId >= DISP_MAX
        || !gr_draw[displayId] || !gr_backend[displayId]) {
        printf("gr_fb_blank_mi: display id is invalid\n");
        return;
    }

    gr_backend[displayId]->Blank(blank);

    /*
    if (!blank) {
        int overscan_percent = android::base::GetIntProperty("ro.minui.overscan_percent", 0);
        overscan_offset_x = gr_draw[displayId]->width * overscan_percent / 100;
        overscan_offset_y = gr_draw[displayId]->height * overscan_percent / 100;

        gr_flip_mi(displayId);
        gr_flip_mi(displayId);
        if (!gr_draw[displayId]) {
            printf("gr_fb_blank_mi: gr_draw becomes nullptr after gr_flip\n");
            return;
        }
    }
    */
    //gr_backend[displayId]->Blank(blank);
}

void gr_clear_mi(int displayId) {

    if (displayId < 0 || displayId >= DISP_MAX || !gr_draw[displayId]) return;

    if ((gr_current & 0xff) == ((gr_current >> 8) & 0xff) &&
        (gr_current & 0xff) == ((gr_current >> 16) & 0xff) &&
        (gr_current & 0xff) == ((gr_current >> 24) & 0xff) &&
        gr_draw[displayId]->row_bytes == gr_draw[displayId]->width * gr_draw[displayId]->pixel_bytes) {
          memset(gr_draw[displayId]->data(), gr_current & 0xff, gr_draw[displayId]->height * gr_draw[displayId]->row_bytes);
    } else {
        uint32_t* px = reinterpret_cast<uint32_t*>(gr_draw[displayId]->data());
        int row_diff = gr_draw[displayId]->row_bytes / gr_draw[displayId]->pixel_bytes - gr_draw[displayId]->width;
        for (int y = 0; y < gr_draw[displayId]->height; ++y) {
            for (int x = 0; x < gr_draw[displayId]->width; ++x) {
                *px++ = gr_current;
            }
            px += row_diff;
        }
    }
}

void gr_fill_mi(int displayId, int x1, int y1, int x2, int y2) {

    if (displayId < 0 || displayId >= DISP_MAX || !gr_draw[displayId]) return;

    x1 += overscan_offset_x;
    y1 += overscan_offset_y;

    x2 += overscan_offset_x;
    y2 += overscan_offset_y;

    if (outside(displayId, x1, y1) || outside(displayId, x2 - 1, y2 - 1)) return;

    int row_pixels = gr_draw[displayId]->row_bytes / gr_draw[displayId]->pixel_bytes;
    uint32_t* p = PixelAt(displayId, gr_draw[displayId], x1, y1, row_pixels);
    uint8_t alpha = static_cast<uint8_t>(((gr_current & alpha_mask) >> 24));
    if (alpha > 0) {
        for (int y = y1; y < y2; ++y) {
            uint32_t* px = p;
            for (int x = x1; x < x2; ++x) {
                *px = pixel_blend(alpha, *px);
                incr_x(displayId, &px, row_pixels);
            }
            incr_y(displayId, &p, row_pixels);
        }
    }
}

void gr_blit_mi(int displayId, const GRSurface* source, int sx, int sy, int w, int h, int dx, int dy) {
    if (source == nullptr) return;

    if (displayId < 0 || displayId >= DISP_MAX || !gr_draw[displayId]) return;

    if (gr_draw[displayId]->pixel_bytes != source->pixel_bytes) {
        printf("gr_blit_mi: source has wrong format\n");
        return;
    }

    dx += overscan_offset_x;
    dy += overscan_offset_y;

    if (outside(displayId, dx, dy) || outside(displayId, dx + w - 1, dy + h - 1)) return;

    if (rotation[displayId] != GRRotation::NONE) {
        int src_row_pixels = source->row_bytes / source->pixel_bytes;
        int row_pixels = gr_draw[displayId]->row_bytes / gr_draw[displayId]->pixel_bytes;
        const uint32_t* src_py =
            reinterpret_cast<const uint32_t*>(source->data()) + sy * source->row_bytes / 4 + sx;
        uint32_t* dst_py = PixelAt(displayId, gr_draw[displayId], dx, dy, row_pixels);

        for (int y = 0; y < h; y += 1) {
            const uint32_t* src_px = src_py;
            uint32_t* dst_px = dst_py;
            for (int x = 0; x < w; x += 1) {
                *dst_px = *src_px++;
                incr_x(displayId, &dst_px, row_pixels);
            }
            src_py += src_row_pixels;
            incr_y(displayId, &dst_py, row_pixels);
        }
    } else {
        const uint8_t* src_p = source->data() + sy * source->row_bytes + sx * source->pixel_bytes;
        uint8_t* dst_p = gr_draw[displayId]->data() + dy * gr_draw[displayId]->row_bytes + dx * gr_draw[displayId]->pixel_bytes;

        for (int i = 0; i < h; ++i) {
            memcpy(dst_p, src_p, w * source->pixel_bytes);
            src_p += source->row_bytes;
            dst_p += gr_draw[displayId]->row_bytes;
        }
    }
}

void gr_rotate_mi(int displayId, GRRotation rot) {
    rotation[displayId] = rot;
}

void gr_color_mi(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    uint32_t r32 = r, g32 = g, b32 = b, a32 = a;

    if (pixel_format == PixelFormat::ARGB || pixel_format == PixelFormat::BGRA) {
        gr_current = (a32 << 24) | (r32 << 16) | (g32 << 8) | b32;
    } else {
        gr_current = (a32 << 24) | (b32 << 16) | (g32 << 8) | r32;
    }
}

unsigned int gr_get_width_mi(const GRSurface* surface) {
    if (surface == nullptr) {
      return 0;
    }

    return surface->width;
}

unsigned int gr_get_height_mi(const GRSurface* surface) {
    if (surface == nullptr) {
      return 0;
    }

    return surface->height;
}
