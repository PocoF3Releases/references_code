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
#include <utils/threads.h>
#include <sys/prctl.h>

#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <cutils/properties.h>

#include "DisplayEffect.h"
#include "DisplayFeatureService.h"
#include "display_property.h"

#ifdef HAS_DISPLAYFEATURE_HAL
#include <vendor/xiaomi/hardware/displayfeature/1.0/IDisplayFeature.h>
#include <vendor/xiaomi/hardware/displayfeature/1.0/types.h>
#endif

#define LOG_TAG "DisplayFeatureService"

#ifdef HAS_DISPLAYFEATURE_HAL
using ::vendor::xiaomi::hardware::displayfeature::V1_0::IDisplayFeature;
using ::vendor::xiaomi::hardware::displayfeature::V1_0::Status;
#endif
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_version;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_handle;

namespace android {

static unsigned int eyecare_threshold;
static unsigned int eyecare_level;

char * printFeatureMode(int fmode)
{
    switch (fmode) {
        case SET_AD:
            return "SET_AD";
        case SET_QUALCOMM_MODE:
            return "SET_QUALCOMM_MODE";
        case SET_PCC_MODE:
            return "SET_PCC_MODE";
        case SET_PANEL_MODE:
            return "SET_PANEL_MODE";
        case SET_SVI:
            return "SET_SVI";
    }
    return NULL;
}
// ---------------------------------------------------------------------------

DisplayFeatureService::DisplayFeatureService()
    :mCurBL(0)
{
    eyecare_threshold = property_get_int32(RO_EYECARE_BRIGHTNESS_THRESHOLD, 7);
    eyecare_level = property_get_int32(RO_EYECARE_BRIGHTNESS_LEVEL, 2);
    ALOGD("%d %d", eyecare_threshold, eyecare_level);
}

DisplayFeatureService::~DisplayFeatureService()
{

}

void DisplayFeatureService::instantiate()
{
    defaultServiceManager()->addService(
            String16("DisplayFeatureControl"), new DisplayFeatureService());
    ALOGI("add DisplayFeatureControl into ServiceManager");
}

int DisplayFeatureService::setFunctionEnable(int displayId, int caseId,
    int modeId, int cookie)
{
    Mutex::Autolock autoLock(mLock);
    int ret = 0;
    DisplayEffect &df = DisplayEffect::getInstance();

    ALOGD("%s enter", __func__);

    if (!property_get_bool(SYS_DISPLAYFEATURE_ENTRY_ENABLE, false))
        return ret;

    if (caseId == SET_OPTION) {
        ret = df.handleDispSwitchOption(modeId, cookie);
        ALOGD("%s exit ret %d", __func__, ret);
        return ret;
    }
#ifdef HAS_DISPLAYFEATURE_HAL
    static sp<IDisplayFeature> sClientDisplayFeature = NULL;
    if(sClientDisplayFeature == NULL) {
        sClientDisplayFeature = IDisplayFeature::getService();
        if(sClientDisplayFeature == NULL) {
            ALOGE("Query DisplayFeature service returned null!");
            return -1;
        }
    }
    Return<Status> tmpReturn = sClientDisplayFeature->setFunction(displayId,
                caseId, modeId, cookie);
    if (tmpReturn != Status::OK) {
        ALOGD("set function failure!");
        ret = -1;
    }
#endif
    ALOGD("%s exit ret %d", __func__, ret);
    return ret;
}

int DisplayFeatureService::setFeatureEnable(int displayId, int caseId, int modeId, int cookie)
{
    Mutex::Autolock autoLock(mLock);
    int ret = 0;

    ALOGD("setFeatureEnable(displayId=%d caseId=%d modeId=%d cookie=%d)",
        displayId, caseId, modeId, cookie);
    DisplayEffect &df = DisplayEffect::getInstance();
    df.set_displayId(displayId);
    ALOGD("setFeatureEnable(displayId=%d )", df.get_displayId());

    switch (caseId) {
        case AUTOMATIC_CONTRAST_STATE: {
            if (modeId == AUTOMATIC_CONTRAST_NORMAL) {
                ret = df.HandleACNormal(cookie, getBrightness());
                ALOGD("caseId %d modeId %d %d", caseId, modeId, DISPLAY_ENV_ADAPTIVE_NORMAL);
            } else if (modeId == AUTOMATIC_CONTRAST_WARM) {
                ret = df.HandleACWarm(cookie, getBrightness());
                ALOGD("caseId %d modeId %d %d", caseId, modeId, DISPLAY_ENV_ADAPTIVE_WARM);
            } else if (modeId == AUTOMATIC_CONTRAST_COLD) {
                ret = df.HandleACCold(cookie, getBrightness());
                ALOGD("caseId %d modeId %d %d", caseId, modeId, DISPLAY_ENV_ADAPTIVE_COLD);
            } else {
                ALOGE("%s Error param modeId: %d", __func__, modeId);
            }
        } break;
        case INCREASED_CONTRAST_STATE: {
            ret = df.HandleIncreasedContrast(cookie);
        } break;
        case STANDARD_STATE: {
            ret = df.HandleStandard(cookie);
        } break;
        case EYECARE_STATE: {
            ret = df.HandleEyeCare(modeId, cookie);
        } break;
        case COLORTEMPERATURE_ADJUST_STATE: {
            ALOGD("%s param caseId: %d modeId:%d cookie: %d",
            __func__, caseId, modeId, cookie);
        } break;
        case KEEP_WHITE_POINT_SRGB_STATE: {
            ret = df.HandleKeepWhitePointSRGB(modeId, cookie);
        } break;
        case CABC_MODE_CUSTOM_STATE: {
            ret = df.HandleCabcModeCustom(modeId, cookie);
        } break;
        case AD_STATE: {
            ret = df.HandleAD(modeId, cookie);
        } break;
        case SUNLIGHT_SCREEN_STATE: {
            ret = df.HandleSunlightScreen(modeId, cookie);
        } break;
        case NIGHT_STATE: {
            ret = df.HandleNightMode(modeId, cookie);
        } break;
        case HBM_STATE: {
            ret = df.HandleHBM(modeId, cookie);
        } break;
        case CAMERA_STATE: {
            ret = df.HandleCameraRequest(modeId, cookie);
        } break;
        case GAME_STATE: {
            ret = df.HandleGameMode(modeId, cookie);
        } break;
        case DC_BACKLIGHT_STATE: {
            ret = df.HandleDCBacklight(modeId, cookie);
        } break;
        case FPS_SWITCH_STATE: {
            ret = df.HandleFpsSwitch(modeId, cookie);
        } break;
        case DOZE_BRIGHTNESS_STATE: {
            ret = df.handleDozeBrightnessMode(modeId, cookie);
        } break;
        case EXPERT_STATE: {
            ret = df.HandleExpert(modeId, cookie);
        } break;
        case VIDEO_STATE: {
            ret = df.HandleVideoMode(modeId, cookie);
        } break;
        case ACTIVE_APP_STATE: {
            ret = df.HandleActiveApp(modeId, cookie);
        } break;
        case SMART_DFPS_STATE: {
            ret = df.HandleSmartDfpsState(modeId, cookie);
        } break;
        case DUMP_STATE: {
            ret = df.HandleFrameDump(modeId, cookie);
        } break;
        case PAPER_MODE_STATE: {
            ret = df.HandlePaperMode(modeId, cookie);
        } break;
        case TRUE_TONE_STATE: {
            ret = df.HandleTrueTone(modeId, cookie);
        } break;
        case COLOR_TEMP_STATE: {
            ret = df.HandleColorTempMode(modeId, cookie);
        } break;
        case BCBC_CUST_STATE: {
            ret = df.HandleBCBC(modeId, cookie);
        } break;
        case DISPLAY_SCENE_STATE: {
            ret = df.HandleDisplayScene(modeId, cookie);
        } break;
        case DARK_MODE_STATE: {
            ret = df.HandleDarkMode(modeId, cookie);
        } break;
        case ADAPTIVE_HDR: {
            ret = df.HandleAdaptiveHDR(modeId, cookie);
        } break;
        case DITHER_STATE: {
            ret = df.HandleDither(modeId, cookie);
        } break;
        case TOUCH_STATE: {
            ret = df.HandleTouchState(modeId, cookie);
        } break;
        case ORITATION_STATE: {
            ret = df.HandleOritationState(modeId, cookie);
        } break;
        case CUP_BLACK_COVERED_STATE: {
            ret = df.HandleCupBlackCoveredState(modeId, cookie);
        } break;
        case DOLBY_VISION_STATE: {
            ret = df.HandleDVStatus(modeId, cookie);
        } break;
         case PICTURE_HDR_STATE: {
            ret = df.HandlePicHDR(modeId, cookie);
        } break;
        default: {
            ALOGE("%s param error caseId: %d modeId: %d cookie: %d",
                __func__, caseId, modeId, cookie);
        } break;
    }

    return ret;
}

void DisplayFeatureService::notify(int displayId, int caseId, int modeId, int cookie)
{
    switch (caseId) {
        case DIMMING_BRIGHTNESS_STATE: {
          ALOGD("notify modeId=%d", modeId);
          DisplayEffect &df = DisplayEffect::getInstance();
          setBrightness(modeId);
          df.NotifyBrightness(modeId);
        } break;
    }
}

void DisplayFeatureService::sendMessage(int caseId, int modeId, int cookie)
{
#ifdef HAS_DISPLAYFEATURE_HAL
    static sp<IDisplayFeature> sClientDisplayFeature = NULL;
    if(sClientDisplayFeature == NULL) {
        sClientDisplayFeature = IDisplayFeature::getService();
        if(sClientDisplayFeature == NULL) {
            ALOGE("Query DisplayFeature service returned null!");
            return;
        }
    }
    const hidl_string notify_str("displayfeatureservice");
    sClientDisplayFeature->sendMessage(caseId, modeId, notify_str);
#endif
}

void DisplayFeatureService::onMessageReceived(int what)
{
    DisplayEffect &df = DisplayEffect::getInstance();
    switch (what) {
        case DISPLAY_ENV_ADAPTIVE_NORMAL: {
            df.HandleACNormal(0, getBrightness());
        } break;
        case DISPLAY_ENV_ADAPTIVE_COLD: {
            df.HandleACCold(0, getBrightness());
        } break;
        case DISPLAY_ENV_ADAPTIVE_WARM: {
            df.HandleACWarm(0, getBrightness());
        } break;
        case DISPLAY_COLOR_ENHANCE: {
            df.HandleIncreasedContrast(0);
        } break;
        case DISPLAY_STANDARD: {
            df.HandleStandard(0);
        } break;
        case DISPLAY_BRIGHTNESS_NOTIFY: {
            ALOGD("Handle BL = %d",getBrightness());
            df.HandleBrightnessInteract(getBrightness(),0);
        } break;
    }
}

void DisplayFeatureService::onMessageReceivedExt(int what, int value, int cookie)
{
    DisplayEffect &df = DisplayEffect::getInstance();
    switch (what) {
        case DISPLAY_EYECARE: {
            df.HandleEyeCare(value, cookie);
        } break;
        case DISPLAY_COLOR_TEMPERATURE: {
        } break;
        case DISPLAY_CABC_MODE_SWITCH: {
            df.HandleCabcModeCustom(value, cookie);
        } break;
        case DISPLAY_AD: {
            df.HandleSunlightScreen(value, cookie);
        } break;
        case DISPLAY_NIGHT_MODE: {
            df.HandleNightMode(value, cookie);
        } break;
        case DISPLAY_HDR: {
            df.HandleHDR(value, cookie);
        } break;
    }
}

void DisplayFeatureService::waitForEvent()
{

}

void *DisplayFeatureService::threadWrapper(void *me)
{
    return (void *)(uintptr_t)static_cast<DisplayFeatureService *>(me)->threadFunc();
}

int DisplayFeatureService::threadFunc()
{
    prctl(PR_SET_NAME, (unsigned long)"DisplayFeatureService Thread", 0, 0, 0);
    do {
        waitForEvent();
    } while (!mthread_exit);
    return 0;
}

void DisplayFeatureService::setBrightness(int value)
{
    Mutex::Autolock autoLock(mBLLock);
    mCurBL = value;
}

int DisplayFeatureService::getBrightness()
{
    Mutex::Autolock autoLock(mBLLock);
    return mCurBL;
}


// ---------------------------------------------------------------------------

ANDROID_SINGLETON_STATIC_INSTANCE(DisplayEffect);

DisplayEffect::DisplayEffect()
    : Singleton<DisplayEffect>()
{
    m_displayId = 0;
    mEffectStatus = 0;
}

DisplayEffect::~DisplayEffect()
{
    ALOGD("DisplayEffect destroy");
    mEffectStatus = 0;
}

int DisplayEffect::featureEnablePolicy(int featureId, int value,
    int cookie, int caseid, bool enable)
{
    int ret = -1;

    return ret;
}

void DisplayEffect::set_displayId(int displayId)
{
    m_displayId = displayId;
}

int DisplayEffect::get_displayId()
{
    return m_displayId;
}

int DisplayEffect::setDisplayEffect(int fMode, int value, int cookie)
{
    int ret = -1;
    ret = featureEnablePolicy(fMode, value, cookie);
    if (ret != 0) {
        ALOGE("%s %s fail value: 0x%x ret: %d",
            __FUNCTION__, printFeatureMode(fMode), value, ret);
    }
    return ret;
}

void DisplayEffect::sendDisplayEvent(int event)
{
    ALOGD("%s %d", __func__, event);
}

void DisplayEffect::sendDisplayEventExt(int event, int value, int cookie)
{
    ALOGD("%s %d", __func__, event);
}

int DisplayEffect::getNightModeStatus()
{
    Mutex::Autolock autoLock(mStatusLock);
    return (mEffectStatus>>DISPLAY_NIGHT_MODE)&0x1;
}

int DisplayEffect::getEyecareParams(unsigned int level)
{
    int eyecare_params = -1;
    return eyecare_params;
}

int DisplayEffect::getEffectStatus()
{
    Mutex::Autolock autoLock(mStatusLock);
    return mEffectStatus;
}

int DisplayEffect::getEffectStatus(int index)
{
    Mutex::Autolock autoLock(mStatusLock);
    return (mEffectStatus>>index)&0x1;
}

void DisplayEffect::setEffectStatus(int status)
{
    Mutex::Autolock autoLock(mStatusLock);
    mEffectStatus = status;
    ALOGD("%s 0x%x", __func__, mEffectStatus);
}

int DisplayEffect::enableEyecareNightMode(int brightness, int fMode)
{
    int ret = -1;

    return ret;
}

int DisplayEffect::handleLuminousScreen(int brightness, int fMode)
{
    int ret = -1;

    return ret;
}

int DisplayEffect::handleCCBB(int brightness, int fMode)
{
    int ret = -1;
    return ret;
}

int DisplayEffect::handleDispSwitchOption(int modeId, int cookie)
{
    int ret = 0;

    return ret;
}

int DisplayEffect::handleDisplayRequest(int displayId, int caseId, int modeId, int cookie)
{
    int ret = 0;
#ifdef HAS_DISPLAYFEATURE_HAL
    static sp<IDisplayFeature> sClientDisplayFeature = NULL;
    if(sClientDisplayFeature == NULL) {
        sClientDisplayFeature = IDisplayFeature::getService();
        if(sClientDisplayFeature == NULL) {
            ALOGE("Query DisplayFeature service returned null!");
            return -1;
        }
    }
    Return<Status> tmpReturn = sClientDisplayFeature->setFeature(displayId,
                caseId, modeId, cookie);
    if (tmpReturn != Status::OK) {
        ALOGD("set feature failure!");
        ret = -1;
    }
#endif
    return ret;
}

void DisplayEffect::NotifyBrightness(int brightness)
{
#ifdef HAS_DISPLAYFEATURE_HAL
    static android::sp<IDisplayFeature> sClientDisplayFeature = NULL;
    if(sClientDisplayFeature == NULL) {
        sClientDisplayFeature = IDisplayFeature::getService();
        if(sClientDisplayFeature == NULL) {
            ALOGE("Query DisplayFeature service returned null!");
            return;
        }
    }
    sClientDisplayFeature->notifyBrightness(brightness);
#endif
}

int DisplayEffect::HandleACNormal(int cookie, int brightness)
{
    int ret = -1;
    int luminous = -1, status = 0, displayId = 0;
    displayId = get_displayId();
    ALOGD("%s enter", __func__);
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::AUTOMATIC_CONTRAST_STATE,
                IDisplayFeatureControl::AUTOMATIC_CONTRAST_NORMAL,
                cookie);
    if(ret == 0) {
        status = 0x1 << DISPLAY_ENV_ADAPTIVE_NORMAL;
        setEffectStatus(status);
    }
    ALOGD("%s exit ret %d status 0x%x", __func__, ret, status);
    return ret;
}

int DisplayEffect::HandleACCold(int cookie, int brightness)
{
    int ret = -1;
    int luminous = -1, status = 0, displayId = 0;
    displayId = get_displayId();
    ALOGD("%s enter", __func__);
    Mutex::Autolock autoLock(mEffectLock);

    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::AUTOMATIC_CONTRAST_STATE,
                IDisplayFeatureControl::AUTOMATIC_CONTRAST_COLD,
                cookie);
    if(ret == 0) {
        status = 0x1 << DISPLAY_ENV_ADAPTIVE_COLD;
        setEffectStatus(status);
    }
    ALOGD("%s exit ret %d status 0x%x", __func__, ret, status);
    return ret;
}

int DisplayEffect::HandleACWarm(int cookie, int brightness)
{
    int ret = -1;
    int luminous = -1, status = 0, displayId = 0;
    displayId = get_displayId();
    ALOGD("%s enter", __func__);
    Mutex::Autolock autoLock(mEffectLock);

    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::AUTOMATIC_CONTRAST_STATE,
                IDisplayFeatureControl::AUTOMATIC_CONTRAST_WARM,
                cookie);
    if(ret == 0) {
        status = 0x1 << DISPLAY_ENV_ADAPTIVE_WARM;
        setEffectStatus(status);
    }
    ALOGD("%s exit ret %d status 0x%x", __func__, ret, status);
    return ret;
}

int DisplayEffect::HandleIncreasedContrast(int cookie)
{
    int ret = -1, status = 0, displayId = 0;
    displayId = get_displayId();
    ALOGD("%s enter", __func__);
    Mutex::Autolock autoLock(mEffectLock);

    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::INCREASED_CONTRAST_STATE,
                IDisplayFeatureControl::INCREASED_CONTRAST_STATE,
                cookie);
    if(ret == 0) {
        status = 0x1 << DISPLAY_COLOR_ENHANCE;
        setEffectStatus(status);
    }
    ALOGD("%s exit ret %d status 0x%x", __func__, ret, status);
    return ret;
}

int DisplayEffect::HandleStandard(int cookie)
{
    int ret = -1, status = 0, displayId = 0;
    displayId = get_displayId();
    ALOGD("%s enter", __func__);
    Mutex::Autolock autoLock(mEffectLock);

    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::STANDARD_STATE,
                IDisplayFeatureControl::STANDARD_STATE,
                cookie);
    if(ret == 0) {
        status = 0x1 << DISPLAY_STANDARD;
        setEffectStatus(status);
    }
    ALOGD("%s exit ret %d status 0x%x", __func__, ret, status);
    return ret;
}

int DisplayEffect::HandleEyeCare(int modeId, int cookie)
{
    int ret = -1, status = 0, displayId = 0;
    ALOGD("%s enter modeId %d", __func__, modeId);
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);

    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::EYECARE_STATE,
                modeId, cookie);
    if(ret == 0) {
        status = 0x1 << DISPLAY_EYECARE;
        setEffectStatus(status);
    }
    ALOGD("%s exit ret %d status 0x%x", __func__, ret, status);
    return ret;
}

int DisplayEffect::HandleNightMode(int modeId, int cookie)
{
    ALOGD("%s enter", __func__);
    int ret = -1, status = 0, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::NIGHT_STATE,
                modeId, cookie);
    if(ret == 0) {
        status = 0x1 << DISPLAY_NIGHT_MODE;
        setEffectStatus(status);
    }

    ALOGD("%s exit", __func__);
    return ret;
}

int DisplayEffect::HandleHBM(int modeId, int cookie)
{
    ALOGD("%s enter", __func__);
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ALOGD("%s enter %d %d", __func__, modeId, cookie);
    ret = handleDisplayRequest(displayId,
               IDisplayFeatureControl::HBM_STATE,
               modeId, cookie);

    ALOGD("%s exit", __func__);
    return ret;
}

int DisplayEffect::HandleHDR(int modeId, int cookie)
{
    ALOGD("%s enter", __func__);
    int ret = -1;
    Mutex::Autolock autoLock(mEffectLock);

    ALOGD("%s exit", __func__);
    return ret;
}

int DisplayEffect::HandleCabcModeCustom(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);

    ALOGD("%s enter %d %d", __func__, modeId, cookie);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::CABC_MODE_CUSTOM_STATE,
                modeId, cookie);
    ALOGD("%s exit 0x%x", __func__, modeId);
    return ret;
}

int DisplayEffect::HandleKeepWhitePointSRGB(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);

    ALOGD("%s enter", __func__);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::KEEP_WHITE_POINT_SRGB_STATE,
                modeId, cookie);

    ALOGD("%s exit ret %d modeId %d", __func__, ret, modeId);

    return ret;
}

int DisplayEffect::HandleAD(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::AD_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleSunlightScreen(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::SUNLIGHT_SCREEN_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleBrightnessInteract(int modeId, int cookie)
{
    int ret = -1;
    static int last_bl = 0;
    ALOGD("%s enter brightness %d", __func__, modeId);
    Mutex::Autolock autoLock(mEffectLock);

    ALOGD("%s exit ret %d", __func__, ret);
    return ret;
}

int DisplayEffect::HandleCTAdapt(int modeId, int cookie)
{
    int ret = 0;

    return ret;
}

int DisplayEffect::HandleCameraRequest(int modeId,int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::CAMERA_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleGameMode(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::GAME_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleDCBacklight(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::DC_BACKLIGHT_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleFpsSwitch(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::FPS_SWITCH_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::handleDozeBrightnessMode(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::DOZE_BRIGHTNESS_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleExpert(int caseId, int value)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::EXPERT_STATE,
                caseId, value);
    return ret;
}

int DisplayEffect::HandleTouchState(int caseId, int value)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::TOUCH_STATE,
                caseId, value);
    return ret;
}

int DisplayEffect::HandleOritationState(int caseId, int value)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::ORITATION_STATE,
                caseId, value);
    return ret;
}

int DisplayEffect::HandleCupBlackCoveredState(int caseId, int value)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::CUP_BLACK_COVERED_STATE,
                caseId, value);
    return ret;
}

int DisplayEffect::HandleVideoMode(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::VIDEO_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleActiveApp(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::ACTIVE_APP_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleDither(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::DITHER_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleSmartDfpsState(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
		IDisplayFeatureControl::SMART_DFPS_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleFrameDump(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::DUMP_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandlePaperMode(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::PAPER_MODE_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleTrueTone(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::TRUE_TONE_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleColorTempMode(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::COLOR_TEMP_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleBCBC(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::BCBC_CUST_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleDisplayScene(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::DISPLAY_SCENE_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleDarkMode(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::DARK_MODE_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleAdaptiveHDR(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::ADAPTIVE_HDR,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandleDVStatus(int modeId, int cookie)
{
    int ret = -1, displayId = 0;
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::DOLBY_VISION_STATE,
                modeId, cookie);
    return ret;
}

int DisplayEffect::HandlePicHDR(int modeId, int cookie)
{
    int ret = -1, status = 0, displayId = 0;
    ALOGD("%s enter modeId %d,cookie %d", __func__, modeId, cookie);
    displayId = get_displayId();
    Mutex::Autolock autoLock(mEffectLock);
    ret = handleDisplayRequest(displayId,
                IDisplayFeatureControl::PICTURE_HDR_STATE,
                modeId, cookie);
    ALOGD("%s exit ret %d ", __func__, ret);
    return ret;
}

void DisplayEffect::setInt32(string key, int value)
{
    Mutex::Autolock autoLock(mDataLock);
    map<string,int>::iterator   it;
    it = mData.find(key);
    if(it == mData.end()) {
        mData.insert(pair<string,int>(key,value));
        //ALOGD("test insert value %s 0x%x", key.c_str(), value);
    } else
        it->second = value;
}

int DisplayEffect::getInt32(string key, int *value)
{
    int ret = 0;
    Mutex::Autolock autoLock(mDataLock);
    map<string,int>::iterator   it;
    if(mData.empty())
        return -1;
    it = mData.find(key);
    if(it == mData.end()) {
        ALOGW("No find %s",key.c_str());
        ret = -1;
    }
    else
        *value = it->second;
    return ret;
}

}

