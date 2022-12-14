#include "colortempmode.h"
#include "DisplayEffectBase.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "COLORTEMPMODE"
#endif

namespace android {

ColorTempMode::ColorTempMode(const sp < DisplayEffectBase > & display_effect)
    : mDisplayEffect(display_effect) {
    if(!mDisplayEffect.get()) {
        return;
    }
}

 ColorTempMode::~ColorTempMode()
{

}

 int ColorTempMode::enableColorTempMode(int modeId, int cookie)
{
    int ret = -1;
    struct OutputParam sParams;

    ALOGD("%s enter", __func__);

    sParams.function_id = DISPLAY_BREAK_CT_DIMMING;
    sParams.param1 = CT_DIMMING_NON_BREAK;
    ret = mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);

    sParams.function_id = DISPLAY_CT_ADJUST;
    sParams.param1 = modeId;
    sParams.param2 = CT_DIMMING_NONE;
    ret |= mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);

    mDisplayEffect->setDispParam(mDisplayEffect->mDisplayId, COLOR_TEMP_STATE, modeId, cookie);
    ALOGD("%s exit", __func__);
    return ret;
}
} //namespace android