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

#ifndef TCP_SOCKET_MONITOR_H
#define TCP_SOCKET_MONITOR_H

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <android-base/thread_annotations.h>
#include <utils/String16.h>
#include "IMQSNative.h"

#include "Fwmark.h"

struct inet_diag_msg;
struct tcp_info;

namespace android {

class TcpSocketMonitor {
  public:
    using time_point = std::chrono::time_point<std::chrono::steady_clock>;

    using TcpStatusListenerMapRecip =
            std::map<sp<ITcpStatusListener> *, sp<IBinder::DeathRecipient>>;
    using TcpStatusListenerMap =
            std::map<sp<ITcpStatusListener> *, sp<ITcpStatusListener>>;


    static const std::chrono::milliseconds kDefaultPollingInterval;

    static TcpSocketMonitor *instance();
    static void finalize();
    TcpSocketMonitor();
    ~TcpSocketMonitor();

    void setPollingInterval(std::chrono::milliseconds nextSleepDurationMs);
    void resumePolling();
    void suspendPolling();
    void registerTcpStatusListener(int32_t uid, sp<ITcpStatusListener>& listener);
    void unregisterTcpStatusListener(int32_t uid, sp<ITcpStatusListener>& listener);

  private:
    void poll();
    void waitForNextPoll();
    bool isRunning();
    void updateSocketStats(Fwmark mark, const struct inet_diag_msg *sockinfo,
                           const struct tcp_info *tcpinfo, uint32_t tcpinfoLen, uint32_t count) REQUIRES(mLock);

    // Lock guarding all reads and writes to member variables.
    std::mutex mLock;
    // Used by the polling thread for sleeping between poll operations.
    std::condition_variable mCv;
    // The thread that polls sock_diag continuously.
    std::thread mPollingThread;
    // The duration of a sleep between polls. Can be updated by the instance owner for dynamically
    // adjusting the polling rate.
    std::chrono::milliseconds mNextSleepDurationMs GUARDED_BY(mLock);
    // The time of the last successful poll operation.
    time_point mLastPoll GUARDED_BY(mLock);
    // True if the polling thread should sleep until notified.
    bool mIsSuspended GUARDED_BY(mLock);
    // True while the polling thread should poll.
    bool mIsRunning GUARDED_BY(mLock);

    std::unordered_map<int32_t, TcpStatusListenerMap> mTcpStatusListenerMaps;
    TcpStatusListenerMapRecip mTcpStatusListenerMapRecip;

    uint32_t mCount = 0;
    uint32_t mRepeat = 0;
    uint32_t mSumRtt = 0;
    uint32_t mCurrentCount = -1;
};

}  // namespace android

#endif /* TCP_SOCKET_MONITOR_H */
