#ifndef _DSPP_PLATFORM_H_
#define _DSPP_PLATFORM_H_
#include "../common/DSPP_Common.h"
#include "mi_stc_service.h"
#include "DisplayFeatureHal.h"
#include "disp_color_apis.h"
#include "display_config.h"
#include "df_log.h"

using namespace qdisplaymode;

namespace android {
class DSPP_Platform : public DSPP_Common
{
public:
    DSPP_Platform(int displayId);
    ~DSPP_Platform() {}
    int setPccConfig(double *rgbCoeff, int bl = -1);
    int setMode(int mode, int cookie);
    int setColorMode(int mode, int cookie, enum DISPLAY_PP_MODE * renderIntent);
    int setGlobalPA(int caseId, int value, int dimming, GlobalPAParam param);
    int setGammaCorrection(int targetGamma, int type, struct DispPPParam *pSDPPParams);
    int setIGCDither(int modeId, int strength);
    int setLut(int lutId, int cookie, int dimming);
    int DisablePcc();
    int DisablePA();
    int DisableIGC();
    int DisableGC();
    int DisableLut();
    int forceScreenRefresh();

private:
    int mDisplayId = DISPLAY_PRIMARY;
};

} //namespace android
#endif
