/*
 * Copyright (C) 2014 The Android Open Source Project
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

#define LOG_TAG "DnsProxyBlocker"

#include "DnsProxyBlocker.h"
#include "cutils/log.h"
#include <private/android_filesystem_config.h>

DnsProxyBlocker* DnsProxyBlocker::sInstance = NULL;

namespace {
    enum {
        TYPE_MOBILE,
        TYPE_WIFI,
        TYPE_MOBILE_ROAM,
        TYPE_ALL,
        TYPE_OTHER
    };

    enum {
        RULE_ALLOWED,
        RULE_REJECT
    };
}

void DnsProxyBlocker::make() {
    if (!sInstance) sInstance = new DnsProxyBlocker();
}

DnsProxyBlocker* DnsProxyBlocker::getInstance() {
    if (!sInstance) sInstance = new DnsProxyBlocker();
    return sInstance;
}
DnsProxyBlocker::~DnsProxyBlocker() {}

void DnsProxyBlocker::addAddress(const std::string& address) {
    std::lock_guard<std::mutex> lock(mAddressLock);
    if (!mAddresses.count(address)) {
        mAddresses.insert(address);
        ALOGD("add address(%s)", address.c_str());
    }
}

void DnsProxyBlocker::clearAddress() {
    std::lock_guard<std::mutex> lock(mAddressLock);
    mAddresses.clear();
    ALOGD("clear all addresses");
}

bool DnsProxyBlocker::isGetPhoneNumberAddress(const std::string& address) {
    std::lock_guard<std::mutex> lock(mAddressLock);
    if (mAddresses.count(address)) {
        return true;
    }
    return false;
}

bool DnsProxyBlocker::isDnsBlocked(uid_t uid) {
    if(isUidBlockedBySecFireWall(uid)) return true;
    return false;
}

// Intercept DNS Resolve
void DnsProxyBlocker::setDefaultNetworkType(int type) {
    ALOGI("default network type: %d ", type);
    std::lock_guard<std::mutex> lock(mLock);
    mDefaultNetworkType = type;
}

// Intercept DNS Resolve
void DnsProxyBlocker::updateFirewallForUid(int uid, int rule, int type) {
    if(uid <= 0){
        return;
    }
    ALOGI("rule: %d, uid: %d, type: %d", rule, uid, type);
    std::lock_guard<std::mutex> lock(mLock);
    if (type == TYPE_MOBILE) {
        if (rule == RULE_REJECT) {
            if(!mUidRulesForMobile.count(uid)) {
                mUidRulesForMobile.insert(uid);
            }
        } else {
            mUidRulesForMobile.erase(uid);
        }
    } else if (type == TYPE_WIFI) {
        if (rule == RULE_REJECT) {
            if(!mUidRulesForWiFi.count(uid)) {
                mUidRulesForWiFi.insert(uid);
            }
        } else {
            mUidRulesForWiFi.erase(uid);
        }
    } else if (type == TYPE_MOBILE_ROAM) {
        if (rule == RULE_ALLOWED) {
            if(!mUidRulesForMobileRoam.count(uid)) {
                mUidRulesForMobileRoam.insert(uid);
            }
        } else {
            mUidRulesForMobileRoam.erase(uid);
        }
    } else if (type == TYPE_ALL) {
        if (rule == RULE_REJECT) {
            if(!mUidRulesForAll.count(uid)) {
                mUidRulesForAll.insert(uid);
            }
        } else {
            mUidRulesForAll.erase(uid);
        }
    }
}

// Intercept DNS Resolve
bool DnsProxyBlocker::isUidBlockedBySecFireWall(uid_t uid) {
    //双开应用单独处理，分身空间应用按主应用处理
    if (uid / AID_USER != USER_XSPACE) {
        uid = uid % AID_USER;
    }
    if (uid <= 2000) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mLock);
    if (mUidRulesForAll.count(uid)
        || (mDefaultNetworkType == TYPE_MOBILE && mUidRulesForMobile.count(uid))
        || (mDefaultNetworkType == TYPE_WIFI && mUidRulesForWiFi.count(uid))
        || (mDefaultNetworkType == TYPE_MOBILE_ROAM && !mUidRulesForMobileRoam.count(uid))) {
        ALOGI("MIUILOG-BlockDNS for uid: %d, currentNetworkType: %d", uid, mDefaultNetworkType);
        return true;
    }
    return false;
}

void DnsProxyBlocker::dump(int fd) {
    dprintf(fd,"Dns Blocked mAddresses:\n  ");
    std::lock_guard<std::mutex> addrlock(mAddressLock);
    for (const std::string& address : mAddresses) {
        dprintf(fd, "%s\t", address.c_str());
    }
    dprintf(fd, "\n");
}
