/*
 * Copyright (C) 2020, Xiaomi Inc. All rights reserved.
 *
 */
#include "QsyncFeature.h"
#include <log/log.h>
#include <utils/Errors.h>
#include "SurfaceFlinger.h"
#include <cutils/properties.h>
#include <ftl/fake_guard.h>
#include <scheduler/Fps.h>
#include <DisplayHardware/DisplayMode.h>

namespace android {

QsyncFeature::QsyncFeature() {
    mMiFlinger = nullptr;
    mQsyncSupport = false;
    mMaxQsyncFps = 0;
    mMinQsyncFps = 0;
    mLastDesiredVsyncHz = 0;
}

void QsyncFeature::selectBestQsyncMinFps(int qsyncBaseTiming, int pkgSetFps) {
    if (!mUseDynamicQsyncMinFps)
        return;

    int division, remainder;
    division = qsyncBaseTiming / pkgSetFps;
    remainder = qsyncBaseTiming % pkgSetFps;
    if (remainder) {
         division = division + 1;
         pkgSetFps = qsyncBaseTiming / division;
    }

    mPkgSetFps = pkgSetFps;

    if (mQsyncMode) {
        if (mPkgSetFps > mMaxQsyncFps) {
            ALOGD("pkgName(%s) set FPS(%d) not support in qsync(%d), disable qsync",
                    mPkgName.c_str(), pkgSetFps, mQsyncMode);
            if (mQsyncEnableStatus) {
                    mQsyncMinFps = 0;
                    mQsyncMode = 0;
                    mPkgSetFps = 0;
                    mDesiredVsyncHz = 0;
                    mQsyncBaseTiming = 120;
            }
        } else {
            if (!mQsyncMinFps) {
                if(mPkgSetFps && mPkgSetFps >= mMinQsyncFps) {
                    mQsyncMinFps = mPkgSetFps;
                } else
                    mQsyncMinFps = mMinQsyncFps;
            }
        }
    }
}

int QsyncFeature::findBaseTimingHwcId()
{
    int ret = NO_ERROR;
    if (mQsyncBaseTiming) {
        DisplayModePtr outMode;
        if (mUseQsyncTimingMode) {
            const auto display = FTL_FAKE_GUARD(mFlinger->mStateLock, mFlinger->getDefaultDisplayDeviceLocked());
            const auto& supportedModes = display->getSupportedModes();

            for (const auto& [id, mode] : supportedModes) {
                if ((mode->getFps().getIntValue() == mQsyncBaseTiming) &&
                    mMiFlinger->isDdicQsyncModeGroup(mode->getGroup())) {
                    mQsyncTimingModeGroup = mode->getGroup();
                    outMode = mode;
                    break;
                }
            }
        } else {
            mFlinger->getModeFromFps(mQsyncBaseTiming, outMode);
        }

        if(outMode) {
            mQsyncBaseTimingHwcId = outMode->getHwcId();
            ALOGD("Find qsync base timing  hwc id(%d)", mQsyncBaseTimingHwcId);
        } else {
            mQsyncTimingModeGroup = -1;
            ALOGD("No match qsync base timing!");
            ret = BAD_VALUE;
        }
    }
    return ret;
}

int32_t QsyncFeature::getQsyncTimingModeGroup() {
    return mQsyncTimingModeGroup;
}

void QsyncFeature::ParseQsyncConfig() {
    char value[PROPERTY_VALUE_MAX];
    bool qsyncSupport = property_get_bool("ro.vendor.mi_sf.qsync_support", false);
    ALOGD("ParseQsyncConfig: qsync_support:%d", qsyncSupport);

    if (qsyncSupport) {
        mQsyncSupport = qsyncSupport;
        property_get("ro.vendor.mi_sf.qsync_support_min_fps_range", value, "0,0");
        std::string qsyncFpsValue(value);
        mMinQsyncFps = stoi(qsyncFpsValue);
        qsyncFpsValue = (qsyncFpsValue.substr(qsyncFpsValue.find(',') + 1));
        mMaxQsyncFps = stoi(qsyncFpsValue);
        ALOGD("Get qsync property Min(%d), Max(%d) FPS",mMinQsyncFps, mMaxQsyncFps);
    }
}

void QsyncFeature::qsyncInit(MiSurfaceFlingerImpl* miSf, const sp<SurfaceFlinger>& flinger) {
    ParseQsyncConfig();

    if(!mQsyncSupport) {
        ALOGI("Qsync does not support!");
        return;
    }

    mFlinger = flinger;
    mMiFlinger = miSf;

#ifdef QTI_DISPLAY_CONFIG_ENABLED
    int ret = ::DisplayConfig::ClientInterface::Create("MiSurfaceFlinger"+std::to_string(0),
                                                       nullptr,
                                                       &mDisplayConfigIntf);
    if(ret || !mDisplayConfigIntf) {
        ALOGE("DisplayConfig HIDL not present\n");
        mDisplayConfigIntf = nullptr;
    } else {
        ALOGV("DisplayConfig HIDL is present\n");
    }
#else
    mDisplayConfigIntf = nullptr;
#endif

}

int QsyncFeature::updateQsyncStatus(int qsyncMode, String8 pkgName, int pkgSetFps, int qsyncMinFps, int qsyncBaseTiming, int desiredVsyncHz) {
    int ret = -1;

    if (!mQsyncSupport) {
        ALOGI("Qsync does not support!\n");
        return ret;
    }

    Mutex::Autolock _l(mQsyncStateMutex);

    mQsyncMode = qsyncMode;
    mPkgName = pkgName;
    mUseQsyncTimingMode = true;

    if (mQsyncMode) {
        mPkgSetFps = pkgSetFps;
        mQsyncMinFps = qsyncMinFps;

        if (desiredVsyncHz)
            mDesiredVsyncHz = desiredVsyncHz;

        if (qsyncBaseTiming)
            mQsyncBaseTiming = qsyncBaseTiming;
        else
            mQsyncBaseTiming = 120;

        selectBestQsyncMinFps(mQsyncBaseTiming, pkgSetFps);
    } else {
        //enable idle
        mQsyncBaseTiming = 0;
        mPkgSetFps = 0;
        //reset qsync min fps
        mQsyncMinFps = 0;
        //reset vsync
        mDesiredVsyncHz = 0;
    }

    ALOGI("qsync(%d), PkgFps(%d), pkgName(%s), BaseTiming(%d), MinFps(%d), Vsync(%d)",
                    mQsyncMode, mPkgSetFps, mPkgName.c_str(), mQsyncBaseTiming, mQsyncMinFps, mDesiredVsyncHz);
    /*enable or disalbe qsync*/
    if (mQsyncEnableStatus != mQsyncMode)
        ret = enableQsync(mQsyncMode);
    else
        ALOGI("Same qsync state, do nothing!\n");

    if ((mLastDesiredVsyncHz != mDesiredVsyncHz) && mQsyncEnableStatus) {
        updateVsync(true);
    }

    if (!mQsyncMode && (ret == NO_ERROR))
        updateVsync(false);

    ATRACE_INT("QsyncStatus", mQsyncEnableStatus);
    return ret;
}

int QsyncFeature::enableQsync(int qsyncMode) {
    int ret = NO_ERROR;
    bool waitActiveMode = false;
    int count = 0;
    ::DisplayConfig::QsyncMode qsyncModeHwc = ::DisplayConfig::QsyncMode::kNone;

    if(mDisplayConfigIntf) {
        if(qsyncMode != 0)  {
            qsyncModeHwc = ::DisplayConfig::QsyncMode::kWaitForCommitEachFrame;
        }

        /*lock fps for enable qsync*/
        {

            ret = findBaseTimingHwcId();
            if (ret != NO_ERROR)
                return ret;

            /*vsync mix in minfps, so send once for performance when on/off*/
            /*enable qsync must set minfps, disable qsync minfps must be zero*/
            updateQsyncMinFps();

            /*Only change mode, do call*/
            if (mQsyncBaseTiming) {
                const auto display = FTL_FAKE_GUARD(mFlinger->mStateLock, mFlinger->getDefaultDisplayDeviceLocked());
                const auto displayId = display->getId();
                const auto physicalDisplayId = PhysicalDisplayId::tryCast(displayId);
                std::optional<hal::HWDisplayId> activeModeHwcId;
                mMiFlinger->mSetIdleForceFlag = true;
                mMiFlinger->forceSkipIdleTimer(true);
                mMiFlinger->changeFps(mQsyncBaseTiming, true, mUseQsyncTimingMode);
                mFlinger->scheduleRepaint();
                while (!waitActiveMode && qsyncMode && count < MAX_CYCLE_COUNT) {
                    //mDisplayConfigIntf->GetActiveConfig(DisplayConfig::DisplayType::kPrimary, &configActiveHwc);
                    activeModeHwcId = mFlinger->getHwComposer().getActiveMode(*physicalDisplayId);
                    ALOGD("Qsync active config(%d)", *activeModeHwcId);
                    if (mQsyncBaseTimingHwcId == *activeModeHwcId) {
                        waitActiveMode = true;
                        break;
                    }
                    usleep(1000);
                    count++;
                };
                if (count == MAX_CYCLE_COUNT) {
                    ALOGE("Qsync set failed! No lock FPS(%d)", mQsyncBaseTiming);
                    mMiFlinger->mSetIdleForceFlag = false;
                    mMiFlinger->forceSkipIdleTimer(false);
                    if (mMiFlinger->mUnsetSettingFps != 0)
                        mMiFlinger->mCurrentSettingFps = mMiFlinger->mUnsetSettingFps;
                    if (mMiFlinger->mCurrentSettingFps < 60)
                        mMiFlinger->mCurrentSettingFps = 60;
                    mMiFlinger->changeFps(mMiFlinger->mCurrentSettingFps, false);
                    forceClearQsyncStatus();
                    return TIMED_OUT;
                }
            } else {
                if (!mMiFlinger->isFod()) {
                    mMiFlinger->mSetIdleForceFlag = false;
                    mMiFlinger->forceSkipIdleTimer(false);
                    if (mMiFlinger->mUnsetSettingFps != 0)
                        mMiFlinger->mCurrentSettingFps = mMiFlinger->mUnsetSettingFps;
                    if (mMiFlinger->mCurrentSettingFps < 60)
                        mMiFlinger->mCurrentSettingFps = 60;
                    mMiFlinger->changeFps(mMiFlinger->mCurrentSettingFps, false);
                }
            }
        }

        mDisplayConfigIntf->SetQsyncMode(disp, qsyncModeHwc);
        mQsyncEnableStatus = qsyncMode;
    }

    return ret;
}

void QsyncFeature::updateQsyncMinFps() {
    if (!mUseDynamicQsyncMinFps)
        return;

    if (mDisplayConfigIntf) {
        if (mDesiredVsyncHz > 0 && mQsyncBaseTiming > mDesiredVsyncHz)
            mQsyncMinFps |= static_cast<uint32_t>(mDesiredVsyncHz << DESIRED_VSYNC_OFFSET) & DESIRED_VSYNC_OFFSET_MASK;

        if (mQsyncMinFps != mLastQsyncMinFps)
            mDisplayConfigIntf->SetQsyncMinFpsIndex(disp, static_cast<uint32_t>(mQsyncMinFps));
        ALOGD("update qsync minfps(%d)-->(%d)", mLastQsyncMinFps, mQsyncMinFps);
        mLastQsyncMinFps = mQsyncMinFps;
    }
}

void QsyncFeature::updateVsync(bool fakeVsyncEnable) {
    if (!fakeVsyncEnable) {
        mFlinger->mScheduler->qsyncDisableVsync(false);
        mFlinger->mScheduler->disableHardwareVsync(true);
        mFlinger->mScheduler->resyncToHardwareVsync(true, Fps::fromPeriodNsecs(mFlinger->getVsyncPeriodFromHWC()), true);
        ALOGD("update qsync vsync done!");
    } else {
        size_t i = 0;
        const nsecs_t fake_period = static_cast<nsecs_t>(1e9f / mDesiredVsyncHz);
        int64_t timestamp = 0;
        const auto display = FTL_FAKE_GUARD(mFlinger->mStateLock, mFlinger->getDefaultDisplayDeviceLocked());
        const auto displayId = PhysicalDisplayId::tryCast(display->getId());
        const auto hwcDisplayId = mFlinger->getHwComposer().fromPhysicalDisplayId(*displayId);
        mFlinger->mScheduler->qsyncDisableVsync(false);

        mFlinger->mScheduler->disableHardwareVsync(true);
        mFlinger->mScheduler->resyncToHardwareVsync(true, Fps::fromPeriodNsecs(fake_period), true);

        mFlinger->updatePhaseConfiguration(Fps::fromValue(mDesiredVsyncHz));
        for (i = 0; i < kMinimumSamplesForPrediction; i++) {
            mFlinger->onComposerHalVsync(*hwcDisplayId, timestamp, fake_period);
            timestamp += fake_period;
        }
        mFlinger->updatePhaseConfiguration(Fps::fromValue(mDesiredVsyncHz));

        mFlinger->mScheduler->qsyncDisableVsync(true);
        ALOGD("update qsync fake vsync %dHZ done!", mDesiredVsyncHz);
    }
    mLastDesiredVsyncHz = mDesiredVsyncHz;
}

void QsyncFeature::forceClearQsyncStatus()
{
    mQsyncMinFps = 0;
    mDesiredVsyncHz = 0;
    updateQsyncMinFps();
    if (mQsyncEnableStatus) {
        auto qsyncModeHwc = ::DisplayConfig::QsyncMode::kNone;
        mDisplayConfigIntf->SetQsyncMode(disp, qsyncModeHwc);
        mQsyncEnableStatus = false;
    }
    //mMiFlinger->updateDisplayLockFps(0);
    updateVsync(false);
}

} // namespace android
