#include "videomode.h"
#include "DisplayEffectBase.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "VIDEOMODE"
#endif

namespace android {
int DFLOG::loglevel = 1;
VideoMode::VideoMode(const sp<DisplayEffectBase>& display_effect)
      : mDisplayEffect(display_effect) {
    if (!mDisplayEffect.get()) {
        return;
    }
    mDSPP = new DSPP_Platform(mDisplayEffect->mDisplayId);
}

VideoMode::~VideoMode()
{

}

int VideoMode::enableVideoMode(int modeId, int cookie)
{
     int ret = 0;
    struct OutputParam sParams;
    int enable = 1;
    int lastMode = mVideoModeId;

    if (modeId < VIDEO_MODE_ORIGIN || modeId >= VIDEO_MODE_MAX) {
        DF_LOGE("invalid input %d", modeId);
        return ret;
    }
    mVideoModeId = modeId;
    if (mDisplayEffect->mHDRStatus || (mDisplayState != kStateOn && VIDEO_MODE_ORIGIN != modeId))
        return ret;
#ifndef MTK_PLATFORM
    if (VIDEO_MODE_ORIGIN != modeId) {
        if (lastMode == VIDEO_MODE_ORIGIN) {
            sParams.function_id = DISPLAY_COLOR_MODE_NATURE;
            ret = mDisplayEffect->setDisplayEffect(SET_COLOR_MODE, &sParams);
            if (GetExpertStatus())
                mDisplayEffect->HandleExpert(EXPERT_DISABLE, 0);
        }
            sParams.function_id = COLOR_LUT;
            sParams.param1 = modeId + 3; //Lut index 0 ~ 3 used for factory calibration.
            sParams.param2 = enable;
            sParams.vec_payload.push_back(1);
            ret = mDisplayEffect->setDisplayEffect(SET_DSPP_MODE, &sParams);
            mDisplayEffect->mLutId = modeId + 3;
    } else {
        mDisplayEffect->restoreQcomMode();
        RestoreAIDisplay();
    }
    if (VIDEO_MODE_SDR_TO_HDR == modeId) {
        mDisplayEffect->mDisplayVSDR2HDR->setVSDR2HDRFunction(VSDR2HDR_ON,0);
    } else if (modeId != VIDEO_MODE_SDR_TO_HDR && VIDEO_MODE_SDR_TO_HDR == lastMode){
        mDisplayEffect->mDisplayVSDR2HDR->setVSDR2HDRFunction(VSDR2HDR_OFF,0);
    }
#else
    if (property_get_int32("ro.vendor.pq.mtk_disp_c3d_support", 0)) {
        if (modeId) {
            if (GetExpertStatus()) {
                mDisplayEffect->HandleExpert(EXPERT_DISABLE, 0);
            }
            if (!mDisplayEffect->GetHdrStatus()) {
                if (modeId == 2) {
                    ret = mDSPP->setC3dLut(0, C3D_LUT_OFF);
                    mDisplayEffect->onCallback(SET_COLOR_MODE_CMD, modeId);
                } else {
                    int value = 0;
                    mDisplayEffect->onCallback(SET_COLOR_MODE_CMD, 0);
                    sParams.function_id = COLOR_LUT;
                    sParams.param1 = C3D_LUT_MODE_VIVID + modeId - 1; //Lut index 0 ~ 3 used for factory calibration.
                    sParams.param2 = C3D_LUT_SET;
                    sParams.vec_payload.push_back(0);
                    ret = mDisplayEffect->setDisplayEffect(SET_DSPP_MODE, &sParams);
                    mDisplayEffect->mLutId = modeId + 3;

                    sParams.function_id = DISPLAY_PQ_MODE;
                    sParams.param1 = DISP_PQ_MODE_MOVIE_0 + modeId;
                    ret |= mDisplayEffect->setDisplayEffect(SET_MTK_MODE, &sParams);
                }
            }
        } else {
            mDisplayEffect->onCallback(SET_COLOR_MODE_CMD, 0);
            mDisplayEffect->restoreQcomMode();
        }
    } else if (property_get_bool("ro.vendor.video.style.by.layer.support", false)) {
        if (modeId) {
            if (GetExpertStatus()) {
                mDisplayEffect->HandleExpert(EXPERT_DISABLE, 0);
            }
        } else {
            mDisplayEffect->restoreQcomMode();
        }
    } else {
        if (modeId) {
            int value = 0;
            if (lastMode == VIDEO_MODE_ORIGIN) {
                if (GetExpertStatus()) {
                    mDisplayEffect->HandleExpert(EXPERT_DISABLE, 0);
                }
            }

            sParams.function_id = DISPLAY_PQ_MODE;
            sParams.param1 = DISP_PQ_MODE_MOVIE_0 + modeId;
            ret = mDisplayEffect->setDisplayEffect(SET_MTK_MODE, &sParams);
        } else {
            mDisplayEffect->restoreQcomMode();
        }
    }
#endif
    return ret;
}

int VideoMode::GetExpertStatus()
{
    int ret;
    ret = mDisplayEffect->GetExpertMode();
    return ret;
}

void VideoMode::RestoreAIDisplay()
{
    mDisplayEffect->RestoreAIDisplay();
}

void VideoMode::dump(std::string& result)
{
    char res_ch[1024] = {0};
    result.append("\nVB:\n");
    snprintf(res_ch, sizeof(res_ch), "Disp ID: %d, Mode: %d, HDR: %d\n",
                             mDisplayEffect->mDisplayId, mVideoModeId, mDisplayEffect->mHDRStatus);
    result.append(std::string(res_ch));
}
void VideoMode::SetDisplayState(int display_state)
{
    mDisplayState = display_state;
}
}//namespace android
