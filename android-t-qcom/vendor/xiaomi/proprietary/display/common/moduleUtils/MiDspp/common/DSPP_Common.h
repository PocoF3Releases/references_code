#ifndef _DSPP_COMMON_H_
#define _DSPP_COMMON_H_
#include <utils/RefBase.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <vector>
#include <string>
#include "DFComDefine.h"
#include "display_config.h"

using std::string;
namespace android {
enum DATASPACE {
    DATASPACE_SRGB = 1,
    DATASPACE_P3 = 3,
};

class DSPP_Common : public RefBase
{
public:
    DSPP_Common(unsigned int displayId);
    virtual ~DSPP_Common();
    virtual int setPccConfig(double *rgbCoeff, int bl = -1);
    virtual int setDREFunction(int value);
    virtual int setESSFunction(int value);
    virtual int setAALBypass(int value);
    virtual int setDCStatus(int value);
    virtual int setPQMode(int value);
    virtual int setHSVC(int type, int value);
    virtual int setRGB(uint32_t r_gain, uint32_t g_gain, uint32_t b_gain, int32_t step);
    virtual int setGammaIndex(int value);
    virtual int setGamePQMode(int value);
    virtual int setC3dLut(int lutId, int state);
    virtual int setMode(int mode, int cookie);
    virtual int setColorMode(int mode, int cookie, enum DISPLAY_PP_MODE * renderIntent);
    virtual int forceScreenRefresh();
    virtual int setGlobalPA(int caseId, int value, int dimming, GlobalPAParam param);
    virtual int setGammaCorrection(int targetGamma, int type, struct DispPPParam *pSDPPParams);
    virtual int setIGCDither(int modeId, int strength);
    virtual int setLut(int lutId, int cookie, int dimming);
    virtual int DisablePcc();
    virtual int DisablePA();
    virtual int DisableIGC();
    virtual int DisableGC();
    virtual int DisableLut();

    int mDisplayId = 0;
    const std::vector<std::string> mLutFileName = {
        "LUT_sRGB_D65_", "LUT_P3_D65_",
        "LUT_sRGB_D75_", "LUT_P3_D75_",
    };
};
} //namespace android
#endif
