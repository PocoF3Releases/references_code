#define LOG_TAG "octvm_drv"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>

#include <cutils/properties.h>
#include "display_config.h"

#include "octvm.h"
#include "drv/platform_power.h"

#define PANEL_INFO  "/sys/class/graphics/fb0/msm_fb_panel_info"

using namespace qdutils;

static bool s_dfps_enable;
static uint32_t s_dfps_rate;

void init_device_dfps_state(struct dfps_info_t **dfps_info) {
    s_dfps_enable = false;
    s_dfps_rate = 0;

    bool dyn_fps_support = false;
    uint32_t min_fps = 0;
    uint32_t max_fps = 0;
    FILE *fp = fopen(PANEL_INFO, "r");
    if (fp) {
        char *line = NULL;
        size_t len = 0;
        ssize_t read_len;
        while ((read_len = getline(&line, &len, fp)) != -1) {
            if (line[read_len - 1] == '\n') line[read_len -1] = '\0';
            char *name;
            char *value;
            if (strstr(line, "dyn_fps_en") != NULL) {
                name = strtok(line, "=");
                value = strtok(NULL, "=");
                dyn_fps_support = atoi(value);
            } else if (strstr(line, "min_fps") != NULL) {
                name = strtok(line, "=");
                value = strtok(NULL, "=");
                min_fps = atoi(value);
            } else if (strstr(line, "max_fps") != NULL) {
                name = strtok(line, "=");
                value = strtok(NULL, "=");
                max_fps = atoi(value);
            }
        }
    }
    printf("dyn_fps_support = %d, min_fps = %d, max_fps = %d",
            dyn_fps_support, min_fps, max_fps);
    if (dyn_fps_support) {
        *dfps_info = (struct dfps_info_t *)malloc(sizeof(struct dfps_info_t));
        (*dfps_info)->min_fps = min_fps;
        (*dfps_info)->max_fps = max_fps;
    }
}

void configure_dfps_enable(bool enable) {
    if (enable != s_dfps_enable) {
        if (enable) {
            printf("binder call to qdutils::configureDynRefreshRate disable");
            configureDynRefreshRate(qdutils::DISABLE_METADATA_DYN_REFRESH_RATE, 0);
            s_dfps_rate = 0;
        } else {
            printf("binder call to qdutils::configureDynRefreshRate enable");
            qdutils::configureDynRefreshRate(qdutils::ENABLE_METADATA_DYN_REFRESH_RATE, 0);
        }
        s_dfps_enable = enable;
    }
}

void configure_dfps_rate(uint32_t rate) {
    if (!s_dfps_enable) {
        return;
    }
    if (rate != s_dfps_rate) {
        printf("binder call to qdutils::configureDynRefreshRate");
        configureDynRefreshRate(qdutils::SET_BINDER_DYN_REFRESH_RATE, rate);
        s_dfps_rate = rate;
    }
}

