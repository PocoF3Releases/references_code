#ifndef _TRUETONE_H_
#define _TRUETONE_H_
#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <fstream>
#include <cutils/properties.h>
#include <unistd.h>
#include <pthread.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include "SensorCtrlIntf.h"
#include "DisplayFeatureHal.h"
#include "parseXml.h"
#include "DisplayPostprocessing.h"
#include "display_effects.h"
#ifndef MTK_PLATFORM
#include "fpsmonitor.h"
#endif

//#include "DisplayEffectBase.h"
#define TRUE_TONE_ON_OFF_DIMMING 10
enum DISPLAY_TRUETONE_STATUS{
        TRUETONE_NORMAL,
        TRUETONE_D65,
};

namespace android {
class DisplayEffectBase;
class TrueTone : public RefBase
{
public:
    TrueTone(const sp<DisplayEffectBase>& display_effect);
    virtual ~TrueTone();
    void init();
    int enableTrueTone(int modeId, int cookie);
    void sensorCallback(int event, const Event &info);
    int timerCallback(int caseId, int generation, int value);
    int setPCCConfigTrueTone(int cct, int mode);
    void SetDisplayState(int display_state);
    void setActiveApp(int modeId);
    void updateTrueToneStatus(int status);
    void HandleTouchState(int caseId, int value);
    void dump(std::string& result);
    int trueToneEnable = 0;
    int displayId = 0;
private:
    void initTrueToneParam();
    int HandleCCT(int forcecct);
    int trueToneClose();
    void HandleCCTBacklight(float cct, int enable);
    int HandleTrueToneStatus(int mode);
    int getEffectStatus(int index);
    int mInitDone = 0;
    int mEffectStatus = 0;
    sp<MiParseXml> mParseXml[DISPLAY_MAX];
    sp<DisplayEffectBase> mDisplayEffect = nullptr;
    unsigned int mTimerGeneration[TIMER_STATE_MAX] = {0};
    int mEnableTrueTone = 0;
    float mSensorCCT = 0.0;
    float mSensorX = 0.0;
    float mSensorY = 0.0;
    float mLastCCT = -1.0;
    float mTargetCCT = 0.0;
    int mCctStepMs = 32;
    int mCctDimmingTime = 4;
    int mAppleLikeGallery = 0;
    int mDisplayTruetoneStatus = TRUETONE_NORMAL;
    struct TroneToneZone *pAppZones = nullptr;
    struct TroneToneZone *pCCTZones = nullptr;
    struct TroneToneZone *pCCTThreshold = nullptr;
    struct TroneToneZone mTimeZones = {0, 0, 0, 0, 0, 0};
    int mAppZoneNum = 3;
    int mCCTZoneNum = 3;
    int mCCTThresholdNum = 12;
    int mTruetoneOnOffdimming = 0;
    int mTruetoneDebug = 0;
    float mCCTBacklightCoeff = 1.0;
    float mlastlux = 0.0;
    int mCCTLuxThreshold = 50;
    bool mCctDisplayState = false;
    Mutex mTruetoneLock;
    TrueToneParams mPccParam;
    int mDisplayState = kStateOn;
    double mTrueToneCoeff[ARRAY_SIZE_EXP] = {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0};
    int mCurApp = NONE;
    int mTouchState = 0;
    int mSkipCCT = 0;
    int mBlCalibExpLvl = 0;
    std::vector<double> mBlCalibCoeff;
    int mBlCalibThresh = 0;
    int mBlCalibCctThresh = 0;
}; // class TrueTone
} // namespace android

#endif
