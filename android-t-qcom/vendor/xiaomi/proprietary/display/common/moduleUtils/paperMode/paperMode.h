#ifndef _PAPER_MODE_H_
#define _PAPER_MODE_H_
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
#include <vector>
#include "parseXml.h"
#include "display_effects.h"
#include "DisplayFeatureHal.h"
#include "DisplayPostprocessing.h"

namespace android {
#define PAPER_COLOR_NUM 3
#define PAPER_PCC_COEFF_NUM 9
#define EYECARE_PCC_COEFF_NUM 9
#define EYECARE_POWER_NUM 4
typedef struct PaperModeParams {
    double PaperCoeff[PAPER_COLOR_NUM][PAPER_PCC_COEFF_NUM];
    double EyeCareCoeff[EYECARE_PCC_COEFF_NUM][EYECARE_POWER_NUM];
    int minCCT;
    int maxCCT;
    int startLevel;
    int maxLevel;
} PaperModeParams;
class DisplayEffectBase;

class PaperMode : public RefBase
{
public:
    PaperMode(const sp<DisplayEffectBase>& display_effect);
    ~PaperMode();
    int setPaperMode(int modeId);
    int setEyeCare(int modeId);
    int getPaperId();
    int clearPaperMode();
    void dump(std::string& result);
    int displayId = 0;
private:
    void init();
    int setPCCConfigPaperMode();
    int setPCCCoeffsToHWC();
    sp<MiParseXml> mParseXml[DISPLAY_MAX];
    sp<DisplayEffectBase> mDisplayEffect = nullptr;
    PaperModeParams mParam;
    double mPaperCoeff[PAPER_PCC_COEFF_NUM] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    double mEyeCareCoeff[EYECARE_PCC_COEFF_NUM] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    int mInitDone = 0;
    int mPaperId = 0;
    int mEyeCareLevel = 0;
}; // PaperMode
} // namespace android
#endif