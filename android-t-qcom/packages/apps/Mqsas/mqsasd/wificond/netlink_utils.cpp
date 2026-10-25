#define LOG_TAG "slm-netlink"
#include "wificond/netlink_utils.h"
#include "wificond/nl80211_attribute.h"


#include <poll.h>
#include <array>
#include <algorithm>
#include <bitset>
#include <map>
#include <string>
#include <vector>

#include <utils/Timers.h>
#include <net/if.h>


namespace android {
namespace wificond {

constexpr int kReceiveBufferSize = 8 * 1024;
constexpr uint32_t kBroadcastSequenceNumber = 0;
constexpr int kMaximumNetlinkMessageWaitMilliSeconds = 1000000;
uint8_t ReceiveBuffer[kReceiveBufferSize];

static NetlinkUtils *m_instance =NULL;

NetlinkUtils::NetlinkUtils()
    :sequence_number_(0) {}
NetlinkUtils::~NetlinkUtils() {}

NetlinkUtils* NetlinkUtils::instance() {
  if (m_instance == NULL)
    m_instance = new NetlinkUtils();
  return m_instance;
}

int NetlinkUtils::GetStationInfo(const char *ifacename, const char* bssid_mac) {
  Start();
  NL80211Packet get_station(
               GetFamilyId(),
              NL80211_CMD_GET_STATION,
              GetSequenceNumber(),
              getpid());
  interface_index_ = if_nametoindex(ifacename);

  string bssid_ = bssid_mac;
  if(bssid_.length() != 17) {
      return 0;
  }
  std::array<uint8_t, ETH_ALEN> mac_address_;
  for (size_t i = 0; i < bssid_.length(); i += 3) {
      mac_address_[i/3] = ::strtol( bssid_.substr( i, 2 ).c_str(), 0, 16 );
  }

  get_station.AddAttribute(NL80211Attr<uint32_t>(NL80211_ATTR_IFINDEX, interface_index_));
  get_station.AddAttribute(NL80211Attr<std::array<uint8_t, ETH_ALEN>>(NL80211_ATTR_MAC, mac_address_));
  std::unique_ptr<const NL80211Packet> response;
  if (!SendMessageAndGetSingleResponse(get_station, &response)) {
    ALOGE(" NL80211_CMD_GET_STATION failed");
    return 0;
  }
  if (response->GetCommand() != NL80211_CMD_NEW_STATION) {
    ALOGE("Wrong command in response to a get station request: %d", static_cast<int>(response->GetCommand()));
    return 0;
  }
  NL80211NestedAttr sta_info(0);
  if (!response->GetAttribute(NL80211_ATTR_STA_INFO, &sta_info)) {
    ALOGE("Failed to get NL80211_ATTR_STA_INFO");
    return 0;
  }

  int8_t current_rssi;
  if (!sta_info.GetAttributeValue(NL80211_STA_INFO_SIGNAL, &current_rssi)) {
    ALOGE("Failed to get NL80211_STA_INFO_SIGNAL");
    return 0;
  }
  return current_rssi;
}

uint32_t NetlinkUtils::GetSequenceNumber() {
  if (++sequence_number_ == kBroadcastSequenceNumber) {
    ++sequence_number_;
  }
  return sequence_number_;
}

uint16_t NetlinkUtils::GetFamilyId() {
  return message_types_[NL80211_GENL_NAME].family_id;
}

void NetlinkUtils::OnNewFamily(unique_ptr<const NL80211Packet> packet) {
  if (packet->GetMessageType() != GENL_ID_CTRL) {
    ALOGE("Wrong message type for new family message");
    return;
  }
  if (packet->GetCommand() != CTRL_CMD_NEWFAMILY) {
    ALOGE("Wrong command for new family message");
    return;
  }
  uint16_t family_id;
  if (!packet->GetAttributeValue(CTRL_ATTR_FAMILY_ID, &family_id)) {
    ALOGE("Failed to get family id");
    return;
  }
  string family_name;
  if (!packet->GetAttributeValue(CTRL_ATTR_FAMILY_NAME, &family_name)) {
    ALOGE("Failed to get family name");
    return;
  }
  if (family_name != NL80211_GENL_NAME) {
     ALOGE("Ignoring none nl80211 netlink families");
  }
  MessageType nl80211_type(family_id);
  message_types_[family_name] = nl80211_type;
  // Exract multicast groups.
  NL80211NestedAttr multicast_groups(0);
  if (packet->GetAttribute(CTRL_ATTR_MCAST_GROUPS, &multicast_groups)) {
    vector<NL80211NestedAttr> groups;
    if (!multicast_groups.GetListOfNestedAttributes(&groups)) {
      return;
    }
    for (auto& group : groups) {
      string group_name;
      uint32_t group_id = 0;
      if (!group.GetAttributeValue(CTRL_ATTR_MCAST_GRP_NAME, &group_name)) {
        ALOGE("Failed to get group name");
        continue;
      }
      if (!group.GetAttributeValue(CTRL_ATTR_MCAST_GRP_ID, &group_id)) {
        ALOGE("Failed to get group id");
        continue;
      }
      message_types_[family_name].groups[group_name] = group_id;
    }
  }
}

bool NetlinkUtils::DiscoverFamilyId() {
  NL80211Packet get_family_request(GENL_ID_CTRL,
                                   CTRL_CMD_GETFAMILY,
                                   GetSequenceNumber(),
                                   getpid());
  NL80211Attr<string> family_name(CTRL_ATTR_FAMILY_NAME, NL80211_GENL_NAME);
  get_family_request.AddAttribute(family_name);
  unique_ptr<const NL80211Packet> response;
  if (!SendMessageAndGetSingleResponse(get_family_request, &response)) {
    ALOGE("Failed to get NL80211 family info");
    return false;
  }
  OnNewFamily(std::move(response));
  if (message_types_.find(NL80211_GENL_NAME) == message_types_.end()) {
    ALOGE("Failed to get NL80211 family id");
    return false;
  }
  return true;
}

bool NetlinkUtils::Start() {
  bool setup_rt = SetupSocket(&sync_netlink_fd_);
  if (!setup_rt) {
    ALOGE("Failed to setup synchronous netlink socket");
    return false;
  }

  // Request family id for nl80211 messages.
  if (!DiscoverFamilyId()) {
    return false;
  }
  return true;
}

bool NetlinkUtils::SetupSocket(base::unique_fd* netlink_fd) {
  struct sockaddr_nl nladdr;

  memset(&nladdr, 0, sizeof(nladdr));
  nladdr.nl_family = AF_NETLINK;

  netlink_fd->reset(
  socket(PF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_GENERIC));
  if (netlink_fd->get() < 0) {
    ALOGE("Failed to create netlink socket");
    return false;
  }
  // Set maximum receive buffer size.
  // Datagram which is larger than this size will be discarded.
  if (setsockopt(netlink_fd->get(),
                 SOL_SOCKET,
                 SO_RCVBUFFORCE,
                 &kReceiveBufferSize,
                 sizeof(kReceiveBufferSize)) < 0) {
    ALOGE("Failed to set uevent socket SO_RCVBUFFORCE option");
    return false;
  }
  if (bind(netlink_fd->get(),
           reinterpret_cast<struct sockaddr*>(&nladdr),
           sizeof(nladdr)) < 0) {
    ALOGE("Failed to bind netlink socket");
    return false;
  }
  return true;
}

bool NetlinkUtils::SendMessageAndGetSingleResponse(
    const NL80211Packet& packet,
    std::unique_ptr<const NL80211Packet>* response) {

  std::vector<std::unique_ptr<const NL80211Packet>> response_vec;
  if (!SendMessageAndGetResponses(packet, &response_vec)) {
    return false;
  }
  *response = std::move(response_vec[0]);
  return true;
}

void AppendPacket(std::vector<unique_ptr<const NL80211Packet>>* vec,
                  std::unique_ptr<const NL80211Packet> packet) {
  vec->push_back(std::move(packet));
}

bool NetlinkUtils::SendMessageAndGetResponses(
    const NL80211Packet& packet,
    std::vector<std::unique_ptr<const NL80211Packet>>* response) {
  if (!SendMessageInternal(packet, sync_netlink_fd_.get())) {
    ALOGD(" send message %d ", sync_netlink_fd_.get());
    return false;
  }
  // Polling netlink socket, waiting for GetFamily reply.
  struct pollfd netlink_output;
  memset(&netlink_output, 0, sizeof(netlink_output));
  netlink_output.fd = sync_netlink_fd_.get();
  netlink_output.events = POLLIN;

  uint32_t sequence = packet.GetMessageSequence();

  int time_remaining = kMaximumNetlinkMessageWaitMilliSeconds;
  // Multipart messages may come with seperated datagrams, mac_address_ with a
  // NLMSG_DONE message.
  // ReceivePacketAndRunHandler() will remove the handler after receiving a
  // NLMSG_DONE message.
  message_handlers_[sequence] = std::bind(AppendPacket, response, _1);

  while (time_remaining > 0 &&
      message_handlers_.find(sequence) != message_handlers_.end()) {
    nsecs_t interval = systemTime(SYSTEM_TIME_MONOTONIC);
    int poll_return = poll(&netlink_output,
                           1,
                           time_remaining);

    if (poll_return == 0) {
      ALOGE("Failed to poll netlink fd: time out ");
      message_handlers_.erase(sequence);
      return false;
    } else if (poll_return == -1) {
      ALOGE("Failed to poll netlink fd");
      message_handlers_.erase(sequence);
      return false;
    }
    ReceivePacketAndRunHandler(sync_netlink_fd_.get());
    interval = systemTime(SYSTEM_TIME_MONOTONIC) - interval;
    time_remaining -= static_cast<int>(ns2ms(interval));
  }
  if (time_remaining <= 0) {
    ALOGE("Timeout waiting for netlink reply messages");
    message_handlers_.erase(sequence);
    return false;
  }
  return true;
}

void NetlinkUtils::ReceivePacketAndRunHandler(int fd) {
  ssize_t len = read(fd, ReceiveBuffer, kReceiveBufferSize);

  if (len == -1) {
    ALOGE("Failed to read packet from buffer");
    return;
  }
  if (len == 0) {
    return;
  }
  // There might be multiple message in one datagram payload.
  uint8_t* ptr = ReceiveBuffer;
  while (ptr < ReceiveBuffer + len) {
    // peek at the header.
    if (ptr + sizeof(nlmsghdr) > ReceiveBuffer + len) {
      ALOGE("payload is broken.");
      return;
    }
    const nlmsghdr* nl_header = reinterpret_cast<const nlmsghdr*>(ptr);
    unique_ptr<NL80211Packet> packet(
        new NL80211Packet(vector<uint8_t>(ptr, ptr + nl_header->nlmsg_len)));
    ptr += nl_header->nlmsg_len;
    if (!packet->IsValid()) {
      ALOGE("Receive invalid packet");
      return;
    }
    // Some document says message from kernel should have port id equal 0.
    // However in practice this is not always true so we don't check that.

    uint32_t sequence_number = packet->GetMessageSequence();

    // Handle multicasts.
    if (sequence_number == kBroadcastSequenceNumber) {
      // BroadcastHandler(std::move(packet));
      continue;
    }

    auto itr = message_handlers_.find(sequence_number);
    // There is no handler for this sequence number.
    if (itr == message_handlers_.end()) {
       ALOGE("No handler for message: %d" ,sequence_number);
      return;
    }
    // A multipart message is terminated by NLMSG_DONE.
    // In this case we don't need to run the handler.
    // NLMSG_NOOP means no operation, message must be discarded.
    uint32_t message_type =  packet->GetMessageType();
    if (message_type == NLMSG_DONE || message_type == NLMSG_NOOP) {
      message_handlers_.erase(itr);
      return;
    }
    if (message_type == NLMSG_OVERRUN) {
      ALOGD("Get message overrun notification");
      message_handlers_.erase(itr);
      return;
    }

    // In case we receive a NLMSG_ERROR message:
    // NLMSG_ERROR could be either an error or an ACK.
    // It is an ACK message only when error code field is set to 0.
    // An ACK could be return when we explicitly request that with NLM_F_ACK.
    // An ERROR could be received on NLM_F_ACK or other failure cases.
    // We should still run handler in this case, leaving it for the caller
    // to decide what to do with the packet.

    bool is_multi = packet->IsMulti();
    // Run the handler.
    itr->second(std::move(packet));
    // Remove handler after processing.
    if (!is_multi) {
      message_handlers_.erase(itr);
    }
  }
}

bool NetlinkUtils::SendMessageInternal(const NL80211Packet& packet, int fd) {
  const vector<uint8_t>& data = packet.GetConstData();
  ssize_t bytes_sent =
      TEMP_FAILURE_RETRY(send(fd, data.data(), data.size(), 0));
  if (bytes_sent == -1) {
    ALOGE(" Failed to send netlink message");
    return false;
  }
  return true;
}

} // namespace wificond
} // namespace android
