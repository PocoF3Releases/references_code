#ifndef _BRIGHTNESS_MI_H_
#define _BRIGHTNESS_MI_H_
#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <string>
#include <fstream>
#include <cutils/properties.h>
#include <unistd.h>
#include <pthread.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include <poll.h>
#include "display_property.h"
#include "parseXml.h"
#include "DisplayFeatureHal.h"
#include <vector>
#include "display_property.h"
#include "mi_display_device_manager.h"
#include "DisplayPostprocessing.h"
#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/AMessage.h>
#include "df_log.h"
#ifndef MTK_PLATFORM
#include "IQService.h"
#define PRIMARY_BACKLIGHT_PATH "/sys/class/backlight/panel0-backlight/brightness"
#else
#define PRIMARY_BACKLIGHT_PATH "/sys/class/leds/lcd-backlight/brightness"
#endif
#define SECONDARY_BACKLIGHT_PATH "/sys/class/backlight/panel1-backlight/brightness"

namespace android {
class DisplayEffectBase;
typedef struct MiBrightnessParam {
    int Mi12BitEnable;
    int hbmEnable;
    int IcDimmingDisable;
    int hbmType;
    int dcType;
    int dcDBVThresh;
    int dcDimmingEnable;
    int panelMaxDbv;
    int panelVirtualMax;
    int hbmMaxDbv;
    int manualMaxBl;
    int autoMaxBl;
    int minBl;
    int pwmMinDBV;
    int Mi12BitThresh;
    int Mi12BitPanelDBVThresh;
    int GabbEnable;
    int GabbThresh;
    int pccDCDivision;
    int silkyBrightnessEnable;
    // Dynamic params
    int brightness;
    int lastNoZeroBrightness;
    int dbv;
    int hbmStatus;
    int hdrStatus;
    double pccVal;
    int DCBLEnable;
    double dcPccVal;
} MiBrightnessParam;

enum HBM_TYPE {
    HBM_TYPE_2047 = 0,
    HBM_TYPE_4095 = 1,
};

enum DC_TYPE {
    DC_TYPE_CRC = 0,
    DC_TYPE_PANEL = 1,
    DC_TYPE_PCC = 2,
    DC_TYPE_QCOM_DC_DIMMING = 3,
};

enum HBM_CMD {
    HBM_CMD_OFF = 0,
    HBM_CMD_ON = 1,
};

enum BL_UPDATE_TYPE {
    UPDATE_NORMAL = -1,
    UPDATE_AFTER_RESUME = -2,
};

class MiBrightness : public AHandler
{
public:
    MiBrightness(const sp<DisplayEffectBase>& display_effect);
    virtual ~MiBrightness();
    void init();
    int timerCallback(int caseId, int generation, int value);
    void updateBlCoeff(float coeff);
    void HandleHDR(int enable);
    void setDCMode(int enable);
    MiBrightnessParam getBrightnessParam();
    int getDBV(int brightness);
    double getPcc(int brightness);
    int writeDBV(int value);
    void SetDisplayState(int State);
    int HandleBrightness(int value);
    int Set12BitPCCConfig(int brightness);
    int getDCMode();
    int getLastNonZeroBl();
    int setDimming(int enable);
    void dump(std::string& result);
    int displayId;

    enum {
        kBRIGHTNESS             = 'BRTN',
    };

    MiBrightnessParam param;
    // Params for 12/13 bit.
    std::vector<int> BlLut;
    std::vector<int> BlLutDC;
    std::vector<float> pccLut;
    std::vector<float> pccLutDC;
    // Params for PCC DC
    std::vector<float> pccDCCoeff;
    bool restoreAfterResume = false;

protected:
    void onMessageReceived(const sp<AMessage> &msg);

private:
    static void *ThreadWrapperBacklight(void *);
    int threadFuncBacklight();
    int registerPollEvent();
    int writeDBVCmd(int value);
    int HandleHBM(int enable);
    int HandleHDRHBM(int brightness);
    int HandleDCDimming(int brightness);
    int SetPCCConfigDC(int brightness, int dbv);
    int HandleDCBacklight(int brightness, int dbv, int delay);
    void enableDCDimming(int enable);
    void sendBrightness(int brightness);
    sp<DisplayEffectBase> mDisplayEffect = nullptr;
    sp<DisplayDeviceManager> miDDM = nullptr;
    int mBacklightPoll;
    pthread_t mThreadBacklight;
    pollfd mDFPollFd;
    sp<MiParseXml> mParseXml[DISPLAY_MAX];
    static constexpr const char* kDispFeaturePath = "/dev/mi_display/disp_feature";
    unsigned int mTimerGeneration[TIMER_STATE_MAX] = {0};
    float mCoeff = 1.0;
    Mutex mBLLock;
    int mDisplayState = 0;
    sp<ALooper> mLooper;
}; // class MiBrightness
} //namespace android

#endif
