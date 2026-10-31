#define LOG_TAG "octvm_drv_xm"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "octvm.h"
#include "drv/platform_power.h"
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <vendor/xiaomi/hardware/displayfeature/1.0/IDisplayFeature.h>
#include <vendor/xiaomi/hardware/displayfeature/1.0/types.h>

using namespace android;
using ::vendor::xiaomi::hardware::displayfeature::V1_0::IDisplayFeature;
using ::vendor::xiaomi::hardware::displayfeature::V1_0::DISPLAY_CABC_MODE;
using ::vendor::xiaomi::hardware::displayfeature::V1_0::FeatureId;
using ::vendor::xiaomi::hardware::displayfeature::V1_0::Status;
using ::android::hardware::Return;

#ifdef DISPLAY_FEATURE_BCBC_SUPPORT
using ::vendor::xiaomi::hardware::displayfeature::V1_0::DISPLAY_BCBC_MODE;
#endif

#ifdef DISPLAY_FEATURE_DFPS_SUPPORT
using ::vendor::xiaomi::hardware::displayfeature::V1_0::DISPLAY_DFPS_MODE;
#endif

static int mode_to_platform_code(const char *lcd_mode)
{
    int platform_code;
    if (strcmp(lcd_mode, "en") == 0) {
        platform_code = static_cast<int>(DISPLAY_CABC_MODE::DISPALY_CABC_OFF);
    } else if (strcmp(lcd_mode, "ui") == 0) {
        platform_code = static_cast<int>(DISPLAY_CABC_MODE::DISPALY_CABC_MODE_GUI);
    } else if (strcmp(lcd_mode, "still") == 0) {
        platform_code = static_cast<int>(DISPLAY_CABC_MODE::DISPALY_CABC_MODE_STILL);
    } else if (strcmp(lcd_mode, "movie") == 0) {
        platform_code = static_cast<int>(DISPLAY_CABC_MODE::DISPALY_CABC_MODE_MOVIE);
    } else {
        platform_code = static_cast<int>(DISPLAY_CABC_MODE::DISPALY_CABC_MODE_DEFAULT);
    }
    return platform_code;
}

int32_t platform_set_lcd_mode(const char *lcd_mode)
{
    int mode_code = mode_to_platform_code(lcd_mode);
    sp<IDisplayFeature> mClient = IDisplayFeature::getService();
    if(mClient == NULL) {
        fprintf(stderr, "platform_set_lcd_mode IDisplayFeature getService failed");
        return 0;
    }

    Return<Status> ret = mClient->setFeature(0,static_cast<int>(FeatureId::CABC_MODE_CUSTOM_STATE), mode_code, 0);
    printf("platform_set_lcd_mode %s(%d) %s\n", lcd_mode, mode_code, ret.isOk() ? "success" : "failed");
    return 0;
}

#ifdef DISPLAY_FEATURE_BCBC_SUPPORT
int32_t platform_set_bcbc_cust_state(const char * cust_state) {
    sp<IDisplayFeature> mClient = IDisplayFeature::getService();
    if(mClient == NULL) {
        fprintf(stderr, "platform_set_bcbc_cust_state IDisplayFeature getService failed");
        return -1;
    }

    if (strcmp(cust_state, "enable") == 0) {
        Return<Status> ret = mClient->setFeature(0,static_cast<int>(FeatureId::BCBC_CUST_STATE)
                , static_cast<int>(DISPLAY_BCBC_MODE::DISPALY_BCBC_ON), 0);
        printf("platform_set_bcbc_cust_state enable %s\n", ret.isOk() ? "success" : "failed");
    } else {
        Return<Status> ret = mClient->setFeature(0,static_cast<int>(FeatureId::BCBC_CUST_STATE)
                , static_cast<int>(DISPLAY_BCBC_MODE::DISPALY_BCBC_OFF), 0);
        printf("platform_set_bcbc_cust_state disable %s\n", ret.isOk() ? "success" : "failed");
    }
    return 0;
}
#endif

#ifdef DISPLAY_FEATURE_DFPS_SUPPORT
int32_t platform_set_dfps_cust_state(const char * cust_state) {
    sp<IDisplayFeature> mClient = IDisplayFeature::getService();
    if(mClient == NULL) {
        fprintf(stderr, "platform_set_dfps_cust_state IDisplayFeature getService failed");
        return -1;
    }

    if (strcmp(cust_state, "enable") == 0) {
        Return<Status> ret = mClient->setFeature(0, static_cast<int>(FeatureId::DFPS_STATE)
                , static_cast<int>(DISPLAY_DFPS_MODE::DISPALY_DFPS_ON), 0);
        printf("platform_set_dfps_cust_state enable %s\n", ret.isOk() ? "success" : "failed");
    } else {
        Return<Status> ret = mClient->setFeature(0, static_cast<int>(FeatureId::DFPS_STATE)
                , static_cast<int>(DISPLAY_DFPS_MODE::DISPALY_DFPS_OFF), 0);
        printf("platform_set_dfps_cust_state disable %s\n", ret.isOk() ? "success" : "failed");
    }
    return 0;
}
#endif
