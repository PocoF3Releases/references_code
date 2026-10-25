//
// Created by mi on 21-3-15.
//
#include "OemNetdImpl.h"
#include <string>
#include <logwrap/logwrap.h>
#include "MiuiNetworkController.h"
#include "MiuiTrafficController.h"
#include "DiffServController.h"
#include <netinet/in.h>
#include "MiuiSlmBpfController.h"

namespace com {
namespace android {
namespace internal {
namespace net {
int OemNetdImpl::enableWmmer(bool enabled) {
    return TrafficController::get()->enableWmmer(enabled);
}
int OemNetdImpl::enableLimiter(bool enabled) {
    return TrafficController::get()->enableLimiter(enabled);
}
int OemNetdImpl::updateWmm(int uid, int wmm) {
    return TrafficController::get()->updateWmm(std::to_string(uid).c_str(), wmm);
}
int OemNetdImpl::whiteListUid(int uid, bool add) {
    return TrafficController::get()->whiteListUid(std::to_string(uid).c_str(), add);
}
int OemNetdImpl::limit(bool enabled, long bw) {
    return TrafficController::get()->limit(enabled, bw);
}
int OemNetdImpl::enableIpRestore(bool enabled) {
    return TrafficController::get()->enableIpRestore(enabled);
}
int OemNetdImpl::listenUidDataActivity(int protocol, int uid,
                              int label, int timeout, bool listen) {
    return TrafficController::get()->listenUidDataActivity(protocol, std::to_string(uid).c_str(),
                                                                       label, timeout, listen);
}
int OemNetdImpl::updateIface(const std::string& iface) {
    return TrafficController::get()->updateIface(iface.c_str());
}
int OemNetdImpl::addMiuiFirewallSharedUid(int uid) {
    return MiuiNetworkController::get()->addMiuiFirewallSharedUid(uid);
}
int OemNetdImpl::updateFirewallForPackage(const std::string & packageName, int uid, int rule, int type) {
    return MiuiNetworkController::get()->updateFirewallForPackage(packageName, uid, rule, type);
}
int OemNetdImpl::setDefaultNetworkType(int type) {
    return MiuiNetworkController::get()->setDefaultNetworkType(type);
}
int OemNetdImpl::enableRps(const std::string & iface) {
    return MiuiNetworkController::get()->enableRps(iface);
}
int OemNetdImpl::disableRps(const std::string & iface) {
    return MiuiNetworkController::get()->disableRps(iface);
}
int OemNetdImpl::enable() {
    return DiffServController::get()->enable();
}
int OemNetdImpl::disable() {
    return DiffServController::get()->disable();
}
int OemNetdImpl::setUidQos(int protocol, int uid, int tos, bool add) {
    return DiffServController::get()->setUidQos(protocol, std::to_string(uid).c_str(), tos, add);
}
void OemNetdImpl::setPidForPackage(const std::string & packageName, int pid, int uid) {
    MiuiNetworkController::get()->setPidForPackage(packageName, pid, uid);
}
int OemNetdImpl::enableMobileTrafficLimit(bool enabled, const std::string& iface) {
    return TrafficController::get()->enableMobileTrafficLimit(enabled, iface.c_str());
}
int OemNetdImpl::setMobileTrafficLimit(bool enabled, long bw) {
    return TrafficController::get()->setMobileTrafficLimit(enabled, bw);
}
int OemNetdImpl::whiteListUidForMobileTraffic(int uid, bool add) {
    return TrafficController::get()->whiteListUidForMobileTraffic(std::to_string(uid).c_str(), add);
}
unsigned int OemNetdImpl::getMiuiSlmVoipUdpPort(int uid) {
    return MiuiSlmBpfController::get()->getMiuiSlmVoipUdpPort(uid);
}
unsigned int OemNetdImpl::getMiuiSlmVoipUdpAddress(int uid) {
    return MiuiSlmBpfController::get()->getMiuiSlmVoipUdpAddress(uid);
}
bool OemNetdImpl::setMiuiSlmBpfUid(int uid) {
    return MiuiSlmBpfController::get()->setMiuiSlmBpfUid(uid);
}

int OemNetdImpl::checkMiuiNetworkFirewall(SocketClient* client, int* socketFd, unsigned netId){
    return MiuiNetworkController::get()->checkMiuiNetworkFirewall(client, socketFd, netId);
}

int OemNetdImpl::enableAutoForward(const std::string& addr, int fwmark, bool enabled) {
    return TrafficController::get()->enableAutoForward(addr, fwmark, enabled);
}

extern "C" IMiuiNetd* create() {
    return new OemNetdImpl;
}
extern "C" void destroy(IMiuiNetd* impl) {
    delete impl;
}
}  // namespace net
}  // namespace internal
}  // namespace android
}  // namespace com

