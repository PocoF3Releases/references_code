//
// Created by mi on 21-3-11.
//
#include <iostream>
#include <dlfcn.h>
#include "OemNetd.h"

namespace com {
namespace android {
namespace internal {
namespace net {

void* OemNetd::LibOemNetdImpl = NULL;
IMiuiNetd* OemNetd::ImplInstance = NULL;
std::mutex OemNetd::StubLock;

IMiuiNetd* OemNetd::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!ImplInstance) {
        LibOemNetdImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibOemNetdImpl) {
            Create* create = (Create *)dlsym(LibOemNetdImpl, "create");
            ImplInstance = create();
        }
    }
    return ImplInstance;
}
void OemNetd::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibOemNetdImpl) {
        Destroy* destroy = (Destroy *)dlsym(LibOemNetdImpl, "destroy");
        destroy(ImplInstance);
        dlclose(LibOemNetdImpl);
        LibOemNetdImpl = NULL;
        ImplInstance = NULL;
    }
}
int OemNetd::enableWmmer(bool enabled) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->enableWmmer(enabled);
    }
    return ret;
}
int OemNetd::enableLimiter(bool enabled) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->enableLimiter(enabled);
    }
    return ret;
}
int OemNetd::updateWmm(int uid, int wmm) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->updateWmm(uid, wmm);
    }
    return ret;
}
int OemNetd::whiteListUid(int uid, bool add) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->whiteListUid(uid, add);
    }
    return ret;
}
int OemNetd::limit(bool enabled, long bw) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->limit(enabled, bw);
    }
    return ret;
}
int OemNetd::enableIpRestore(bool enabled) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->enableIpRestore(enabled);
    }
    return ret;
}
int OemNetd::listenUidDataActivity(int protocol, int uid,
                              int label, int timeout, bool listen) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->listenUidDataActivity(protocol, uid, label, timeout, listen);
    }
    return ret;
}
int OemNetd::updateIface(const std::string& iface) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->updateIface(iface);
    }
    return ret;
}
int OemNetd::addMiuiFirewallSharedUid(int uid) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->addMiuiFirewallSharedUid(uid);
    }
    return ret;
}
int OemNetd::updateFirewallForPackage(const std::string & packageName, int uid, int rule, int type) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->updateFirewallForPackage(packageName, uid, rule, type);
    }
    return ret;
}
int OemNetd::setDefaultNetworkType(int type) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->setDefaultNetworkType(type);
    }
    return ret;
}
int OemNetd::enableRps(const std::string & iface) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->enableRps(iface);
    }
    return ret;
}
int OemNetd::disableRps(const std::string & iface) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->disableRps(iface);
    }
    return ret;
}
int OemNetd::enable() {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->enable();
    }
    return ret;
}
int OemNetd::disable() {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->disable();
    }
    return ret;
}
int OemNetd::setUidQos(int protocol, int uid, int tos, bool add) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->setUidQos(protocol, uid, tos, add);
    }
    return ret;
}
void OemNetd::setPidForPackage(const std::string & packageName, int pid, int uid) {
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        Inetd->setPidForPackage(packageName, pid, uid);
    }
}
int OemNetd::enableMobileTrafficLimit(bool enabled, const std::string& iface) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->enableMobileTrafficLimit(enabled, iface);
    }
    return ret;
}
int OemNetd::setMobileTrafficLimit(bool enabled, long bw) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->setMobileTrafficLimit(enabled, bw);
    }
    return ret;
}
int OemNetd::whiteListUidForMobileTraffic(int uid, bool add) {
    int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->whiteListUidForMobileTraffic(uid, add);
    }
    return ret;
}
unsigned int OemNetd::getMiuiSlmVoipUdpPort(int uid) {
    unsigned int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->getMiuiSlmVoipUdpPort(uid);
    }
    return ret;
}
unsigned int OemNetd::getMiuiSlmVoipUdpAddress(int uid) {
    unsigned int ret = 0;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->getMiuiSlmVoipUdpAddress(uid);
    }
    return ret;
}
bool OemNetd::setMiuiSlmBpfUid(int uid) {
    bool ret = false;
    IMiuiNetd* Inetd = GetImplInstance();
    if (Inetd) {
        ret = Inetd->setMiuiSlmBpfUid(uid);
    }
    return ret;
}

int OemNetd::enableAutoForward(const std::string& addr, int fwmark, bool enabled){
       unsigned int ret = 0;
       IMiuiNetd* Inetd = GetImplInstance();
       if (Inetd) {
           ret = Inetd->enableAutoForward(addr, fwmark, enabled);
       }
       return ret;
}

int OemNetd::checkMiuiNetworkFirewall(SocketClient* client, int* socketFd, unsigned netId){
       unsigned int ret = 0;
       IMiuiNetd* Inetd = GetImplInstance();
       if (Inetd) {
           ret = Inetd->checkMiuiNetworkFirewall(client, socketFd, netId);
       }
       return ret;

}

}  // namespace net
}  // namespace internal
}  // namespace android
}  // namespace com
