#ifndef _TRAFFIC_CONTROLLER_H_
#define _TRAFFIC_CONTROLLER_H_

#include "utils/RWLock.h"

#include <set>
#include <map>
#include <string>

class TrafficWmmer {
    public:
        TrafficWmmer(const char* iface);
        int enable();
        int disable();
        void updateIface(const char* iface);
        bool ready() {return mInited && mEnabled;}
        int listen(const char* name, int timeout, bool add);
        int update(const char* uid, uint32_t wmm);
        uint32_t getMark(const uid_t uid);

    private:
        int attachChain(bool attached);
        uint32_t getWmmLocked(uid_t uid);
        int addUidChain(const char *uid, const char* skbMark, bool add);
        int attachRestoreMark(bool attached);

        enum {
            WMM_AC_BE, WMM_AC_VI, WMM_AC_VO, WMM_AC_MAX
        };

        static const char* TABLE;
        static const char* LOCAL_WMM;
        static const char* mWmmSkbClass[WMM_AC_MAX];
        static const uint32_t mWmmSkbMarkShift[WMM_AC_MAX];
        std::string mWmmSkbMark[WMM_AC_MAX];
        std::string mWmmSkbMarkMask[WMM_AC_MAX];
        std::string mIface;
        bool mEnabled, mInited;
        mutable android::RWLock mRWLock;
        std::map<uid_t, uint32_t> mUids;
};

class TrafficLimiter {
    public:
        TrafficLimiter(const char* iface);
        bool ready() {return mInited;}
        int enable();
        int disable();
        void updateIface(const char* iface);
        int limit(const bool enabled, const long bw);
        int addOrRemoveUid(const char* uid, bool add);

    private:
        int tachLimitChain(bool attached, long limit, long burst);
        int tachLimiterChain(bool attached);
        int tachDropChain(bool attched);

        static const char* TABLE;
        static const char* LOCAL_LIMITER;
        std::string mIface;
        mutable android::RWLock mRWLock;
        std::set<uid_t> mUids;
        bool mEnabled, mInited, mLimited;
        long mLastLimit, mLastBurst;
};

class TrafficIdleTimer {
    public:
        TrafficIdleTimer(const char* iface);
        int enable();
        int disable();
        void updateIface(const char* iface);
        int addOrRemoveIdleTimer(uint32_t protocol, const char* uid,
                int label, uint32_t timeout, bool add);
    private:
        int attachChain(bool attached);

        static const char* TABLE;
        static const char* LOCAL_IDLETIMER;
        std::string mIface;
        bool mEnabled, mInited;
};

class TrafficMobileLimiter {
    public:
        TrafficMobileLimiter();
        bool ready() {return mInited;}
        int enable(const char* iface);
        int disable();
        void updateIface(const char* iface);
        int limit(const bool enabled, const long bw);
        int addOrRemoveUid(const char* uid, bool add);

    private:
        int addMobileLimitRule(bool attached, long limit, long burst);
        int addMobileDropRule(bool attched);
        int makeMobileLimitCommand(bool attached);

        static const char* TABLE;
        static const char* LOCAL_MOBILE_LIMITER;
        std::string mIface;
        mutable android::RWLock mRWLock;
        std::set<uid_t> mUids;
        bool mEnabled, mInited, mLimited;
        long mLastLimit, mLastBurst;
};

/**
 * Control network traffic.
 */
class TrafficController {
    public:
        static TrafficController *get();
        bool ready();
        int enableWmmer(bool enabled);
        int updateWmm(const char* uid, uint32_t wmm);
        int enableLimiter(bool enabled);
        int whiteListUid(const char* uid, bool add);
        int limit(bool enabled, long bw);
        void markIfNeeded(uid_t uid, uint32_t &mark);
        int enableIpRestore(bool enabled);
        int listenUidDataActivity(uint32_t protocol, const char* uid,
                int label, uint32_t timeout, bool listen);
        int updateIface(const char* iface);
        int enableMobileTrafficLimit(bool enabled, const char* iface);
        int setMobileTrafficLimit(bool enabled, long bw);
        int whiteListUidForMobileTraffic(const char* uid, bool add);
        int enableAutoForward(const std::string& addr, int fwmark, bool enabled);

    private:
        TrafficController();
        static TrafficController *sInstance;
        bool mEnabled;
        std::string mIface;
        TrafficWmmer *tm;
        TrafficLimiter *tl;
        TrafficIdleTimer *ti;
        TrafficMobileLimiter *tml;
};
#endif
