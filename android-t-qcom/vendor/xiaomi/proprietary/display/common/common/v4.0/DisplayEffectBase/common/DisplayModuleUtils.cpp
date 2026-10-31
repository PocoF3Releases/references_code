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
#include <display_property.h>
#include "DisplayModuleUtils.h"
#include "DisplayEffectBase.h"
#include "FodSensorMonitor.h"
#include <mi_disp.h>

namespace android {

DisplayBCBC::DisplayBCBC(const sp<DisplayEffectBase>& display_effect)
    : mDisplayEffect(display_effect) {

  DF_LOGD("enter mHistControl = %d", mDisplayEffect->mHistControl);

  if (!mDisplayEffect.get()) {
      return;
  }

  if (mDisplayEffect->mHistControl) {
     mThreadHistExit = 0;
     mBCBCHistCond = PTHREAD_COND_INITIALIZER;
     mBCBCHistlock = PTHREAD_MUTEX_INITIALIZER;
      pthread_attr_t attra_hist;
      pthread_attr_init(&attra_hist);
      pthread_attr_setdetachstate(&attra_hist, PTHREAD_CREATE_JOINABLE);
      pthread_create(&mThreadBCBCHist, &attra_hist, ThreadWrapperBCBCHist, this);
      pthread_attr_destroy(&attra_hist);
  }
}

void DisplayBCBC::configureBCBC()
{
    return;
}

void DisplayBCBC::setDisplayState(int display_state)
{
    mDisplayState = display_state;
    if (mDisplayState != kStateOn && BCBC_ON == mBCBCEnabled) {
        DF_LOGV("Display isn't ON, close bcbc function");
        setBCBCFunction(BCBC_OFF, 1);
    } else if (mDisplayState == kStateOn && BCBC_ON == mBCBCEnabled) {
        setBCBCFunction(BCBC_ON, 1);
    }
}

int DisplayBCBC::setBCBCFunction(int mode, int  cookie)
{
    if (BCBC_ON == mode) {
        DF_LOGD("open BCBC");
        if (mHistStatus == HIST_STOPPED)
            histogram_operation(HIST_CMD_START);
        pthread_cond_signal(&mBCBCHistCond);
    } else if (BCBC_OFF == mode) {
        DF_LOGD("close BCBC");
        if (mHistStatus == HIST_STARTED)
            histogram_operation(HIST_CMD_STOP);
    }

    if (1 != cookie)
        mBCBCEnabled = mode;

    return 0;
}

int DisplayBCBC::getBCBCStatus()
{
    return mBCBCEnabled;
}

int DisplayBCBC::histogram_operation(int cmd)
{
    int value = 0;
    struct OutputParam sParams;

    sParams.function_id = DISPLAY_FEATURE_HIST;
    sParams.param2 = BCBC_FEATURE;
    Mutex::Autolock autoLock(mHistLock);

    switch ((enum HANDLE_HIST_CMD)cmd) {
        case HIST_CMD_START: {
            if (mHistStatus != HIST_CMD_STOP) {
                DF_LOGD("BCBC histogram_operation-Start, histgram status is wrong, status:%d!", mHistStatus);
                break;
            }

            sParams.param1 = HIST_CMD_START;
            mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
            mHistStatus = HIST_STARTED;
        } break;
        case HIST_CMD_GET: {
            if (mHistStatus != HIST_STARTED) {
                DF_LOGD("BCBC histogram_operation-Get, histgram status is wrong, status:%d!", mHistStatus);
                break;
            }

            sParams.param1 = HIST_CMD_GET;
            mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
        } break;
        case HIST_CMD_STOP: {
            if (mHistStatus != HIST_STARTED) {
                DF_LOGD("BCBC histogram_operation-Stop, histgram status is wrong, status:%d!", mHistStatus);
                break;
            }

            sParams.param1 = HIST_CMD_STOP;
            mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
            mHistStatus = HIST_STOPPED;
        } break;
        default:
            DF_LOGD("BCBC histogram_operation, cmd don't support!");
    }

    return mHistStatus;
}

void *DisplayBCBC::ThreadWrapperBCBCHist(void *me) {
    return (void *)(uintptr_t)static_cast<DisplayBCBC *>(me)->threadFuncBCBCHist();
}

int DisplayBCBC::threadFuncBCBCHist() {

    while (mThreadHistExit == 0) {
        pthread_mutex_lock(&mBCBCHistlock);
        pthread_cond_wait(&mBCBCHistCond, &mBCBCHistlock);
        pthread_mutex_unlock(&mBCBCHistlock);

        usleep(20000);

        /*get histogram value*/
        while (histogram_operation(HIST_CMD_GET)) {
            /* read histogram value per 500ms*/
            usleep(500000);
        }
    }

    return 0;
}

/* frame rate setting*/
DisplayFramerateSwitchThread::DisplayFramerateSwitchThread(const char* name
	, int display_id)
      : mName(name),
        mDisplayId(display_id),
        mRunning(false),
        mFramerateIndex(-1) {

}

DisplayFramerateSwitchThread::~DisplayFramerateSwitchThread()
{
    mRunning = false;
    requestExitAndWait();
}

void DisplayFramerateSwitchThread::runThreadLoop()
{
    if (getTid() == -1) {
        mRunning = true;
        run(mName, android::PRIORITY_DISPLAY);
        ALOGD("DisplayFramerateSwitchThread(%s) : runThreadLoop", mName);
    }
}

void DisplayFramerateSwitchThread::setFramerateIndex(int index)
{
    mFramerateIndex = index;
    ALOGD("setFramerateIndex, index:%d", index);
}

bool DisplayFramerateSwitchThread::threadLoop()
{
    int retry_cnt = 600;
    int cmd_type = 0;
    bool is_callback_ready = false;

    while (mRunning && retry_cnt--) {
        is_callback_ready = DisplayUtil::isRegisterCallbackDone();
        if (is_callback_ready && mFramerateIndex >= 0 ) {
            cmd_type = (mDisplayId ? 10036 : 10035);
            DisplayUtil::onCallback(cmd_type, mFramerateIndex);
            ALOGD("threadloop, set fps idex:%d", mFramerateIndex);
            return false;
        }

        usleep(200000);
    }

    mRunning = false;

    return false;
}
}
