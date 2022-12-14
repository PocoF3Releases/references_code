#define LOG_TAG "octvm_power"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "AutoSavingThread.h"

extern "C" {
#include "klo_cpu_utils.h"
}

using namespace android;

AutoSavingThread::AutoSavingThread(struct autosave_params *params):
        mEnabled(false), mCpuNum(0), mCycleStartTime(0), mLastAllCpuTimeArray(NULL) {
    mWindowLength = params->windowLength;
    mForceIdleOffPct = params->forceIdleOffPct;
    mForceBusyOffPct = params->forceBusyOffPct;
}

bool AutoSavingThread::setWindowLength(unsigned long windowLength)
{
    if (windowLength != 0) {
        Mutex::Autolock lock(mLock);
        mWindowLength = windowLength;
        return true;
    }
    return false;
}

bool AutoSavingThread::setForceIdleOffPct(int forceIdleOffPct)
{
    if (forceIdleOffPct >= 0 && forceIdleOffPct <= 100) {
        Mutex::Autolock lock(mLock);
        mForceIdleOffPct = forceIdleOffPct;
        return true;
    }
    return false;
}

bool AutoSavingThread::setForceBusyOffPct(int forceBusyOffPct)
{
    if (forceBusyOffPct >= 0 && forceBusyOffPct <= 100) {
        Mutex::Autolock lock(mLock);
        mForceBusyOffPct = forceBusyOffPct;
        return true;
    }
    return false;
}

bool AutoSavingThread::updateThreadParam(struct autosave_params *newParams)
{
    int nFIOPct = mForceIdleOffPct;
    int nFBOPct = mForceBusyOffPct;
    unsigned long nWindow = mWindowLength;

    if (newParams->windowLength != 0) {
        nWindow = newParams->windowLength;
    }
    if (newParams->forceIdleOffPct >= 0 && newParams->forceIdleOffPct <= 100) {
        nFIOPct = newParams->forceIdleOffPct;
    }
    if (newParams->forceBusyOffPct >= 0 && newParams->forceBusyOffPct <= 100) {
        nFBOPct = newParams->forceBusyOffPct;
    }
    if (nWindow != 0 && nFIOPct >= 0 && nFIOPct <= 100 && nFBOPct >= 0 && nFBOPct <= 100) {
        Mutex::Autolock lock(mLock);
        mWindowLength = nWindow;
        mForceIdleOffPct = nFIOPct;
        mForceBusyOffPct = nFBOPct;
        return true;
    }
    return false;
}

bool AutoSavingThread::updateThreadParam(cJSON *config_data)
{
    bool ret = false;
    if (config_data != NULL) {
        struct autosave_params *params = (autosave_params*)calloc(1, sizeof(*params));
        cJSON *pData = cJSON_GetObjectItem(config_data, "windowLength");
        if (pData != NULL && pData->type == cJSON_String) {
            params->windowLength = strtoul(pData->valuestring, NULL, 0);
        }
        pData = cJSON_GetObjectItem(config_data, "forceIdleOffPct");
        if (pData != NULL && pData->type == cJSON_String) {
            params->forceIdleOffPct = atoi(pData->valuestring);
        }
        pData = cJSON_GetObjectItem(config_data, "forceBusyOffPct");
        if (pData != NULL && pData->type == cJSON_String) {
            params->forceBusyOffPct = atoi(pData->valuestring);
        }
        ret = updateThreadParam(params);
        free(params);
    }
    return ret;
}

void AutoSavingThread::dumpParams(int fd)
{
    String8 dBuffer;
    dBuffer.appendFormat("\nCurrent AutoSaving Parameters:\n");
    dBuffer.appendFormat("\twindowLength:%lu\n", mWindowLength);
    dBuffer.appendFormat("\tforceIdleOffPct:%d\n", mForceIdleOffPct);
    dBuffer.appendFormat("\tforceBusyOffPct:%d\n", mForceBusyOffPct);
    dBuffer.appendFormat("\n");
    write(fd, dBuffer.string(), dBuffer.length());
}

void AutoSavingThread::onPowerModeChanged(PowerModeHandler* targetModeHandler)
{
    printf("onPowerModeChanged[%s] called", targetModeHandler->getModeProperty()->mode_property);

    bool currentState = mEnabled;
    bool targetState = targetModeHandler->getModeProperty()->autosave != 0;
    struct autosave_params *newParams =targetModeHandler->getCustAutoSaveParams();
    if (targetState == true && newParams != NULL) {
        updateThreadParam(newParams);
    }
    if (currentState != targetState) {
        Mutex::Autolock lock(mLock);
        if (targetState == true && !isRunning()) {
            run("PowerAutoSave", PRIORITY_NORMAL);
        }
        mEnabled = targetState;
        if (mEnabled) {
            printf("Enable AutoSaving(w %lu, iof %d, bof %d)", mWindowLength, mForceIdleOffPct, mForceBusyOffPct);
        } else {
            printf("Disable AutoSaving");
        }
    }
}

bool AutoSavingThread::threadLoop() {
    if (mWindowLength == 0) {
        fprintf(stderr, "AutoSaving Thread should run with valid window length");
        return false;
    }

    bool enable = false;
    unsigned long sleepInterval;
    mCpuNum = get_total_cpus();
    mCpuStateArray = (unsigned int *)calloc(mCpuNum, sizeof(unsigned int));
    mLastAllCpuTimeArray = (unsigned long *)calloc(mCpuNum, sizeof(unsigned long));

    while (!exitPending()) {
        {
            Mutex::Autolock lock(mLock);
            sleepInterval = mWindowLength * 1000;
            if (mEnabled) {
                struct timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                mCycleStartTime = ts.tv_sec*1000 + ts.tv_nsec/1000;
                get_all_cpu_time(mLastAllCpuTimeArray, mCpuNum);
            }
            enable = mEnabled;
        }
        usleep(sleepInterval);
        {
            Mutex::Autolock lock(mLock);
            unsigned long cycleEndTime, timeGap;
            unsigned long *curAllCpuTimeArray = NULL;
            if (enable) {
                struct timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                cycleEndTime = ts.tv_sec*1000 + ts.tv_nsec/1000;
                timeGap = (cycleEndTime - mCycleStartTime)/10;  // ms -> jiffies
                curAllCpuTimeArray = (unsigned long *)calloc(mCpuNum, sizeof(unsigned long));
                get_all_cpu_time(curAllCpuTimeArray, mCpuNum);
                for (int i = 0; i < mCpuNum; i++) {
                    unsigned long cpui = curAllCpuTimeArray[i] - mLastAllCpuTimeArray[i];
                    if (cpui > (timeGap*mForceBusyOffPct + 50)/100) {
                        mCpuStateArray[i] = CPU_STATE_BUSY;
                    } else if (cpui < (timeGap*mForceIdleOffPct + 50)/100) {
                        mCpuStateArray[i] = CPU_STATE_IDLE;
                    } else {
                        mCpuStateArray[i] = CPU_STATE_NORMAL;
                    }
                }
                platform_autosave_handle(cycleEndTime - mCycleStartTime, mCpuStateArray, mCpuNum);
                free(curAllCpuTimeArray);
            }
        }
    }
    free(mLastAllCpuTimeArray);
    free(mCpuStateArray);
    return false;
}