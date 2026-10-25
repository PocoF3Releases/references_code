/*
 * Copyright (C) 2013 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License") = 0;
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

#ifndef _IDISPLAY_FEATURE_CONTROL_H_
#define _IDISPLAY_FEATURE_CONTROL_H_

#include <binder/IInterface.h>
#include <binder/Parcel.h>


namespace android {

enum DISPLAY_CABC_MODE {
    DISPALY_CABC_OFF = 0,
    DISPALY_CABC_MODE_DEFAULT = 1,
    DISPALY_CABC_MODE_GUI = 2,
    DISPALY_CABC_MODE_STILL = 3,
    DISPALY_CABC_MODE_MOVIE = 4,
};

class IDisplayFeatureControl : public IInterface
{
public:
    enum {
        SET_AD = IBinder::FIRST_CALL_TRANSACTION,
        SET_PANEL_MODE = IBinder::FIRST_CALL_TRANSACTION+1,
        SET_QUALCOMM_MODE = IBinder::FIRST_CALL_TRANSACTION+2,
        SET_COLOR_TEMPERATURE = IBinder::FIRST_CALL_TRANSACTION+3,
        SET_SVI = IBinder::FIRST_CALL_TRANSACTION+4,
        SET_PCC_MODE = IBinder::FIRST_CALL_TRANSACTION+5,
        SET_QC_DPPS_MODE = IBinder::FIRST_CALL_TRANSACTION+6,
        SET_MTK_MODE = IBinder::FIRST_CALL_TRANSACTION+7,
        SET_LTM = IBinder::FIRST_CALL_TRANSACTION+8,
        NOTIFY = IBinder::FIRST_CALL_TRANSACTION+97,
        SET_OPTION = IBinder::FIRST_CALL_TRANSACTION+98,
        SET_FEATURE_COMMOM = IBinder::FIRST_CALL_TRANSACTION+99,
    };

    enum {
        AUTOMATIC_CONTRAST_STATE,
        INCREASED_CONTRAST_STATE,
        STANDARD_STATE,
        EYECARE_STATE,
        COLORTEMPERATURE_ADJUST_STATE,
        DIMMING_BRIGHTNESS_STATE,
        KEEP_WHITE_POINT_SRGB_STATE,
        CABC_MODE_CUSTOM_STATE,
        AD_STATE,
        NIGHT_STATE,
        HDR_STATE,
        HBM_STATE,
        SUNLIGHT_SCREEN_STATE,
        NOTIFY_DISPLAY_STATE,
        CAMERA_STATE,
        EXT_COLOR_PROC_STATE,
        CHANGE_COLOR_MODE_STATE,
        FOD_SCREEN_ON_STATE,
        BCBC_CUST_STATE,
        GAME_STATE,
        DC_BACKLIGHT_STATE,
        DFPS_STATE,
        FOD_MONITOR_ON_STATE,
        COLOR_TEMP_STATE,
        FPS_SWITCH_STATE,
        DOZE_BRIGHTNESS_STATE,
        EXPERT_STATE,
        VIDEO_STATE,
        ACTIVE_APP_STATE,
        SMART_DFPS_STATE,
        DUMP_STATE,
        PAPER_MODE_STATE,
        TRUE_TONE_STATE,
        NOTIFY_10BIT_STATE,
        FRAME_REFRESH_STATE,
        SRE_STATE,
        DISPLAY_SCENE_STATE,
        FOD_LOWBRIGHTNESS_FOD_STATE,
        DARK_MODE_STATE,
        COLOR_MODE,
        FOSS_STATE,
        DITHER_STATE,
        LIGHT_BIT_SWITCH_STATE,
        ADAPTIVE_HDR,
        DOLBY_VISION_STATE = 44,
        TOUCH_STATE = 45,
        ORITATION_STATE = 46,
        CUP_BLACK_COVERED_STATE = 47,
        PICTURE_HDR_STATE = 51,
    };

    enum {
        AUTOMATIC_CONTRAST_WARM = 1,
        AUTOMATIC_CONTRAST_NORMAL = 2,
        AUTOMATIC_CONTRAST_COLD = 3,

    };

    DECLARE_META_INTERFACE(DisplayFeatureControl);

    // setFeatureEnable function set display effect,
    //   the function need dispatch each case to specific impl function in derived class;
    // @param  displayId = 0.main display
    //         caseId  ex.AUTOMATIC_CONTRAST_STATE
    //         modeId  ex.AUTOMATIC_CONTRAST_NORMAL,EyeCare Index.
    //         cookie  ex. other values.
    // @return 0 if the setFeatureEnable was made successfully.
    virtual int setFeatureEnable(int displayId, int caseId, int modeId, int cookie) = 0;
    virtual int setFunctionEnable(int displayId, int caseId, int modeId, int cookie) = 0;
    virtual void notify(int displayId, int caseId, int modeId, int cookie) = 0;
};

// ----------------------------------------------------------------------------

class BnDisplayFeatureControl : public BnInterface<IDisplayFeatureControl>
{
public:
    virtual status_t onTransact(uint32_t code,
                                const Parcel& data,
                                Parcel* reply,
                                uint32_t flags = 0);
};

};

#endif
