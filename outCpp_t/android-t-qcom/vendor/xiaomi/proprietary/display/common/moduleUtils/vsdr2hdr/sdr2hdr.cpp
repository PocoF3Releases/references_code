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
#include <timer.hpp>
#include <sys/inotify.h>
#include <display_property.h>
#include <mi_disp.h>
#include "sdr2hdr.h"
#include "DisplayEffectBase.h"

using snapdragoncolor::IMiStcService;

namespace android {
int DFLOG::loglevel = 1;
DisplayVSDR2HDR::DisplayVSDR2HDR(const sp<DisplayEffectBase>& display_effect)
      : mDisplayEffect(display_effect) {

    DF_LOGD("enter mHistControl = %d", mDisplayEffect->mHistControl);

    if (!mDisplayEffect.get()) {
        return;
    }

    if (mDisplayEffect->mHistControl) {
       mThreadHistExit = 0;
       mVsdr2hdrHistCond = PTHREAD_COND_INITIALIZER;
       mVsdr2hdrHistlock = PTHREAD_MUTEX_INITIALIZER;
        pthread_attr_t attc_hist;
        pthread_attr_init(&attc_hist);
        pthread_attr_setdetachstate(&attc_hist, PTHREAD_CREATE_JOINABLE);
        pthread_create(&mThreadVsdr2hdrHist, &attc_hist, ThreadWrapperVSDR2HDRHist, this);
        pthread_attr_destroy(&attc_hist);

        mVsdr2hdrCPIGCCond = PTHREAD_COND_INITIALIZER;
        mVsdr2hdrIGClock = PTHREAD_MUTEX_INITIALIZER;
        pthread_attr_t attc_igc;
        pthread_attr_init(&attc_igc);
        pthread_attr_setdetachstate(&attc_igc, PTHREAD_CREATE_JOINABLE);
        pthread_create(&mThreadVsdr2hdrIGC, &attc_igc, ThreadWrapperVSDR2HDRIGC, this);
        pthread_attr_destroy(&attc_igc);
    }

}

int DisplayVSDR2HDR::setVSDR2HDRFunction(int mode, int  cookie)
{
    int ret = 0;

    DF_LOGD("mDisplayId = %d, enable = %d", mDisplayEffect->mDisplayId, mode);

    if (128 != cookie)
        mVsdr2hdrEnabled = mode;

    if (VSDR2HDR_ON == mode) {
        histogram_operation(HIST_CMD_START);
        pthread_cond_signal(&mVsdr2hdrHistCond);
    } else if (VSDR2HDR_OFF == mode) {
        mContrastHistInfo = 3;
        histogram_operation(HIST_CMD_STOP);
        pthread_cond_signal(&mVsdr2hdrCPIGCCond);
    }

    return ret;
}

void DisplayVSDR2HDR::setDisplayState(int display_state)
{
    mDisplayState = display_state;
    if (mDisplayState != kStateOn && true == mVsdr2hdrEnabled) {
        DF_LOGV("Display isn't ON, close VSDR2HDR function");
        setVSDR2HDRFunction(VSDR2HDR_OFF, 0);
    }
}

int DisplayVSDR2HDR::getVSDR2HDRStatus()
{
    return mVsdr2hdrEnabled;
}

int DisplayVSDR2HDR::histogram_operation(int cmd)
{
    int value = 0;
    struct OutputParam sParams;

    sParams.function_id = DISPLAY_FEATURE_HIST;
    sParams.param2 = VSDR2HDR_FEATURE;
    Mutex::Autolock autoLock(mVsdr2hdrHistLock);

    switch ((enum HANDLE_HIST_CMD)cmd) {
        case HIST_CMD_START: {
            if (mHistStatus != HIST_CMD_STOP) {
                DF_LOGD("VSDR2HDR histogram_operation-Start, histgram status is wrong, status:%d!", mHistStatus);
                break;
            }

            sParams.param1 = HIST_CMD_START;
            mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
            mHistStatus = HIST_STARTED;
        } break;
        case HIST_CMD_GET: {
            if (mHistStatus != HIST_STARTED) {
                DF_LOGD("VSDR2HDR histogram_operation-Get, histgram status is wrong, status:%d!", mHistStatus);
                break;
            }

            sParams.param1 = HIST_CMD_GET;
            mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
            mContrastHistInfo = sParams.len;
            if (mContrastHistInfo > 3)
                mContrastHistInfo = 3;
        } break;
        case HIST_CMD_STOP: {
            if (mHistStatus != HIST_STARTED) {
                DF_LOGD("VSDR2HDR histogram_operation-Stop, histgram status is wrong, status:%d!", mHistStatus);
                break;
            }

            sParams.param1 = HIST_CMD_STOP;
            mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
            mHistStatus = HIST_STOPPED;
        } break;
        default:
            DF_LOGD("VSDR2HDR histogram_operation, cmd don't support!");
    }

    return mHistStatus;
}

void *DisplayVSDR2HDR::ThreadWrapperVSDR2HDRHist(void *me) {
    return (void *)(uintptr_t)static_cast<DisplayVSDR2HDR *>(me)->threadFuncVSDR2HDRHist();
}

int DisplayVSDR2HDR::threadFuncVSDR2HDRHist() {
    int last_contrast_hist_info = 3;

    while (mThreadHistExit == 0) {
        pthread_mutex_lock(&mVsdr2hdrHistlock);
        pthread_cond_wait(&mVsdr2hdrHistCond, &mVsdr2hdrHistlock);
        pthread_mutex_unlock(&mVsdr2hdrHistlock);

        usleep(30000);
        last_contrast_hist_info = 3;

        /*get histogram value*/
        while (histogram_operation(HIST_CMD_GET)) {
            if (mVsdr2hdrEnabled && last_contrast_hist_info != mContrastHistInfo) {
                pthread_cond_signal(&mVsdr2hdrCPIGCCond);
                last_contrast_hist_info = mContrastHistInfo;
            }

            /* read histogram value per 100ms*/
            usleep(100000);
        }
    }

    return 0;
}

void *DisplayVSDR2HDR::ThreadWrapperVSDR2HDRIGC(void *me) {
    return (void *)(uintptr_t)static_cast<DisplayVSDR2HDR *>(me)->threadFuncVSDR2HDRIGC();
}

int DisplayVSDR2HDR::threadFuncVSDR2HDRIGC() {
    int val  = 0, diff = 0;
    int totalCircle = 0;
    int levelGoal = 0;
    int lastContrastHistInfo = 0;
    struct OutputParam sParams;
    struct OutputParam sCloseParams;

    while (mThreadHistExit == 0) {
        pthread_mutex_lock(&mVsdr2hdrIGClock);
        pthread_cond_wait(&mVsdr2hdrCPIGCCond, &mVsdr2hdrIGClock);
        pthread_mutex_unlock(&mVsdr2hdrIGClock);

        if (lastContrastHistInfo == 3 && mContrastHistInfo == 3)
            continue;

        DF_LOGW("lastContrastHistInfo=%d, contrast_hist_info=%d",
            lastContrastHistInfo, mContrastHistInfo);

        if ((0 == lastContrastHistInfo && 2 == mContrastHistInfo))
            totalCircle = 30;
        else if (2 == lastContrastHistInfo && 0 == mContrastHistInfo)
            totalCircle = 50;
        else
            totalCircle = 20;

        lastContrastHistInfo = mContrastHistInfo;

        /* vsdr2hdr set hue and saturation logical */
        if (3 == lastContrastHistInfo) {
            levelGoal = 0;
        } else if ((0 == lastContrastHistInfo || 1 == lastContrastHistInfo || 2 == lastContrastHistInfo)) {
            levelGoal = 3;
        }
        if (!mVsdr2hdrEnabled || 3 == lastContrastHistInfo) {
            /* set to default gamma */
            sCloseParams.vec_payload.clear();
            sCloseParams.vec_payload.push_back(0);  // Enable
            sCloseParams.vec_payload.push_back(0);  // File ID
            sCloseParams.vec_payload.push_back(totalCircle); // Dimming Step
            sCloseParams.function_id = COLOR_GAMMA;
            sCloseParams.param1 = 0;
            sCloseParams.param2 = GAMMA_IGC;
            mDisplayEffect->setDisplayEffect(SET_DSPP_MODE, &sCloseParams);
            /* set to default hue and saturation */
            if (!mDisplayEffect->getEffectStatus(DISPLAY_EXPERT)
                || (mDisplayEffect->mCurParam.expertParam[4] == 0
                &&  mDisplayEffect->mCurParam.expertParam[5] == 0)) {
                    if (mDisplayEffect->getEffectStatus(DISPLAY_EXPERT)) {
                        sCloseParams.function_id = COLOR_PA;
                        sCloseParams.param1 = EXPERT_SRE_H_AND_S;
                        sCloseParams.param2 = 0;
                        sCloseParams.vec_payload.clear();
                        sCloseParams.vec_payload.push_back(1);
                        mDisplayEffect->setDisplayEffect(SET_DSPP_MODE, &sCloseParams);
                    } else {
                        sCloseParams.function_id = COLOR_PA_DISABLE;
                        mDisplayEffect->setDisplayEffect(SET_DSPP_MODE, &sCloseParams);
                    }
            }
            continue;
        }

        /* vsdr2hdr set stc logical */
        sParams.vec_payload.clear();
        sParams.vec_payload.push_back(1);  // Enable
        sParams.vec_payload.push_back(0);  // File ID
        sParams.vec_payload.push_back(totalCircle);  // Dimming Step
        for(int m = 0; m < IGC_TABLE_LEN; m++)
            sParams.vec_payload.push_back(mVsdr2hdrIgcGamma[lastContrastHistInfo][m]);
        {
            Mutex::Autolock autoLock(mVsdr2hdrIgcLock);
            sParams.function_id = COLOR_GAMMA;
            sParams.param1 = 0xFFF;  /* sre flag pass to stc interface */
            sParams.param2 = GAMMA_SRE_IGC;
            mDisplayEffect->setDisplayEffect(SET_DSPP_MODE, &sParams);
        }

        /* SRE set hue and saturation logical */
        if (!mDisplayEffect->getEffectStatus(DISPLAY_EXPERT)
               || (mDisplayEffect->mCurParam.expertParam[4] == 0
               && mDisplayEffect->mCurParam.expertParam[5] == 0)) {
               sParams.function_id = COLOR_PA;
               if (mDisplayEffect->getEffectStatus(DISPLAY_EXPERT))
                   sParams.param1 = EXPERT_SRE_H_AND_S;
               else
                   sParams.param1 = NOT_EXPERT_SRE_H_AND_S;
               sParams.param2 = ((levelGoal/2) << 8) |(0);
               sParams.vec_payload.clear();
               sParams.vec_payload.push_back(1);
               mDisplayEffect->setDisplayEffect(SET_DSPP_MODE, &sParams);
        }
    }

    return 0;
}

} //namespace android
