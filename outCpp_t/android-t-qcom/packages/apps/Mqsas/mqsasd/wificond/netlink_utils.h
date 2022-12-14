/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef WIFICOND_NET_NETLINK_UTILS_H_
#define WIFICOND_NET_NETLINK_UTILS_H_

#include <array>
#include <functional>
#include <string>
#include <vector>
#include <map>

#include <cutils/log.h>
#include <utils/Log.h>
#include <linux/if_ether.h>

#include <android-base/macros.h>
#include <android-base/unique_fd.h>

#include <linux/nl80211.h>
#include "wificond/nl80211_packet.h"

#define ETH_ALEN 6

using std::unique_ptr;
using std::vector;
using std::placeholders::_1;
using std::string;
using std::array;

namespace android {
namespace wificond {

struct MessageType {
   // This constructor is needed by map[key] operation.
  MessageType() {};
  explicit MessageType(uint16_t id) {
    family_id = id;
  };
  uint16_t family_id;
  // Multicast groups supported by the family.  The string and mapping to
  // a group id are extracted from the CTRL_CMD_NEWFAMILY message.
  std::map<std::string, uint32_t> groups;
};

struct StationInfo {
  StationInfo() = default;
  StationInfo(uint32_t station_tx_packets_,
              uint32_t station_tx_failed_,
              uint32_t station_tx_bitrate_,
              int8_t current_rssi_,
              uint32_t station_rx_bitrate_)
      : station_tx_packets(station_tx_packets_),
        station_tx_failed(station_tx_failed_),
        station_tx_bitrate(station_tx_bitrate_),
        current_rssi(current_rssi_),
        station_rx_bitrate(station_rx_bitrate_) {}
  // Number of successfully transmitted packets.
  int32_t station_tx_packets;
  // Number of tramsmission failures.
  int32_t station_tx_failed;
  // Transimission bit rate in 100kbit/s.
  uint32_t station_tx_bitrate;
  // Current signal strength.
  int8_t current_rssi;
  // Last Received unicast packet bit rate in 100kbit/s.
  uint32_t station_rx_bitrate;
  // There are many other counters/parameters included in station info.
  // We will add them once we find them useful.
};

// Provides NL80211 helper functions.
class NetlinkUtils {
    public:
    NetlinkUtils();
    ~NetlinkUtils();

    static NetlinkUtils* instance();
  // Get station info from kernel.
  // |*out_station_info]| is the struct of available station information.
  // Returns true on success.
    int GetStationInfo(const char *ifacename, const char* bssid_mac);
    bool Start();

    private:
    android::base::unique_fd sync_netlink_fd_;
    bool SendMessageInternal(const NL80211Packet& packet, int fd);
    bool SetupSocket(base::unique_fd* netlink_fd);
    bool SendMessageAndGetSingleResponse(const NL80211Packet& packet,
      unique_ptr<const NL80211Packet>* response);
    bool SendMessageAndGetResponses(const NL80211Packet& packet,
      vector<unique_ptr<const NL80211Packet>>* response);
    void ReceivePacketAndRunHandler(int fd);
    uint16_t GetFamilyId();
    bool DiscoverFamilyId();
    uint32_t GetSequenceNumber();
    // bool NetlinkUtils::creatClientInterface();
    void OnNewFamily(unique_ptr<const NL80211Packet> packet);
    std::map<uint32_t,
      std::function<void(std::unique_ptr<const NL80211Packet>)>> message_handlers_;
    std::map<std::string, MessageType> message_types_;
    uint32_t sequence_number_;
    uint32_t interface_index_;

};

}  // namespace wificond
}  // namespace android

#endif  // WIFICOND_NET_NETLINK_UTILS_H_
