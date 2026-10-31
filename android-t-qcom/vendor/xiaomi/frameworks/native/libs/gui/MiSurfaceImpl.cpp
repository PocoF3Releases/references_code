/*
 *    Copyright 2021, Xiaomi Corporation All rights reserved.
 *
 *    Add sensorlist whitelist feature for idle task control
 *
 */

#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <utils/Log.h>
#include <utils/Trace.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cutils/properties.h>
#include "MiSurfaceImpl.h"
#include "FrameBudgetIndicator.h"
#include "Framectr.h"
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

namespace android {

void MiSurfaceImpl::skipSlotRef(sp<Surface> surface, int i)
{
    if (mCurrentDFPid == -1) {
        mCurrentDFPid = getpid();
    }
    if ((mCurrentDFPid != -1) && mDFProcName.empty())
    {
        getDynamicFpsPidName(mCurrentDFPid);
    }

    if (!mDFProcName.empty() && mDFProcName.find("com.android.camera") != std::string::npos)
    {
        std::string consumeName(surface->getConsumerName().c_str());
        if (surface != NULL && consumeName.find("MiuiCamera-Snapshot") != std::string::npos) {
            if (surface->mReportRemovedBuffers) {
                surface->mRemovedBuffers.push_back(surface->mSlots[i].buffer);
            }

            surface->mSlots[i].buffer = nullptr;
            ALOGV("%s::queueBuffer and slot cleared", surface->getConsumerName().string());
        }
    }
}

MiSurfaceImpl::~MiSurfaceImpl(){
    if(wdInotify != -1 && fdInotify != -1) {
        inotify_rm_watch(fdInotify, wdInotify);
    }
    if(fdInotify != -1) {
        close(fdInotify);
    }
}

void MiSurfaceImpl::updateDynamicFPSConfig(std::string mDFProcName) {
    const char *dynamicFpsConfig = "/data/system/mcd/df";
    std::ifstream in(dynamicFpsConfig);
    std::string line;
    if (in.is_open()) {
        while (std::getline(in, line)) {
            std::istringstream iss(line);
            std::string package;
            int dynamicFps;
            if (!(iss >> package >> dynamicFps)) {
                mDynamicFps = -1;
                ALOGE("DF parse error: %s", strerror(errno));
                in.close();
                return ;
            } else {
                if (mDFProcName.find(package) != std::string::npos || flagInotify) {
                    mDynamicFps = dynamicFps;
                    ALOGI("DF top: %s : %d", mDFProcName.c_str(), mDynamicFps);
                    in.close();
                    return ;
                }
            }
        }
        in.close();
    } else {
        ALOGE("DF open fail: %s", strerror(errno));
    }
    mDynamicFps = -1;
}

void MiSurfaceImpl::getDynamicFpsPidName(int mCurrentDFPid) {
    std::string mDynamicFpsPidPath = "/proc/" + std::to_string(mCurrentDFPid) + "/cmdline";
    std::ifstream in(mDynamicFpsPidPath);
    if (in.is_open()) {
        Mutex::Autolock lock(mMutex);
        while (std::getline(in, mDFProcName)) {
            if (!(mDFProcName.empty())) {
                break ;
            }
        }
        in.close();
    } else {
        ALOGE("DF is not exist");
    }
    return ;
}

void MiSurfaceImpl::doDynamicFps()
{
    // currently we update the target fps every 5s
    const int updateInternal = 5;

    int64_t timestamp = systemTime(SYSTEM_TIME_MONOTONIC);

    if (CC_UNLIKELY(mLastQueueBufferTimestamp == NATIVE_WINDOW_TIMESTAMP_AUTO)) {
        mLastQueueBufferTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);
    }

    if (mCurrentDFPid == -1) {
        mCurrentDFPid = getpid();
    }

    if ((mCurrentDFPid != -1) && mDFProcName.empty()) {
        getDynamicFpsPidName(mCurrentDFPid);
        // Why transfer updateDynamicFPSConfig() here? When the process instantiate Surface,
        // we judge the current process whether Support DF or not.
        updateDynamicFPSConfig(mDFProcName);
        if (mDynamicFps >= 0) {
            ALOGI("DF %s Support Dynamic FPS.", mDFProcName.c_str());
            mSupportDF = true;
            mLastUpdateTimestamp = mLastQueueBufferTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);
            if (fdInotify == -1) {
                fdInotify = inotify_init1(O_NONBLOCK);
            }
            if (wdInotify == -1) {
                wdInotify = inotify_add_watch(fdInotify, "/data/system/mcd/df", IN_CLOSE_WRITE);
            }
            if (fdInotify != -1 && wdInotify != -1) {
                flagInotify = true;
            } else {
                ALOGE("Fail to use INO for dynamic FPS");
            }
        }
    }

    if (flagInotify) {
        if (fdInotify >= 0) {
            resReadInotify = read(fdInotify, bufInotify, BUF_LEN);
        }
        if (resReadInotify > 0) {
            updateDynamicFPSConfig(mDFProcName);
        }
    } else if ((mDynamicFps > 0 && mSupportDF) && ns2s(timestamp - mLastUpdateTimestamp) > updateInternal) {
        updateDynamicFPSConfig(mDFProcName);
        mLastUpdateTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);
    }
    resReadInotify = -1;
    if (CC_UNLIKELY(mDynamicFps > 0 && mSupportDF)) {
        int64_t sleepTime = s2ns(1)/mDynamicFps - (timestamp - mLastQueueBufferTimestamp);
        if (sleepTime > 0) {
            ATRACE_INT("DF", mDynamicFps);
            std::this_thread::sleep_for(std::chrono::nanoseconds(sleepTime));
        }
        mLastQueueBufferTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);
    }
}

int MiSurfaceImpl::doNotifyFbc(const int32_t& value){
   int ret = 0;
   ret = notifyFbc(value);
   return ret;
}

int MiSurfaceImpl::DoNotifyFbc(const int32_t& value, const uint64_t& bufID, const int32_t& param, const uint64_t& id){
   int ret = 0;
   ret = NotifyFbc(value,bufID,param,id);
   return ret;
}

bool MiSurfaceImpl::supportFEAS() {
    if (mInited) {
        return mEnableFEAS;
    }

    //You can add the support products at here
    char targetProducts[] = "ingres#diting#munch#socrates_cn#mondrian";
    char device[PROPERTY_VALUE_MAX] = { 0 };
    if (property_get("ro.product.product.name", device, NULL) > 0) {
        if(strstr(targetProducts, device)) {
            mInited =  true;
            mEnableFEAS = true;
            return true;
        } else {
            mInited =  true;
            mEnableFEAS = false;
            return false;
        }
    }

    mInited =  true;
    mEnableFEAS = false;
    return false;
}

extern "C" IMiSurfaceStub* create() {
    return new MiSurfaceImpl;
}

extern "C" void destroy(IMiSurfaceStub* impl) {
    delete impl;
}

}
