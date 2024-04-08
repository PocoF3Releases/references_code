#ifndef _DISPLAY_FEATURE_SERVICE_H_
#define _DISPLAY_FEATURE_SERVICE_H_

#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <pthread.h>
#include "IDisplayFeatureControl.h"

namespace android {

enum DISPLAY_EFECTS {
    DISPLAY_ENV_ADAPTIVE_COLD = 0,
    DISPLAY_ENV_ADAPTIVE_NORMAL,
    DISPLAY_ENV_ADAPTIVE_WARM,
    DISPLAY_COLOR_ENHANCE,
    DISPLAY_STANDARD,
    DISPLAY_EYECARE,
    DISPLAY_COLOR_TEMPERATURE,
    DISPLAY_KEEP_WP_SRGB,
    DISPLAY_CABC_MODE_SWITCH,
    DISPLAY_BRIGHTNESS_NOTIFY,
    DISPLAY_NIGHT_MODE,
    DISPLAY_CCBB,
    DISPLAY_AD,
    DISPLAY_HDR,

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
    DISPPARAM_HBM_OFF = 0xF0000,
    DISPPARAM_NORMALMODE1 = 0x100000,
    DISPPARAM_SRGB = 0x300000,
};

class DisplayFeatureService : public BnDisplayFeatureControl
{
public:

    DisplayFeatureService();
    ~DisplayFeatureService();

    static void instantiate();

    virtual int setFeatureEnable(int displayId, int caseId, int modeId, int cookie);
    virtual int setFunctionEnable(int displayId, int caseId, int modeId, int cookie);
    virtual void notify(int displayId, int caseId, int modeId, int cookie);

    void onMessageReceived(int what);
    void onMessageReceivedExt(int what, int value, int cookie);
    void waitForEvent();
    static void * threadWrapper(void *me);
    int threadFunc();

protected:
    void setBrightness(int value);
    int getBrightness();
    void sendMessage(int caseId, int modeId, int cookie);
private:
    Mutex mLock;
    Mutex mBLLock;
    int mCurBL = 0;
    int mPreviousCaseId;
    int mPreviousModeId;
    pthread_t  mThread;
    bool mthread_exit = false;
};

}; // namespace android


#endif
