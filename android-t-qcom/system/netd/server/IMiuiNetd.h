//
// Created by mi on 21-3-11.
//

#ifndef ANDROID_IOEMNETD_H
#define ANDROID_IOEMNETD_H
#include <string>
#include <sysutils/SocketClient.h>
namespace com {
namespace android {
namespace internal {
namespace net {
class IMiuiNetd {
public:
    virtual ~IMiuiNetd() {}
    virtual int enableWmmer(bool enabled);
    virtual int enableLimiter(bool enabled);
    virtual int updateWmm(int uid, int wmm);
    virtual int whiteListUid(int uid, bool add);
    virtual int limit(bool enabled, long bw);
    virtual int enableIpRestore(bool enabled);
    virtual int listenUidDataActivity(int protocol, int uid,
                              int label, int timeout, bool listen);
    virtual int updateIface(const std::string& iface);
    virtual int addMiuiFirewallSharedUid(int uid);
    virtual int updateFirewallForPackage(const std::string & packageName, int uid, int rule, int type);
    virtual int setDefaultNetworkType(int type);
    virtual int enableRps(const std::string & iface);
    virtual int disableRps(const std::string & iface);
    virtual int enable();
    virtual int disable();
    virtual int setUidQos(int protocol, int uid, int tos, bool add);
    virtual void setPidForPackage(const std::string & packageName, int pid, int uid);
    virtual int enableMobileTrafficLimit(bool enabled, const std::string& iface);
    virtual int setMobileTrafficLimit(bool enabled, long bw);
    virtual int whiteListUidForMobileTraffic(int uid, bool add);
    virtual unsigned int getMiuiSlmVoipUdpPort(int uid);
    virtual unsigned int getMiuiSlmVoipUdpAddress(int uid);
    virtual bool setMiuiSlmBpfUid(int uid);
    virtual int enableAutoForward(const std::string& addr, int fwmark, bool enabled);
    virtual int checkMiuiNetworkFirewall(SocketClient* client, int* socketFd, unsigned netId);
};
typedef IMiuiNetd* Create();
typedef void Destroy(IMiuiNetd *);
}  // namespace net
}  // namespace internal
}  // namespace android
}  // namespace com
#endif //ANDROID_IOEMNETD_H
