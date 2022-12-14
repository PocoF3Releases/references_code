/*
 * Copyright (C) 2013 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License") = 0;
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/Singleton.h>
#include <utils/SortedVector.h>
#include <utils/String8.h>
#include <sys/prctl.h>
#include <errno.h>
#include <assert.h>
#include <sys/poll.h>

#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <cutils/properties.h>
#include <fstream>

#include "../common/display_property.h"
#include "DisplayColorManager_hidl.h"
#include "DisplayFeatureHal.h"
#include "DisplayPostprocessing.h"
#include "../common/v4.0/ParamManager2.h"
#include <media/stagefright/foundation/ADebug.h>

#include "DisplayEffectPlatform.h"
#include "cust_define.h"

#define DEFAULT_EXPERT_GAMUT 1

#define LOG_TAG "DisplayFeatureHal"

using std::fstream;

namespace android {

DisplayEffectPlatform::DisplayEffectPlatform(unsigned int displayId)
            : DisplayEffectBase(displayId)
{
    DF_LOGV("DisplayEffectPlatform");
}

DisplayEffectPlatform::~DisplayEffectPlatform()
{
}

int DisplayEffectPlatform::HandleEyeCare(int modeId, int cookie)
{
    int ret = -1, value = 0;
    DF_LOGD("%s enter modeId %d", __func__, modeId);
    struct OutputParam sParams;
    Mutex::Autolock autoLock(mEffectLock);

    if (modeId < 0 || modeId > 255) {
        DF_LOGE("Invalid modeId %d", modeId);
        modeId = modeId < 0 ? 0 : 255;
    }
    if (getEffectStatus(DISPLAY_EYECARE)) {
        sParams.function_id = DISPLAY_EYECARE_MODE;
        sParams.param1 = modeId;
        ret = setDisplayEffect(SET_PCC_MODE, &sParams);

        setDispParam(0, EYECARE_STATE, modeId, cookie);
        if (modeId == 0) {
            clearEyeCareStatus();
            if (!GetHdrStatus()) {
                if (!mGameModeId)
                    restoreQcomMode();
            }
        }
        DF_LOGD("%s exit ret %d", __func__, ret);
        return ret;
    }

    if (modeId > 0) {
        sParams.function_id = DISPLAY_EYECARE_MODE;
        sParams.param1 = modeId;
        ret = setDisplayEffect(SET_PCC_MODE, &sParams);
        /* CABC implement by platform */
        ret |= HandleCabcModeCustom(DISPALY_CABC_MODE_DEFAULT,255);

        _setEffectStatus(0x1 << DISPLAY_EYECARE);
        setDispParam(0, EYECARE_STATE, modeId, cookie);
    }
    DF_LOGD("%s exit ret %d", __func__, ret);

    return ret;
}

int DisplayEffectPlatform::HandleCabcModeCustom(int modeId, int cookie)
{
    int ret = -1;
    int value = 0;
    struct OutputParam sParams;
    DF_LOGD("%s enter %d %d", __func__, modeId, cookie);
    if ((getEffectStatus() >> DISPLAY_COLOR_ENHANCE) & 0x3) {
        DF_LOGD("effect status 0x%x not support cabc on", getEffectStatus());
        if (mCABCstate > 0 ) {
           sParams.function_id = DISPLAY_CABC;
           sParams.param1 = CABC_OFF;
           ret = setDisplayEffect(SET_MTK_MODE, &sParams);
           mCABCstate = CABC_OFF;
        }
        return ret;
    }

    switch (modeId) {
        case DISPALY_CABC_OFF:
            value = CABC_OFF;
            break;
        case DISPALY_CABC_MODE_DEFAULT:/* Resume CABC mode */
        case DISPALY_CABC_MODE_GUI:
            value = CABC_UI_ON;
            break;
        case DISPALY_CABC_MODE_STILL:
            value = CABC_STILL_ON;
            break;
        case DISPALY_CABC_MODE_MOVIE:
            value = CABC_MOVIE_ON;
            break;
        default:
            DF_LOGE("%s: invalid CABC mode %d", __func__, modeId);
            return ret;
    }

    mCABCstate = value;
    sParams.function_id = DISPLAY_CABC;
    sParams.param1 = value;
    ret = setDisplayEffect(SET_MTK_MODE, &sParams);

    DF_LOGD("%s exit 0x%x, mCABCstate %d", __func__, value, mCABCstate);
    return ret;
}

int DisplayEffectPlatform::HandleAD(int modeId, int cookie)
{
    int ret = -1;
    struct OutputParam sParams;

	DF_LOGD("%s enter modeId:%d ,cookie:%d ", __func__, modeId, cookie);

    if ((modeId != DISPLAY_DRE) || (cookie != DRE_ENABLE && cookie != DRE_DISABLE))
        return ret;

    sParams.function_id = modeId;
    sParams.param1 = cookie;
    ret |= setDisplayEffect(SET_MTK_MODE, &sParams);

    return ret;
}

int DisplayEffectPlatform::HandleHDR(int modeId, int cookie)
{
    int ret = 0, status = 0;
    static int last_modeId;
    Mutex::Autolock autoLock(mHDRLock);
    struct OutputParam sParams;

    DF_LOGD("%s enter %d", __func__, modeId);

    DisplayUtil::onCallback(NOTIFY_HDR_STATE, modeId);

    if (modeId) {
        if (getEffectStatus(DISPLAY_EXPERT)) {
            HandleExpert(EXPERT_DISABLE, 0);
        }
        mHDRStatus |= 1 << HDR10;
        status = 0x1 << DISPLAY_HDR;
        setEffectStatus(status);

        if (modeId == 1) {
            sParams.function_id = DISPLAY_PQ_MODE;
            sParams.param1 = DISP_PQ_HDR ;
            ret |= setDisplayEffect(SET_MTK_MODE, &sParams);
            ret |= HandleC3dLut(C3D_LUT_MODE_P3_D65, C3D_LUT_SET);
        }
        sParams.function_id = DISPLAY_DRE;
        sParams.param1 = DRE_DISABLE;
        ret |= setDisplayEffect(SET_MTK_MODE, &sParams);
    } else {
        mHDRStatus &= ~(1 << HDR10);
        sParams.function_id = DISPLAY_DRE;
        sParams.param1 = DRE_ENABLE;
        ret |= setDisplayEffect(SET_MTK_MODE, &sParams);
        if (last_modeId != modeId) {
            restoreQcomMode();
        }
    }

    last_modeId = modeId;
    DF_LOGD("%s exit ret:%d", __func__, ret);
    return ret;
}

void DisplayEffectPlatform::HandleDVStatus(int enable, int cookie)
{
    DF_LOGD("%s enter enable %d", __func__, enable);
    DisplayUtil::onCallback(NOTIFY_DOLBY_STATE, enable);
    int value = enable ? 2 : 0;
    struct OutputParam sParams;
    if (enable) {
        sParams.function_id = DISPLAY_PQ_MODE;
        sParams.param1 = DISP_PQ_DV ;
        setDisplayEffect(SET_MTK_MODE, &sParams);
        HandleC3dLut(C3D_LUT_MODE_DV, C3D_LUT_SET);
    }
    HandleHDR(value, cookie);
}

int DisplayEffectPlatform::HandleDataBase(int value, int cookie)
{
    int ret = 0;
    mWCGCorlorGamut = value;
    struct OutputParam sParams;
    DF_LOGD("enable:%d ", value);
    if (mWCGState)  {
        if (DATASPACE_SRGB == value) {
            if (getEffectStatus(DISPLAY_EXPERT)) {
                sParams.function_id = DISPLAY_PQ_MODE;
                sParams.param1 = DISP_PQ_EXPERT_sRGB;
                ret |= setDisplayEffect(SET_MTK_MODE, &sParams);
                ret |= HandleC3dLut(C3D_LUT_MODE_NATURE_SRGB, C3D_LUT_SET);
            } else {
                sParams.function_id = DISPLAY_PQ_MODE;
                sParams.param1 = DISP_PQ_STANDARD;
                ret |= setDisplayEffect(SET_MTK_MODE, &sParams);
                ret |= HandleC3dLut(C3D_LUT_MODE_SRGB_D65, C3D_LUT_SET);
            }
        } else if (DATASPACE_P3 == value) {
            if (getEffectStatus(DISPLAY_EXPERT)) {
                sParams.function_id = DISPLAY_PQ_MODE;
                sParams.param1 = DISP_PQ_EXPERT_P3;
                ret |= setDisplayEffect(SET_MTK_MODE, &sParams);
                ret |= HandleC3dLut(C3D_LUT_MODE_NATURE_P3, C3D_LUT_SET);
            } else {
                sParams.function_id = DISPLAY_PQ_MODE;
                sParams.param1 = DISP_PQ_P3_D65;
                ret |= setDisplayEffect(SET_MTK_MODE, &sParams);
                ret |= HandleC3dLut(C3D_LUT_MODE_P3_D65, C3D_LUT_SET);
            }
        }
    }
    DF_LOGD("exit  mWCGCorlorGamut = %d", mWCGCorlorGamut);

    return ret;
}

int DisplayEffectPlatform::HandleCameraRequest(int modeId, int cookie)
{
    int ret = -1, status = 0, value = 0;
    struct OutputParam sParams;

    DF_LOGD("enter value %d", modeId);
    if (modeId) {
        sParams.function_id = DISPLAY_PQ_MODE;
        sParams.param1 = DISP_PQ_DEFAULT;
        ret = setDisplayEffect(SET_MTK_MODE, &sParams);

        sParams.function_id = DISPLAY_CLEAR_PCC;
        ret |= setDisplayEffect(SET_PCC_MODE, &sParams);
        if (getEffectStatus(DISPLAY_EXPERT))
            ret |= HandleExpert(EXPERT_DISABLE, 0);
        status = 0x1 << DISPLAY_CAMERA;
        setEffectStatus(status);
    }
    else {
        restoreQcomMode();
    }
    DF_LOGD("exit");
    return ret;
}

int DisplayEffectPlatform::HandleGameMode(int modeId, int cookie)
{
    int ret = -1, status = 0, value = 0;
    struct OutputParam sParams;
    static int last_mode = -1;

    last_mode = mGameModeId;
    mGameModeId = modeId;

    DF_LOGD("enter, modeId %d, last_mode %d", modeId, last_mode);

    if (getEffectStatus(DISPLAY_EXPERT)) {
        if (modeId) {
            if (last_mode == 0)
                HandleExpert(EXPERT_DISABLE, 0);
        } else {
            restoreQcomMode();
        }
    }

    if (property_get_int32("ro.vendor.pq.mtk_disp_game_pq_support", 0)) {
        sParams.function_id = DISPLAY_GAME_PQ_MODE;
    // pq will restore when pq index < game_1 in postprocessing(modeId == 0).
        sParams.param1 = DISP_PQ_GAME_1 + modeId - 1;
        ret = setDisplayEffect(SET_MTK_MODE, &sParams);
    } else {
        if (modeId >= 2) {
            value = mDRELux << 16 | 1;
            sParams.function_id = DISPLAY_DRE;
            sParams.param1 = value;
            ret = setDisplayEffect(SET_MTK_MODE, &sParams);
        } else {
            value =  1;
            sParams.function_id = DISPLAY_DRE;
            sParams.param1 = value;
            ret = setDisplayEffect(SET_MTK_MODE, &sParams);
        }
        if (modeId) {
            sParams.function_id = DISPLAY_PQ_MODE;
            sParams.param1 = DISP_PQ_GAME_1 + modeId - 1;
            ret = setDisplayEffect(SET_MTK_MODE, &sParams);
        } else  {
            if (last_mode > 0)
                restoreQcomMode();
        }
    }

    DF_LOGD("exit");
    return ret;
}

int DisplayEffectPlatform::Handle10Bit(int modeId, int cookie)
{
    int ret = 0;
    struct OutputParam sParams;
    int brightness = 0;
    char BLValue[10];
    DF_LOGD("%s enter %d, the mActiveAppId: %d", __func__, modeId, curApp);
    if (modeId) {
        m10BitLayer = 1;
        if (curApp == IQIYI) {
            mHDRStatus |= 1 << CUVA_HDR;
            sParams.function_id = DISPLAY_PQ_MODE;
            sParams.param1 = DISP_PQ_HDR ;
            ret |= setDisplayEffect(SET_MTK_MODE, &sParams);
            ret |= HandleC3dLut(C3D_LUT_MODE_P3_D65, C3D_LUT_SET);
        }
    } else {
        m10BitLayer = 0;
        if ((mHDRStatus >> CUVA_HDR) & 0x1) {
            mHDRStatus &= ~(1 << CUVA_HDR);
            if (!mHDRStatus)
                restoreQcomMode();
        }
    }
    return ret;
}

int DisplayEffectPlatform::onSuspend()
{
    int ret = -1;
    int value = 0;
    struct OutputParam sParams;

    DF_LOGD("enter");

    sParams.function_id = DISPLAY_DRE;
    sParams.param1 = DRE_DISABLE;
    ret = setDisplayEffect(SET_MTK_MODE, &sParams);

    mSuspendFlag = 1;

    return ret;
}

int DisplayEffectPlatform::onResume()
{
    int ret = 0, value = 0;
    struct OutputParam sParams;
    DF_LOGD("enter");

    restoreQcomMode();
    sp<AMessage> msg = new AMessage(DisplayEffectPlatform::kDISPLAY_BRIGHTNESS_NOTIFY, this);
    msg->setInt32("Brightness", -1);
    msg->setInt32("VirtualBl", -1);
    msg->post();

    sParams.function_id = DISPLAY_DRE;
    sParams.param1 = DRE_ENABLE;
    ret = setDisplayEffect(SET_MTK_MODE, &sParams);

    return ret;
}

} // namespace android

