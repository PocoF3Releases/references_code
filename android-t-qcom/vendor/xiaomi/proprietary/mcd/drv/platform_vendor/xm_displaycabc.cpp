#define LOG_TAG "octvm_drv_xm"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include "IDisplayFeatureControl.h"

#include "octvm.h"
#include "drv/platform_power.h"

using namespace android;

static int mode_to_platform_code(const char *lcd_mode)
{
    int platform_code;
    if (strcmp(lcd_mode, "en") == 0) {
        platform_code = DISPALY_CABC_OFF;
    } else if (strcmp(lcd_mode, "ui") == 0) {
        platform_code = DISPALY_CABC_MODE_GUI;
    } else if (strcmp(lcd_mode, "still") == 0) {
        platform_code = DISPALY_CABC_MODE_STILL;
    } else if (strcmp(lcd_mode, "movie") == 0) {
        platform_code = DISPALY_CABC_MODE_MOVIE;
    } else {
        platform_code = DISPALY_CABC_MODE_DEFAULT;
    }
    return platform_code;
}

int32_t platform_set_lcd_mode(const char *lcd_mode)
{
    int mode_code = mode_to_platform_code(lcd_mode);
    sp<IBinder> binder = defaultServiceManager()->getService(String16("DisplayFeatureControl"));
    if (binder == NULL) {
        fprintf(stderr, "can not get the service of DisplayFeatureControl!");
        return -1;
    }
    printf("platform_set_lcd_mode: mode %s(%d)\n", lcd_mode, mode_code);
    const sp<IDisplayFeatureControl>& displayfeatureservice = interface_cast<IDisplayFeatureControl>(binder);
    return displayfeatureservice->setFeatureEnable(0, IDisplayFeatureControl::CABC_MODE_CUSTOM_STATE, mode_code, 0);
}
