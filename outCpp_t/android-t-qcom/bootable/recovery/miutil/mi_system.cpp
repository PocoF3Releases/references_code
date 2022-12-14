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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <unistd.h>

#include <fs_mgr.h>
#include <android-base/logging.h>
#include <android-base/file.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#include "recovery_utils/roots.h"

#include "miutil/mi_system.h"
#include "cutils/properties.h"

#define MMC_SECTOR_SIZE 512
#define DUALBOOT_OFFSET_SECTORS 8

static constexpr const char* BRIGHTNESS_FILE = "/sys/class/leds/lcd-backlight/brightness";
static constexpr const char* MAX_BRIGHTNESS_FILE = "/sys/class/leds/lcd-backlight/max_brightness";
static constexpr const char* NEW_BRIGHTNESS_FILE = "/sys/class/backlight/panel0-backlight/brightness";
static constexpr const char* NEW_MAX_BRIGHTNESS_FILE = "/sys/class/backlight/panel0-backlight/max_brightness";

static const int oled_light = 40;
static const int lcd_light = 80;

/* Dualboot message
 * The new misc partition map is below
 * |--------------------|
 * | bootloader_message | 8 sectors, 4KB
 * |--------------------|
 * | dual_boot_message  |
 * |--------------------|
 * The command field should be boot-system1, when we want to
 * boot the second system. On any other value, LK will boot
 * into the first system.
 */
struct dual_boot_message {
    char command[32];
};

static int get_current_system_id_block(const Volume* v);
static int set_reboot_message_block(int system_id, const Volume* v);

/* We have preference on the 1st system */
int get_current_system_id(void)
{
    Volume* v = volume_for_mount_point("/misc");
    if (v == NULL) {
      LOG(ERROR) << "Cannot load volume /misc!";
      return 0;
    }
    if (strcmp(v->fs_type.c_str(), "mtd") == 0) {
        LOG(ERROR) << "Get reboot message on mtd not supported!";
        return 0;
    } else if (strcmp(v->fs_type.c_str(), "emmc") == 0) {
        return get_current_system_id_block(v);
    }
    LOG(ERROR) << "unknown misc partition fs_type:" << v->fs_type;
    return 0;
}

int set_reboot_message(int system_id)
{
    Volume* v = volume_for_mount_point("/misc");
    if (v == NULL) {
      LOG(ERROR) << "Cannot load volume /misc!";
      return -1;
    }
    if (strcmp(v->fs_type.c_str(), "mtd") == 0) {
        LOG(ERROR) << "Set reboot message on mtd not supported!";
        return -1;
    } else if (strcmp(v->fs_type.c_str(), "emmc") == 0) {
        return set_reboot_message_block(system_id, v);
    }
    LOG(ERROR) << "unknown misc partition fs_type " << v->fs_type;
    return -1;
}

void set_screen_backlight(bool on)
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
      LOG(WARNING) << "Failed to read max brightness";
      return;
    }

    unsigned int max_value;
    if (!android::base::ParseUint(android::base::Trim(content), &max_value)) {
      LOG(WARNING) << "Failed to parse max brightness: " << content;
      return;
    }

    int scale = 1;
    if (max_value > 256) {
        scale = (max_value+1)/256;
    }
    int screen_light = lcd_light * scale;
    char screen_type[PROPERTY_VALUE_MAX] = { 0 };
    property_get("ro.display.type", screen_type, NULL);
    if (strncmp(screen_type, "oled", 4) == 0) {    // decrease oled screen light in order to extend its life
        screen_light = oled_light * scale;
    } else {
        property_get("ro.vendor.display.type", screen_type, NULL);
        if (strncmp(screen_type, "oled", 4) == 0) {    // decrease oled screen light in order to extend its life
            screen_light = oled_light * scale;
        }
    }
    LOG(INFO) << "the light in recovery now is " << screen_light;
    if (!android::base::WriteStringToFile(std::to_string(on ? screen_light : 0), brightness_file)) {
        LOG(WARNING) <<  "Failed to set brightness";
    }
}

/*================================================================*/

static int get_current_system_id_block(const Volume* v)
{
    char buf[32];
    int system_id = 0;
    int rc = 0;
    int count = 0;

    FILE* f = fopen(v->blk_device.c_str(), "rb");
    if (f == NULL) {
        LOG(ERROR) << "Can't open " << v->blk_device << ": " << strerror(errno);
        return system_id;
    }

    rc = fseek(f, MMC_SECTOR_SIZE * DUALBOOT_OFFSET_SECTORS, SEEK_SET);
    if (rc) {
        LOG(ERROR) << "Failed seeking " << v->blk_device << ": " << strerror(errno);
        goto out;
    }
    memset(buf, 0, sizeof(buf));
    count = fread(buf, sizeof(buf), 1, f);
    if (count != 1) {
        LOG(ERROR) << "Failed read " << v->blk_device << ": " << strerror(errno);
        goto out;
    }

    if (strcmp(buf, "boot-system1") == 0) {
        system_id = 1;
    }

out:
    if (fclose(f) != 0) {
        LOG(ERROR) << "Failed closing " << v->blk_device << ": " << strerror(errno);
    }
    return system_id;
}

static int set_reboot_message_block(int system_id, const Volume* v)
{
    char buf[32];
    FILE* f = fopen(v->blk_device.c_str(), "wb");
    if (f == NULL) {
        LOG(ERROR) << "Can't open " << v->blk_device << ": " << strerror(errno);
        return -1;
    }
    int rc = fseek(f, MMC_SECTOR_SIZE * DUALBOOT_OFFSET_SECTORS, SEEK_SET);
    if (rc) {
        LOG(ERROR) << "Failed seeking " << v->blk_device << ": " << strerror(errno);
        fclose(f);
        return -1;
    }
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "boot-system%d", system_id ? 1 : 0);
    int count = fwrite(buf, sizeof(buf), 1, f);
    if (count != 1) {
        LOG(ERROR) << "Failed writing " << v->blk_device << ": " << strerror(errno);
        fclose(f);
        return -1;
    }
    if (fclose(f) != 0) {
        LOG(ERROR) << "Failed closing " << v->blk_device << ": " << strerror(errno);
        return -1;
    }
    return 0;
}
