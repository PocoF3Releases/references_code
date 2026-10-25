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
#include <utils/SortedVector.h>
#include <utils/String8.h>
#include <sys/prctl.h>
#include <errno.h>
#include <assert.h>
#include <media/stagefright/foundation/ADebug.h>
#include <binder/Parcel.h>
#include <cutils/properties.h>
#include <fstream>

#include <display_property.h>
#include <DisplayFeatureHal.h>
#include <XmlParser.h>
#include <ParamManager2.h>
#include "DisplayEffect.h"
#include "DisplayEffectBase.h"
#include "DFComDefine.h"
#include <mi_disp.h>

#define LOG_TAG "DisplayFeatureHal"

static android::DisplayFeatureHal::DFModuleMethods g_df_module_methods;

df_module_t HAL_MODULE_INFO_SYM = {
  .common = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = DISPLAYFEATURE_HARDWARE_MODULE_ID,
    .name = "XIAOMI Hardware Display Feature Module",
    .author = "Xiaomi",
    .methods = &g_df_module_methods,
    .dso = 0,
    .reserved = {0},
  }
};

using std::fstream;

namespace android {
// ---------------------------------------------------------------------------
int DisplayFeatureHal::mDisplayNum = 0;
Mutex DisplayFeatureHal::mLock;
int DFLOG::loglevel = 1;

DisplayFeatureHal::DisplayFeatureHal(const hw_module_t *module)
{
    df_device_t::common.tag = HARDWARE_DEVICE_TAG;
    df_device_t::common.version = DF_MODULE_API_VERSION_0_1;
    df_device_t::common.module = const_cast<hw_module_t *>(module);
    df_device_t::common.close = close;
    df_device_t::getCapabilities = getCapabilities;
    df_device_t::getFunction = getFunction;
    DF_LOGV("DisplayFeatureHal 0x%x", df_device_t::common.version);
}

DisplayFeatureHal::~DisplayFeatureHal()
{
    DF_LOGV("~DisplayFeatureHal");
}

int DisplayFeatureHal::setFunctionEnable(struct df_device* device,
    int displayId, int caseId, int modeId, int cookie)
{
    Mutex::Autolock autoLock(mLock);
    int ret = 0;

    if (displayId >= mDisplayNum || displayId < 0) {
        return ret;
    }

    DisplayFeatureHal *displayFeatureHal = static_cast<DisplayFeatureHal *>(device);
    sp<DisplayEffect> df = static_cast<DisplayEffect *>(displayFeatureHal->mDisplayEffect[displayId].get());

    DF_LOGD("enter");

    if (!property_get_bool(SYS_DISPLAYFEATURE_ENTRY_ENABLE, false))
        return ret;

    if (caseId == SET_OPTION) {
        ret = df->handleDispSwitchOption(modeId, cookie);
        DF_LOGV("exit ret %d", ret);
        return ret;
    }

    struct OutputParam sParams;
    sParams.function_id = modeId;
    sParams.param1 = cookie;
    ret = df->featureEnablePolicy(caseId, &sParams);
    if (ret != 0) {
        DF_LOGE("SET_MODE fail value: 0x%x ret: %d", modeId, ret);
        return ret;
    }
    DF_LOGD("exit ret %d", ret);

    return ret;
}

bool DisplayFeatureHal::checkDeviceState(struct df_device* device, int displayId)
{
    sp<DisplayEffect> df;
    DisplayFeatureHal *displayFeatureHal = static_cast<DisplayFeatureHal *>(device);
    df = static_cast<DisplayEffect *>(displayFeatureHal->mDisplayEffect[displayId].get());
    if (df->getDisplayState() == kStateOn) {
        return true;
    }
    return false;
}

bool DisplayFeatureHal::setFeatureStatusCheck(struct df_device* device,
    int displayId, int caseId, int modeId, int cookie)
{
    ParamManager pmMgr;
    int ret = 0;
    struct OutputParam sParams;

    if (displayId >= mDisplayNum || displayId < 0) {
        return ret;
    }

    DisplayFeatureHal *displayFeatureHal = static_cast<DisplayFeatureHal *>(device);
    sp<DisplayEffect> df = static_cast<DisplayEffect *>(displayFeatureHal->mDisplayEffect[displayId].get());

    if (caseId == NOTIFY_DISPLAY_STATE || caseId == TOUCH_STATE ||
            caseId == ORITATION_STATE||caseId == CUP_BLACK_COVERED_STATE)
        return true;

    if (caseId == DOZE_BRIGHTNESS_STATE) {
        df->handleDozeBrightnessMode(modeId);
        return false;
    }

    bool wcgEnable = checkWCGEnable(caseId, modeId, cookie);
    df->setWCGState(wcgEnable);
    notifySFWcgState(wcgEnable);

    /* FIXME:need to remove to other place */
    if (!df->mRead) {
        ret = df->parseXml();
        if (ret == 0)
            df->mRead = 1;
#ifdef MTK_PLATFORM
        df->HandleAD(DISPLAY_DRE, DRE_ENABLE);
#else
        df->HandleAD(LTM_ON, 0);
#endif
        DisplayUtil::onCallback(40000, 0);
        DisplayUtil::setDCParseCallbackEnable(false);
        if (property_get_bool(PERSIST_DISPLAYFEATURE_DC_BL_ENABLE, false)) {
            int generation = 0;
            generation = df->GetMessageGeneration(DC_BACKLIGHT_STATE) + 1;
            df->SetMessageGeneration(DC_BACKLIGHT_STATE, generation);
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_DC_BACKLIGHT, df);
            msg->setInt32("Generation", generation);
            msg->setInt32("CaseId", DC_BACKLIGHT_STATE);
            msg->setInt32("Value", 1);
            msg->setInt32("Cookie", cookie);
            msg->post();
        }
        df->restoreExpertParam();
    }

    return true;
}

int DisplayFeatureHal::setFeatureEnable(struct df_device* device,
    int displayId, int caseId, int modeId, int cookie)
{
    Mutex::Autolock autoLock(mLock);
    int ret = -1;
    ParamManager pmMgr;

    if (displayId >= mDisplayNum || displayId < 0) {
        return ret;
    }

    if (caseId != TOUCH_STATE)
        DF_LOGD("displayId = %d, FeatureId=%s modeId=%d cookie=%d",
             displayId, getFeatureIdName(caseId), modeId, cookie);

    DisplayFeatureHal *displayFeatureHal = static_cast<DisplayFeatureHal *>(device);
    sp<DisplayEffect> df = static_cast<DisplayEffect *>(displayFeatureHal->mDisplayEffect[displayId].get());
    if (!setFeatureStatusCheck(device, displayId, caseId, modeId, cookie))
        return 0;

    switch (caseId) {
        case AUTOMATIC_CONTRAST_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_ENV_ADAPTIVE_NORMAL, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case INCREASED_CONTRAST_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_COLOR_ENHANCE, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case STANDARD_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_STANDARD, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case EYECARE_STATE: {
            int generation = 0;
            generation = df->GetMessageGeneration(caseId) + 1;
            df->SetMessageGeneration(caseId, generation);
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_EYECARE, df);
            msg->setInt32("Generation", generation);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case COLORTEMPERATURE_ADJUST_STATE: {
            DF_LOGV("param caseId: %d modeId:%d cookie: %d",
            caseId, modeId, cookie);
        } break;
        case KEEP_WHITE_POINT_SRGB_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_KEEP_WP_SRGB, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case CABC_MODE_CUSTOM_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_CABC_MODE_SWITCH, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case AD_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_AD, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case NIGHT_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_NIGHT_MODE, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case HDR_STATE: {
            int generation = 0;
            generation = df->GetMessageGeneration(caseId) + 1;
            df->SetMessageGeneration(caseId, generation);
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_HDR, df);
            msg->setInt32("Generation", generation);
            msg->setInt32("CaseId", caseId);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case HBM_STATE: {
            /* hbm here only be used by fod, then we ignore actual cookie */
/* need fix*/
#if 0
            if (cookie != 3 && cookie != 4 && cookie != 5)
                return 0;

            if (cookie == 3) {
                ret = df->HandleHBM(modeId, 0);
            }

            if (cookie == 4) {
                ret = df->HandlePreHBM();
            }

            if (cookie == 5) {
                ret = df->HandleBacklightFOD(modeId, 0);
            }
#endif
        } break;
        case EXT_COLOR_PROC_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_EXT_COLOR_PROC, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case FOD_SCREEN_ON_STATE: {
            //ret = df->handleFodScreenOn(modeId);
        } break;
        case FOD_MONITOR_ON_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_FOD_SCREEN_ON, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case CAMERA_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_CAMERA, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case BCBC_CUST_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_BCBC_CUSTOM, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case GAME_STATE: {
            for (int i = 0; i < mDisplayNum; i++) {
                df = static_cast<DisplayEffect *>(displayFeatureHal->mDisplayEffect[i].get());
                sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_GAME, df);
                msg->setInt32("Value", modeId);
                msg->setInt32("Cookie", cookie);
                msg->post();
            }
        } break;
        case SUNLIGHT_SCREEN_STATE: {
            int generation = 0;
            generation = df->GetMessageGeneration(caseId) + 1;
            df->SetMessageGeneration(caseId, generation);
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_SUNLIGHT_SCREEN, df);
            msg->setInt32("Generation", generation);
            msg->setInt32("CaseId", caseId);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case DC_BACKLIGHT_STATE: {
            int generation = 0;
            generation = df->GetMessageGeneration(caseId) + 1;
            df->SetMessageGeneration(caseId, generation);
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_DC_BACKLIGHT, df);
            msg->setInt32("Generation", generation);
            msg->setInt32("CaseId", caseId);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case COLOR_TEMP_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_COLOR_TEMPERATURE, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case FPS_SWITCH_STATE: {
            df->HandleFpsSwitch(modeId, cookie);
        } break;
        case EXPERT_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_EXPERT, df);
            msg->setInt32("Case", cookie);
            msg->setInt32("Value", modeId);
            msg->post();
        } break;
        case VIDEO_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_VIDEO, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case NOTIFY_BRIGHTNESS_STATE: {
            DF_LOGV("BRIGHTNESS notify value=%d", modeId);
            if (df->GetHdrStatus()) {
                df->setBrightness(modeId);
                return ret;
            }

            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_BRIGHTNESS_NOTIFY, df);
            msg->setInt32("Brightness", modeId);
            msg->setInt32("VirtualBl", 0);
            msg->post();
        } break;
        case NOTIFY_DISPLAY_STATE: {
            DF_LOGV("Display state notify value=%d", modeId);

            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_STATE_NOTIFY, df);
            msg->setInt32("state", modeId);
            msg->post();

        } break;
        case PAPER_MODE_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_PAPER_MODE, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case TRUE_TONE_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_TRUE_TONE, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case FOD_LOWBRIGHTNESS_FOD_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kLOW_BRIGHTNESS_FOD, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case DISPLAY_SCENE_STATE: {
            DF_LOGV("Display scene value=%d", modeId);
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_SCENE, df);
            msg->setInt32("modeId", modeId);
            msg->setInt32("cookie", cookie);
            msg->post();
        } break;
        case ACTIVE_APP_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_ACTIVE_APP, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case DARK_MODE_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDARK_MODE, df);
            msg->setInt32("modeId", modeId);
            msg->setInt32("cookie", cookie);
            msg->post();
        } break;
        case ADAPTIVE_HDR: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kADAPTIVE_HDR, df);
            msg->setInt32("modeId", modeId);
            msg->setInt32("cookie", cookie);
            msg->post();
        } break;
        case DITHER_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_DITHER_MODE, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case TOUCH_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_TOUCH_STATE, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case ORITATION_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_ORITATION_STATE, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case CUP_BLACK_COVERED_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_CUP_BLACK_COVERED_STATE, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case DOLBY_VISION_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_DOLBY_VISION_STATE, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case DATABASE_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_DATABASE, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case PICTURE_HDR_STATE: {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_PIC_HDR, df);
            msg->setInt32("Value", modeId);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        default: {
            DF_LOGE("param error caseId: %d modeId: %d cookie: %d",
                caseId, modeId, cookie);
        } break;
    }

    return ret;
}

void DisplayFeatureHal::sendMessage(struct df_device* device,
    int index, int value, const string& cmd)
{
    int displayId = 0;
    int cookie = 0;
    displayId = index >> 16;
    index &= 0xFF;

    if (displayId >= mDisplayNum || displayId < 0)  {
        DF_LOGE("invalid displayId = %d, mDisplayNum(%d)",
            displayId, mDisplayNum);
        return;
    }
    DF_LOGD("displayId = %d, FeatureId=%s modeId=%d",
             displayId, getFeatureIdName(index), value);

    DisplayFeatureHal *displayFeatureHal = static_cast<DisplayFeatureHal *>(device);
    sp<DisplayEffect> df = static_cast<DisplayEffect *>(displayFeatureHal->mDisplayEffect[displayId].get());
    switch (index) {
        case HDR_STATE: {
            int generation = 0;
            generation = df->GetMessageGeneration(index) + 1;
            df->SetMessageGeneration(index, generation);
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_HDR, df);
            msg->setInt32("Generation", generation);
            msg->setInt32("CaseId", index);
            msg->setInt32("Value", value);
            msg->setInt32("Cookie", cookie);
            msg->post();
        } break;
        case NOTIFY_DISPLAY_STATE: {
            DF_LOGD("Display state notify value=%d", value);
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_STATE_NOTIFY, df);
            msg->setInt32("state", value);
            msg->post();

            for (int i = 0; i < mDisplayNum; i++) {
                if (i == displayId) {
                    continue;
                }

                sp<DisplayEffect> spDispEffect = static_cast<DisplayEffect *>(displayFeatureHal->mDisplayEffect[i].get());
                sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_STATE_NOTIFY_MUTUAL, spDispEffect);
                msg->setInt32("display", displayId);
                msg->setInt32("state", value);
                msg->post();
            }
        } break;
        case NOTIFY_10BIT_STATE:
        {
            int generation = 0;
            generation = df->GetMessageGeneration(index) + 1;
            df->SetMessageGeneration(index, generation);
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_10BIT, df);
            msg->setInt32("Generation", generation);
            msg->setInt32("CaseId", index);
            msg->setInt32("Value", value);
            msg->setInt32("Cookie", 0);
            msg->post();
        } break;
        case DITHER_STATE:
        {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_DITHER_MODE, df);
            msg->setInt32("Value", value);
            msg->setInt32("Cookie", 0);
            msg->post();
        } break;
        case FRAME_REFRESH_STATE:
        default:
            break;
    }
}
/*JDL need confirm*/
void DisplayFeatureHal::setListener(struct df_device* device, NOTIFY_CHANGED listener)
{
    DisplayFeatureHal *displayFeatureHal = static_cast<DisplayFeatureHal *>(device);
    sp<DisplayEffect> df = static_cast<DisplayEffect *>(displayFeatureHal->mDisplayEffect[DISPLAY_PRIMARY].get());

    DF_LOGD("set listener: 0x%p", listener);

    sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_SET_CALLBACK, df);
    msg->setPointer("callback", (void *)listener);
    msg->post();
}

void DisplayFeatureHal::dump(struct df_device* device, std::string& result)
{
    DisplayFeatureHal *displayFeatureHal = static_cast<DisplayFeatureHal *>(device);
    sp<DisplayEffect> df = static_cast<DisplayEffect *>(displayFeatureHal->mDisplayEffect[DISPLAY_PRIMARY].get());

    for(int i = 0; i < mDisplayNum; i++) {
        df = static_cast<DisplayEffect *>(displayFeatureHal->mDisplayEffect[i].get());
        df->dumpInfo(result);
   }
}

void DisplayFeatureHal::notifySFWcgState(bool enable)
{
    DF_LOGD("enable = %d", enable);
    DisplayUtil::onCallback(10000, enable);/* JDL need confirm*/
}

bool DisplayFeatureHal::checkModeForWCG(int caseId, int modeId)
{
  bool ret = false;

  switch (caseId) {
    case NOTIFY_BRIGHTNESS_STATE:
    case KEEP_WHITE_POINT_SRGB_STATE:
    case CABC_MODE_CUSTOM_STATE:
    case AD_STATE:
    case HDR_STATE:
    case HBM_STATE:
    case NOTIFY_DISPLAY_STATE:
    case EXT_COLOR_PROC_STATE:
    case FOD_SCREEN_ON_STATE:
    case NIGHT_STATE:
    case BCBC_CUST_STATE:
    case EYECARE_STATE:
    case DC_BACKLIGHT_STATE:
    case FOD_MONITOR_ON_STATE:
    case COLOR_TEMP_STATE:
    case GAME_STATE:
    case DFPS_STATE:
    case FPS_SWITCH_STATE:
    case DOZE_BRIGHTNESS_STATE:
    case FOD_LOWBRIGHTNESS_FOD_STATE:
    case VIDEO_STATE:
    case ACTIVE_APP_STATE:
    case SMART_DFPS_STATE:
    case DUMP_STATE:
    case PAPER_MODE_STATE:
    case TRUE_TONE_STATE:
    case NOTIFY_10BIT_STATE:
    case DISPLAY_SCENE_STATE:
    case DARK_MODE_STATE:
    case DITHER_STATE:
    case TOUCH_STATE:
    case ORITATION_STATE:
    case CUP_BLACK_COVERED_STATE:
    case DATABASE_STATE:
      ret = true;
      break;
    case CAMERA_STATE:
      if (modeId == 1)
        ret = false;
      if (modeId == 0)
        ret = true;
      break;
    default:
      ret = false;
      break;
  }
  return ret;
}

bool DisplayFeatureHal::checkWCGEnable(int caseId, int modeId, int cookie)
{
    static bool wcg_enable = false;
    ParamManager pmMgr;

    if (caseId == STANDARD_STATE || caseId == HDR_STATE ||
        (wcg_enable && caseId != EXPERT_STATE && checkModeForWCG(caseId, modeId))) {
        wcg_enable = true;
    } else if (caseId == EXPERT_STATE) {
        if (cookie == EXPERT_GAMUT) {
            if (modeId == 1)
                wcg_enable = true;
            else
                wcg_enable = false;
        } else if (cookie == EXPERT_RESTORE) {
            if (pmMgr.getParam(pmMgr.ExpertNode[EXPERT_NODE_NAME].c_str(),
                pmMgr.ExpertNode[EXPERT_NODE_GAMUT].c_str()) == 1)
                wcg_enable = true;
            else
                wcg_enable = false;
        }
    } else {
        wcg_enable = false;
    }

    return wcg_enable;
}

int DisplayFeatureHal::deinit()
{
    DF_LOGV("DisplayFeatureHal::deinit");

    return 0;
}

int DisplayFeatureHal::init()
{
    unsigned int i = 0;
    DF_LOGV("DisplayFeatureHal::init");

    mDisplayNum = 0;
    for (i = 0; i < DISPLAY_MAX; i++) {
        if (i == DISPLAY_SECONDARY && access(DISP1_DEV_PATH, F_OK)) {
            DF_LOGD("DisplayFeatureHal doesn't support secondary display");
            return 0;
        }

        mDisplayEffect[i] = new DisplayEffect(i);
        sp<DisplayEffect> df = static_cast<DisplayEffect *>(mDisplayEffect[i].get());
        df->start();
        mDisplayNum ++;
    }

    return 0;
}

int DisplayFeatureHal::open(const hw_module_t *module, const char *name, hw_device_t **device)
{
    Mutex::Autolock autoLock(mLock);
    DF_LOGV("DisplayFeatureHal::open %s", name);

    if (!module || !name || !device) {
        DF_LOGE("Invalid parameters.");
        return -EINVAL;
    }

    if (!strcmp(name, DF_DISPLAYFEATURE)) {
        DisplayFeatureHal *df_session = new DisplayFeatureHal(module);
        if (!df_session) {
            return -ENOMEM;
        }

        int status = df_session->init();
        if (status != 0) {
            return status;
        }

        df_device_t *df_device = df_session;
        *device = reinterpret_cast<hw_device_t *>(df_device);
    }

    return 0;
}

int DisplayFeatureHal::close(hw_device_t *device)
{
    Mutex::Autolock autoLock(mLock);

    if (!device) {
        return -EINVAL;
    }

    df_device_t *df_device = reinterpret_cast<df_device_t *>(device);
    DisplayFeatureHal *df_session = static_cast<DisplayFeatureHal *>(df_device);

    df_session->deinit();

    return 0;
}

void DisplayFeatureHal::getCapabilities(struct df_device *device, uint32_t *outCount,
                                 int32_t *outCapabilities)
{

}

template <typename PFN, typename T>
static df_function_pointer_t AsFP(T function) {
  static_assert(std::is_same<PFN, T>::value, "Incompatible function pointer");
  return reinterpret_cast<df_function_pointer_t>(function);
}

df_function_pointer_t DisplayFeatureHal::getFunction(struct df_device *device,
                                                int32_t int_descriptor)
{
  auto descriptor = static_cast<FunctionDescriptor>(int_descriptor);
  switch (descriptor) {
      case FunctionDescriptor::SetFeature:
        return AsFP<DF_PFN_SET_FEATURE>(DisplayFeatureHal::setFeatureEnable);
      case FunctionDescriptor::SetFunction:
        return AsFP<DF_PFN_SET_FUNCTION>(DisplayFeatureHal::setFunctionEnable);
      case FunctionDescriptor::SendMessage:
        return AsFP<DF_PFN_SEND_MESSAGE>(DisplayFeatureHal::sendMessage);
      case FunctionDescriptor::SetListener:
        return AsFP<DF_PFN_SET_LISTENER>(DisplayFeatureHal::setListener);
      case FunctionDescriptor::Dump:
        return AsFP<DF_PFN_DUMP>(DisplayFeatureHal::dump);
      default:
          DF_LOGE("Unknown/Unimplemented function descriptor: %d (%s)", int_descriptor,
                     to_string(descriptor).c_str());
        return nullptr;
  }
  return nullptr;
}
}
