/**
 * Copyright (c) 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NETD_SERVER_OEM_NETD_LISTENER_H
#define NETD_SERVER_OEM_NETD_LISTENER_H

#include <map>
#include <mutex>

#include <android-base/thread_annotations.h>
#include "com/android/internal/net/BnOemNetd.h"
#include "com/android/internal/net/IOemNetdUnsolicitedEventListener.h"

namespace com {
namespace android {
namespace internal {
namespace net {

class OemNetdListener : public BnOemNetd {
  public:
    using OemUnsolListenerMap = std::map<const ::android::sp<IOemNetdUnsolicitedEventListener>,
                                         const ::android::sp<::android::IBinder::DeathRecipient>>;

    OemNetdListener() = default;
    ~OemNetdListener() = default;
    static ::android::sp<::android::IBinder> getListener();

    ::android::binder::Status isAlive(bool* alive) override;
    ::android::binder::Status registerOemUnsolicitedEventListener(
            const ::android::sp<IOemNetdUnsolicitedEventListener>& listener) override;
    // MIUI ADD: START
    ::android::binder::Status enableWmmer(bool enabled, bool* ret) override;
    ::android::binder::Status enableLimitter(bool enabled, bool* ret) override;
    ::android::binder::Status updateWmm(int uid, int wmm, bool* ret) override;
    ::android::binder::Status whiteListUid(int uid, bool add, bool* ret) override;
    ::android::binder::Status setLimit(bool enabled, long rate, bool* ret) override;
    ::android::binder::Status enableIptablesRestore(bool enabled, bool* ret) override;
    ::android::binder::Status listenUidDataActivity(int protocol, int uid, int label,
                                                           int timeout, bool listen, bool* ret) override;
    ::android::binder::Status updateIface(const std::string& iface, bool* ret) override;
    ::android::binder::Status addMiuiFirewallSharedUid(int uid, bool* ret) override;
    ::android::binder::Status setMiuiFirewallRule(const std::string& packageName, int uid, int rule,
                                                        int type, bool* ret) override;
    ::android::binder::Status setCurrentNetworkState(int state, bool* ret) override;
    ::android::binder::Status enableRps(const std::string& iface, bool enable, bool* ret) override;

    ::android::binder::Status notifyFirewallBlocked(int code, const std::string& packageName);
    // Returns a copy of the registered listeners.
    OemUnsolListenerMap getOemNetdUnsolicitedEventListenerMap() EXCLUDES(mOemUnsolicitedMutex);
    // ::android::binder::Status doFireWallForUid(const ::std::string& fwType, const ::std::vector<int32_t>& uids, bool isWhitelistUid);
    // ::android::binder::Status doRestoreSockForUid(const ::std::string& fwType);
    // ::android::binder::Status socketDestroy(const std::vector<::android::net::UidRangeParcel>& uids,const std::vector<int32_t>& skipUids) override;
    ::android::binder::Status enableQos(bool enabled, bool* ret) override;
    ::android::binder::Status setQos(int protocol, int uid, int tos, bool add, bool* ret) override;
    ::android::binder::Status setPidForPackage(const ::std::string& packageName, int pid, int uid);

    // Mobile Traffic Controller
    ::android::binder::Status enableMobileTrafficLimit(bool enabled, const std::string& iface, bool* ret) override;
    ::android::binder::Status setMobileTrafficLimit(bool enabled, long rate, bool* ret) override;
    ::android::binder::Status whiteListUidForMobileTraffic(int uid, bool add, bool* ret) override;
    // SLM bpf VOIP Address map
    ::android::binder::Status setMiuiSlmBpfUid(int uid, bool* ret);
    ::android::binder::Status getMiuiSlmVoipUdpAddress(int uid, int64_t* aidl_return);
    ::android::binder::Status getMiuiSlmVoipUdpPort(int uid, int32_t* aidl_return);

    ::android::binder::Status enableAutoForward(const std::string& addr, int fwmark, bool enabled, int32_t* aidl_return) override;
    // END
  private:
    std::mutex mOemUnsolicitedMutex;

    OemUnsolListenerMap mOemUnsolListenerMap GUARDED_BY(mOemUnsolicitedMutex);

    void registerOemUnsolicitedEventListenerInternal(
            const ::android::sp<IOemNetdUnsolicitedEventListener>& listener)
            EXCLUDES(mOemUnsolicitedMutex);
    void unregisterOemUnsolicitedEventListenerInternal(
            const ::android::sp<IOemNetdUnsolicitedEventListener>& listener)
            EXCLUDES(mOemUnsolicitedMutex);
};

}  // namespace net
}  // namespace internal
}  // namespace android
}  // namespace com

#endif  // NETD_SERVER_OEM_NETD_LISTENER_H
