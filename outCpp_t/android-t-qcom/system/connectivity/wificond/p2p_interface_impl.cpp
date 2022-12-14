#include "wificond/p2p_interface_impl.h"

#include <vector>

#include <android-base/logging.h>
#include <utils/Timers.h>

#include "wificond/logging_utils.h"
#include "wificond/net/mlme_event.h"
#include "wificond/net/netlink_utils.h"
#include "wificond/p2p_interface_binder.h"
#include "wificond/scanning/scan_result.h"
#include "wificond/scanning/scan_utils.h"
#include "wificond/scanning/scanner_impl.h"

using android::net::wifi::nl80211::IP2pInterface;
using android::net::wifi::nl80211::NativeScanResult;
using android::sp;
using android::wifi_system::InterfaceTool;

using std::array;
using std::endl;
using std::string;
using std::unique_ptr;
using std::vector;

using namespace std::placeholders;

namespace android{
namespace wificond {

P2pMlmeEventHandlerImpl::P2pMlmeEventHandlerImpl(P2pInterfaceImpl* p2p_interface)
    : p2p_interface_(p2p_interface) {
}

P2pMlmeEventHandlerImpl::~P2pMlmeEventHandlerImpl() {
}

void P2pMlmeEventHandlerImpl::OnConnect(unique_ptr<MlmeConnectEvent> event) {
  if (!event->IsTimeout() && event->GetStatusCode() == 0) {
    p2p_interface_->is_associated_ = true;
    p2p_interface_->RefreshAssociateFreq();
    p2p_interface_->bssid_ = event->GetBSSID();
  } else {
    if (event->IsTimeout()) {
      LOG(INFO) << "Connect timeout";
    }
    p2p_interface_->is_associated_ = false;
    p2p_interface_->bssid_.fill(0);
  }
}

void P2pMlmeEventHandlerImpl::OnRoam(unique_ptr<MlmeRoamEvent> event) {
  p2p_interface_->is_associated_ = true;
  p2p_interface_->RefreshAssociateFreq();
  p2p_interface_->bssid_ = event->GetBSSID();
}

void P2pMlmeEventHandlerImpl::OnAssociate(unique_ptr<MlmeAssociateEvent> event) {
  if (!event->IsTimeout() && event->GetStatusCode() == 0) {
    p2p_interface_->is_associated_ = true;
    p2p_interface_->RefreshAssociateFreq();
    p2p_interface_->bssid_ = event->GetBSSID();
  } else {
    if (event->IsTimeout()) {
      LOG(INFO) << "Associate timeout";
    }
    p2p_interface_->is_associated_ = false;
    p2p_interface_->bssid_.fill(0);
  }
}

void P2pMlmeEventHandlerImpl::OnDisconnect(unique_ptr<MlmeDisconnectEvent> event) {
  p2p_interface_->is_associated_ = false;
  p2p_interface_->bssid_.fill(0);
}

void P2pMlmeEventHandlerImpl::OnDisassociate(unique_ptr<MlmeDisassociateEvent> event) {
  p2p_interface_->is_associated_ = false;
  p2p_interface_->bssid_.fill(0);
}

void P2pMlmeEventHandlerImpl::OnChSwitchNotify() {
  p2p_interface_->RefreshAssociateFreq();
}

P2pInterfaceImpl::P2pInterfaceImpl(const std::string& interface_name,
                                   uint32_t interface_index,
                                   InterfaceTool* if_tool,
                                   NetlinkUtils* netlink_utils,
                                   ScanUtils* scan_utils)
    : interface_name_(interface_name),
      interface_index_(interface_index),
      if_tool_(if_tool),
      netlink_utils_(netlink_utils),
      scan_utils_(scan_utils),
      mlme_event_handler_(new P2pMlmeEventHandlerImpl(this)),
      binder_(new P2pInterfaceBinder(this)),
      is_associated_(false),
      associate_freq_(0) {
  LOG(INFO) << "P2p interface constructor " << interface_name_;
  netlink_utils_->SubscribeMlmeEvent(
    interface_index_,
    mlme_event_handler_.get());
  if_tool_->SetUpState(interface_name_.c_str(), true);
}

sp<android::net::wifi::nl80211::IP2pInterface> P2pInterfaceImpl::GetBinder() const {
  return binder_;
}

P2pInterfaceImpl::~P2pInterfaceImpl() {
  LOG(INFO) << "P2p interface destructor " << interface_name_;
  binder_->NotifyImplDead();
  if_tool_->SetUpState(interface_name_.c_str(), false);
  netlink_utils_->UnsubscribeMlmeEvent(interface_index_);
}

void P2pInterfaceImpl::Dump(std::stringstream* ss) const {
  *ss << "-------- Dump of p2p interface with index: "
      << interface_index_ << " and name: " << interface_name_
      << "--------" << endl;
  *ss << "-------- Dump End --------"<< endl;
}

bool P2pInterfaceImpl::SignalPoll(vector<int32_t>* out_signal_poll_results) {
  LOG(DEBUG) << "begin to get station_info";
  if (!IsAssociated()) {
    LOG(INFO) << "Fail RSSI polling because wifi is not associated.";
    goto get_if;
  }

  StationInfo station_info;
  if (!netlink_utils_->GetStationInfo(interface_index_,
                                      bssid_,
                                      &station_info)) {
    LOG(INFO) << "Failed to get StationInfo";
    goto get_if;
  }
  out_signal_poll_results->push_back(
      static_cast<int32_t>(station_info.current_rssi));
  // Convert from 100kbit/s to Mbps.
  out_signal_poll_results->push_back(
      static_cast<int32_t>(station_info.station_tx_bitrate/10));
  // Convert from 100kbit/s to Mbps.
  out_signal_poll_results->push_back(
      static_cast<int32_t>(station_info.station_rx_bitrate/10));
  // Association frequency.
  out_signal_poll_results->push_back(
      static_cast<int32_t>(associate_freq_));
  // Association bandwidth.
  out_signal_poll_results->push_back(
      static_cast<int32_t>(station_info.station_bandwidth));

  return true;

get_if:
  InterfaceInfo interface_info;
  if (!netlink_utils_->GetInterfaceInfo(interface_index_,
                                      bssid_,
                                      &interface_info)) {
    LOG(INFO) << "Interface not found or not GO";
    return false;
  } else {
    out_signal_poll_results->push_back(
        static_cast<int32_t>(-1));
    // not need
    out_signal_poll_results->push_back(
        static_cast<int32_t>(-1));
    // not need
    out_signal_poll_results->push_back(
        static_cast<int32_t>(-1));
    // GO frequency.
    out_signal_poll_results->push_back(
        static_cast<int32_t>(interface_info.freq));
    // GO bandwidth.
    out_signal_poll_results->push_back(
        static_cast<int32_t>(interface_info.width));
    return true;
  }
}

bool P2pInterfaceImpl::RefreshAssociateFreq() {
  // wpa_supplicant fetches associate frequency using the latest scan result.
  // We should follow the same method here before we find a better solution.
  std::vector<NativeScanResult> scan_results;
  if (!scan_utils_->GetScanResult(interface_index_, &scan_results)) {
    return false;
  }
  for (auto& scan_result : scan_results) {
    if (scan_result.associated) {
      associate_freq_ = scan_result.frequency;
    }
  }
  return false;
}

bool P2pInterfaceImpl::IsAssociated() const {
    return is_associated_;
  }

}  // namespace wificond
}  // namespace android
