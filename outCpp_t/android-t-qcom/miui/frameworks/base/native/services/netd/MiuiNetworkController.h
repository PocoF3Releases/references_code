#ifndef MIUI_NETWORK_CONTROLLER_H_
#define MIUI_NETWORK_CONTROLLER_H_

#include <map>
#include <string>

#include "utils/RWLock.h"
#include "NetlinkManager.h"
#include "sysutils/SocketListener.h"
#include "utils/LruCache.h"

using android::net::NetlinkManager;

#include "com/android/internal/net/IOemNetd.h"
#include "android/net/INetd.h"

class MiuiNetworkController : public android::OnEntryRemoved<pid_t, std::string*> {
    public:
        static MiuiNetworkController *get();
        int checkMiuiNetworkFirewall(SocketClient* client, int* socketFd, unsigned netId);
        int setDefaultNetworkType(int type);
        int updateFirewallForPackage(const std::string & packageName, int uid, int rule, int type);
        int addMiuiFirewallSharedUid(int sharedUid);
        void setPidForPackage(const std::string & packageName, int pid, int uid);
        int enableRps(const std::string & iface);
        int disableRps(const std::string & iface);
        /**
        * Used as a callback when an entry is removed from the cache.
        */
        void operator()(pid_t& pid, std::string*& pkg) override;

    private:
        MiuiNetworkController();
        std::string getPackageNameByPid(pid_t pid);
        void notify(int code, const char *format, ...);
        std::string getCachePkg(pid_t pid, uid_t uid);
        void notifyBlock(const std::string & packageName);
        std::string findMapKeyByValue(std::map<std::string, uid_t> packageRulesMap, uid_t value);
        int stringToFile(const char* path, const char* file, const char *value);
        void notifyFirewallBlocked(int code, const std::string & packageName);

        static MiuiNetworkController *sInstance;
        android::net::NetlinkManager *mNm;
        android::LruCache<pid_t, std::string*> mCache;
        mutable android::RWLock mRWLock;
        mutable android::RWLock mPackageDataRWLock;
        std::map<std::string, uid_t> mPackageRulesForMobile;
        std::map<std::string, uid_t> mPackageRulesForWiFi;
        std::map<std::string, uid_t> mPackageRulesForMobileRoam;
        std::map<std::string, uid_t> mPackageRulesForAll;
        std::string mCurrentRejectPackage;
        std::map<int,int> mSharedUidMap;
        std::map<int, std::string> mPackageInfoMap;
        int mDefaultNetworkType;
        android::sp<com::android::internal::net::IOemNetd> mOemNetd;
        const static int BlockNotifyCode = 699;
        const static int BlockReturnCode = -13;
        const static int CacheMaxSize = 40;
        const static int USER_XSPACE = 999;

        const char* RPS_FILE = "rps_cpus";
        const char* RFS_FILE = "rps_flow_cnt";
};

#endif
