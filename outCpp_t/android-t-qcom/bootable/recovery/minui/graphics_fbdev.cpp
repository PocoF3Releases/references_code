/*
 * Copyright (C) 2014 The Android Open Source Project
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

#include "graphics_fbdev.h"

#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>

#include <android-base/unique_fd.h>

#include "minui/minui.h"

// MIUI ADD: START
#include <android-base/file.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <android-base/properties.h>
#include <string>
static constexpr const char* BRIGHTNESS_FILE = "/sys/class/leds/lcd-backlight/brightness";
static constexpr const char* MAX_BRIGHTNESS_FILE = "/sys/class/leds/lcd-backlight/max_brightness";
static constexpr const char* NEW_BRIGHTNESS_FILE = "/sys/class/backlight/panel0-backlight/brightness";
static constexpr const char* NEW_MAX_BRIGHTNESS_FILE = "/sys/class/backlight/panel0-backlight/max_brightness";

static const int oled_light = 40;
static const int lcd_light = 80;
// END

std::unique_ptr<GRSurfaceFbdev> GRSurfaceFbdev::Create(size_t width, size_t height,
                                                       size_t row_bytes, size_t pixel_bytes) {
  // Cannot use std::make_unique to access non-public ctor.
  return std::unique_ptr<GRSurfaceFbdev>(new GRSurfaceFbdev(width, height, row_bytes, pixel_bytes));
}

// MIUI ADD: START for fb, for g7 and other fb device. cause of savemode to reopen bright
static void set_screen_backlight(bool on)
{
  std::string max_brightness_file = MAX_BRIGHTNESS_FILE;
  std::string brightness_file = BRIGHTNESS_FILE;
  if (access(brightness_file.c_str(), R_OK | W_OK)) {
    brightness_file = NEW_BRIGHTNESS_FILE;
  }
  if (access(max_brightness_file.c_str(), R_OK)) {
    max_brightness_file = NEW_MAX_BRIGHTNESS_FILE;
  }

  // Set the initial brightness level based on the max brightness. Note that reading the initial
  // value from BRIGHTNESS_FILE doesn't give the actual brightness value (bullhead, sailfish), so
  // we don't have a good way to query the default value.
  std::string content;
  if (!android::base::ReadFileToString(max_brightness_file, &content)) {
    fprintf(stderr, "Failed to read max brightness\n");
    return;
  }

  unsigned int max_value;
  if (!android::base::ParseUint(android::base::Trim(content), &max_value)) {
    fprintf(stderr, "Failed to parse max brightness: %s\n", content.c_str());
    return;
  }

  int scale = 1;
  if (max_value > 256) {
    scale = (max_value+1)/256;
  }
  int screen_light = lcd_light * scale;
  std::string screen_type_str = android::base::GetProperty("ro.display.type", "");
  if (strncmp(screen_type_str.c_str(), "oled", 4) == 0) {    // decrease oled screen light in order to extend its life
    screen_light = oled_light * scale;
  } else {
    screen_type_str = android::base::GetProperty("ro.vendor.display.type", "");
    if (strncmp(screen_type_str.c_str(), "oled", 4) == 0) {    // decrease oled screen light in order to extend its life
      screen_light = oled_light * scale;
    }
  }
  fprintf(stderr, "the light in recovery now is %d\n", screen_light);
  if (!android::base::WriteStringToFile(std::to_string(on ? screen_light : 0), brightness_file)) {
    fprintf(stderr, "Failed to set brightness");
  }
}
// END

void MinuiBackendFbdev::Blank(bool blank) {
  // MIUI ADD
  set_screen_backlight(!blank);
  // int ret = ioctl(fb_fd, FBIOBLANK, blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK);
  // if (ret < 0) perror("ioctl(): blank");
  // END
}

void MinuiBackendFbdev::SetDisplayedFramebuffer(size_t n) {
  if (n > 1 || !double_buffered) return;

  vi.yres_virtual = gr_framebuffer[0]->height * 2;
  vi.yoffset = n * gr_framebuffer[0]->height;
  vi.bits_per_pixel = gr_framebuffer[0]->pixel_bytes * 8;
  if (ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
    perror("active fb swap failed");
  }
  displayed_buffer = n;
}

GRSurface* MinuiBackendFbdev::Init() {
  android::base::unique_fd fd(open("/dev/graphics/fb0", O_RDWR | O_CLOEXEC));
  if (fd == -1) {
    perror("cannot open fb0");
    return nullptr;
  }

  fb_fix_screeninfo fi;
  if (ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
    perror("failed to get fb0 info");
    return nullptr;
  }

  if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0) {
    perror("failed to get fb0 info");
    return nullptr;
  }

  // We print this out for informational purposes only, but
  // throughout we assume that the framebuffer device uses an RGBX
  // pixel format.  This is the case for every development device I
  // have access to.  For some of those devices (eg, hammerhead aka
  // Nexus 5), FBIOGET_VSCREENINFO *reports* that it wants a
  // different format (XBGR) but actually produces the correct
  // results on the display when you write RGBX.
  //
  // If you have a device that actually *needs* another pixel format
  // (ie, BGRX, or 565), patches welcome...

  printf(
      "fb0 reports (possibly inaccurate):\n"
      "  vi.bits_per_pixel = %d\n"
      "  vi.red.offset   = %3d   .length = %3d\n"
      "  vi.green.offset = %3d   .length = %3d\n"
      "  vi.blue.offset  = %3d   .length = %3d\n",
      vi.bits_per_pixel, vi.red.offset, vi.red.length, vi.green.offset, vi.green.length,
      vi.blue.offset, vi.blue.length);

  void* bits = mmap(0, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (bits == MAP_FAILED) {
    perror("failed to mmap framebuffer");
    return nullptr;
  }

  memset(bits, 0, fi.smem_len);

  gr_framebuffer[0] =
      GRSurfaceFbdev::Create(vi.xres, vi.yres, fi.line_length, vi.bits_per_pixel / 8);
  gr_framebuffer[0]->buffer_ = static_cast<uint8_t*>(bits);
  memset(gr_framebuffer[0]->buffer_, 0, gr_framebuffer[0]->height * gr_framebuffer[0]->row_bytes);

  gr_framebuffer[1] =
      GRSurfaceFbdev::Create(gr_framebuffer[0]->width, gr_framebuffer[0]->height,
                             gr_framebuffer[0]->row_bytes, gr_framebuffer[0]->pixel_bytes);

  /* check if we can use double buffering */
  if (vi.yres * fi.line_length * 2 <= fi.smem_len) {
    double_buffered = true;

    gr_framebuffer[1]->buffer_ =
        gr_framebuffer[0]->buffer_ + gr_framebuffer[0]->height * gr_framebuffer[0]->row_bytes;
  } else {
    double_buffered = false;

    // Without double-buffering, we allocate RAM for a buffer to draw in, and then "flipping" the
    // buffer consists of a memcpy from the buffer we allocated to the framebuffer.
    memory_buffer.resize(gr_framebuffer[1]->height * gr_framebuffer[1]->row_bytes);
    gr_framebuffer[1]->buffer_ = memory_buffer.data();
  }

  gr_draw = gr_framebuffer[1].get();
  memset(gr_draw->buffer_, 0, gr_draw->height * gr_draw->row_bytes);
  fb_fd = std::move(fd);
  SetDisplayedFramebuffer(0);

  printf("framebuffer: %d (%zu x %zu)\n", fb_fd.get(), gr_draw->width, gr_draw->height);

  Blank(true);
  Blank(false);

  return gr_draw;
}

GRSurface* MinuiBackendFbdev::Flip() {
  if (double_buffered) {
    // Change gr_draw to point to the buffer currently displayed, then flip the driver so we're
    // displaying the other buffer instead.
    gr_draw = gr_framebuffer[displayed_buffer].get();
    SetDisplayedFramebuffer(1 - displayed_buffer);
  } else {
    // Copy from the in-memory surface to the framebuffer.
    memcpy(gr_framebuffer[0]->buffer_, gr_draw->buffer_, gr_draw->height * gr_draw->row_bytes);
  }
  return gr_draw;
}
