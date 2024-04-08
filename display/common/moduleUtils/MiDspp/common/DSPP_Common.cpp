#include "DSPP_Common.h"
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "DisplayFeatureHal"
#endif
namespace android {
DSPP_Common::DSPP_Common(unsigned int displayId) {
    mDisplayId = displayId;
}

DSPP_Common::~DSPP_Common()
{

}

int DSPP_Common::setPccConfig(double *rgbCoeff, int bl)
{
    return 0;
}

int DSPP_Common::setDREFunction(int value)
{
    return 0;
}

int DSPP_Common::setESSFunction(int value)
{
    return 0;
}

int DSPP_Common::setAALBypass(int value)
{
    return 0;
}

int DSPP_Common::setDCStatus(int value)
{
    return 0;
}

int DSPP_Common::setPQMode(int value)
{
    return 0;
}

int DSPP_Common::setHSVC(int type, int value)
{
    return 0;
}

int DSPP_Common::setRGB(uint32_t r_gain, uint32_t g_gain, uint32_t b_gain, int32_t step)
{
    return 0;
}

int DSPP_Common::setGammaIndex(int value)
{
    return 0;
}

int DSPP_Common::setGamePQMode(int value)
{
    return 0;
}

int DSPP_Common::setC3dLut(int lutId, int state)
{
    return 0;
}

int DSPP_Common::setMode(int mode, int cookie)
{
    return 0;
}

int DSPP_Common::setColorMode(int mode, int cookie, enum DISPLAY_PP_MODE * renderIntent)
{
    return 0;
}

int DSPP_Common::forceScreenRefresh()
{
    return 0;
}

int DSPP_Common::setGlobalPA(int caseId, int value, int dimming, GlobalPAParam param)
{
    return 0;
}

int DSPP_Common::setGammaCorrection(int targetGamma, int type, struct DispPPParam *pSDPPParams)
{
    return 0;
}

int DSPP_Common::setIGCDither(int modeId, int strength)
{
    return 0;
}

int DSPP_Common::setLut(int lutId, int cookie, int dimming)
{
    return 0;
}

int DSPP_Common::DisablePcc()
{
    return 0;
}

int DSPP_Common::DisablePA()
{
    return 0;
}

int DSPP_Common::DisableIGC()
{
    return 0;
}

int DSPP_Common::DisableGC()
{
    return 0;
}

int DSPP_Common::DisableLut()
{
    return 0;
}
}
