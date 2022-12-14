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
#include <time.h>

#include <unistd.h>
#include "checkfs.h"
#include <android-base/properties.h>

#define MMCBLK0_DIR "/sys/class/block/mmcblk0"
#define MMCBLK0_DEVICE_DIR "/sys/class/block/mmcblk0/device"

static void print_contents(const char *dir, const char *filename)
{
    FILE *f = NULL;
    char path[256] = { 0 };

    snprintf(path, sizeof (path), "%s/%s", dir, filename);
    f = fopen(path, "r");
    if (f == NULL) {
        printf("failed to open '%s': %s\n", path, strerror(errno));
        return;
    }

    char buf[1024] = { 0 };
    ssize_t bytes_read = fread(buf, 1, sizeof (buf), f);
    if (bytes_read > 0) {
        printf("%s: %s", filename, buf);
    }

    fclose(f);
}

static void print_emmc_info(void)
{
    printf("============= emmc ===============\n");
    print_contents(MMCBLK0_DEVICE_DIR, "name");
    print_contents(MMCBLK0_DEVICE_DIR, "manfid");
    print_contents(MMCBLK0_DEVICE_DIR, "date");
    print_contents(MMCBLK0_DEVICE_DIR, "cid");
    print_contents(MMCBLK0_DEVICE_DIR, "csd");
    print_contents(MMCBLK0_DEVICE_DIR, "oemid");
    print_contents(MMCBLK0_DEVICE_DIR, "erase_size");
    print_contents(MMCBLK0_DEVICE_DIR, "preferred_erase_size");
    print_contents(MMCBLK0_DIR, "stat");
    printf("\n");
}

static const char *s_prop_name[] = {
    "ro.build.fingerprint",
    "ro.build.version.release",
    "ro.build.version.incremental",
    "ro.build.product",
    "ro.product.device",
    "ro.product.mod_device",
    "ro.miui.ui.version.name",
    NULL,
};

static void print_prop_info(void)
{
    printf("============= prop ===============\n");
    int i;
    std::string value;
    for (i = 0; s_prop_name[i] != NULL; ++i) {
        //property_get(s_prop_name[i], value, "unknown");
        value = android::base::GetProperty(s_prop_name[i], "unknown");
        printf("%s=%s\n", s_prop_name[i], value.c_str());
    }
    printf("\n");
}

void print_system_filesystem_info(void)
{
    int i;
    const char *system[] = {
        // qcom
        "/dev/block/platform/msm_sdcc.1/by-name/system",
        "/dev/block/platform/msm_sdcc.1/by-name/system1",
        "/dev/block/bootdevice/by-name/system",
        "/dev/block/platform/bootdevice/by-name/system",
        // mtk
        "/dev/block/platform/mtk-msdc.0/by-name/system",
        NULL,
    };

    for (i = 0; system[i] != NULL; ++i) {
        if (access(system[i], F_OK)) {
            continue;
        }
        dump_ext4fs_linux(system[i]);
    }
}

void print_device_info(const char *package, time_t start)
{
    printf("Updating package '%s' ...\n", package);
    printf("Starting updater on %s", ctime(&start));
    print_emmc_info();
    print_prop_info();
}

#define OTAD_LOG_FILE "/cache/otad/otad.log"
void cleanup(int updater_result)
{
    /* 1. Print the system filesystem informations. */
    print_system_filesystem_info();

    /* 2. We must unlink otad.log after updater successully.
     * if we don't do that, will be cause next ota updater used to
     * full ota package. otad.log includes all of items about modify
     * files under "/system" directory.
     */
    if (updater_result == 0 && unlink(OTAD_LOG_FILE)) {
        printf("Failed to unlink '%s'\n", OTAD_LOG_FILE);
    }
}
