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

#define LOG_TAG "OemNetd"

#include "OemNetdListener.h"
// MIUI ADD
#include "OemNetd.h"

namespace com {
namespace android {
namespace internal {
namespace net {

::android::sp<::android::IBinder> OemNetdListener::getListener() {
    // Thread-safe initialization.
    static ::android::sp<OemNetdListener> listener = ::android::sp<OemNetdListener>::make();
    static ::android::sp<::android::IBinder> sIBinder = ::android::IInterface::asBinder(listener);
    return sIBinder;
}

::android::binder::Status OemNetdListener::isAlive(bool* alive) {
    *alive = true;
    return ::android::binder::Status::ok();
}

// MIUI ADD: START
::android::binder::Status OemNetdListener::enableWmmer(bool enabled, bool* ret) {
    *ret = true;
    ALOGD("enableWmmer: %s", enabled ? "enable" : "disable");
    if( OemNetd::enableWmmer(enabled) != 0 )
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::enableLimitter(bool enabled, bool* ret) {
    *ret = true;
    ALOGD("enableLimitter: %s", enabled ? "enable" : "disable");
    if( OemNetd::enableLimiter(enabled) != 0 )
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::updateWmm(int uid, int wmm, bool* ret) {
    *ret = true;
    ALOGD("updateWmm: uid=%d, wmm=%d", uid, wmm);
    if( OemNetd::updateWmm(uid, wmm) != 0 )
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::whiteListUid(int uid, bool add, bool* ret) {
    *ret = true;
    ALOGD("whiteListUid: uid=%d, wmm=%s", uid, add ? "add" : "del");
    if( OemNetd::whiteListUid(uid, add) != 0 )
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::setLimit(bool enabled, long rate, bool* ret) {
    *ret = true;
    ALOGD("setLimit: %s, rate=%ld", enabled ? "enable" : "disable", rate);
    if( OemNetd::limit(enabled, rate) != 0 )
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::enableIptablesRestore(bool enabled, bool* ret) {
    *ret = true;
    ALOGD("setLimit: %s", enabled ? "enable" : "disable");
    if( OemNetd::enableIpRestore(enabled) != 0 )
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::listenUidDataActivity(int protocol, int uid, int label,
                                                        int timeout, bool listen, bool* ret) {
    *ret = true;
    ALOGD("setLimit: protocol=%d, uid=%d, label=%d, timeout=%d, listen=%s",
                     protocol, uid, label, timeout, listen ? "true" : "false");
    if( OemNetd::listenUidDataActivity(protocol, uid,
                                                        label, timeout, listen) != 0)
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::updateIface(const std::string& iface, bool* ret) {
    *ret = true;
    ALOGD("updateIface: iface=%s", iface.c_str());
    if( OemNetd::updateIface(iface) != 0 )
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::addMiuiFirewallSharedUid(int uid, bool* ret) {
    *ret = true;
    ALOGD("addMiuiFirewallSharedUid: uid=%d", uid);
    if( OemNetd::addMiuiFirewallSharedUid(uid) != 0)
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::setMiuiFirewallRule(const std::string& packageName, int uid,
                                                     int rule, int type, bool* ret) {
    *ret = true;
    ALOGD("setMiuiFirewallRule: packageName=%s, uid=%d, rule=%d, type=%d",
                                packageName.c_str(), uid, rule, type);
    if( OemNetd::updateFirewallForPackage(packageName, uid, rule, type) !=0 )
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::setCurrentNetworkState(int state, bool* ret) {
    *ret = true;
    ALOGD("setCurrentNetworkState: state=%d", state);
    if( OemNetd::setDefaultNetworkType(state) !=0 )
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::enableRps(const std::string& iface, bool enable, bool* ret) {
    int res;
    ALOGD("enableRps: iface=%s, %s", iface.c_str(), enable ? "enable" : "disable");

    if(enable)
        res = OemNetd::enableRps(iface);
    else
        res = OemNetd::disableRps(iface);

    *ret = res == 0 ? true : false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::notifyFirewallBlocked(int code, const std::string& packageName) {
    ALOGD("notifyFirewallBlocked: code=%d, pkg=%s", code, packageName.c_str());
    const auto listenerMap = getOemNetdUnsolicitedEventListenerMap();
    for (auto& listener : listenerMap) {
       listener.first->onFirewallBlocked(code, packageName);
    }
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::enableQos(bool enabled, bool* ret) {
    *ret = true;
    ALOGD("enableQos: %s", enabled ? "enable" : "disable");
    if (enabled)	 {
        if (OemNetd::enable() != 0)
            *ret = false;
    } else {
        if (OemNetd::disable() != 0)
            *ret = false;
    }
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::setQos(int protocol, int uid, int tos, bool add, bool* ret) {
    *ret = true;
    ALOGD("setQos: uid=%d, tos=%d, add=%s", uid, tos, add ? "enable" : "disable");
    if ( OemNetd::setUidQos(protocol, uid, tos, add) !=0)
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::setPidForPackage(const ::std::string& packageName, int pid, int uid) {
    ALOGD("setPidForPackage: packageName=%s, pid=%d, pid=%d", packageName.c_str(), pid, uid);
    OemNetd::setPidForPackage(packageName, pid, uid);
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::enableMobileTrafficLimit(bool enabled, const std::string& iface, bool* ret) {
    *ret = true;
    ALOGD("enableMobileTrafficLimit: iface=%s, %s", iface.c_str(), enabled ? "enable" : "disable");
    if( OemNetd::enableMobileTrafficLimit(enabled, iface) != 0 )
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::setMobileTrafficLimit(bool enabled, long rate, bool* ret) {
    *ret = true;
    ALOGD("setMobileTrafficLimit: %s, rate=%ld ", enabled ? "enable" : "disable", rate);
    if( OemNetd::setMobileTrafficLimit(enabled, rate) != 0 )
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::whiteListUidForMobileTraffic(int uid, bool add, bool* ret) {
    *ret = true;
    ALOGD("whiteListUidForMobileTraffic: uid=%d, %s", uid, add ? "add" : "del");
    if( OemNetd::whiteListUidForMobileTraffic(uid, add) != 0 )
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::setMiuiSlmBpfUid(int uid, bool* ret) {
    *ret = true;
    ALOGD("setMiuiSlmBpfUid: uid=%d", uid);
    if (OemNetd::setMiuiSlmBpfUid(uid) != true)
        *ret = false;
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::getMiuiSlmVoipUdpAddress(int uid, int64_t* aidl_return) {
    ALOGD("getMiuiSlmVoipUdpAddress: uid=%d", uid);
    if (aidl_return != nullptr) {
        if ((*aidl_return = OemNetd::getMiuiSlmVoipUdpAddress(uid)) < 0)
            *aidl_return = 0;
    }
    return ::android::binder::Status::ok();
}

::android::binder::Status OemNetdListener::getMiuiSlmVoipUdpPort(int uid, int32_t* aidl_return) {
    ALOGD("getMiuiSlmVoipUdpPort: uid=%d", uid);
    if (aidl_return != nullptr) {
        if ((*aidl_return = OemNetd::getMiuiSlmVoipUdpPort(uid)) < 0)
            *aidl_return = 0;
    }
    return ::android::binder::Status::ok();
}

// MIUI ADD: START
::android::binder::Status OemNetdListener::enableAutoForward(const std::string& addr, int fwmark,
                                                             bool enabled, int32_t* aidl_return) {
    if( OemNetd::enableAutoForward(addr, fwmark, enabled) == 0 ) {
        *aidl_return = 0;
    }
    return ::android::binder::Status::ok();
}
// MIUI ADD: END

OemNetdListener::OemUnsolListenerMap OemNetdListener::getOemNetdUnsolicitedEventListenerMap() {
    std::lock_guard lock(mOemUnsolicitedMutex);
    return mOemUnsolListenerMap;
}

::android::binder::Status OemNetdListener::registerOemUnsolicitedEventListener(
        const ::android::sp<IOemNetdUnsolicitedEventListener>& listener) {
    registerOemUnsolicitedEventListenerInternal(listener);
    listener->onRegistered();
    return ::android::binder::Status::ok();
}

void OemNetdListener::registerOemUnsolicitedEventListenerInternal(
        const ::android::sp<IOemNetdUnsolicitedEventListener>& listener) {
    std::lock_guard lock(mOemUnsolicitedMutex);

    // Create the death listener.
    class DeathRecipient : public ::android::IBinder::DeathRecipient {
      public:
        DeathRecipient(OemNetdListener* oemNetdListener,
                       ::android::sp<IOemNetdUnsolicitedEventListener> listener)
            : mOemNetdListener(oemNetdListener), mListener(std::move(listener)) {}
        ~DeathRecipient() override = default;
        void binderDied(const ::android::wp<::android::IBinder>& /* who */) override {
            mOemNetdListener->unregisterOemUnsolicitedEventListenerInternal(mListener);
        }

      private:
        OemNetdListener* mOemNetdListener;
        ::android::sp<IOemNetdUnsolicitedEventListener> mListener;
    };
    ::android::sp<::android::IBinder::DeathRecipient> deathRecipient =
            new DeathRecipient(this, listener);

    ::android::IInterface::asBinder(listener)->linkToDeath(deathRecipient);

    mOemUnsolListenerMap.insert({listener, deathRecipient});
}

void OemNetdListener::unregisterOemUnsolicitedEventListenerInternal(
        const ::android::sp<IOemNetdUnsolicitedEventListener>& listener) {
    std::lock_guard lock(mOemUnsolicitedMutex);
    mOemUnsolListenerMap.erase(listener);
}

}  // namespace net
}  // namespace internal
}  // namespace android
}  // namespace com
