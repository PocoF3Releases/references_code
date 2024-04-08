#ifndef _FLATMODE_H_
#define _FLATMODE_H_
#include <iostream>
#include <stdio.h>
#include <string>
#include <math.h>
#include <algorithm>
#include <ctime>
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
#include <mi_disp.h>
#include <vector>
#include "parseXml.h"
#include "display_effects.h"
#include "DisplayFeatureHal.h"
#include "DisplayPostprocessing.h"


//#include "DisplayEffectBase.h"

namespace android {
class DisplayEffectBase;
class FlatMode : public RefBase
{
public:
    FlatMode(const sp<DisplayEffectBase>& display_effect);
    virtual ~FlatMode() { };
    int displayId = DISPLAY_PRIMARY;
    int mFlatModeStatus = FEATURE_OFF;
    int setFlatMode(int fMode, struct OutputParam *pSParams);
    int HandleAppFlatMode(int modeId, int cookie);
    int HandlePicHDRFlatMode(int modeId, int cookie);
private:
    sp<MiParseXml> mParseXml[DISPLAY_MAX];
    sp<DisplayEffectBase> mDisplayEffect = nullptr;
    /*open flatmode in douyin*/
    int AppFlatModeEnable;
    int mInitDone = 0;
    void init();
}; // class FlatMode
} // namespace android

#endif

