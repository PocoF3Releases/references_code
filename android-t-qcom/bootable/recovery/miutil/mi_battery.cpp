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

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <android-base/logging.h>
#include <android-base/properties.h>

#include "dirent.h"
#include "miutil/mi_battery.h"
#include <vector>

#define BATTERY_STATUS_PATH "/sys/class/power_supply/battery/status"
#define BATTERY_CAPACITY_PATH "/sys/class/power_supply/battery/capacity"
#define USB_ONLINE_PATH "/sys/class/android_usb/android0/state"
static bool IS_NON_AB = android::base::GetProperty("ro.boot.slot_suffix", "").empty();

static int read_from_file(const char* path, char* buf, size_t size)
{
    int fd = open(path, O_RDONLY, 0);
    if (fd == -1) {
        LOG(ERROR) << "Could not open:" << path;
        return -1;
    }

    size_t count = read(fd, buf, size);
    if (count > 0) {
        count = (count < size) ? count : size - 1;
        while (count > 0 && buf[count-1] == '\n') count--;
        buf[count] = '\0';
    } else {
        buf[0] = '\0';
    }

    close(fd);
    return count;
}

static int read_int(const char* path)
{
    const int SIZE = 128;
    char buf[SIZE];
    int value = 0;

    if (read_from_file(path, buf, SIZE) > 0) {
        value = atoi(buf);
    }
    return value;
}

// Return true if USB is connected.
static bool read_usb_state() {
    int fd = open(USB_ONLINE_PATH, O_RDONLY);
    if (fd < 0) {
        printf("failed to open /sys/class/android_usb/android0/state: %s\n",
         strerror(errno));
        return 0;
    }

    char buf[128];
    memset(&buf, 0, sizeof(buf));
    /* USB is connected if android_usb state is CONFIGURED */
    int connected = (read(fd, &buf, 12) >= 1) && (strstr(buf, "CONFIGURED"));
    if (close(fd) < 0) {
        printf("failed to close /sys/class/android_usb/android0/state: %s\n",
         strerror(errno));
    }
    return connected;
}

static int is_battery_charging(void)
{
    const int SIZE = 128;
    char buf[SIZE];

    int value = 0;
    if(!IS_NON_AB && access(BATTERY_STATUS_PATH, R_OK)) {
       return 1;
    }
    if (read_from_file(BATTERY_STATUS_PATH, buf, SIZE) > 0) {
        if (buf[0] == 'C') {
            value = 1;
        }
    }
    return value;
}

struct battery_state read_battery_state(void)
{
    struct battery_state state;
    state.is_charging = is_battery_charging();
    state.online = read_usb_state();

    if(!IS_NON_AB && access(BATTERY_CAPACITY_PATH, R_OK)) {
      state.level = 100;
      return state;
    }
    state.level = read_int(BATTERY_CAPACITY_PATH);
    return state;
}
