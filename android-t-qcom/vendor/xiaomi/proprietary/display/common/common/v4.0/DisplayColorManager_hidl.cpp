
#include "DisplayColorManager_hidl.h"
#include "display_config.h"
#include <utils/Log.h>
#include "DisplayFeatureHal.h"
#include <vector>
#include <math.h>
#include "DisplayPostprocessing.h"

#define LOG_TAG "DisplayFeatureHal"
#define LOG_NDEBUG     0
#define GC_TABLE_LEN   1024
#define IGC_TABLE_LEN  256
#define GC_PARAM_MAX   1023
#define IGC_PARAM_MAX  4095
#define LUT_SIZE       17*17*17

using std::string;

namespace android {

DisplayColorManager::DisplayColorManager(int displayId)
    :mDisplayId(displayId)
{
    mPaParam.hue = 0;
    mPaParam.saturation = 0.0;
    mPaParam.value = 0.0;
    mPaParam.contrast = 0.0;
    mPaParam.R = 255;
    mPaParam.G = 255;
    mPaParam.B = 255;
    mPaParam.gamma = 220;
    mPaParam.mode = 0;
    mDSPP = new DSPP_Platform(displayId);
}

DisplayColorManager::~DisplayColorManager()
{
}

int DisplayColorManager::setColorModeWithModeId(int mode, int cookie)
{
    int ret = 0;
    if (mDSPP.get()) {
        ret = mDSPP->setMode(mode, cookie);
    }
    return ret;
}

int DisplayColorManager::setColorModeWithRenderIntent(struct DispPPParam *pSDPPParams)
{
    int ret = 0;
    int mode = pSDPPParams->function_id;
    enum DISPLAY_PP_MODE renderIntent = DISPLAY_PP_MODE_mode258;

    if (mDSPP.get())
        ret = mDSPP->setColorMode(mode, pSDPPParams->param1, &renderIntent);
#ifndef MTK_PLATFORM
    DisplayUtil::onCallback(SET_COLOR_MODE_CMD, renderIntent);
#endif
    return ret;
}

void DisplayColorManager::setParams(void* params)
{
    if (params != NULL)
        memcpy(&mPaParam, params, sizeof(mPaParam));
}

int DisplayColorManager::setColorService(struct DispPPParam *pSDPPParams)
{
    int ret = 0;
    int dimming = 0;
    DF_LOGV("Enter");
    if (!pSDPPParams) {
        DF_LOGW("invalid parameter");
        return ret;
    }

    int fvalue = pSDPPParams->param1;
    int cookie = pSDPPParams->param2;
    switch (( enum QC_COLORSERVICE_TYPE)pSDPPParams->function_id) {
        case COLOR_PCC: {
            ret = setPCC(fvalue, cookie);
        } break;
        case COLOR_GAMMA: {
            ret = setGammaCorrectionCfg(fvalue, cookie, pSDPPParams);
        } break;
        case COLOR_PA: {
            dimming = pSDPPParams->vec_payload[0];
            ret = setGlobalPACfg(fvalue, cookie, dimming);
        } break;
        case COLOR_CLEAR: {
            if (fvalue) {
                ret = setGlobalPACfg(EXPERT_CLEAR, 0, 1);
                ret |= setPCC(EXPERT_CLEAR, 0);
                pSDPPParams->vec_payload.push_back(1); // set GC enable
                pSDPPParams->vec_payload.push_back(0); // set GC file id
                pSDPPParams->vec_payload.push_back(1); // set GC dimming
                ret |= setGammaCorrectionCfg(220, GAMMA_GC, pSDPPParams);
                break;
            }
            ret |= setPCC(EXPERT_CLEAR, 0);
            DisablePA();
            DisableGC();
        } break;
        case COLOR_RESTORE: {
            pSDPPParams->vec_payload.push_back(1); // set GC enable
            pSDPPParams->vec_payload.push_back(0); // set GC file id
            pSDPPParams->vec_payload.push_back(1); // set GC dimming
            ret = setGlobalPACfg(EXPERT_RESTORE, 0, 1);
            ret |= setGammaCorrectionCfg(mPaParam.gamma, GAMMA_GC, pSDPPParams);
            ret |= setPCC(EXPERT_RESTORE, 0);
        } break;
        case COLOR_LUT: {
            dimming = pSDPPParams->vec_payload[0];
            ret = setLut(fvalue, cookie, dimming);
        } break;
        case COLOR_IGC_DITHER: {
            ret = setIGCDither(fvalue, cookie);
        } break;
        case COLOR_PA_DISABLE: {
            DisablePA();
        } break;
        default:
            DF_LOGE("error mode ...");
    }

    return ret;

}

int DisplayColorManager::setGlobalPACfg(int caseId, int value, int dimming)
{
    int ret = 0;
    DF_LOGE("enter caseId %d, value %d, dimming %d", caseId, value, dimming);

    switch (caseId) {
        case EXPERT_H:
            mPaParam.hue = value;
            break;
        case EXPERT_S:
            mPaParam.saturation = value;
            break;
        case EXPERT_V:
            mPaParam.value = value;
            break;
        case EXPERT_CONTRAST:
            mPaParam.contrast = value;
            break;
        default:
            break;
    }
    if (mDSPP.get())
        ret = mDSPP->setGlobalPA(caseId, value, dimming, mPaParam);

    DF_LOGV("return %d", ret);
    return ret;
}

int DisplayColorManager::setGammaCorrectionCfg(int targetGamma, int type, struct DispPPParam *pSDPPParams)
{
    int ret = 0;

    if (mDSPP.get())
        ret = mDSPP->setGammaCorrection(targetGamma, type, pSDPPParams);

    return ret;
}

int DisplayColorManager::setIGCDither(int modeId, int strength)
{
    int ret = 0;

    if (mDSPP.get())
        ret = mDSPP->setIGCDither(modeId, strength);

    return ret;
}

int DisplayColorManager::setLut(int modeId, int cookie, int dimming)
{
    int ret = 0;

    if (mDSPP.get())
        ret = mDSPP->setLut(modeId, cookie, dimming);

    return ret;
}

int DisplayColorManager::setPCC(int caseId, int value)
{
    int ret = 0;
    DF_LOGV("setPcc value case %d value %d", caseId, value);
    if (caseId != EXPERT_CLEAR) {
        switch (caseId) {
            case EXPERT_R:
                mPaParam.R = value;
                break;
            case EXPERT_G:
                mPaParam.G = value;
                break;
            case EXPERT_B:
                mPaParam.B = value;
                break;
            case EXPERT_RESTORE:
                break;
            default:
                DF_LOGE("Invalid caseId %d", caseId);
        }
    } else {
        mPaParam.R = 255;
        mPaParam.G = 255;
        mPaParam.B = 255;
    }

    return ret;
}

void DisplayColorManager::getPCC(double* coeffs)
{
    if (coeffs != NULL) {
        coeffs[0] = mPaParam.R/255.0;
        coeffs[1] = mPaParam.G/255.0;
        coeffs[2] = mPaParam.B/255.0;
    }
}

int DisplayColorManager::DisablePcc()
{
    int ret = 0;

    if (mDSPP.get())
        ret = mDSPP->DisablePcc();

    return ret;
}

int DisplayColorManager::DisablePA()
{
    int ret = 0;

    if (mDSPP.get())
        ret = mDSPP->DisablePA();

    return ret;
}

int DisplayColorManager::DisableIGC()
{
    int ret = 0;

    if (mDSPP.get())
        ret = mDSPP->DisableIGC();

    return ret;
}

int DisplayColorManager::DisableGC()
{
    int ret = 0;

    if (mDSPP.get())
        ret = mDSPP->DisableGC();

    return ret;
}

int DisplayColorManager::DisableLut()
{
    int ret = 0;

    if (mDSPP.get())
        ret = mDSPP->DisableLut();

    return ret;
}

int DisplayColorManager::forceScreenRefresh()
{
    DF_LOGV("Enter");
    int ret = 0;

    if (mDSPP.get())
        ret = mDSPP->forceScreenRefresh();

    return ret;
}

}
