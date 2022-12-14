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

#ifndef NETD_DNS_PROXY_BLOCKER_H
#define NETD_DNS_PROXY_BLOCKER_H

#include <sys/types.h>
#include <mutex>
#include <set>
class DnsProxyBlocker {
  public:
    virtual ~DnsProxyBlocker();
    static void make();
    static DnsProxyBlocker* getInstance();
    bool isGetPhoneNumberAddress(const std::string& address);
    void addAddress(const std::string& address);
    void clearAddress();
    bool isDnsBlocked(uid_t);
    void dump(int fd);
    void updateFirewallForUid(int uid, int rule, int type);
    void setDefaultNetworkType(int type);

  private:
    static DnsProxyBlocker* sInstance;
    std::set<std::string> mAddresses;
    std::mutex mAddressLock;
    bool isUidBlockedBySecFireWall(uid_t);
    std::mutex mLock;
    int mDefaultNetworkType;
    std::set<uid_t> mUidRulesForMobile;
    std::set<uid_t> mUidRulesForWiFi;
    std::set<uid_t> mUidRulesForMobileRoam;
    std::set<uid_t> mUidRulesForAll;
    const static int USER_XSPACE = 999;
};

#endif  // NETD_DNS_PROXY_BLOCKER_H
