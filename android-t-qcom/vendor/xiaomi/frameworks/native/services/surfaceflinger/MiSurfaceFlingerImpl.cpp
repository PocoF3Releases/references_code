/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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

#include <log/log.h>
#include <cutils/properties.h>
#include <ftl/fake_guard.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <hardware/hwcomposer_defs.h>
#include <compositionengine/Display.h>
#include <compositionengine/LayerFE.h>
#include <compositionengine/OutputLayer.h>
#include <compositionengine/LayerFECompositionState.h>
#include <compositionengine/impl/OutputCompositionState.h>
#include <compositionengine/RenderSurface.h>
#include <private/gui/ComposerServiceAIDL.h>
#include "MiSurfaceFlingerImpl.h"
#include <cutils/properties.h>
#include <android-base/file.h>
#include <android-base/macros.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>

// BSP-Game: add Game Fps Stat
#include "GameLayerController/FpsStat.h"

using namespace android::gamelayercontroller;
// END
// MIUI+ send SecureWin state
#include "OutPutExtra/OutputExtra.h"
using namespace android::outputExtra;
// END

namespace android {

using gui::WindowInfo;

using namespace fps_approx_ops;

static void DisplayFeatureCallbackFunc(void *user, int event, int value, float r, float g, float b) {
    MiSurfaceFlingerImpl * source = (MiSurfaceFlingerImpl *) user;
    if (source) {
        source->HandleDisplayFeatureCallback(event, value, r, g, b);
    }
}

MiSurfaceFlingerImpl::MiSurfaceFlingerImpl() {
    ALOGD("%s enter", __func__);
}

MiSurfaceFlingerImpl::~MiSurfaceFlingerImpl() {
    ALOGD("%s enter", __func__);
}

/*
    android T version and above will follow the following command rules:
    ro.vendor.mi_sf.*
    vendor.mi_sf.*
    persist.vendor.mi_sf.*
*/
void MiSurfaceFlingerImpl::init_property(){
    char value[PROPERTY_VALUE_MAX] = {0};
    mIsSupportTpIdleLowBrightness = property_get_bool("ro.vendor.mi_sf.ltpo.tp.idle.lowbrightness.support", true);
    mIsSupportSkipIdlefor_90hz = property_get_bool("ro.vendor.mi_sf.skip_idle_for_90hz", false);
    mIsSyncTpForLtpo = property_get_bool("ro.vendor.mi_sf.ltpo.sync.tp", false);
    mForceHWCBrightness = property_get_bool("ro.vendor.mi_sf.hwc_backlight.support", false);
    mIsSupportLockFpsByPowerFull = property_get_bool("ro.vendor.mi_sf.ltpo.powerfull.with.charger.support", false);
    mIsSupportTouchIdle = property_get_bool("ro.vendor.mi_sf.touch.idle.enable", false);
    mIsSupportSmartpenIdle = property_get_bool("ro.vendor.mi_sf.smartpen.idle.enable", false);
    mIsSupportUltimatePerf = property_get_bool("ro.vendor.mi_sf.ultimate.perf.support", false);
    mIsLtpoPanel = property_get_bool("ro.vendor.mi_sf.ltpo.support", false);
    mProtectDozeState = property_get_bool("ro.vendor.mi_sf.protect.doze.enable", false);
    mIsAodDetectEnable = property_get_bool("ro.vendor.mi_sf.detect.aod.enable", false);
    mIsSupportVideoOrCameraFrameRate = property_get_bool("ro.vendor.mi_sf.video_or_camera_fps.support", false);
    mIsDualBuitInDisp = property_get_bool("ro.vendor.mi_sf.dual_builtin_disp", false);
    mFakeGcp = property_get_bool("ro.vendor.mi_sf.fake_gcp", false);
    mSupportFodneed = property_get_bool("ro.vendor.mi_sf.fodneed.support", false);
    mAllowAospVrr = property_get_bool("ro.vendor.mi_sf.aosp_vrr_support", true);
    mPropDynamicLog = property_get_bool("persist.vendor.mi_sf.dynamic.log", false);
    mIsModeChangeOpt = property_get_bool("ro.vendor.display.mode_change_optimize.enable", false);

    if (!mPlatform_8250) {
        mSetVsyncOptimizeFlag = true;
        mIsSupportSkipIdle = property_get_bool("persist.vendor.mi_sf.force_skip_idle_fps", false);
        property_get("persist.vendor.disable_idle_fps.threshold", value, SKIP_IDLE_THRESHOLD);
        mSkipIdleThreshold = strtoul(value, NULL, 10);
        if (property_get_bool("ro.vendor.soft_backlight.enable", false)) {
            mMaxBrightness = 4095;
        } else {
            mMaxBrightness = 2047;
        }

        mCupMuraEraselayerEnabled = property_get_bool("ro.vendor.mi_sf.cup.muraerase.layer.enable", false);
        ALOGD("fodInit: mCupMuraEraselayerEnabled:%d", mCupMuraEraselayerEnabled);
    }
    mCheckCupHbmCoverlayerEnabled = property_get_bool("ro.vendor.mi_sf.need.check.cup.hbm.coverlayer.enable", false);
    ALOGD("fodInit: mCheckCupHbmCoverlayerEnabled:%d", mCheckCupHbmCoverlayerEnabled);

    mCCTNeedCheckTouch = property_get_bool("ro.vendor.mi_sf.cct.need.check.touch.enable", false);
    ALOGD("fodInit: mCCTNeedCheckTouch:%d", mCCTNeedCheckTouch);

    mSecondRefreshRateOverlayEnabled = property_get_bool("vendor.mi_sf.enable_second_refresh_rate_overlay", false);

    mGalleryP3DVOpt = property_get_bool("ro.vendor.mi_sf.gallery_p3_dolby_opt", false);

    mEmbeddedEnable = property_get_bool("ro.config.miui_activity_embedding_enable", false);

    mIsSupportAutomodeTpIdle = property_get_bool("ro.vendor.mi_sf.enable_tp_idle_automode", false);
    ALOGD("mIsSupportAutomodeTpIdle:%d", mIsSupportAutomodeTpIdle);

    mIsSupportAutomodeForMaxFpsSetting = property_get_bool("ro.vendor.mi_sf.enable_automode_for_maxfps_setting", false);
    ALOGD("mIsSupportAutomodeForMaxFpsSetting:%d", mIsSupportAutomodeForMaxFpsSetting);

    mIsSkipPromotionFpsfor_90hz = property_get_bool("ro.vendor.mi_sf.skip_promtionfps_for_90hz", false);
    ALOGD("mIsSkipPromotionFpsfor_90hz:%d", mIsSkipPromotionFpsfor_90hz);

    // MIUI ADD: Added just for Global HBM Dither (J2S-T)
    mGlobalHBMDitherEnabled = property_get_bool("ro.vendor.mi_sf.display.global_hbm_dither_enabled", false);
    // MIUI ADD: END
}

void MiSurfaceFlingerImpl::init_property_old(){
    mIsSupportTpIdleLowBrightness &= property_get_bool("ro.vendor.display.ltpo.tp.idle.lowbrightness.support", true);
    mIsSupportSkipIdlefor_90hz |= property_get_bool("ro.vendor.display.skip_idle_for_90hz", false);
    mIsSyncTpForLtpo |= property_get_bool("ro.vendor.display.ltpo.sync.tp", false);
    mForceHWCBrightness |= property_get_bool("ro.vendor.display.hwc_backlight.support", false);
    mIsSupportLockFpsByPowerFull |= property_get_bool("ro.vendor.display.ltpo.powerfull.with.charger.support", false);
    mIsSupportTouchIdle |= property_get_bool("ro.vendor.display.touch.idle.enable", false);
    mIsLtpoPanel |= property_get_bool("ro.vendor.display.idle_default_fps.support", false);
    mProtectDozeState |= property_get_bool("ro.vendor.display.protect.doze.enable", false);
    mIsAodDetectEnable |= property_get_bool("ro.vendor.sf.detect.aod.enable", false);
    mIsSupportVideoOrCameraFrameRate |= property_get_bool("ro.vendor.display.video_or_camera_fps.support", false);
    mIsDualBuitInDisp |= property_get_bool("ro.vendor.display.dual_builtin_disp", false);
    mFakeGcp |= property_get_bool("ro.vendor.fake_gcp", false);
    mSupportFodneed |= property_get_bool("ro.vendor.display.fodneed.support", false);
    mIsSupportSmartpenIdle |= property_get_bool("ro.vendor.display.smartpen.idle.enable", false);

    if (!mPlatform_8250) {
        mIsSupportSkipIdle |= property_get_bool("persist.vendor.disable_idle_fps", false);
 
        mCupMuraEraselayerEnabled |= property_get_bool("ro.vendor.cup.muraerase.layer.enable", false);
        ALOGD("fodInit: mCupMuraEraselayerEnabled:%d", mCupMuraEraselayerEnabled);
    }
    mCheckCupHbmCoverlayerEnabled |= property_get_bool("ro.vendor.need.check.cup.hbm.coverlayer.enable", false);
    ALOGD("fodInit: mCheckCupHbmCoverlayerEnabled:%d", mCheckCupHbmCoverlayerEnabled);

    mCCTNeedCheckTouch |= property_get_bool("ro.vendor.cct.need.check.touch.enable", false);
    ALOGD("fodInit: mCCTNeedCheckTouch:%d", mCCTNeedCheckTouch);
}

int MiSurfaceFlingerImpl::init(sp<SurfaceFlinger> flinger) {
    int ret = 0;
    char value[PROPERTY_VALUE_MAX] = {0};

    mFlinger = flinger;

    mIdleFrameRate = property_get_int32("ro.vendor.display.idle_default_fps", 0);
    mFodMonitorFrameRate = property_get_int32("ro.vendor.display.fod_monitor_default_fps", 0);

    const int setFpsStatTimerMs = property_get_int32("ro.vendor.display.set_fps_stat_timer_ms", 0);
    if (setFpsStatTimerMs) {
        if (setFpsStatTimerMs > 1000) {
            const_cast<int&>(setFpsStatTimerMs) = 1000;
        }
        mFpsStatTimerCount = (unsigned long) ((int) 1000 / setFpsStatTimerMs);
        mFpsStatValue.reserve(mFpsStatTimerCount);
        for (auto i = 0u; i < mFpsStatTimerCount; i++) {
            mFpsStatValue.push_back(0);
        }
        ALOGD("mFpsStatTimerCount = %lu, size = %d", mFpsStatTimerCount, mFpsStatValue.size());
        for (auto i = 0u; i < mFpsStatValue.size(); i++) {
            ALOGD("mFpsStatTimerCount = %d", mFpsStatValue[i]);
        }
    }
    fpsStrategy = property_get_bool("ro.vendor.fps.switch.default", false);
    ALOGD("fpsStrategy = %d setFpsStatTimerMs = %d", fpsStrategy, setFpsStatTimerMs);
    if (const auto millis = setFpsStatTimerMs; millis > 0) {
        mFpsStatTimer.emplace(
                "FpsStatTimer", std::chrono::milliseconds(millis),
                [this] { FpsStatTimerCallback(scheduler::Scheduler::TimerState::Reset); },
                [this] { FpsStatTimerCallback(scheduler::Scheduler::TimerState::Expired); });
        mFpsStatTimer->start();
    }

    mPlatform_8250 = property_get_bool("ro.vendor.display.platform_8250", false);

    init_property();
    init_property_old();

    if (mPlatform_8250) {
        property_get("persist.vendor.dc_backlight.threshold", value, "0");
        mSkipIdleThreshold = strtoul(value, NULL, 10);
        ALOGD("mSkipIdleThreshold = %lu", mSkipIdleThreshold);

        mDozeStatePropertySet = property_get_bool("ro.vendor.doze.state.notify.enable", false);
        ALOGD("the doze state is: %d\n.", mDozeStatePropertySet);

        mBacklightMonitorThread = new BacklightMonitorThread();
        if (mBacklightMonitorThread->Start(this, flinger) != NO_ERROR) {
            ALOGE("Run BacklightMonitorThread failed!");
        }

        property_get("persist.sys.sf.native_mode", value, "0");
        flinger->mDisplayColorSetting = static_cast<DisplayColorSetting>(atoi(value));

        if (property_get_bool("ro.vendor.soft_backlight.enable", false)) {
            mMaxBrightness = 4095;
        } else if (property_get_bool("ro.vendor.hbm_backlight.enable", false)) {
            mMaxBrightness = 6142;
        } else {
            mMaxBrightness = 2047;
        }
    }

    if (mIsLtpoPanel) {
        if (property_get("ro.vendor.display.primary_idle_refresh_rate", value, "") > 0) {
            parseIdleFpsByBrightness(MI_DISP_PRIMARY, "ro.vendor.display.primary_idle_refresh_rate");
        }

        if (property_get("ro.vendor.display.secondary_idle_refresh_rate", value, "") > 0) {
            parseIdleFpsByBrightness(MI_DISP_SECONDARY, "ro.vendor.display.secondary_idle_refresh_rate");
        }
    }

    mDFSupportDV = property_get_bool("ro.vendor.display.dolbyvision.support", false);

    mPaperContrastOpt = property_get_bool("ro.vendor.display.papercontrast.opt", false);

    initBlackListForFilterInput();

    mSmartFpsEnable = property_get_bool("ro.vendor.smart_dfps.enable", false);

    mIsFodFingerPrint = property_get_bool("ro.hardware.fp.fod", false);
    ALOGD("Fod FingerPrint: %d", mIsFodFingerPrint);

    mDisplayFeatureGet = new DisplayFeatureGet();
    if (mDisplayFeatureGet.get()) {
        mDisplayFeatureGet->registerCallback(0, new DisplayFeatureCallback(DisplayFeatureCallbackFunc, this));
    }
    mFodMonitorThread = new FodMonitorThread();
    if (mFodMonitorThread->Start(this, flinger) != NO_ERROR) {
        ALOGE("Run FodMonitorThread failed!");
    }

    if (property_get("ro.boot.oled_panel_id", value, "0A") > 0) {
        if(!strncmp(value, "0A", PROPERTY_VALUE_MAX)) {
            mPanelId = 1;
        } else {
            mPanelId = 0;
        }
    }
    ALOGD("%s mPanelId: %d", __func__, mPanelId);

    if (mIsFodFingerPrint) {
        mLocalHbmEnabled = property_get_bool("ro.vendor.localhbm.enable", false);

        if (property_get("ro.product.device", value, "") > 0 && !strncmp(value, "venus", PROPERTY_VALUE_MAX)) {
            if (property_get("ro.boot.oled_panel_id", value, "FF") > 0) {
                if(!strncmp(value, "K2_0A_MP", PROPERTY_VALUE_MAX) && mLocalHbmEnabled) {
                    mLocalHbmEnabled = true;
                } else {
                    mLocalHbmEnabled = false;
                }
            }
        }
        ALOGD("fodInit: value:%s mLocalHbmEnabled:%d", value, mLocalHbmEnabled);

        property_get("ro.hardware.fp.fod.touch.ctl.version", value, "");
        if(!strncmp(value, "2.0", PROPERTY_VALUE_MAX)) {
            mFodEngineEnabled  = true;
        }
        if (mLocalHbmEnabled && mFodEngineEnabled) {
            mFingerprintStateManager = new FingerprintStateManager();
            if (mFingerprintStateManager != nullptr) {
                mFingerprintStateManager->init();
            }
        }

        memset(value, 0x0, sizeof(value));
        property_get("persist.vendor.sys.fp.mulexposupport", value, "0");
        ALOGD("MiSurfaceFlingerImpl init mSupportMulExpo=%d", atoi(value));

        mFodSystemBacklight = 1;
        mTargetBrightness = FingerPrintDaemonGet::TARGET_BRIGHTNESS_600NIT;

        mFPMessageThread = new FPMessageThread();
        if (mFPMessageThread->Start() != NO_ERROR) {
            ALOGE("Run FPMessageThread failed!");
        }

        mTouchMonitorThread = new TouchMonitorThread();
        if (mTouchMonitorThread->Start(this, flinger) != NO_ERROR) {
            ALOGE("Run TouchMonitorThread failed!");
        }
    }

    if (mIsSupportSmartpenIdle) {
        mPenMonitorThread = new PenMonitorThread();
        if (mPenMonitorThread->Start(this, flinger) != NO_ERROR) {
            ALOGE("Run PenMonitorThread failed!");
        }
    }

    getAffinityLimits("ro.miui.affinity.sfuireset", "0-7",
                        mResetUIAffinityCpuBegin, mResetUIAffinityCpuEnd);
    getAffinityLimits("ro.miui.affinity.sfui", "0-0",
                        mUIAffinityCpuBegin, mUIAffinityCpuEnd);
    // true means use the prop property to bind the big core
    bool usePropOrAutoCheck = true;
    if (0 == mUIAffinityCpuBegin && 0 == mUIAffinityCpuEnd) {
        int cpuCombType = getCPUCombType();
        if (CPUCOMBTYPE_NOT_EXIST != cpuCombType) {
            usePropOrAutoCheck = false;
            if (CPUCOMBTYPE_6_2 == cpuCombType) {
                mUIAffinityCpuBegin = mREAffinityCpuBegin = 6;
                mUIAffinityCpuEnd = mREAffinityCpuEnd = 7;
                ALOGI("cpu comb type is 6_2");
            } else if (CPUCOMBTYPE_4_4 == cpuCombType) {
                mUIAffinityCpuBegin = mREAffinityCpuBegin = 4;
                mUIAffinityCpuEnd = mREAffinityCpuEnd = 7;
                ALOGI("cpu comb type is 4_4");
            }
        }
    }
    if (usePropOrAutoCheck) {
        getAffinityLimits("ro.miui.affinity.sfui", "4-7",
                            mUIAffinityCpuBegin, mUIAffinityCpuEnd);
        getAffinityLimits("ro.miui.affinity.sfre", "4-7",
                            mREAffinityCpuBegin, mREAffinityCpuEnd);
    }
#ifdef MI_SF_FEATURE
    mHdrDimmerSupported = property_get_bool("persist.sys.hdr_dimmer_supported", false);
#endif

#ifdef QTI_DISPLAY_CONFIG_ENABLED
    mQsyncFeature = new QsyncFeature();
    if (mQsyncFeature)
        mQsyncFeature->qsyncInit(this, flinger);

#endif
    return ret;
}

void MiSurfaceFlingerImpl::parseIdleFpsByBrightness(disp_display_type displaytype, const std::string& params) {
    std::vector<std::string> typeStrings;
    std::vector<std::string> idleRefreshRateListStrings;
    std::vector<std::string> idleBrightnessListStrings;
    std::vector<int> idleRefreshRateList;
    std::vector<int> idleBrightnessList;
    unsigned int i = 0;

    if (displaytype >= MI_DISP_MAX) {
        return;
    }

    std::string paramsString = android::base::GetProperty(params, " ");
    ALOGD("parseIdleFpsByBrightness:paramsString %s", paramsString.c_str());

    splitString(paramsString, typeStrings, ":");
    for(i = 0; i < typeStrings.size(); i++) {
        ALOGD("parseIdleFpsByBrightness:typeStrings %s", typeStrings[i].c_str());
    }

    splitString(typeStrings[0], idleRefreshRateListStrings, ",");
    for(i = 0; i < idleRefreshRateListStrings.size(); i++) {
        idleRefreshRateList.push_back(atoi(idleRefreshRateListStrings[i].c_str()));
        ALOGD("parseIdleFpsByBrightness:idleRefreshRateListStrings %s(%d)", idleRefreshRateListStrings[i].c_str(), idleRefreshRateList[i]);
    }

    splitString(typeStrings[1], idleBrightnessListStrings, ",");
    for(i = 0; i < idleBrightnessListStrings.size(); i++) {
        idleBrightnessList.push_back(atoi(idleBrightnessListStrings[i].c_str()));
        ALOGD("parseIdleFpsByBrightness:idleBrightnessListStrings %s(%d)", idleBrightnessListStrings[i].c_str(), idleBrightnessList[i]);
    }

    if (MI_DISP_PRIMARY == displaytype) {
        primaryIdleRefreshRateList = idleRefreshRateList;
        primaryIdleBrightnessList = idleBrightnessList;
    }
    else if (MI_DISP_SECONDARY == displaytype) {
        secondaryIdleRefreshRateList = idleRefreshRateList;
        secondaryIdleBrightnessList = idleBrightnessList;
    }
    for(auto value: idleRefreshRateList) {
        ALOGD("parseIdleFpsByBrightness:displaytype %d idleRefreshRateList %d", displaytype, value);
    }
    for(auto value: idleBrightnessList) {
        ALOGD("parseIdleFpsByBrightness:displaytype %d idleBrightnessList %d", displaytype, value);
    }
}

void MiSurfaceFlingerImpl::splitString(const std::string& s, std::vector<std::string>& tokens, const std::string& delimiters)
{
        std::string::size_type lastPos = s.find_first_not_of(delimiters, 0);
        std::string::size_type pos = s.find_first_of(delimiters, lastPos);
        while (std::string::npos != pos || std::string::npos != lastPos) {
                tokens.push_back(s.substr(lastPos, pos - lastPos));
                lastPos = s.find_first_not_of(delimiters, pos);
                pos = s.find_first_of(delimiters, lastPos);
        }
}

bool MiSurfaceFlingerImpl::isFodEngineEnabled() {
    return mFodEngineEnabled;
}

void MiSurfaceFlingerImpl::FpsStatTimerClean() {
    ALOGD("FpsStatTimerClean");
    Mutex::Autolock _l(mFlinger->mStateLock);
    sp<const DisplayDevice> displayDevice = FTL_FAKE_GUARD(mFlinger->mStateLock, mFlinger->getDefaultDisplayDeviceLocked());
    mFlinger->mCurrentState.traverseInZOrder([&](Layer* layer) {
        if (layer->belongsToDisplay(displayDevice->getLayerStack()) && layer->getBuffer() && layer->getBuffer()->getNativeBuffer()) {
            layer->mFrameTracker.mFpsNumFences = 0;
        }
    });
}

void MiSurfaceFlingerImpl::resetFpsStatTimer() {
    if (mFpsStatTimer) {
        mFpsStatTimer->reset();
    }
}

void MiSurfaceFlingerImpl::FpsStatTimerCallback(scheduler::Scheduler::TimerState state) {
    if (scheduler::Scheduler::TimerState::Expired == state) {
        if (mPowerMode != hal::PowerMode::OFF) {
            mFpsStatTimer->reset();
            static unsigned long number = 0;
            int mMaxFps = 0;
            bool isQiyi = false;
            bool isCamera = false;
            bool isIdleState = false;
            ALOGV("FpsStatTimerCallback");
            {
                std::lock_guard<std::mutex> lock(mFlinger->mScheduler->mPolicyLock);
                isIdleState = mFlinger->mScheduler->mPolicy.isIdleState;
            }
            {
                Mutex::Autolock _l(mFlinger->mStateLock);
                if (const auto displayDevice = mFlinger->getDefaultDisplayDeviceLocked()) {
                    mFlinger->mCurrentState.traverseInZOrder([&](Layer* layer) {
                        if (layer->belongsToDisplay(displayDevice->getLayerStack()) && layer->getBuffer() && layer->getBuffer()->getNativeBuffer()) {
                            ALOGV("FpsStatTimerCallback layer name=%s,  mFpsNumFences = %d", layer->getName().c_str(),  layer->mFrameTracker.mFpsNumFences);
                            if (mMaxFps < layer->mFrameTracker.mFpsNumFences) {
                                mMaxFps = layer->mFrameTracker.mFpsNumFences;
                            }
                            layer->mFrameTracker.mFpsNumFences = 0;
                            if (layer->getName().find("com.qiyi.video") != std::string::npos) {
                                isQiyi = true;
                            }
                            if (layer->getName().find("com.android.camera") != std::string::npos) {
                                isCamera = true;
                            }
                        }
                    });

                    mFpsStatValue[number % mFpsStatTimerCount] = mMaxFps;
                    mMaxFps = 0;
                    for (auto i = 0u; i < mFpsStatValue.size(); i++) {
                        mMaxFps += mFpsStatValue[i];
                    }
                    number++;

                    if (displayDevice) {
                        displayDevice->setFpsForSecondRefreshRateOverlay(mMaxFps);
                        displayDevice->showSecondRefreshRateOverlay(isIdleState);
                    }

                }
            }
            for (const DisplayModeIterator modeIt : mFlinger->mScheduler->mRefreshRateConfigs->getPrimaryRefreshRateByPolicyLocked()) {
                const auto& mode = modeIt->second;
                ALOGV("FpsStatTimerCallback iterator fps= %d %d\n", mMaxFps, mode->getFps().getIntValue());
                if (mMaxFps < (mode->getFps().getValue() * 1.1)) {
                    mVideoFrameRate = mode->getFps().getValue();
                    ALOGV("FpsStatTimerCallback find video fps= %.2f\n", mVideoFrameRate);
                    break;
                }
            }

            if ((MI_VIDEO_FPS_24HZ == (int)mVideoFrameRate) && (isQiyi || isCamera)) {
                mVideoFrameRate = MI_VIDEO_FPS_30HZ;
            }

            if ((int)mVideoFrameRate > MI_VIDEO_FPS_60HZ || MI_VIDEO_FPS_40HZ == (int)mVideoFrameRate) {
                mVideoFrameRate = MI_VIDEO_FPS_60HZ;
            }

            if (mSetVideoFlag) {
                if (mVideoLastFrameRate != mVideoFrameRate) {
                    ALOGV("%s : video fps change, mMaxFps = %d, mVideoFrameRate = %f(%f)", __func__, mMaxFps, mVideoFrameRate, mVideoLastFrameRate);
                    setVideoCodecFpsSwitchState();
                }
            }
            mVideoLastFrameRate = mVideoFrameRate;
            ALOGV("%s : mMaxFps = %d, mVideoFrameRate = %f", __func__, mMaxFps, mVideoFrameRate);
        }
    }
}

void MiSurfaceFlingerImpl::queryPrimaryDisplayLayersInfo(const std::shared_ptr<android::compositionengine::Output>
                                        output,
                                        uint32_t& layersInfo) {
    for(auto* outputLayer: output->getOutputLayersOrderedByZ()){
        std::string currentLayerName= outputLayer->getLayerFE().getDebugName();

        if (currentLayerName.find("hbm_overlay") != std::string::npos ||
            currentLayerName.find("enroll_overlay") != std::string::npos){
            layersInfo |= 0x00000001;
        }
    }
    ALOGV("%s : layersInfo: 0x%x ", __func__, layersInfo);
    return;
}

bool MiSurfaceFlingerImpl::hookMiSetColorTransform(const compositionengine::CompositionRefreshArgs& args,
                                         const std::optional<android::HalDisplayId> halDisplayId,
                                         bool isDisconnected) {
    if (mLocalHbmEnabled) {
         return false;
    }

    if (isDisconnected || !halDisplayId) {
         return true;
    }

    if (CC_LIKELY(!args.colorTransformMatrix)) {
        return true;
    }

    auto it = mCurrentColorTransformMap.find(halDisplayId.value());

    if (args.layersInfo == 0x00000001) {
        ALOGV("%s: clear hwc color matrix", __FUNCTION__);
        auto& hwc = mFlinger->getHwComposer();
        status_t result = hwc.setColorTransform(*halDisplayId, mat4());
        if (it == mCurrentColorTransformMap.end()) {
            ALOGV("new colorTransformMatrix mat4 for hbm scene");
            mCurrentColorTransformMap.emplace(halDisplayId.value(), mat4());
        } else {
            ALOGV("clear existed hwc color matrix for hbm scene");
            mCurrentColorTransformMap[halDisplayId.value()] = mat4();
        }

        ALOGE_IF(result != NO_ERROR, "Failed to set color transform on display %d", result);
        return true;
    }

    if (it == mCurrentColorTransformMap.end()) {
        ALOGV("new colorTransformMatrix");
        mCurrentColorTransformMap.emplace(halDisplayId.value(), *args.colorTransformMatrix);
    } else if (*args.colorTransformMatrix == mCurrentColorTransformMap[halDisplayId.value()]) {
        ALOGV("existed colorTransformMatrix");
        return true;
    }

    mCurrentColorTransformMap[halDisplayId.value()] = *args.colorTransformMatrix;

    ALOGV("%s: mCurrentColorTransformMap size:%zu \n use hwc color matrix mCurrentColorTransformMap[%s]: %s",
          __FUNCTION__, mCurrentColorTransformMap.size(), to_string(halDisplayId.value()).c_str(), mCurrentColorTransformMap[halDisplayId.value()].asString().c_str());

    return false;
}

#ifdef MI_FEATURE_ENABLE
void MiSurfaceFlingerImpl::setAlphaInDozeState() {

    sp<const DisplayDevice> displayDevice = FTL_FAKE_GUARD(mFlinger->mStateLock, mFlinger->getDefaultDisplayDeviceLocked());
    if ((displayDevice->getPowerMode() == hal::PowerMode::ON &&
        mLastPowerModeAOD == hal::PowerMode::OFF) && !mMarkAlpha) {
        mLastPowerModeAOD = displayDevice->getPowerMode();
        mFlinger->mDrawingState.traverseInZOrder([&](Layer* layer) {
            if (auto layerFE = layer->getCompositionEngineLayerFE()) {
                if (layer->mNonZeroAlpha) {
                    layer->mNonZeroAlpha = false;
                }
            }
        });
        goto PowerStore;
    }

    if (displayDevice->getPowerMode() == hal::PowerMode::OFF &&
        mLastPowerModeAOD == hal::PowerMode::ON) {
         mFlinger->mDrawingState.traverseInZOrder([&](Layer* layer) {
            if (auto layerFE = layer->getCompositionEngineLayerFE()) {
                if ((layer->getName().find("AOD") == std::string::npos) &&
                    (layer->getName().find("ColorFade") == std::string::npos)) {
                    if (1.0_hf == layer->getAlpha()) {
                        layer->mNonZeroAlpha = true;
                    }
                }
            }
         });
    }

    if (displayDevice->getPowerMode() == hal::PowerMode::DOZE) {
         mFlinger->mDrawingState.traverseInZOrder([&](Layer* layer) {
            if (auto layerFE = layer->getCompositionEngineLayerFE()) {
                if ((layer->getName().find("AOD") == std::string::npos) &&
                    (layer->getName().find("ColorFade") == std::string::npos) &&
                    (layer->getName().find("RefreshRateOverlay") == std::string::npos)) {
                    if (1.0_hf == layer->getAlpha()) {
                        layer->setAlpha(0.0f);
                        layer->mNonZeroAlpha = true;
                    }
                }
            }
         });
         mMarkAlpha = true;
    }

    if (displayDevice->getPowerMode() == hal::PowerMode::ON &&
        displayDevice->getPowerMode() != mLastPowerModeAOD) {
        mFlinger->mDrawingState.traverseInZOrder([&](Layer* layer) {
            if (auto layerFE = layer->getCompositionEngineLayerFE()) {
                if (0.0_hf == layer->getAlpha() && layer->mNonZeroAlpha) {
                    layer->setAlpha(1.0f);
                    layer->mNonZeroAlpha = false;
                }
            }
        });
        mMarkAlpha = false;
    }

PowerStore:
    mLastPowerModeAOD = displayDevice->getPowerMode();
}
#endif

void MiSurfaceFlingerImpl::updateAodRealFps(bool hasAOD) {
    Mutex::Autolock _l(mFlinger->mStateLock);

    const auto display = mFlinger->getDefaultDisplayDeviceLocked();
    Fps refreshRateTemp = 60_Hz;
    bool isDdicAutoMode = false;
    bool isDdicIdleMode = false;
    bool isQsyncMode = isQsyncEnabled();
    if (hasAOD) {
        mSetIdleForceFlag = true;
        forceSkipIdleTimer(true);
        refreshRateTemp = Fps::fromValue(MI_IDLE_FPS_30HZ);
    } else {
        if (!mFodFlag) {
            mSetIdleForceFlag = false;
            if (mIsLtpoPanel) {
                forceSkipIdleTimer(false);
            }
            else {
                if (getCurrentBrightness() > mSkipIdleThreshold) {
                    forceSkipIdleTimer(false);
                }
            }
        }
        if (display) {
            isDdicAutoMode = isDdicAutoModeGroup(display->getActiveMode()->getGroup());
            isDdicIdleMode = isDdicIdleModeGroup(display->getActiveMode()->getGroup());
            if (isDdicIdleMode) {
                if (getDdicMinFpsByGroup(display->getActiveMode()->getGroup()) == MI_IDLE_FPS_10HZ) {
                    refreshRateTemp = Fps::fromValue(MI_IDLE_FPS_10HZ);
                } else {
                    refreshRateTemp = Fps::fromValue(MI_IDLE_FPS_1HZ);
                }
            } else {
                refreshRateTemp = display->getActiveMode()->getFps();
            }
        }
    }

    if (display && display->mRefreshRateOverlay) {
        display->mRefreshRateOverlay->changeRefreshRate(refreshRateTemp, isDdicAutoMode || isQsyncMode);
        if (display->mSecondRefreshRateOverlay)
            display->mSecondRefreshRateOverlay->changeRefreshRate(refreshRateTemp, isDdicAutoMode || isQsyncMode);
    }
}

bool MiSurfaceFlingerImpl::checkVideoFullScreen() {
    ALOGV("checkVideoFullScreen");
    Mutex::Autolock _l(mFlinger->mStateLock);
    bool isFullScreen = false, isFullScreenRatio = false, isRot = false;
    sp<const DisplayDevice> displayDevice = FTL_FAKE_GUARD(mFlinger->mStateLock, mFlinger->getDefaultDisplayDeviceLocked());
    scheduler::RefreshRateConfigs::Policy currentPolicy = mFlinger->mScheduler->mRefreshRateConfigs->getCurrentPolicy();
    auto defaultModeWidth = (displayDevice->getMode(currentPolicy.defaultMode))->getWidth();
    auto defaultModeHeight = (displayDevice->getMode(currentPolicy.defaultMode))->getHeight();
    mFlinger->mCurrentState.traverseInZOrder([&](Layer* layer) {
        if (const auto outputLayer = layer->findOutputLayerForDisplay(displayDevice.get())) {
            const auto& outputLayerState = outputLayer->getState();
            if ((toString(outputLayerState.bufferTransform).find("ROT_90") != std::string::npos ||
                (toString(outputLayerState.bufferTransform).find("ROT_270") != std::string::npos))) {
                isRot = true;
            }
            const Rect& frame = outputLayerState.displayFrame;
            if (layer->getName().find("SurfaceView") != std::string::npos &&
                ((frame.right - frame.left) * (frame.bottom - frame.top) > defaultModeWidth * defaultModeHeight * 0.4)) {
                isFullScreenRatio = true;
            }
            ALOGV("checkVideoFullScreen layer name = %s,  isRot = %d, isFullScreenRatio=%d, (%d,%d)", layer->getName().c_str(), isRot, isFullScreenRatio, defaultModeWidth, defaultModeHeight);
        }
    });
    isFullScreen = isFullScreenRatio & isRot;
    ALOGV("checkVideoFullScreen isfullscreen = %d", isFullScreen);
    return isFullScreen;
}

void MiSurfaceFlingerImpl::checkLayerStack() {

    bool hasIconLayer = false;
    bool hasHbmOverlay = false;
    bool hasLauncher = false;
    bool hasColorFade = false;
    bool hasWallPaper = false;
    bool hasAOD = false;
    bool hasGxzwAnim = false;
    int mTempFodFlag = 1;
    bool hasCupBlackLayer = false;
    bool hasResolutionSwitch = false;
    bool hasP3Layers = false;

    float alpha = float(0.001);

    mHasP3Layers = false;
#ifdef MI_FEATURE_ENABLE
    if (mProtectDozeState) {
        setAlphaInDozeState();
    }
#endif
    if (mSetVideoFullScreenFlag) {
        bool isVideoFullScreen = checkVideoFullScreen();
        if (mPreVideoFullScreen != isVideoFullScreen) {
            ALOGD("checkVideoFullScreen Changed isVideoFullScreen = %d", isVideoFullScreen);
        }
        mPreVideoFullScreen = isVideoFullScreen;
    }

    sp<Layer> layer_proc;
    mFlinger->mDrawingState.traverseInZOrder([&](Layer* layer) {
        if (!mPlatform_8250) {
            if (mCupMuraEraselayerEnabled) {
                if (layer->getName().find("cupMuraCovered") != std::string::npos) {
                    if (mFodMonitorThread != NULL) {
                        alpha = updateCupMuraEraseLayerAlpha(mFodMonitorThread->getCurrentBrightnessClone());
                        if (abs(((float)layer->getAlpha()) -alpha) >=0.0003) {
                            ALOGD("set cupMuraCovered from %f to %f", ((float)layer->getAlpha()), alpha);
                            layer->setAlpha(alpha);
                            mFlinger->setTransactionFlags(eTraversalNeeded);
                        }
                    }
                }
            }
        }

        if (auto layerFE = layer->getCompositionEngineLayerFE()) {
            if (mCheckCupHbmCoverlayerEnabled) {
                if (layer->getName().find("cameraBlackCovered") != std::string::npos) {
                    hasCupBlackLayer = true;
                }
            }

            if ((layer->getName().find("hbm_overlay") != std::string::npos ||
                 layer->getName().find("enroll_overlay") != std::string::npos ||
                 layer->getName().find("gxzw_anim") != std::string::npos) &&
                layer->getAlpha() > 0.05) {
                ATRACE_NAME("hbm_overlay_draw");
                hasHbmOverlay = true;
            }

            if (layer->getName().find("gxzw_anim") != std::string::npos) {
                hasGxzwAnim = true;
            }

            if (layer->getName().find("gxzw_icon") != std::string::npos) {
                hasIconLayer = true;
            }

            if (layer->getName().find("com.miui.home.launcher.Launcher") != std::string::npos) {
                hasLauncher = true;
            }

            if (layer->getName().find("ColorFade") != std::string::npos) {
                hasColorFade = true;
            }

            if (layer->getName().find("AOD") != std::string::npos) {
                hasAOD = true;
            }

            if (layer->getName().find("com.miui.miwallpaper.wallpaperservice.ImageWallpaper") != std::string::npos) {
                hasWallPaper = true;
            }

            if (!mIsCts && (layer->getName().find(".cts.") != std::string::npos)) {
                mIsCts = true;
                std::lock_guard<std::mutex> lock(mFlinger->mScheduler->mPolicyLock);
                mFlinger->mScheduler->mPolicy.isIdleTimerSkip = true;
                ALOGD("set CTS running. %s", layer->getName().c_str());
            }

            if((hasHbmOverlay && !mLocalHbmEnabled) || hasAOD){
                if ((layer->getName().find("MiuiPaperContrastOverlay") != std::string::npos)
                   && layer->getAlpha() > 0.05) {
                    ALOGD("set MiuiPaperContrastOverlay alpha 0");
                    layer->setAlpha(0);
                    mFlinger->setTransactionFlags(eTraversalNeeded);
                }
            } else if ((layer->getName().find("MiuiPaperContrastOverlay") != std::string::npos)
                       && (layer->getAlpha() < 0.05) && !mSetPCAlpha){
                ALOGD("set MiuiPaperContrastOverlay alpha 1");
                layer->setAlpha(1);
                mFlinger->setTransactionFlags(eTraversalNeeded);
            }

            if (mSaveSurfaceName.length() > 0 && layer->getName().find(mSaveSurfaceName) != std::string::npos) {
                {
                    Mutex::Autolock _l(mFogFlagMutex);
                    mTempFodFlag = mFodFlag;
                }
                if (mTempFodFlag == 0 && mFodPowerStateRollOver) {
                    ATRACE_NAME("SaveSurfaceName");
                    ALOGD("SaveSurfaceName=%s composed", mSaveSurfaceName.c_str());
                    if (mSupportFodneed) {
                        if (mFPMessageThread != nullptr)
                            mFPMessageThread->saveSurfaceCompose();
                    }
                    mSaveSurfaceName.clear();
                }
            }

            if (mPaperContrastOpt) {
                if (layer->getName().find("ScreenResolutionLayer") != std::string::npos) {
                    hasResolutionSwitch = true;
                } else if (layer->getName().find("MiuiPaperContrastOverlay") != std::string::npos) {
                    layer_proc = layer;
                }
            }

            if (mGalleryP3DVOpt && layer->getDataSpace() == ui::Dataspace::DISPLAY_P3) {
                mHasP3Layers = true;
            }
        }
    });

    if (mPreHasCameraCoveredLayer != hasCupBlackLayer) {
        ALOGD("CUP Changed cameraBlackCovered=%d", hasCupBlackLayer);
        if (mDisplayFeatureGet) {
            if (hasCupBlackLayer == true)
                mDisplayFeatureGet->setFeature(0, (int32_t)FeatureId::CUP_BLACK_COVERED_STATE, 1, 0);
            else
                mDisplayFeatureGet->setFeature(0, (int32_t)FeatureId::CUP_BLACK_COVERED_STATE, 0, 0);
        }
    }
    mPreHasCameraCoveredLayer = hasCupBlackLayer;

    if (mPreHasGxzwAnim != hasGxzwAnim) {
        ALOGD("gxzw_anim Changed hasGxzwAnim=%d", hasGxzwAnim);
        if (mFingerprintStateManager != nullptr) {
            Mutex::Autolock lock(mFingerprintStateManager->mFingerprintStateManagerMutex);
            mPreHasGxzwAnim = hasGxzwAnim;
            mFingerprintStateManager->fingerprintConditionUpdate(FingerprintStateManager::FOD_ICON_LAYER_STATE, mPreHasGxzwAnim);
        }
    }
    mPreHasGxzwAnim = hasGxzwAnim;

    if (mPreIconVisiable != hasIconLayer) {
        ALOGD("gxzw_icon Changed hasIconLayer=%d", hasIconLayer);
    }
    mPreIconVisiable = hasIconLayer;

    if (mPreHasHbmOverlay != hasHbmOverlay) {
        ALOGD("HbmOverlay Changed hasHbmOverlay=%d", hasHbmOverlay);
    }
    mPreHasHbmOverlay = hasHbmOverlay;

    if (mPreLauncher != hasLauncher) {
        if (mPlatform_8250) {
            notifyDfLauncherFlag(hasLauncher);
        }
        ALOGD("Launcher Changed hasLauncher=%d", hasLauncher);
    }
    mPreLauncher = hasLauncher;

    if (mPreAodLayer != hasAOD) {
        if (mIsAodDetectEnable) {
            notifyDfAodFlag(hasAOD);
        }
        updateAodRealFps(hasAOD);
        ALOGD("AOD hasAOD =%d", hasAOD);
    }
    mSetAodFlag = mPreAodLayer = hasAOD;

    if (mPreWallPaper != hasWallPaper) {
        ALOGD("WallPaper Changed hasWallPaper=%d", hasWallPaper);
    }
    mPreWallPaper = hasWallPaper;

    if (mPreHasColorFade != hasColorFade) {
        ALOGD("ColorFade Changed hasColorFade=%d", hasColorFade);
        if (mFingerprintStateManager != nullptr) {
            Mutex::Autolock lock(mFingerprintStateManager->mFingerprintStateManagerMutex);
            mPreHasColorFade = hasColorFade;
            mFingerprintStateManager->fingerprintConditionUpdate(FingerprintStateManager::COLOR_FADE_LAYER_STATE, mPreHasColorFade);
        }
    }

    if (hasResolutionSwitch && layer_proc.get() && !mSetPCAlpha) {
        ALOGD("set paper contrast layer to transparent");
        layer_proc->setAlpha(0);
        mFlinger->setTransactionFlags(eTraversalNeeded);
        mSetPCAlpha = true;
    } else if (mSetPCAlpha && !hasResolutionSwitch && layer_proc.get()) {
        ALOGD("recovery paper contrast layer");
        layer_proc->setAlpha(1);
        mFlinger->setTransactionFlags(eTraversalNeeded);
        mSetPCAlpha = false;
    }

    mPreHasColorFade = hasColorFade;
}


double MiSurfaceFlingerImpl::getHBMOverlayAlphaLocked() NO_THREAD_SAFETY_ANALYSIS {
    double alpha = 0.0f;

    if (mPlatform_8250) {
        if (mLocalHbmEnabled)
            return alpha;

        sp<const DisplayDevice> displayDevice = FTL_FAKE_GUARD(mFlinger->mStateLock, mFlinger->getDefaultDisplayDeviceLocked());
        hal::PowerMode mode = displayDevice->getPowerMode();
        int factor = 2047;
        int backlight = mBrightnessBackup;

        factor = mMaxBrightness >= 4095 ? 4095 : 2047;
        if (mode != hal::PowerMode::ON) {
            if ((mFodFlag == 0 || mFodFlag == 2 ||
                    (mTouchMonitorThread != NULL && mTouchMonitorThread->getTouchStatus()))
                            && (mFodSystemBacklight > 0)) {
                backlight = mFodSystemBacklight == 1 ? 8 : mFodSystemBacklight;
            } else {
                backlight = DisplayFeatureGet::AOD_HIGH_BRIGHTNESS * factor / 2047;
            }
        }

        // UPGR8150R-1565, porting from cepheus-q, AOD High brightness = 205
        if (mode == hal::PowerMode::DOZE || mode == hal::PowerMode::DOZE_SUSPEND || mode == hal::PowerMode::OFF ) {
            mFodSystemBacklight = (mFodSystemBacklight >= 205)? 205 : mFodSystemBacklight;
            int iconBacklight = mFodSystemBacklight;
            if (!(mTouchMonitorThread->getTouchStatus()) && mFodFlag == 1 && mFodSystemBacklight <= 205){
                iconBacklight = 205;
                ALOGD("Prevent fod icon from low brightness, set iconBacklight is AOD_HIGH_BRIGHTNESS = 205");
            }
            backlight = iconBacklight;
        }

        ALOGD("getHBMOverlayAlpha backlight=%d, mFodSystemBacklight=%d, mBrightnessBackup=%d",
                backlight, mFodSystemBacklight, mBrightnessBackup);

        if (mTargetBrightness == FingerPrintDaemonGet::TARGET_BRIGHTNESS_600NIT) {
            if (mMaxBrightness >= 4095) {
                if (!mDozeStatePropertySet) {
                    if (mPanelId == 1) {
                        if (backlight <= 32)
                            alpha = 1 - (0.0008 * backlight + 0.062);
                        else if (backlight >= 33 && backlight <= 250)
                            alpha = 1 - (0.0008 * backlight + 0.0548);
                        else if (backlight >= 251 && backlight <= 280)
                            alpha = 1 - (0.0003 * backlight + 0.175);
                        else if (backlight >= 281 && backlight <= 400)
                            alpha = 1 - pow((backlight * 5.0) / (4095 * 7), 1.0/2.26);
                        else if (backlight >= 401 && backlight <= 4095)
                            alpha = 1 - pow((backlight * 5.0) / (4095 * 7), 1.0/2.2);
                    } else if (mPanelId == 0) {
                        if (backlight <= 60)
                            alpha = 1 - (0.0005 * backlight + 0.0608);
                        else if (backlight >= 61 && backlight <= 220)
                            alpha = 1 - (0.000845 * backlight + 0.0408);
                        else if (backlight >= 221 && backlight <= 399)
                            alpha = 1 - (0.000304 * backlight + 0.1595);
                        else if (backlight >= 400 && backlight <= 4095)
                            alpha = 1 - pow((backlight * 0.69 / 4095.0), 1.0/2.2);
                    }
                } else
                    if (backlight <= 60)
                        alpha = 1 - (0.0005 * backlight + 0.0608);
                    else if (backlight >= 61 && backlight <= 220)
                        alpha = 1 - (0.000845 * backlight + 0.0408);
                    else if (backlight >= 221 && backlight <= 399)
                        alpha = 1 - (0.000304 * backlight + 0.1595);
                    else if (backlight >= 400 && backlight <= 4095)
                        alpha = 1 - pow((backlight * 0.641 / 4095.0), 1.0/2.2);
            } else {
                alpha = 1 - pow((backlight * 1.0 / 2047) * 500.0 / 700.0, 0.455);
            }
        } else if (mTargetBrightness == FingerPrintDaemonGet::TARGET_BRIGHTNESS_300NIT) {
            alpha = 1 - pow((backlight * 1.0 / (1680 * factor / 2047)), 0.455);
        } else {
            ALOGE("getHBMOverlayAlpha invalid mTargetBrightness");
        }
        if(abs(alpha) < 1.0/255) {
            alpha = 1.0/255;
        }
        return alpha;
    } else {
        if (mLocalHbmEnabled)
            return alpha;

        sp<const DisplayDevice> displayDevice = mFlinger->getDefaultDisplayDeviceLocked();
        hal::PowerMode mode = displayDevice->getPowerMode();
        int backlight = mBrightnessBackup;

        if (mode != hal::PowerMode::ON) {
            if ((mFodFlag == 0 || mFodFlag == 2 ||
                    (mTouchMonitorThread != NULL && mTouchMonitorThread->getTouchStatus()))
                            && (mFodSystemBacklight > 0)) {
                backlight = mFodSystemBacklight == 1 ? 8 : mFodSystemBacklight;
            } else {
                backlight =  DisplayFeatureGet::AOD_HIGH_BRIGHTNESS * mMaxBrightness / 2047;
            }
        }

        ALOGD("getHBMOverlayAlpha backlight=%d, mFodSystemBacklight=%d, mBrightnessBackup=%d",
                backlight, mFodSystemBacklight, mBrightnessBackup);

        if (mTargetBrightness == FingerPrintDaemonGet::TARGET_BRIGHTNESS_600NIT) {
            alpha = 1 - pow((backlight * 1.0 / 2047) * 500.0 / 900.0, 0.455);
        } else if (mTargetBrightness == FingerPrintDaemonGet::TARGET_BRIGHTNESS_300NIT) {
            alpha = 1 - pow((backlight * 1.0 / (1680 * mMaxBrightness / 2047)), 0.455);
        } else {
            ALOGE("getHBMOverlayAlpha invalid mTargetBrightness");
        }

        return alpha;
    }
}

int32_t MiSurfaceFlingerImpl::getCurrentSettingFps() {
    int32_t  Fps = 0;

    scheduler::RefreshRateConfigs::Policy policy = mFlinger->mScheduler->mRefreshRateConfigs->getCurrentPolicy();
    Fps = policy.primaryRange.max.getIntValue();
    ALOGD("%s: Fps: %d", __func__, Fps);

    return Fps;
}
void MiSurfaceFlingerImpl::updateUnsetFps(int32_t flag) {
    mUnsetSettingFps = flag;
    ALOGD("%s, receive mUnsetSettingFps: %d", __func__, mUnsetSettingFps);
}

void MiSurfaceFlingerImpl::changeFps(int32_t fps, bool fixed, bool isUseQsyncGroup) {
    int32_t groupId = 0;
    static bool fixed_last = false;
    ALOGD("%s, fps:%d", __func__, fps);
    if (fps >= 0) {
        const auto displayToken = ComposerServiceAIDL::getInstance().getInternalDisplayToken();
        {
            if (!isUseQsyncGroup) {
                sp<const DisplayDevice> displayDevice = FTL_FAKE_GUARD(mFlinger->mStateLock, mFlinger->getDefaultDisplayDeviceLocked());
                groupId = displayDevice->refreshRateConfigs().getActiveMode()->getGroup();
                groupId &= 0xFF;
            }else {
                if (mQsyncFeature)
                    groupId =  mQsyncFeature->getQsyncTimingModeGroup();
            }
            for (auto& iter: mFlinger->mScheduler->mRefreshRateConfigs->getAllRefreshRates()) {
                if (iter.second->getFps() == Fps::fromValue(fps) && (iter.second->getGroup() == groupId)) {
                    if (fixed) {
                        if (!fixed_last) {
                            mUnsetSettingFps = 0;
                            mCurrentSettingFps = getCurrentSettingFps();
                        }
                    }
                    
                    mFlinger->mDebugDisplayModeSetByBackdoorLine = __LINE__;
                    mFlinger->mDebugDisplayModeSetByBackdoor = false;
                    status_t result =
                        mFlinger->setDesiredDisplayModeSpecs(displayToken,iter.second->getId().value(),
                            false, fixed ? fps: 0, fps, fixed ? fps: 0, fps);
                    if (fixed) {
                        mFlinger->mDebugDisplayModeSetByBackdoor = true;
                        mFlinger->mDebugDisplayModeSetByBackdoorLine = __LINE__;
                    }
                    ALOGD("set fps setDesiredDisplayConfigSpecs: fps = %d  groupId = %d fixed = %d result = %d",
                            fps, groupId, fixed, result);
                    break;
                }
            }
        }
        fixed_last = fixed;
    }
}

status_t MiSurfaceFlingerImpl::enableSmartDfps(int32_t fps){
    int32_t groupId = 0;

    ALOGD("%s, fps:%d", __func__, fps);
    mFlinger->mDebugDisplayModeSetByBackdoorLine = __LINE__;
    mFlinger->mDebugDisplayModeSetByBackdoor = false;
    if (fps >= 0) {
        const auto displayToken = ComposerServiceAIDL::getInstance().getInternalDisplayToken();
        {
            sp<const DisplayDevice> displayDevice = FTL_FAKE_GUARD(mFlinger->mStateLock, mFlinger->getDefaultDisplayDeviceLocked());
            groupId = displayDevice->refreshRateConfigs().getActiveMode()->getGroup();
            groupId &= 0xFF;
            for (auto& iter: mFlinger->mScheduler->mRefreshRateConfigs->getAllRefreshRates()) {
                if (iter.second->getFps() == Fps::fromValue(fps) && (iter.second->getGroup() == groupId)) {
                    status_t result =
                            mFlinger->setDesiredDisplayModeSpecs(displayToken,iter.second->getId().value(),
                                                         false, 0, fps, 0, fps);
                    ALOGD("set fps setDesiredDisplayConfigSpecs:fps = %d result = %d", fps,
                          result);
                    mFlinger->mDebugDisplayModeSetByBackdoorLine = __LINE__;
                    mFlinger->mDebugDisplayModeSetByBackdoor = true;
                    break;
                }
            }
        }
    }
    return NO_ERROR;
}

void MiSurfaceFlingerImpl::setVideoCodecFpsSwitchState(void) {
    mFlinger->mScheduler->mRefreshRateConfigs->resetBestRefreshRateCache();
    mFlinger->mScheduler->setVideoCodecFpsSwitchState(scheduler::Scheduler::VideoCodecFpsSwitchState::Change);
    std::lock_guard<std::mutex> lock(mFlinger->mScheduler->mPolicyLock);
    mFlinger->mScheduler->mPolicy.videoCodec = scheduler::Scheduler::VideoCodecFpsSwitchState::NoChange;
}


bool MiSurfaceFlingerImpl::isAutoModeAllowed(void) {
    return !mFodFlag && !mIsCts && !mSetQsyncFlag && !mSetInputFlag && mSetAutoModeFlag && !mPreHasGxzwAnim;
}

const char* MiSurfaceFlingerImpl::getFpsSwitchModuleName(const int& module) const {
    switch (module) {
        case FPS_SWITCH_BENCHMARK: return "BenchMark";
        case FPS_SWITCH_TOPAPP: return "TopApp-B";
        case FPS_SWITCH_VIDEO: return "Video";
        case FPS_SWITCH_POWER: return "Power";
        case FPS_SWITCH_MULTITASK: return "MultiTask";
        case FPS_SWITCH_CAMERA: return "Camera";
        case FPS_SWITCH_THERMAL: return "Thermal";
        case FPS_SWITCH_GAME: return "Game";
        case FPS_SWITCH_SETTINGS: return "Settings";
        case FPS_SWITCH_SMART: return "Smtfps";
        case FPS_SWITCH_LAUNCHER: return "Launcher";
        case FPS_SWITCH_WHITE_LIST: return "TopApp-W";
        case FPS_SWITCH_DEFAULT: return "Default";
        case FPS_SWITCH_VIDC: return "VideoCodec";
        case FPS_SWITCH_EBOOK:return "Ebook";
        case FPS_SWITCH_TP_IDLE:return "TpIdle";
        case FPS_SWITCH_PROMOTION:return "Promotion";
        case FPS_SWITCH_SHORT_VIDEO:return "ShortVideo";
        case FPS_SWITCH_HIGH_FPS_VIDEO:return "HighFpsVideo";
        case FPS_SWITCH_POWER_FULL_ON_CHARGER:return "PowerFullOnCharger";
        case FPS_SWITCH_MAP_QSYNC: return "MapAPP";
        case FPS_SWITCH_ULTIMATE_PERF: return "Ultimate_perf";
        case FPS_SWITCH_VIDEO_FULL_SCREEN: return "Video_full_screen";
        case FPS_SWITCH_AUTOMODE: return "Automode";
        case FPS_SWITCH_ANIMATION_SCREEN: return "Animation";
        case FPS_SWITCH_GOOGLE_VRR: return "VRR";
        case FPS_SWITCH_INPUT: return "Input";
        case FPS_SWITCH_LOWER_VIDEO: return "Lower_video";
        default: return "Unknown";
    }
}

void MiSurfaceFlingerImpl::updateScene(const int& module, const int& value, const String8& pkg) {
    ALOGD("%s: module = %d (%s), value = %d, pkg = %s, Vrr = %d\n", __FUNCTION__,
            module, getFpsSwitchModuleName(module), value,
            pkg.isEmpty() ? "Unknown" : pkg.c_str(), mAllowAospVrr);
    Mutex::Autolock _l(mUpdateSceneMutex);
    switch (module) {
        case FPS_SWITCH_LOWER_VIDEO: {
            mSetLowerVideoFlag = value;
            ALOGD("updateScene mSetLowerVideoFlag status %d\n", mSetLowerVideoFlag);
        } break;
        case FPS_SWITCH_INPUT: {
            mSetInputFlag = value;
            setVideoCodecFpsSwitchState();
            ALOGD("updateScene mSetInputFlag status %d\n", mSetInputFlag);
        } break;
        case FPS_SWITCH_GOOGLE_VRR: {
            mSetGoogleVrrFlag = value;
            ALOGD("updateScene mSetGoogleVrrFlag status %d\n", mSetGoogleVrrFlag);
        } break;
        case FPS_SWITCH_AUTOMODE: {
            mSetAutoModeFlag = value;
            ALOGD("updateScene mSetInputFlag status %d\n", mSetAutoModeFlag);
        } break;
        case FPS_SWITCH_ANIMATION_SCREEN: {
            mSetAnimationFlag = value;
            if (mSetAnimationFlag) {
                mSetQsyncVsyncRate = 120;
            } else {
                mSetQsyncVsyncRate = 60;
            }
#ifdef QTI_DISPLAY_CONFIG_ENABLED
            if (mQsyncFeature && mSetQsyncFlag && !isFod()) {
                    mQsyncFeature->updateQsyncStatus(mSetQsyncFlag, pkg, 10, 10, 60, mSetQsyncVsyncRate);
            }
#endif
            ALOGD("updateScene mSetAnimationFlag status %d, vsync %d\n", mSetAnimationFlag, mSetQsyncVsyncRate);
        } break;
        case FPS_SWITCH_VIDEO_FULL_SCREEN: {
            mSetVideoFullScreenFlag = value;
            mSetVideoFlag = value;
            ALOGD("updateScene VideoFullScreen status %d\n", mSetVideoFullScreenFlag);
        } break;
        case FPS_SWITCH_POWER_FULL_ON_CHARGER: {
            if (mIsSupportLockFpsByPowerFull) {
                mSetLockFpsPowerFullFlag = value;
                forceSkipIdleTimer(value);
                ALOGD("updateScene power full on charger status %d\n", value);
            }
        } break;
        case FPS_SWITCH_PROMOTION: {
            if (mIsLtpoPanel) {
                mSetPromotionFlag = value;
                mAllowAospVrr =  value ? false: true;
                if (value) {
                    mFlinger->mScheduler->mTouchTimer->setInterval(std::chrono::milliseconds(ePromationTpIdleTimeoutInterval));
                }
                else {
                    mFlinger->mScheduler->mTouchTimer->setInterval(std::chrono::milliseconds(eTpIdleTimeoutInterval));
                }
                ALOGD("updateScene Promotion %d\n", mSetPromotionFlag);
            }
        } break;
        case FPS_SWITCH_BENCHMARK: {
            mSetIdleForceFlag = value;
            forceSkipIdleTimer(value);
            ALOGD("updateScene benchmark %d\n", value);
        } break;
        case FPS_SWITCH_VIDEO: {
            if (mIsSupportVideoOrCameraFrameRate) {
                mSetVideoFlag = value;
                ALOGD("updateScene mSetVideoFlag %d\n", mSetVideoFlag);
            }
        } break;
        case FPS_SWITCH_SHORT_VIDEO: {
            if (mIsSupportVideoOrCameraFrameRate) {
                mSetShotVideoFlag = value;
                mFlinger->mScheduler->mTouchTimer->setInterval(std::chrono::milliseconds(eTpIdleTimeoutInterval));
                ALOGD("updateScene ShortVideoFlag %d\n", value);
            }
        } break;
        case FPS_SWITCH_HIGH_FPS_VIDEO: {
            if (mIsSupportVideoOrCameraFrameRate) {
                mSetVideoHighFpsFlag = value;
                ALOGD("updateScene mSetVideoHighFpsFlag %d\n", value);
            }
        } break;
        case FPS_SWITCH_CAMERA: {
            if (mIsSupportVideoOrCameraFrameRate) {
                mSetLowerVideoFlag = mSetCameraFlag = value;
                mFlinger->mScheduler->mTouchTimer->setInterval(std::chrono::milliseconds(eTpIdleTimeoutInterval));
                ALOGD("updateScene mSetCameraFlag %d\n", mSetCameraFlag);
            }
        } break;
        case FPS_SWITCH_EBOOK: {
            if (!mPlatform_8250) {
                if (mIsSyncTpForLtpo) {
                    mSetEbookIdleFlag = value;
                    if (mSetEbookIdleFlag) {
                        if (mFodMonitorThread->getCurrentBrightnessClone() > MI_IDLE_FPS_1HZ_BRIGHTNESS) {
                            mIdleFrameRate = MI_IDLE_FPS_1HZ;
                        }
                    } else {
                        if (MI_IDLE_FPS_1HZ == mIdleFrameRate) {
                            mIdleFrameRate = MI_IDLE_FPS_10HZ;
                        }
                    }
                    ALOGD("updateScene mSetEbookIdleFlag %d\n", mSetEbookIdleFlag);
                 }
            }
        } break;
        case FPS_SWITCH_TP_IDLE: {
            mSetLtpoTpIdleFlag = value;
            mFlinger->mScheduler->mTouchTimer->setInterval(std::chrono::milliseconds(eTpIdleTimeoutInterval));
            ALOGD("updateScene mSetTpIdleFlag %d\n", mSetLtpoTpIdleFlag);
        } break;
        case FPS_SWITCH_MAP_QSYNC: {
            if (strcmp(pkg, "FOD") != 0) {
                mSetQsyncFlag = value;
            }
            else if(0 == value) {
                mQsyncFeature->updateQsyncStatus(value, pkg, 10, 10, 60, 60);
                break;
            }
            if (mSetAnimationFlag && mSetQsyncFlag) {
                ALOGD("do nothing for mSetAnimationFlag setted!");
            } else {
#ifdef QTI_DISPLAY_CONFIG_ENABLED
                if (mQsyncFeature) {
                    if (!mSetAnimationFlag) {
                        if (mSetQsyncFlag && isFod()) {
                            ALOGD("do nothing for fod setted!");
                            break;
                        }
                    }
                    mQsyncFeature->updateQsyncStatus(value, pkg, 10, 10, 60, 60);
                }
#endif
            }
            ALOGD("updateScene qsync statue %d by %s, vsync %d\n", mSetQsyncFlag, pkg.c_str(), mSetQsyncVsyncRate);
        } break ;
        default:
            break;
    }
}

void MiSurfaceFlingerImpl::isRestoreFps(bool is_restorefps, int32_t lock_fps) {
    if (is_restorefps) {
        if (lock_fps != 0) {
            changeFps(lock_fps, false);
        }
        mAllowAospVrr = true;
        mSetCmdLockFpsFlag = false;
        forceSkipIdleTimer(false);
        forceSkipTouchTimer(false);
    } else {
        mAllowAospVrr = false;
        mSetCmdLockFpsFlag = true;
        forceSkipIdleTimer(true);
        forceSkipTouchTimer(true);
        changeFps(lock_fps, true);
    }
}

void MiSurfaceFlingerImpl::setLockFps(DisplayModePtr mode) {
    mFlinger->mScheduler->mSchedulerCallback.requestDisplayMode(std::move(mode),
                                                                                                                        scheduler::DisplayModeEvent::Changed);
}

status_t MiSurfaceFlingerImpl::updateDisplayLockFps(int32_t lockFps, bool ddicAutoMode, int ddicMinFps) {
    ALOGD("%s: lockFps:%d, ddicAutoMode:%d, ddicMinFps:%d\n", __FUNCTION__, lockFps, ddicAutoMode, ddicMinFps);
    bool is_restorefps = false;
    static bool is_last_resetorefps = true;
    scheduler::RefreshRateConfigs::Policy policy = mFlinger->mScheduler->mRefreshRateConfigs->getCurrentPolicy();
    int32_t mode_group = 0;
    const DisplayModePtr& modePtr =
        mFlinger->mScheduler->mRefreshRateConfigs->getAllRefreshRates().get(policy.defaultMode)->get();
    if (modePtr)
        mode_group = modePtr->getGroup();

    bool is_valid = false;
    bool is_ddicIdleMode = false;
    if (MI_IDLE_FPS_10HZ == lockFps || MI_IDLE_FPS_1HZ == lockFps) {
        is_ddicIdleMode= true;
        ddicMinFps = lockFps;
        lockFps = MI_IDLE_FPS_120HZ;
        mode_group = (int32_t)(MI_DDIC_IDLE_MODE_GROUP | (((uint32_t)lockFps & 0xFF)  << 16) |
                          (((uint32_t)ddicMinFps & 0xFF) << 8) |
                          (mode_group & 0xFF));
    }
    if (ddicAutoMode) {
        mode_group = (int32_t)(MI_DDIC_AUTO_MODE_GROUP | (((uint32_t)lockFps & 0xFF)  << 16) |
                          (((uint32_t)ddicMinFps & 0xFF) << 8) |
                          (mode_group & 0xFF));
    }
    ALOGD("%s: mode_group = %d", __FUNCTION__, mode_group);

    if (0 == lockFps) {
        isRestoreFps(true, mUnsetSettingFps);
        return NO_ERROR;
    }

    const auto display = getActiveDisplayDevice();
    if (!display) {
        return UNEXPECTED_NULL;
    }
    const auto supportedModes = display->getSupportedModes();
    if (supportedModes.size() <= 0) {
        return BAD_VALUE;
    }

    for (const auto& mode : supportedModes) {
        DisplayModePtr modePtr = mode.second;
        if (modePtr->getFps() == Fps::fromValue(lockFps) && modePtr->getGroup() == mode_group) {
            is_valid = true;
            break;
        }
    }
    if (!is_valid) {
        ALOGD("%s: lock fps(%d) is invalid!", __FUNCTION__, lockFps);
        return BAD_VALUE;
    }

    isRestoreFps(false, lockFps);

    for (auto& iter: mFlinger->mScheduler->mRefreshRateConfigs->getAllRefreshRates()) {
        if (iter.second->getFps() == Fps::fromValue(lockFps) && (iter.second->getGroup() == mode_group)) {
            ALOGD("%s choose FPS:%d, mode:%d", __FUNCTION__, iter.second->getFps().getIntValue(), mode_group);
            setLockFps(iter.second);
        }
    }

    return NO_ERROR;
}

status_t MiSurfaceFlingerImpl::updateGameFlag(int32_t flag) {
    ALOGD("updateGameFlag %d\n", flag);
    //Flag used to set game flag
    mSetGameFlag = flag;
    forceSkipIdleTimer(mSetGameFlag);
    return NO_ERROR;
}

status_t MiSurfaceFlingerImpl::updateVideoInfo(s_VideoInfo *videoInfo) {
    ALOGD("updateVideoInfo mSetBulletChatFlag %d, video fps %f \n", videoInfo->bulletchatState, videoInfo->videofps);
    //Flag used to set bulletchat flag
    mSetBulletChatState = videoInfo->bulletchatState;
    mAppVideoFrameRate = videoInfo->videofps;
    if (mAppVideoFrameRate > 0) {
        mFlinger->mScheduler->mTouchTimer->setInterval(std::chrono::milliseconds(eTpTimeoutInterval));
    }
    else {
        mFlinger->mScheduler->mTouchTimer->setInterval(std::chrono::milliseconds(eFpsStatTimeoutInterval));
    }
    mFlinger->mScheduler->onTouchHint();
    return NO_ERROR;
}

bool VideoFpsGreaterSort (s_VideoDecoderInfo a, s_VideoDecoderInfo b) {
    return (a.floatfps > b.floatfps);
}

status_t MiSurfaceFlingerImpl::updateVideoDecoderInfo(s_VideoDecoderInfo *videoDecoderInfo) {
    std::vector<s_VideoDecoderInfo>::iterator iter;
    float max_video_fps;
    bool isSameVideoPid = false;
    bool isUpdateVideoFps = true;
    if (videoDecoderInfo->state) {
        {
            Mutex::Autolock _l(mVideoDecoderMutex);
            if(!mVideoDecoderFps.empty()) {
                ALOGD("updateVideoDecoderInfo clear video fps\n");
                mVideoDecoderFps.clear();
            }
            if(mVideoDecoderFps.empty()) {
                mVideoDecoderFps.push_back(*videoDecoderInfo);
                ALOGD("updateVideoDecoderInfo empty, add the first new video fps\n");
            }
            for (iter = mVideoDecoderFps.begin(); iter != mVideoDecoderFps.end(); iter++) {
                if (iter->tid == videoDecoderInfo->tid) {
                    iter->floatfps = videoDecoderInfo->floatfps;
                    iter->intfps = videoDecoderInfo->intfps;
                    isSameVideoPid = true;
                    ALOGD("updateVideoDecoderInfo find same tid, update new video fps\n");
                }
            }
            if (!isSameVideoPid) {
                ALOGD("updateVideoDecoderInfo can't find same video fps, add new video fps\n");
                mVideoDecoderFps.push_back(*videoDecoderInfo);
            }
            for (uint32_t i = 0; i < mVideoDecoderFps.size(); i++) {
                ALOGV("updateVideoDecoderInfo fps= %d,  tid = %d, size = %d\n", mVideoDecoderFps[i].intfps, mVideoDecoderFps[i].tid, mVideoDecoderFps.size());
            }
            std::sort(mVideoDecoderFps.begin(), mVideoDecoderFps.end(), VideoFpsGreaterSort);
            for (uint32_t i = 0; i < mVideoDecoderFps.size(); i++) {
                ALOGD("after updateVideoDecoderInfo fps= %d,  tid = %d\n", mVideoDecoderFps[i].intfps, mVideoDecoderFps[i].tid);
            }
            max_video_fps = mVideoDecoderFps.begin()->floatfps;
        }
        for (const DisplayModeIterator modeIt : mFlinger->mScheduler->mRefreshRateConfigs->getPrimaryRefreshRateByPolicyLocked()) {
            const auto& mode = modeIt->second;
            ALOGD("updateVideoDecoderInfo iterator fps= %.2f, %d\n", max_video_fps, mode->getFps().getIntValue());
            if (max_video_fps < (mode->getFps().getValue() * 1.1)) {
                mVideoDecoderFrameRate = mode->getFps().getValue();
                if (mSetLowerVideoFlag) {
                    mVideoDecoderFrameRateReal = mVideoDecoderFrameRate;
                    break;
                }
                if (isUpdateVideoFps && mVideoDecoderFrameRate < 60) {
                    mVideoDecoderFrameRateReal = mVideoDecoderFrameRate;
                    isUpdateVideoFps = false;
                    continue;
                }
                else {
                    ALOGD("updateVideoDecoderInfo find fps= %.2f\n", mVideoDecoderFrameRate);
                    break;
                }
            }
        }
        if (getCurrentSettingFps() < mVideoDecoderFrameRate) {
            mVideoDecoderFrameRate = getCurrentSettingFps();
        }
        if (mSetShotVideoFlag) {
            if ((int)mVideoDecoderFrameRateReal <= MI_VIDEO_FPS_40HZ) {
                mVideoDecoderFrameRateReal = MI_VIDEO_FPS_40HZ;
            }
        }
        setVideoCodecFpsSwitchState();
    }
    else {
        Mutex::Autolock _l(mVideoDecoderMutex);
        uint32_t tid_mask = 0xFFFFFF00;
        for (iter = mVideoDecoderFps.begin(); iter != mVideoDecoderFps.end(); iter++) {
            if ((iter->tid & tid_mask) == (videoDecoderInfo->tid & tid_mask)) {
                ALOGD("updateVideoDecoderInfo erase fps %d\n", iter->intfps);
                mVideoDecoderFps.erase(iter);
                break;
            }
        }
    }
    ALOGD("ShotVideoFlag %d,VideoHighFpsFlag %d,mVideoFrameRate %d,%f\n", mSetShotVideoFlag, mSetVideoHighFpsFlag, videoDecoderInfo->intfps, mVideoFrameRate);
    return NO_ERROR;
}

void MiSurfaceFlingerImpl::updateFpsRangeByLTPO(DisplayDevice* displayDevice, FpsRange* fpsRange) {
    ALOGV("%s updateFpsRangeByLTPO", __func__);
    const auto supportedModes = displayDevice->getSupportedModes();
    if (supportedModes.size() <= 0) {
        return;
    }
    for (const auto& mode : supportedModes) {
        if (isDdicIdleModeGroup(mode.second->getGroup())) {
            if (getDdicMinFpsByGroup(mode.second->getGroup()) == MI_IDLE_FPS_1HZ) {
                if (fpsRange->min.getIntValue() > MI_IDLE_FPS_1HZ) {
                    fpsRange->min = Fps::fromValue(MI_IDLE_FPS_1HZ);
                }
            } else if (getDdicMinFpsByGroup(mode.second->getGroup()) == MI_IDLE_FPS_10HZ) {
                if (fpsRange->min.getIntValue() > MI_IDLE_FPS_10HZ) {
                    fpsRange->min = Fps::fromValue(MI_IDLE_FPS_10HZ);
                }
            }
        }
    }
    return;
}

void MiSurfaceFlingerImpl::setDVStatus(int32_t enable) {
    ALOGV("%s DVStatus = %d", __func__, enable);
    sp<IDisplayFeature> dfClient(mDisplayFeatureGet->getDisplayFeatureService());
    if (dfClient != NULL) {
        if (mDFSupportDV) {
            dfClient->setFeature(0, 44, enable, 0);
            mDolbyVisionState = enable ? true : false;
        }
    }
}

void MiSurfaceFlingerImpl::updateMiFodFlag(compositionengine::Output *output,
                                           const compositionengine::CompositionRefreshArgs& refreshArgs) {
    auto& outputState = output->editState();
    if (!outputState.layerFilter.toInternalDisplay) {
        return;
    }

    ALOGV("%s mFodFlag:%d devOptForceClientComposition:%d", __func__, mFodFlag, refreshArgs.devOptForceClientComposition);

    bool hasColorFade = false;
    for (auto* outputLayer : output->getOutputLayersOrderedByZ()) {
        std::string currentLayerName = outputLayer->getLayerFE().getDebugName();
        if (currentLayerName.find("ColorFade") != std::string::npos) {
            ALOGD("%s layer[%s] ColorFade\n", __FUNCTION__, currentLayerName.c_str());
            hasColorFade = true;
       }
    }

    for (auto* outputLayer : output->getOutputLayersOrderedByZ()) {
        if (mPlatform_8250) {
            if (mFodFlag == 0) {
                outputLayer->miFodLayer |= MI_FOD_UNLOCK_SUCCESS;
            } else {
                outputLayer->miFodLayer &= (uint32_t)~MI_FOD_UNLOCK_SUCCESS;
            }
        }

        // Change for FOD
        std::string currentLayerName = outputLayer->getLayerFE().getDebugName();
        if (!hasColorFade) {
            if (currentLayerName.find("hbm_overlay") != std::string::npos
                || currentLayerName.find("enroll_overlay") != std::string::npos){
                ATRACE_BEGIN("hbm_overlay");
                outputLayer->miFodLayer |= MI_FLINGER_LAYER_FOD_HBM_OVERLAY;
                ALOGD("%s FOD Add hbm_overlay", __FUNCTION__);
                ATRACE_END();
            } else if (currentLayerName.find("gxzw_icon") != std::string::npos) {
                ATRACE_BEGIN("gxzw_icon");
                outputLayer->miFodLayer |= MI_FLINGER_LAYER_FOD_ICON;
                ALOGV("%s FOD Add gxzw_icon", __FUNCTION__);
                ATRACE_END();
            } else if (!mPlatform_8250 && currentLayerName.find("gxzw_anim") != std::string::npos) {
                ATRACE_BEGIN("gxzw_anim");
                outputLayer->miFodLayer |= MI_FLINGER_LAYER_FOD_ANIM;
                ALOGV("%s FOD Add gxzw_anim", __FUNCTION__);
                ATRACE_END();
            } else if (mPlatform_8250 && mLocalHbmEnabled && currentLayerName.find("gxzw_anim") != std::string::npos) {
                 ATRACE_BEGIN("gxzw_anim");
                 outputLayer->miFodLayer |= MI_FLINGER_LAYER_FOD_ANIM;
                 ALOGV("%s FOD Add gxzw_anim", __FUNCTION__);
                 ATRACE_END();
            } else if ((currentLayerName.find("AOD") != std::string::npos)
                && !(currentLayerName.find(" AOD") != std::string::npos)){
                outputLayer->miFodLayer |= MI_FLINGER_LAYER_AOD_LAYER;
            }
        } else {
            if (currentLayerName.find("hbm_overlay") != std::string::npos
                || currentLayerName.find("enroll_overlay") != std::string::npos){
                outputLayer->miFodLayer &= (uint32_t)~MI_FLINGER_LAYER_FOD_HBM_OVERLAY;
                ALOGV("%s Clear MI_LAYER_FOD_HBM_OVERLAY flag", __FUNCTION__);
            } else if (currentLayerName.find("gxzw_icon") != std::string::npos) {
                outputLayer->miFodLayer &= (uint32_t)~MI_FLINGER_LAYER_FOD_ICON;
                ALOGV("%s Clear MI_LAYER_FOD_ICON flag", __FUNCTION__);
            } else if (!mPlatform_8250 && currentLayerName.find("gxzw_anim") != std::string::npos) {
                outputLayer->miFodLayer &= (uint32_t)~MI_FLINGER_LAYER_FOD_ANIM;
                ALOGV("%s Clear MI_LAYER_FOD_ANIM flag", __FUNCTION__);
            } else if (mPlatform_8250 && mLocalHbmEnabled && currentLayerName.find("gxzw_anim") != std::string::npos) {
                 outputLayer->miFodLayer &= (uint32_t)~MI_FLINGER_LAYER_FOD_ANIM;
                 ALOGV("%s Clear MI_LAYER_FOD_ANIM flag", __FUNCTION__);
            } else if ((currentLayerName.find("AOD") != std::string::npos)
                && !(currentLayerName.find(" AOD") != std::string::npos)){
                outputLayer->miFodLayer &= (uint32_t)~MI_FLINGER_LAYER_AOD_LAYER;
            }
        }
    }
}

void MiSurfaceFlingerImpl::updateFodFlag(int32_t flag) {
    if (!mIsFodFingerPrint || mLocalHbmEnabled)
        return;

    /* mFodFlag
       1: fod monitor status;
       0: fod authenticated, fod not monitor;
       2: other authenticated, fod not monitor.
    */
    ALOGD("%s FodFlag=%d", __func__, flag);
    static int fodScreenOn = 0;

    if (flag == 1) {
        if (mPlatform_8250) {
            mDisplayFeatureGet->setFODFlag(false);
        }

        {
            Mutex::Autolock _l(mFogFlagMutex);
            fodScreenOn += 1;
            if (fodScreenOn == 2) {
                mFodFlag = flag;
            }
        }
        if (mDisplayFeatureGet) {
            mDisplayFeatureGet->setFeature(0, (int32_t)FeatureId::FOD_MONITOR_ON_STATE, flag, 1);
        }
    } else if (flag == 0 || flag == 2) {
        fodScreenOn = 0;

        if (mPlatform_8250) {
            mDisplayFeatureGet->setFODFlag(true);
        }

        if (mDisplayFeatureGet) {
            mDisplayFeatureGet->setFeature(0, (int32_t)FeatureId::FOD_MONITOR_ON_STATE, flag, 0);
        }
        {
            Mutex::Autolock _l(mFogFlagMutex);
            mFodFlag = flag;
        }

        if (mPlatform_8250) {
            if (mBacklightMonitorThread != NULL) {
                Mutex::Autolock _l(mFlinger->mStateLock);
                updateHBMOverlayAlphaLocked(static_cast<int>(mBacklightMonitorThread->getCurrentBrightness()));
            }
        } else {
            if (mFodMonitorThread != NULL) {
                Mutex::Autolock _l(mFlinger->mStateLock);
                updateHBMOverlayAlphaLocked(mFodMonitorThread->getCurrentBrightness());
            }
        }
    }

    mFlinger->scheduleRepaint();
}

int MiSurfaceFlingerImpl::isFod() {
    Mutex::Autolock _l(mFogFlagMutex);
    return mFodFlag;
}
void MiSurfaceFlingerImpl::updateLHBMFodFlag(int32_t flag) {
    if (!mLocalHbmEnabled || !mIsFodFingerPrint)
        return;

    ALOGD("%s FodFlag=%d", __func__, flag);

    if (flag == ENROLL_START || flag == AUTH_START) {
        if (mPlatform_8250) {
            mDisplayFeatureGet->setFODFlag(false);
        }

       if(mFodFlag == 0) {
            {
                Mutex::Autolock _l(mFogFlagMutex);
                mFodFlag = 1;
            }
            if (mSetQsyncFlag) {
                updateScene(FPS_SWITCH_MAP_QSYNC, 0, (String8)"FOD");
            }
            mSetIdleForceFlag = true;
            forceSkipIdleTimer(true);
            if (mFodMonitorFrameRate != 0) {
                mCurrentSettingFps = getCurrentSettingFps();
                ALOGD("%s mCurrentSettingFps = %d", __func__, mCurrentSettingFps);
                changeFps((int32_t)mFodMonitorFrameRate, true);
            }
        }
        if (mDisplayFeatureGet) {
            mDisplayFeatureGet->setFeature(0, (int32_t)FeatureId::FOD_MONITOR_ON_STATE, flag, 1);
        }
    } else if (flag == FINGERPRINT_NONE || flag == ENROLL_STOP|| flag == AUTH_STOP) {
        if (mPlatform_8250) {
            mDisplayFeatureGet->setFODFlag(true);
        }

        if (mDisplayFeatureGet) {
            mDisplayFeatureGet->setFeature(0, (int32_t)FeatureId::FOD_MONITOR_ON_STATE, flag, 0);
        }

        if(mFodFlag == 1) {
            {
                Mutex::Autolock _l(mFogFlagMutex);
                mFodFlag = 0;
            }
            if (mSetQsyncFlag) {
                updateScene(FPS_SWITCH_MAP_QSYNC, 1, (String8)"FOD");
            }
            else {
                mSetIdleForceFlag = false;
                forceSkipIdleTimer(false);
                if (mFodMonitorFrameRate != 0) {
                    if (mUnsetSettingFps != 0)
                        mCurrentSettingFps = mUnsetSettingFps;
                    if (mCurrentSettingFps < 60)
                        mCurrentSettingFps = 60;
                    changeFps(mCurrentSettingFps, false);
                    ALOGD("%s restore to mCurrentSettingFps = %d", __func__, mCurrentSettingFps);
                }
            }
        }

        if (mPlatform_8250) {
            if (mBacklightMonitorThread != NULL) {
                Mutex::Autolock _l(mFlinger->mStateLock);
                updateHBMOverlayAlphaLocked(static_cast<int>(mBacklightMonitorThread->getCurrentBrightness()));
            }
        } else {
            if (mFodMonitorThread != NULL) {
                Mutex::Autolock _l(mFlinger->mStateLock);
                updateHBMOverlayAlphaLocked(mFodMonitorThread->getCurrentBrightness());
            }
        }

    }

    mFlinger->scheduleRepaint();
}

void MiSurfaceFlingerImpl::updateSystemUIFlag(int32_t flag) {
    if (!mIsFodFingerPrint)
        return;

    ALOGD("%s SystemUI Flag=%d", __func__, flag);

    if (flag == 0 && mFodSystemBacklight != -1) {
        //ALOGD("writeBacklightClone %d", mFodSystemBacklight);
        //mFodMonitorThread->writeBacklightClone(mFodSystemBacklight);

        if (mFPMessageThread != nullptr)
            mFPMessageThread->saveSurfaceCompose();
    }

    bool needRefresh = false;
    float alpha = 1.0f;
    {
        Mutex::Autolock _l(mFlinger->mStateLock);
        if (flag == 1) {
            alpha = 1.0f; // visiable
        } else if (flag == 0) {
            alpha = 0.0f; // non visiable
        }

        sp<const DisplayDevice> displayDevice = FTL_FAKE_GUARD(mFlinger->mStateLock, mFlinger->getDefaultDisplayDeviceLocked());
        mFlinger->mCurrentState.traverseInZOrder([&](Layer* layer) {
            if (layer->belongsToDisplay(displayDevice->getLayerStack())) {
                if (layer->getName().find("StatusBar") != std::string::npos
                        && !(layer->getName().find(" StatusBar") != std::string::npos)
                        && layer->getName().length() < sizeof("StatusBar") + 4) {
                    ALOGD("StatusBar setAlpha=%f\n", alpha);
                    layer->setAlpha(alpha);
                    needRefresh = true;
                    mFlinger->setTransactionFlags(eTraversalNeeded);
                }
                if (layer->getName().find("NotificationShade") != std::string::npos
                        && !(layer->getName().find(" NotificationShade") != std::string::npos)
                        && layer->getName().length() < sizeof("NotificationShade") + 4) {
                    ALOGD("NotificationShade setAlpha=%f\n", alpha);
                    layer->setAlpha(alpha);
                    needRefresh = true;
                    mFlinger->setTransactionFlags(eTraversalNeeded);
                }
                if (layer->getName().find("AOD") != std::string::npos
                        && !(layer->getName().find(" AOD") != std::string::npos)
                        && alpha == 0.0f) {
                    ALOGD("AOD setAlpha=%f\n", alpha);
                    layer->setAlpha(alpha);
                    needRefresh = true;
                    mFlinger->setTransactionFlags(eTraversalNeeded);
                }
            }
        });
    }

    if (needRefresh) {
        mFlinger->scheduleRepaint();
    }
}

void MiSurfaceFlingerImpl::saveFodSystemBacklight(int backlight) {
    if (!mIsFodFingerPrint)
        return;

    ALOGD("mFodSystemBacklight=%d", backlight);
    mFodSystemBacklight = backlight;
}

void MiSurfaceFlingerImpl::updateHBMOverlayAlphaLocked(int brightness) NO_THREAD_SAFETY_ANALYSIS {
//    mIsFodFingerPrint = 0;
    if (!mIsFodFingerPrint)
        return;

    sp<const DisplayDevice> displayDevice = mFlinger->getDefaultDisplayDeviceLocked();
    hal::PowerMode mode;

    bool needRefresh = false;
    {
        half alpha = 0.0_hf;
        mode = displayDevice->getPowerMode();

        if (mode == hal::PowerMode::ON) {
            if (brightness == 0) {
                return;
            } else {
                mBrightnessBackup = brightness;
            }
        }

        alpha = static_cast<float>(getHBMOverlayAlphaLocked());
        if (!mLocalHbmEnabled)
            ALOGD("brightness=%d alpha=%f\n", brightness, static_cast<float>(alpha));

        mFlinger->mCurrentState.traverseInZOrder([&](Layer* layer) {
            if ((layer->getName().find("hbm_overlay") != std::string::npos
                    && !(layer->getName().find(" hbm_overlay") != std::string::npos))
                    || (layer->getName().find("enroll_overlay") != std::string::npos
                    && !(layer->getName().find(" enroll_overlay") != std::string::npos))) {
                if (mode == hal::PowerMode::ON
                        || ((mode == hal::PowerMode::DOZE || mode == hal::PowerMode::DOZE_SUSPEND)
                        && layer->getDrawingState().color.a != 0.0_hf)) {
                    if (alpha != layer->getAlpha()) {
                        ALOGD("hbm_overlay alpha changed, need to update alpha = %f", static_cast<float>(alpha));
                        layer->setAlpha(alpha);
                        needRefresh = true;
                        mFlinger->setTransactionFlags(eTraversalNeeded);
                    }
                }
            }
        });
    }

    if (needRefresh) {
        mFlinger->setTransactionFlags(eTraversalNeeded);
        mFlinger->scheduleRepaint();
    }
}

void MiSurfaceFlingerImpl::updateIconOverlayAlpha(half alpha) {
    ALOGD("updateIconOverlayAlpha %f\n", static_cast<float>(alpha));
    bool needRefresh = false;
    {
        Mutex::Autolock _l(mFlinger->mStateLock);
        mFlinger->mCurrentState.traverseInZOrder([&](Layer* layer) {
            if (layer->getName().find("gxzw_icon") != std::string::npos
                    && !(layer->getName().find(" gxzw_icon") != std::string::npos)) {
                ALOGD("updateIconOverlayAlpha setAlpha=%f\n", static_cast<float>(alpha));
                layer->setAlpha(alpha);
                needRefresh = true;
                mFlinger->setTransactionFlags(eTraversalNeeded);
            }
        });
    }

    if (needRefresh) {
        if (mPlatform_8250) {
            if (mBacklightMonitorThread != NULL) {
                Mutex::Autolock _l(mFlinger->mStateLock);
                updateHBMOverlayAlphaLocked(static_cast<int>(mBacklightMonitorThread->getCurrentBrightness()));
            }
        } else {
            if (mFodMonitorThread != NULL) {
                Mutex::Autolock _l(mFlinger->mStateLock);
                updateHBMOverlayAlphaLocked(mFodMonitorThread->getCurrentBrightness());
             }

            if (mFodMonitorThread != NULL) {
                Mutex::Autolock _l(mFlinger->mStateLock);
                updateHBMOverlayAlphaLocked(mFodMonitorThread->getCurrentBrightness());
            }
        }
        mFlinger->scheduleRepaint();
    }
}

void MiSurfaceFlingerImpl::prePowerModeFOD(hal::PowerMode mode) NO_THREAD_SAFETY_ANALYSIS {
    if (!mIsFodFingerPrint)
        return;

    ALOGD("prePowerModeFOD %d", mode);

    if (mFingerprintStateManager != nullptr) {
        Mutex::Autolock lock(mFingerprintStateManager->mFingerprintStateManagerMutex);
        mPrePowerMode = mode;
        mFingerprintStateManager->fingerprintConditionUpdate(FingerprintStateManager::DISPLAY_POWER_STATE, (int32_t)mPrePowerMode);
    } else {
        mPrePowerMode = mode;
    }

}

void MiSurfaceFlingerImpl::postPowerModeFOD(hal::PowerMode mode) NO_THREAD_SAFETY_ANALYSIS {
    if (mPowerMode == hal::PowerMode::OFF && mode != hal::PowerMode::OFF) {
        resetFpsStatTimer();
    }
    mPowerMode = mode;
    if (!mIsFodFingerPrint)
        return;

    int backlight;

    ALOGD("postPowerModeFOD %d", mode);

    if (mode == hal::PowerMode::ON) {
        mFodPowerStateRollOver = true;
        if (!mPlatform_8250) {
            mFodSystemBacklight = -1;
        }
    }

    if (mPlatform_8250) {
        if (mBacklightMonitorThread != NULL) {
            if (mBacklightMonitorThread->getCurrentBrightness() == 0 && mode == hal::PowerMode::ON) {
                if (mFodSystemBacklight == 1)
                    backlight = 8;
                else{
                    backlight = mFodSystemBacklight;
                    updateHBMOverlayAlphaLocked(backlight);
                }
            } else {
                updateHBMOverlayAlphaLocked(static_cast<int>(mBacklightMonitorThread->getCurrentBrightness()));
            }
        }
    } else {
        if (mFodMonitorThread != NULL) {
            if (mFodMonitorThread->getCurrentBrightness() == 0 && mode == hal::PowerMode::ON) {
                if (mFodSystemBacklight == -1)
                    backlight = 8;
                else{
                    backlight = mFodSystemBacklight;
                    updateHBMOverlayAlphaLocked(backlight);
                }
            } else {
                updateHBMOverlayAlphaLocked(mFodMonitorThread->getCurrentBrightness());
            }
        }
    }
}

void MiSurfaceFlingerImpl::notifySaveSurface(String8 name) {
    if (!mIsFodFingerPrint)
        return;

    ALOGD("%s: Surface name=%s", __FUNCTION__, name.c_str());
    mSaveSurfaceName = name;
    mFodPowerStateRollOver = false;

    if (mPlatform_8250) {
        bool needRefresh = false;
        if (name == String8("com.miui.home/com.miui.home.launcher.Launcher")) {
            Mutex::Autolock _l(mFlinger->mStateLock);
            mFlinger->mCurrentState.traverseInZOrder([&](Layer* layer) {
                if (layer->getName().find("com.miui.home/com.miui.home.launcher.Launcher") != std::string::npos
                    && !(layer->getName().find(" com.miui.home/com.miui.home.launcher.Launcher") != std::string::npos)) {
                    ALOGD("Launcher setAlpha=0 mBlockLauncher");
                    mBlockLauncher = true;
                    layer->setAlpha(0.01f);
                    needRefresh = true;
                    mFlinger->setTransactionFlags(eTraversalNeeded);
                }
            });
        }

        if (needRefresh) {
            mFlinger->scheduleRepaint();
        }
    }
}

unsigned long MiSurfaceFlingerImpl::getCurrentBrightness() {
    if (mPlatform_8250 && mBacklightMonitorThread != NULL) {
        return mBacklightMonitorThread->getCurrentBrightness();
    } else if (mFodMonitorThread != NULL) {
        return mFodMonitorThread->getCurrentBrightnessClone();
    } else {
        return 0;
    }
}

void MiSurfaceFlingerImpl::lowBrightnessFOD(int32_t state) {
     if (!mIsFodFingerPrint)
        return;

    ALOGD("%s state=%d", __func__, state);

    if (mDisplayFeatureGet) {
        mDisplayFeatureGet->setFeature(0, (int32_t)FeatureId::FOD_LOWBRIGHTNESS_FOD_STATE, state, 0);
    }

    if (mFingerprintStateManager != nullptr) {
        Mutex::Autolock lock(mFingerprintStateManager->mFingerprintStateManagerMutex);
        mFingerprintStateManager->fingerprintConditionUpdate(FingerprintStateManager::FOD_LOWBRIGHTNESS_ENABLE_STATE, state);
    }
}


void MiSurfaceFlingerImpl::writeFPStatusToKernel(int fingerPrintStatus,
                                                 int lowBrightnessFOD) {
    if (mPlatform_8250) {
        ALOGV("%s fingerPrintStatus=%d, lowBrightnessFOD=%d", __func__, fingerPrintStatus, lowBrightnessFOD);
        if(mLocalHbmEnabled)
        {
            mFingerprintStatus = fingerPrintStatus;
            if (mFodMonitorThread) {
                mFodMonitorThread->WriteFPStatusToKernel(fingerPrintStatus|(lowBrightnessFOD << 3));
            }
        }
    } else {
        ALOGD("%s fingerPrintStatus=%d, lowBrightnessFOD=%d", __func__, fingerPrintStatus, lowBrightnessFOD);

        mFingerprintStatus = fingerPrintStatus;

        if (mFodMonitorThread) {
            mFodMonitorThread->writeFPStatusToKernel(fingerPrintStatus, lowBrightnessFOD);
        }
    }
}


bool MiSurfaceFlingerImpl::validateLayerSize(uint32_t w, uint32_t h) {
    unsigned long const maxSurfaceDims =
    std::min(mFlinger->getRenderEngine().getMaxTextureSize(), mFlinger->getRenderEngine().getMaxViewportDims());

    // never allow a surface larger than what our underlying GL implementation
    // can handle.
    if ((uint32_t(w) > maxSurfaceDims) || (uint32_t(h) > maxSurfaceDims)) {
        ALOGE("dimensions too large %u x %u", uint32_t(w), uint32_t(h));
        return false;
    }

    return true;
}

void MiSurfaceFlingerImpl::getHBMLayerAlpha(Layer* layer, float& alpha) {
    if (mIsFodFingerPrint) {
        if ((layer->getName().find("hbm_overlay") != std::string::npos
                && !(layer->getName().find(" hbm_overlay") != std::string::npos))
                    || (layer->getName().find("enroll_overlay") != std::string::npos
                    && !(layer->getName().find(" enroll_overlay") != std::string::npos))) {
            //ALOGD("---Before setAlpha = %f", alpha);
            if (alpha != 0.0f) {
                alpha = static_cast<float>(getHBMOverlayAlphaLocked());
                //ALOGD("---After setAlpha = %f", alpha);
            } else {
                ALOGD("setAlpha hbm_overlay (alpha==0)");
            }
        }

        if (mPlatform_8250) {
            if (layer->getName().find("com.miui.home/com.miui.home.launcher.Launcher") != std::string::npos
                  && !(layer->getName().find(" com.miui.home/com.miui.home.launcher. ") != std::string::npos)) {
                if (mBlockLauncher) {
                    ALOGD("Launcher Blocking setAlpha=%f", alpha);
                    alpha = 0.01f;
                }
                ALOGD("---setAlpha Launcher=%f", alpha);
            }
        } else {
            if (mLocalHbmEnabled
                && (layer->getName().find("enroll_overlay") != std::string::npos
                && !(layer->getName().find(" enroll_overlay") != std::string::npos))) {
                alpha = 0.1f;
            }

            if (mLocalHbmEnabled
                && layer->getName().find("gxzw_icon") != std::string::npos
                && !(layer->getName().find(" gxzw_icon") != std::string::npos)) {
                alpha = 0.0f;
            }
        }
    }
}

void MiSurfaceFlingerImpl::forceSkipTouchTimer(bool skip) {
    {
         std::lock_guard<std::mutex> lock(mFlinger->mScheduler->mPolicyLock);
         mFlinger->mScheduler->mPolicy.isTouchTimerSkip = skip;
         ALOGD("isTouchTimerSkip is %d", mFlinger->mScheduler->mPolicy.isTouchTimerSkip);
    }

    mFlinger->mScheduler->onTouchHint();
}

void MiSurfaceFlingerImpl::forceSkipIdleTimer(bool skip) {
    const auto display = getActiveDisplayDevice();

    if (!mFlinger->mScheduler->holdRefreshRateConfigs()->mIdleTimer)
         return;

    // when ultimate performance is opened, need to disable smartfps setting.
    skip = skip || mSetUltimatePerfFlag;
    if (!mPlatform_8250) {
        if (!skip ) {
            if (mFodMonitorThread->getCurrentBrightnessClone() <= mSkipIdleThreshold) {
                if (display && !display->isPrimary()) {
                    if (mIsLtpoPanel) {
                        // L18 Sec Panel is run
                        ALOGD("Avoid sec display panel set idle framerate in this brightness");
                        return ;
                    }
                }
                else {
                    if ( !mIsLtpoPanel || mIsSupportSkipIdle) {
                        return ;
                    }
                }
            }
            if (mSetIdleForceFlag || mSetLockFpsFlag || mSetLockFpsPowerFullFlag || mIsCts || mSetGameFlag) {
                return;
            }
        }
    } else {
        if(!skip && mBacklightMonitorThread->getCurrentBrightness() <= mSkipIdleThreshold) {
            return;
        }

        if (!skip && mIsSupportSmartpenIdle) {
            if (mPenMonitorThread->getPenStatus()) {
                return;
            }
        }
    }

    {
         std::lock_guard<std::mutex> lock(mFlinger->mScheduler->mPolicyLock);
         mFlinger->mScheduler->mPolicy.isIdleTimerSkip = skip;
         ALOGD("isIdleTimerSkip is %d,mSetIdleForceFlag = %d", mFlinger->mScheduler->mPolicy.isIdleTimerSkip,mSetIdleForceFlag);
    }

    mFlinger->mScheduler->resetIdleTimer();
}

bool MiSurfaceFlingerImpl::isForceSkipIdleTimer(){
    return mFlinger->mScheduler->mPolicy.isIdleTimerSkip;
}

bool MiSurfaceFlingerImpl::isMinIdleFps(nsecs_t period) {
    bool isMinIdleFps = false;
    if (100 == period / 1e6f) {
        isMinIdleFps = true;
    }
    return isMinIdleFps;
}

nsecs_t MiSurfaceFlingerImpl::setMinIdleFpsPeriod(nsecs_t period) {
    return (nsecs_t)(833 * period / 10000);
}

bool MiSurfaceFlingerImpl::isLtpoPanel() {
    return mIsLtpoPanel;
}
bool MiSurfaceFlingerImpl::allowCtsPass(std::string name) {
    return name.find("cts.device.graphicsstats") != std::string::npos || name.find("android.camera.cts") != std::string::npos;
}

bool MiSurfaceFlingerImpl::isDdicIdleModeGroup(int32_t modeGroup) {
    return ((modeGroup & MI_DDIC_MODE_GROUP_MASK) == MI_DDIC_IDLE_MODE_GROUP);
}

bool MiSurfaceFlingerImpl::isDdicAutoModeGroup(int32_t modeGroup) {
    return ((modeGroup & MI_DDIC_MODE_GROUP_MASK) == MI_DDIC_AUTO_MODE_GROUP);
}

bool MiSurfaceFlingerImpl::isDdicQsyncModeGroup(int32_t modeGroup) {
    return ((modeGroup & MI_DDIC_MODE_GROUP_MASK) == MI_DDIC_QSYNC_MODE_GROUP);
}

bool MiSurfaceFlingerImpl::isNormalModeGroup(int32_t modeGroup) {
    return ((modeGroup & MI_DDIC_MODE_GROUP_MASK) == 0);
}

uint32_t MiSurfaceFlingerImpl::getDdicMinFpsByGroup(int32_t modeGroup) {
    uint32_t ddicMinFpsMask = 0xFF << 8;
    return (((uint32_t)modeGroup & ddicMinFpsMask) >> 8);
}

int32_t MiSurfaceFlingerImpl::getGroupByBrightness(int32_t fps, int32_t modeGroup) {
    int32_t mode_group = modeGroup & 0xFF;
    ALOGD("mIsSupportAutomodeForMaxFpsSetting is %d, getCurrentBrightness() = %d(%d)",
                   mIsSupportAutomodeForMaxFpsSetting, getCurrentBrightness(), mSkipIdleThreshold);
    if (mIsSupportAutomodeForMaxFpsSetting && getCurrentBrightness() > mSkipIdleThreshold && MI_SETTING_FPS_120HZ == fps && isAutoModeAllowed()) {
        mode_group = MI_DDIC_AUTO_MODE_GROUP | (fps << 16) | (MI_IDLE_FPS_1HZ << 8) | mode_group;
    }
    return mode_group;
}

DisplayModePtr MiSurfaceFlingerImpl::setIdleFps(int32_t mode_group, bool *minFps) {
    DisplayModePtr ret = nullptr;
    if  (!mFlinger || !mFlinger->mBootFinished) {
        goto error;
    }
    if (mIsLtpoPanel) {
        float mIdleFrameRateTemp;
        if (MI_IDLE_FPS_10HZ == mIdleFrameRate) {
            mIdleFrameRateTemp = MI_IDLE_FPS_120HZ;
            mode_group = MI_DDIC_IDLE_MODE_GROUP | (MI_IDLE_FPS_120HZ << 16) | (mIdleFrameRate << 8) | (mode_group & 0xFF);
        }
        else if (MI_IDLE_FPS_1HZ == mIdleFrameRate) {
            mIdleFrameRateTemp = MI_IDLE_FPS_120HZ;
            mode_group = MI_DDIC_IDLE_MODE_GROUP | (MI_IDLE_FPS_120HZ << 16) | (mIdleFrameRate << 8) | (mode_group & 0xFF);
        } else {
            mIdleFrameRateTemp = mIdleFrameRate;
        }

        const auto display = getActiveDisplayDevice();
        if (!display) {
            goto error;
        }
        const auto supportedModes = display->getSupportedModes();
        if (supportedModes.size() <= 0) {
            goto error;
        }

        bool is_valid = false;
        Fps idleFps = Fps::fromValue(mIdleFrameRateTemp);
        for (const auto& mode : supportedModes) {
            if (mode.second->getFps() == idleFps && mode.second->getGroup() == mode_group) {
                is_valid = true;
                break;
            }
        }

        if (!is_valid) {
            ALOGD("idle fps(%f) is invalid!", mIdleFrameRate);
            goto error;
        }

        for (auto& iter: mFlinger->mScheduler->mRefreshRateConfigs->getAllRefreshRates()) {
            if (iter.second->getFps() == Fps::fromValue(mIdleFrameRateTemp) && (iter.second->getGroup() == mode_group)) {
                ALOGD("setIdleFps choose FPS:%d, mode:%d", iter.second->getFps().getIntValue(), mode_group);
                DisplayModePtr refreshrate = iter.second;
                return refreshrate;
            }
        }
    }
error:
    *minFps = true;
    return ret;
}

DisplayModePtr MiSurfaceFlingerImpl::setVideoFps(int32_t mode_group) {
    if (mIsSupportVideoOrCameraFrameRate) {
        float mVideoFps = MI_VIDEO_FPS_60HZ;
        {
            Mutex::Autolock _l(mVideoDecoderMutex);
            if (!mVideoDecoderFps.empty()) {
                if (mSetShotVideoFlag) {
                    mVideoFps = mVideoDecoderFrameRateReal;
                }
                else {
                    mVideoFps = mVideoDecoderFrameRate > mVideoFrameRate ? mVideoDecoderFrameRate : mVideoFrameRate;
                    ALOGV("setVideoFps choose FPS:%f(%f, %f)", mVideoFps, mVideoDecoderFrameRate, mVideoFrameRate);
                    if((int)mVideoDecoderFrameRate < MI_VIDEO_FPS_60HZ) {
                        if (mVideoDecoderFrameRate > mVideoFrameRate && mVideoDecoderFrameRateReal == mVideoFrameRate) {
                            mVideoFps = mVideoDecoderFrameRate;
                            ALOGV("setVideoFps hasn't bullet choose FPS:%f(%f, %f)", mVideoFps, mVideoDecoderFrameRate, mVideoFrameRate);
                        }
                        else if(mVideoDecoderFrameRate == mVideoFrameRate && mVideoDecoderFrameRateReal < mVideoFrameRate) {
                            mVideoFps = MI_VIDEO_FPS_60HZ;
                            ALOGV("setVideoFps has bullet choose FPS:%f(%f, %f)", mVideoFps, mVideoDecoderFrameRate, mVideoFrameRate);
                        }
                    }
                }
            }
        }
        for (auto& iter: mFlinger->mScheduler->mRefreshRateConfigs->getAllRefreshRates()) {
            if (iter.second->getFps() == Fps::fromValue(mVideoFps) && (iter.second->getGroup() == mode_group)) {
                ALOGD("setVideoFps choose FPS:%d(%f, %f), mode:%d", iter.second->getFps().getIntValue(), mVideoDecoderFrameRate, mVideoFrameRate, mode_group);
                DisplayModePtr refreshrate = iter.second;
                return refreshrate;
            }
        }
    }
    auto iter = mFlinger->mScheduler->mRefreshRateConfigs->getAllRefreshRates().cbegin();
    DisplayModePtr minrefreshrate = iter->second;
    return minrefreshrate;
}

DisplayModePtr MiSurfaceFlingerImpl::setTpIdleFps(int32_t mode_group) {
    mode_group = mIsSupportAutomodeTpIdle ? MI_DDIC_AUTO_MODE_GROUP | (MI_IDLE_FPS_60HZ << 16) | (MI_IDLE_FPS_1HZ << 8) | mode_group : mode_group;
    for (auto& iter: mFlinger->mScheduler->mRefreshRateConfigs->getAllRefreshRates()) {
        if (iter.second->getFps() == Fps::fromValue(MI_IDLE_FPS_60HZ) && (iter.second->getGroup() == mode_group)) {
            ALOGD("setTpIdleFps choose FPS:%d, mode:%d", iter.second->getFps().getIntValue(), mode_group);
            DisplayModePtr refreshrate = iter.second;
            return refreshrate;
        }
    }
    auto iter = mFlinger->mScheduler->mRefreshRateConfigs->getAllRefreshRates().cbegin();
    DisplayModePtr minrefreshrate = iter->second;
    return minrefreshrate;
}

DisplayModePtr MiSurfaceFlingerImpl::setPromotionModeId(int32_t defaultMode) {
    scheduler::RefreshRateConfigs::GlobalSignals consideredSignals;
    DisplayModePtr newModePtr;
    std::tie(newModePtr, consideredSignals) = mFlinger->mScheduler->chooseDisplayMode();
    DisplayModeId newModeId = newModePtr->getId();
    DisplayModeId defaultModeId = DisplayModeId(defaultMode);
    DisplayModePtr defaultModePtr = mFlinger->mScheduler->mRefreshRateConfigs->getAllRefreshRates().get(defaultModeId)->get();

    ALOGD("setPromotionModeId prev newModeId = %d \n", newModeId.value());
    const bool touchActive = mFlinger->mScheduler->mTouchTimer && mFlinger->mScheduler->mPolicy.touch == scheduler::Scheduler::TouchState::Active;
    if (newModePtr->getFps().getValue() > defaultModePtr->getFps().getValue() && !touchActive) {
        newModeId = defaultModeId;
    }
    if (mFlinger->mScheduler->mRefreshRateConfigs->getAllRefreshRates().get(newModeId)->get()->getFps().getValue() < mIdleFrameRate) {
        newModeId = DisplayModeId(0);
    }
    {
        std::lock_guard<std::mutex> lock(mFlinger->mScheduler->mPolicyLock);
        if (mIsSkipPromotionFpsfor_90hz && (MI_IDLE_FPS_90HZ == defaultModePtr->getFps().getIntValue())) {
            newModeId = mLastPromotionModeId;
            ALOGD("setPromotionModeId skip, defaultMode %d, newModeId = %d \n", defaultMode, newModeId.value());
        }
        mLastPromotionModeId = defaultModeId;
        mFlinger->mScheduler->mPolicy.mode = mFlinger->mScheduler->mRefreshRateConfigs->getAllRefreshRates().get(newModeId)->get();
    }
    ALOGD("setPromotionModeId defaultMode %d, newModeId = %d \n", defaultMode, newModeId.value());
    return mFlinger->mScheduler->mPolicy.mode;
}

bool MiSurfaceFlingerImpl::isResetIdleTimer() {
     bool resetIdleTimer = true;
     if (mIsSupportTouchIdle) {
         resetIdleTimer = false;
         if (mSetTpIdleFlag) {
             if (mLayerRealUpdateFlag1 || mLayerRealUpdateFlag2) {
                 resetIdleTimer = true;
             }
             else {
                 resetIdleTimer = false;
             }
         }
     }
     return resetIdleTimer;
}

int MiSurfaceFlingerImpl::getCurrentPolicyNoLocked(){
    return mFlinger->mScheduler->mRefreshRateConfigs->getCurrentPolicyNoLocked().primaryRange.max.getIntValue();
}

bool MiSurfaceFlingerImpl::isVideoScene() {
     return (mSetVideoFlag || mSetCameraFlag || mSetShotVideoFlag || mSetVideoHighFpsFlag) && mIsSupportVideoOrCameraFrameRate && !mTouchflag &&
     isSupportLowBrightnessFpsSwitch(getCurrentPolicyNoLocked());
}

bool MiSurfaceFlingerImpl::isSupportLowBrightnessFpsSwitch(int32_t current_setting_fps) {
    bool isSupportLowBrightness = true;

    if (mIsLtpoPanel) {
        const auto display = getActiveDisplayDevice();
        if (display && !display->isPrimary()) {
            if (getCurrentBrightness() < mSkipIdleThreshold) {
                isSupportLowBrightness = false;
            }
        }
        else {
            if (!mIsSupportTpIdleLowBrightness && (MI_IDLE_FPS_90HZ == current_setting_fps)) {
                if (getCurrentBrightness() < mSkipIdleThreshold) {
                    isSupportLowBrightness = false;
                }
            }
        }
    }
    else {
        if (getCurrentBrightness() < mSkipIdleThreshold) {
            isSupportLowBrightness = false;
        }
    }

    if (mIsSupportSmartpenIdle) {
        if (mPenMonitorThread->getPenStatus()) {
            isSupportLowBrightness = false;
        }
        else {
            isSupportLowBrightness = true;
        }
    }

    return isSupportLowBrightness;
}

bool MiSurfaceFlingerImpl::isTpIdleScene(int32_t current_setting_fps) {
     return !mSetUltimatePerfFlag && mSetLtpoTpIdleFlag && !mTouchflag && isSupportLowBrightnessFpsSwitch(current_setting_fps);
}

const char* MiSurfaceFlingerImpl::getSceneModuleName(const scene_module_t& module) const {
    switch (module) {
        case CAMERA_SCENE: return "Camera";
        case VIDEO_SCENE:return "Video";
        case TP_IDLE_SCENE:return "TpIdle";
        case PROMOTION_SCENE:return "Promotion";
        case AOD_SCENE:return "AOD";
        case SHORT_VIDEO_SCENE:return "SHORT_VIDEO";
        case STATUSBAR_SCENE:return "StatusBar";
        default: return "Unknown";
    }
}

bool MiSurfaceFlingerImpl::isSceneExist(scene_module_t scene_module) {
     bool ret = false;
     switch (scene_module) {
         case CAMERA_SCENE: {
             ret = mSetCameraFlag && mIsSupportVideoOrCameraFrameRate && !mTouchflag;
         } break;
         case VIDEO_SCENE: {
             ret = mSetVideoFlag && mIsSupportVideoOrCameraFrameRate && !mTouchflag;
         } break;
         case TP_IDLE_SCENE: {
             ret = mSetLtpoTpIdleFlag && !mTouchflag;
         } break;
         case PROMOTION_SCENE: {
             ret = mSetPromotionFlag;
         } break;
         case AOD_SCENE: {
             ret = mSetAodFlag;
         } break;
         case SHORT_VIDEO_SCENE: {
             ret = mSetShotVideoFlag;
         } break;
         case STATUSBAR_SCENE: {
             ret = mSetStatusFlag;
         } break;
         case VSYNC_OPTIMIZE_SCENE: {
             ret = mSetVsyncOptimizeFlag;
         } break;
         case FORCE_SKIP_IDLE_FOR_90HZ: {
             ret = mIsSupportSkipIdlefor_90hz;
         } break;
         case FORCE_SKIP_IDLE_FOR_AUTOMODE_120HZ: {
             ret = mIsSupportAutomodeForMaxFpsSetting && getCurrentBrightness() > mSkipIdleThreshold;
         } break;
         case VIDEO_FULL_SCREEN_SCENE: {
             if (mSetVideoFullScreenFlag && (int)mVideoDecoderFrameRate <= MI_VIDEO_FPS_60HZ && !mSetAnimationFlag &&
                 isSupportLowBrightnessFpsSwitch(getCurrentPolicyNoLocked())) {
                 ret = mPreVideoFullScreen;
             }
         } break;
#ifdef QTI_DISPLAY_CONFIG_ENABLED
         case MAP_QSYNC_SCENE: {
             if (mQsyncFeature) {
                 ret = mSetQsyncFlag;
             }
         } break;
#endif
         case FOD_SCENE: {
             ret = isFod();
         } break;
         default:
            break;
     }

     ALOGV("%s: module = %d (%s), value = %d \n", __FUNCTION__,
          scene_module, getSceneModuleName(scene_module), ret);

     return ret;
}

void MiSurfaceFlingerImpl::signalLayerUpdate(uint32_t scense, sp<Layer> layer) {
     if (mIsSupportTouchIdle) {
         if (mSetTpIdleFlag) {
             if (layer->getWindowType()  != WindowInfo::Type::STATUS_BAR) {
                 if (LAYER_REAL_UPDATE_SCENE_1 == scense) {
                     mLayerRealUpdateFlag1 = true;
                 }
                 else if (LAYER_REAL_UPDATE_SCENE_2 == scense) {
                     mLayerRealUpdateFlag2 = true;
                 }
             }
         }
     }
     else {
         if (layer->getWindowType()  != WindowInfo::Type::STATUS_BAR) {
                 if (LAYER_REAL_UPDATE_SCENE_1 == scense) {
                     mLayerRealUpdateFlag1 = true;
                 }
                 else if (LAYER_REAL_UPDATE_SCENE_2 == scense) {
                     mLayerRealUpdateFlag2 = true;
                 }
         }
     }
     ALOGV("%s: scense %d, mLayerRealUpdateFlag %d,%d, layer name:%s", __func__, scense, mLayerRealUpdateFlag1, mLayerRealUpdateFlag2, layer->getName().c_str());
}

void MiSurfaceFlingerImpl::setLayerRealUpdateFlag(uint32_t scense, bool flag) {
     if (LAYER_REAL_UPDATE_SCENE_1 == scense) {
         mLayerRealUpdateFlag1 = flag;
     }
     else if (LAYER_REAL_UPDATE_SCENE_2 == scense) {
         mLayerRealUpdateFlag2 = flag;
     }
}

void MiSurfaceFlingerImpl::setGameArgs(CompositionRefreshArgs& refreshArgs){
    ALOGV("%s: rfreshArgs.gameColorMode: %d", __func__, refreshArgs.gameColorMode);
    if (mGameColorMode) {
        refreshArgs.gameColorMode = LAYER_GAME;
    }else{
        refreshArgs.gameColorMode = LAYER_UNKNOWN;
    }
    Mutex::Autolock _l(mGamePackageNameMutex);
    refreshArgs.gamePackageName = mGamePackageName;
 }

void MiSurfaceFlingerImpl::saveDisplayColorSetting(DisplayColorSetting displayColorSetting){
    ALOGV("displayColorSetting:%d", displayColorSetting);
    mDisplayColorSettingBackup= displayColorSetting;
}

void MiSurfaceFlingerImpl::updateForceSkipIdleTimer(int mode, bool isPrimary){
    ALOGV("%s: mode:%d,isPrimary:%d ", __func__, mode, isPrimary);
    if (!mPlatform_8250) {
        if (mode == HWC_POWER_MODE_DOZE || mode == HWC_POWER_MODE_DOZE_SUSPEND) {
            mSetIdleForceFlag = true;
            forceSkipIdleTimer(true);
        } else if (mode == HWC_POWER_MODE_NORMAL) {
            if (!mFodFlag) {
                mSetIdleForceFlag = false;
                if (mIsLtpoPanel) {
                    if(!isPrimary && mFodMonitorThread->getCurrentBrightnessClone() <= mSkipIdleThreshold) {
                        forceSkipIdleTimer(true);
                        return;
                    }
                    forceSkipIdleTimer(false);
                }
                else {
                    if (mFodMonitorThread->getCurrentBrightnessClone() > mSkipIdleThreshold) {
                        forceSkipIdleTimer(false);
                    }
                }
            }
        }
    }
}

void MiSurfaceFlingerImpl::updateGameColorSetting(String8 gamePackageName, int gameColorMode, DisplayColorSetting& mDisplayColorSetting){
    ALOGV("gamePackageName:%s, gameColorMode:%d, mDisplayColorSetting:%d", gamePackageName.string(), gameColorMode, mDisplayColorSetting);
    int GameColorIntentId[] = {0, 263, 264, 265};
    mGameColorMode = gameColorMode;
    if (mFakeGcp) {
        setDisplayFeatureGameMode(gameColorMode);
        return;
    } else {
        Mutex::Autolock _l(mGamePackageNameMutex);
        mGamePackageName = std::string(gamePackageName.string());
        setDisplayFeatureGameMode(gameColorMode);
        if (gameColorMode == 0) {
            mGamePackageName = std::string("gamelayerdefault");
            mDisplayColorSetting = mDisplayColorSettingBackup;
        } else {
            mDisplayColorSetting = static_cast<DisplayColorSetting>(GameColorIntentId[gameColorMode]);
        }
        mFlinger->scheduleRepaint();
    }
}

void MiSurfaceFlingerImpl::saveDisplayDozeState(unsigned long doze_state){
    ALOGD("mDozeState:%lu", doze_state);
    if (mPlatform_8250) {
        mDozeState = doze_state;
    }
}


void MiSurfaceFlingerImpl::setDisplayFeatureGameMode(int32_t game_mode){
    ALOGD("%s gamemode=%d", __func__, game_mode);
    if (mDisplayFeatureGet) {
        if (mFoldStatus == 1) {
            mDisplayFeatureGet->setFeature(1, (int32_t)FeatureId::GAME_STATE, game_mode, 0);
        } else {
            mDisplayFeatureGet->setFeature(0, (int32_t)FeatureId::GAME_STATE, game_mode, 0);
        }
    }
    else{
        ALOGE("%s mDisplayFeatureGet=NULL", __func__);
    }
}

void MiSurfaceFlingerImpl::launcherUpdate(Layer* layer) {
    if (!layer)
        return;

    ALOGV("%s, layer->name: %s", __func__, layer->getName().c_str());
    if (mPlatform_8250) {
        if (layer->getName().find("com.miui.home/com.miui.home.launcher.Launcher") != std::string::npos
                    && !(layer->getName().find(" com.miui.home/com.miui.home.launcher.Launcher") != std::string::npos)) {
            sp<const DisplayDevice> displayDevice = mFlinger->getDefaultDisplayDevice();
            hal::PowerMode mode = displayDevice->getPowerMode();;

            bool needRefresh = false;
            {
                Mutex::Autolock _l(mFlinger->mStateLock);
                if (mBlockLauncher && (mode == hal::PowerMode::ON)) {
                    mFlinger->mCurrentState.traverseInZOrder([&](Layer* layer) {
                        if (layer->getName().find("com.miui.home/com.miui.home.launcher.Launcher") != std::string::npos
                                && !(layer->getName().find(" com.miui.home/com.miui.home.launcher.Launcher") != std::string::npos)) {
                            ALOGD("launcherUpdate");
                            mBlockLauncher = false;
                            layer->setAlpha(1.0_hf);
                            needRefresh = true;
                            mFlinger->setTransactionFlags(eTraversalNeeded);
                        }
                    });
                }
            }

            if (needRefresh) {
                mFlinger->scheduleRepaint();
            }
        }
    }
}

/*
  * Description:
  *    Android aosp Variable Refresh rate are only allowed in CTS refresh test. In user normal case,
  *    xiaomi vrr solution will take over the switching.
  *
  * TEST:
  *    run cts-on-gsi -m  CtsViewTestCases -t android.view.cts.DisplayRefreshRateTest#testRefreshRate
  * Retult:
  *    I/ModuleListener: [1/1] android.view.cts.DisplayRefreshRateTest#testRefreshRate PASSED
  */
bool MiSurfaceFlingerImpl::allowAospVrr() {
    return mAllowAospVrr;
}

bool MiSurfaceFlingerImpl::isModeChangeOpt() {
    return mIsModeChangeOpt;
}

void MiSurfaceFlingerImpl::notifyDfAodFlag(int32_t flag) {
    ALOGD("%s flag=%d", __func__, flag);
    if (mPlatform_8250) {
        if (mDisplayFeatureGet != NULL) {
            if (flag == 0) {
                ALOGD("%s aod layer disappear", __func__);
                mDisplayFeatureGet->setFeature(0, 25, 3, 0); /* 25-DOZE_BRIGHTNESS_STATE, 3 means aod layer disappear */
            } else if (flag == 1) {
                ALOGD("%s aod layer appear", __func__);
                mDisplayFeatureGet->setFeature(0, 25, 4, 0); /* 25-DOZE_BRIGHTNESS_STATE, 4 means aod layer appear */
            }
        }
    }
}

void MiSurfaceFlingerImpl::notifyDfLauncherFlag(int32_t flag) {
    ALOGD("%s flag=%d", __func__, flag);
    if (mPlatform_8250) {
        if (mSmartFpsEnable) {
            sp<IDisplayFeature> dfClient(mDisplayFeatureGet->getDisplayFeatureService());
            if (dfClient != NULL) {
                dfClient->setFeature(0, 24, flag, 246); /* 24-FPS_SWITCH_STATE, 246-FPS_SWITCH_LAUNCHER */
            }
            ALOGD("%s launcher changed[%d]", __func__, flag);
        }
    }
}

void MiSurfaceFlingerImpl::disableOutSupport(bool* outSupport){
    if (!outSupport)
        return;

    ALOGV("%s, outSupport=%d", __func__, (*outSupport));
    if (mPlatform_8250) {
        (*outSupport)= false;
    }

    if (mForceHWCBrightness) {
         (*outSupport)= true;
         ALOGD("%s, hwc backlight outSupport=%d", __func__, (*outSupport));
    }
}
// BSP-Game: add Game Fps Stat
sp<Layer> MiSurfaceFlingerImpl::getGameLayer(const char* activityName) {
    Mutex::Autolock lock(mFlinger->mStateLock);
    bool isSurfaceView = false;
    bool isBlast = false;
    Layer* rightLayer = nullptr;
    String8 surfaceView = String8("SurfaceView[");
    String8 bgSurface = String8("Background for SurfaceView[");
    String8 blastSurface = String8("BLAST");
    sp<Layer> rightLayerSp;

    mFlinger->mCurrentState.traverseInZOrder([&](Layer* layer) {
        if (!isBlast &&
                layer->getName().find(activityName) != std::string::npos) {
            if (!isSurfaceView)
                rightLayer = layer;
            if (layer->getName().find(bgSurface.c_str()) == std::string::npos &&
                    layer->getName().find(surfaceView.c_str()) != std::string::npos) {
                isSurfaceView = true;
                if (layer->getName().find(blastSurface.c_str()) != std::string::npos) {
                    rightLayer = layer;
                    isBlast = true;
                }
            }
        }
    });
    if (rightLayer != nullptr) {
        rightLayerSp = rightLayer;
        ALOGI("get Layer succeed, layer name: %s", rightLayer->getName().c_str());
    }
    else
        ALOGE("get Layer failed, activity: %s", activityName);
    return rightLayerSp;
}

bool MiSurfaceFlingerImpl::isFpsStatSupported(Layer* layer) {
    return (layer->mFpsStat != nullptr);
}

void MiSurfaceFlingerImpl::createFpsStat(Layer* layer) {
    if (layer->mFpsStat == nullptr) {
        layer->mFpsStat = new FpsStat();
    }
}

void MiSurfaceFlingerImpl::destroyFpsStat(Layer* layer) {
    if (layer->mFpsStat != nullptr) {
        FpsStat* fpsStat = reinterpret_cast<FpsStat*>(layer->mFpsStat);
        delete fpsStat;
        layer->mFpsStat = nullptr;
    }
}

void MiSurfaceFlingerImpl::insertFrameTime(Layer* layer, nsecs_t time) {
    if (layer->mFpsStat != nullptr) {
        FpsStat* fpsStat = reinterpret_cast<FpsStat*>(layer->mFpsStat);
        if (fpsStat->isEnabled())
            fpsStat->insertFrameTime(time);
    }
}

void MiSurfaceFlingerImpl::insertFrameFenceTime(Layer* layer, const std::shared_ptr<FenceTime>& fence) {
    if (layer->mFpsStat != nullptr) {
        FpsStat* fpsStat = reinterpret_cast<FpsStat*>(layer->mFpsStat);
        if (fpsStat->isEnabled())
            fpsStat->insertFrameFenceTime(fence);
    }
}

bool MiSurfaceFlingerImpl::isFpsStatEnabled(Layer* layer) {
    if (layer->mFpsStat != nullptr) {
        FpsStat* fpsStat = reinterpret_cast<FpsStat*>(layer->mFpsStat);
        return fpsStat->isEnabled();
    }
    return false;
}

void MiSurfaceFlingerImpl::enableFpsStat(Layer* layer, bool enable) {
    if (layer->mFpsStat != nullptr) {
        FpsStat* fpsStat = reinterpret_cast<FpsStat*>(layer->mFpsStat);
        fpsStat->enable(enable);
    }
}

void MiSurfaceFlingerImpl::setBadFpsStandards(Layer* layer, int targetFps, int thresh1, int thresh2) {
    if (layer->mFpsStat != nullptr) {
        FpsStat* fpsStat = reinterpret_cast<FpsStat*>(layer->mFpsStat);
        fpsStat->setBadFpsStandards(targetFps, thresh1, thresh2);
    }
}

void MiSurfaceFlingerImpl::reportFpsStat(Layer* layer, Parcel* reply) {
    float avgFps, midFps;
    int pauseCounts;
    double lowRate, jitterRate, stabilityRate, fpsStDev;
    int isBadFrame = 0;
    String16 fpsDis = String16("");
    os::PersistableBundle bundle;
    if (layer->mFpsStat != nullptr) {
        FpsStat* fpsStat = reinterpret_cast<FpsStat*>(layer->mFpsStat);
        avgFps = fpsStat->getAverageFps();
        midFps = fpsStat->getMiddleFps();
        lowRate = fpsStat->getLowFpsRate();
        jitterRate = fpsStat->getJitterRate();
        stabilityRate = fpsStat->getStabilityRate();
        fpsStDev = fpsStat->getFpsStDev();
        pauseCounts = fpsStat->getPauseCounts();
        isBadFrame = fpsStat->getIsBadFrame() == true ? 1 : 0;
        if (isBadFrame) {
            fpsDis = fpsStat->getBadFrameDis();
        }
        bundle.putDouble(String16("avgFps"), static_cast<double>(avgFps));
        bundle.putDouble(String16("midFps"), static_cast<double>(midFps));
        bundle.putDouble(String16("lowRate"), lowRate);
        bundle.putInt(String16("pauseCounts"), pauseCounts);
        bundle.putDouble(String16("jitterRate"), jitterRate);
        bundle.putDouble(String16("stabilityRate"), stabilityRate);
        bundle.putDouble(String16("fpsStDev"), fpsStDev);
        bundle.putInt(String16("isBadFrame"), isBadFrame);
        bundle.putString(String16("fpsDis"), fpsDis);
        reply->writeParcelable(bundle);
    }
}

bool MiSurfaceFlingerImpl::isFpsMonitorRunning(Layer* layer) {
    if (layer->mFpsStat != nullptr) {
        FpsStat* fpsStat = reinterpret_cast<FpsStat*>(layer->mFpsStat);
        return (fpsStat->currentMonitorThreadState() != 0);
    }
    return false;
}

void MiSurfaceFlingerImpl::startFpsMonitor(Layer* layer, int timeout) {
    if (layer->mFpsStat != nullptr) {
        FpsStat* fpsStat = reinterpret_cast<FpsStat*>(layer->mFpsStat);
        fpsStat->createMonitorThread(timeout);
    }
}

void MiSurfaceFlingerImpl::stopFpsMonitor(Layer* layer) {
    if (layer->mFpsStat != nullptr) {
        FpsStat* fpsStat = reinterpret_cast<FpsStat*>(layer->mFpsStat);
        fpsStat->distroyMonitorThread();
    }
}
// END
void MiSurfaceFlingerImpl::saveDefaultResolution(DisplayModes supportedModes) {
    DisplayModePtr mode = supportedModes.begin()->second;
    auto width = mode->getWidth();
    auto height = mode->getHeight();
    std::string value;
    value.append(std::to_string(width));
    value.append("x");
    value.append(std::to_string(height));
    property_set("persist.sys.miui_default_resolution", value.c_str());
    ALOGD("saveDefaultResolution width= %d, height =%d", width, height);
}

int MiSurfaceFlingerImpl::getCPUMaxFreq(const char* filepath) {
    int fd = -1;
    char value[10] = {0};
    int maxCpuFreq = -1;
    fd = open(filepath, O_RDONLY);
    if (-1 == fd) {
        ALOGE("open %s failed, errno=%d", filepath, errno);
        return -1;
    }

    if (-1 == read(fd, value, 10)) {
        ALOGE("read %s failed, errno=%d", filepath, errno);
        close(fd);
        return -1;
    }

    ALOGI("current big core's max freq is %s", value);
    maxCpuFreq = atoi(value);
    close(fd);

    return maxCpuFreq;
}

int MiSurfaceFlingerImpl::getCPUCombType() {
#define MAX_CPU4_FREQ_PATH "/sys/devices/system/cpu/cpu4/cpufreq/cpuinfo_max_freq"
#define MAX_CPU6_FREQ_PATH "/sys/devices/system/cpu/cpu6/cpufreq/cpuinfo_max_freq"
    int cpu4MaxFreq = -1;
    int cpu6MaxFreq = -1;

    cpu4MaxFreq = getCPUMaxFreq(MAX_CPU4_FREQ_PATH);
    cpu6MaxFreq = getCPUMaxFreq(MAX_CPU6_FREQ_PATH);

    if (-1 != cpu4MaxFreq && -1 != cpu6MaxFreq) {
        if (cpu4MaxFreq != cpu6MaxFreq) {
            return CPUCOMBTYPE_6_2;
        }
        return CPUCOMBTYPE_4_4;
    }

    return CPUCOMBTYPE_NOT_EXIST;
}

bool MiSurfaceFlingerImpl::getAffinityLimits(const char* affinityProp, const char* defValue,
                                                int& cpuBegin, int& cpuEnd) {
    char value[PROPERTY_VALUE_MAX] = {0};
    property_get(affinityProp, value, defValue);
    if (3 == strlen(value) && '-' == value[1]) {
        cpuBegin = value[0] - '0';
        cpuEnd   = value[2] - '0';
    } else {
        ALOGE("surfaceflinger getAffinityLimits has bad value=%s", value);
        return false;
    }

    return true;
}

bool MiSurfaceFlingerImpl::setAffinityImpl(const int& cpuBegin, const int& cpuEnd,
                                            const int& threadId) {
    if ((cpuBegin >= 0 && cpuBegin <= 7) &&
            (cpuEnd >= 0 && cpuEnd <= 7) &&
            cpuEnd >= cpuBegin) {
        cpu_set_t set;
        CPU_ZERO(&set);
        for (int i = cpuBegin; i <= cpuEnd; ++i) {
            CPU_SET(i, &set);
        }
        if (-1 == sched_setaffinity(threadId, sizeof(cpu_set_t), &set)) {
            ALOGE("surfaceflinger setAffinity failed, error=%d", errno);
            return false;
        }
    } else {
        ALOGE("surfaceflinger setAffinity failed, cpuBegin=%d, cpuEnd=%d",
                cpuBegin, cpuEnd);
        return false;
    }

    return true;
}

void MiSurfaceFlingerImpl::setAffinity(const int& affinityMode) {
    // affinityMode: 0 means reset to default
    // affinityMode: 1 means bind big cores
    ALOGI("surfaceflinger setAffinity mode=%d", affinityMode);
    if (mPreviousFrameAffinityMode == affinityMode)
        return;

    mPreviousFrameAffinityMode = affinityMode;

    if (0 == affinityMode) {
        if (!setAffinityImpl(mResetUIAffinityCpuBegin,
                mResetUIAffinityCpuEnd, gettid())) {
            ALOGE("surfaceflinger resetAffinity failed.");
        }
    } else {
        if (!setAffinityImpl(mUIAffinityCpuBegin, mUIAffinityCpuEnd,
                gettid())) {
             ALOGE("surfaceflinger setAffinity failed.");
        }
    }
}

void MiSurfaceFlingerImpl::setreAffinity(const int& reTid) {
    ALOGI("surfaceflinger setreAffinity reTid=%d", reTid);
    if (!setAffinityImpl(mREAffinityCpuBegin, mREAffinityCpuEnd, reTid)) {
        ALOGE("surfaceflinger setreAffinity failed.");
    }
}

DisplayFeatureCallback::DisplayFeatureCallback(callback_t cb, void *user) {
    mCbf = cb;
    mUser = user;
}

Return<void> DisplayFeatureCallback::displayfeatureInfoChanged(int32_t caseId, int32_t value, float red, float green, float blue)
{
    mCbf(mUser, caseId, value, red, green, blue);
    return Void();
}

void MiSurfaceFlingerImpl::HandleDisplayFeatureCallback(int event, int value, float r, float g, float b) {
    switch ((enum BACKDOOR_CODE)event) {
        case BACKDOOR_CODE::UPDATE_WCG_STATE: {
        } break;
        case BACKDOOR_CODE::UPDATE_DFPS_MODE: {
        } break;
        case BACKDOOR_CODE::UPDATE_SECONDARY_FRAME_RATE: {
            ALOGD("callback update frame rate");
            updateRefreshRate(value, (int)r);
        } break;
        case BACKDOOR_CODE::UPDATE_SMART_DFPS_MODE: {
        } break;
        case BACKDOOR_CODE::UPDATE_PCC_LEVEL: {
            ALOGV("Update pcc %f %f %f", r, g, b);
        } break;
        case BACKDOOR_CODE::SET_COLOR_MODE_CMD: {
        } break;
        case BACKDOOR_CODE::SET_DC_PARSE_STATE: {
        } break;
        case BACKDOOR_CODE::NOTIFY_HDR_STATE: {
        } break;
        case BACKDOOR_CODE::SEND_HBM_STATE: {
        } break;
        case BACKDOOR_CODE::LOCK_FPS_REQUEST: {
            ALOGD("callback LOCKFPS value %d", value);
            mSetLockFpsFlag = value;
            forceSkipIdleTimer(value);
        } break;
        case BACKDOOR_CODE::SET_FOLD_STATE: {
            ALOGD("callback set fold state %d", value);
            if (mFlinger->mIsFold == value)
                break;
            {
                Mutex::Autolock lock(mFlinger->mStateLock);
                mFoldStatus = value;
                mFlinger->mIsFold = value;
            }

            auto display = mFlinger->getDefaultDisplayDeviceLocked();
            mFlinger->mActiveDisplayTransformHint= display->getTransformHint();

            if (display->mRefreshRateOverlay) {
                mFlinger->mUpdateRefreshrateOverlayForFold = true;
                mFlinger->enableRefreshRateOverlay(true);
            }
        } break;
        default:
            ALOGE("Invalid DisplayFeature Callback Code %d", event);
            break;
    }
}

#if MI_SCREEN_PROJECTION
void MiSurfaceFlingerImpl::setDiffScreenProjection(uint32_t isScreenProjection, Output* outPut){
    auto& outputState = outPut->editState();
    outputState.isScreenProjection = isScreenProjection;
}

void MiSurfaceFlingerImpl::setLastFrame(uint32_t isLastFrame, Output* outPut){
    auto& outputState = outPut->editState();
    outputState.isLastFrame = isLastFrame;
}

void MiSurfaceFlingerImpl::setCastMode(uint32_t isCastMode, Output* outPut){
    auto& outputState = outPut->editState();
    outputState.isCastMode = isCastMode;
}

bool MiSurfaceFlingerImpl::isNeedSkipPresent(Output* outPut){
    const auto& outputState = outPut->getState();
    if (!outputState.layerFilter.toInternalDisplay
            && outputState.isScreenProjection != 0
            && !outputState.isEnabled) {
        return true;
    }
    return false;
}

void MiSurfaceFlingerImpl::rebuildLayerStacks(Output* outPut){
    auto& outputState = outPut->editState();
    if (!outputState.layerFilter.toInternalDisplay
            && outputState.isScreenProjection != 0) {
        outPut->setCompositionEnabled(true);
        if (outputState.isLastFrame != 0) {
            outPut->setCompositionEnabled(false);
        }
    }
}

void MiSurfaceFlingerImpl::ensureOutputLayerIfVisible(Output* outPut,
                          const compositionengine::LayerFECompositionState* layerFEState){
    const auto& outputState = outPut->getState();
    if (!outputState.layerFilter.toInternalDisplay
            && outputState.isScreenProjection != 0
            && layerFEState->lastFrameLayer != 0) {
        outPut->setCompositionEnabled(false);
    }
}

bool MiSurfaceFlingerImpl::includesLayer(const sp<compositionengine::LayerFE>& layerFE, const Output* outPut) {
    const auto* layerFEState = layerFE->getCompositionState();
    if (!layerFEState)
        return false;

    std::optional<uint32_t> layerStackId = layerFEState->outputFilter.layerStack.id;
    bool internalOnly = layerFEState->outputFilter.toInternalDisplay;
    const auto& outputState = outPut->getState();

    if (layerFEState->recorderBarLayer && outputState.hideRecorderBar) {
        return false;
    }

    if (outputState.isCastMode != 0) {
        if (layerFEState->smallCastLayer != 0) {
            return layerStackId && (*layerStackId == outputState.layerFilter.layerStack.id)
                    && !outputState.layerFilter.toInternalDisplay;
        } else {
            return layerStackId && (*layerStackId == outputState.layerFilter.layerStack.id)
                    && outputState.layerFilter.toInternalDisplay;
        }
    } else {
        if (!outputState.layerFilter.toInternalDisplay && ((outputState.isScreenProjection != 0 && layerFEState->privateLayer != 0)
#ifdef MI_SF_FEATURE
            || (getMiSecurityDisplayState(outPut) && layerFEState->virtualDisplayHidelayer != 0)
#endif
            )) {
            return false;
        } else {
            return outPut->includesLayer(layerFEState->outputFilter);
        }
    }
    // END
}

bool MiSurfaceFlingerImpl::continueComputeBounds(Layer* layer){
    const Layer::State& s(layer->getDrawingState());
    if (s.flags & layer_state_t::eLayerCast) {
        return false;
    }
    return true;
}

void MiSurfaceFlingerImpl::prepareBasicGeometryCompositionState(
    compositionengine::LayerFECompositionState* compositionState, const Layer::State& drawingState, Layer* layer){
    const auto& parent = layer->mDrawingParent.promote();
    if (parent == nullptr) {
        compositionState->privateLayer = drawingState.screenFlags & eScreenProjection;
    } else {
        compositionState->privateLayer = (drawingState.screenFlags | parent->mDrawingState.screenFlags) & eScreenProjection;
    }
    compositionState->lastFrameLayer = drawingState.screenFlags & eInCallScreenProjection;
    compositionState->smallCastLayer = drawingState.flags & layer_state_t::eLayerCast;
    compositionState->virtualDisplayHidelayer = drawingState.screenFlags & eVirtualdisplayHide;

    // Set dithering layers can only show on primary display.
    // Dithering layers shouldn't show on virtual displays, but should show on screenshot.
    if (drawingState.inputInfo.token != nullptr
            && drawingState.inputInfo.layoutParamsFlags.test(gui::WindowInfo::Flag::DITHER)) {
        compositionState->recorderBarLayer = true;
    }
}

bool MiSurfaceFlingerImpl::setScreenFlags(int32_t screenFlags, Layer* layer) {
    if (layer->mDrawingState.screenFlags == screenFlags)
        return false;
    layer->mDrawingState.sequence++;
    layer->mDrawingState.screenFlags = screenFlags;
    layer->mDrawingState.modified = true;
    layer->setTransactionFlags(eTransactionNeeded);
    return true;
}

void MiSurfaceFlingerImpl::setChildImeFlags(uint32_t flags, uint32_t mask, Layer* layer) {
    // only set eLayerCast flag (small window cast)
    if (mask & layer_state_t::eLayerCast) {
        for (const sp<Layer>& child : layer->mDrawingChildren) {
            uint32_t mChildCastFlag = (flags & layer_state_t::eLayerCast)
                                         ? layer_state_t::eLayerCast : 0;
            if(child->getName().find("ImeContainer") == std::string::npos) {
                child->setFlags(mChildCastFlag, layer_state_t::eLayerCast);
            }
        }
    }
}

String8 MiSurfaceFlingerImpl::getLayerProtoName(Layer* layer, const Layer::State& state){
    String8 result;
    result.append(layer->getName().c_str());
    result.appendFormat("  screenFlags = %d", state.screenFlags);
    return result;
}

void MiSurfaceFlingerImpl::fillInputInfo(gui::WindowInfo* info, Layer* layer){
    if (layer->mDrawingState.flags & layer_state_t::eLayerCast) {
        info->visible = false;
    }
}

uint32_t MiSurfaceFlingerImpl::isCastMode(){
    return mIsCastMode;
}

void MiSurfaceFlingerImpl::setIsCastMode(uint32_t castMode){
    mIsCastMode = castMode;
}

uint32_t MiSurfaceFlingerImpl::isLastFrame(){
    return mIsLastFrame;
}

void MiSurfaceFlingerImpl::setIsLastFrame(uint32_t lastFrame){
    mIsLastFrame = lastFrame;
}

void MiSurfaceFlingerImpl::setupNewDisplayDeviceInternal(sp<DisplayDevice>& display, const DisplayDeviceState& state){
    display->setDiffScreenProjection(state.isScreenProjection);

    auto& outputState = display->getCompositionDisplay()->editState();
    outputState.hideRecorderBar = hideRecorderBar(state);
}

void MiSurfaceFlingerImpl::applyMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays, uint32_t* flags){
    for (const MiuiDisplayState& display : miuiDisplays) {
        *flags = *flags | setMiuiDisplayStateLocked(display);
    }
}
#endif

bool MiSurfaceFlingerImpl::isCallingBySystemui(const int callingPid) {
    char filename[64];
    sprintf(filename, "/proc/%d/cmdline", callingPid);
    FILE *f = fopen(filename, "r");
    if (!f) {
        ALOGD("Can not open file of /proc/%d/cmdline.", callingPid);
        // For some reason, can't open file. But avoid to take screenshot unsuccessfully, return true.
        return false;
    }
    char processName[64] = {0};
    fgets(processName, sizeof(processName) - 1, f);
    if (strcmp(processName, "com.android.systemui:screenshot") == 0
        || strcmp(processName, "com.miui.screenshot") == 0) {
        fclose(f);
        return true;
    }
    fclose(f);
    return false;
}

template <typename F, typename T>
inline std::future<T> MiSurfaceFlingerImpl::schedule(F&& f) {
    auto [task, future] = makeTask(std::move(f));
    mFlinger->mScheduler->postMessage(std::move(task));
    return std::move(future);
}

bool MiSurfaceFlingerImpl::isFold() {
    return mFoldStatus ? true : false;
}

bool MiSurfaceFlingerImpl::isDualBuitinDisp() {
    return mIsDualBuitInDisp;
}

bool MiSurfaceFlingerImpl::isCTS() {
    return mIsCts || mSetGoogleVrrFlag;
}

status_t MiSurfaceFlingerImpl::updateRefreshRate(int force, int mode) {

    if (!mIsDualBuitInDisp) {
        return NO_ERROR;
    }

    auto future = schedule([=]() -> status_t {
        scheduler::RefreshRateConfigs::Policy currentPolicy = mFlinger->mScheduler->mRefreshRateConfigs->getCurrentPolicy();

        Mutex::Autolock lock(mFlinger->mStateLock);
        auto display = getActiveDisplayDevice();
        ALOGV("updateRefreshRate, current: %s", currentPolicy.toString().c_str());

        DisplayModePtr preferredRefreshRate = mFlinger->mScheduler->getPreferredDisplayMode();
        if (force) {
            currentPolicy.defaultMode = DisplayModeId(mode);
        } else {
            setDesiredMultiDisplayPolicy(display, currentPolicy, false);
        }

        if (mFlinger->mScheduler->mRefreshRateConfigs->isModeAllowed(preferredRefreshRate->getId())) {
            mFlinger->setDesiredActiveMode({preferredRefreshRate, scheduler::DisplayModeEvent::Changed});
        }

        return NO_ERROR;
    });

    return future.get();
}

sp<DisplayDevice> MiSurfaceFlingerImpl::getActiveDisplayDevice() NO_THREAD_SAFETY_ANALYSIS {
    auto display = mFlinger->getVsyncSource();
    if (!display) {
        display = mFlinger->getDefaultDisplayDeviceLocked();
    }

    return display;
}

status_t MiSurfaceFlingerImpl::setDesiredMultiDisplayPolicy(const sp<DisplayDevice>& display,
            const std::optional<scheduler::RefreshRateConfigs::Policy>& policy, bool overridePolicy) {
    if (!mIsDualBuitInDisp) {
        return NO_ERROR;
    }

    if (overridePolicy) {
        return mFlinger->mScheduler->mRefreshRateConfigs->setOverridePolicy(policy);
    }

    const auto supportedModes = display->getSupportedModes();
    if (supportedModes.size() <= 0) {
        return BAD_VALUE;
    }

    DisplayModePtr it = supportedModes.begin()->second;
    DisplayModePtr minFpsMode = it;
    DisplayModePtr maxFpsMode = it;
    for (const auto& mode : supportedModes) {
        if (mode.second->getFps() < minFpsMode->getFps()) {
            minFpsMode = mode.second;
        }
        if (mode.second->getFps() > maxFpsMode->getFps()) {
            maxFpsMode = mode.second;
        }
    }

    if (maxFpsMode->getFps() < policy->primaryRange.max
            || minFpsMode->getFps() > policy->primaryRange.max) {
        return BAD_VALUE;
    }

    const scheduler::RefreshRateConfigs::Policy multi_policy{policy->defaultMode,
                                                   policy->allowGroupSwitching,
                                                   {minFpsMode->getFps(), policy->primaryRange.max},
                                                   {minFpsMode->getFps(), policy->appRequestRange.max}};
    ALOGV("setDesiredMultiDisplayPolicy, current: %s", multi_policy.toString().c_str());

    return mFlinger->mScheduler->mRefreshRateConfigs->setDisplayManagerPolicy(multi_policy);
}

status_t MiSurfaceFlingerImpl::setDesiredMultiDisplayModeSpecsInternal(const sp<DisplayDevice>& display,
        const std::optional<scheduler::RefreshRateConfigs::Policy>& policy, bool overridePolicy) NO_THREAD_SAFETY_ANALYSIS {
    ALOGV("Setting desired display in, mode specs: %s", policy->toString().c_str());
    if (!mIsDualBuitInDisp || !mFlinger->isInternalDisplay(display)) {
        return BAD_VALUE;
    }

    status_t ret = setDesiredMultiDisplayPolicy(display, policy, overridePolicy) ;
    if (ret == scheduler::RefreshRateConfigs::CURRENT_POLICY_UNCHANGED) {
        return NO_ERROR;
    } else if (ret != NO_ERROR) {
        return BAD_VALUE;
    }

    scheduler::RefreshRateConfigs::Policy currentPolicy = mFlinger->mScheduler->mRefreshRateConfigs->getCurrentPolicy();

    ALOGV("Setting desired multi-display mode specs: %s", currentPolicy.toString().c_str());

    if (!allowAospVrr()) {
        // TODO(b/140204874): Leave the event in until we do proper testing with all apps that might
        // be depending in this callback.
        const auto activeMode = display->getActiveMode();
        const nsecs_t vsyncPeriod = activeMode->getVsyncPeriod();
        const auto physicalId = display->getPhysicalId();
        mFlinger->mScheduler->onPrimaryDisplayModeChanged(mFlinger->mAppConnectionHandle, activeMode);
    }

    mFlinger->toggleKernelIdleTimer();

    auto preferredRefreshRate = mFlinger->mScheduler->getPreferredDisplayMode();
    ALOGV("trying to switch to Scheduler preferred mode %d (%s)",
          preferredRefreshRate->getId().value(), to_string(preferredRefreshRate->getFps()).c_str());

    if (mFlinger->mScheduler->mRefreshRateConfigs->isModeAllowed(preferredRefreshRate->getId())) {
        ALOGV("switching to Scheduler preferred display mode %d",
              preferredRefreshRate->getId().value());
        mFlinger->setDesiredActiveMode({preferredRefreshRate, scheduler::DisplayModeEvent::Changed});
        uint32_t hwcDisplayId;
        if (mFlinger->isDisplayExtnEnabled() && mFlinger->getHwcDisplayId(display, &hwcDisplayId)) {
            mFlinger->setDisplayExtnActiveConfig(hwcDisplayId, (uint32_t)preferredRefreshRate->getId().value());
            setupIdleTimeoutHandle(hwcDisplayId, mFlinger->mDynamicSfIdleEnabled, display->isPrimary());
        }
    } else {
        LOG_ALWAYS_FATAL("Desired display mode not allowed: %d",
                         preferredRefreshRate->getId().value());
    }

    mFlinger->setRefreshRates(display);

    return NO_ERROR;
}

void MiSurfaceFlingerImpl::setupIdleTimeoutHandle(uint32_t hwcDisplayId, bool enableDynamicIdle, bool isPrimary) {
    if (!enableDynamicIdle) {
        return;
    }

    if (mIsDualBuitInDisp) {
        mFlinger->mScheduler->handleIdleTimeout(true);
    } else {
        mFlinger->setupIdleTimeoutHandling(hwcDisplayId, isPrimary);
    }
}

void MiSurfaceFlingerImpl::dumpDisplayFeature(std::string& result)
{
    if (mDisplayFeatureGet) {
        mDisplayFeatureGet->Dump(result);
    } else {
        ALOGE("%s Istub id null", __func__);
    }
}

float MiSurfaceFlingerImpl::updateCupMuraEraseLayerAlpha(unsigned long brightness) {
    float alpha = (float)0.0;

    if (!mCupMuraEraselayerEnabled)
       return alpha;

    if (brightness > 0 && brightness <=418) {
        if (brightness >= 145)
          alpha = (float)0.002;
      else if (brightness >= 75)
            alpha = (float)0.010;
        else if (brightness >= 40)
            alpha = (float)0.020;
       else
           alpha = (float)0.025;
        return alpha;
    } else {
        return (float)0.0;
    }
}

void MiSurfaceFlingerImpl::setOrientationStatustoDF() {
    const auto* display = FTL_FAKE_GUARD(mFlinger->mStateLock, mFlinger->getDefaultDisplayDeviceLocked()).get();

    const auto orientation = display->getOrientation();
    if (ui::Transform::ROT_90 == ui::Transform::toRotationFlags(orientation)){
        mDisplayOritation = 90;
    } else if (ui::Transform::ROT_270 == ui::Transform::toRotationFlags(orientation)) {
        mDisplayOritation = 270;
    } else {
        mDisplayOritation = 0;
    }

    if (mCheckCupHbmCoverlayerEnabled) {
        if (mlastDisplayOritation != mDisplayOritation) {
            if (mDisplayFeatureGet) {
                ALOGE("notify displayfeature mDisplayOritation = %d", mDisplayOritation);
                mDisplayFeatureGet->setFeature(0, (int32_t)FeatureId::ORITATION_STATE, mDisplayOritation, 0);
            }
            mlastDisplayOritation = mDisplayOritation;
        }
    }
}

void MiSurfaceFlingerImpl::setFrameBufferSizeforResolutionChange(sp<DisplayDevice> displayDevice) {
    //Change FB size according to miui resolution
    char resValue[PROPERTY_VALUE_MAX] = {0};
    int resWidth = -1, resHeight = -1;
    property_get("persist.sys.miui_resolution", resValue, "-1,-1,-1");
    sscanf(resValue, "%d,%d,", &resWidth, &resHeight);
    const ui::Size miuiResolution(resWidth, resHeight);
    ALOGD(" %s: MIUI resolution = %dx%d", __FUNCTION__, resWidth, resHeight);
    if (displayDevice->isPrimary() && miuiResolution.isValid() && displayDevice->getSize() != miuiResolution) {
        base::unique_fd fd;
        displayDevice->setDisplaySize(resWidth, resHeight);
        auto display = displayDevice->getCompositionDisplay();
        display->getRenderSurface()->setViewportAndProjection();
        display->getRenderSurface()->flipClientTarget(true);
        display->getRenderSurface()->queueBuffer(std::move(fd));
        display->getRenderSurface()->flipClientTarget(false);
        // releases the FrameBuffer that was acquired as part of queueBuffer()
        display->getRenderSurface()->onPresentDisplayCompleted();
    }
}

#if MI_SCREEN_PROJECTION
uint32_t MiSurfaceFlingerImpl::setMiuiDisplayStateLocked(const MiuiDisplayState& s){
    const ssize_t index = mFlinger->mCurrentState.displays.indexOfKey(s.token);
    if (index < 0) return 0;

    uint32_t flags = 0;
    const uint32_t what = s.what;
    DisplayDeviceState& state = mFlinger->mCurrentState.displays.editValueAt((size_t)index);
    if (what & MiuiDisplayState::eIsScreenProjectionChanged) {
        if (state.isScreenProjection != s.isScreenProjection) {
            state.isScreenProjection = s.isScreenProjection;
            flags |= eDisplayTransactionNeeded;
        }
    }

    for (const auto& [_, display] : mFlinger->mDisplays) {
        if (what & MiuiDisplayState::eIsCastProjectionChanged) {
            mIsCastMode = s.isCastProjection;
            display->getCompositionDisplay()->setCastMode(mIsCastMode);
        }

        if (what & MiuiDisplayState::eIsLastFrameChanged) {
            mIsLastFrame = s.isLastFrame;
            display->getCompositionDisplay()->setLastFrame(mIsLastFrame);
        }
    }

    return flags;
}

void MiSurfaceFlingerImpl::setClientStateLocked(uint64_t what, sp<Layer>& layer, uint32_t* flags,
                                                  const layer_state_t& s){
    if (what & layer_state_t::eScreenProjected) {
        if (layer->setScreenFlags((int32_t)s.screenFlags))
            *flags = *flags | eTraversalNeeded;
    }
}

String8 MiSurfaceFlingerImpl::getDumpsysInfo(){
    String8 result;
    result.appendFormat("  One machine dual use screen mode:  mIsCastMode=%d, mIsLastFrame=%d\n",
           mIsCastMode, mIsLastFrame);
    result.append("\n");
    return result;
}

bool MiSurfaceFlingerImpl::showContineTraverseLayersInLayerStack(const sp<Layer>& layer){
    return (layer->getDrawingState().flags & layer_state_t::eLayerCast) /*&& display->isPrimary()*/;
}

void MiSurfaceFlingerImpl::setMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays,
                                     uint32_t flags, int64_t desiredPresentTime){
    ATRACE_CALL();

    const int64_t postTime = systemTime();

    bool permissions = mFlinger->callingThreadHasUnscopedSurfaceFlingerAccess();

    Mutex::Autolock _l(mFlinger->mStateLock);

    Vector<ComposerState> state;

    mFlinger->applyMiuiTransactionState({}, state, miuiDisplays, {}, flags, {}, desiredPresentTime,
                          {}, {}, postTime, permissions, {}, {}, {}, {}, {});
}
#endif

#if MI_VIRTUAL_DISPLAY_FRAMERATE
bool MiSurfaceFlingerImpl::skipComposite(const sp<DisplayDevice>& display){
    return display->isVirtual() && !display->needToComposite();
}

void MiSurfaceFlingerImpl::setLimitedFrameRate(const sp<DisplayDevice>& display, const DisplayDeviceState& state){
    display->setLimitedFrameRate(state.limitedFrameRate);
}

void MiSurfaceFlingerImpl::updateLimitedFrameRate(const sp<DisplayDevice>& display,
            const DisplayDeviceState& currentState, const DisplayDeviceState& drawingState){
    if(display->isVirtual()) {
        if(currentState.limitedFrameRate != drawingState.limitedFrameRate) {
            display->setLimitedFrameRate(currentState.limitedFrameRate);
        }
    }
}

void MiSurfaceFlingerImpl::updateDisplayStateLimitedFrameRate(uint32_t what, DisplayDeviceState& state,
            const DisplayState& s, uint32_t &flags){
    if (what & DisplayState::eLimitedFrameRateChanged) {
        if (state.limitedFrameRate != s.limitedFrameRate) {
            state.limitedFrameRate = s.limitedFrameRate;
            flags |= eDisplayTransactionNeeded;
        }
    }
}

void MiSurfaceFlingerImpl::initLimitedFrameRate(DisplayState& d){
    d.limitedFrameRate = 0;
}

void MiSurfaceFlingerImpl::setLimitedFrameRateDisplayDevice(DisplayDevice * display, uint32_t frameRate){
    ALOGI("setLimitedFrameRate = %d",frameRate);
    if(frameRate < 1) {
        display->setLimitedFrameRateReal(0);
        ALOGE("frameRate < 1 , so just set as no-limited");
        return;
    }
    display->setLimitedFrameRateReal(frameRate);
    display->setIntervalReal(1000*1000*1000 / display->getLimitedFrameRate());
}

bool MiSurfaceFlingerImpl::needToCompositeDisplayDevice(DisplayDevice * display){
    if(!display || !display->isVirtual() || display->getLimitedFrameRate() == 0) {
        return true;
    }

    //calibrate sentry every 1 second
    static nsecs_t sentryTimeStamp = 0;
    const static nsecs_t one_second_ns = 1*1000*1000*1000;
    if((systemTime(SYSTEM_TIME_MONOTONIC) - sentryTimeStamp) > one_second_ns) {
        sentryTimeStamp = systemTime(SYSTEM_TIME_MONOTONIC);
        display->setNextFrameTsReal(sentryTimeStamp);
    }

    bool ret = false;
    if(systemTime(SYSTEM_TIME_MONOTONIC) > display->getNextFrameTs()) {
        ret = true;
        display->setNextFrameTsReal(display->getNextFrameTs() + display->getInterval());
    }
    return ret;
}
#endif
void MiSurfaceFlingerImpl::addDolbyVisionHdr(std::vector<ui::Hdr> &hdrTypes) {
    if (!hdrTypes.empty())
        ALOGD("%s enter hdrTypes %d", __func__, hdrTypes[0]);

    if(property_get_bool("debug.config.media.video.dolby_vision_suports",0)) {
        ALOGD("debug.config.media.video.dolby_vision_suports == true");
        ui::Hdr HDR_TYPE_DOLBY_VISION = (ui::Hdr)1;
        if(hdrTypes[0] != HDR_TYPE_DOLBY_VISION){
            hdrTypes.push_back(HDR_TYPE_DOLBY_VISION);
        }
    }else{
        ALOGD("debug.config.media.video.dolby_vision_suports == false");
    }
}
// MIUI+ send SecureWin state
void MiSurfaceFlingerImpl::setSecureProxy(sp<IBinder> secureProxy) {
    mSecureProxy = secureProxy;
}

sp<IBinder> MiSurfaceFlingerImpl::getSecureProxy() {
    return mSecureProxy;
}

void MiSurfaceFlingerImpl::updateOutputExtra(std::shared_ptr<compositionengine::Output> output,
                bool isInternalDisplay, int sequenceId) {
    if (output->mOutputExtra == nullptr) {
        output->mOutputExtra = new OutputExtra();
    }
    OutputExtra* outputExtra = reinterpret_cast<OutputExtra*>(output->mOutputExtra);
    outputExtra->setDisplayState(isInternalDisplay);
    outputExtra->setSequenceId(sequenceId);
    if(mSecureProxy == nullptr) {
        outputExtra->setLastFrameState(false);
    }
}

void MiSurfaceFlingerImpl::handleSecureProxy(compositionengine::Output* output,
                uint32_t layerStackId) {
    if (output->mOutputExtra == nullptr || mSecureProxy == nullptr) {
        return;
    }
    OutputExtra* outputExtra = reinterpret_cast<OutputExtra*>(output->mOutputExtra);
    const String16 synergyCallback("com.android.synergy.Callback");
    auto layers = output->getOutputLayersOrderedByZ();
    bool currentFrameSecure = false;
    currentFrameSecure = std::any_of(layers.begin(), layers.end(), [](auto* layer) {
            return layer->getLayerFE().getCompositionState()->isSecure;
    });
    if(outputExtra->isLastFrameSecure() != currentFrameSecure &&
        (outputExtra->isInternalDisplay() || layerStackId)) {
            outputExtra->setLastFrameState(currentFrameSecure);
            Parcel data;
            data.writeInterfaceToken(synergyCallback);
            data.writeInt32(currentFrameSecure);
            data.writeInt32(outputExtra->isInternalDisplay() ? 0 : outputExtra->getSequenceId());
            uint32_t CODE_SECURE_WINDOW = 2;
            mSecureProxy->transact(CODE_SECURE_WINDOW, data, nullptr, IBinder::FLAG_ONEWAY);
    }
}
// END

#ifdef MI_SF_FEATURE
    void MiSurfaceFlingerImpl::setNewMiSecurityDisplayState(sp<DisplayDevice>& display, const DisplayDeviceState& state) {
        if(display->isVirtual()) {
                std::shared_ptr<compositionengine::Output> output = display->getCompositionDisplay();
                if (output->mOutputExtra == nullptr) {
                    output->mOutputExtra = new OutputExtra();
                }
                OutputExtra* outputExtra = reinterpret_cast<OutputExtra*>(output->mOutputExtra);
                outputExtra->setMiSecurityDisplayState(state.miSecurityDisplay);
        }
    }
    void MiSurfaceFlingerImpl::setMiSecurityDisplayState(const sp<DisplayDevice>& display,
            const DisplayDeviceState& currentState, const DisplayDeviceState& drawingState) {
        if(display->isVirtual()) {
            if(currentState.miSecurityDisplay != drawingState.miSecurityDisplay) {
                std::shared_ptr<compositionengine::Output> output = display->getCompositionDisplay();
                if (output->mOutputExtra == nullptr) {
                    output->mOutputExtra = new OutputExtra();
                }
                OutputExtra* outputExtra = reinterpret_cast<OutputExtra*>(output->mOutputExtra);
                outputExtra->setMiSecurityDisplayState(currentState.miSecurityDisplay);
            }
        }

    }
    bool MiSurfaceFlingerImpl::getMiSecurityDisplayState(const compositionengine::Output* output) {
        if (output->mOutputExtra == nullptr) {
            return false;
        }
        OutputExtra* outputExtra = reinterpret_cast<OutputExtra*>(output->mOutputExtra);
        return outputExtra->getMiSecurityDisplayState();
    }
    void MiSurfaceFlingerImpl::setVirtualDisplayCoverBlur(const Layer* layer, compositionengine::LayerFE::LayerSettings& layerSettings) {
        layerSettings.coverBlur = layer->mDrawingState.screenFlags & eVirtualdisplayCoverBlur;
    }
    void MiSurfaceFlingerImpl::updateMiSecurityDisplayState(uint32_t what, DisplayDeviceState& state,
                const DisplayState& s, uint32_t &flags) {
        if (what & DisplayState::eIsDisplaySecurityChanged) {
            if (state.miSecurityDisplay != s.isDisplaySecurity) {
                state.miSecurityDisplay = s.isDisplaySecurity;
                flags |= eDisplayTransactionNeeded;
            }
        }
    }
#endif
#ifdef CURTAIN_ANIM
void MiSurfaceFlingerImpl::updateCurtainStatus(
        std::shared_ptr<compositionengine::Output> output) {
    if (output->mOutputExtra == nullptr) {
        output->mOutputExtra = new OutputExtra();
    }
    OutputExtra* outputExtra = reinterpret_cast<OutputExtra*>(output->mOutputExtra);
    outputExtra->setCurtainAnimRate(mCurtainAnimRate);
    outputExtra->enableCurtainAnim(mEnableCurtainAnim);
}

bool MiSurfaceFlingerImpl::setCurtainAnimRate(float rate) {
    mCurtainAnimRate = rate;
    return true;
}

bool MiSurfaceFlingerImpl::enableCurtainAnim(bool isEnable) {
    mEnableCurtainAnim = isEnable;
    return true;
}

void MiSurfaceFlingerImpl::getCurtainStatus(compositionengine::Output* output,
        renderengine::DisplaySettings* displaySettings) {
    if (output->mOutputExtra == nullptr || displaySettings == nullptr) {
        return;
    }
    OutputExtra* outputExtra = reinterpret_cast<OutputExtra*>(output->mOutputExtra);
    displaySettings->enableCurtainAnim = outputExtra->isEnableCurtainAnim();
    displaySettings->curtainAnimRate = outputExtra->getCurtainAnimRate();
}

void MiSurfaceFlingerImpl::updateCurtainAnimClientComposition(bool* forceClientComposition) {
    if (mEnableCurtainAnim) {
        *forceClientComposition = true;
    }
}
#endif

#ifdef MI_SF_FEATURE
bool MiSurfaceFlingerImpl::setShadowType(int shadowType, Layer* layer) {
    if (layer->mDrawingState.shadowSettings.shadowType == shadowType) {
        return false;
    }
    layer->mDrawingState.sequence++;
    layer->mDrawingState.shadowSettings.shadowType = shadowType;
    layer->mDrawingState.modified = true;
    layer->setTransactionFlags(eTransactionNeeded);
    return true;
}
bool MiSurfaceFlingerImpl::setShadowLength(float length, Layer* layer) {
    if (layer->mDrawingState.shadowSettings.length == length) {
        return false;
    }
    layer->mDrawingState.sequence++;
    layer->mDrawingState.shadowSettings.length = length;
    layer->mDrawingState.modified = true;
    layer->setTransactionFlags(eTransactionNeeded);
    return true;
}
bool MiSurfaceFlingerImpl::setShadowColor(const half4& color, Layer* layer) {
    vec4 tmpColor(color.r, color.g, color.b, color.a);
    if (layer->mDrawingState.shadowSettings.color == tmpColor) {
        return false;
    }
    layer->mDrawingState.sequence++;
    layer->mDrawingState.shadowSettings.color = tmpColor;
    layer->mDrawingState.modified = true;
    layer->setTransactionFlags(eTransactionNeeded);
    return true;
}
bool MiSurfaceFlingerImpl::setShadowOffset(float offsetX, float offsetY, Layer* layer) {
    if (layer->mDrawingState.shadowSettings.offsetX == offsetX
            && layer->mDrawingState.shadowSettings.offsetY == offsetY) {
        return false;
    }
    layer->mDrawingState.sequence++;
    layer->mDrawingState.shadowSettings.offsetX = offsetX;
    layer->mDrawingState.shadowSettings.offsetY = offsetY;
    layer->mDrawingState.modified = true;
    layer->setTransactionFlags(eTransactionNeeded);
    return true;
}
bool MiSurfaceFlingerImpl::setShadowOutset(float outset, Layer* layer) {
    if (layer->mDrawingState.shadowSettings.outset == outset) {
        return false;
    }
    layer->mDrawingState.sequence++;
    layer->mDrawingState.shadowSettings.outset = outset;
    layer->mDrawingState.modified = true;
    layer->setTransactionFlags(eTransactionNeeded);
    return true;
}
bool MiSurfaceFlingerImpl::setShadowLayers(int32_t numOfLayers, Layer* layer) {
    if (layer->mDrawingState.shadowSettings.numOfLayers == numOfLayers) {
        return false;
    }
    layer->mDrawingState.sequence++;
    layer->mDrawingState.shadowSettings.numOfLayers = numOfLayers;
    layer->mDrawingState.modified = true;
    layer->setTransactionFlags(eTransactionNeeded);
    return true;
}
void MiSurfaceFlingerImpl::prepareMiuiShadowClientComposition(
                                compositionengine::LayerFE::LayerSettings& caster,
                                const Rect& layerStackRect, Layer* layer) {
    renderengine::MiuiShadowSettings state = layer->getDrawingState().shadowSettings;
    if (state.shadowType == 0) {
        return;
    }
    state.boundaries = layer->getBounds();
    Rect screenBounds = layer->getScreenBounds();
    if (state.lightPos.z < 50.f) {
        state.lightPos.x = ((layerStackRect.width() / 2.f) - screenBounds.left);
        state.lightPos.y -= screenBounds.top;
    } else {
        state.lightPos.x += (layerStackRect.width() / 2.f);
        state.lightPos.y += (layerStackRect.height() / 2.f);
    }
    const float casterAlpha = caster.alpha;
    const bool casterIsOpaque =
            ((caster.source.buffer.buffer != nullptr) && caster.source.buffer.isOpaque);

    // If the casting layer is translucent, we need to fill in the shadow underneath the layer.
    // Otherwise the generated shadow will only be shown around the casting layer.
    state.casterIsTranslucent = !casterIsOpaque || (casterAlpha < 1.0f);
    state.color *= casterAlpha;

    if (state.color.a > 0.f) {
        caster.miShadow = state;
    }
}
void MiSurfaceFlingerImpl::setShadowClientStateLocked(uint64_t what,sp<Layer>& layer,
                                uint32_t* flags, const layer_state_t& s) {
    if (what & layer_state_t::eShadowChanged) {
        if (layer->setShadowType(s.shadowType))
            *flags |= eTraversalNeeded;
        if (layer->setShadowLength(s.shadowLength))
            *flags |= eTraversalNeeded;
        if (layer->setShadowColor(s.shadowColor))
            *flags |= eTraversalNeeded;
        if (layer->setShadowOffset(s.shadowOffset[0], s.shadowOffset[1]))
            *flags |= eTraversalNeeded;
        if (layer->setShadowOutset(s.shadowOutset))
            *flags |= eTraversalNeeded;
        if (layer->setShadowLayers(s.shadowLayers))
            *flags |= eTraversalNeeded;
    }
}

// HDR Dimmer
void MiSurfaceFlingerImpl::updateHdrDimmerState(
        std::shared_ptr<compositionengine::Output> output, int colorScheme) {
    if (output->mOutputExtra == nullptr) {
        output->mOutputExtra = new OutputExtra();
    }
    OutputExtra* outputExtra = reinterpret_cast<OutputExtra*>(output->mOutputExtra);
    outputExtra->enableHdrDimmer(mHdrDimmerEnabled, mHdrDimmerFactor);
    outputExtra->setColorScheme(colorScheme);
}

bool MiSurfaceFlingerImpl::enableHdrDimmer(bool enable, float factor) {
    mHdrDimmerEnabled = enable;
    mHdrDimmerFactor = factor;
    return true;
}

void MiSurfaceFlingerImpl::getHdrDimmerState(compositionengine::Output* output,
        renderengine::DisplaySettings* displaySettings) {
    if (output->mOutputExtra == nullptr || displaySettings == nullptr) {
        return;
    }
    OutputExtra* outputExtra = reinterpret_cast<OutputExtra*>(output->mOutputExtra);
    displaySettings->hdrDimmerEnabled = outputExtra->isHdrDimmerEnabled();
    displaySettings->hdrDimmerFactor = outputExtra->getHdrDimmerFactor();
    displaySettings->colorScheme = outputExtra->getColorScheme();
}

bool MiSurfaceFlingerImpl::getHdrDimmerClientComposition() {
    return mHdrDimmerEnabled;
}
// END
#endif

bool MiSurfaceFlingerImpl::isSecondRefreshRateOverlayEnabled(){
    return mSecondRefreshRateOverlayEnabled;
}

bool MiSurfaceFlingerImpl::isEmbeddedEnable(){
    return mEmbeddedEnable;
}

void MiSurfaceFlingerImpl::changeLayerDataSpace(Layer* layer)
{
    if (mGalleryP3DVOpt && mDolbyVisionState && mHasP3Layers && layer
        && layer->getName().find("com.miui.video.gallery.galleryvideo.FrameLocalPlayActivity") != std::string::npos) {
        layer->setDataspace(ui::Dataspace::DISPLAY_P3);
    }
}

#ifdef QTI_DISPLAY_CONFIG_ENABLED
int MiSurfaceFlingerImpl::enableQsync(int qsyncMode, String8 pkgName, int pkgSetFps, int qsyncMinFps, int qsyncBaseTiming, int desiredVsync)
{
    int ret = NO_ERROR;
    if (mQsyncFeature)
        mQsyncFeature->updateQsyncStatus(qsyncMode, pkgName, pkgSetFps, qsyncMinFps, qsyncBaseTiming, desiredVsync);
    return ret;
}

bool MiSurfaceFlingerImpl::isQsyncEnabled()
{
    bool ret = false;
    if (mQsyncFeature)
        ret = mQsyncFeature->isQsyncEnabled() && !isFod();
    return ret;
}
#endif

// MIUI ADD: Added just for Global HBM Dither (J2S-T)
bool MiSurfaceFlingerImpl::isGlobalHBMDitherEnabled() {
    return mGlobalHBMDitherEnabled;
}
// Filter unnecessary InputWindowInfo
void MiSurfaceFlingerImpl::setFilterInputFlags(sp<BufferStateLayer> layer) {
    for (const auto layerName : mFilterInputList) {
        if (layer->getName().find(layerName) != std::string::npos) {
            ALOGE("The layer(%s) need filter InputWindowInfo to InputFlinger.", layer->getName().c_str());
            layer->mNeedFilterInputWindowInfo = true;
            return;
        }
    }
}

// Return true if need filter inputWindowInfo, else return false.
bool MiSurfaceFlingerImpl::filterUnnecessaryInput(const BufferLayer* layer) {
    if (layer->mNeedFilterInputWindowInfo) {
        if(layer->mDrawingState.inputInfo.inputConfig.test(WindowInfo::InputConfig::NO_INPUT_CHANNEL)
            || (layer->mDrawingState.inputInfo.inputConfig.test(WindowInfo::InputConfig::NOT_TOUCHABLE))) {
            return true;
        }
    }
    return false;
}

void MiSurfaceFlingerImpl::initBlackListForFilterInput(){
    mFilterInputList.push_back("com.miui.miwallpaper");
    mFilterInputList.push_back("Wallpaper BBQ wrapper");
    mFilterInputList.push_back("RoundCornerBottom");
    mFilterInputList.push_back("RoundCornerTop");
}

// dynamic SurfaceFlinger Log
void MiSurfaceFlingerImpl::setDynamicSfLog(bool dynamic) {
    ALOGI("setDynamicSfLog prop:%d dynamic:%d", mPropDynamicLog, dynamic);
    mDynamicLog = dynamic;
}

bool MiSurfaceFlingerImpl::isDynamicSfLog() {
    return (mPropDynamicLog || mDynamicLog);
}

sp<DisplayDevice> MiSurfaceFlingerImpl::getSecondaryDisplayDeviceLocked() REQUIRES(mFlinger->mStateLock) {
    const auto hwcSecondaryDisplayId = mFlinger->getHwComposer().getSecondaryHwcDisplayId();
    const auto physicalDisplayId = hwcSecondaryDisplayId ?
    mFlinger->getHwComposer().toPhysicalDisplayId(hwcSecondaryDisplayId): std::nullopt;
    const auto token = physicalDisplayId ? mFlinger->getPhysicalDisplayTokenLocked(*physicalDisplayId) : nullptr;
    if (token){
        return mFlinger->getDisplayDeviceLocked(token);
    }
    return nullptr;
}

void MiSurfaceFlingerImpl::initSecondaryDisplayLocked(Vector<DisplayState>& displays) NO_THREAD_SAFETY_ANALYSIS {
     char value[PROPERTY_VALUE_MAX] = {'\0'};
     property_get("persist.sys.muiltdisplay_type", value, "0");
     int fold_device = atoi(value);
     //MuiltDisplay type:SINGLE(0),MULTI_NORMAL(1),MULTI_FOLDER(2)
     if (fold_device != 2) {
         return;
     }
    const auto SecondaryDisplay = getSecondaryDisplayDeviceLocked();
    if (SecondaryDisplay) {
        const sp<IBinder> secondaryToken = SecondaryDisplay->getDisplayToken().promote();
        LOG_ALWAYS_FATAL_IF(secondaryToken == nullptr);
        DisplayState secondaryDisplayState;
        secondaryDisplayState.what = DisplayState::eDisplayProjectionChanged |DisplayState::eLayerStackChanged;
        secondaryDisplayState.token = secondaryToken;
        secondaryDisplayState.layerStack.id = 1;
        secondaryDisplayState.orientation = ui::ROTATION_0;

        secondaryDisplayState.orientedDisplaySpaceRect.makeInvalid();
        secondaryDisplayState.layerStackSpaceRect.makeInvalid();
        secondaryDisplayState.width = 0;
        secondaryDisplayState.height = 0;
        displays.add(secondaryDisplayState);
        mFlinger->setPowerModeInternal(SecondaryDisplay, hal::PowerMode::ON);
    }
}

extern "C" IMiSurfaceFlingerStub* create() {
    return new MiSurfaceFlingerImpl;
}

extern "C" void destroy(IMiSurfaceFlingerStub* impl) {
    delete impl;
}

}
