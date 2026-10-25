/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "traffic"

#include <set>
#include <string>
#include <sys/wait.h>
#include <stdlib.h>
#include <netinet/ip.h>

#include <cutils/log.h>
#include <logwrap/logwrap.h>
#include <cutils/properties.h>

#include "NetdConstants.h"
#include "NetdConstantsExtend.h"
#include "MiuiTrafficController.h"
#include "oem_iptables_hook.h"

namespace {

    int cleanChain(const char* chain, const char* table) {
        char *cmds;
        asprintf(&cmds, "*%s\n:%s -\nCOMMIT\n", table, chain);
        int rc = execIptablesRestoreFunction(V4V6, cmds);
        free(cmds);
        if (rc) {
            ALOGE("clean %s chain failed", chain);
        }
        return rc;
    }
    int createChain(const char* chain, const char* table) {
        int rc = cleanChain(chain, table);
        if (rc) {
            ALOGE("create %s chain failed", chain);
        }
        return rc;
    }
}


/*********** TrafficMonitor ******************************/
const char* TrafficWmmer::TABLE = "mangle";
const char* TrafficWmmer::LOCAL_WMM = "tc_wmm";
const char* TrafficWmmer::mWmmSkbClass[] = {NULL, "0:5", "0:6"};
const uint32_t TrafficWmmer::mWmmSkbMarkShift[] = {0, 20, 21};

TrafficWmmer::TrafficWmmer(const char* iface) :
    mIface(iface), mEnabled(false), mInited(false) {
    mUids.clear();

    char buf[23] = {0};
    snprintf(buf, 23, "0x%x", 1 << mWmmSkbMarkShift[WMM_AC_VI]);
    mWmmSkbMark[WMM_AC_VI] = buf;
    snprintf(buf, 23, "0x%x", 1 << mWmmSkbMarkShift[WMM_AC_VO]);
    mWmmSkbMark[WMM_AC_VO] = buf;

    snprintf(buf, 23, "0x%x/0x%x", 1 << mWmmSkbMarkShift[WMM_AC_VI], 1 << mWmmSkbMarkShift[WMM_AC_VI]);
    mWmmSkbMarkMask[WMM_AC_VI] = buf;
    snprintf(buf, 23, "0x%x/0x%x", 1 << mWmmSkbMarkShift[WMM_AC_VO], 1 << mWmmSkbMarkShift[WMM_AC_VO]);
    mWmmSkbMarkMask[WMM_AC_VO] = buf;

    int rc = createChain(LOCAL_WMM, TABLE);
    if (rc) {
        ALOGE("create %s chain failed.", LOCAL_WMM);
        return;
    }

    mInited = true;
    return;
}

void TrafficWmmer::updateIface(const char* iface) {
    mIface = iface;
}

int TrafficWmmer::attachChain(bool attached) {
    const char* opFlag = attached ? "-A" : "-D";
    char *cmds;
    asprintf(&cmds, "*%s\n%s OUTPUT -o %s -j %s\nCOMMIT\n", TABLE, opFlag, mIface.c_str(), LOCAL_WMM);
    int rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("%s OUTPUT chain failed", (attached ? "attach" : "detach"));
    }
    return rc;
}

int TrafficWmmer::attachRestoreMark(bool attached) {
    char buf[100] = {0};
    const char* opFlag = attached ? "-A" : "-D";
    snprintf(buf, 99, "0x%x", ~(1 << mWmmSkbMarkShift[WMM_AC_VO] | 1 << mWmmSkbMarkShift[WMM_AC_VI]));
    char *cmds;
    asprintf(&cmds, "*%s\n%s %s -j MARK --and-mark %s\nCOMMIT\n", TABLE, opFlag, LOCAL_WMM, buf);
    int rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("%s restore mark failed", (attached ? "attach" : "detach"));
    }
    return rc;
}

int TrafficWmmer::enable() {
    int rc = 0;

    if (!mInited || mEnabled) {
        ALOGE("traffic wmmer has been enabled or unInited.");
        return 0;
    }

    rc = listen(mIface.c_str(), 3 ,true);

    if (rc) {
        ALOGE("attach a classify class failed.");
        goto error_create_chain;
    }
    if (attachRestoreMark(true)) {
        ALOGE("attach restore mark failed.");
        goto error_create_chain;
    }
    rc = attachChain(true);
    if (rc) {
        ALOGE("enable %s chain failed.", LOCAL_WMM);
        goto error_create_chain;
    }

    mEnabled = true;
    return 0;

error_create_chain:
    cleanChain(LOCAL_WMM, TABLE);
    return rc;
}

int TrafficWmmer::disable() {
    int rc = 0;
    if (!ready() || !mEnabled) {
        ALOGE("traffic wmmer has been disabled.");
        return 0;
    }
    rc = attachChain(false);
    rc |= listen(mIface.c_str(), 3 ,false);
    rc |= cleanChain(LOCAL_WMM, TABLE);
    mUids.clear();
    mEnabled = false;
    return rc;
}

int TrafficWmmer::listen(const char* name, int timeout, bool add) {
    const char* opFlag = add ? "-A" : "-D";
    char *cmds;
    asprintf(&cmds, "*%s\n%s %s -m mark --mark %s -j CLASSIFY --set-class %s\nCOMMIT\n"
            , TABLE, opFlag, LOCAL_WMM, mWmmSkbMarkMask[WMM_AC_VI].c_str(), mWmmSkbClass[WMM_AC_VI]);
    int rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("attach a classify class failed %d", __LINE__);
    }

    asprintf(&cmds, "*%s\n%s %s -m mark --mark %s -j CLASSIFY --set-class %s\nCOMMIT\n"
            , TABLE, opFlag, LOCAL_WMM, mWmmSkbMarkMask[WMM_AC_VO].c_str(), mWmmSkbClass[WMM_AC_VO]);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("attach a classify class failed %d", __LINE__);
    }

    char value[23] = {0};
    snprintf(value, 23, "%d", timeout);
    char skb_mark_mask[23] = {0};
    snprintf(skb_mark_mask, 23, "0x%x/0x%x", 0x0, ((1 << mWmmSkbMarkShift[WMM_AC_VI]) | (1 << mWmmSkbMarkShift[WMM_AC_VO])));
    /*
     * NetlinkHandler accept integer labels only from Android Q
     */
    if (strncmp("wlan", name, 4) == 0)
        name = "1"; // Mached with ConnectivityManager.TYPE_WIFI
    asprintf(&cmds, "*%s\n%s %s -m mark ! --mark %s -j IDLETIMER --timeout %s --label %s --send_nl_msg\nCOMMIT\n"
            , TABLE, opFlag, LOCAL_WMM, skb_mark_mask, value, name);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("idletime opterion fialed.");
    }

    return rc;
}

uint32_t TrafficWmmer::getWmmLocked(uid_t uid) {
    std::map<uid_t, uint32_t>::iterator itr;
    itr = mUids.find(uid);
    if (itr != mUids.end()) {
        return itr->second;
    }
    return 0;
}

int TrafficWmmer::addUidChain(const char *uid, const char* skbMark, bool add) {
    const char* opFlag = add ? "-I" : "-D";
    char *cmds;
    asprintf(&cmds, "*%s\n%s %s -m owner --uid-owner %s -j MARK --or-mark %s\nCOMMIT\n", TABLE, opFlag, LOCAL_WMM, uid, skbMark);
    int rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("%s add uid failed uid = %s rc = %d", add ? "add" : "remove", uid, rc);
    }
    return rc;
}

int TrafficWmmer::update(const char* uid, uint32_t wmm) {
    int rc = 0;

    if (!ready() || wmm >= WMM_AC_MAX) {
        ALOGE("traffic monitor has been disabled or argment error.");
        return rc;
    }
    uid_t uuid = (uid_t)atol(uid);
    uint32_t pwmm = WMM_AC_BE;
    bool oldRemove = false;
    bool newAdd = false;
    {
        android::RWLock::AutoWLock _l(mRWLock);
        pwmm = getWmmLocked(uuid);
        if (pwmm != wmm) {
            if (pwmm == WMM_AC_BE && wmm != WMM_AC_BE) {
                // add uid wmm to wmm.
                newAdd = true;
                mUids[uuid]= wmm;
            } else if (pwmm != WMM_AC_BE && wmm == WMM_AC_BE) {
                // remove uid wmm.
                oldRemove = true;
                mUids.erase(uuid);
            } else {
                 // WMM changed.
                 oldRemove = true;
                 newAdd = true;
                 mUids[uuid]= wmm;
            }
         }
    }
    if (oldRemove)
        rc = addUidChain(uid, mWmmSkbMark[pwmm].c_str(), false);
    if (newAdd)
        rc |= addUidChain(uid, mWmmSkbMark[wmm].c_str(), true);
    return rc;
}

uint32_t TrafficWmmer::getMark(const uid_t uid) {
    if (!ready()) {
        return 0;
    }
    android::RWLock::AutoRLock lock(mRWLock);
    uint32_t shift = mWmmSkbMarkShift[getWmmLocked(uid)];
    if (shift > 0) {
        return 1 << shift;
    }
    return 0;
}

/*********** TrafficLimiter ******************************/
const char* TrafficLimiter::TABLE = "filter";
const char* TrafficLimiter::LOCAL_LIMITER = "tc_limiter";

TrafficLimiter::TrafficLimiter(const char* iface) :
    mIface(iface), mEnabled(false), mInited(false),
    mLimited(false), mLastLimit(0), mLastBurst(0) {
    int rc = createChain(LOCAL_LIMITER, TABLE);
    if (rc) {
        ALOGE("create %s chain fialed.", LOCAL_LIMITER);
        return;
    }
    mUids.clear();
    mInited = true;
    return;
}

void TrafficLimiter::updateIface(const char* iface) {
    mIface = iface;
}

int TrafficLimiter::tachLimiterChain(bool attached) {
    int rc = 0;
    const char* opFlag = attached ? "-A" : "-D";
    char *cmds;
    asprintf(&cmds, "*%s\n%s INPUT -i %s -j %s\nCOMMIT\n", TABLE, opFlag, mIface.c_str(), LOCAL_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
    }
    asprintf(&cmds, "*%s\n%s OUTPUT -o %s -j %s\nCOMMIT\n", TABLE, opFlag, mIface.c_str(), LOCAL_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
    }
    return rc;
}

int TrafficLimiter::enable() {
    int rc = 0;

    if (!mInited || mEnabled) {
        ALOGE("traffic limiter has been enabled or unInited.");
        return 0;
    }

    /**
     * iptables -t filter -A tc_limiter -m owner
                --uid-owner 0-9999 -j RETURN
     * iptables -t filter -A tc_limiter -p tcp !
                --tcp-flags SYN,RST,FIN NONE -j RETURN
     * iptables -t filter -A tc_limiter -m length
                --length 0:80 -p tcp --tcp-flags ACK ACK -j RETURN
     * iptables -t filter -A tc_limiter -p udp -m multiport
                --dports 53,5353,137,500,4500 -j RETURN
     * iptables -t filter -A tc_limiter -p icmp -j RETURN
     * iptables -t filter -A tc_limiter -p icmpv6 -j RETURN
     * iptables -t filter -A tc_limiter -p esp -j RETURN
     * iptables -t filter -A tc_limiter -m connbytes
                --connbytes 0:250 --connbytes-dir reply
                --connbytes-mode avgpkt -j RETURN
     */
    char *cmds;
    asprintf(&cmds, "*%s\n-A %s -m owner --uid-owner 0-9999 -j RETURN\nCOMMIT\n", TABLE, LOCAL_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
        goto error;
    }
    asprintf(&cmds, "*%s\n-A %s -p tcp ! --tcp-flags SYN,RST,FIN NONE -j RETURN\nCOMMIT\n", TABLE, LOCAL_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
        goto error;
    }
    asprintf(&cmds, "*%s\n-A %s -m length --length 0:80 -p tcp --tcp-flags ACK ACK -j RETURN\nCOMMIT\n", TABLE, LOCAL_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
        goto error;
    }
    // dns, mdns, nbns, vowifi isakmp packet
    asprintf(&cmds, "*%s\n-A %s -p udp -m multiport --dports 53,5353,137,500,4500 -j RETURN\nCOMMIT\n", TABLE, LOCAL_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
        goto error;
    }
    asprintf(&cmds, "*%s\n-A %s -p icmp -j RETURN\nCOMMIT\n", TABLE, LOCAL_LIMITER);
    rc = execIptablesRestoreFunction(V4, cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
        goto error;
    }
    asprintf(&cmds, "*%s\n-A %s -p icmpv6 -j RETURN\nCOMMIT\n", TABLE, LOCAL_LIMITER);
    rc = execIptablesRestoreFunction(V6, cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
        goto error;
    }
    // vowifi esp packet
    asprintf(&cmds, "*%s\n-A %s -p esp -j RETURN\nCOMMIT\n", TABLE, LOCAL_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
        goto error;
    }
    asprintf(&cmds, "*%s\n-A %s -m connbytes --connbytes 0:500 --connbytes-dir reply --connbytes-mode avgpkt -j RETURN\nCOMMIT\n", TABLE, LOCAL_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
    }
    tachLimiterChain(true);

    mEnabled = true;
    mLimited = false;
    mUids.clear();
    return 0;

error:
    cleanChain(LOCAL_LIMITER, TABLE);
    free(cmds);
    return rc;
}

int TrafficLimiter::disable() {
    mEnabled = false;
    mLimited = false;
    mLastLimit = mLastBurst = 0;
    mUids.clear();
    tachLimiterChain(false);
    return cleanChain(LOCAL_LIMITER, TABLE);
}

int TrafficLimiter::tachLimitChain(bool attach, long limit, long burst) {
    char climit[20] = {0};
    snprintf(climit, 19, "%ld/s", limit);
    char cburst[20] = {0};
    snprintf(cburst, 19, "%ld/s", burst);
    const char* opFlag = attach ? "-A" : "-D";
    char *cmds;
    asprintf(&cmds, "*%s\n%s %s -m hashlimit --hashlimit-above %s --hashlimit-burst %s --hashlimit-name %s -j DROP\nCOMMIT\n"
            , TABLE, opFlag, LOCAL_LIMITER, climit, cburst, mIface.c_str());
    int rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("%s, %d", __func__, __LINE__);
    }
    return rc;
}

int TrafficLimiter::tachDropChain(bool attach) {
    const char* opFlag = attach ? "-A" : "-D";
    char *cmds;
    asprintf(&cmds, "*%s\n%s %s -j DROP\nCOMMIT\n", TABLE, opFlag, LOCAL_LIMITER);
    int rc = execIptablesRestoreFunction(V4, cmds);
    free(cmds);
    if (rc) {
        ALOGE("%s ipv4 drop chain failed", (attach ? "attach" : "detach"));
    }

    asprintf(&cmds, "*%s\n%s %s -j DROP\nCOMMIT\n", TABLE, opFlag, LOCAL_LIMITER);
    rc = execIptablesRestoreFunction(V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("%s ipv6 drop chain failed", (attach ? "attach" : "detach"));
    }
    return rc;
}

int TrafficLimiter::limit(const bool enabled, const long bw) {
    int rc = 0;
    if (!ready() || !mEnabled) {
        return rc;
    }

    if (!mLimited && enabled) {
        if (bw <= 0) {
            tachDropChain(true);
        } else {
            long packetPerSec = bw / 1500;
            long burst = packetPerSec / 10;
            long limit = packetPerSec - burst;
            if (burst <= 0) {
                burst = 1;
            }
            if (limit <= 0) {
                limit = 1;
            }
            mLastLimit = limit;
            mLastBurst = burst;
            tachLimitChain(true, mLastLimit, mLastBurst);
        }
        mLimited = true;
    } else if (mLimited && !enabled) {
        if (mLastLimit == 0) {
            tachDropChain(false);
        } else {
            tachLimitChain(false, mLastLimit, mLastBurst);
        }
        mLastLimit = mLastBurst = 0;
        mLimited = false;
    }

    return rc;
}
int TrafficLimiter::addOrRemoveUid(const char* uid, bool add) {
    int rc = 0;

    if (!ready() || !mEnabled) {
        return rc;
    }
    android::RWLock::AutoWLock _l(mRWLock);
    uid_t uuid = (uid_t)atol(uid);
    bool hasUid = mUids.count(uuid);
    if (add && hasUid) {
        ALOGE("The uid %d exists already", uuid);
        return rc;
    } else if (!add && !hasUid) {
        ALOGE("No such uid %d", uuid);
        return rc;
    }
    const char* opFlag = add ? "-I" : "-D";
    char *cmds;
    asprintf(&cmds, "*%s\n%s %s -m owner --uid-owner %s -j RETURN\nCOMMIT\n", TABLE, opFlag, LOCAL_LIMITER, uid);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc != 0) {
        ALOGE("%s whitelist %s failed rc = %d.", (add ? "add" : "remove"), uid, rc);
    }
    if (add) {
        mUids.insert(uuid);
    } else {
        mUids.erase(uuid);
    }
    return rc;
}

/*****************   TrafficIdleTimer  *************************/
const char* TrafficIdleTimer::TABLE = "mangle";
const char* TrafficIdleTimer::LOCAL_IDLETIMER = "tc_idletimer";
TrafficIdleTimer::TrafficIdleTimer(const char* iface) :
        mIface(iface), mEnabled(false), mInited(false) {
    if (createChain(LOCAL_IDLETIMER, TABLE)) {
        ALOGE("create %s chain from %s table failed.", LOCAL_IDLETIMER, TABLE);
        return;
    }
    mInited = true;
}

void TrafficIdleTimer::updateIface(const char* iface) {
    mIface = iface;
}

int TrafficIdleTimer::attachChain(bool attached) {
    const char* opFlag = attached ? "-A" : "-D";
    char *cmds;
    asprintf(&cmds, "*%s\n%s INPUT -i %s -j %s\nCOMMIT\n", TABLE, opFlag, mIface.c_str(), LOCAL_IDLETIMER);
    int rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("%s INPUT chain failed.", (attached ? "attach" : "detach"));
        free(cmds);
        return rc;
    }
    asprintf(&cmds, "*%s\n%s OUTPUT -o %s -j %s\nCOMMIT\n", TABLE, opFlag, mIface.c_str(), LOCAL_IDLETIMER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("%s OUTPUT chain failed.", (attached ? "attach" : "detach"));
    }
    free(cmds);
    return rc;
}

int TrafficIdleTimer::enable() {
    if (!mInited || mEnabled) {
        ALOGE("traffic idle timer has been enabled or unInited.");
        return 0;
    }
    int rc = attachChain(true);
    if (rc) {
        ALOGE("enable traffic idle timer failed");
        cleanChain(LOCAL_IDLETIMER, TABLE);
        return rc;
    }
    mEnabled = true;
    return rc;
}

int TrafficIdleTimer::disable() {
    mEnabled = false;
    attachChain(false);
    return cleanChain(LOCAL_IDLETIMER, TABLE);
}

int TrafficIdleTimer::addOrRemoveIdleTimer(uint32_t protocol, const char* uid,
            int label, uint32_t timeout, bool add) {
    if (enable()) {
        ALOGE("traffic idle timer is not ready yet");
        return 0;
    }
    char timeoutStr[11]; //enough to store any 32-bit unsigned decimal
    snprintf(timeoutStr, sizeof(timeoutStr), "%u", timeout);
    const char* protoStr;
    if (protocol == IPPROTO_TCP) {
        protoStr = "tcp";
    } else if (protocol == IPPROTO_UDP) {
        protoStr = "udp";
    } else {
        protoStr = "all";
    }
    const char* opFlag = add ? "-A" : "-D";
    char *cmds;
    asprintf(&cmds, "*%s\n%s %s -p %s -m owner --uid-owner %s -j IDLETIMER --timeout %s --label %d --send_nl_msg\nCOMMIT\n"
                , TABLE, opFlag, LOCAL_IDLETIMER, protoStr, uid, timeoutStr, label);
    int rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("%s idle timer failed", (add ? "add" : "remove"));
    }
    if (!add) {
        disable();
    }
    return rc;
}

/*********** TrafficMobileLimiter ******************************/
const char* TrafficMobileLimiter::TABLE = "filter";
const char* TrafficMobileLimiter::LOCAL_MOBILE_LIMITER = "tc_mobile_limiter";

TrafficMobileLimiter::TrafficMobileLimiter() :
    mIface(""), mEnabled(false), mInited(false),
    mLimited(false), mLastLimit(0), mLastBurst(0) {
    int rc = createChain(LOCAL_MOBILE_LIMITER, TABLE);
    if (rc) {
        ALOGE("TrafficMobileLimiter create %s chain fialed.", LOCAL_MOBILE_LIMITER);
        return;
    }
    mUids.clear();
    mInited = true;
    return;
}

void TrafficMobileLimiter::updateIface(const char* iface) {
    mIface = iface;
}

int TrafficMobileLimiter::makeMobileLimitCommand(bool attached) {
    int rc = 0;
    const char* opFlag = attached ? "-A" : "-D";
    char *cmds;
    if (strcmp(mIface.c_str(), "") == 0) {
        ALOGE("makeMobileLimitCommand: mIface is empty %d", __LINE__);
       return rc;
    }
    //Only control output
    asprintf(&cmds, "*%s\n%s OUTPUT -o %s -j %s\nCOMMIT\n", TABLE, opFlag, mIface.c_str(), LOCAL_MOBILE_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
    }
    return rc;
}

int TrafficMobileLimiter::enable(const char* iface) {
    int rc = 0;
    if (!mInited || mEnabled) {
        ALOGE("mobile traffic limiter has been enabled or unInited.");
        return 0;
    }
    mIface = iface;
    ALOGE("enable: mobile mIface is  %s", mIface.c_str());

    /**
     * iptables -t filter -A tc_limiter -m owner
                --uid-owner 0-9999 -j RETURN
     * iptables -t filter -A tc_limiter -p tcp !
                --tcp-flags SYN,RST,FIN NONE -j RETURN
     * iptables -t filter -A tc_limiter -m length
                --length 0:64 -p tcp --tcp-flags ACK ACK -j RETURN
     * iptables -t filter -A tc_limiter -m connbytes
                --connbytes 0:250 --connbytes-dir reply
                --connbytes-mode avgpkt -j RETURN
     */
    char *cmds;
    asprintf(&cmds, "*%s\n-A %s -m owner --uid-owner 0-9999 -j RETURN\nCOMMIT\n", TABLE, LOCAL_MOBILE_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
        goto error;
    }
    asprintf(&cmds, "*%s\n-A %s -p tcp ! --tcp-flags SYN,RST,FIN NONE -j RETURN\nCOMMIT\n", TABLE, LOCAL_MOBILE_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
        goto error;
    }
    asprintf(&cmds, "*%s\n-A %s -m length --length 0:53 -p tcp --tcp-flags ACK ACK -j RETURN\nCOMMIT\n", TABLE, LOCAL_MOBILE_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
        goto error;
    }
    asprintf(&cmds, "*%s\n-A %s -m connbytes --connbytes 0:500 --connbytes-dir reply --connbytes-mode avgpkt -j RETURN\nCOMMIT\n", TABLE, LOCAL_MOBILE_LIMITER);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("iptables exec failed %d", __LINE__);
    }
    makeMobileLimitCommand(true);

    mEnabled = true;
    mLimited = false;
    mUids.clear();
    return 0;

error:
    cleanChain(LOCAL_MOBILE_LIMITER, TABLE);
    free(cmds);
    return rc;
}


int TrafficMobileLimiter::disable() {
    mEnabled = false;
    mLimited = false;
    mLastLimit = mLastBurst = 0;
    mUids.clear();
    makeMobileLimitCommand(false);
    return cleanChain(LOCAL_MOBILE_LIMITER, TABLE);
}

int TrafficMobileLimiter::addMobileLimitRule(bool attach, long limit, long burst) {
    int rc = 0;
    char climit[20] = {0};
    snprintf(climit, 19, "%ld/s", limit);
    char cburst[20] = {0};
    snprintf(cburst, 19, "%ld", burst);
    const char* opFlag = attach ? "-A" : "-D";
    char *cmds;
    if (strcmp(mIface.c_str(), "") == 0) {
        ALOGE("addMobileLimitRule: mIface is empty %d", __LINE__);
       return rc;
    }
    asprintf(&cmds, "*%s\n%s %s -m hashlimit --hashlimit-above %s --hashlimit-burst %s --hashlimit-name %s -j DROP\nCOMMIT\n"
            , TABLE, opFlag, LOCAL_MOBILE_LIMITER, climit, cburst, mIface.c_str());
    rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("%s, %d", __func__, __LINE__);
    }
    return rc;
}

int TrafficMobileLimiter::addMobileDropRule(bool attach) {
    const char* opFlag = attach ? "-A" : "-D";
    char *cmds;
    asprintf(&cmds, "*%s\n%s %s -j DROP\nCOMMIT\n", TABLE, opFlag, LOCAL_MOBILE_LIMITER);
    int rc = execIptablesRestoreFunction(V4, cmds);
    free(cmds);
    if (rc) {
        ALOGE("addMobileDropRule %s ipv4 drop chain failed", (attach ? "attach" : "detach"));
    }

    asprintf(&cmds, "*%s\n%s %s ! -p icmpv6 -j DROP\nCOMMIT\n", TABLE, opFlag, LOCAL_MOBILE_LIMITER);
    rc = execIptablesRestoreFunction(V6, cmds);
    free(cmds);
    if (rc) {
        ALOGE("addMobileDropRule %s ipv6 drop chain failed", (attach ? "attach" : "detach"));
    }
    return rc;
}

int TrafficMobileLimiter::limit(const bool enabled, const long bw) {
    int rc = 0;
    if (!ready() || !mEnabled || (strcmp(mIface.c_str(), "") == 0)) {
        return rc;
    }

    if (!mLimited && enabled) {
        if (bw <= 0) {
            addMobileDropRule(true);
        } else {
            long packetPerSec = bw / 1500;
            long burst = packetPerSec / 10;
            long limit = packetPerSec - burst;
            if (burst <= 0) {
                burst = 1;
            }
            if (limit <= 0) {
                limit = 1;
            }
            mLastLimit = limit;
            mLastBurst = burst;
            addMobileLimitRule(true, mLastLimit, mLastBurst);
        }
        mLimited = true;
    } else if (mLimited && !enabled) {
        if (mLastLimit == 0) {
            addMobileDropRule(false);
        } else {
            addMobileLimitRule(false, mLastLimit, mLastBurst);
        }
        mLastLimit = mLastBurst = 0;
        mLimited = false;
    }

    return rc;
}

int TrafficMobileLimiter::addOrRemoveUid(const char* uid, bool add) {
    int rc = 0;

    if (!ready() || !mEnabled) {
        return rc;
    }
    android::RWLock::AutoWLock _l(mRWLock);
    uid_t uuid = (uid_t)atol(uid);
    bool hasUid = mUids.count(uuid);
    ALOGE("%s TrafficMobileLimiter whitelist %s", (add ? "add" : "remove"), uid);
    if (add && hasUid) {
        ALOGE("The uid %d exists already", uuid);
        return rc;
    } else if (!add && !hasUid) {
        ALOGE("No such uid %d", uuid);
        return rc;
    }
    const char* opFlag = add ? "-I" : "-D";
    char *cmds;
    asprintf(&cmds, "*%s\n%s %s -m owner --uid-owner %s -j RETURN\nCOMMIT\n", TABLE, opFlag, LOCAL_MOBILE_LIMITER, uid);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    free(cmds);
    if (rc != 0) {
        ALOGE("%s TrafficMobileLimiter whitelist %s failed rc = %d.", (add ? "add" : "remove"), uid, rc);
    }
    if (add) {
        mUids.insert(uuid);
    } else {
        mUids.erase(uuid);
    }
    return rc;
}

/*****************   TrafficController  *************************/
TrafficController *TrafficController::sInstance = NULL;

TrafficController *TrafficController::get() {
    if (!sInstance) {
        sInstance = new TrafficController();
    }

    return sInstance;
}

TrafficController::TrafficController():
    mEnabled(false), tm(NULL), tl(NULL) {

    char value[100];
    property_get("wifi.interface", value, "wlan0");
    mIface = value;

    tm = new TrafficWmmer(mIface.c_str());
    tl = new TrafficLimiter(mIface.c_str());
    ti = new TrafficIdleTimer(mIface.c_str());
    tml = new TrafficMobileLimiter();
}

int TrafficController::enableWmmer(bool enabled) {
    int rc = 0;
    if (enabled)
        rc = tm->enable();
    else
        rc = tm->disable();
    if (rc) {
        ALOGE("traffic wmmer enable failed, rc = %d", rc);
    }
    return rc;
}

int TrafficController::enableLimiter(bool enabled) {
    int rc = 0;
    if (enabled) {
        rc |= tl->enable();
    } else {
        rc = tl->disable();
    }
    if (rc) {
        ALOGE("traffic limiter enable failed, rc = %d", rc);
    }
    return rc;
}

int TrafficController::updateWmm(const char* uid, uint32_t wmm) {
    return tm->update(uid, wmm);
}

int TrafficController::whiteListUid(const char* uid, bool add) {
    return tl->addOrRemoveUid(uid, add);
}

int TrafficController::limit(bool enabled, long bw) {
    return tl->limit(enabled, bw);
}

int TrafficController::enableIpRestore(bool enabled) {
    return enableIptablesRestore(enabled);
}

void TrafficController::markIfNeeded(uid_t uid, uint32_t &mark) {
    mark |= tm->getMark(uid);
}

int TrafficController::listenUidDataActivity(uint32_t protocol, const char* uid,
                                                    int label, uint32_t timeout, bool listen) {
    return ti->addOrRemoveIdleTimer(protocol, uid, label, timeout, listen);
}

int TrafficController::updateIface(const char* iface) {
    mIface = iface;
    tl->updateIface(iface);
    tm->updateIface(iface);
    ti->updateIface(iface);
    tml->updateIface(iface);
    return 0;
}

int TrafficController::enableMobileTrafficLimit(bool enabled, const char* iface) {
    int rc = 0;
    if (enabled) {
        rc |= tml->enable(iface);
    } else {
        rc = tml->disable();
    }
    if (rc) {
        ALOGE("mobile traffic limit enable failed, rc = %d", rc);
    }
    return rc;
}

int TrafficController::setMobileTrafficLimit(bool enabled, long bw) {
    return tml->limit(enabled, bw);
}

int TrafficController::whiteListUidForMobileTraffic(const char* uid, bool add) {
    return tml->addOrRemoveUid(uid, add);
}

int TrafficController::enableAutoForward(const std::string& addr, int fwmark, bool enabled) {
    int rc = 0;
    const char* opFlag = enabled ? "-A" : "-D";
    if (addr.empty()) {
        ALOGE("Address is null, skip...");
        return rc;
    }
    std::string::size_type pos = addr.rfind('.');
    if(pos == std::string::npos) {
        ALOGE("Illegal Ip detect...");
    }
    std::string destIpPoll = addr.substr(0, pos) + ".0/24";
    char *cmds;
    asprintf(&cmds, "*nat\n%s POSTROUTING -d %s -j SNAT --to-source %s\nCOMMIT\n",
                                                  opFlag, destIpPoll.c_str(), addr.c_str());
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("iptables cmd: %s execute failed %d", cmds, rc);
    }
    asprintf(&cmds, "*mangle\n%s OUTPUT -d %s -j MARK --set-mark %d\nCOMMIT\n",
                                                      opFlag, destIpPoll.c_str(), fwmark);
    rc = execIptablesRestoreFunction(V4V6, cmds);
    if (rc) {
        ALOGE("iptables cmd: %s execute failed %d", cmds, rc);
    }
    free(cmds);
    return rc;
}

