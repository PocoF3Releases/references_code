#define LOG_NDEBUG 0
#define LOG_TAG "miuiFirewall"

#include "MiuiNetworkController.h"

#include <cutils/log.h>
#include <selinux/selinux.h>
#include <binder/IServiceManager.h>
#include <private/android_filesystem_config.h>
#include <android-base/file.h>
#include <sstream>
#include <pthread.h>

constexpr const char *SYSTEM_SERVER_CONTEXT = "u:r:system_server:s0";

MiuiNetworkController *MiuiNetworkController::sInstance = NULL;
pthread_mutex_t mutex;

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

MiuiNetworkController *MiuiNetworkController::get() {
    if (!sInstance) {
        pthread_mutex_lock(&mutex);
        if (!sInstance) {
            sInstance = new MiuiNetworkController();
        }
        pthread_mutex_unlock(&mutex);
    }
    return sInstance;
}

MiuiNetworkController::MiuiNetworkController()
    : mNm(NetlinkManager::Instance())
    , mCache(CacheMaxSize)
    , mCurrentRejectPackage("")
    , mDefaultNetworkType(TYPE_OTHER)
    , mOemNetd(NULL)
{
    // add remove cache listener
    mCache.setOnEntryRemovedListener(this);
}

bool isSystemServer(SocketClient* client) {
    if (client->getUid() != AID_SYSTEM) {
        return false;
    }

    char *context;
    if (getpeercon(client->getSocket(), &context)) {
        return false;
    }

    // We can't use context_new and context_type_get as they're private to libselinux. So just do
    // a string match instead.
    bool ret = !strcmp(context, SYSTEM_SERVER_CONTEXT);
    freecon(context);

    return ret;
}

int MiuiNetworkController::checkMiuiNetworkFirewall(SocketClient* client,
        int* socketFd __unused, unsigned netId __unused) {

    if(isSystemServer(client)) {
        ALOGI("miui firewall isSystemServer deadlock");
        return RULE_ALLOWED;
    }
    //双开应用单独处理，分身空间应用按主应用处理
    uid_t uid ;
    if (client->getUid() / AID_USER == USER_XSPACE) {
        uid = client->getUid();
    }else{
        uid = client->getUid() % AID_USER;
    }

    auto it = mSharedUidMap.find(uid);
    if (it != mSharedUidMap.end()) {
        std::string packageName = getCachePkg(client->getPid(), uid);
        if (packageName.empty()) {
            return RULE_ALLOWED;
        }

        if (!packageName.compare("android")) {
            ALOGI("miui firewall always allow android");
            return RULE_ALLOWED;
        }

        android::RWLock::AutoRLock _l(mRWLock);

        // check all block packageName
        if (mPackageRulesForAll.size()) {
            auto it = mPackageRulesForAll.find(packageName);
            if (it != mPackageRulesForAll.end()) {
               return BlockReturnCode;
            }
        }

        if (mDefaultNetworkType == TYPE_MOBILE && mPackageRulesForMobile.size()) {
            auto it = mPackageRulesForMobile.find(packageName);
            if (it != mPackageRulesForMobile.end()) {
                notifyBlock(packageName);
                return BlockReturnCode;
            }
        } else if(mDefaultNetworkType == TYPE_WIFI && mPackageRulesForWiFi.size()) {
            auto it = mPackageRulesForWiFi.find(packageName);
            if (it != mPackageRulesForWiFi.end()) {
                notifyBlock(packageName);
                return BlockReturnCode;
            }
        } else if(mDefaultNetworkType == TYPE_MOBILE_ROAM && mPackageRulesForMobileRoam.size()) {
            auto it = mPackageRulesForMobileRoam.find(packageName);
            if (it != mPackageRulesForMobileRoam.end()) {
                return RULE_ALLOWED;
            } else {
                notifyBlock(packageName);
                return BlockReturnCode;
            }
        }
    } else {
        android::RWLock::AutoRLock _l(mRWLock);

        // check all block packageName
        std::string packageName = findMapKeyByValue(mPackageRulesForAll, uid);
        if(!packageName.empty()){
            return BlockReturnCode;
        }

        if (mDefaultNetworkType == TYPE_MOBILE) {
            packageName = findMapKeyByValue(mPackageRulesForMobile, uid);
            if(!packageName.empty()){
                notifyBlock(packageName);
                return BlockReturnCode;
            }
        }else if(mDefaultNetworkType == TYPE_WIFI) {
            packageName = findMapKeyByValue(mPackageRulesForWiFi, uid);
            if(!packageName.empty()){
                notifyBlock(packageName);
                return BlockReturnCode;
            }
        }else if(mDefaultNetworkType == TYPE_MOBILE_ROAM) {
            packageName = findMapKeyByValue(mPackageRulesForMobileRoam, uid);
            if(!packageName.empty()){
                return RULE_ALLOWED;
            }else{
                packageName = getCachePkg(client->getPid(), uid);
                notifyBlock(packageName);
                return BlockReturnCode;
            }
        }
    }
    return RULE_ALLOWED;
}

std::string MiuiNetworkController::findMapKeyByValue(std::map<std::string, uid_t> packageRulesMap, uid_t value) {
    if (packageRulesMap.size()) {
        std::map<std::string, uid_t>::iterator iter = packageRulesMap.begin();
        while(iter != packageRulesMap.end()) {
            if(iter->second == value) {
                return iter->first;
            }
            iter++;
        }
    }
    return "";
}

void MiuiNetworkController::notifyBlock(const std::string & packageName) {
    if(packageName.compare(mCurrentRejectPackage)) {
        mCurrentRejectPackage = packageName;
        ALOGI("App restrict %s", packageName.c_str());
        notifyFirewallBlocked(BlockNotifyCode, packageName);
    }
}

// add package name lru cache
std::string MiuiNetworkController::getCachePkg(pid_t pid, uid_t uid){
    android::RWLock::AutoWLock _l(mPackageDataRWLock);
    if(mPackageInfoMap.size() == 0) {
        ALOGI("mPackageInfoMap is empty! pid: %d , uid： %d", pid , uid);
        return "";
    }

    auto iter = mPackageInfoMap.find(pid);
    if (iter != mPackageInfoMap.end()) {
        return iter->second;
    }
    return "";
}

// remove cache callback
void MiuiNetworkController::operator()(pid_t& pid __unused, std::string*& pkg) {
    delete pkg;
}

void MiuiNetworkController::notifyFirewallBlocked(int code, const std::string& packageName) {
    if (mOemNetd == NULL) {
        android::sp<android::IBinder> binder =
            android::defaultServiceManager()->checkService(android::String16("netd"));
        android::sp<android::net::INetd> netd;
        if (binder != nullptr) {
            netd = android::interface_cast<android::net::INetd>(binder);
        }
        if (netd != NULL) {
            android::binder::Status status = netd->getOemNetd(&binder);
            if (binder != nullptr) {
                mOemNetd = android::interface_cast<com::android::internal::net::IOemNetd>(binder);
            }
        }
    }
    if (mOemNetd != NULL) {
        mOemNetd->notifyFirewallBlocked(code, packageName);
    } else {
        ALOGI("get oemNetd failed.");
    }
}

int MiuiNetworkController::setDefaultNetworkType(int type) {
    ALOGI("default network type: %d ", type);
    android::RWLock::AutoWLock _l(mRWLock);
    mDefaultNetworkType = type;
    mCurrentRejectPackage = "";
    return 0;
}

int MiuiNetworkController::addMiuiFirewallSharedUid(int sharedUid) {
    android::RWLock::AutoWLock _l(mRWLock);
    mSharedUidMap.insert(std::pair<int, int>(sharedUid, sharedUid));
    return 0;
}

int MiuiNetworkController::updateFirewallForPackage(const std::string & packageName, int uid, int rule, int type) {
    if(packageName.empty() || uid <= 0){
        return 0;
    }
    ALOGI("packageName : %s, rule: %d, uid: %d, type: %d", packageName.c_str(), rule, uid, type);
    android::RWLock::AutoWLock _l(mRWLock);
    if (type == TYPE_MOBILE) {
        if (rule == RULE_REJECT) {
            std::map<std::string, uid_t>::iterator iterMobile = mPackageRulesForMobile.find(packageName);
            if(iterMobile != mPackageRulesForMobile.end()) {
                mPackageRulesForMobile.erase(iterMobile);
            }
            mPackageRulesForMobile.insert(std::pair<std::string, int>(packageName, uid));
        } else {
            mPackageRulesForMobile.erase(packageName);
        }
        return 0;
    } else if (type == TYPE_WIFI) {
        if (rule == RULE_REJECT) {
            std::map<std::string, uid_t>::iterator iterWifi = mPackageRulesForWiFi.find(packageName);
            if(iterWifi != mPackageRulesForWiFi.end()) {
                mPackageRulesForWiFi.erase(iterWifi);
            }
            mPackageRulesForWiFi.insert(std::pair<std::string, int>(packageName, uid));
        } else {
            mPackageRulesForWiFi.erase(packageName);
        }
        return 0;
    } else if (type == TYPE_MOBILE_ROAM) {
        if (rule == RULE_ALLOWED) {
            std::map<std::string, uid_t>::iterator iterRoam = mPackageRulesForMobileRoam.find(packageName);
            if(iterRoam != mPackageRulesForMobileRoam.end()) {
                mPackageRulesForMobileRoam.erase(iterRoam);
            }
            mPackageRulesForMobileRoam.insert(std::pair<std::string, int>(packageName, uid));
        } else {
            mPackageRulesForMobileRoam.erase(packageName);
        }
        return 0;
    } else if (type == TYPE_ALL) {
        if (rule == RULE_REJECT) {
            std::map<std::string, uid_t>::iterator iterAll = mPackageRulesForAll.find(packageName);
            if(iterAll != mPackageRulesForAll.end()) {
                mPackageRulesForAll.erase(iterAll);
            }
            mPackageRulesForAll.insert(std::pair<std::string, int>(packageName, uid));
        } else {
            mPackageRulesForAll.erase(packageName);
        }
        return 0;
    }
    return 1;
}

void MiuiNetworkController::setPidForPackage(const std::string & packageName, int pid, int uid __unused) {
    android::RWLock::AutoWLock _l(mPackageDataRWLock);
    std::map<int, std::string>::iterator iter = mPackageInfoMap.find(pid);
    if(iter != mPackageInfoMap.end()) {
        mPackageInfoMap.erase(iter);
    }
    mPackageInfoMap.insert(std::pair<int, std::string>(pid, packageName));
}

int MiuiNetworkController::enableRps(const std::string & iface) {
    int ret = 0;
    ALOGI("enableRps: %s", iface.c_str());
    android::RWLock::AutoWLock _l(mRWLock);
    std::stringstream ss;
    for (int index = 0; index < 5; index++ ) {
        ss << "/sys/class/net/" << iface << "/queues/rx-";
        ss << index << "/";
        ret = stringToFile(ss.str().c_str(), RPS_FILE, "ff");
        ret = stringToFile(ss.str().c_str(),  RFS_FILE, "4096");
        ss.str("");
    }
    return ret;
}

int MiuiNetworkController::disableRps(const std::string & iface) {
    int ret = 0;
    ALOGI("disableRps: %s ", iface.c_str());
    android::RWLock::AutoWLock _l(mRWLock);
    std::stringstream ss;
    for (int index = 0; index < 5; index++ ) {
        ss << "/sys/class/net/" << iface << "/queues/rx-";
        ss << index << "/";
        ret = stringToFile(ss.str().c_str(), RPS_FILE, "00");
        ret = stringToFile(ss.str().c_str(), RFS_FILE, "00");
        ss.str("");
    }
    return ret;
}

int MiuiNetworkController::stringToFile(const char* path, const char* file, const char *value) {
    ALOGI("stringToFile path: %s, file: %s, value: %s", path, file, value);
    std::stringstream sspath;
    sspath << path << file;
    return android::base::WriteStringToFile(value, sspath.str().c_str()) ? 0 : 1;
}
