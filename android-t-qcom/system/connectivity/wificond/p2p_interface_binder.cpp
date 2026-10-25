#include "wificond/p2p_interface_binder.h"

#include <algorithm>
#include <vector>

#include <linux/if_ether.h>

#include <android-base/logging.h>

#include <binder/Status.h>

#include "wificond/p2p_interface_impl.h"

using android::binder::Status;
using android::net::wifi::nl80211::IWifiScannerImpl;
using std::vector;

namespace android{
namespace wificond{

P2pInterfaceBinder::P2pInterfaceBinder(P2pInterfaceImpl* impl)
    : impl_(impl) {
}

P2pInterfaceBinder::~P2pInterfaceBinder() {
}

Status P2pInterfaceBinder::signalPoll(
    vector<int32_t>* out_signal_poll_results) {
  if (impl_ == nullptr) {
    return  Status::ok();
  }
  impl_->SignalPoll(out_signal_poll_results);
  return Status::ok();
}

}  // namespace wificond
}  // namespace android
