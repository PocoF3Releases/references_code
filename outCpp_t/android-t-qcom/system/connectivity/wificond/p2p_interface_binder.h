#ifndef WIFICOND_P2P_INTERFACE_BINDER_H_
#define WIFICOND_P2P_INTERFACE_BINDER_H_

#include <android-base/macros.h>
#include <binder/Status.h>

#include "android/net/wifi/nl80211/BnP2pInterface.h"

namespace android {
namespace wificond {

class P2pInterfaceImpl;

class P2pInterfaceBinder : public android::net::wifi::nl80211::BnP2pInterface {
 public:
  explicit P2pInterfaceBinder(P2pInterfaceImpl* impl);
  ~P2pInterfaceBinder() override;

  // Called by |impl_| its destruction.
  // This informs the binder proxy that no future manipulations of |impl_|
  // by remote processes are possible.
  void NotifyImplDead() { impl_ = nullptr; }

  ::android::binder::Status signalPoll(
      std::vector<int32_t>* out_signal_poll_results) override;

 private:
  P2pInterfaceImpl* impl_;

  DISALLOW_COPY_AND_ASSIGN(P2pInterfaceBinder);
};

}  //namespace wificond
}  //namespace android

#endif  // WIFICOND_P2P_INTERFACE_BINDER_H_
