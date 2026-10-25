
#ifndef _DISPLAY_COLOR_MANAGER_H_
#define _DISPLAY_COLOR_MANAGER_H_

#include <utils/Singleton.h>
#include "display_effects.h"
#include "display_utils.h"
#include "DSPP_Platform.h"

namespace android {

enum QC_COLORSERVICE_TYPE {
     COLOR_PCC,
     COLOR_GAMMA,
     COLOR_PA,
     COLOR_CLEAR,
     COLOR_RESTORE,
     COLOR_LUT,
     COLOR_IGC_DITHER,
     COLOR_PA_DISABLE,
     COLOR_MAX,
};

class DisplayColorManager: public RefBase
{
public:
    DisplayColorManager(int displayId);
    ~DisplayColorManager();
    int setColorModeWithRenderIntent(struct DispPPParam *pSDPPParams);
    int setColorModeWithModeId(int mode, int cookie);
    int setGammaCorrectionCfg(int targetGamma, int type, struct DispPPParam *pSDPPParams);
    int setPCC(int caseId, int value);
    int setColorService(struct DispPPParam *pSDPPParams);
    int setGlobalPACfg(int caseId, int value, int dimming);
    void setParams(void* params);
    void getPCC(double* coeffs);
    int forceScreenRefresh();
    int setLut(int modeId, int cookie, int dimming);
    int setIGCDither(int modeId, int strength);
    int DisablePcc();
    int DisablePA();
    int DisableIGC();
    int DisableGC();
    int DisableLut();
private:
    GlobalPAParam mPaParam;
    int mDisplayId = 0;
    sp<DSPP_Platform> mDSPP = nullptr;
};

};
#endif // _DISPLAY_COLOR_MANAGER_H_
