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

#ifndef DISPLAYFEATURE_GET_H
#define DISPLAYFEATURE_GET_H

#include <utils/StrongPointer.h>
#include <utils/Mutex.h>

#include <vendor/xiaomi/hardware/displayfeature/1.0/IDisplayFeature.h>
#include <vendor/xiaomi/hardware/displayfeature/1.0/types.h>
#include <vendor/xiaomi/hardware/displayfeature/1.0/IDisplayFeatureCallback.h>
using vendor::xiaomi::hardware::displayfeature::V1_0::FeatureId;
using vendor::xiaomi::hardware::displayfeature::V1_0::BACKDOOR_CODE;
using vendor::xiaomi::hardware::displayfeature::V1_0::IDisplayFeature;
using ::vendor::xiaomi::hardware::displayfeature::V1_0::IDisplayFeatureCallback;
using ::android::hardware::Return;
using ::android::hardware::Void;
using android::hidl::base::V1_0::IBase;
using android::hardware::hidl_death_recipient;

#define DISPLAYFEATURE_HAL_STATUS_PROP "init.svc.vendor.displayfeature-hal-1-0"
#define DISPLAYFEATURE_DUMP_ENABLE_PROP "ro.vendor.displayfeature.dump"

namespace android {

class DisplayFeatureGet : public hidl_death_recipient {
public:
    DisplayFeatureGet() { }
    virtual ~DisplayFeatureGet() { }

    virtual void serviceDied(uint64_t cookie, const wp<IBase>& who);
    const sp<IDisplayFeature> getDisplayFeatureService();
    void setDisplayFeatureGameMode(int gameColorMode);
    bool setFeature(int32_t display, int32_t cmd,
                    int32_t param, int32_t cookie);
    void registerCallback(int32_t display, const sp<IDisplayFeatureCallback>& callback);
    void Dump(std::string& result);
    int32_t setFODFlag(bool success);
    int32_t setFingerprintState(int fp_status);

     enum DISPPARAM_MODE {
        DISPPARAM_WARM = 0x1,
        DISPPARAM_DEFAULT = 0x2,
        DISPPARAM_COLD = 0x3,
        DISPPARAM_PAPERMODE8 = 0x5,
        DISPPARAM_PAPERMODE1 = 0x6,
        DISPPARAM_PAPERMODE2 = 0x7,
        DISPPARAM_PAPERMODE3 = 0x8,
        DISPPARAM_PAPERMODE4 = 0x9,
        DISPPARAM_PAPERMODE5 = 0xA,
        DISPPARAM_PAPERMODE6 = 0xB,
        DISPPARAM_PAPERMODE7 = 0xC,
        DISPPARAM_WHITEPOINT_XY = 0xE,
        DISPPARAM_CE_ON = 0x10,
        DISPPARAM_CE_OFF = 0xF0,
        DISPPARAM_CABCUI_ON = 0x100,
        DISPPARAM_CABCSTILL_ON = 0x200,
        DISPPARAM_CABCMOVIE_ON = 0x300,
        DISPPARAM_CABC_OFF = 0x400,
        DISPPARAM_SKIN_CE_CABCUI_ON = 0x500,
        DISPPARAM_SKIN_CE_CABCSTILL_ON = 0x600,
        DISPPARAM_SKIN_CE_CABCMOVIE_ON = 0x700,
        DISPPARAM_SKIN_CE_CABC_OFF = 0x800,
        DISPPARAM_DIMMING_OFF = 0xE00,
        DISPPARAM_DIMMING = 0xF00,
        DISPPARAM_ACL_L1 = 0x1000,
        DISPPARAM_ACL_L2 = 0x2000,
        DISPPARAM_ACL_L3 = 0x3000,
        DISPPARAM_ACL_OFF = 0xF000,
        DISPPARAM_FP_STATUS = 0xE000,
        DISPPARAM_HBM_ON = 0x10000,
        DISPPARAM_HBM_FOD_ON = 0x20000,
        DISPPARAM_HBM_FOD2NORM = 0x30000,
        DISPPARAM_DC_ON = 0x40000,
        DISPPARAM_DC_OFF = 0x50000,
        DISPPARAM_UNLOCK_SUCCESS = 0x70000,
        DISPPARAM_UNLOCK_FAIL = 0x80000,
        DISPPARAM_HBM_FOD_OFF = 0xE0000,
        DISPPARAM_HBM_OFF = 0xF0000,
        DISPPARAM_LCD_HBM_L1_ON = 0xB0000,
        DISPPARAM_LCD_HBM_L2_ON = 0xC0000,
        DISPPARAM_LCD_HBM_L3_ON = 0xD0000,
        DISPPARAM_LCD_HBM_OFF = 0xE0000,
        DISPPARAM_NORMALMODE1 = 0x100000,
        DISPPARAM_P3 = 0x200000,
        DISPPARAM_SRGB = 0x300000,
        DISPPARAM_SKIN_CE = 0x400000,
        DISPPARAM_SKIN_CE_OFF = 0x500000,
        DISPPARAM_DOZE_BRIGHTNESS_HBM = 0x600000,
        DISPPARAM_DOZE_BRIGHTNESS_LBM = 0x700000,
        DISPPARAM_HBM_BACKLIGHT_RESEND = 0xA00000,
        DISPPARAM_HBM_FOD_ON_FLAG = 0xB00000,
        DISPPARAM_HBM_FOD_OFF_FLAG = 0xC00000,
        DISPPARAM_FOD_BACKLIGHT = 0xD00000,
        DISPPARAM_CRC_OFF = 0xF00000,
        DISPPARAM_FOD_BACKLIGHT_ON = 0x1000000,
        DISPPARAM_FOD_BACKLIGHT_OFF = 0x2000000,
    };

    static const int AOD_HIGH_BRIGHTNESS = 205;
    static const int AOD_LOW_BRIGHTNESS = 21;

private:

    int32_t setDisplayParam(int32_t param);
    static constexpr const char* kDISP_PARAM_PATH = "/sys/class/drm/card0-DSI-1/disp_param";
    static constexpr const char* kDISP_DIM_LAYER_ALPHA_PATH = "/sys/class/drm/card0-DSI-1/dim_alpha";
    static const int STRING_LEN = 255;

    Mutex mServiceLock;
    sp<IDisplayFeature> mDisplayFeatureService;
};

}

#endif
