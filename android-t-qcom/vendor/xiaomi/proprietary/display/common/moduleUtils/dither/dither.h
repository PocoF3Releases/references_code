#ifndef _DITHER_H_
#define _DITHER_H_
#include "SensorCtrlIntf.h"
#include "DisplayFeatureHal.h"
#include "mi_stc_service.h"
#include "DisplayPostprocessing.h"
namespace android {
class DisplayEffectBase;
class Dither : public RefBase
{
public:
    Dither(const sp<DisplayEffectBase>& display_effect);
    virtual ~Dither() {}
    void init();
    void sensorCallback(int event, const Event &info);
    void setIGCDither(int enable, int strength);
    static void *ThreadWrapperIGCDitherHist(void *);
    int threadFuncIGCDitherHist();
    int histogram_operation(int cmd);
    void setDisplayState(int display_state);
    int getDefaultStrength();
    int displayId = DISPLAY_PRIMARY;
private:
    sp<DisplayEffectBase> mDisplayEffect;
    sp<MiParseXml> mParseXml[kMaxNumImpl];
    pthread_t mThreadIGCDitherHist;
    pthread_cond_t mIGCDitherHistCond;
    pthread_mutex_t mIGCDitherHistlock;
    Mutex mHistLock;
    int mThreadHistExit;
    int mHistStatus = HIST_CMD_STOP;
    int mIGCDitherEnable = 0;
    int mIGCDitherStrength = 0;
    int mZeroGayScaleRatio = 0;
    int current_dither_en = 0;
    int mDisplayState;
    int mLastLux = 11;
    int mDefaultStrength = 5;
    std::vector<int> mDitherHist;
    std::vector<int> mDither0GrayScaleThresh;
}; // Class Dither
} //namespace android
#endif
