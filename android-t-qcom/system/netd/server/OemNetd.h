//
// Created by mi on 21-3-11.
//

#ifndef ANDROID_OEMNETD_H
#define ANDROID_OEMNETD_H
#include "IMiuiNetd.h"
#include <mutex>

#define LIBPATH "/system_ext/lib64/liboemnetdimpl.so"
#include <sysutils/SocketClient.h>

namespace com {
namespace android {
namespace internal {
namespace net {
class OemNetd {
private:
    OemNetd() {}
    static void *LibOemNetdImpl;
    static IMiuiNetd *ImplInstance;
    static IMiuiNetd *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;
public:
    virtual ~OemNetd() {}
    static int enableWmmer(bool enabled);
    static int enableLimiter(bool enabled);
    static int updateWmm(int uid, int wmm);
    static int whiteListUid(int uid, bool add);
    static int limit(bool enabled, long bw);
    static int enableIpRestore(bool enabled);
    static int listenUidDataActivity(int protocol, int uid,
                              int label, int timeout, bool listen);
    static int updateIface(const std::string& iface);
    static int addMiuiFirewallSharedUid(int uid);
    static int updateFirewallForPackage(const std::string & packageName, int uid, int rule, int type);
    static int setDefaultNetworkType(int type);
    static int enableRps(const std::string & iface);
    static int disableRps(const std::string & iface);
    static int enable();
    static int disable();
    static int setUidQos(int protocol, int uid, int tos, bool add);
    static void setPidForPackage(const std::string & packageName, int pid, int uid);
    static int enableMobileTrafficLimit(bool enabled, const std::string& iface);
    static int setMobileTrafficLimit(bool enabled, long bw);
    static int whiteListUidForMobileTraffic(int uid, bool add);
    static unsigned int getMiuiSlmVoipUdpPort(int uid);
    static unsigned int getMiuiSlmVoipUdpAddress(int uid);
    static bool setMiuiSlmBpfUid(int uid);
    static int enableAutoForward(const std::string& addr, int fwmark, bool enabled);
    static int checkMiuiNetworkFirewall(SocketClient* client, int* socketFd, unsigned netId);
};
}  // namespace net
}  // namespace internal
}  // namespace android
}  // namespace com

#endif //ANDROID_OEMNETD_H
