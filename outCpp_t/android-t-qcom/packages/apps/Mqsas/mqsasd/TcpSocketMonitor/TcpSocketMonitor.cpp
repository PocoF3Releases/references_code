/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "mqsasd-TcpSocketMonitor"

#include <iostream>
#include <chrono>
#include <cinttypes>
#include <thread>
#include <vector>
#include <string>

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <linux/tcp.h>
#include <cutils/log.h>
#include<string.h>
#include <stdio.h>
#include <stdlib.h>

#include "SockDiag.h"
#include "TcpSocketMonitor.h"

namespace android {

using std::chrono::duration_cast;
using std::chrono::steady_clock;
using std::chrono::milliseconds;

#define FILTER_SERVER "::ffff:"

static TcpSocketMonitor *mInstance = NULL;
//tcp diag interval
const milliseconds TcpSocketMonitor::kDefaultPollingInterval = milliseconds(5000);


// Helper macro for reading fields into struct tcp_info and handling different struct tcp_info
// versions in the kernel.
#define TCPINFO_GET(ptr, fld, len, zero) \
        (((ptr) != nullptr && (offsetof(struct tcp_info, fld) + sizeof((ptr)->fld)) < len) ? \
        (ptr)->fld : zero)

void TcpSocketMonitor::setPollingInterval(milliseconds nextSleepDurationMs) {

    std::lock_guard<std::mutex> guard(mLock);

    mNextSleepDurationMs = nextSleepDurationMs;

    ALOGD(" tcpinfo polling interval set to %lld ms", mNextSleepDurationMs.count());
}

void TcpSocketMonitor::resumePolling() {
    ALOGD("resumePolling");
    bool wasSuspended;
    {
        std::lock_guard<std::mutex> guard(mLock);

        wasSuspended = mIsSuspended;
        mIsSuspended = false;
        ALOGD(" resuming tcpinfo polling (interval=5)");
    }

    if (wasSuspended) {
        mCv.notify_all();
    }
}

void TcpSocketMonitor::suspendPolling() {
    ALOGD("suspendPolling");
    std::lock_guard<std::mutex> guard(mLock);
    mIsSuspended = true;
}

void TcpSocketMonitor::poll() {
    std::lock_guard<std::mutex> guard(mLock);
    if (mIsSuspended) {
        return;
    }
    android::SockDiag sd;
    if (!sd.open()) {
        ALOGE(" Error opening sock diag for polling TCP socket info");
        return;
    }

    const auto now = steady_clock::now();
    const auto tcpInfoReader = [this](Fwmark mark ,const struct inet_diag_msg *sockinfo,
                                           const struct tcp_info *tcpinfo,
                                           uint32_t tcpinfoLen, uint32_t count) NO_THREAD_SAFETY_ANALYSIS {
        if (sockinfo == nullptr || tcpinfo == nullptr || tcpinfoLen == 0) {
            return;
        }
        updateSocketStats(mark, sockinfo, tcpinfo, tcpinfoLen, count);
    };

    if (int ret = sd.getLiveTcpInfos(tcpInfoReader, mCount)) {
        ALOGE(" Failed to poll TCP socket info: %s",strerror(-ret));
        return;
    }
    mCount++;
    mCount %= 16;
    mLastPoll = now;
}

void TcpSocketMonitor::waitForNextPoll() {
    bool isSuspended;
    milliseconds nextSleepDurationMs;
    {
        std::lock_guard<std::mutex> guard(mLock);
        isSuspended = mIsSuspended;
        nextSleepDurationMs= mNextSleepDurationMs;
    }

    std::unique_lock<std::mutex> ul(mLock);
    if (isSuspended) {
        mCv.wait(ul);
    } else {
        mCv.wait_for(ul, nextSleepDurationMs);
    }
}

bool TcpSocketMonitor::isRunning() {
    std::lock_guard<std::mutex> guard(mLock);
    return mIsRunning;
}

void TcpSocketMonitor::updateSocketStats(Fwmark mark,
                                         const struct inet_diag_msg *sockinfo,
                                         const struct tcp_info *tcpinfo,
                                         uint32_t tcpinfoLen, uint32_t count) NO_THREAD_SAFETY_ANALYSIS {

    uint32_t uid = sockinfo->idiag_uid;
    uint32_t rtt = TCPINFO_GET(tcpinfo, tcpi_rtt, tcpinfoLen, 0);
    uint32_t netId = mark.netId;

    char daddr[INET6_ADDRSTRLEN] = {};
    uint32_t host;
    inet_ntop(sockinfo->idiag_family, &(sockinfo->id.idiag_dst), daddr, sizeof(daddr));
    host = ntohs(sockinfo->id.idiag_dport);

    if (host == 80 || host == 443) {
        return;
    }

    // Change when move to real test enviroment
    auto it = mTcpStatusListenerMaps.find(uid);
    if (it == mTcpStatusListenerMaps.end()) {
       return;
    }
    for (auto it_ = it->second.begin(); it_ != it->second.end(); ++it_) {
        if (mCurrentCount != count ) {
            if (mSumRtt != 0 && mRepeat != 0) {
                it_->second->onRttInfoEvent(uid, mSumRtt / mRepeat, netId);
            }
            mCurrentCount = count;
            mSumRtt = rtt;
            mRepeat = 1;
        } else {
            mSumRtt += rtt;
            mRepeat++;
        }
    }
}

void TcpSocketMonitor::registerTcpStatusListener(int32_t uid, sp<ITcpStatusListener>& listener) {

    ALOGD("registerTcpStatusListener: %p", &listener);
    // Create the death listener.
    class DeathRecipient : public IBinder::DeathRecipient {
      public:
        DeathRecipient(TcpSocketMonitor* tcpSocketMonitor,
                       int32_t _uid, android::sp<ITcpStatusListener>& _listener)
            : mTcpSocketMonitor(tcpSocketMonitor), mUid(_uid), mListener(_listener) {
            ALOGD("DeathRecipient mListener: %p", &mListener);
        }
        ~DeathRecipient() override = default;
        void binderDied(const android::wp<android::IBinder>& ) override {
            mTcpSocketMonitor->unregisterTcpStatusListener(mUid, mListener);
        }

      private:
        TcpSocketMonitor* mTcpSocketMonitor;
        int32_t mUid;
        android::sp<ITcpStatusListener>& mListener;
    };
    sp<IBinder::DeathRecipient> deathRecipient = new DeathRecipient(this, uid, listener);
    android::IInterface::asBinder(listener)->linkToDeath(deathRecipient);

    mTcpStatusListenerMapRecip.insert({&listener, deathRecipient});

    auto it = mTcpStatusListenerMaps.find(uid);
    if (it == mTcpStatusListenerMaps.end()) {
        TcpStatusListenerMap listenerMap = {{&listener, listener}};
        mTcpStatusListenerMaps.insert(std::pair<int32_t, TcpStatusListenerMap>(uid, listenerMap));
    } else {
        it->second.insert({&listener, listener});
    }

    resumePolling();
}

void TcpSocketMonitor::unregisterTcpStatusListener(int32_t uid, sp<ITcpStatusListener>& listener) {

    ALOGD("unregisterTcpStatusListener: %p", &listener);
    TcpStatusListenerMapRecip::iterator mapDeathIt = mTcpStatusListenerMapRecip.find(&listener);
    if (mapDeathIt != mTcpStatusListenerMapRecip.end()) {
        mTcpStatusListenerMapRecip.erase(mapDeathIt);
    }

    auto listenerMapIt = mTcpStatusListenerMaps.find(uid);
    if (listenerMapIt != mTcpStatusListenerMaps.end()) {
        TcpStatusListenerMap listenerMap = listenerMapIt->second;
        TcpStatusListenerMap::iterator it = listenerMap.find(&listener);
        if (it != listenerMap.end()) {
            ALOGD("remove listener, key: %p", it->first);
            listenerMap.erase(it);
        }
        if (!listenerMap.empty()) {
            return;
        }
    }

    mTcpStatusListenerMaps.erase(uid);
    if (mTcpStatusListenerMaps.empty()) {
        suspendPolling();
    }
}

TcpSocketMonitor::TcpSocketMonitor() {
    std::lock_guard<std::mutex> guard(mLock);

    mNextSleepDurationMs = kDefaultPollingInterval;
    mIsRunning = true;
    mIsSuspended = true;

    mPollingThread = std::thread([this] {
       (void) this;
       while (isRunning()) {
            poll();
            waitForNextPoll();
        }
    });
}

TcpSocketMonitor::~TcpSocketMonitor() {
    {
        std::lock_guard<std::mutex> guard(mLock);
        mIsRunning = false;
        mIsSuspended = true;
    }
    mCv.notify_all();
    mPollingThread.join();
}

TcpSocketMonitor *TcpSocketMonitor::instance() {
    if (mInstance == NULL) {
        mInstance = new TcpSocketMonitor();
    }
    return mInstance;
}

void TcpSocketMonitor::finalize() {
   if (mInstance != NULL) {
      delete mInstance;
      mInstance = NULL;
   }
}

}  // namespace android
