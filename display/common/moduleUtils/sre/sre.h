#ifndef _SRE_H_
#define _SRE_H_

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
#include "mi_stc_service.h"

namespace android {
class DisplayEffectBase;

typedef struct SreGammaMap_T {
    int map[15];
} SreGammaMap_T;

typedef struct histData_T {
    int histData[8];
} histData_T;

typedef struct SREConfigParams {
    std::vector<int> sreLevels;
    std::vector<int> hdrSreLevels;
    std::vector<SreGammaMap_T> sreGammaMap;
    std::vector<int> sreDivisor;
    std::vector<int> hdrSreDivisor;
    int sreExitThreshold;
    int hdrSreExitThreshold;
    int sreLevelNum;
    int sreHistZoneNum;
    int originalModeGcIndex;
    std::vector<histData_T> histData0Zone;
    std::vector<histData_T> histData1Zone;
} SREConfigParams;

class Sre : public RefBase
{
public:
    Sre(const sp<DisplayEffectBase>& display_effect);
    virtual ~Sre() { };
    int HandleSunlightScreen(int modeId, int cookie);
    int getSunlightLevel(int lux);
    int enableSRE(bool enable);
    int setSREFunction(int mode, int  cookie);
    int setSREStrength(int lux, bool freeze);
    void setDisplayState(int display_state);
    int getSREStatus();
    void sensorCallback(int event, const Event &info);
    int histogram_operation(int cmd);
    static void *ThreadWrapperSREHist(void *);
    int threadFuncSREHist();
    static void *ThreadWrapperSREIGC(void *);
    int threadFuncSREIGC();
    void setDisplayEffect(int modeId);
    void dump(std::string& result);
    bool mSreIgcOrigin = true;
    int displayId = DISPLAY_PRIMARY;
private:
    void SetSREToSTC(int enable);
    void setSreGCIndex(int index);
    int getGCIndex(int modeId);
    void init();
    sp<MiParseXml> mParseXml[kMaxNumImpl];
    sp<DisplayEffectBase> mDisplayEffect = nullptr;
    pthread_t mThreadSREHist;
    pthread_cond_t mCPHistCond;
    pthread_mutex_t mHistlock;
    int mThreadHistExit;
    int mHistStatus = HIST_CMD_STOP;
    Mutex mHistLock;
    int sre_hist_info;
    int mDisplayState;
    int realIGCLevel;
    pthread_t mThreadIGC;
    pthread_cond_t mCPIGCCond;
    pthread_mutex_t mIGClock;
    int luxLevelIndex;
    SREConfigParams mSREConfigParams;
    int mSREEnabled;
    int mLastLux = 0;
    bool mLastFreezeStatus = false;
    int mLastSLPCCLevel = 0;
    int mDataZoneIndex = 0;
    int mDimmingStep = 0;
    int mHDREnable = 0;
    int mInitDone = 0;
    std::vector<int> mLastSreLevels;
    int mLastExitThreshold;
    int mCurModeId = 0;
    int mSreStatus = 0;
    int mDivisor = 0;
    int mGcIndex = -1;
    std::map<int, int> mGCIndexMap;
}; //Class Sre
}  //namespace android
#endif
