#ifndef _DISPLAY_FEATURE_HAL_H_
#define _DISPLAY_FEATURE_HAL_H_

#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <pthread.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include "../hardware/displayfeature_defs.h"
#include "display_utils.h"
#include "df_log.h"
#include "DFComDefine.h"

namespace android {


enum DISPLAY_EFECTS {
    DISPLAY_ENV_ADAPTIVE_COLD = 0,
    DISPLAY_ENV_ADAPTIVE_NORMAL,
    DISPLAY_ENV_ADAPTIVE_WARM,
    DISPLAY_COLOR_ENHANCE,
    DISPLAY_STANDARD,
    DISPLAY_EYECARE,
    DISPLAY_COLOR_TEMPERATURE,
    DISPLAY_NIGHT_MODE,
    DISPLAY_KEEP_WP_SRGB,
    DISPLAY_CABC_MODE_SWITCH,
    DISPLAY_BRIGHTNESS_NOTIFY,
    DISPLAY_CCBB,
    DISPLAY_AD,
    DISPLAY_HDR,
    DISPLAY_NIGHTMODE_EYECARE,
    DISPLAY_CAMERA,
    DISPLAY_EXT_COLOR,
    DISPLAY_GAME,
    DISPLAY_EXPERT,
    DISPLAY_EFECTS_MAX
};

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
    DISPPARAM_MAX_LUMINANCE_READ = 0xF,
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
    DISPPARAM_HBM_ON = 0x10000,
    DISPPARAM_HBM_FOD_ON = 0x20000,
    DISPPARAM_HBM_FOD2NORM = 0x30000,
    DISPPARAM_DC_ON = 0x40000,
    DISPPARAM_DC_OFF = 0x50000,
    DISPPARAM_HBM_FOD_OFF = 0xE0000,
    DISPPARAM_HBM_OFF = 0xF0000,
    DISPPARAM_LCD_HBM_L1_ON = 0xB0000,
    DISPPARAM_LCD_HBM_L2_ON = 0xC0000,
    DISPPARAM_LCD_HBM_L3_ON = 0xD0000,
    DISPPARAM_LCD_HBM_OFF = 0xA0000,
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
    DISPPARAM_FLAT_MODE_ON = 0x5000000,
    DISPPARAM_FLAT_MODE_OFF = 0x6000000,
    DISPPARAM_DITHER_ON = 0x7000000,
    DISPPARAM_DITHER_OFF = 0x8000000,
    DISPPARAM_IDLE_ON = 0xA000000,
    DISPPARAM_IDLE_OFF = 0xB000000,
    DISPPARAM_DFPS_LEVEL1 = 0x10000000,
    DISPPARAM_DFPS_LEVEL2 = 0x20000000,
    DISPPARAM_DFPS_LEVEL3 = 0x30000000,
    DISPPARAM_DFPS_LEVEL4 = 0x40000000,
    DISPPARAM_DFPS_LEVEL5 = 0x50000000,
    DISPPARAM_DFPS_LEVEL6 = 0x60000000,
    DISPPARAM_DFPS_LEVEL7 = 0x70000000, /* 144hz */
};

enum FPS_LEVEL {
    FPS_MARGIN = 2,
    FPS_24HZ   = 24, /* typical movie fps */
    FPS_25HZ   = 25,
    FPS_30HZ   = 30,
    FPS_48HZ   = 48,
    FPS_50HZ   = 50,
    FPS_60HZ   = 60,
    FPS_90HZ   = 90,
    FPS_120HZ  = 120,
    FPS_144HZ  = 144,
};

typedef enum {
    FPS_SWITCH_DEFAULT    = 244,
    FPS_SWITCH_BENCHMARK  = 247,
    FPS_SWITCH_VIDEO      = 248,
    FPS_SWITCH_TOPAPP     = 249,
    FPS_SWITCH_POWER      = 250,
    FPS_SWITCH_MULTITASK  = 251,
    FPS_SWITCH_CAMERA     = 252,
    FPS_SWITCH_THERMAL    = 253,
    FPS_SWITCH_GAME       = 254,
    FPS_SWITCH_SETTINGS   = 255,
    FPS_SWITCH_SMART      = 256,
    FPS_SWITCH_MAX        = FPS_SWITCH_SMART,
} FPS_SWITCH_MODULE;

typedef enum {
    DITHER_SWITCH_DEFAULT    = 244,
    DITHER_SWITCH_VIDEO      = 248,
    DITHER_SWITCH_MAX,
} DITHER_MDOE;

static inline const char* getFpsSwitchAppName(int module) {
    switch (module) {
        case FPS_SWITCH_BENCHMARK: return "BenchMark";
        case FPS_SWITCH_TOPAPP: return "TopApp";
        case FPS_SWITCH_VIDEO: return "Video";
        case FPS_SWITCH_POWER: return "Power";
        case FPS_SWITCH_MULTITASK: return "MultiTask";
        case FPS_SWITCH_CAMERA: return "Camera";
        case FPS_SWITCH_THERMAL: return "Thermal";
        case FPS_SWITCH_GAME: return "Game";
        case FPS_SWITCH_SETTINGS: return "Settings";
        case FPS_SWITCH_SMART: return "Smtfps";
        case FPS_SWITCH_DEFAULT: return "Default";
        default: return "Unknown";
    }
}

enum HBM_LEVEL {
    HBM_OFF,
    HBM_L1,
    HBM_L2,
    HBM_L3,
};

enum HBM_STATUS {
    HBM_FOD = 0,
    HBM_NORMAL,
    HBM_HDR,
};

enum DOZE_BRIGHTNESS_MODE{
	DOZE_TO_NORMAL = 0,
	DOZE_BRIGHTNESS_HBM,
	DOZE_BRIGHTNESS_LBM,
};

enum FLAT_MODE_STATUS {
    FLAT_MODE_OFF = 0,
    FLAT_MODE_NORMAL,
    FLAT_MODE_NATURE,
};

enum AI_DISP_LUT {
    AI_DISP_LUT_0 = 17,
    AI_DISP_LUT_1,
    AI_DISP_LUT_2,
};

static inline const char* getDozeBrightnessModeName(int mode) {
    switch (mode) {
        case DOZE_TO_NORMAL: return "DOZE_TO_NORMAL";
        case DOZE_BRIGHTNESS_HBM: return "DOZE_BRIGHTNESS_HBM";
        case DOZE_BRIGHTNESS_LBM: return "DOZE_BRIGHTNESS_LBM";
        default: return "Unknown";
    }
}

enum DF_DisplayState {
  kStateOff,        //!< Display is OFF. Contents are not rendered in this state. Client will not
                    //!< receive VSync events in this state. This is default state as well.

  kStateOn,         //!< Display is ON. Contents are rendered in this state.

  kStateDoze,       //!< Display is ON and it is configured in a low power state.

  kStateDozeSuspend,
                    //!< Display is ON in a low power state and continue showing its current
                    //!< contents indefinitely until the mode changes.

  kStateStandby,    //!< Display is OFF. Client will continue to receive VSync events in this state
                    //!< if VSync is enabled. Contents are not rendered in this state.
};

static inline const char* getDisplayStateName(int State) {
    switch (State) {
        case kStateOff: return "Off";
        case kStateOn: return "On";
        case kStateDoze: return "Doze";
        case kStateDozeSuspend: return "DozeSuspend";
        case kStateStandby: return "Standby";
        default: return "Unknown";
    }
}

enum LTM_USERMODE_INDEX {
    LTM_USERMODE_DEFAULT = 0,
    LTM_USERMODE_GAME,
    LTM_USERMODE_GALLERY,
    LTM_USERMODE_VIDEO,
    LTM_USERMODE_MAX,
};

static inline const char* getLtmUserModeName(int LtmUserMode) {
    switch (LtmUserMode) {
        case LTM_USERMODE_DEFAULT: return "default";
        case LTM_USERMODE_GAME: return "game";
        case LTM_USERMODE_GALLERY: return "gallery";
        case LTM_USERMODE_VIDEO: return "video";
        default: return "Unknown";
    }
}

enum DISPLAY_FEATURE_TIMER_STATE {
    TIMER_HBM_OFF,
    TIMER_HBM_ON,
    TIMER_SENSOR_OFF,
    TIMER_SENSOR_ON,
    TIMER_HBM_TEMP_CHECK,
    TIMER_FOOL_PROOF_HBM,
    TIMER_HBM_PERMISSION,
    TIMER_TRUE_TONE_ON,
    TIMER_TRUE_TONE_OFF,
    TIMER_BACKLIGHT,
    TIMER_AI_DISP,
    TIMER_IGC_DITHER,
    TIMER_CCT_ENABLE,
    TIMER_STATE_MAX,
};

enum DF_AD_STATE {
    AD_OFF,
    AD_ON,
    AD_ON_CHANGE_STRENGTH,
    AD_STRENGTH,
};

enum DF_LTM_STATE {
    LTM_OFF,
    LTM_ON,
    LTM_ON_CHANGE_STRENGTH,
    LTM_STRENGTH,
    LTM_VIDEO,
    LTM_USERMODE,
    LTM_HDR,
};

enum BCBC_STATE {
    BCBC_OFF,
    BCBC_ON,
};
enum VSDR2HDR_STATE {
    VSDR2HDR_OFF,
    VSDR2HDR_ON,
};

enum CUP_CASE_LIST {
    CUP_R,
    CUP_G,
    CUP_B,
    CUP_MAX,
};

enum GAME_MODE {
    GAME_MODE_ORIGIN = 0,
    GAME_MODE_VIVID,
    GAME_MODE_BRIGHT,
    GAME_MODE_VIVID_BRIGHT,
};

enum VIDEO_MODE {
    VIDEO_MODE_ORIGIN = 0,
    VIDEO_MODE_VIVID,
    VIDEO_MODE_SDR_TO_HDR,
    VIDEO_MODE_RETRO,
    VIDEO_MODE_COLORLESS,
    VIDEO_MODE_WARM,
    VIDEO_MODE_COOL,
    VIDEO_MODE_LIME,
    VIDEO_MODE_JAZZ,
    VIDEO_MODE_FRESH,
    VIDEO_MODE_PINK,
    VIDEO_MODE_MACHINE,
    VIDEO_MODE_MAX,
};

struct DispParam {
    int displayId;
    int caseId;
    int modeId;
    int cookie;
    int eyeCareLevel;
    int CTLevel;
    int expertParam[EXPERT_MAX];
};

struct TroneToneZone {
    float start;
    float end;
    float coeffA;
    float coeffB;
    float coeffC;
    float coeffN;
};

enum DISPLAY_LIST {
    DISPLAY_PRIMARY,
    DISPLAY_SECONDARY,
    DISPLAY_MAX,
};

enum Df_monitor_event {
    DF_EVENT_BACKLIGHT_CLONE,
    DF_EVENT_UI_READY,
    DF_EVENT_MAX,
};

enum {
    MI_LAYER_FOD_HBM_OVERLAY = 0x1000000,
    MI_LAYER_FOD_ICON = 0x2000000,
    MI_LAYER_AOD_LAYER = 0x4000000,
    MI_FOD_UNLOCK_SUCCESS = 0x8000000,
    MI_LAYER_MAX,
};

enum {
    MI_DF_SCENE_DEFAULT,
    MI_DF_SCENE_0,
    MI_DF_SCENE_1,
    MI_DF_SCENE_2,
    MI_DF_SCENE_3,
    MI_DF_SCENE_4,
    MI_DF_SCENE_MAX,
};

class DisplayFeatureHal : public RefBase, df_device_t
{
public:
    struct DFModuleMethods : public hw_module_methods_t {
          DFModuleMethods() { hw_module_methods_t::open = DisplayFeatureHal::open; }
    };

    DisplayFeatureHal(const hw_module_t *module);
    ~DisplayFeatureHal();
    int init();
    int deinit();
    static bool checkDeviceState(struct df_device* device, int displayId);
    static bool setFeatureStatusCheck(struct df_device* device, int displayId, int caseId, int modeId, int cookie);
    static int setFeatureEnable(struct df_device* device, int displayId, int caseId, int modeId, int cookie);
    static int setFunctionEnable(struct df_device* device, int displayId, int caseId, int modeId, int cookie);
    static void sendMessage(struct df_device* device, int index, int value, const string& cmd);
    static void setListener(struct df_device* device, NOTIFY_CHANGED listener);
    static void notifySFWcgState(bool enable);
    static bool checkModeForWCG(int caseId, int modeId);
    static bool checkWCGEnable(int caseId, int modeId, int cookie);
    static void dump(struct df_device* device, std::string& result);

private:
     // df methods
    static int open(const hw_module_t *module, const char *name, hw_device_t **device);
    static int close(hw_device_t *device);
    static void getCapabilities(struct df_device *device, uint32_t *outCount,
        int32_t *outCapabilities);
    static df_function_pointer_t getFunction(struct df_device *device, int32_t descriptor);

private:
    static Mutex mLock;
    static int mDisplayNum;
    sp<RefBase> mDisplayEffect[DISPLAY_MAX];
};

}; // namespace android


#endif
