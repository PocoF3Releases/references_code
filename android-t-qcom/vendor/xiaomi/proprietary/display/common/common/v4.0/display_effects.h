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

#ifndef _DISPLAY_EFFECTS_H_
#define _DISPLAY_EFFECTS_H_

enum {
    SET_AD = 1,
    SET_PANEL_MODE,
    SET_COLOR_MODE,
    SET_COLOR_TEMPERATURE,
    SET_SVI,
    SET_PCC_MODE,
    SET_FOSS,
    SET_DSPP_MODE,
    SET_OPTION,//function's log and switch dynamically
    SET_MTK_MODE,
    SET_LTM,
    SET_BUF_DUMP,
    SET_QDCM_MODE,
    SET_MAX_FEATURE_CNT,
};

enum {
    AUTOMATIC_CONTRAST_STATE,
    INCREASED_CONTRAST_STATE,
    STANDARD_STATE,
    EYECARE_STATE,
    COLORTEMPERATURE_ADJUST_STATE,
    NOTIFY_BRIGHTNESS_STATE,
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
    DATABASE_STATE = 318,
    CASE_MAX_STATE,
    PICTURE_HDR_STATE = 51,
};

enum {
    SCENE_NULL,
    SCENE_PORTRAIT,
    SCENE_FOOD,
    SCENE_WATERSIDES,
    SCENE_BLUESKY,
    SCENE_MOUNTAIN,
    SCENE_SUNRISE,
    SCENE_ARCHITECTURE,
    SCENE_SNOW,
    SCENE_VEHICLES,
    SCENE_FLOWERS,
    SCENE_PLANTS,
    SCENE_ANIMAL,
    SCENE_INDOOR,
    SCENE_TEXTURE,
    SCENE_NIGHT,
    SCENE_DOCUMENT,
    SCENE_FOOTBAL,
    SCENE_MAX,
};

static inline const char* getFeatureIdName(int FeatureId) {
    switch (FeatureId) {
        case AUTOMATIC_CONTRAST_STATE: return "AUTOMATIC_CONTRAST_STATE";
        case INCREASED_CONTRAST_STATE: return "INCREASED_CONTRAST_STATE";
        case STANDARD_STATE: return "STANDARD_STATE";
        case EYECARE_STATE: return "EYECARE_STATE";
        case COLORTEMPERATURE_ADJUST_STATE: return "COLORTEMPERATURE_ADJUST_STATE";
        case NOTIFY_BRIGHTNESS_STATE: return "NOTIFY_BRIGHTNESS_STATE";
        case KEEP_WHITE_POINT_SRGB_STATE: return "KEEP_WHITE_POINT_SRGB_STATE";
        case CABC_MODE_CUSTOM_STATE: return "CABC_MODE_CUSTOM_STATE";
        case AD_STATE: return "AD_STATE";
        case NIGHT_STATE: return "NIGHT_STATE";
        case HDR_STATE: return "HDR_STATE";
        case HBM_STATE: return "HBM_STATE";
        case SUNLIGHT_SCREEN_STATE: return "SUNLIGHT_SCREEN_STATE";
        case NOTIFY_DISPLAY_STATE: return "NOTIFY_DISPLAY_STATE";
        case CAMERA_STATE: return "CAMERA_STATE";
        case EXT_COLOR_PROC_STATE: return "EXT_COLOR_PROC_STATE";
        case CHANGE_COLOR_MODE_STATE: return "CHANGE_COLOR_MODE_STATE";
        case FOD_SCREEN_ON_STATE: return "FOD_SCREEN_ON_STATE";
        case BCBC_CUST_STATE: return "BCBC_CUST_STATE";
        case GAME_STATE: return "GAME_STATE";
        case DC_BACKLIGHT_STATE: return "DC_BACKLIGHT_STATE";
        case DFPS_STATE: return "DFPS_STATE";
        case FOD_MONITOR_ON_STATE: return "FOD_MONITOR_ON_STATE";
        case COLOR_TEMP_STATE: return "COLOR_TEMP_STATE";
        case FPS_SWITCH_STATE: return "FPS_SWITCH_STATE";
        case DOZE_BRIGHTNESS_STATE: return "DOZE_BRIGHTNESS_STATE";
        case EXPERT_STATE: return "EXPERT_STATE";
        case VIDEO_STATE: return "VIDEO_STATE";
        case ACTIVE_APP_STATE: return "ACTIVE_APP_STATE";
        case SMART_DFPS_STATE: return "SMART_DFPS_STATE";
        case DUMP_STATE: return "DUMP_STATE";
        case PAPER_MODE_STATE: return "PAPER_MODE_STATE";
        case TRUE_TONE_STATE: return "TRUE_TONE_STATE";
        case FOD_LOWBRIGHTNESS_FOD_STATE: return "FOD_LOWBRIGHTNESS_FOD_STATE";
        case ADAPTIVE_HDR: return "ADAPTIVE_HDR";
        case DITHER_STATE: return "DITHER_STATE";
        case TOUCH_STATE: return "TOUCH_STATE";
        case ORITATION_STATE: return "ORITATION_STATE";
        case CUP_BLACK_COVERED_STATE: return "CUP_BLACK_COVERED_STATE";
        case DOLBY_VISION_STATE: return "DOLBY_VISION_STATE";
        case NOTIFY_10BIT_STATE: return "NOTIFY_10BIT_STATE";
        case DATABASE_STATE: return "DATABASE_STATE";
        case PICTURE_HDR_STATE: return "PICTURE_HDR_STATE";
        default: return "Unknown";
    }
}

enum {
    AUTOMATIC_CONTRAST_WARM = 1,
    AUTOMATIC_CONTRAST_NORMAL = 2,
    AUTOMATIC_CONTRAST_COLD = 3,
};

enum DISPLAY_CABC_MODE {
    DISPALY_CABC_OFF = 0,
    DISPALY_CABC_MODE_DEFAULT = 1,
    DISPALY_CABC_MODE_GUI = 2,
    DISPALY_CABC_MODE_STILL = 3,
    DISPALY_CABC_MODE_MOVIE = 4,
};

enum DISPLAY_ACL_MODE {
    DISPALY_ACL_OFF = 0,
    DISPALY_ACL_HIGH = 1,
    DISPALY_ACL_MEDIUM = 2,
    DISPALY_ACL_LOW = 3,
};

enum ACTIVE_APPID {
    NONE = 0,
    IQIYI = 1,
    TENCENT_VIDEO = 2,
    BILIBILI = 3,
    CAMERA = 4,
    GALLERY = 5,
    MIUI_PHOTOSHOP = 6,
    YOUKU = 7,
    TIKTOK = 8,
    KUAISHOU = 9,
    MIVIDEO = 10,
    CNTV = 11,
};
#endif
