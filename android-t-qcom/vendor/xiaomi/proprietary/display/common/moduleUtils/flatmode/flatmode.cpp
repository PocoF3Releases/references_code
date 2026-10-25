#include "flatmode.h"
#include "DisplayEffectBase.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "FLATMODE"
#endif

using namespace std;
namespace android {
int DFLOG::loglevel = 1;
FlatMode::FlatMode(const sp<DisplayEffectBase>& display_effect)
    : mDisplayEffect(display_effect) {
    if (mDisplayEffect.get()) {
        displayId = mDisplayEffect->mDisplayId;
    }
    mParseXml[displayId] = new MiParseXml();
    if (mParseXml[displayId].get()) {
        init();
    }
}

void FlatMode::init()
{
    int ret = 0;
    char p[4096];
    if (!mParseXml[displayId].get()) {
        return;
    }
    string key("AppFlatModeEnable");
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        AppFlatModeEnable = atoi(p);
        DF_LOGV("AppFlatModeEnable: %d",atoi(p));
    }
    mInitDone = 1;

}
int FlatMode::setFlatMode(int fMode, struct OutputParam *pSParams)
{
    struct OutputParam sParams;
    int ret = 0;
    if (!mInitDone) {
        DF_LOGE("flatmode param not initialized!");
        return ret;
    }
    DF_LOGV("modeId %d, display state %d", pSParams->function_id, mDisplayEffect->mDisplayState);
    sParams.function_id = DISP_FEATURE_FLAT_MODE;
    sParams.param1 = ((pSParams->function_id == DISPLAY_COLOR_MODE_sRGB ||
          pSParams->function_id == DISPLAY_COLOR_MODE_EXPERT_WCG ||
          pSParams->function_id == DISPLAY_COLOR_MODE_DV_FLAT_ON) ?
                       FEATURE_ON : FEATURE_OFF);
    if (AppFlatModeEnable && mDisplayEffect->curApp == TIKTOK) {
        sParams.param1 = FEATURE_ON;
    }
    if (mFlatModeStatus != sParams.param1 || sParams.param1 == FEATURE_ON) {
        if (AppFlatModeEnable) {
            if (mDisplayEffect->curApp != TIKTOK)
                mFlatModeStatus = sParams.param1;
        } else {
            mFlatModeStatus = sParams.param1;
        }
        ret = mDisplayEffect->featureEnablePolicy(SET_PANEL_MODE, &sParams);
        if (ret != 0) {
            DF_LOGE("failed to set flat mode: value: 0x%x", pSParams->param1);
        }
    }
    return ret;
}

int FlatMode::HandleAppFlatMode(int modeId, int cookie)
{
    int ret = 0;
    struct OutputParam sParams;
    if (AppFlatModeEnable && mFlatModeStatus == FEATURE_OFF) {
        if (modeId == TIKTOK) {
            sParams.function_id = DISP_FEATURE_FLAT_MODE;
            sParams.param1 = FEATURE_ON;
            ret = mDisplayEffect->setDisplayEffect(SET_PANEL_MODE, &sParams);
        }
        if (modeId != TIKTOK && mDisplayEffect->curApp == TIKTOK) {
            sParams.function_id = DISP_FEATURE_FLAT_MODE;
            sParams.param1 = FEATURE_OFF;
            ret = mDisplayEffect->setDisplayEffect(SET_PANEL_MODE, &sParams);
        }
    }
    return ret;
}

int FlatMode::HandlePicHDRFlatMode(int modeId, int cookie)
{
    int ret = 0;
    struct OutputParam sParams;
    if (AppFlatModeEnable && mFlatModeStatus == FEATURE_OFF) {
        if (modeId && mDisplayEffect->curApp == GALLERY) {
            sParams.function_id = DISP_FEATURE_FLAT_MODE;
            sParams.param1 = FEATURE_ON;
            ret = mDisplayEffect->setDisplayEffect(SET_PANEL_MODE, &sParams);
        }
        else {
            sParams.function_id = DISP_FEATURE_FLAT_MODE;
            sParams.param1 = FEATURE_OFF;
            ret = mDisplayEffect->setDisplayEffect(SET_PANEL_MODE, &sParams);
        }
    }
    return ret;
}

}//namespace android

