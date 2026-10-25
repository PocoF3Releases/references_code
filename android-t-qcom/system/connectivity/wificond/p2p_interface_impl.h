#ifndef WIFICOND_P2P_INTERFACE_IMPL_H_
#define WIFICOND_P2P_INTERFACE_IMPL_H_

#include <array>
#include <string>
#include <vector>

#include <android-base/macros.h>
#include <wifi_system/interface_tool.h>
#include <utils/StrongPointer.h>

#include "android/net/wifi/nl80211/IP2pInterface.h"
#include "wificond/net/mlme_event_handler.h"
#include "wificond/net/netlink_utils.h"
#include "wificond/scanning/scanner_impl.h"

namespace android {
namespace wificond {

class P2pInterfaceBinder;
class P2pInterfaceImpl;
class ScanUtils;

class P2pMlmeEventHandlerImpl : public MlmeEventHandler {
 public:
  P2pMlmeEventHandlerImpl(P2pInterfaceImpl* p2p_interface);
  ~P2pMlmeEventHandlerImpl() override;
  void OnConnect(std::unique_ptr<MlmeConnectEvent> event) override;
  void OnRoam(std::unique_ptr<MlmeRoamEvent> event) override;
  void OnAssociate(std::unique_ptr<MlmeAssociateEvent> event) override;
  void OnDisconnect(std::unique_ptr<MlmeDisconnectEvent> event) override;
  void OnDisassociate(std::unique_ptr<MlmeDisassociateEvent> event) override;
  void OnChSwitchNotify() override;

 private:
  P2pInterfaceImpl* p2p_interface_;
};

class P2pInterfaceImpl {
 public:
  P2pInterfaceImpl(const std::string& interface_name,
                   uint32_t interface_index,
                   android::wifi_system::InterfaceTool* if_tool,
                   NetlinkUtils* netlink_utils,
                   ScanUtils* scan_utils);
  virtual ~P2pInterfaceImpl();

  // Get a pointer to the binder representing this P2pInterfaceImpl.
  android::sp<android::net::wifi::nl80211::IP2pInterface> GetBinder() const;

  bool SignalPoll(std::vector<int32_t>* out_signal_poll_results);
  virtual bool IsAssociated() const;
  void Dump(std::stringstream* ss) const;

 private:
  bool RefreshAssociateFreq();

  const std::string interface_name_;
  const uint32_t interface_index_;
  android::wifi_system::InterfaceTool* const if_tool_;
  NetlinkUtils* const netlink_utils_;
  ScanUtils* const scan_utils_;
  const std::unique_ptr<P2pMlmeEventHandlerImpl> mlme_event_handler_;
  const android::sp<P2pInterfaceBinder> binder_;

  // Cached information for this connection.
  bool is_associated_;
  std::array<uint8_t, ETH_ALEN> bssid_;
  uint32_t associate_freq_;

  friend class P2pMlmeEventHandlerImpl;
  DISALLOW_COPY_AND_ASSIGN(P2pInterfaceImpl);
};

}  // namespace wificond
}  // namespace android

#endif  // WIFICOND_P2P_INTERFACE_IMPL_H_
