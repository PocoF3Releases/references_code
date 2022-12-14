/*
 * Copyright (C) 2011-2017 The Android Open Source Project
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

#include <charger/healthd_mode_charger.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include <optional>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/macros.h>
#include <android-base/strings.h>

#include <linux/netlink.h>
#include <sys/socket.h>

#include <cutils/android_get_control_file.h>
#include <cutils/klog.h>
#include <cutils/misc.h>
#include <cutils/properties.h>
#include <cutils/uevent.h>
#include <sys/reboot.h>

#include <suspend/autosuspend.h>

#include "AnimationParser.h"
#include "healthd_draw.h"

#include <aidl/android/hardware/health/BatteryStatus.h>
#include <health/HealthLoop.h>
#include <healthd/healthd.h>
#include <2k/batt.h>
#include <2k/cap.h>
#include <2k/digits.h>
#include <1080p/batt.h>
#include <1080p/cap.h>
#include <1080p/digits.h>
#include <1220p/batt.h>
#include <1220p/cap.h>
#include <1220p/digits.h>
#include <FOLD_MAIN/batt.h>
#include <FOLD_MAIN/cap.h>
#include <FOLD_MAIN/digits.h>
#include <FOLD_FLIP/batt.h>
#include <FOLD_FLIP/cap.h>
#include <FOLD_FLIP/digits.h>
#include <main/batt.h>
#include <main/cap.h>
#include <main/digits.h>
#if !defined(__ANDROID_VNDK__)
#include "charger.sysprop.h"
#endif

using std::string_literals::operator""s;
using namespace android;
using aidl::android::hardware::health::BatteryStatus;
using android::hardware::health::HealthLoop;

// main healthd loop
extern int healthd_main(void);

// minui globals
char* locale;

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define MSEC_PER_SEC (1000LL)
#define NSEC_PER_MSEC (1000000LL)

#define BATTERY_UNKNOWN_TIME (2 * MSEC_PER_SEC)
#define POWER_ON_KEY_TIME (2 * MSEC_PER_SEC)
#define UNPLUGGED_SHUTDOWN_TIME (3 * MSEC_PER_SEC)
#define UNPLUGGED_DISPLAY_TIME (3 * MSEC_PER_SEC)
#define MAX_BATT_LEVEL_WAIT_TIME (3 * MSEC_PER_SEC)
#define LED_PREPARE_TIME (12 * MSEC_PER_SEC)
#define UNPLUGGED_SHUTDOWN_TIME_PROP "ro.product.charger.unplugged_shutdown_time"

#define LAST_KMSG_MAX_SZ (32 * 1024)

#define BACKLIGHT_TOGGLE_PATH "/sys/class/backlight/panel0-backlight/brightness"
#define BACKLIGHT_SECOND_TOGGLE_PATH "/sys/class/backlight/panel1-backlight/brightness"

#define CHARGER_PRESENT_PATH "/sys/class/power_supply/usb/online"
#define WIRELESS_PRESENT_PATH_OLD_PLATFORM "/sys/class/power_supply/dc/online"
#define WIRELESS_PRESENT_PATH "/sys/class/power_supply/wireless/online"
#define LOGE(x...) KLOG_ERROR("charger", x);
#define LOGW(x...) KLOG_WARNING("charger", x);
#define LOGV(x...) KLOG_DEBUG("charger", x);

#define BATT_START_LARGE_X 711
namespace android {

#if defined(__ANDROID_VNDK__)
static constexpr const char* vendor_animation_desc_path =
        "/vendor/etc/res/values/charger/animation.txt";
static constexpr const char* vendor_animation_root = "/vendor/etc/res/images/";
static constexpr const char* vendor_default_animation_root = "/vendor/etc/res/images/default/";
#else

// Legacy animation resources are loaded from this directory.
static constexpr const char* legacy_animation_root = "/res/images/";

// Built-in animation resources are loaded from this directory.
static constexpr const char* system_animation_root = "/system/etc/res/images/";

// Resources in /product/etc/res overrides resources in /res and /system/etc/res.
// If the device is using the Generic System Image (GSI), resources may exist in
// both paths.
static constexpr const char* product_animation_desc_path =
        "/product/etc/res/values/charger/animation.txt";
static constexpr const char* product_animation_root = "/product/etc/res/images/";
static constexpr const char* animation_desc_path = "/res/values/charger/animation.txt";
#endif
static int DIGIT_X, DIGIT_Y, PER_START_Y, PER_X, PER_Y, BATT_WIDTH, BATT_HEIGHT, BATT_START_X, BATT_START_Y,
	   BATT_SHELL_GAP, BATT_SHELL_THICK, CAP_ENDS_HEIGHT, CAP_WIDTH, CAP_HEIGHT, CAP_Y;

static int DIGIT_X_SECOND, DIGIT_Y_SECOND, PER_START_Y_SECOND, PER_X_SECOND, PER_Y_SECOND,
        BATT_WIDTH_SECOND, BATT_HEIGHT_SECOND, BATT_START_X_SECOND, BATT_START_Y_SECOND,
        BATT_SHELL_GAP_SECOND, BATT_SHELL_THICK_SECOND, CAP_ENDS_HEIGHT_SECOND, CAP_WIDTH_SECOND, CAP_HEIGHT_SECOND, CAP_Y_SECOND;

char const *const WHITE_FILE = "/sys/class/leds/white/brightness";
char const *const RED_FILE = "/sys/class/leds/red/brightness";
char const *const GREEN_FILE = "/sys/class/leds/green/brightness";
char const *const BLUE_FILE = "/sys/class/leds/blue/brightness";
char const *const COLOR_EFFECT_FILE = "/sys/class/leds/aw22xxx_led/effect";
char const *const COLOR_CFG_FILE = "/sys/class/leds/aw22xxx_led/cfg";
unsigned char* per_data;
unsigned char* batt_flash_data;
unsigned char* batt_data ;
unsigned char* cap_line_red_bottom_data;
unsigned char* cap_line_green_bottom_data;
unsigned char* cap_line_green_top_data;
unsigned char* cap_line_red_data;
unsigned char* cap_line_green_data;

// Added for dual panel project: such as cetus, begin
unsigned char* per_data_second;
unsigned char* batt_flash_data_second;
unsigned char* batt_data_second ;
unsigned char* cap_line_red_bottom_data_second;
unsigned char* cap_line_green_bottom_data_second;
unsigned char* cap_line_green_top_data_second;
unsigned char* cap_line_red_data_second;
unsigned char* cap_line_green_data_second;

#define PLATFORM_L2     1
#define PLATFORM_L3     2
#define PLATFORM_K1      3
#define PLATFORM_K2      4
#define PLATFORM_J18     5
#define PLATFORM_K11     6
#define PLATFORM_J20S    7
#define PLATFORM_J3S     8
#define PLATFORM_J11     9
#define PLATFORM_K82     10
#define PLATFORM_L10     11
#define PLATFORM_M11A    12
#define PLATFORM_M2      13
#define PLATFORM_K81     14
#define PLATFORM_M11     15
#define PLATFORM_L1      16
#define PLATFORM_L81A    17
#define PLATFORM_L2S     18
#define PLATFORM_L18     19
#define PLATFORM_L12     20

int platform_device = 0;
int max_brightness;
static bool Dual_Screen = false;

static int64_t init_time = 0;

static const animation BASE_ANIMATION = {
    .text_clock =
        {
            .pos_x = 0,
            .pos_y = 0,

            .color_r = 255,
            .color_g = 255,
            .color_b = 255,
            .color_a = 255,

            .font = nullptr,
        },
    .text_percent =
        {
            .pos_x = 0,
            .pos_y = 0,

            .color_r = 255,
            .color_g = 255,
            .color_b = 255,
            .color_a = 255,
        },

    .run = false,

    .frames = nullptr,
    .cur_frame = 0,
    .num_frames = 0,
    .first_frame_repeats = 2,

    .cur_cycle = 0,
    .num_cycles = 3,

    .cur_level = 0,
    .cur_status = BATTERY_STATUS_UNKNOWN,
};

void Charger::InitDefaultAnimationFrames() {
    owned_frames_ = {
            {
                    .disp_time = 750,
                    .min_level = 0,
                    .max_level = 19,
                    .surface = NULL,
            },
            {
                    .disp_time = 750,
                    .min_level = 0,
                    .max_level = 39,
                    .surface = NULL,
            },
            {
                    .disp_time = 750,
                    .min_level = 0,
                    .max_level = 59,
                    .surface = NULL,
            },
            {
                    .disp_time = 750,
                    .min_level = 0,
                    .max_level = 79,
                    .surface = NULL,
            },
            {
                    .disp_time = 750,
                    .min_level = 80,
                    .max_level = 95,
                    .surface = NULL,
            },
            {
                    .disp_time = 750,
                    .min_level = 0,
                    .max_level = 100,
                    .surface = NULL,
            },
    };
}

Charger::Charger(ChargerConfigurationInterface* configuration)
    : batt_anim_(BASE_ANIMATION), configuration_(configuration) {}

Charger::~Charger() {}
static int screen_width;
static int screen_height;
static int screen_width_second;
static int screen_height_second;

static int write_str(char const* path, char const* val)
{
    int fd;
    static int already_warned = 0;
    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[32] = {0,};
        strncpy(buffer, val, 31);
        int amt = write(fd, buffer, strlen(buffer) + 1);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            LOGE("write_str failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}
void Charger::write_leds(int brightness)
{
    char str[200];
    int cap;
    cap = health_info_.battery_level;
    sprintf(str, "%d", brightness);
    if (platform_device == PLATFORM_J20S) {
        write_str(WHITE_FILE, str);
    } else if (platform_device == PLATFORM_J3S) {
        write_str(WHITE_FILE, str);
    } else if (platform_device == PLATFORM_J11) {
        write_str(RED_FILE, str);
        write_str(GREEN_FILE, str);
        write_str(BLUE_FILE, str);
    } else if (platform_device == PLATFORM_L10) {
        write_str(COLOR_EFFECT_FILE, str);
        memset(str, 0, sizeof(str));
        sprintf(str, "%d", 1);
        write_str(COLOR_CFG_FILE, str);
    }
}
void Charger::update_leds()
{
    static int status = 0;
    int cap;
    cap = health_info_.battery_level;
    if (cap <= 99) {
            if (status != 1) {
                    write_leds(max_brightness);
                    status = 1;
            }
    } else if (cap == 100) {
            if (status != 2) {
                    write_leds(0);
                    status = 2;
            }
    } else{
            if (status != 3) {
                    write_leds(max_brightness);
                    status = 3;
                    LOGW("cap = %d\n",cap);
            }
    }
}
static int set_backlight(int toggle)
{
    int fd;
    char buffer[10];

    if (platform_device == PLATFORM_M11)
        return 0;

    fd = open(BACKLIGHT_TOGGLE_PATH, O_RDWR);
    if (fd < 0) {
        LOGE("Could not open backlight node : %s", strerror(errno));
        return 0;
    }
    if (toggle) {
        LOGE("Enabling backlight");
        snprintf(buffer, sizeof(int), "%d\n", 307);
    } else {
        LOGE("Disabling backlight");
        snprintf(buffer, sizeof(int), "%d\n", 0);
    }
    if (write(fd, buffer, strlen(buffer)) < 0) {
        LOGE("Could not write to backlight node : %s", strerror(errno));
    }
    close(fd);
    return 0;
}

static int set_backlight_second(int toggle)
{
    int fd;
    char buffer[10];

    fd = open(BACKLIGHT_SECOND_TOGGLE_PATH, O_RDWR);
    if (fd < 0) {
        LOGE("Could not open second backlight node : %s", strerror(errno));
        return 0;
    }
    if (toggle) {
        LOGE("Enabling second backlight");
        snprintf(buffer, sizeof(int), "%d\n", 100);
    } else {
        LOGE("Disabling second backlight");
        snprintf(buffer, sizeof(int), "%d\n", 0);
    }
    if (write(fd, buffer, strlen(buffer)) < 0) {
        LOGE("Could not write to second backlight node : %s", strerror(errno));
    }
    close(fd);
    return 0;
}

/* current time in milliseconds */
static int64_t curr_time_ms() {
    timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return tm.tv_sec * MSEC_PER_SEC + (tm.tv_nsec / NSEC_PER_MSEC);
}

#define MAX_KLOG_WRITE_BUF_SZ 256

static void dump_last_kmsg(void) {
    std::string buf;
    char* ptr;
    size_t len;

    LOGW("\n");
    LOGW("*************** LAST KMSG ***************\n");
    LOGW("\n");
    const char* kmsg[] = {
        // clang-format off
        "/sys/fs/pstore/console-ramoops-0",
        "/sys/fs/pstore/console-ramoops",
        "/proc/last_kmsg",
        // clang-format on
    };
    for (size_t i = 0; i < arraysize(kmsg) && buf.empty(); ++i) {
        auto fd = android_get_control_file(kmsg[i]);
        if (fd >= 0) {
            android::base::ReadFdToString(fd, &buf);
        } else {
            android::base::ReadFileToString(kmsg[i], &buf);
        }
    }

    if (buf.empty()) {
        LOGW("last_kmsg not found. Cold reset?\n");
        goto out;
    }

    len = min(buf.size(), LAST_KMSG_MAX_SZ);
    ptr = &buf[buf.size() - len];

    while (len > 0) {
        size_t cnt = min(len, MAX_KLOG_WRITE_BUF_SZ);
        char yoink;
        char* nl;

        nl = (char*)memrchr(ptr, '\n', cnt - 1);
        if (nl) cnt = nl - ptr + 1;

        yoink = ptr[cnt];
        ptr[cnt] = '\0';
        klog_write(6, "<4>%s", ptr);
        ptr[cnt] = yoink;

        len -= cnt;
        ptr += cnt;
    }

out:
    LOGW("\n");
    LOGW("************* END LAST KMSG *************\n");
    LOGW("\n");
}

int Charger::RequestEnableSuspend() {
	return 0;
    if (!configuration_->ChargerEnableSuspend()) {
        return 0;
    }
    return autosuspend_enable();
}

int Charger::RequestDisableSuspend() {
	return 0;
    if (!configuration_->ChargerEnableSuspend()) {
        return 0;
    }
    return autosuspend_disable();
}
void draw_capacity(unsigned char* buf, unsigned int width, unsigned height, int left, int top)
{
    GRSurface* surface = NULL;
    int w, h, x, y;
    res_create_display_surface_from_rgb_data((const char*)buf, width, height, &surface);
    w = gr_get_width(surface);
    h = gr_get_height(surface);
    x = left;
    y = top;
    gr_blit(surface, 0, 0, w, h, x, y);
    res_free_surface(surface);
}

void draw_capacity_dual_panel(int dis_id, unsigned char* buf, unsigned int width, unsigned height, int left, int top)
{
    GRSurface* surface = NULL;
    int w, h, x, y;
    res_create_display_surface_from_rgb_data((const char*)buf, width, height, &surface);

    w = gr_get_width_mi(surface);
    h = gr_get_height_mi(surface);
    x = left;
    y = top;

    gr_blit_mi(dis_id, surface, 0, 0, w, h, x, y);

    res_free_surface(surface);
}

void draw_digit(unsigned int digit, unsigned int start_x)
{
    if (digit < 10) {
		if (platform_device == PLATFORM_L2 || platform_device == PLATFORM_L1 || platform_device == PLATFORM_K1 ||
			platform_device == PLATFORM_K2 || platform_device == PLATFORM_M11A || platform_device == PLATFORM_M2 ||
			platform_device == PLATFORM_M11 || platform_device == PLATFORM_L2S)
			draw_capacity(digits_data_2k[digit], DIGIT_X, DIGIT_Y, start_x, PER_START_Y);
		else if (platform_device == PLATFORM_J18)
			draw_capacity(digits_data_main_1[digit], DIGIT_X, DIGIT_Y, start_x, PER_START_Y);
		else if (platform_device == PLATFORM_L12)
			draw_capacity(digits_data_1220p[digit], DIGIT_X, DIGIT_Y, start_x, PER_START_Y);
		else
			draw_capacity(digits_data_1080p[digit], DIGIT_X, DIGIT_Y, start_x, PER_START_Y);
    }
}
void draw_percentage(unsigned int start_x)
{
    draw_capacity(per_data, PER_X, PER_Y, start_x, PER_START_Y);
}
void draw_battery_shell(int cap)
{
    if(cap < 1)
        draw_capacity(batt_flash_data, BATT_WIDTH, BATT_HEIGHT, BATT_START_X, BATT_START_Y);
    else
        draw_capacity(batt_data, BATT_WIDTH, BATT_HEIGHT, BATT_START_X, BATT_START_Y);
}
void draw_battery_body_capacity(int cap)
{
    int cap_start_x, cap_start_y;
    int i, cap_h, cap_body;
    unsigned char* cap_data = cap_line_red_data;
    cap_body = cap - 2;
    if(cap_body < 0)
        cap_body = 0;
    cap_h = (CAP_HEIGHT * cap_body)/96;
    cap_start_x = BATT_START_X + BATT_SHELL_GAP + BATT_SHELL_THICK;
    cap_start_y = BATT_START_Y + BATT_HEIGHT - BATT_SHELL_GAP - CAP_ENDS_HEIGHT - BATT_SHELL_THICK - cap_h;
    if(cap > 10 )
        cap_data = cap_line_green_data;

    for (i = 0; i < cap_h; i++) {
        draw_capacity(cap_data, CAP_WIDTH, 1, cap_start_x, cap_start_y + i);
    }
}
void draw_battery_bottom_capacity(int cap)
{
    int cap_start_x, cap_start_y;
    cap_start_x = BATT_START_X + BATT_SHELL_GAP + BATT_SHELL_THICK;
    cap_start_y = BATT_START_Y + BATT_HEIGHT - BATT_SHELL_GAP - CAP_ENDS_HEIGHT - BATT_SHELL_THICK;
    if(cap > 0 && cap <= 10)
        draw_capacity(cap_line_red_bottom_data, CAP_WIDTH, CAP_ENDS_HEIGHT, cap_start_x, cap_start_y);
    else if(cap > 10)
        draw_capacity(cap_line_green_bottom_data, CAP_WIDTH, CAP_ENDS_HEIGHT, cap_start_x, cap_start_y);
}
void draw_battery_up_capacity(int cap)
{
    if(cap > 98)
        draw_capacity(cap_line_green_top_data, CAP_WIDTH, CAP_ENDS_HEIGHT, BATT_START_X + BATT_SHELL_GAP + BATT_SHELL_THICK, BATT_START_Y + BATT_SHELL_GAP + CAP_Y);
}

void draw_digit_dual_panel(int dis_id, unsigned int digit, unsigned int start_x)
{
    if (digit < 10) {
         if (dis_id == DISP_SECONDARY)
            draw_capacity_dual_panel(dis_id, digits_data_1080p[digit], DIGIT_X_SECOND, DIGIT_Y_SECOND, start_x, PER_START_Y_SECOND);
        else
            draw_capacity_dual_panel(dis_id, digits_data_main[digit], DIGIT_X, DIGIT_Y, start_x, PER_START_Y);
    }
}

void draw_percentage_dual_panel(int dis_id, unsigned int start_x)
{
    if (dis_id == DISP_SECONDARY)
        draw_capacity_dual_panel(dis_id, per_data_second, PER_X_SECOND, PER_Y_SECOND, start_x, PER_START_Y_SECOND);
    else
        draw_capacity_dual_panel(dis_id, per_data, PER_X, PER_Y, start_x, PER_START_Y);
}

void draw_battery_shell_dual_panel(int dis_id, int cap)
{
    if(cap < 1) {
         if (dis_id == DISP_SECONDARY)
            draw_capacity_dual_panel(dis_id, batt_flash_data_second, BATT_WIDTH_SECOND, BATT_HEIGHT_SECOND, BATT_START_X_SECOND, BATT_START_Y_SECOND);
        else
            draw_capacity_dual_panel(dis_id, batt_flash_data, BATT_WIDTH, BATT_HEIGHT, BATT_START_X, BATT_START_Y);
    } else {
        if (dis_id == DISP_SECONDARY)
            draw_capacity_dual_panel(dis_id, batt_data_second, BATT_WIDTH_SECOND, BATT_HEIGHT_SECOND, BATT_START_X_SECOND, BATT_START_Y_SECOND);
        else
            draw_capacity_dual_panel(dis_id, batt_data, BATT_WIDTH, BATT_HEIGHT, BATT_START_X, BATT_START_Y);
    }
}

void draw_battery_body_capacity_dual_panel(int dis_id, int cap)
{
    int cap_start_x, cap_start_y, cap_width;
    int i, cap_h, cap_body;
    unsigned char* cap_data = cap_line_red_data;

    cap_body = cap - 2;
    if(cap_body < 0)
        cap_body = 0;
    // draw battery capacity
    if (dis_id == DISP_SECONDARY) {
        cap_h = (CAP_HEIGHT_SECOND * cap_body)/96;
        cap_start_x = BATT_START_X_SECOND+ BATT_SHELL_GAP_SECOND + BATT_SHELL_THICK_SECOND;
        cap_start_y = BATT_START_Y_SECOND + BATT_HEIGHT_SECOND - BATT_SHELL_GAP_SECOND - CAP_ENDS_HEIGHT_SECOND - BATT_SHELL_THICK_SECOND - cap_h;
        cap_width = CAP_WIDTH_SECOND;
    } else {
        cap_h = (CAP_HEIGHT * cap_body)/96;
        cap_start_x = BATT_START_X + BATT_SHELL_GAP + BATT_SHELL_THICK;
        cap_start_y = BATT_START_Y + BATT_HEIGHT - BATT_SHELL_GAP - CAP_ENDS_HEIGHT - BATT_SHELL_THICK - cap_h;
         cap_width = CAP_WIDTH;
    }

    if(cap > 10 ) {
        if (dis_id == DISP_SECONDARY)
            cap_data = cap_line_green_data_second;
         else
            cap_data = cap_line_green_data;
    }

    for (i = 0; i < cap_h; i++) {
        draw_capacity_dual_panel(dis_id, cap_data, cap_width, 1, cap_start_x, cap_start_y + i);
    }

}

//draw the capacity at the bottom or top of battery
void draw_battery_bottom_capacity_dual_panel(int dis_id, int cap)
{
    int cap_start_x, cap_start_y, cap_width, cap_ends_height;
    unsigned char* cap_line_red_bottom_data_temp;
    unsigned char* cap_line_green_bottom_data_temp;

    if (dis_id == DISP_SECONDARY) {
        cap_width = CAP_WIDTH_SECOND;
        cap_ends_height = CAP_ENDS_HEIGHT_SECOND;
        cap_start_x = BATT_START_X_SECOND + BATT_SHELL_GAP_SECOND + BATT_SHELL_THICK_SECOND;
        cap_start_y = BATT_START_Y_SECOND + BATT_HEIGHT_SECOND - BATT_SHELL_GAP_SECOND - CAP_ENDS_HEIGHT_SECOND - BATT_SHELL_THICK_SECOND;
        cap_line_red_bottom_data_temp = cap_line_red_bottom_data_second;
        cap_line_green_bottom_data_temp = cap_line_green_bottom_data_second;
    } else {
        cap_width = CAP_WIDTH;
        cap_ends_height = CAP_ENDS_HEIGHT;
        cap_start_x = BATT_START_X + BATT_SHELL_GAP + BATT_SHELL_THICK;
        cap_start_y = BATT_START_Y + BATT_HEIGHT - BATT_SHELL_GAP - CAP_ENDS_HEIGHT - BATT_SHELL_THICK;
        cap_line_red_bottom_data_temp = cap_line_red_bottom_data;
        cap_line_green_bottom_data_temp = cap_line_green_bottom_data;
    }

    if(cap > 0 && cap <= 10)
        draw_capacity_dual_panel(dis_id, cap_line_red_bottom_data_temp, cap_width, cap_ends_height, cap_start_x, cap_start_y);
    else if(cap > 10)
        draw_capacity_dual_panel(dis_id, cap_line_green_bottom_data_temp, cap_width, cap_ends_height, cap_start_x, cap_start_y);
}

void draw_battery_up_capacity_dual_panel(int dis_id, int cap)
{
    unsigned char* cap_line_green_top_data_temp;
    if(cap > 98) {
        if (dis_id == DISP_SECONDARY) {
            cap_line_green_top_data_temp = cap_line_green_top_data_second;
            draw_capacity_dual_panel(dis_id, cap_line_green_top_data_temp, CAP_WIDTH_SECOND, CAP_ENDS_HEIGHT_SECOND, BATT_START_X_SECOND + BATT_SHELL_GAP_SECOND + BATT_SHELL_THICK_SECOND, BATT_START_Y_SECOND + BATT_SHELL_GAP_SECOND + CAP_Y_SECOND);
        } else {
           cap_line_green_top_data_temp = cap_line_green_top_data;
            draw_capacity_dual_panel(dis_id, cap_line_green_top_data_temp, CAP_WIDTH, CAP_ENDS_HEIGHT, BATT_START_X + BATT_SHELL_GAP + BATT_SHELL_THICK, BATT_START_Y + BATT_SHELL_GAP + CAP_Y);
        }
    }
}

static int BATT_START_X_J18_FLIP = 331;
void res_battery_init(void)
{
	if (platform_device == PLATFORM_L1 ||platform_device == PLATFORM_L2 || platform_device == PLATFORM_K1 ||
		platform_device == PLATFORM_K2 || platform_device == PLATFORM_M11A || platform_device == PLATFORM_M2 ||
		platform_device == PLATFORM_M11 || platform_device == PLATFORM_L2S) {
		pdigit_info = &digit_info_2k;
		pbatt_info = &batt_info_2k;
		pcap_info = &cap_info_2k;
		per_data = per_data_2k;
		batt_flash_data = batt_flash_data_2k;
		batt_data = batt_data_2k;
		cap_line_red_bottom_data = cap_line_red_bottom_data_2k;
		cap_line_green_bottom_data = cap_line_green_bottom_data_2k;
		cap_line_green_top_data = cap_line_green_top_data_2k;
		cap_line_green_data = cap_line_green_data_2k;
		cap_line_red_data = cap_line_red_data_2k;
	} else if (platform_device == PLATFORM_J18) {
		pdigit_info = &digit_info_main_1;
		pbatt_info = &batt_info_main_1;
		pcap_info = &cap_info_main_1;
		per_data = per_data_main_1;
		batt_flash_data = batt_flash_data_main_1;
		batt_data = batt_data_main_1;
		cap_line_red_bottom_data = cap_line_red_bottom_data_main_1;
		cap_line_green_bottom_data = cap_line_green_bottom_data_main_1;
		cap_line_green_top_data = cap_line_green_top_data_main_1;
		cap_line_green_data = cap_line_green_data_main_1;
		cap_line_red_data = cap_line_red_data_main_1;

        // dual panel flip panel init resources begin
		pdigit_info_second = &digit_info_flip;
		pbatt_info_second = &batt_info_flip;
		pcap_info_second = &cap_info_flip;
		per_data_second= per_data_flip;
		batt_flash_data_second= batt_flash_data_flip;
		batt_data_second= batt_data_flip;
		cap_line_red_bottom_data_second= cap_line_red_bottom_data_flip;
		cap_line_green_bottom_data_second = cap_line_green_bottom_data_flip;
		cap_line_green_top_data_second = cap_line_green_top_data_flip;
		cap_line_green_data_second = cap_line_green_data_flip;
		cap_line_red_data_second = cap_line_red_data_flip;
		DIGIT_X_SECOND = pdigit_info_second->DIGIT_X;
		DIGIT_Y_SECOND = pdigit_info_second->DIGIT_Y;
		PER_START_Y_SECOND = pdigit_info_second->PER_START_Y;
		PER_X_SECOND = pdigit_info_second->PER_X;
		PER_Y_SECOND = pdigit_info_second->PER_Y;
		BATT_WIDTH_SECOND = pbatt_info_second->BATT_WIDTH;
		BATT_HEIGHT_SECOND = pbatt_info_second->BATT_HEIGHT;
		BATT_START_X_SECOND = BATT_START_X_J18_FLIP;
		BATT_START_Y_SECOND = pbatt_info_second->BATT_START_Y;
		BATT_SHELL_GAP_SECOND = pbatt_info_second->BATT_SHELL_GAP;
		BATT_SHELL_THICK_SECOND = pbatt_info_second->BATT_SHELL_THICK;
		CAP_ENDS_HEIGHT_SECOND = pcap_info_second->CAP_ENDS_HEIGHT;
		CAP_WIDTH_SECOND = pcap_info_second->CAP_WIDTH;
		CAP_HEIGHT_SECOND = pcap_info_second->CAP_HEIGHT;
		CAP_Y_SECOND = pcap_info_second->CAP_Y;
		//dual panel flip panel init resources end
	} else if(platform_device == PLATFORM_L18) { 
        pdigit_info = &digit_info_main;
		pbatt_info = &batt_info_main;
		pcap_info = &cap_info_main;
		per_data = per_data_main;
		batt_flash_data = batt_flash_data_main;
		batt_data = batt_data_main;
		cap_line_red_bottom_data = cap_line_red_bottom_data_main;
		cap_line_green_bottom_data = cap_line_green_bottom_data_main;
		cap_line_green_top_data = cap_line_green_top_data_main;
		cap_line_green_data = cap_line_green_data_main;
		cap_line_red_data = cap_line_red_data_main;
		// dual panel flip panel init resources begin
		pdigit_info_second = &digit_info_flip;
		pbatt_info_second = &batt_info_flip;
		pcap_info_second = &cap_info_flip;
		per_data_second= per_data_flip;
		batt_flash_data_second= batt_flash_data_flip;
		batt_data_second= batt_data_flip;
		cap_line_red_bottom_data_second= cap_line_red_bottom_data_flip;
		cap_line_green_bottom_data_second = cap_line_green_bottom_data_flip;
		cap_line_green_top_data_second = cap_line_green_top_data_flip;
		cap_line_green_data_second = cap_line_green_data_flip;
		cap_line_red_data_second = cap_line_red_data_flip;
		DIGIT_X_SECOND = pdigit_info_second->DIGIT_X;
		DIGIT_Y_SECOND = pdigit_info_second->DIGIT_Y;
		PER_START_Y_SECOND = pdigit_info_second->PER_START_Y;
		PER_X_SECOND = pdigit_info_second->PER_X;
		PER_Y_SECOND = pdigit_info_second->PER_Y;
		BATT_WIDTH_SECOND = pbatt_info_second->BATT_WIDTH;
		BATT_HEIGHT_SECOND = pbatt_info_second->BATT_HEIGHT;
		BATT_START_X_SECOND = pbatt_info_second->BATT_START_X;
		BATT_START_Y_SECOND = pbatt_info_second->BATT_START_Y;
		BATT_SHELL_GAP_SECOND = pbatt_info_second->BATT_SHELL_GAP;
		BATT_SHELL_THICK_SECOND = pbatt_info_second->BATT_SHELL_THICK;
		CAP_ENDS_HEIGHT_SECOND = pcap_info_second->CAP_ENDS_HEIGHT;
		CAP_WIDTH_SECOND = pcap_info_second->CAP_WIDTH;
		CAP_HEIGHT_SECOND = pcap_info_second->CAP_HEIGHT;
		CAP_Y_SECOND = pcap_info_second->CAP_Y;
		//dual panel flip panel init resources end

	}else if(platform_device == PLATFORM_L12) {
		pdigit_info = &digit_info_1220p;
		pbatt_info = &batt_info_1220p;
		pcap_info = &cap_info_1220p;
		per_data = per_data_1220p;
		batt_flash_data = batt_flash_data_1220p;
		batt_data = batt_data_1220p;
		cap_line_red_bottom_data = cap_line_red_bottom_data_1220p;
		cap_line_green_bottom_data = cap_line_green_bottom_data_1220p;
		cap_line_green_top_data = cap_line_green_top_data_1220p;
		cap_line_green_data = cap_line_green_data_1220p;
		cap_line_red_data = cap_line_red_data_1220p;
	} else {
		pdigit_info = &digit_info_1080p;
		pbatt_info = &batt_info_1080p;
		pcap_info = &cap_info_1080p;
		per_data = per_data_1080p;
		batt_flash_data = batt_flash_data_1080p;
		batt_data = batt_data_1080p;
		cap_line_red_bottom_data = cap_line_red_bottom_data_1080p;
		cap_line_green_bottom_data = cap_line_green_bottom_data_1080p;
		cap_line_green_top_data = cap_line_green_top_data_1080p;
		cap_line_green_data = cap_line_green_data_1080p;
		cap_line_red_data = cap_line_red_data_1080p;
	}
	DIGIT_X = pdigit_info->DIGIT_X;
	DIGIT_Y = pdigit_info->DIGIT_Y;
	PER_START_Y = pdigit_info->PER_START_Y;
	PER_X = pdigit_info->PER_X;
	PER_Y = pdigit_info->PER_Y;
	BATT_WIDTH = pbatt_info->BATT_WIDTH;
	BATT_HEIGHT = pbatt_info->BATT_HEIGHT;
    	if((platform_device == PLATFORM_K82)
			|| (platform_device == PLATFORM_K81)
			|| (platform_device == PLATFORM_L81A))
	    BATT_START_X = BATT_START_LARGE_X;
    	else
	    BATT_START_X = pbatt_info->BATT_START_X;
	BATT_START_Y = pbatt_info->BATT_START_Y;
	BATT_SHELL_GAP = pbatt_info->BATT_SHELL_GAP;
	BATT_SHELL_THICK = pbatt_info->BATT_SHELL_THICK;
	CAP_ENDS_HEIGHT = pcap_info->CAP_ENDS_HEIGHT;
	CAP_WIDTH = pcap_info->CAP_WIDTH;
	CAP_HEIGHT = pcap_info->CAP_HEIGHT;
	CAP_Y = pcap_info->CAP_Y;
}
void draw_battery_percentage(int cap)
{
    int per_start_x;
    if (cap < 10) {
        per_start_x = screen_width/2 - DIGIT_X - 5;
        draw_digit(cap, per_start_x);
        per_start_x += DIGIT_X;
        draw_percentage(per_start_x);
    }
    if ((cap >= 10) && (cap < 100)) {
        per_start_x = screen_width/2 - DIGIT_X - DIGIT_X/2 - 5;
        draw_digit(cap/10, per_start_x);
        per_start_x += DIGIT_X;
        draw_digit(cap%10, per_start_x);
        per_start_x += DIGIT_X;
        draw_percentage(per_start_x);
    }
    if (cap == 100) {
        per_start_x = screen_width/2 - DIGIT_X - DIGIT_X - 5;
        draw_digit(1, per_start_x);
        per_start_x += DIGIT_X;
        draw_digit(0, per_start_x);
        per_start_x += DIGIT_X;
        draw_digit(0, per_start_x);
        per_start_x += DIGIT_X;
        draw_percentage(per_start_x);
    }
}

void draw_battery_percentage_dual_panel(int dis_id, int cap)
{
    int per_start_x;
    int digit_x, screen_width_temp;

    if (dis_id == DISP_SECONDARY) {
        digit_x = DIGIT_X_SECOND;
        screen_width_temp = screen_width_second;
    } else {
        digit_x = DIGIT_X;
        screen_width_temp = screen_width;
    }
    if (cap < 10) {
        per_start_x = screen_width_temp/2 - digit_x - 5;
        draw_digit_dual_panel(dis_id, cap, per_start_x);
        per_start_x += digit_x;
        draw_percentage_dual_panel(dis_id, per_start_x);
    }
    if ((cap >= 10) && (cap < 100)) {
        per_start_x = screen_width_temp/2 - digit_x - digit_x/2 - 5;
        draw_digit_dual_panel(dis_id, cap/10, per_start_x);
        per_start_x += digit_x;
        draw_digit_dual_panel(dis_id, cap%10, per_start_x);
        per_start_x += digit_x;
        draw_percentage_dual_panel(dis_id, per_start_x);
    }
    if (cap == 100) {
        per_start_x = screen_width_temp/2 - digit_x - digit_x - 5;
        draw_digit_dual_panel(dis_id, 1, per_start_x);
        per_start_x += digit_x;
        draw_digit_dual_panel(dis_id, 0, per_start_x);
        per_start_x += digit_x;
        draw_digit_dual_panel(dis_id, 0, per_start_x);
        per_start_x += digit_x;
        draw_percentage_dual_panel(dis_id, per_start_x);
    }

}

void draw_battery_capacity(int cap)
{
    draw_battery_bottom_capacity(cap);
    draw_battery_body_capacity(cap);
    draw_battery_up_capacity(cap);
}


void draw_battery_capacity_dual_panel(int dis_id, int cap)
{
    //draw bottom
    draw_battery_bottom_capacity_dual_panel(dis_id, cap);
    //draw body
    draw_battery_body_capacity_dual_panel(dis_id, cap);
    //draw top
    draw_battery_up_capacity_dual_panel(dis_id, cap);
}

void clear_screen()
{
    gr_color(0, 0, 0, 255);
    gr_clear();
}

void clear_screen_dual_panel(int dis_id)
{
    gr_color_mi(0, 0, 0, 255);
    gr_clear_mi(dis_id);
}

void Charger::draw_screen()
{
    int cap;
    clear_screen();
    cap = health_info_.battery_level;
    draw_battery_shell(cap);
    draw_battery_capacity(cap);
    draw_battery_percentage(cap);
    gr_flip();
}

void Charger::draw_screen_dual_panel(int dis_id)
{
    int cap;

    clear_screen_dual_panel(dis_id);

    LOGE("screen_width: %d screen_height: %d screen_width_flip:%d\n", screen_width, screen_height, screen_width_second);

    // read battery capcacity
    cap = health_info_.battery_level;

    //draw battery frame
    draw_battery_shell_dual_panel(dis_id, cap);

    //draw battery capacity
    draw_battery_capacity_dual_panel(dis_id, cap);

    //draw battery percentage
    draw_battery_percentage_dual_panel(dis_id, cap);

    gr_flip_mi(dis_id);
}

static void kick_animation(animation* anim) {
    anim->run = true;
}

static void reset_animation(animation* anim) {
    anim->cur_cycle = 0;
    anim->cur_frame = 0;
    anim->run = false;
}

void Charger::UpdateScreenState(int64_t now) {
    int disp_time;

    if (!batt_anim_.run || now < next_screen_transition_) return;

    // If battery level is not ready, keep checking in the defined time
    if (health_info_.battery_level == 0 && health_info_.battery_status == BatteryStatus::UNKNOWN) {
        if (wait_batt_level_timestamp_ == 0) {
            // Set max delay time and skip drawing screen
            wait_batt_level_timestamp_ = now + MAX_BATT_LEVEL_WAIT_TIME;
            LOGV("[%" PRId64 "] wait for battery capacity ready\n", now);
            return;
        } else if (now <= wait_batt_level_timestamp_) {
            // Do nothing, keep waiting
            return;
        }
        // If timeout and battery level is still not ready, draw unknown battery
    }

    if (healthd_draw_ == nullptr) {
        std::optional<bool> out_screen_on = configuration_->ChargerShouldKeepScreenOn();
        if (out_screen_on.has_value()) {
            if (!*out_screen_on) {
                LOGV("[%" PRId64 "] leave screen off\n", now);
                batt_anim_.run = false;
                next_screen_transition_ = -1;
                if (configuration_->ChargerIsOnline()) {
                    RequestEnableSuspend();
                }
                return;
            }
        }

        healthd_draw_ = HealthdDraw::Create(&batt_anim_);
        screen_width = gr_fb_width();
        screen_height = gr_fb_height();
        if (healthd_draw_ == nullptr) return;

#if !defined(__ANDROID_VNDK__)
        if (android::sysprop::ChargerProperties::disable_init_blank().value_or(false)) {
            LOGE("set backlight to false and blank panels\n");
            if (Dual_Screen == true) {
                set_backlight_second(false);
                gr_fb_blank_mi(DISP_SECONDARY, true);
                usleep(50 * 1000);
                set_backlight(false);
                gr_fb_blank_mi(DISP_PRIMARY, true);
            } else {
                set_backlight(false);
                healthd_draw_->blank_screen(true, static_cast<int>(drm_));
            }
            screen_blanked_ = true;
        }
#endif
    }

    /* animation is over, blank screen and leave */
    if (batt_anim_.num_cycles > 0 && batt_anim_.cur_cycle == batt_anim_.num_cycles) {
        reset_animation(&batt_anim_);
        LOGV("animation is over, blank screen and leave\n");
        next_screen_transition_ = -1;
        charger_power_key_ = false;
        if (Dual_Screen == true) {
            set_backlight_second(false);
            gr_fb_blank_mi(DISP_SECONDARY, true);
            usleep(10 * 1000);
            set_backlight(false);
            gr_fb_blank_mi(DISP_PRIMARY, true);
         } else {
            set_backlight(false);
            healthd_draw_->blank_screen(true, static_cast<int>(drm_));
        }
        screen_blanked_ = true;
        LOGV("[%" PRId64 "] animation done\n", now);
        if (configuration_->ChargerIsOnline()) {
            RequestEnableSuspend();
        }
        return;
    }

    disp_time = batt_anim_.frames[batt_anim_.cur_frame].disp_time;

    /* turn off all screen */
    if (screen_switch_ == SCREEN_SWITCH_ENABLE) {
        healthd_draw_->blank_screen(true, 0 /* drm */);
        healthd_draw_->blank_screen(true, 1 /* drm */);
        healthd_draw_->rotate_screen(static_cast<int>(drm_));
        screen_blanked_ = true;
        screen_switch_ = SCREEN_SWITCH_DISABLE;
    }

    /* unblank the screen on first cycle and first frame */
    if (batt_anim_.cur_cycle == 0 && batt_anim_.cur_frame == 0) {
        LOGV("unblank main  screen on first cycle and first frame\n");
       if (Dual_Screen == true) {
             gr_fb_blank_mi(DISP_SECONDARY, false);
             set_backlight_second(true);
             usleep(10 * 1000);
             gr_fb_blank_mi(DISP_PRIMARY, false);
             set_backlight(true);
         } else {
             healthd_draw_->blank_screen(false, static_cast<int>(drm_));
             set_backlight(true);
         }
    }

    /* animation starting, set up the animation */
    if (batt_anim_.cur_frame == 0) {
        LOGV("[%" PRId64 "] animation starting\n", now);
        batt_anim_.cur_level = health_info_.battery_level;
            /* find first frame given current battery level */

                // repeat the first frame first_frame_repeats times

    }

    /* draw the new frame (@ cur_frame) */
    //healthd_draw_->redraw_screen(&batt_anim_, surf_unknown_);
   if (Dual_Screen == true) {
        draw_screen_dual_panel(DISP_SECONDARY);
        usleep(10 * 1000);
        draw_screen_dual_panel(DISP_PRIMARY);
    } else {
        draw_screen();
    }


    if (batt_anim_.cur_cycle == 0) {
        if (Dual_Screen == true)
            set_backlight_second(true);
        set_backlight(true);
    }

    /* if we don't have anim frames, we only have one image, so just bump
     * the cycle counter and exit
     */
    if (batt_anim_.num_frames == 0 || batt_anim_.cur_level < 0) {
        LOGW("[%" PRId64 "] animation missing or unknown battery status\n", now);
        next_screen_transition_ = now + BATTERY_UNKNOWN_TIME;
        batt_anim_.cur_cycle++;
        return;
    }

    /* schedule next screen transition */
    next_screen_transition_ = curr_time_ms() + disp_time;

    /* advance frame cntr to the next valid frame only if we are charging
     * if necessary, advance cycle cntr, and reset frame cntr
     */
    if (configuration_->ChargerIsOnline()) {
        batt_anim_.cur_frame++;

        while (batt_anim_.cur_frame < batt_anim_.num_frames &&
               (batt_anim_.cur_level < batt_anim_.frames[batt_anim_.cur_frame].min_level ||
                batt_anim_.cur_level > batt_anim_.frames[batt_anim_.cur_frame].max_level)) {
            batt_anim_.cur_frame++;
        }
        if (batt_anim_.cur_frame >= batt_anim_.num_frames) {
            batt_anim_.cur_cycle++;
            batt_anim_.cur_frame = 0;

            /* don't reset the cycle counter, since we use that as a signal
             * in a test above to check if animation is over
             */
        }
    } else {
        /* Stop animating if we're not charging.
         * If we stop it immediately instead of going through this loop, then
         * the animation would stop somewhere in the middle.
         */
        batt_anim_.cur_frame = 0;
        batt_anim_.cur_cycle++;
    }
}


int Charger::SetKeyCallback(int code, int value) {
    int64_t now = curr_time_ms();
    int down = !!value;

    if (code > KEY_MAX) return -1;

    /* ignore events that don't modify our state */
    if (keys_[code].down == down) return 0;

    /* only record the down even timestamp, as the amount
     * of time the key spent not being pressed is not useful */
    if (down) keys_[code].timestamp = now;
    keys_[code].down = down;
    keys_[code].pending = true;
    if (down) {
        LOGV("[%" PRId64 "] key[%d] down\n", now, code);
    } else {
        int64_t duration = now - keys_[code].timestamp;
        int64_t secs = duration / 1000;
        int64_t msecs = duration - secs * 1000;
        LOGV("[%" PRId64 "] key[%d] up (was down for %" PRId64 ".%" PRId64 "sec)\n", now, code,
             secs, msecs);
    }

    return 0;
}

int Charger::SetSwCallback(int code, int value) {
    if (code > SW_MAX) return -1;
    if (code == SW_LID) {
        if ((screen_switch_ == SCREEN_SWITCH_DEFAULT) || ((value != 0) && (drm_ == DRM_INNER)) ||
            ((value == 0) && (drm_ == DRM_OUTER))) {
            screen_switch_ = SCREEN_SWITCH_ENABLE;
            drm_ = (value != 0) ? DRM_OUTER : DRM_INNER;
            keys_[code].pending = true;
        }
    }

    return 0;
}

void Charger::UpdateInputState(input_event* ev) {
    if (ev->type == EV_SW && ev->code == SW_LID) {
        SetSwCallback(ev->code, ev->value);
        return;
    }

    if (ev->type != EV_KEY) return;
    SetKeyCallback(ev->code, ev->value);
}

void Charger::SetNextKeyCheck(key_state* key, int64_t timeout) {
    int64_t then = key->timestamp + timeout;

    if (next_key_check_ == -1 || then < next_key_check_) next_key_check_ = then;
}

void Charger::ProcessKey(int code, int64_t now) {
    key_state* key = &keys_[code];

    if (code == KEY_POWER) {
        if (key->down) {
             if (charger_power_key_ == false) {
                if(Dual_Screen == true) {
                    set_backlight_second(false);
                        //usleep(10 * 1000);
                    set_backlight(false);
                    gr_fb_blank_mi(DISP_SECONDARY, true);
                    gr_fb_blank_mi(DISP_PRIMARY, true);
                } else {
                    set_backlight(false);
                    healthd_draw_->blank_screen(true, static_cast<int>(drm_));
                }
                charger_power_key_ = true;
            }
            int64_t reboot_timeout = key->timestamp + POWER_ON_KEY_TIME;
            if (now >= reboot_timeout) {
                /* We do not currently support booting from charger mode on
                   all devices. Check the property and continue booting or reboot
                   accordingly. */
                if (property_get_bool("ro.enable_boot_charger_mode", false)) {
                    LOGW("[%" PRId64 "] booting from charger mode\n", now);
                    property_set("sys.boot_from_charger_mode", "1");
                } else {
                    if (batt_anim_.cur_level >= boot_min_cap_) {
                        LOGW("[%" PRId64 "] rebooting\n", now);
                        if(Dual_Screen == true)
                            gr_exit_mi();
                        reboot(RB_AUTOBOOT);
                    } else {
                        LOGV("[%" PRId64
                             "] ignore power-button press, battery level "
                             "less than minimum\n",
                             now);
                    }
                }
            } else {
                /* if the key is pressed but timeout hasn't expired,
                 * make sure we wake up at the right-ish time to check
                 */
                SetNextKeyCheck(key, POWER_ON_KEY_TIME);

                /* Turn on the display and kick animation on power-key press
                 * rather than on key release
                 */
               // kick_animation(&batt_anim_);
               //request_suspend(false);
            }
        } else {
            /* if the power key got released, force screen state cycle */
            if (key->pending) {
                if (!batt_anim_.run) {
                kick_animation(&batt_anim_);
                } else {
                    reset_animation(&batt_anim_);
                    next_screen_transition_ = -1;
                    if(Dual_Screen == true) {
                        gr_fb_blank_mi(DISP_SECONDARY, true);
                        usleep(10 * 1000);
                        gr_fb_blank_mi(DISP_PRIMARY, true);
                    } else 
                        healthd_draw_->blank_screen(true, static_cast<int>(drm_));
                    if (configuration_->ChargerIsOnline())
                        RequestEnableSuspend();
                }
            }
        }
    }

    key->pending = false;
}

void Charger::ProcessHallSensor(int code) {
    key_state* key = &keys_[code];

    if (code == SW_LID) {
        if (key->pending) {
            reset_animation(&batt_anim_);
            kick_animation(&batt_anim_);
            RequestDisableSuspend();
        }
    }

    key->pending = false;
}

void Charger::HandleInputState(int64_t now) {
    ProcessKey(KEY_POWER, now);

    if (next_key_check_ != -1 && now > next_key_check_) next_key_check_ = -1;

    ProcessHallSensor(SW_LID);
}

void Charger::HandlePowerSupplyState(int64_t now) {
    int timer_shutdown = UNPLUGGED_SHUTDOWN_TIME;
    if (!have_battery_state_) return;

    if (!configuration_->ChargerIsOnline()) {
        RequestDisableSuspend();
        if (next_pwr_check_ == -1) {
            /* Last cycle would have stopped at the extreme top of battery-icon
             * Need to show the correct level corresponding to capacity.
             *
             * Reset next_screen_transition_ to update screen immediately.
             * Reset & kick animation to show complete animation cycles
             * when charger disconnected.
             */
            timer_shutdown =
                    property_get_int32(UNPLUGGED_SHUTDOWN_TIME_PROP, UNPLUGGED_SHUTDOWN_TIME);
            next_screen_transition_ = now - 1;
            reset_animation(&batt_anim_);
            kick_animation(&batt_anim_);
            next_pwr_check_ = now + timer_shutdown;
            LOGW("[%" PRId64 "] device unplugged: shutting down in %" PRId64 " (@ %" PRId64 ")\n",
                 now, (int64_t)timer_shutdown, next_pwr_check_);
        } else if (now >= next_pwr_check_) {
            LOGW("[%" PRId64 "] shutting down\n", now);
            if(Dual_Screen == true)
                gr_exit_mi();
            reboot(RB_POWER_OFF);
        } else {
            /* otherwise we already have a shutdown timer scheduled */
        }
    } else {
        /* online supply present, reset shutdown timer if set */
        if (next_pwr_check_ != -1) {
            /* Reset next_screen_transition_ to update screen immediately.
             * Reset & kick animation to show complete animation cycles
             * when charger connected again.
             */
            RequestDisableSuspend();
            next_screen_transition_ = now - 1;
            reset_animation(&batt_anim_);
            kick_animation(&batt_anim_);
            LOGW("[%" PRId64 "] device plugged in: shutdown cancelled\n", now);
        }
        next_pwr_check_ = -1;
    }
}

void Charger::OnHeartbeat() {
    // charger* charger = &charger_state;
    int64_t now = curr_time_ms();

    HandleInputState(now);
    HandlePowerSupplyState(now);

    /* do screen update last in case any of the above want to start
     * screen transitions (animations, etc)
     */
    UpdateScreenState(now);
    if ((platform_device != PLATFORM_L10) || (now - init_time > LED_PREPARE_TIME))
        update_leds();
}

void Charger::OnHealthInfoChanged(const ChargerHealthInfo& health_info) {
    if (!have_battery_state_) {
        have_battery_state_ = true;
        next_screen_transition_ = curr_time_ms() - 1;
        RequestDisableSuspend();
        reset_animation(&batt_anim_);
        kick_animation(&batt_anim_);
    }
    health_info_ = health_info;
}

int Charger::OnPrepareToWait(void) {
    int64_t now = curr_time_ms();
    int64_t next_event = INT64_MAX;
    int64_t timeout;

    LOGV("[%" PRId64 "] next screen: %" PRId64 " next key: %" PRId64 " next pwr: %" PRId64 "\n",
         now, next_screen_transition_, next_key_check_, next_pwr_check_);

    if (next_screen_transition_ != -1) next_event = next_screen_transition_;
    if (next_key_check_ != -1 && next_key_check_ < next_event) next_event = next_key_check_;
    if (next_pwr_check_ != -1 && next_pwr_check_ < next_event) next_event = next_pwr_check_;

    if (next_event != -1 && next_event != INT64_MAX)
        timeout = max(0, next_event - now);
    else
        timeout = -1;

    return (int)timeout;
}

int Charger::InputCallback(int fd, unsigned int epevents) {
    input_event ev;
    int ret;

    ret = ev_get_input(fd, epevents, &ev);
    if (ret) return -1;
    UpdateInputState(&ev);
    return 0;
}

static void charger_event_handler(HealthLoop* /*charger_loop*/, uint32_t /*epevents*/) {
    int ret;

    ret = ev_wait(-1);
    if (!ret) ev_dispatch();
}

void Charger::InitAnimation() {
    bool parse_success;

    std::string content;

#if defined(__ANDROID_VNDK__)
    if (base::ReadFileToString(vendor_animation_desc_path, &content)) {
        parse_success = parse_animation_desc(content, &batt_anim_);
        batt_anim_.set_resource_root(vendor_animation_root);
    } else {
        LOGW("Could not open animation description at %s\n", vendor_animation_desc_path);
        parse_success = false;
    }
#else
    if (base::ReadFileToString(product_animation_desc_path, &content)) {
        parse_success = parse_animation_desc(content, &batt_anim_);
        batt_anim_.set_resource_root(product_animation_root);
    } else if (base::ReadFileToString(animation_desc_path, &content)) {
        parse_success = parse_animation_desc(content, &batt_anim_);
        // Fallback resources always exist in system_animation_root. On legacy devices with an old
        // ramdisk image, resources may be overridden under root. For example,
        // /res/images/charger/battery_fail.png may not be the same as
        // system/core/healthd/images/battery_fail.png in the source tree, but is a device-specific
        // image. Hence, load from /res, and fall back to /system/etc/res.
        batt_anim_.set_resource_root(legacy_animation_root, system_animation_root);
    } else {
        LOGW("Could not open animation description at %s\n", animation_desc_path);
        parse_success = false;
    }
#endif

#if defined(__ANDROID_VNDK__)
    auto default_animation_root = vendor_default_animation_root;
#else
    auto default_animation_root = system_animation_root;
#endif

    if (!parse_success) {
        LOGW("Could not parse animation description. "
             "Using default animation with resources at %s\n",
             default_animation_root);
        batt_anim_ = BASE_ANIMATION;
        batt_anim_.animation_file.assign(default_animation_root + "charger/battery_scale.png"s);
        InitDefaultAnimationFrames();
        batt_anim_.frames = owned_frames_.data();
        batt_anim_.num_frames = owned_frames_.size();
    }
    if (batt_anim_.fail_file.empty()) {
        batt_anim_.fail_file.assign(default_animation_root + "charger/battery_fail.png"s);
    }

    LOGV("Animation Description:\n");
    LOGV("  animation: %d %d '%s' (%d)\n", batt_anim_.num_cycles, batt_anim_.first_frame_repeats,
         batt_anim_.animation_file.c_str(), batt_anim_.num_frames);
    LOGV("  fail_file: '%s'\n", batt_anim_.fail_file.c_str());
    LOGV("  clock: %d %d %d %d %d %d '%s'\n", batt_anim_.text_clock.pos_x,
         batt_anim_.text_clock.pos_y, batt_anim_.text_clock.color_r, batt_anim_.text_clock.color_g,
         batt_anim_.text_clock.color_b, batt_anim_.text_clock.color_a,
         batt_anim_.text_clock.font_file.c_str());
    LOGV("  percent: %d %d %d %d %d %d '%s'\n", batt_anim_.text_percent.pos_x,
         batt_anim_.text_percent.pos_y, batt_anim_.text_percent.color_r,
         batt_anim_.text_percent.color_g, batt_anim_.text_percent.color_b,
         batt_anim_.text_percent.color_a, batt_anim_.text_percent.font_file.c_str());
    for (int i = 0; i < batt_anim_.num_frames; i++) {
        LOGV("  frame %.2d: %d %d %d\n", i, batt_anim_.frames[i].disp_time,
             batt_anim_.frames[i].min_level, batt_anim_.frames[i].max_level);
    }
}

long read_input_online(char const* path)
{
    char str[200];
    int fd;
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOGE("Could not open charger/dc/wireless online node : %s", strerror(errno));
        return 0;
    }
    read(fd, str, 200);
    close(fd);
    return atol(str);
}
static int check_count = 0;
void Charger::OnInit(struct healthd_config* config) {
    int ret;
    int i;
    int epollfd;
    char device[PROPERTY_VALUE_MAX] = { 0 };
    int charger_present, wireless_present, dc_present;

    dump_last_kmsg();
    if (property_get("ro.product.device", device, NULL) > 0) {
	    if (!strncmp(device, "zeus", strlen("zeus"))){
		    platform_device = PLATFORM_L2;
	    } else if (!strncmp(device, "star", strlen("star"))) {
		    platform_device = PLATFORM_K1;
	    } else if (!strncmp(device, "cupid", strlen("cupid"))) {
		    platform_device = PLATFORM_L3;
	    } else if (!strncmp(device, "mars", strlen("mars"))) {
		    platform_device = PLATFORM_K1;
	    } else if (!strncmp(device, "venus", strlen("venus"))) {
		    platform_device = PLATFORM_K2;
	    } else if (!strncmp(device, "cetus", strlen("cetus"))) {
		    platform_device = PLATFORM_J18;
	    } else if (!strncmp(device, "haydn", strlen("haydn"))) {
		    platform_device = PLATFORM_K11;
	    } else if (!strncmp(device, "vayu", strlen("vayu")) ||
		!strncmp(device, "bhima", strlen("bhima"))) {
		    platform_device = PLATFORM_J20S;
		    max_brightness = 40;
	    } else if (!strncmp(device, "apollo", strlen("apollo"))) {
		    platform_device = PLATFORM_J3S;
		    max_brightness = 2;
	    } else if (!strncmp(device, "lmi", strlen("lmi"))) {
		    platform_device = PLATFORM_J11;
		    max_brightness = 30;
	    } else if (!strncmp(device, "nabu", strlen("nabu"))) {
		    platform_device = PLATFORM_K82;
	    } else if (!strncmp(device, "ingres", strlen("ingres"))) {
		    platform_device = PLATFORM_L10;
		    max_brightness = 16;
	    } else if (!strncmp(device, "mondrian", strlen("mondrian"))) {
                    platform_device = PLATFORM_M11A;
            } else if (!strncmp(device, "nuwa", strlen("nuwa"))){
                    platform_device = PLATFORM_M2;
            } else if (!strncmp(device, "socrates", strlen("socrates"))){
                    platform_device = PLATFORM_M11;
            } else if (!strncmp(device, "thor", strlen("thor"))) {
		    platform_device = PLATFORM_L1;
            } else if (!strncmp(device, "loki", strlen("loki"))) {
		    platform_device = PLATFORM_L1;
	    } else if (!strncmp(device, "enuma", strlen("enuma")) ||
                    !strncmp(device, "elish", strlen("elish"))) {
                    platform_device = PLATFORM_K81;
	    } else if (!strncmp(device, "dagu", strlen("dagu"))){
                    platform_device = PLATFORM_L81A;
	    } else if (!strncmp(device, "unicorn", strlen("unicorn"))) {
                    platform_device = PLATFORM_L2S;
	    } else if (!strncmp(device, "zizhan", strlen("zizhan"))){
                    platform_device = PLATFORM_L18;
	    } else if (!strncmp(device, "diting", strlen("diting"))){
		    platform_device = PLATFORM_L12;
	    } else {
		    platform_device = 0;
		    max_brightness = 0;
	    }
    }
    if (platform_device == PLATFORM_K11) {
            usleep(2000  * 1000);
    }

    if(platform_device == PLATFORM_J18 || platform_device == PLATFORM_L18)
        Dual_Screen = true;
    else
        Dual_Screen = false;
     /* charger run too quick and can not obtain usb online status
       wait at most 5 sec to check usb online status in chargeonlymode
       avoid repeatly reboot issue in chargeonlymode.
    */
    while (check_count < 5) {
          charger_present = read_input_online(CHARGER_PRESENT_PATH);
          wireless_present = read_input_online(WIRELESS_PRESENT_PATH);
          dc_present = read_input_online(WIRELESS_PRESENT_PATH_OLD_PLATFORM);
          if (charger_present == 0 && wireless_present == 0 && dc_present == 0) {
              usleep(1000 * 1000);
              check_count++;
          } else {
              check_count = 5;
              break;
          }
          LOGE("charger_present=%d, check_count=%d\n",
               charger_present, check_count);
    }
    LOGW("--------------- STARTING CHARGER MODE ---------------\n");

    ret = ev_init(
            std::bind(&Charger::InputCallback, this, std::placeholders::_1, std::placeholders::_2));
    if (!ret) {
        epollfd = ev_get_epollfd();
        configuration_->ChargerRegisterEvent(epollfd, &charger_event_handler, EVENT_WAKEUP_FD);
    }

    InitAnimation();
    res_battery_init();

    ret = CreateDisplaySurface(batt_anim_.fail_file, &surf_unknown_);
    if (ret < 0) {
#if !defined(__ANDROID_VNDK__)
        LOGE("Cannot load custom battery_fail image. Reverting to built in: %d\n", ret);
        ret = CreateDisplaySurface((system_animation_root + "charger/battery_fail.png"s).c_str(),
                                   &surf_unknown_);
#endif
        if (ret < 0) {
            LOGE("Cannot load built in battery_fail image\n");
            surf_unknown_ = NULL;
        }
    }

    GRSurface** scale_frames;
    int scale_count;
    int scale_fps;  // Not in use (charger/battery_scale doesn't have FPS text
                    // chunk). We are using hard-coded frame.disp_time instead.
    ret = CreateMultiDisplaySurface(batt_anim_.animation_file, &scale_count, &scale_fps,
                                    &scale_frames);
    if (ret < 0) {
        LOGE("Cannot load battery_scale image\n");
        batt_anim_.num_frames = 0;
        batt_anim_.num_cycles = 1;
    } else if (scale_count != batt_anim_.num_frames) {
        LOGE("battery_scale image has unexpected frame count (%d, expected %d)\n", scale_count,
             batt_anim_.num_frames);
        batt_anim_.num_frames = 0;
        batt_anim_.num_cycles = 1;
    } else {
        for (i = 0; i < batt_anim_.num_frames; i++) {
            batt_anim_.frames[i].surface = scale_frames[i];
        }
    }
    drm_ = DRM_INNER;
    screen_switch_ = SCREEN_SWITCH_DEFAULT;
    ev_sync_key_state(std::bind(&Charger::SetKeyCallback, this, std::placeholders::_1,
                                std::placeholders::_2));

    (void)ev_sync_sw_state(
            std::bind(&Charger::SetSwCallback, this, std::placeholders::_1, std::placeholders::_2));

    next_screen_transition_ = -1;
    next_key_check_ = -1;
    next_pwr_check_ = -1;
    wait_batt_level_timestamp_ = 0;

    // Retrieve healthd_config from the existing health HAL.
    configuration_->ChargerInitConfig(config);

    boot_min_cap_ = config->boot_min_cap;
    init_time = curr_time_ms();
    // gr_init_mi for dual panel such as cetus
    if (Dual_Screen == true) {
         gr_init_mi();
         LOGW("gr_init_mi\n");
         screen_width = gr_fb_width_mi(DISP_PRIMARY);
         screen_height = gr_fb_height_mi(DISP_PRIMARY);
         screen_width_second = gr_fb_width_mi(DISP_SECONDARY);
         screen_height_second = gr_fb_height_mi(DISP_SECONDARY);
         LOGW("DISP_PRIMARY:screen_width =%d,screen_height =%d,DISP_SECONDARY:screen_width_second = %d,screen_height_second = %d\n", screen_width, screen_height,screen_width_second,screen_height_second);
    }
}

int Charger::CreateDisplaySurface(const std::string& name, GRSurface** surface) {
    return res_create_display_surface(name.c_str(), surface);
}

int Charger::CreateMultiDisplaySurface(const std::string& name, int* frames, int* fps,
                                       GRSurface*** surface) {
    return res_create_multi_display_surface(name.c_str(), frames, fps, surface);
}

void set_resource_root_for(const std::string& root, const std::string& backup_root,
                           std::string* value) {
    if (value->empty()) {
        return;
    }

    std::string new_value = root + *value + ".png";
    // If |backup_root| is provided, additionally check whether the file under |root| is
    // accessible or not. If not accessible, fallback to file under |backup_root|.
    if (!backup_root.empty() && access(new_value.data(), F_OK) == -1) {
        new_value = backup_root + *value + ".png";
    }

    *value = new_value;
}

void animation::set_resource_root(const std::string& root, const std::string& backup_root) {
    CHECK(android::base::StartsWith(root, "/") && android::base::EndsWith(root, "/"))
            << "animation root " << root << " must start and end with /";
    CHECK(backup_root.empty() || (android::base::StartsWith(backup_root, "/") &&
                                  android::base::EndsWith(backup_root, "/")))
            << "animation backup root " << backup_root << " must start and end with /";
    set_resource_root_for(root, backup_root, &animation_file);
    set_resource_root_for(root, backup_root, &fail_file);
    set_resource_root_for(root, backup_root, &text_clock.font_file);
    set_resource_root_for(root, backup_root, &text_percent.font_file);
}

}  // namespace android
