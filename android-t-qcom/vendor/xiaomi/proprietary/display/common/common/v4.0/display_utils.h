#ifndef _DISPLAY_UTILS_H_
#define _DISPLAY_UTILS_H_

#include <stdint.h>
#include <map>
#include "displayfeature_defs.h"
#include "DFComDefine.h"

using std::map;
using std::string;
using std::make_pair;
using std::pair;

#define MATRIX_ORDER    3
#define MATRIX_SIZE     4
namespace android {

enum DisplayMode {
    kModeDefault,
    kModeVideo,
    kModeCommand,
};

enum BacklightType {
    kBLNone,
    kDDIC,
    kWLed,
    kPWM,
};

enum PanelOrientType {
    kOrientNone,
    kHorzFlip,
    kVertFlip,
    kHorzVertFlip,
};

enum DsiConfigType {
    kDsiConfigNone,
    kSingleDisp,
    kExtBrige,
    kSplitDisp,
    kSplitExtBrige,
};

enum BACKDOOR_CODE {
    DISPLAY_INFO_GRAY = 0,
    DOZE_BRIGHTNESS_STATE = 25,
    UPDATE_WCG_STATE = 10000,
    UPDATE_DFPS_MODE = 10035,
    UPDATE_SECONDARY_FRAME_RATE = 10036,
    UPDATE_SMART_DFPS_MODE = 10037,
    UPDATE_PCC_LEVEL = 20000,
    SET_COLOR_MODE_CMD = 30000,
    SET_DC_PARSE_STATE = 40000,
    NOTIFY_HDR_STATE = 50000,
    SEND_HBM_STATE = 60000,
    LOCK_FPS_REQUEST = 70000,
    SET_FOLD_STATE = 80000,
    NOTIFY_DOLBY_STATE = 90000,
};

struct PanelInfo {
    char panel_name[256] = {0};              // Panel name
    bool dfps_support = false;               // Panel Supports dynamic fps
    char panel_mode[10] = {0};               // Display mode
    bool ccbb_enabled = false;               // Panel need ccbb mode
    bool bl_notify_enabled = false;          // brightness notify mode
    char bl_type[10] = {0};                  // backlight type
    int mdp_transfer_time_us = 0;            // mdp transfer time per frame(us)
};

class DisplayUtil {
public:
    static int matrix_inverse(float matrix[MATRIX_ORDER][2*MATRIX_ORDER]);
    static void getDisplayMode(int device_node, struct PanelInfo *panel_info);
    static int getPanelInfo(int device_node, struct PanelInfo *panel_info);
    static int getPanelName(int device_node,char *panel_name);
    static int readPanelMaxLuminance(int &max_luminance);
    static int readMaxBrightness(long *max_brightness);
    static int readPanelWhiteXYCoordinate(int &x_coordinate, int &y_coordinate);
    static int readWPMaxBrightness(long *max_wp_brightness);
    static int readPanelNTC(int *temperature);
    static int readPanelTemp(int *temperature);
    static int writePanelDsiCmds(char* cmds);
    static int writeDCBacklight(char* value);
    static int writeDualBacklight(char* value);
    static int writeDozeBrightness(char* value);
    static int parseLine(const char *input, const char *delim, char *tokens[],
                            const unsigned int max_token, unsigned int *count);
    static void xy_to_XYZ(double x, double y,
        double *xXYZ, double *yXYZ, double *zXYZ);
    static void xy_to_cct(double x, double y, double *cct);
    static void xy_to_ulvl(double x, double y,
        double *ul, double *vl);
    static void ulvl_to_xy(double ul,double vl,
        double *xl,double *yl);
    static void cct_to_xy(const double cct,
        double *x, double *y);
    static void registerCallback(void *callback);
    static bool isRegisterCallbackDone();
    static void onCallback(int display_id, int value,
        float red = 1.0f, float green = 1.0f, float blue = 1.0f);
    static void setDCParseCallbackEnable(bool enable);
private:
    static NOTIFY_CHANGED mCallback;
    static bool mDCCBEnable;
};


#define DISP_LOG(fmt, args...) \
    do { \
        if (disp_log_en) \
            ALOGD(fmt, ##args); \
        else \
            ALOGV(fmt, ##args); \
    } while (0)


extern bool disp_log_en;

#ifdef MTK_PLATFORM
#define MAX_BACKLIGHT_PATH "/sys/class/leds/lcd-backlight/max_brightness"
#define FB_0_PATH "/dev/graphics/fb0"
#ifdef PLATFORM_NON_QCOM
#define DISP_PARAM_PATH "/sys/devices/platform/mtkfb/disp_param"
#else
#define DISP_PARAM_PATH "/sys/devices/virtual/graphics/fb0/msm_fb_dispparam"
#endif
#define DISP_READ_WHITE_POINT "0x0E"
#else
#define MAX_BACKLIGHT_PATH "/sys/class/backlight/panel0-backlight/max_brightness"
#define DISP_PARAM_PATH "/sys/class/drm/card0-DSI-1/disp_param"
#define DISP_READ_WHITE_POINT "0x0E"
#endif
#define DISP_READ_MAX_LUMINANCE "0x0F"
#define DISP_NTC_PATH "/sys/class/thermal/thermal_zone3/temp"

#define MAX_WP_BACKLIGHT_PATH "/sys/class/mi_display/disp-DSI-0/wp_info"
#define DISP0_DEV_PATH "/sys/class/mi_display/disp-DSI-0"
#define DISP1_DEV_PATH "/sys/class/mi_display/disp-DSI-1"
};
#endif //_DISPLAY_UTILS_H_


