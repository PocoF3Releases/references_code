/*
 * Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
*/
/******************************************************************************
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *      * Neither the name of The Linux Foundation nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#define LOG_TAG "btif_acm"

#include "btif_acm.h"
#include <base/bind.h>
#include <base/bind_helpers.h>
#include <base/logging.h>
#include <base/strings/stringprintf.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <future>
#include "bta_closure_api.h"
#include <hardware/bluetooth.h>
#include <hardware/bt_acm.h>
#include "btcore/include/device_features.h"
#include "device/include/controller.h"
#include "audio_hal_interface/a2dp_encoding.h"
#include "bt_common.h"
#include "bt_utils.h"
#include "bta/include/bta_api.h"
#include "btif/include/btif_a2dp_source.h"
#include "btif_common.h"
#include <base/callback.h>
#include "audio_a2dp_hw/include/audio_a2dp_hw.h"
#include "btif_av_co.h"
#include "btif_util.h"
#include "btu.h"
#include "common/state_machine.h"
#include "osi/include/allocator.h"
#include "osi/include/osi.h"
#include "osi/include/properties.h"
#include "btif/include/btif_bap_config.h"
#include "bta_bap_uclient_api.h"
#include "connected_iso_api.h"
#include "btif_bap_codec_utils.h"
#include "bta/include/bta_csip_api.h"
#include <base/threading/thread.h>
#include "osi/include/thread.h"
#include "btif/include/btif_storage.h"
#include <pthread.h>
#include "bta_api.h"
#include <hardware/bt_pacs_client.h>
#include <hardware/bt_bap_uclient.h>
#include "btif/include/btif_vmcp.h"
#include "btif/include/btif_acm_source.h"
#include "l2c_api.h"
#include "bt_types.h"
#include "btm_int.h"
#include "btm_api.h"
#include "btif_api.h"
#include <inttypes.h>

/*****************************************************************************
 *  Constants & Macros
 *****************************************************************************/
#define LE_AUDIO_MASK                      0x00000300
#define LE_AUDIO_NOT_AVAILABLE             0x00000100
#define LE_AUDIO_AVAILABLE_NOT_LICENSED    0x00000200  //LC3
#define LE_AUDIO_AVAILABLE_LICENSED        0x00000300  //LC3Q

#define QHS_SUPPORT_MASK                   0x00000C00
#define QHS_SUPPORT_NOT_AVAILABLE          0x00000400
#define QHS_SUPPORT_AVAILABLE              0x00000800

#define LE_AUDIO_CS_3_1ST_BYTE_INDEX       0x00
#define LE_AUDIO_CS_3_2ND_BYTE_INDEX       0x01
#define LE_AUDIO_CS_3_3RD_BYTE_INDEX       0x02
#define LE_AUDIO_CS_3_4TH_BYTE_INDEX       0x03
#define LE_AUDIO_CS_3_5TH_BYTE_INDEX       0x04
#define LE_AUDIO_CS_3_7TH_BYTE_INDEX       0x06
#define LE_AUDIO_CS_3_8TH_BYTE_INDEX       0x07

#define  RX_ONLY                           0x1
#define  TX_ONLY                           0x2
#define  TX_RX_BOTH                        0x3

#define DO_NOT_CHANGE                      0x0
#define MODE_HIGH_QUALITY                  0x1
#define MODE_LOW_LATENCY_0                 0x2
#define MODE_LOW_LATENCY_1                 0x3
#define MODE_LOW_LATENCY_2                 0x4

#define FRAME_DUR_7_5_MSEC                 7500
#define FRAME_DUR_10_MSEC                  10000

#define META_DATA_SIZE                     0x0A

static RawAddress active_bda = {};
static constexpr int kDefaultMaxConnectedAudioDevices = 5;
CodecConfig current_active_config;
CodecConfig current_active_gaming_rx_config;

static CodecConfig current_media_config;
static CodecConfig current_voice_config;
static CodecConfig current_recording_config;
static CodecConfig current_gaming_rx_config;

uint16_t current_active_profile_type = 0;
uint16_t old_active_profile_type = 0;
uint16_t active_context_type = 0;
uint16_t current_active_context_type = 0;

bool Is_FT_Change_Supported = false;
bool Is_BN_Variation_Supported = false;

bool is_lc3q_supported_on_dut = false;
bool is_aptx_supported_on_dut = false;

uint8_t dut_lc3_encoder_ver = 0;
uint8_t dut_lc3_decoder_ver = 0;
uint8_t dut_aptx_encoder_ver = 0;
uint8_t dut_aptx_decoder_ver = 0; // unused currently

std::map<uint32_t, CodecSampleRate> freq_to_sample_rate_map = {
   {  8000,  CodecSampleRate::CODEC_SAMPLE_RATE_8000  },
   { 16000,  CodecSampleRate::CODEC_SAMPLE_RATE_16000 },
   { 24000,  CodecSampleRate::CODEC_SAMPLE_RATE_24000 },
   { 32000,  CodecSampleRate::CODEC_SAMPLE_RATE_32000 },
   { 44100,  CodecSampleRate::CODEC_SAMPLE_RATE_44100 },
   { 48000,  CodecSampleRate::CODEC_SAMPLE_RATE_48000 },
   { 88200,  CodecSampleRate::CODEC_SAMPLE_RATE_88200 },
   { 96000,  CodecSampleRate::CODEC_SAMPLE_RATE_96000 }
};

using bluetooth::bap::pacs::PacsClientInterface;
using bluetooth::bap::pacs::PacsClientCallbacks;
using bluetooth::bap::pacs::ConnectionState;
using bluetooth::bap::pacs::CodecConfig;
using bluetooth::bap::ucast::UcastClientInterface;
using bluetooth::bap::ucast::UcastClientCallbacks;
using bluetooth::bap::ucast::UcastClient;
using bluetooth::bap::ucast::StreamState;
using bluetooth::bap::ucast::StreamConnect;
using bluetooth::bap::ucast::StreamType;

using bluetooth::bap::pacs::CodecIndex;
using bluetooth::bap::pacs::CodecPriority;
using bluetooth::bap::pacs::CodecSampleRate;
using bluetooth::bap::pacs::CodecBPS;
using bluetooth::bap::pacs::CodecChannelMode;
using bluetooth::bap::pacs::CodecFrameDuration;
using bluetooth::bap::ucast::CodecQosConfig;
using bluetooth::bap::ucast::StreamStateInfo;
using bluetooth::bap::ucast::StreamConfigInfo;
using bluetooth::bap::ucast::StreamReconfig;
using bluetooth::bap::ucast::CISConfig;
using bluetooth::bap::pacs::CodecDirection;
using bluetooth::bap::ucast::CONTENT_TYPE_MEDIA;
using bluetooth::bap::ucast::CONTENT_TYPE_CONVERSATIONAL;
using bluetooth::bap::ucast::CONTENT_TYPE_LIVE;
using bluetooth::bap::ucast::CONTENT_TYPE_UNSPECIFIED;
using bluetooth::bap::ucast::CONTENT_TYPE_INSTRUCTIONAL;
using bluetooth::bap::ucast::CONTENT_TYPE_NOTIFICATIONS;
using bluetooth::bap::ucast::CONTENT_TYPE_ALERT;
using bluetooth::bap::ucast::CONTENT_TYPE_MAN_MACHINE;
using bluetooth::bap::ucast::CONTENT_TYPE_EMERGENCY;
using bluetooth::bap::ucast::CONTENT_TYPE_RINGTONE;
using bluetooth::bap::ucast::CONTENT_TYPE_SOUND_EFFECTS;
using bluetooth::bap::ucast::CONTENT_TYPE_GAME;

using bluetooth::bap::ucast::ASE_DIRECTION_SRC;
using bluetooth::bap::ucast::ASE_DIRECTION_SINK;
using bluetooth::bap::ucast::ASCSConfig;
using bluetooth::bap::ucast::LE_2M_PHY;
using bluetooth::bap::ucast::LE_QHS_PHY;
using bluetooth::bap::ucast::AUDIO_LOC_RIGHT;
using bluetooth::bap::ucast::AUDIO_LOC_LEFT;

using base::Bind;
using base::Unretained;
using base::IgnoreResult;
using bluetooth::Uuid;

Uuid UUID_SERV_CLASS_WMCP = Uuid::FromString
                              ("2587db3c-ce70-4fc9-935f-777ab4188fd7");

extern void do_in_bta_thread(const base::Location& from_here,
                             const base::Closure& task);

static void btif_ft_change_event_cback(uint8_t evt_len, uint8_t *p_data);

bool reconfig_acm_initiator(const RawAddress& peer_address, int profileType, bool ui_req);

static void btif_acm_initiator_dispatch_sm_event(const RawAddress& peer_address,
                                                  btif_acm_sm_event_t event);

static void btif_acm_initiator_dispatch_stop_req(const RawAddress& peer_address,
                                                 bool wait_for_signal, bool ui_req);

static bt_status_t change_codec_config_acm_initiator_req(const RawAddress& peer_address,
                                                     char* msg, bool devUIReq);

bt_status_t set_active_acm_initiator_internal(const RawAddress& peer_address,
                                            uint16_t profileType, bool ui_req);

bt_status_t set_active_acm_initiator(const RawAddress& peer_address,
                                            uint16_t profileType);

static void btif_acm_check_and_cancel_in_call_tracker_timer_();

void btif_acm_update_lc3q_params(int64_t* cs3, tBTIF_ACM* p_acm_data);
extern void btif_acm_source_profile_on_suspended(tA2DP_CTRL_ACK status,
                                          uint16_t sub_profile);

uint16_t btif_acm_bap_to_acm_context(uint16_t bap_context);
uint16_t btif_acm_profile_to_context(uint16_t profile);
RawAddress btif_acm_get_active_dev() {
  return active_bda;
}

static bool SelectCodecQosConfig(const RawAddress& bd_addr, int profile_type,
                               int context_type, int direction, int config_type,
                               bool is_game_vbc, std::string requested_config);
std::mutex acm_session_wait_mutex_;
std::mutex acm_set_active_wait_mutex_;
std::condition_variable acm_session_wait_cv;
bool acm_session_wait;
extern bluetooth::bap::pacs::PacsClientInterface* btif_pacs_client_get_interface();
extern bool btif_acm_update_acm_context(uint16_t acm_context,
                                        uint16_t acm_profile,
                                        uint16_t curr_acm_profile,
                                        const RawAddress& peer_address);
extern uint16_t get_cur_profile_type();
/*****************************************************************************
 *  Local type definitions
 *****************************************************************************/

class BtifCsipEvent {
 public:
  BtifCsipEvent(uint32_t event, const void* p_data, size_t data_length);
  BtifCsipEvent(const BtifCsipEvent& other);
  BtifCsipEvent() = delete;
  ~BtifCsipEvent();
  BtifCsipEvent& operator=(const BtifCsipEvent& other);

  uint32_t Event() const { return event_; }
  void* Data() const { return data_; }
  size_t DataLength() const { return data_length_; }
  std::string ToString() const;
  static std::string EventName(uint32_t event);

 private:
  void DeepCopy(uint32_t event, const void* p_data, size_t data_length);
  void DeepFree();

  uint32_t event_;
  void* data_;
  size_t data_length_;
};

class BtifAcmEvent {
 public:
  BtifAcmEvent(uint32_t event, const void* p_data, size_t data_length);
  BtifAcmEvent(const BtifAcmEvent& other);
  BtifAcmEvent() = delete;
  ~BtifAcmEvent();
  BtifAcmEvent& operator=(const BtifAcmEvent& other);

  uint32_t Event() const { return event_; }
  void* Data() const { return data_; }
  size_t DataLength() const { return data_length_; }
  std::string ToString() const;
  static std::string EventName(uint32_t event);

 private:
  void DeepCopy(uint32_t event, const void* p_data, size_t data_length);
  void DeepFree();

  uint32_t event_;
  void* data_;
  size_t data_length_;
};

class BtifAcmPeer;

class BtifAcmStateMachine : public bluetooth::common::StateMachine {
 public:
  enum {
    kStateIdle,            // ACM state disconnected
    kStateOpening,         // ACM state connecting
    kStateOpened,          // ACM state connected
    kStateStarted,         // ACM state streaming
    kStateReconfiguring,   // ACM state reconfiguring
    kStateClosing,         // ACM state disconnecting
  };

  class StateIdle : public State {
   public:
    StateIdle(BtifAcmStateMachine& sm)
        : State(sm, kStateIdle), peer_(sm.Peer()) {}
    void OnEnter() override;
    void OnExit() override;
    bool ProcessEvent(uint32_t event, void* p_data) override;

   private:
    BtifAcmPeer& peer_;
  };

  class StateOpening : public State {
   public:
    StateOpening(BtifAcmStateMachine& sm)
        : State(sm, kStateOpening), peer_(sm.Peer()) {}
    void OnEnter() override;
    void OnExit() override;
    bool ProcessEvent(uint32_t event, void* p_data) override;

   private:
    BtifAcmPeer& peer_;
  };

  class StateOpened : public State {
   public:
    StateOpened(BtifAcmStateMachine& sm)
        : State(sm, kStateOpened), peer_(sm.Peer()) {}
    void OnEnter() override;
    void OnExit() override;
    bool ProcessEvent(uint32_t event, void* p_data) override;

   private:
    BtifAcmPeer& peer_;
  };

  class StateStarted : public State {
   public:
    StateStarted(BtifAcmStateMachine& sm)
        : State(sm, kStateStarted), peer_(sm.Peer()) {}
    void OnEnter() override;
    void OnExit() override;
    bool ProcessEvent(uint32_t event, void* p_data) override;

   private:
    BtifAcmPeer& peer_;
  };

  class StateReconfiguring : public State {
   public:
    StateReconfiguring(BtifAcmStateMachine& sm)
        : State(sm, kStateReconfiguring), peer_(sm.Peer()) {}
    void OnEnter() override;
    void OnExit() override;
    bool ProcessEvent(uint32_t event, void* p_data) override;

   private:
    BtifAcmPeer& peer_;
  };

  class StateClosing : public State {
   public:
    StateClosing(BtifAcmStateMachine& sm)
        : State(sm, kStateClosing), peer_(sm.Peer()) {}
    void OnEnter() override;
    void OnExit() override;
    bool ProcessEvent(uint32_t event, void* p_data) override;
    void CheckAndUpdateDisconnection();

   private:
    BtifAcmPeer& peer_;
  };

  BtifAcmStateMachine(BtifAcmPeer& btif_acm_peer) : peer_(btif_acm_peer) {
    state_idle_ = new StateIdle(*this);
    state_opening_ = new StateOpening(*this);
    state_opened_ = new StateOpened(*this);
    state_started_ = new StateStarted(*this);
    state_reconfiguring_ = new StateReconfiguring(*this);
    state_closing_ = new StateClosing(*this);

    AddState(state_idle_);
    AddState(state_opening_);
    AddState(state_opened_);
    AddState(state_started_);
    AddState(state_reconfiguring_);
    AddState(state_closing_);
    SetInitialState(state_idle_);
  }

  BtifAcmPeer& Peer() { return peer_; }

 private:
  BtifAcmPeer& peer_;
  StateIdle* state_idle_;
  StateOpening* state_opening_;
  StateOpened* state_opened_;
  StateStarted* state_started_;
  StateReconfiguring* state_reconfiguring_;
  StateClosing* state_closing_;
};

class BtifAcmPeer {
 public:
  enum {
    kFlagPendingLocalSuspend            = 0x001,
    kFlagPendingReconfigure             = 0x002,
    kFlagPendingStart                   = 0x004,
    kFlagPendingStop                    = 0x008,
    kFLagPendingStartAfterReconfig      = 0x010,
    kFLagPendingReconfigAfterStart      = 0x020,
    kFLagPendingStartAfterPeerReconfig  = 0x040,
    kFlagClearSuspendAfterPeerSuspend   = 0x080,
    kFlagCallAndMusicSync               = 0x100,
    kFlagMediaSubUsecaseSync            = 0x200,
    kFlagPendingSetActiveDev            = 0x400,
    kFlagPendingAlsAndMetadataSync      = 0x800
  };

  enum {
    kFlagAggresiveMode             = 0x01,
    kFlagRelaxedMode               = 0x02,
  };

  static constexpr uint64_t  kTimeoutLockReleaseMs = 5 * 1000;

  BtifAcmPeer(const RawAddress& peer_address, uint8_t peer_sep,
              uint8_t set_id, uint8_t cig_id, uint8_t cis_id);
  ~BtifAcmPeer();

  bt_status_t Init();
  void Cleanup();

  /**
   * Check whether the peer can be deleted.
   *
   * @return true if the pair can be deleted, otherwise false
   */
  bool CanBeDeleted() const;

  bool IsPeerActiveForMusic() const {
    return (SetId() == MusicActiveSetId());
  }
  bool IsPeerActiveForVoice() const {
    return (SetId() == VoiceActiveSetId());
  }

  bool IsAcceptor() const { return (peer_sep_ == ACM_TSEP_SNK); }

  const RawAddress& MusicActivePeerAddress() const;
  const RawAddress& VoiceActivePeerAddress() const;
  uint8_t MusicActiveSetId() const;
  uint8_t VoiceActiveSetId() const;

  const RawAddress& PeerAddress() const {
    return peer_address_;
  }

  void SetPeerPacsState(ConnectionState state) {
    pacs_state = state;
  }

  ConnectionState GetPeerPacsState() {
    return pacs_state;
  }

  void SetPeerSrcPacsData(std::vector<CodecConfig> src_pac_records) {
    src_records = src_pac_records;
  }

  std::vector<CodecConfig> GetPeerSrcPacsData() {
    return src_records;
  }

  void SetPeerSnkPacsData(std::vector<CodecConfig> sink_pac_records) {
    snk_records = sink_pac_records;
  }

  std::vector<CodecConfig> GetPeerSnkPacsData() {
    return snk_records;
  }

  void SetSrcSelectableCapability(std::vector<CodecConfig> source_selectable_capability) {
    src_selectable_capability = source_selectable_capability;
  }

  std::vector<CodecConfig> GetSrcSelectableCapability() {
    return src_selectable_capability;
  }

  void SetSnkSelectableCapability(std::vector<CodecConfig> sink_selectable_capability) {
    snk_selectable_capability = sink_selectable_capability;
  }

  std::vector<CodecConfig> GetSnkSelectableCapability() {
    return snk_selectable_capability;
  }

  void SetContextType(uint16_t contextType) {
    context_type_ = context_type_ | contextType;
  }

  uint16_t GetContextType() {
    return context_type_;
  }

  void ResetContextType(uint16_t contextType) {
    context_type_ &= ~contextType;
  }

  void SetProfileType(uint16_t profileType) {
    profile_type_ = profile_type_ | profileType;
  }

  uint16_t GetProfileType() {
    return profile_type_;
  }

  void SetPrefProfileType(uint16_t prefprofileType) {
    pref_profile_type_ = pref_profile_type_ | prefprofileType;
  }

  uint16_t GetPrefProfileType() {
    return pref_profile_type_;
  }

  void ResetPrefProfileType(uint16_t prefprofileType) {
    pref_profile_type_ &= ~prefprofileType;
  }

  void ResetProfileType(uint16_t profileType) {
    profile_type_ &= ~profileType;
  }

  void SetRcfgProfileType(uint16_t profileType) {
    rcfg_profile_type_ = profileType;
  }

  uint16_t GetRcfgProfileType() {
    return rcfg_profile_type_;
  }

  void SetActProfileType(uint16_t profileType) {
    act_profile_type_ = profileType;
  }

  uint16_t GetactProfileType() {
    return act_profile_type_;
  }

  void SetStreamContextType(uint16_t contextType) {
    stream_context_type_ = contextType;
  }

  uint16_t GetStreamContextType() {
    return stream_context_type_;
  }

  void SetStreamProfileType(uint16_t profileType) {
    stream_profile_type_ = profileType;
  }

  uint16_t GetStreamProfileType() {
    return stream_profile_type_;
  }

  void SetPeerVoiceRxState(StreamState state) {
    voice_rx_state = state;
  }

  StreamState GetPeerVoiceRxState() {
    return voice_rx_state;
  }

  void SetPeerVoiceTxState(StreamState state) {
    voice_tx_state = state;
  }

  StreamState GetPeerVoiceTxState() {
    return voice_tx_state;
  }

  void SetPeerMusicTxState(StreamState state) {
    music_tx_state = state;
  }

  StreamState GetPeerMusicTxState() {
    return music_tx_state;
  }

  void SetPeerMusicRxState(StreamState state) {
    music_rx_state = state;
  }

  StreamState GetPeerMusicRxState() {
    return music_rx_state;
  }

  bool IsConnectedForMusic() {
    return (music_tx_state == StreamState::CONNECTED ||
            music_tx_state == StreamState::STREAMING ||
            music_rx_state == StreamState::CONNECTED ||
            music_rx_state == StreamState::STREAMING);
  }

  bool IsConnectedForVoice() {
    return ((voice_tx_state == StreamState::CONNECTED &&
             voice_rx_state == StreamState::CONNECTED) ||
            (voice_tx_state == StreamState::STREAMING &&
             voice_rx_state == StreamState::STREAMING));
  }

  void SetPeerLatency(uint16_t peerLatency) {
    peer_latency_ = peerLatency;
  }

  uint16_t GetPeerLatency() {
    return peer_latency_;
  }

  void SetRcfgContextType(uint16_t context_type) {
    rcfg_context_type_ = context_type;
  }

  uint16_t GetRcfgContextType() {
    return rcfg_context_type_;
  }

  bool SetCodecQosConfig (const RawAddress& bd_addr, int profile_type,
                                 int context_type, int direction,
                                 bool is_both_tx_rx_support, std::string req_cfg) {
    bool is_codecQos_selected = false;
    DeviceType deviceType;
    btif_bap_get_device_type(bd_addr, &deviceType,
                             static_cast<CodecDirection>(direction));

    BTIF_TRACE_DEBUG("%s: deviceType: %d, is_both_tx_rx_support: %d,"
                     " context_type: %d", __func__, static_cast<DeviceType>(deviceType),
                       is_both_tx_rx_support, context_type);

    //Below property need set for TMAP PTS TCs of config B and C
    //so that it would fill single direction config to Tx or Rx.
    char pts_tmap_conf_B_and_C[PROPERTY_VALUE_MAX] = "false";
    property_get("persist.vendor.btstack.pts_tmap_conf_B_and_C",
                                      pts_tmap_conf_B_and_C, "false");
    LOG_DEBUG(LOG_TAG, "%s: pts_tmap_conf_B_and_C: %s",
                                 __PRETTY_FUNCTION__, pts_tmap_conf_B_and_C);

    if (!strncmp("true", pts_tmap_conf_B_and_C, 4) &&
        context_type == VOICE_CONTEXT) {
      uint16_t num_sink_ases, num_src_ases;
      btif_bap_get_params(bd_addr, nullptr, nullptr, &num_sink_ases,
                          &num_src_ases, nullptr, nullptr);

      BTIF_TRACE_DEBUG("%s: bd_addr: %s, num_sink_ases: %d,"
                       " num_src_ases: %d", __func__, bd_addr.ToString().c_str(),
                         num_sink_ases, num_src_ases);
      if (num_sink_ases && num_src_ases) {
        is_both_tx_rx_support = true;
      } else if((num_sink_ases && !num_src_ases) ||
                (num_src_ases && !num_sink_ases)) {
        is_both_tx_rx_support = false;
      }
    }

    if (deviceType == DeviceType::EARBUD ||
        deviceType == DeviceType::EARBUD_NO_CSIP) {
        is_codecQos_selected = SelectCodecQosConfig(bd_addr, profile_type,
                                                  context_type, direction,
                             EB_CONFIG, is_both_tx_rx_support, req_cfg);
    } else if (deviceType == DeviceType::HEADSET_SPLIT_STEREO) {
        is_codecQos_selected = SelectCodecQosConfig(bd_addr, profile_type,
                                                  context_type, direction,
                             STEREO_HS_CONFIG_1, is_both_tx_rx_support, req_cfg);
    } else if (deviceType == DeviceType::HEADSET_STEREO) {
        is_codecQos_selected = SelectCodecQosConfig(bd_addr, profile_type,
                                                  context_type, direction,
                             STEREO_HS_CONFIG_2, is_both_tx_rx_support, req_cfg);
    }

    BTIF_TRACE_DEBUG("%s: Peer %s , context type: %d, profile_type: %d,"
                     " direction: %d, is_codecQos_selected: %d",
                     __func__, bd_addr.ToString().c_str(), context_type,
                     profile_type, direction, is_codecQos_selected);
    return is_codecQos_selected;
  }

  void set_peer_media_codec_config(CodecConfig &codec_config) {
    peer_media_codec_config = codec_config;
  }

  CodecConfig get_peer_media_codec_config() {
    return peer_media_codec_config;
  }

  void set_peer_media_qos_config(QosConfig &qos_config) {
    peer_media_qos_config = qos_config;
  }

  QosConfig get_peer_media_qos_config() {
    return peer_media_qos_config;
  }

  void set_peer_media_codec_qos_config(CodecQosConfig &codec_qos_config) {
    peer_media_codec_qos_config = codec_qos_config;
  }

  CodecQosConfig get_peer_media_codec_qos_config() {
    return peer_media_codec_qos_config;
  }

  void set_peer_voice_rx_codec_config(CodecConfig &codec_config) {
    peer_voice_rx_codec_config = codec_config;
  }

  CodecConfig get_peer_voice_rx_codec_config() {
    return peer_voice_rx_codec_config;
  }

  void set_peer_voice_rx_qos_config(QosConfig &qos_config) {
    peer_voice_rx_qos_config = qos_config;
  }

  QosConfig get_peer_voice_rx_qos_config() {
    return peer_voice_rx_qos_config;
  }

  void set_peer_voice_rx_codec_qos_config(CodecQosConfig &codec_qos_config) {
    peer_voice_rx_codec_qos_config = codec_qos_config;
  }

  CodecQosConfig get_peer_voice_rx_codec_qos_config() {
    return peer_voice_rx_codec_qos_config;
  }

  void set_peer_voice_tx_codec_config(CodecConfig &codec_config) {
    peer_voice_tx_codec_config = codec_config;
  }

  CodecConfig get_peer_voice_tx_codec_config() {
    return peer_voice_tx_codec_config;
  }

  void set_peer_voice_tx_qos_config(QosConfig &qos_config) {
    peer_voice_tx_qos_config = qos_config;
  }

  QosConfig get_peer_voice_tx_qos_config() {
    return peer_voice_tx_qos_config;
  }

  void set_peer_voice_tx_codec_qos_config(CodecQosConfig &codec_qos_config) {
    peer_voice_tx_codec_qos_config = codec_qos_config;
  }

  CodecQosConfig get_peer_voice_tx_codec_qos_config() {
    return peer_voice_tx_codec_qos_config;
  }

  void set_peer_dev_requested_config(std::string req_config) {
    peer_dev_requested_config = req_config;
  }

  void reset_peer_dev_requested_config() {
    peer_dev_requested_config.clear();
  }

  std::string get_peer_dev_requested_config() {
    return peer_dev_requested_config;
  }

  void set_peer_audio_src_location(uint32_t audio_location) {
    audio_src_location_ = audio_location;
  }

  void set_peer_audio_sink_location(uint32_t audio_location) {
    audio_sink_location_ = audio_location;
  }

  uint32_t get_peer_audio_src_location() {
    return audio_src_location_;
  }

  uint32_t get_peer_audio_sink_location() {
    return audio_sink_location_;
  }

  uint8_t SetId() const { return set_id_; }
  uint8_t CigId() const { return cig_id_; }
  uint8_t CisId() const { return cis_id_; }

  BtifAcmStateMachine& StateMachine() { return state_machine_; }
  const BtifAcmStateMachine& StateMachine() const { return state_machine_; }

  bool IsConnected() const;
  bool IsStreaming() const;
  bool IsReconfinging() const;

  bool CheckConnUpdateMode(uint8_t mode) const {
    return (conn_mode_ == mode);
  }

  void SetConnUpdateMode(uint8_t mode) {
    if(conn_mode_ == mode) return;
    if(mode == kFlagAggresiveMode) {
      BTIF_TRACE_DEBUG("%s: push aggressive intervals", __func__);
      L2CA_UpdateBleConnParams(peer_address_, 16, 32, 0, 1000);
    } else if(mode == kFlagRelaxedMode) {
      BTIF_TRACE_DEBUG("%s: push relaxed intervals", __func__);
      L2CA_UpdateBleConnParams(peer_address_, 40, 56, 0, 1000);
    }
    conn_mode_ = mode;
  }

  void ClearConnUpdateMode() { conn_mode_ = 0; }

  bool CheckFlags(uint16_t flags_mask) const {
    return ((flags_ & flags_mask) != 0);
  }

  /**
   * Set only the flags as specified by the flags mask.
   *
   * @param flags_mask the flags to set
   */
  void SetFlags(uint16_t flags_mask) { flags_ |= flags_mask; }

  /**
   * Clear only the flags as specified by the flags mask.
   *
   * @param flags_mask the flags to clear
   */
  void ClearFlags(uint16_t flags_mask) { flags_ &= ~flags_mask; }

  /**
   * Clear all the flags.
   */
  void ClearAllFlags() { flags_ = 0; }

  /**
   * Get string for the flags set.
   */
  std::string FlagsToString() const;

  uint16_t peer_negotiated_pd_;
  uint16_t cis_established_transport_latency_;
  uint16_t cis_established_base_ft_;
  int8_t active_cig_id;
  std::vector<uint8_t> cis_handle_list;
 private:
  const RawAddress peer_address_;
  const uint8_t peer_sep_;// SEP type of peer device
  uint8_t set_id_, cig_id_, cis_id_;
  BtifAcmStateMachine state_machine_;
  uint16_t flags_;
  uint8_t conn_mode_;
  StreamState voice_rx_state, voice_tx_state, music_tx_state, music_rx_state;
  uint16_t peer_latency_;
  uint16_t context_type_ = 0;
  uint16_t profile_type_ = 0;
  uint16_t pref_profile_type_ = 0;
  uint16_t rcfg_profile_type_ = 0;
  uint16_t act_profile_type_ = 0;
  uint16_t stream_context_type_ = 0;
  uint16_t stream_profile_type_ = 0;
  uint16_t rcfg_context_type_ = 0;
  uint32_t audio_src_location_;
  uint32_t audio_sink_location_;
  std::string peer_dev_requested_config;
  ConnectionState pacs_state;
  std::vector<CodecConfig> snk_records;
  std::vector<CodecConfig> src_records;
  std::vector<CodecConfig> snk_selectable_capability;
  std::vector<CodecConfig> src_selectable_capability;
  CodecConfig peer_media_codec_config, peer_voice_rx_codec_config, peer_voice_tx_codec_config;
  QosConfig peer_media_qos_config, peer_voice_rx_qos_config, peer_voice_tx_qos_config;
  CodecQosConfig peer_media_codec_qos_config, peer_voice_rx_codec_qos_config, peer_voice_tx_codec_qos_config;
};

static void btif_acm_check_and_cancel_lock_release_timer(uint8_t setId);
bool btif_acm_request_csip_unlock(uint8_t setId);

void btif_acm_source_on_stopped();
void btif_acm_source_on_suspended();
void btif_acm_on_idle(void);
bool btif_acm_check_if_requested_devices_stopped();

void btif_acm_source_cleanup(void);

bt_status_t btif_acm_source_setup_codec();
uint16_t btif_acm_get_active_device_latency();

class BtifAcmInitiator {
 public:
  static constexpr uint8_t kCigIdMin = 0;
  static constexpr uint8_t kCigIdMax = BTA_ACM_NUM_CIGS;
  static constexpr uint8_t kPeerMinSetId = BTA_ACM_MIN_NUM_SETID;
  static constexpr uint8_t kPeerMaxSetId = BTA_ACM_MAX_NUM_SETID;

  enum {
    kFlagStatusUnknown = 0x0,
    kFlagStatusUnlocked = 0x1,
    kFlagStatusPendingLock = 0x2,
    kFlagStatusSubsetLocked = 0x4,
    kFlagStatusLocked = 0x8,
    kFlagStatusPendingUnlock = 0x10,
  };

  // acm group procedure timer
  static constexpr uint64_t kTimeoutAcmGroupProcedureMs = 10 * 1000;
  static constexpr uint64_t kTimeoutConnIntervalMs = 5 * 1000;
  static constexpr uint64_t kTimeoutTrackInCallIntervalMs = 4 * 1000;

  BtifAcmInitiator()
      : callbacks_(nullptr),
        enabled_(false),
        max_connected_peers_(kDefaultMaxConnectedAudioDevices),
        music_active_setid_(INVALID_SET_ID),
        voice_active_setid_(INVALID_SET_ID),
        csip_app_id_(0),
        is_csip_reg_(false),
        lock_flags_(0),
        music_set_lock_release_timer_(nullptr),
        voice_set_lock_release_timer_(nullptr),
        acm_group_procedure_timer_(nullptr),
        acm_conn_interval_timer_(nullptr),
        acm_in_call_tracker_timer_(nullptr){}
  ~BtifAcmInitiator();

  bt_status_t Init(
      btacm_initiator_callbacks_t* callbacks, int max_connected_audio_devices,
      const std::vector<CodecConfig>& codec_priorities);
  void Cleanup();
  bool IsSetIdle(uint8_t setId) const;

  btacm_initiator_callbacks_t* Callbacks() { return callbacks_; }
  bool Enabled() const { return enabled_; }

  BtifAcmPeer* FindPeer(const RawAddress& peer_address);
  uint8_t FindPeerSetId(const RawAddress& peer_address);
  uint8_t FindPeerBySetId(uint8_t set_id);
  uint8_t FindPeerCigId(uint8_t set_id);
  uint8_t FindPeerByCigId(uint8_t cig_id);
  uint8_t FindPeerByCisId(uint8_t cig_id, uint8_t cis_id);
  BtifAcmPeer* FindOrCreatePeer(const RawAddress& peer_address);
  BtifAcmPeer* FindMusicActivePeer();

  /**
   * Check whether a connection to a peer is allowed.
   * The check considers the maximum number of connected peers.
   *
   * @param peer_address the peer address to connect to
   * @return true if connection is allowed, otherwise false
   */
  bool AllowedToConnect(const RawAddress& peer_address) const;
  bool IsAcmIdle() const;

  bool IsOtherSetPeersIdle(const RawAddress& peer_address, uint8_t setId) const;

  alarm_t* MusicSetLockReleaseTimer() { return music_set_lock_release_timer_; }
  alarm_t* VoiceSetLockReleaseTimer() { return voice_set_lock_release_timer_; }
  alarm_t* AcmGroupProcedureTimer() { return acm_group_procedure_timer_; }
  alarm_t* AcmConnIntervalTimer() { return acm_conn_interval_timer_; }
  alarm_t* AcmInCallTrackerTimer() { return acm_in_call_tracker_timer_; }

  /**
   * Delete a peer.
   *
   * @param peer_address of the peer to be deleted
   * @return true on success, false on failure
   */
  bool DeletePeer(const RawAddress& peer_address);

  /**
   * Delete all peers that are in Idle state and can be deleted.
   */
  void DeleteIdlePeers();

  /**
   * Get the Music active peer.
   *
   * @return the music active peer
   */
  const RawAddress& MusicActivePeer() const { return music_active_peer_; }

  /**
   * Get the Voice active peer.
   *
   * @return the voice active peer
   */
  const RawAddress& VoiceActivePeer() const { return voice_active_peer_; }

  uint8_t MusicActiveCSetId() const { return music_active_setid_; }
  uint8_t VoiceActiveCSetId() const { return voice_active_setid_; }

  void SetCsipAppId(uint8_t csip_app_id) { csip_app_id_ = csip_app_id; }
  uint8_t GetCsipAppId() const { return csip_app_id_; }

  void SetCsipRegistration(bool is_csip_reg) { is_csip_reg_ = is_csip_reg; }
  bool IsCsipRegistered() const { return is_csip_reg_;}

  void SetMusicActiveGroupStarted(bool flag) { is_music_active_set_started_ = flag; }
  bool IsMusicActiveGroupStarted () { return is_music_active_set_started_; }

  bool IsConnUpdateEnabled() const {
    return (is_conn_update_enabled_ == true);
  }

  void SetOrUpdateGroupLockStatus(uint8_t set_id, int lock_status) {
    std::map<uint8_t, int>::iterator p = set_lock_status_.find(set_id);
    if (p == set_lock_status_.end()) {
      set_lock_status_.insert(std::make_pair(set_id, lock_status));
    } else {
      set_lock_status_.erase(set_id);
      set_lock_status_.insert(std::make_pair(set_id, lock_status));
    }
  }

  int GetGroupLockStatus(uint8_t set_id) {
    auto it = set_lock_status_.find(set_id);
    if (it != set_lock_status_.end()) return it->second;
    return kFlagStatusUnknown;
  }

  bool CheckLockFlags(uint8_t bitlockflags_mask) const {
    return ((lock_flags_ & bitlockflags_mask) != 0);
  }

    /**
     * Set only the flags as specified by the bitlockflags_mask.
     *
     * @param bitlockflags_mask the lock flags to set
     */
  void SetLockFlags(uint8_t bitlockflags_mask) { lock_flags_ |= bitlockflags_mask;}

    /**
     * Clear only the flags as specified by the bitlockflags_mask.
     *
     * @param bitlockflags_mask the lock flags to clear
     */
  void ClearLockFlags(uint8_t bitlockflags_mask) { lock_flags_ &= ~bitlockflags_mask;}

    /**
     * Clear all lock flags.
     */
  void ClearAllLockFlags() { lock_flags_ = 0;}

    /**
     * Get a string for lock flags.
     */
  std::string LockFlagsToString() const;

  bool IsActiveDevStreaming() {
    BtifAcmPeer* twm_peer = FindPeer(active_bda);
    if (twm_peer != nullptr &&
        twm_peer->CheckFlags(BtifAcmPeer::kFlagMediaSubUsecaseSync)) {
      BTIF_TRACE_DEBUG("%s: true", __func__);
      return true;
    } else {
      return false;
    }
  }

  void isPendingDeviceSwitch(const RawAddress& peer_address) {
    BtifAcmPeer *peer = FindPeer(peer_address);
    if (peer != nullptr) {
      if (peer->CheckFlags(BtifAcmPeer::kFlagPendingSetActiveDev)) {
        BTIF_TRACE_DEBUG("%s: Clearing Device switch flag",__func__);
        peer->ClearFlags(BtifAcmPeer::kFlagPendingSetActiveDev);
      } else {
        BTIF_TRACE_DEBUG("%s: Setting Device switch flag",__func__);
        peer->SetFlags(BtifAcmPeer::kFlagPendingSetActiveDev);
      }
    }
  }

  bool SetAcmActivePeer(const RawAddress& peer_address,
                        uint16_t contextType,
                        uint16_t profileType,
                        std::promise<void> peer_ready_promise, bool ui_req) {
    LOG(INFO) << __PRETTY_FUNCTION__ << ": peer: " << peer_address
              << ", music_active_peer_: " << music_active_peer_
              << ", voice_active_peer_: " << voice_active_peer_
              << ", music_active_setid_: " << loghex(music_active_setid_)
              << ", voice_active_setid_: " << loghex(voice_active_setid_)
              << ", ui_req: " << ui_req;

    BTIF_TRACE_DEBUG("%s: current_active_profile_type: %d , profileType: %d",
                                __func__, current_active_profile_type, profileType);

    uint16_t sink_latency;
    RawAddress prev_active_device = active_bda;
    bool pending_switch = false;
    bool was_prev_active_twm_device = ((prev_active_device.address[0] == 0x9E &&
                                        prev_active_device.address[1] == 0x8B &&
                                        prev_active_device.address[2] == 0x00) == false);
    // TO-DO: Have for non-twm also later
    if (was_prev_active_twm_device) {
      BtifAcmPeer* prev_active_peer = FindPeer(prev_active_device);
      if (prev_active_peer != nullptr) {
        BTIF_TRACE_DEBUG("%s: Reset UI config if set already for prev active device %s ",
                                __func__, prev_active_device.ToString().c_str());
        prev_active_peer->reset_peer_dev_requested_config();
      }
    }

    active_bda = peer_address;// for stereo LEA active_bda = peer_address
    BtifAcmPeer* peer = nullptr;
    uint16_t oldProfileType = old_active_profile_type = current_active_profile_type;
    uint16_t old_context = active_context_type;

    if (active_bda.address[0] == 0x9E && active_bda.address[1] == 0x8B &&
                                       active_bda.address[2] == 0x00) {
      BTIF_TRACE_DEBUG("%s: group address ", __func__);
      tBTA_CSIP_CSET cset_info;
      memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
      cset_info = BTA_CsipGetCoordinatedSet(active_bda.address[5]);
      if (cset_info.size != 0) {
        std::vector<RawAddress>::iterator itr;
        BTIF_TRACE_DEBUG("%s: size of set members %d",
                           __func__, (cset_info.set_members).size());
        if ((cset_info.set_members).size() > 0) {
          for (itr =(cset_info.set_members).begin();
                  itr != (cset_info.set_members).end(); itr++) {
            BtifAcmPeer* grp_peer = FindPeer(*itr);
            if (grp_peer != nullptr) {
              if (((contextType == CONTEXT_TYPE_MUSIC) && grp_peer->IsConnectedForMusic()) ||
                  ((contextType == CONTEXT_TYPE_VOICE) && grp_peer->IsConnectedForVoice())) {
                BTIF_TRACE_DEBUG("%s: group peer %s is connected for %d ", __func__,
                                        grp_peer->PeerAddress().ToString().c_str(), contextType);
                peer = grp_peer;
              }
            }
          }
        }
      }
    } else {
      BTIF_TRACE_DEBUG("%s: twm address ", __func__);
      peer = FindPeer(peer_address);
    }
    BTIF_TRACE_DEBUG("%s address byte BDA:%02x", __func__,active_bda.address[5]);

    if(btif_ahim_is_aosp_aidl_hal_enabled() && !peer_address.IsEmpty()) {
      active_context_type = contextType;
      BTIF_TRACE_DEBUG("%s: active_context_type = %d", __func__, active_context_type);
    }

    if (contextType == CONTEXT_TYPE_MUSIC) {
      if (btif_ahim_is_aosp_aidl_hal_enabled()) {
        /*check if previous voice active device is streaming, then STOP it first*/
        if (!voice_active_peer_.IsEmpty() && !active_bda.IsEmpty()) {
          int setid = voice_active_setid_;
          if (setid < INVALID_SET_ID) {
            tBTA_CSIP_CSET cset_info;
            memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
            cset_info = BTA_CsipGetCoordinatedSet(setid);
            if (cset_info.size != 0) {
              std::vector<RawAddress>::iterator itr;
              BTIF_TRACE_DEBUG("%s: size of set members %d",
                                 __func__, (cset_info.set_members).size());
              bool is_grp_peer_streaming_for_voice = false;
              if ((cset_info.set_members).size() > 0) {
                for (itr =(cset_info.set_members).begin();
                                  itr != (cset_info.set_members).end(); itr++) {
                  BtifAcmPeer* grp_peer = FindPeer(*itr);
                  if (grp_peer != nullptr && grp_peer->IsStreaming() &&
                                 grp_peer->GetStreamContextType() == CONTEXT_TYPE_VOICE) {
                    BTIF_TRACE_DEBUG("%s: peer is streaming %s ",
                                     __func__, grp_peer->PeerAddress().ToString().c_str());
                    is_grp_peer_streaming_for_voice = true;
                    grp_peer->SetFlags(BtifAcmPeer::kFlagCallAndMusicSync);
                    btif_acm_check_and_cancel_in_call_tracker_timer_();
                    btif_acm_initiator_dispatch_stop_req(*itr, false, ui_req);
                  }
                }

                BTIF_TRACE_DEBUG("%s: oldProfileType: %d, is_grp_peer_streaming_for_voice: %d",
                                 __func__, oldProfileType, is_grp_peer_streaming_for_voice);

                if (is_grp_peer_streaming_for_voice) {
                  // TODO to send the spream suspended with reconfig for AIDL
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED, oldProfileType);

                  //Below check to skip for LE-A to LE-A switching
                  //Also ensures session close properly when setactive set to null.
                  if (!active_bda.IsEmpty() && voice_active_peer_ == active_bda) {
                    peer_ready_promise.set_value();
                    return true;
                  }
                }
              }
            }
          } else {
            BTIF_TRACE_DEBUG("%s: Voice_active_peer_ is twm device,"
                             " and check for whether streaming context is for voice ", __func__);
            BtifAcmPeer* twm_peer = FindPeer(music_active_peer_);
            if (twm_peer != nullptr && twm_peer->IsStreaming() &&
                !twm_peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend) &&
                                 twm_peer->GetStreamContextType() == CONTEXT_TYPE_VOICE) {
              BTIF_TRACE_DEBUG("%s: music_active_peer_ %s is streaming, send stop ",
                                 __func__, twm_peer->PeerAddress().ToString().c_str());
              twm_peer->SetFlags(BtifAcmPeer::kFlagCallAndMusicSync);
              btif_acm_check_and_cancel_in_call_tracker_timer_();
              btif_acm_initiator_dispatch_stop_req(music_active_peer_, false, ui_req);

              BTIF_TRACE_DEBUG("%s: oldProfileType: %d", __func__, oldProfileType);

              // TODO to send the spream suspended with reconfig for AIDL
              //btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED, oldProfileType);

              //Below check to skip for LE-A to LE-A switching
              //Also ensures session close properly when setactive set to null.
              if (!active_bda.IsEmpty() && music_active_peer_ == active_bda) {
                if (old_context == CONTEXT_TYPE_VOICE) {
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED, BAP_CALL);
                } else {
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED, oldProfileType);
                }
                peer_ready_promise.set_value();
                current_active_profile_type = profileType;
                return true;
              } else if (!active_bda.IsEmpty() &&
                         music_active_peer_ != active_bda && !pending_switch) {
                pending_switch = true;
                isPendingDeviceSwitch(active_bda);
              }
            } else if (!active_bda.IsEmpty() && !music_active_peer_.IsEmpty() &&
                      active_bda != music_active_peer_ && !pending_switch) {
              pending_switch = true;
              isPendingDeviceSwitch(active_bda);
            }
          }
        }
      }

      if (music_active_peer_ == active_bda) {
        //Same active device, profileType may have changed.
        if (btif_ahim_is_aosp_aidl_hal_enabled()) {
          int setid = music_active_setid_;
          if (setid < INVALID_SET_ID) {
            tBTA_CSIP_CSET cset_info;
            memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
            cset_info = BTA_CsipGetCoordinatedSet(setid);
            if (cset_info.size != 0) {
              std::vector<RawAddress>::iterator itr;
              BTIF_TRACE_DEBUG("%s: size of set members %d",
                                 __func__, (cset_info.set_members).size());
              bool is_grp_peer_streaming_for_sub_music = false;
              if ((cset_info.set_members).size() > 0) {
                for (itr =(cset_info.set_members).begin();
                                  itr != (cset_info.set_members).end(); itr++) {
                  BtifAcmPeer* grp_peer = FindPeer(*itr);
                  if (grp_peer != nullptr && grp_peer->IsStreaming() &&
                                 grp_peer->GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
                    BTIF_TRACE_DEBUG("%s: peer is streaming %s ",
                                     __func__, grp_peer->PeerAddress().ToString().c_str());
                    is_grp_peer_streaming_for_sub_music = true;
                    btif_acm_initiator_dispatch_stop_req(*itr, false, ui_req);
                    grp_peer->SetFlags(BtifAcmPeer::kFlagMediaSubUsecaseSync);
                    if (oldProfileType == GCP || profileType == GCP) {
                      grp_peer->SetFlags(BtifAcmPeer::kFlagPendingAlsAndMetadataSync);
                    }
                  }
                }

                BTIF_TRACE_DEBUG("%s: oldProfileType: %d, profileType: %d,"
                                 " is_grp_peer_streaming_for_sub_music: %d ",
                                   __func__, oldProfileType, profileType,
                                   is_grp_peer_streaming_for_sub_music);

                if (is_grp_peer_streaming_for_sub_music) {
                  if(oldProfileType == GCP_RX || profileType == GCP_RX) {
                    btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                         GCP_RX);
                  } else if (oldProfileType == WMCP || profileType == WMCP) {
                    btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                         oldProfileType);
                    btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                         profileType);
                  } else if(oldProfileType == GCP || profileType == GCP) {
                    btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                         oldProfileType);
                  }
                  current_active_profile_type = profileType;
                  peer_ready_promise.set_value();
                  return true;
                }
              }
            }
          } else {
            BTIF_TRACE_DEBUG("%s: music_active_peer_ is twm device ", __func__);
            BtifAcmPeer* twm_peer = FindPeer(music_active_peer_);
            if (twm_peer != nullptr && twm_peer->IsStreaming() &&
                                 twm_peer->GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
              BTIF_TRACE_DEBUG("%s: music_active_peer_ %s is streaming, send stop ",
                                 __func__, twm_peer->PeerAddress().ToString().c_str());
              btif_acm_initiator_dispatch_stop_req(music_active_peer_, false, ui_req);
              if(btif_ahim_is_aosp_aidl_hal_enabled()) {
                twm_peer->SetFlags(BtifAcmPeer::kFlagMediaSubUsecaseSync);

                BTIF_TRACE_DEBUG("%s: oldProfileType: %d, profileType: %d ",
                                     __func__, oldProfileType, profileType);

                if(oldProfileType == GCP_RX || profileType == GCP_RX) {
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                       GCP_RX);
                } else if (oldProfileType == BAP && profileType == WMCP) {
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                       BAP);
                }else if (oldProfileType == WMCP || profileType == WMCP) {
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                       oldProfileType);
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                       profileType);
                } else if(oldProfileType == GCP || profileType == GCP) {
                  twm_peer->SetFlags(BtifAcmPeer::kFlagPendingAlsAndMetadataSync);
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                       oldProfileType);
                }

                current_active_profile_type = profileType;
                peer_ready_promise.set_value();
                return true;
              }
            }
          }
        }

        if ((peer != nullptr) && (current_active_profile_type != 0) &&
            (current_active_profile_type != profileType)) {
          BTIF_TRACE_DEBUG("%s current_active_profile_type: %d, profileType: %d"
                           " peer->GetProfileType() %d", __func__,
                           current_active_profile_type, profileType,
                           peer->GetProfileType());

          if ((peer->GetProfileType() & profileType) == 0) {
            std::unique_lock<std::mutex> guard(acm_session_wait_mutex_);
            acm_session_wait = false;
            if (reconfig_acm_initiator(peer_address, profileType, ui_req)) {
              acm_session_wait_cv.wait_for(guard, std::chrono::milliseconds(3000),
                                                    []{return acm_session_wait;});
              BTIF_TRACE_EVENT("%s: done with signal",__func__);
            }
          } else {
            current_active_profile_type = profileType;
            if (current_active_profile_type == GCP_RX) {
              if(peer != nullptr){
                current_active_config = peer->get_peer_media_codec_config();
              }
              current_active_gaming_rx_config = current_gaming_rx_config;
            } else if (current_active_profile_type == WMCP) {
              current_active_config = current_recording_config;
            } else {
                if(peer != nullptr){
                  current_active_config = peer->get_peer_media_codec_config();
                }
              }

            if (!btif_acm_source_restart_session(music_active_peer_, active_bda)) {
              // cannot set promise but need to be handled within restart_session
              return false;
            }
            if (current_active_profile_type == WMCP) {
              sink_latency = btif_acm_get_active_device_latency();
              BTIF_TRACE_EVENT("%s: sink_latency = %dms", __func__, sink_latency);
              if ((sink_latency > 0) &&
                  !btif_acm_update_sink_latency_change(sink_latency * 10)) {
                BTIF_TRACE_ERROR("%s: unable to update latency", __func__);
              }
            }
          }
          //current_active_profile_type = profileType;
          peer_ready_promise.set_value();
          return true;
        } else {
          if(btif_ahim_is_aosp_aidl_hal_enabled() && (peer != nullptr)) {
            // update the configs properly
            if (current_active_profile_type == GCP_RX) {
              current_active_config = peer->get_peer_media_codec_config();
              current_active_gaming_rx_config = current_gaming_rx_config;
            } else if (current_active_profile_type == WMCP) {
              current_active_config = current_recording_config;
            } else {
              current_active_config = peer->get_peer_media_codec_config();
            }

            BTIF_TRACE_EVENT("%s: only restarting the sessions",__func__);
            if (!btif_acm_source_restart_session(music_active_peer_, active_bda)) {
              // cannot set promise but need to be handled within restart_session
              return false;
            }
          }
          peer_ready_promise.set_value();
          return true;
        }
      }

      if (active_bda.IsEmpty()) {
        BTIF_TRACE_EVENT("%s: set address is empty, shutdown the Acm initiator",
                         __func__);
        btif_acm_check_and_cancel_lock_release_timer(music_active_setid_);
        if ((GetGroupLockStatus(music_active_setid_) ==
                    BtifAcmInitiator::kFlagStatusLocked) ||
            (GetGroupLockStatus(music_active_setid_) ==
                    BtifAcmInitiator::kFlagStatusSubsetLocked)) {
          if (!btif_acm_request_csip_unlock(music_active_setid_)) {
            BTIF_TRACE_ERROR("%s: error unlocking", __func__);
          }
        }
        music_active_peer_ = active_bda;
        if (!btif_ahim_is_aosp_aidl_hal_enabled() ||
            (btif_ahim_is_aosp_aidl_hal_enabled() &&
             music_active_peer_.IsEmpty() && voice_active_peer_.IsEmpty())) {
          btif_acm_source_end_session(music_active_peer_);
          current_active_profile_type = 0;
          active_context_type = 0;
        }

        //current_active_profile_type = 0;
        memset(&current_active_config, 0, sizeof(current_active_config));
        memset(&current_active_gaming_rx_config, 0,
                               sizeof(current_active_gaming_rx_config));
        peer_ready_promise.set_value();
        return true;
      }

      btif_acm_check_and_cancel_lock_release_timer(music_active_setid_);
      if ((GetGroupLockStatus(music_active_setid_) ==
                   BtifAcmInitiator::kFlagStatusLocked) ||
          (GetGroupLockStatus(music_active_setid_) ==
                   BtifAcmInitiator::kFlagStatusSubsetLocked)) {
        if (!btif_acm_request_csip_unlock(music_active_setid_)) {
          BTIF_TRACE_ERROR("%s: error unlocking", __func__);
        }
      }

      /*check if previous active device is streaming, then STOP it first*/
      if (!music_active_peer_.IsEmpty() ||
          (btif_ahim_is_aosp_aidl_hal_enabled() &&
           music_active_peer_ == active_bda)) {
        int setid = music_active_setid_;
        if (setid < INVALID_SET_ID) {
          tBTA_CSIP_CSET cset_info;
          memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
          cset_info = BTA_CsipGetCoordinatedSet(setid);
          if (cset_info.size != 0) {
            std::vector<RawAddress>::iterator itr;
            BTIF_TRACE_DEBUG("%s: size of set members %d",
                               __func__, (cset_info.set_members).size());
            bool is_prev_grp_peer_streaming_for_music = false;
            if ((cset_info.set_members).size() > 0) {
              for (itr =(cset_info.set_members).begin();
                                itr != (cset_info.set_members).end(); itr++) {
                BtifAcmPeer* grp_peer = FindPeer(*itr);
                if (grp_peer != nullptr && grp_peer->IsStreaming() &&
                               grp_peer->GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
                  BTIF_TRACE_DEBUG("%s: peer is streaming %s ",
                                   __func__, grp_peer->PeerAddress().ToString().c_str());
                  is_prev_grp_peer_streaming_for_music = true;
                  btif_acm_initiator_dispatch_stop_req(*itr, false, ui_req);

                  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
                    grp_peer->SetFlags(BtifAcmPeer::kFlagMediaSubUsecaseSync);
                    if (oldProfileType == GCP || current_active_profile_type == GCP) {
                      grp_peer->SetFlags(BtifAcmPeer::kFlagPendingAlsAndMetadataSync);
                    }
                  }
                }
              }

              BTIF_TRACE_DEBUG("%s: oldProfileType: %d,"
                               ", current_active_profile_type: %d "
                               ", is_prev_grp_peer_streaming_for_music: %d",
                               __func__, oldProfileType, current_active_profile_type,
                               is_prev_grp_peer_streaming_for_music);

              if (is_prev_grp_peer_streaming_for_music) {
                if(btif_ahim_is_aosp_aidl_hal_enabled()) {
                  BTIF_TRACE_DEBUG("%s: oldProfileType: %d,"
                                   " current_active_profile_type: %d ", __func__,
                                   oldProfileType, current_active_profile_type);

                  if(oldProfileType == GCP_RX || current_active_profile_type == GCP_RX) {
                    btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                         GCP_RX);
                  } else if (oldProfileType == WMCP || current_active_profile_type == WMCP) {
                    btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                         oldProfileType);
                    btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                         current_active_profile_type);
                  } else if(oldProfileType == GCP || current_active_profile_type == GCP) {
                    btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                         oldProfileType);
                  }

                  //Below check to skip for LE-A to LE-A switching
                  //Also ensures session close properly when setactive set to null.
                  if (!active_bda.IsEmpty() && music_active_peer_ == active_bda) {
                    peer_ready_promise.set_value();
                    return true;
                  }
                }
              }
            }
          }
        } else {
          BTIF_TRACE_DEBUG("%s: music_active_peer_ is twm device ", __func__);
          BtifAcmPeer* twm_peer = FindPeer(music_active_peer_);
          if (twm_peer != nullptr && twm_peer->IsStreaming() &&
            !twm_peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend) &&
                               twm_peer->GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
            BTIF_TRACE_DEBUG("%s: music_active_peer_ %s is streaming, send stop ",
                               __func__, twm_peer->PeerAddress().ToString().c_str());
            btif_acm_initiator_dispatch_stop_req(music_active_peer_, false, ui_req);
            if(btif_ahim_is_aosp_aidl_hal_enabled()) {
              twm_peer->SetFlags(BtifAcmPeer::kFlagMediaSubUsecaseSync);

              BTIF_TRACE_DEBUG("%s: oldProfileType: %d,"
                                " current_active_profile_type: %d ", __func__,
                                oldProfileType, current_active_profile_type);
              if (!active_bda.IsEmpty() && active_bda == twm_peer->PeerAddress()) {
                if(oldProfileType == GCP_RX || current_active_profile_type == GCP_RX) {
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                       GCP_RX);
                } else if (oldProfileType == WMCP || current_active_profile_type == WMCP) {
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                       oldProfileType);
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                       current_active_profile_type);
                } else if(oldProfileType == GCP || current_active_profile_type == GCP) {
                  twm_peer->SetFlags(BtifAcmPeer::kFlagPendingAlsAndMetadataSync);
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                                       oldProfileType);
                }
              } else if (!active_bda.IsEmpty() && active_bda != twm_peer->PeerAddress() && !pending_switch){
                pending_switch = true;
                isPendingDeviceSwitch(active_bda);
                BTIF_TRACE_DEBUG("%s: device switch, let framework take care of it",__func__);
              }
              //Below check to skip for LE-A to LE-A switching
              //Also ensures session close properly when setactive set to null.
              if (!active_bda.IsEmpty() && music_active_peer_ == active_bda) {
                peer_ready_promise.set_value();
                return true;
              }
            }
          }else if(!active_bda.IsEmpty() &&
                    active_bda != music_active_peer_ && !pending_switch) {
            pending_switch = true;
              isPendingDeviceSwitch(active_bda);
          }
        }
      } else if (!active_bda.IsEmpty() && !music_active_peer_.IsEmpty() &&
      music_active_peer_ != active_bda && !pending_switch) {
        //first time connection
        pending_switch = true;
        isPendingDeviceSwitch(active_bda);
      }

      if ((peer != nullptr) && ((peer->GetProfileType() & profileType) == 0)) {
        BTIF_TRACE_DEBUG("%s peer.GetProfileType() %d, profileType %d",
                             __func__, peer->GetProfileType(), profileType);
        std::unique_lock<std::mutex> guard(acm_session_wait_mutex_);
        acm_session_wait = false;
        if (reconfig_acm_initiator(peer_address, profileType, ui_req)) {
          acm_session_wait_cv.wait_for(guard, std::chrono::milliseconds(3000),
                                                 []{return acm_session_wait;});
          BTIF_TRACE_EVENT("%s: done with signal",__func__);
        }
      } else {
        current_active_profile_type = profileType;
        if (current_active_profile_type == GCP_RX) {
          if(peer != nullptr) {
            current_active_config = peer->get_peer_media_codec_config();
          }
          current_active_gaming_rx_config = current_gaming_rx_config;
        } else if (current_active_profile_type == WMCP) {
          current_active_config = current_recording_config;
        } else {
          if (peer != nullptr) {
            BTIF_TRACE_DEBUG("%s: get_peer_media_codec_config", __func__);
            current_active_config = peer->get_peer_media_codec_config();
          } else {
            BTIF_TRACE_DEBUG("%s: current_media_config", __func__);
            current_active_config = current_media_config;
          }
        }

        if (!btif_acm_source_restart_session(music_active_peer_, active_bda)) {
          // cannot set promise but need to be handled within restart_session
          return false;
        }
      }

      music_active_peer_ = active_bda;
      if (active_bda.address[0] == 0x9E && active_bda.address[1] == 0x8B &&
                                         active_bda.address[2] == 0x00) {
        BTIF_TRACE_DEBUG("%s: get set ID from group BD address ", __func__);
        music_active_setid_ = active_bda.address[5];
      } else {
        BTIF_TRACE_DEBUG("%s: get set ID from peer data ", __func__);
        if (peer != nullptr)
          music_active_setid_ = peer->SetId();
      }

      if (current_active_profile_type == WMCP) {
        sink_latency = btif_acm_get_active_device_latency();
        BTIF_TRACE_EVENT("%s: sink_latency = %dms", __func__, sink_latency);
        if ((sink_latency > 0) &&
            !btif_acm_update_sink_latency_change(sink_latency * 10)) {
          BTIF_TRACE_ERROR("%s: unable to update latency", __func__);
        }
      }
      peer_ready_promise.set_value();
      return true;
    } else if (contextType == CONTEXT_TYPE_VOICE) {
      if(btif_ahim_is_aosp_aidl_hal_enabled()) {
        /*check if previous music active device is streaming, then STOP it first*/
        if (!music_active_peer_.IsEmpty() && !active_bda.IsEmpty()) {
          int setid = music_active_setid_;
          if (setid < INVALID_SET_ID) {
            tBTA_CSIP_CSET cset_info;
            memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
            cset_info = BTA_CsipGetCoordinatedSet(setid);
            if (cset_info.size != 0) {
              std::vector<RawAddress>::iterator itr;
              BTIF_TRACE_DEBUG("%s: size of set members %d",
                                 __func__, (cset_info.set_members).size());
              bool is_grp_peer_streaming_for_music = false;
              if ((cset_info.set_members).size() > 0) {
                for (itr =(cset_info.set_members).begin();
                                  itr != (cset_info.set_members).end(); itr++) {
                  BtifAcmPeer* grp_peer = FindPeer(*itr);
                  if (grp_peer != nullptr && grp_peer->IsStreaming() &&
                                 grp_peer->GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
                    BTIF_TRACE_DEBUG("%s: peer is streaming %s ",
                                     __func__, grp_peer->PeerAddress().ToString().c_str());
                    is_grp_peer_streaming_for_music = true;
                    grp_peer->SetFlags(BtifAcmPeer::kFlagCallAndMusicSync);
                    btif_acm_check_and_cancel_in_call_tracker_timer_();
                    btif_acm_initiator_dispatch_stop_req(*itr, false, ui_req);
                  }
                }

                BTIF_TRACE_DEBUG("%s: oldProfileType: %d, is_grp_peer_streaming_for_music: %d",
                                  __func__, oldProfileType, is_grp_peer_streaming_for_music);

                if (is_grp_peer_streaming_for_music) {
                  // TODO to send the spream suspended with reconfig for AIDL
                  btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED, oldProfileType);

                  //Below check to skip for LE-A to LE-A switching
                  //Also ensures session close properly when setactive set to null.
                  if (!active_bda.IsEmpty() && voice_active_peer_ == active_bda) {
                    peer_ready_promise.set_value();
                    return true;
                  }
                }
              }
            }
          } else {
            BTIF_TRACE_DEBUG("%s: music_active_peer_ is twm device ", __func__);
            BtifAcmPeer* twm_peer = FindPeer(music_active_peer_);
            if (twm_peer != nullptr && twm_peer->IsStreaming() &&
                !twm_peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend) &&
                                 twm_peer->GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
              BTIF_TRACE_DEBUG("%s: music_active_peer_ %s is streaming, send stop ",
                                 __func__, twm_peer->PeerAddress().ToString().c_str());
              twm_peer->SetFlags(BtifAcmPeer::kFlagCallAndMusicSync);
              btif_acm_check_and_cancel_in_call_tracker_timer_();
              btif_acm_initiator_dispatch_stop_req(music_active_peer_, false, ui_req);

              BTIF_TRACE_DEBUG("%s: oldProfileType: %d", __func__, oldProfileType);

              // TODO to send the spream suspended with reconfig for AIDL
              if (!active_bda.IsEmpty() && active_bda == twm_peer->PeerAddress()) {
                btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED, oldProfileType);
              } else if (!active_bda.IsEmpty() && !pending_switch){
                pending_switch = true;
                isPendingDeviceSwitch(active_bda);
                BTIF_TRACE_DEBUG("%s: device switch, let framework take care of it",__func__);
              }

              //Below check to skip for LE-A to LE-A switching
              //Also ensures session close properly when setactive set to null.
              if (!active_bda.IsEmpty() && voice_active_peer_ == active_bda) {
                peer_ready_promise.set_value();
                return true;
              }
            } else if (!voice_active_peer_.IsEmpty() && !active_bda.IsEmpty() &&
                       active_bda != voice_active_peer_ && !pending_switch) {
              pending_switch = true;
              isPendingDeviceSwitch(active_bda);
              BTIF_TRACE_DEBUG("%s: Device switch, let framework take care",__func__);
            }
          }
        }
      }

      if (voice_active_peer_ == active_bda) {
        if(btif_ahim_is_aosp_aidl_hal_enabled()) {
          //Same active device, profileType may have changed.
          if (peer != nullptr) {
            BTIF_TRACE_DEBUG("%s current_active_profile_type: %d, profileType: %d"
                             " peer->GetProfileType() %d", __func__,
                             current_active_profile_type, profileType,
                             peer->GetProfileType());

            if ((peer->GetProfileType() & profileType) == 0) {
              std::unique_lock<std::mutex> guard(acm_session_wait_mutex_);
              acm_session_wait = false;
              if (reconfig_acm_initiator(peer_address, profileType, ui_req)) {
                acm_session_wait_cv.wait_for(guard, std::chrono::milliseconds(3000),
                                                      []{return acm_session_wait;});
                BTIF_TRACE_EVENT("%s: done with signal",__func__);
              }
            } else {
              current_active_config = peer->get_peer_voice_tx_codec_config();
              if (!btif_acm_source_restart_session(voice_active_peer_, active_bda)) {
                // cannot set promise but need to be handled within restart_session
                return false;
              }
            }
            peer_ready_promise.set_value();
            return true;
          } else {
            peer_ready_promise.set_value();
            return true;
          }
        } else {
          peer_ready_promise.set_value();
          return true;
        }
      }

      if(btif_ahim_is_aosp_aidl_hal_enabled()) {
        if (active_bda.IsEmpty()) {
          BTIF_TRACE_EVENT("%s: set address is empty, shutdown the Acm initiator",
                           __func__);
          btif_acm_check_and_cancel_lock_release_timer(music_active_setid_);
          if ((GetGroupLockStatus(music_active_setid_) ==
                      BtifAcmInitiator::kFlagStatusLocked) ||
              (GetGroupLockStatus(music_active_setid_) ==
                      BtifAcmInitiator::kFlagStatusSubsetLocked)) {
            if (!btif_acm_request_csip_unlock(music_active_setid_)) {
              BTIF_TRACE_ERROR("%s: error unlocking", __func__);
            }
          }
          voice_active_peer_ = active_bda;
          if(!btif_ahim_is_aosp_aidl_hal_enabled() ||
            (btif_ahim_is_aosp_aidl_hal_enabled() &&
             music_active_peer_.IsEmpty() && voice_active_peer_.IsEmpty())) {
            btif_acm_source_end_session(voice_active_peer_);
            current_active_profile_type = 0;
            active_context_type = 0;
          }

          memset(&current_active_config, 0, sizeof(current_active_config));
          peer_ready_promise.set_value();
          return true;
        }
      } else {
        if (active_bda.IsEmpty()) {
          BTIF_TRACE_EVENT("%s: peer address is empty, shutdown the acm initiator",
                           __func__);
          voice_active_peer_ = active_bda;
          peer_ready_promise.set_value();
          return true;
        }
      }

      if(btif_ahim_is_aosp_aidl_hal_enabled()) {
        btif_acm_check_and_cancel_lock_release_timer(voice_active_setid_);
        if ((GetGroupLockStatus(voice_active_setid_) ==
                     BtifAcmInitiator::kFlagStatusLocked) ||
            (GetGroupLockStatus(voice_active_setid_) ==
                     BtifAcmInitiator::kFlagStatusSubsetLocked)) {
          if (!btif_acm_request_csip_unlock(voice_active_setid_)) {
            BTIF_TRACE_ERROR("%s: error unlocking", __func__);
          }
        }
      }

      /*check if previous active device is streaming, then STOP it first*/
      if (!voice_active_peer_.IsEmpty()) {
        int setid = voice_active_setid_;
        if (setid < INVALID_SET_ID) {
          tBTA_CSIP_CSET cset_info;
          memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
          cset_info = BTA_CsipGetCoordinatedSet(setid);
          if (cset_info.size != 0) {
            std::vector<RawAddress>::iterator itr;
            BTIF_TRACE_DEBUG("%s: size of set members %d",
                                      __func__, (cset_info.set_members).size());
            if ((cset_info.set_members).size() > 0) {
              for (itr =(cset_info.set_members).begin();
                                   itr != (cset_info.set_members).end(); itr++) {
                BtifAcmPeer* grp_peer = FindPeer(*itr);
                if (grp_peer != nullptr && grp_peer->IsStreaming() &&
                                   grp_peer->GetStreamContextType() == CONTEXT_TYPE_VOICE) {
                  BTIF_TRACE_DEBUG("%s: voice peer is streaming %s ",
                               __func__, grp_peer->PeerAddress().ToString().c_str());
                  btif_acm_check_and_cancel_in_call_tracker_timer_();
                  btif_acm_initiator_dispatch_stop_req(*itr, false, ui_req);
                }
              }
            }
          }
        } else {
          BTIF_TRACE_DEBUG("%s: voice_active_peer_ is twm device ", __func__);
          BtifAcmPeer* twm_peer = FindPeer(voice_active_peer_);
          if (twm_peer != nullptr && twm_peer->IsStreaming() &&
                                   twm_peer->GetStreamContextType() == CONTEXT_TYPE_VOICE) {
            BTIF_TRACE_DEBUG("%s: voice_active_peer_ %s is streaming, send stop ",
                              __func__, twm_peer->PeerAddress().ToString().c_str());
            btif_acm_check_and_cancel_in_call_tracker_timer_();
            btif_acm_initiator_dispatch_stop_req(voice_active_peer_, false, ui_req);
            if (!voice_active_peer_.IsEmpty()  && !active_bda.IsEmpty() &&
                active_bda != voice_active_peer_ && !pending_switch) {
              pending_switch = true;
              isPendingDeviceSwitch(active_bda);
            }
          } else {
            if (!voice_active_peer_.IsEmpty() && !active_bda.IsEmpty() &&
                 active_bda != voice_active_peer_ && !pending_switch) {
              pending_switch = true;
              isPendingDeviceSwitch(active_bda);
            }
          }
        }
      }

      if (btif_ahim_is_aosp_aidl_hal_enabled()) {
        if (peer != nullptr)
          current_active_config = peer->get_peer_voice_tx_codec_config();

        if (!btif_acm_source_restart_session(voice_active_peer_, active_bda)) {
          // cannot set promise but need to be handled within restart_session
          return false;
        }
      }

      voice_active_peer_ = active_bda;
      if (active_bda.address[0] == 0x9E && active_bda.address[1] == 0x8B &&
                                             active_bda.address[2] == 0x00) {
        BTIF_TRACE_DEBUG("%s: get set ID from group BD address ", __func__);
        voice_active_setid_ = active_bda.address[5];
      } else {
        BTIF_TRACE_DEBUG("%s: get set ID from peer data ", __func__);
        if (peer != nullptr)
          voice_active_setid_ = peer->SetId();
      }
      peer_ready_promise.set_value();
      return true;
    } else {
      peer_ready_promise.set_value();
      return true;
    }
  }

  void btif_acm_initiator_encoder_user_config_update_req(
      const RawAddress& peer_addr,
      const std::vector<CodecConfig>& codec_user_preferences,
      std::promise<void> peer_ready_promise);


  void UpdateCodecConfig(
      const RawAddress& peer_address,
      const std::vector<CodecConfig>& codec_preferences,
      int contextType,
      int profileType,
      std::promise<void> peer_ready_promise) {
    // Restart the session if the codec for the active peer is updated
    if (!peer_address.IsEmpty() && music_active_peer_ == peer_address) {
      btif_acm_source_end_session(music_active_peer_);
    }

    btif_acm_initiator_encoder_user_config_update_req(
        peer_address, codec_preferences, std::move(peer_ready_promise));
  }

  const std::map<RawAddress, BtifAcmPeer*>& Peers() const { return peers_; }

  std::vector<RawAddress> locked_devices;
 private:
  void CleanupAllPeers();

  btacm_initiator_callbacks_t* callbacks_;
  bool enabled_;
  int max_connected_peers_;

  RawAddress music_active_peer_;
  RawAddress voice_active_peer_;
  uint8_t music_active_setid_;
  uint8_t voice_active_setid_;
  uint8_t music_active_set_locked_dev_count_;
  uint8_t voice_active_set_locked_dev_count_;
  bool is_music_active_set_started_;
  bool is_voice_active_set_started_;
  bool is_conn_update_enabled_;

  uint8_t csip_app_id_;
  bool is_csip_reg_;
  uint8_t lock_flags_;

  alarm_t* music_set_lock_release_timer_;
  alarm_t* voice_set_lock_release_timer_;
  alarm_t* acm_group_procedure_timer_;
  alarm_t* acm_conn_interval_timer_;
  alarm_t* acm_in_call_tracker_timer_;

  std::map<RawAddress, BtifAcmPeer*> peers_;
  std::map<RawAddress, uint8_t> addr_setid_pair;
  std::map<uint8_t, uint8_t> set_cig_pair;//setid and cig id pair
  std::map<RawAddress, std::map<uint8_t, uint8_t> > cig_cis_pair;//cig id and cis id pair
  std::map<uint8_t, int> set_lock_status_;
};

/*****************************************************************************
 *  Static variables
 *****************************************************************************/
static BtifAcmInitiator btif_acm_initiator;
std::vector<CodecConfig> unicast_codecs_capabilities_snk;
std::vector<CodecConfig> unicast_codecs_capabilities_src;
static CodecConfig default_config;
static bool mandatory_codec_selected = false;
static bt_status_t disconnect_acm_initiator(const RawAddress& peer_address,
                                            uint16_t contextType);

static bt_status_t start_stream_acm_initiator(const RawAddress& peer_address,
                                              uint16_t contextType);

static bt_status_t stop_stream_acm_initiator(const RawAddress& peer_address,
                                             uint16_t contextType);

static bt_status_t stop_stream_acm_initiator_internal(
                                             const RawAddress& peer_address,
                                             uint16_t contextType,
                                             bool in_call_tracker_tout);

static void btif_acm_handle_csip_status_locked(std::vector<RawAddress> addr,
                                               uint8_t setId);

static void btif_acm_handle_evt(uint16_t event, char* p_param);
static void btif_report_connection_state(const RawAddress& peer_address,
                                         btacm_connection_state_t state,
                                         uint16_t contextType);
static void btif_report_audio_state(const RawAddress& peer_address,
                                    btacm_audio_state_t state,
                                    uint16_t contextType);

static void btif_acm_check_and_start_lock_release_timer(uint8_t setId);

static void btif_acm_initiator_lock_release_timer_timeout(void* data);

static void btif_acm_check_and_start_group_procedure_timer(uint8_t setId);
static void btif_acm_check_and_start_conn_Interval_timer(BtifAcmPeer* peer);
static void btif_acm_initiator_conn_Interval_timer_timeout(void *data);
static void btif_acm_check_and_cancel_conn_Interval_timer();

static void btif_acm_check_and_start_in_call_tracker_timer_(BtifAcmPeer* peer);

static void btif_acm_check_and_cancel_in_call_tracker_timer_();

static void btif_acm_initiator_in_call_tracker_timer_timeout(void *data);

static void btif_acm_check_and_cancel_group_procedure_timer(uint8_t setId);
static void btif_acm_initiator_group_procedure_timer_timeout(void *data);
bool compare_codec_config_(CodecConfig &first, CodecConfig &second);
void print_codec_parameters(CodecConfig config);
void print_qos_parameters(QosConfig qos_config);
bool select_best_codec_config(const RawAddress& bd_addr,
                              uint16_t context_type,
                              uint8_t profile_type,
                              CodecConfig *codec_config,
                              int dir, int config_type,
                              std::string req_ocdec);
static UcastClientInterface* sUcastClientInterface = nullptr;
static PacsClientInterface* sPacsClientInterface = nullptr;
static uint16_t pacs_client_id = 0;

/*****************************************************************************
 * Local helper functions
 *****************************************************************************/

const char* dump_acm_sm_event_name(btif_acm_sm_event_t event) {
  switch ((int)event) {
    CASE_RETURN_STR(BTA_ACM_DISCONNECT_EVT)
    CASE_RETURN_STR(BTA_ACM_CONNECT_EVT)
    CASE_RETURN_STR(BTA_ACM_START_EVT)
    CASE_RETURN_STR(BTA_ACM_STOP_EVT)
    CASE_RETURN_STR(BTA_ACM_RECONFIG_EVT)
    CASE_RETURN_STR(BTA_ACM_CONFIG_EVT)
    CASE_RETURN_STR(BTIF_ACM_CONNECT_REQ_EVT)
    CASE_RETURN_STR(BTIF_ACM_BAP_CONNECT_REQ_EVT)
    CASE_RETURN_STR(BTA_ACM_PACS_DISCONNECT_EVT)
    CASE_RETURN_STR(BTA_ACM_PACS_CONNECT_EVT)
    CASE_RETURN_STR(BTA_ACM_PACS_DISCOVERY_RES_EVT)
    CASE_RETURN_STR(BTIF_ACM_DISCONNECT_REQ_EVT)
    CASE_RETURN_STR(BTIF_ACM_START_STREAM_REQ_EVT)
    CASE_RETURN_STR(BTIF_ACM_STOP_STREAM_REQ_EVT)
    CASE_RETURN_STR(BTIF_ACM_SUSPEND_STREAM_REQ_EVT)
    CASE_RETURN_STR(BTIF_ACM_RECONFIG_REQ_EVT)
    CASE_RETURN_STR(BTA_ACM_CONN_UPDATE_TIMEOUT_EVT)
    default:
      return "UNKNOWN_EVENT";
  }
}

const char* dump_csip_event_name(btif_csip_sm_event_t event) {
  switch ((int)event) {
    CASE_RETURN_STR(BTA_CSIP_NEW_SET_FOUND_EVT)
    CASE_RETURN_STR(BTA_CSIP_SET_MEMBER_FOUND_EVT)
    CASE_RETURN_STR(BTA_CSIP_CONN_STATE_CHG_EVT)
    CASE_RETURN_STR(BTA_CSIP_LOCK_STATUS_CHANGED_EVT)
    CASE_RETURN_STR(BTA_CSIP_LOCK_AVAILABLE_EVT)
    CASE_RETURN_STR(BTA_CSIP_SET_SIZE_CHANGED)
    CASE_RETURN_STR(BTA_CSIP_SET_SIRK_CHANGED)
    default:
      return "UNKNOWN_EVENT";
  }
}

void btif_acm_signal_session_ready() {
  std::unique_lock<std::mutex> guard(acm_session_wait_mutex_);
  if(!acm_session_wait) {
    acm_session_wait = true;
    acm_session_wait_cv.notify_all();
  } else {
    BTIF_TRACE_WARNING("%s: already signalled ",__func__);
  }
}

void btif_ft_change_event_cback(uint8_t evt_len, uint8_t *p_data) {
  if (evt_len != 6 || p_data == NULL) {
    BTIF_TRACE_WARNING("%s: incorrect event params ",__func__);
    return;
  }
  uint8_t status = p_data[0];
  if (status != 0) {
    BTIF_TRACE_WARNING("%s: status not proper ",__func__);
    return;
  }
  btif_acm_handle_evt(BTA_ACM_LATENCY_UPDATE_EVT, (char*)&p_data);
}

bool fetch_media_tx_codec_qos_config(const RawAddress& bd_addr,
                                     int profile_type,
                                     StreamConnect *conn_media) {
  BTIF_TRACE_DEBUG("%s: Peer %s , profile_type: %d",
                   __func__, bd_addr.ToString().c_str(), profile_type);
  CodecQosConfig conf;
  bool is_media_tx_codec_qos_fetched = false;
  BtifAcmPeer* peer = btif_acm_initiator.FindPeer(bd_addr);
  if (peer == nullptr) {
    BTIF_TRACE_WARNING("%s: peer is NULL", __func__);
    return false;
  }
  std::string cfg;
  is_media_tx_codec_qos_fetched =
              peer->SetCodecQosConfig(peer->PeerAddress(), profile_type,
                                      MEDIA_CONTEXT, SNK, false, cfg);
  BTIF_TRACE_DEBUG("%s: is_media_tx_codec_qos_fetched: %d",
                          __func__, is_media_tx_codec_qos_fetched);
  if (is_media_tx_codec_qos_fetched == false) {
    BTIF_TRACE_DEBUG("%s: media Tx codecQos config not fetched, return", __func__);
    return false;
  }

  conf = peer->get_peer_media_codec_qos_config();
  print_codec_parameters(conf.codec_config);
  print_qos_parameters(conf.qos_config);
  conn_media->codec_qos_config_pair.push_back(conf);
  conn_media->stream_type.type = CONTENT_TYPE_MEDIA;
  conn_media->stream_type.direction = ASE_DIRECTION_SINK;

  if (profile_type == BAP) {
    //Media audio context
    conn_media->stream_type.audio_context = CONTENT_TYPE_MEDIA;
  } else if (profile_type == GCP) {
    //Game audio context
    conn_media->stream_type.audio_context = CONTENT_TYPE_GAME;
  }
  return true;
}

bool fetch_media_rx_codec_qos_config(const RawAddress& bd_addr,
                                     int profile_type,
                                     StreamConnect *conn_media) {
  BTIF_TRACE_DEBUG("%s: Peer %s , profile_type: %d",
                   __func__, bd_addr.ToString().c_str(), profile_type);
  CodecQosConfig conf;
  bool is_media_rx_codec_qos_fetched = false;
  memset(&conf, 0, sizeof(CodecQosConfig));
  BtifAcmPeer* peer = btif_acm_initiator.FindPeer(bd_addr);
  if (peer == nullptr) {
    BTIF_TRACE_WARNING("%s: peer is NULL", __func__);
    return false;
  }
  std::string cfg;
  is_media_rx_codec_qos_fetched =
         peer->SetCodecQosConfig(peer->PeerAddress(), profile_type, MEDIA_CONTEXT,
                                 SRC, false, cfg);
  BTIF_TRACE_DEBUG("%s: is_media_rx_codec_qos_fetched: %d",
                          __func__, is_media_rx_codec_qos_fetched);
  if (is_media_rx_codec_qos_fetched == false) {
    BTIF_TRACE_DEBUG("%s: media Rx codecQos config not fetched, return", __func__);
    return false;
  }

  conf = peer->get_peer_media_codec_qos_config();
  print_codec_parameters(conf.codec_config);
  print_qos_parameters(conf.qos_config);
  conn_media->codec_qos_config_pair.push_back(conf);
  conn_media->stream_type.type = CONTENT_TYPE_MEDIA;
  conn_media->stream_type.direction = ASE_DIRECTION_SRC;

  if (profile_type == WMCP) {
    //Live audio context
    conn_media->stream_type.audio_context = CONTENT_TYPE_LIVE;
  } else if (profile_type == GCP_RX) {
    //Game audio context
    conn_media->stream_type.audio_context = CONTENT_TYPE_GAME;
  }
  return true;
}

bool fetch_voice_rx_codec_qos_config(const RawAddress& bd_addr,
                                     int profile_type,
                                     StreamConnect *conn_voice) {
  BTIF_TRACE_DEBUG("%s: Peer %s , profile_type: %d",
                   __func__, bd_addr.ToString().c_str(), profile_type);
  CodecQosConfig conf;
  bool is_voice_rx_codec_qos_fetched = false;
  BtifAcmPeer* peer = btif_acm_initiator.FindPeer(bd_addr);
  if (peer == nullptr) {
    BTIF_TRACE_WARNING("%s: peer is NULL", __func__);
    return false;
  }
  std::string cfg;
  is_voice_rx_codec_qos_fetched =
    peer->SetCodecQosConfig(peer->PeerAddress(), BAP, VOICE_CONTEXT, SRC, true, cfg);
  BTIF_TRACE_DEBUG("%s: is_voice_rx_codec_qos_fetched: %d",
                          __func__, is_voice_rx_codec_qos_fetched);
  if (is_voice_rx_codec_qos_fetched == false) {
    BTIF_TRACE_DEBUG("%s: voice Rx codecQos config not fetched, return", __func__);
    return false;
  }

  conf = peer->get_peer_voice_rx_codec_qos_config();
  print_codec_parameters(conf.codec_config);
  print_qos_parameters(conf.qos_config);
  conn_voice->codec_qos_config_pair.push_back(conf);
  conn_voice->stream_type.type = CONTENT_TYPE_CONVERSATIONAL;
  conn_voice->stream_type.audio_context = CONTENT_TYPE_CONVERSATIONAL;
  conn_voice->stream_type.direction = ASE_DIRECTION_SRC;
  return true;
}

bool fetch_voice_tx_codec_qos_config(const RawAddress& bd_addr,
                                     int profile_type,
                                     StreamConnect *conn_voice) {
  BTIF_TRACE_DEBUG("%s: Peer %s , profile_type: %d",
                     __func__, bd_addr.ToString().c_str(), profile_type);
  CodecQosConfig conf;
  bool is_voice_tx_codec_qos_fetched = false;
  BtifAcmPeer* peer = btif_acm_initiator.FindPeer(bd_addr);
  if (peer == nullptr) {
    BTIF_TRACE_WARNING("%s: peer is NULL", __func__);
    return false;
  }
  std::string cfg;
  is_voice_tx_codec_qos_fetched =
    peer->SetCodecQosConfig(peer->PeerAddress(), BAP, VOICE_CONTEXT, SNK, true, cfg);
  BTIF_TRACE_DEBUG("%s: is_voice_tx_codec_qos_fetched: %d",
                          __func__, is_voice_tx_codec_qos_fetched);
  if (is_voice_tx_codec_qos_fetched == false) {
    BTIF_TRACE_DEBUG("%s: voice Tx codecQos config not fetched, return", __func__);
    return false;
  }
  conf = peer->get_peer_voice_tx_codec_qos_config();
  print_codec_parameters(conf.codec_config);
  print_qos_parameters(conf.qos_config);
  conn_voice->codec_qos_config_pair.push_back(conf);
  conn_voice->stream_type.type = CONTENT_TYPE_CONVERSATIONAL;
  conn_voice->stream_type.audio_context = CONTENT_TYPE_CONVERSATIONAL;
  conn_voice->stream_type.direction = ASE_DIRECTION_SINK;
  return true;
}

BtifAcmEvent::BtifAcmEvent(uint32_t event, const void* p_data, size_t data_length)
    : event_(event), data_(nullptr), data_length_(0) {
  DeepCopy(event, p_data, data_length);
}

BtifAcmEvent::BtifAcmEvent(const BtifAcmEvent& other)
    : event_(0), data_(nullptr), data_length_(0) {
  *this = other;
}

BtifAcmEvent& BtifAcmEvent::operator=(const BtifAcmEvent& other) {
  DeepFree();
  DeepCopy(other.Event(), other.Data(), other.DataLength());
  return *this;
}

BtifAcmEvent::~BtifAcmEvent() { DeepFree(); }

std::string BtifAcmEvent::ToString() const {
  return BtifAcmEvent::EventName(event_);
}

std::string BtifAcmEvent::EventName(uint32_t event) {
  std::string name = dump_acm_sm_event_name((btif_acm_sm_event_t)event);
  std::stringstream ss_value;
  ss_value << "(0x" << std::hex << event << ")";
  return name + ss_value.str();
}

void BtifAcmEvent::DeepCopy(uint32_t event, const void* p_data,
                           size_t data_length) {
  event_ = event;
  data_length_ = data_length;
  if (data_length == 0) {
    data_ = nullptr;
  } else {
    data_ = osi_malloc(data_length_);
    memcpy(data_, p_data, data_length);
  }
}

void BtifAcmEvent::DeepFree() {
  osi_free_and_reset((void**)&data_);
  data_length_ = 0;
}

BtifCsipEvent::BtifCsipEvent(uint32_t event,
                             const void* p_data,
                             size_t data_length)
    : event_(event), data_(nullptr), data_length_(0) {
  DeepCopy(event, p_data, data_length);
}

BtifCsipEvent::BtifCsipEvent(const BtifCsipEvent& other)
    : event_(0), data_(nullptr), data_length_(0) {
  *this = other;
}

BtifCsipEvent& BtifCsipEvent::operator=(const BtifCsipEvent& other) {
  DeepFree();
  DeepCopy(other.Event(), other.Data(), other.DataLength());
  return *this;
}

BtifCsipEvent::~BtifCsipEvent() { DeepFree(); }

std::string BtifCsipEvent::ToString() const {
  return BtifCsipEvent::EventName(event_);
}

std::string BtifCsipEvent::EventName(uint32_t event) {
  std::string name = dump_csip_event_name((btif_csip_sm_event_t)event);
  std::stringstream ss_value;
  ss_value << "(0x" << std::hex << event << ")";
  return name + ss_value.str();
}

void BtifCsipEvent::DeepCopy(uint32_t event, const void* p_data,
                           size_t data_length) {
  event_ = event;
  data_length_ = data_length;
  if (data_length == 0) {
    data_ = nullptr;
  } else {
    data_ = osi_malloc(data_length_);
    memcpy(data_, p_data, data_length);
  }
}

void BtifCsipEvent::DeepFree() {
  osi_free_and_reset((void**)&data_);
  data_length_ = 0;
}

BtifAcmPeer::BtifAcmPeer(const RawAddress& peer_address, uint8_t peer_sep,
                         uint8_t set_id, uint8_t cig_id, uint8_t cis_id)
    : peer_address_(peer_address),
      peer_sep_(peer_sep),
      set_id_(set_id),
      cig_id_(cig_id),
      cis_id_(cis_id),
      state_machine_(*this),
      flags_(0) {}

BtifAcmPeer::~BtifAcmPeer() { }

std::string BtifAcmPeer::FlagsToString() const {
  std::string result;

  if (flags_ & BtifAcmPeer::kFlagPendingLocalSuspend) {
    if (!result.empty()) result += "|";
    result += "LOCAL_SUSPEND_PENDING";
  }
  if (flags_ & BtifAcmPeer::kFlagPendingReconfigure) {
    if (!result.empty()) result += "|";
    result += "PENDING_RECONFIGURE";
  }
  if (flags_ & BtifAcmPeer::kFlagPendingStart) {
    if (!result.empty()) result += "|";
    result += "PENDING_START";
  }
  if (flags_ & BtifAcmPeer::kFlagPendingStop) {
    if (!result.empty()) result += "|";
    result += "PENDING_STOP";
  }
  if (flags_ & BtifAcmPeer::kFLagPendingStartAfterReconfig) {
    if (!result.empty()) result += "|";
    result += "PENDING_START_AFTER_RECONFIG";
  }
  if (flags_ & BtifAcmPeer::kFLagPendingReconfigAfterStart) {
    if (!result.empty()) result += "|";
    result += "PENDING_RECONFIG_AFTER_START";
  }
  if (flags_ & BtifAcmPeer::kFLagPendingStartAfterPeerReconfig) {
    if (!result.empty()) result += "|";
    result += "PENDING_START_AFTER_PEER_RECONFIG";
  }
  if (flags_ & BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend) {
    if (!result.empty()) result += "|";
    result += "CLEAR_SUSPEND_AFTER_PEER_SUSPEND";
  }
  if (flags_ & BtifAcmPeer::kFlagCallAndMusicSync) {
    if (!result.empty()) result += "|";
    result += "CALL_AND_MEDIA_SYNC";
  }
  if (flags_ & BtifAcmPeer::kFlagMediaSubUsecaseSync) {
    if (!result.empty()) result += "|";
    result += "MEDIA_SUB_USECASE_SYNC";
  }
  if (flags_ & BtifAcmPeer::kFlagPendingSetActiveDev) {
    if (!result.empty()) result += "|";
    result += "PENDING_SET_ACTIVE_DEV";
  }
  if (flags_ & BtifAcmPeer::kFlagPendingAlsAndMetadataSync) {
    if (!result.empty()) result += "|";
    result += "PENDING_ALS_METADATA_SYNC";
  }
  if (result.empty()) result = "None";

  return base::StringPrintf("0x%x(%s)", flags_, result.c_str());
}

bt_status_t BtifAcmPeer::Init() {
  state_machine_.Start();
  return BT_STATUS_SUCCESS;
}

void BtifAcmPeer::Cleanup() {
  state_machine_.Quit();
}

bool BtifAcmPeer::CanBeDeleted() const {
  return (
      (state_machine_.StateId() == BtifAcmStateMachine::kStateIdle) &&
      (state_machine_.PreviousStateId() != BtifAcmStateMachine::kStateInvalid));
}

const RawAddress& BtifAcmPeer::MusicActivePeerAddress() const {
  return btif_acm_initiator.MusicActivePeer();
}
const RawAddress& BtifAcmPeer::VoiceActivePeerAddress() const {
  return btif_acm_initiator.VoiceActivePeer();
}
uint8_t BtifAcmPeer::MusicActiveSetId() const {
  return btif_acm_initiator.MusicActiveCSetId();
}
uint8_t BtifAcmPeer::VoiceActiveSetId() const {
  return btif_acm_initiator.VoiceActiveCSetId();
}

bool BtifAcmPeer::IsConnected() const {
  int state = state_machine_.StateId();
  return ((state == BtifAcmStateMachine::kStateOpened) ||
          (state == BtifAcmStateMachine::kStateStarted));
}

bool BtifAcmPeer::IsStreaming() const {
  int state = state_machine_.StateId();
  return (state == BtifAcmStateMachine::kStateStarted);
}

bool BtifAcmPeer::IsReconfinging() const {
  int state = state_machine_.StateId();
  BTIF_TRACE_DEBUG("IsReconfinging: %d", state);
  return (state == BtifAcmStateMachine::kStateReconfiguring);
}

BtifAcmInitiator::~BtifAcmInitiator() {
  CleanupAllPeers();
}

void init_local_capabilities() {
  int64_t cs3 = 0;
  if (is_lc3q_supported_on_dut)
    cs3 |= LE_AUDIO_AVAILABLE_LICENSED;
  CodecConfig acm_local_capability_lc3 = {
                            CodecIndex::CODEC_INDEX_SOURCE_LC3,
                            CodecPriority::CODEC_PRIORITY_DEFAULT,
                            CodecSampleRate::CODEC_SAMPLE_RATE_8000 |
                            CodecSampleRate::CODEC_SAMPLE_RATE_16000 |
                            CodecSampleRate::CODEC_SAMPLE_RATE_32000 |
                            CodecSampleRate::CODEC_SAMPLE_RATE_24000 |
                            CodecSampleRate::CODEC_SAMPLE_RATE_44100 |
                            CodecSampleRate::CODEC_SAMPLE_RATE_48000,
                            CodecBPS::CODEC_BITS_PER_SAMPLE_24,
                            CodecChannelMode::CODEC_CHANNEL_MODE_STEREO |
                            CodecChannelMode::CODEC_CHANNEL_MODE_MONO,
                            0, 0, cs3, 0};
  unicast_codecs_capabilities_snk.push_back(acm_local_capability_lc3);
  if (is_aptx_supported_on_dut) {
    cs3 = 0;
    cs3 |= QHS_SUPPORT_AVAILABLE;
    CodecConfig acm_local_capability_aptx_le = {
                            CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE,
                            CodecPriority::CODEC_PRIORITY_DEFAULT,
                            CodecSampleRate::CODEC_SAMPLE_RATE_48000 |
                            CodecSampleRate::CODEC_SAMPLE_RATE_96000,
                            CodecBPS::CODEC_BITS_PER_SAMPLE_24,
                            CodecChannelMode::CODEC_CHANNEL_MODE_STEREO,
                            0, 0, 0, 0};
    unicast_codecs_capabilities_snk.push_back(acm_local_capability_aptx_le);
  }
  unicast_codecs_capabilities_src.push_back(acm_local_capability_lc3);
}

void BtifAcmInitiator::Cleanup() {
  LOG_INFO(LOG_TAG, "%s", __PRETTY_FUNCTION__);
  if (!enabled_) return;
  std::promise<void> peer_ready_promise;
  btif_disable_service(BTA_ACM_INITIATOR_SERVICE_ID); // ACM deregistration required?
  btif_vmcp_cleanup();
  CleanupAllPeers();
  alarm_free(music_set_lock_release_timer_);
  music_set_lock_release_timer_ = nullptr;
  alarm_free(acm_group_procedure_timer_);
  acm_group_procedure_timer_ = nullptr;
  alarm_free(acm_conn_interval_timer_);
  acm_conn_interval_timer_ = nullptr;
  alarm_free(acm_in_call_tracker_timer_);
  acm_in_call_tracker_timer_ = nullptr;
  callbacks_ = nullptr;
  enabled_ = false;
  if (sPacsClientInterface != nullptr) {
    sPacsClientInterface->Cleanup(pacs_client_id);
    sPacsClientInterface = nullptr;
  }
  if (sUcastClientInterface != nullptr) {
    sUcastClientInterface->Cleanup();
    sUcastClientInterface = nullptr;
  }
}

BtifAcmPeer* BtifAcmInitiator::FindPeer(const RawAddress& peer_address) {
  auto it = peers_.find(peer_address);
  if (it != peers_.end()) return it->second;
  return nullptr;
}

uint8_t BtifAcmInitiator:: FindPeerSetId(const RawAddress& peer_address) {
    auto it = addr_setid_pair.find(peer_address);
    if (it != addr_setid_pair.end()) return it->second;
    return 0xff;
}

uint8_t BtifAcmInitiator:: FindPeerBySetId(uint8_t setid) {
  for (auto it : addr_setid_pair) {
    if (it.second == setid) {
      return setid;
    }
  }
  return 0xff;
}

uint8_t BtifAcmInitiator:: FindPeerCigId(uint8_t setid) {
    auto it = set_cig_pair.find(setid);
    if (it != set_cig_pair.end()) return it->second;
    return 0xff;
}

uint8_t BtifAcmInitiator:: FindPeerByCigId(uint8_t cigid) {
  for (auto it : set_cig_pair) {
    if (it.second == cigid) {
      return cigid;
    }
  }
  return 0xff;
}

uint8_t BtifAcmInitiator:: FindPeerByCisId(uint8_t cigid, uint8_t cisid) {
  for (auto itr = cig_cis_pair.begin(); itr != cig_cis_pair.end(); itr++) {
    for (auto ptr = itr->second.begin(); ptr != itr->second.end(); ptr++) {
      if (ptr->first == cigid) {
        if (ptr->second == cisid) {
          return cisid;
        }
      }
    }
  }
  return 0xff;
}

BtifAcmPeer* BtifAcmInitiator::FindOrCreatePeer(const RawAddress& peer_address) {
  BTIF_TRACE_DEBUG("%s: peer_address=%s ", __PRETTY_FUNCTION__,
                   peer_address.ToString().c_str());

  BtifAcmPeer* peer = FindPeer(peer_address);
  if (peer != nullptr) return peer;

  uint8_t SetId, CigId, CisId;
  //get the set id from CSIP.
  //TODO: need UUID ?
  Uuid uuid = Uuid::kEmpty;
  LOG_INFO(LOG_TAG, "%s ACM UUID = %s", __func__, uuid.ToString().c_str());

  SetId = BTA_CsipGetDeviceSetId(peer_address, uuid);
  BTIF_TRACE_EVENT("%s: set id from csip : %d", __func__, SetId);

  if (SetId == INVALID_SET_ID) {
    SetId = FindPeerSetId(peer_address);
    // Find next available SET ID to use
    if (SetId == 0xff) {
      for (SetId = kPeerMinSetId; SetId < kPeerMaxSetId; SetId++) {
        if (FindPeerBySetId(SetId) == 0xff) break;
      }
    }
  }
  if (SetId == kPeerMaxSetId) {
    BTIF_TRACE_ERROR(
        "%s: Cannot create peer for peer_address=%s : "
        "cannot allocate unique SET ID",
        __PRETTY_FUNCTION__, peer_address.ToString().c_str());
    return nullptr;
  }
  addr_setid_pair.insert(std::make_pair(peer_address, SetId));

  //Find next available CIG ID to use
  CigId = FindPeerCigId(SetId);
  if (CigId == 0xff) {
    for (CigId = kCigIdMin; CigId < kCigIdMax; ) {
      if (FindPeerByCigId(CigId) == 0xff) break;
      CigId += 4;
    }
  }

  if (CigId == kCigIdMax) {
    BTIF_TRACE_ERROR(
        "%s: cannot allocate unique CIG ID to = %s ",
        __func__, peer_address.ToString().c_str());
    return nullptr;
  }
  set_cig_pair.insert(std::make_pair(SetId, CigId));

  //Find next available CIS ID to use
  for (CisId = kCigIdMin; CisId < kCigIdMax; CisId++) {
    if (FindPeerByCisId(CigId, CisId) == 0xff) break;
  }

  if (CisId == kCigIdMax) {
    BTIF_TRACE_ERROR(
        "%s: cannot allocate unique CIS ID to = %s ",
        __func__, peer_address.ToString().c_str());
    return nullptr;
  }

  //Below property need set for TMAP PTS TCs of config B and C
  //so that it would fill single direction config to Tx or Rx.
  char pts_tmap_conf_B_and_C[PROPERTY_VALUE_MAX] = "false";
  property_get("persist.vendor.btstack.pts_tmap_conf_B_and_C",
                                    pts_tmap_conf_B_and_C, "false");
  LOG_DEBUG(LOG_TAG, "%s: pts_tmap_conf_B_and_C: %s",
                               __PRETTY_FUNCTION__, pts_tmap_conf_B_and_C);

  if (!strncmp("true", pts_tmap_conf_B_and_C, 4)) {
    uint16_t num_sink_ases, num_src_ases;
    btif_bap_get_params(peer_address, nullptr, nullptr, &num_sink_ases,
                        &num_src_ases, nullptr, nullptr);
    if(num_sink_ases && num_src_ases) {
      CisId = 1;
    } else if(num_sink_ases) {
      CisId = 0;
    }
  }

  cig_cis_pair.insert(std::make_pair(peer_address, map<uint8_t, uint8_t>()));
  cig_cis_pair[peer_address].insert(std::make_pair(CigId, CisId));

  LOG_INFO(LOG_TAG,
           "%s: Create peer: peer_address=%s, set_id=%d, cig_id=%d, cis_id=%d",
           __PRETTY_FUNCTION__, peer_address.ToString().c_str(), SetId, CigId, CisId);
  peer = new BtifAcmPeer(peer_address, ACM_TSEP_SNK, SetId, CigId, CisId);
  peer->SetPeerVoiceTxState(StreamState::DISCONNECTED);
  peer->SetPeerVoiceRxState(StreamState::DISCONNECTED);
  peer->SetPeerMusicTxState(StreamState::DISCONNECTED);
  peer->SetPeerMusicRxState(StreamState::DISCONNECTED);
  peer->SetPeerPacsState(ConnectionState::DISCONNECTED);
  peer->set_peer_audio_src_location(0);
  peer->set_peer_audio_sink_location(0);

  if (SetId >= kPeerMinSetId && SetId < kPeerMaxSetId) {
    LOG_INFO(LOG_TAG,
             "%s: Created peer is TWM device",__PRETTY_FUNCTION__);
  }
  peers_.insert(std::make_pair(peer_address, peer));
  peer->Init();
  return peer;
}

BtifAcmPeer* BtifAcmInitiator::FindMusicActivePeer() {
  for (auto it : peers_) {
    BtifAcmPeer* peer = it.second;
    if (peer->IsPeerActiveForMusic()) {
      return peer;
    }
  }
  return nullptr;
}

bool BtifAcmInitiator::AllowedToConnect(const RawAddress& peer_address) const {
  int connected = 0;

  // Count peers that are in the process of connecting or already connected
  for (auto it : peers_) {
    const BtifAcmPeer* peer = it.second;
    switch (peer->StateMachine().StateId()) {
      case BtifAcmStateMachine::kStateOpening:
      case BtifAcmStateMachine::kStateOpened:
      case BtifAcmStateMachine::kStateStarted:
      case BtifAcmStateMachine::kStateReconfiguring:
        if (peer->PeerAddress() == peer_address) {
          return true;  // Already connected or accounted for
        }
        connected++;
        break;
      default:
        break;
    }
  }
  return (connected < max_connected_peers_);
}

bool BtifAcmInitiator::IsAcmIdle() const {
  int connected = 0;

  // Count peers that are in the process of connecting or already connected
  for (auto it : peers_) {
    const BtifAcmPeer* peer = it.second;
    switch (peer->StateMachine().StateId()) {
      case BtifAcmStateMachine::kStateOpening:
      case BtifAcmStateMachine::kStateOpened:
      case BtifAcmStateMachine::kStateStarted:
      case BtifAcmStateMachine::kStateReconfiguring:
      case BtifAcmStateMachine::kStateClosing:
        connected++;
        break;
      default:
        break;
    }
  }
  return (connected == 0);
}

bool BtifAcmInitiator::IsSetIdle(uint8_t setId) const {
  int connected = 0;
  tBTA_CSIP_CSET cset_info = BTA_CsipGetCoordinatedSet(setId);
  std::vector<RawAddress>::iterator itr;
  if ((cset_info.set_members).size() > 0) {
    for (itr = (cset_info.set_members).begin();
                 itr != (cset_info.set_members).end(); itr++) {
      BtifAcmPeer* peer = btif_acm_initiator.FindPeer(*itr);
      switch (peer->StateMachine().StateId()) {
        case BtifAcmStateMachine::kStateOpening:
        case BtifAcmStateMachine::kStateOpened:
        case BtifAcmStateMachine::kStateStarted:
        case BtifAcmStateMachine::kStateReconfiguring:
        case BtifAcmStateMachine::kStateClosing:
          connected++;
          break;
        default:
          break;
      }
    }
  }
  return (connected == 0);
}

bool BtifAcmInitiator::IsOtherSetPeersIdle(const RawAddress& peer_address,
                                           uint8_t setId) const {
  int connected = 0;
  tBTA_CSIP_CSET cset_info = BTA_CsipGetCoordinatedSet(setId);
  std::vector<RawAddress>::iterator itr;
  if ((cset_info.set_members).size() > 0) {
    for (itr = (cset_info.set_members).begin();
                     itr != (cset_info.set_members).end(); itr++) {
      if (*itr  == peer_address) continue;
      BtifAcmPeer* peer = btif_acm_initiator.FindPeer(*itr);
      if (peer == nullptr) continue;
      switch (peer->StateMachine().StateId()) {
        case BtifAcmStateMachine::kStateOpening:
        case BtifAcmStateMachine::kStateOpened:
        case BtifAcmStateMachine::kStateStarted:
        case BtifAcmStateMachine::kStateReconfiguring:
        case BtifAcmStateMachine::kStateClosing:
          connected++;
          break;
        default:
          break;
      }
    }
  }
  return (connected == 0);
}

bool BtifAcmInitiator::DeletePeer(const RawAddress& peer_address) {
  auto it = peers_.find(peer_address);
  if (it == peers_.end()) return false;
  BtifAcmPeer* peer = it->second;
  for (auto itr = addr_setid_pair.begin();
                           itr != addr_setid_pair.end(); ++itr) {
    if (itr->second == peer->SetId()) {
      addr_setid_pair.erase(itr);
      break;
    }
  }
  for (auto itr = set_cig_pair.begin();
                              itr != set_cig_pair.end(); ++itr) {
    if (itr->second == peer->SetId()) {
      set_cig_pair.erase(itr);
      break;
    }
  }
  bool found = false;
  for (auto itr = cig_cis_pair.begin();
                             itr != cig_cis_pair.end(); itr++) {
    for (auto ptr = itr->second.begin();
                             ptr != itr->second.end(); ptr++) {
      if (ptr->first == peer->CigId()) {
        if (ptr->second == peer->CisId()) {
          cig_cis_pair.erase(itr);
          found = true;
          break;
        }
      }
    }
    if (found)
      break;
  }
  peer->Cleanup();
  peers_.erase(it);
  delete peer;
  return true;
}

void BtifAcmInitiator::DeleteIdlePeers() {
  for (auto it = peers_.begin(); it != peers_.end();) {
    BtifAcmPeer* peer = it->second;
    auto prev_it = it++;
    if (!peer->CanBeDeleted()) continue;
    LOG_INFO(LOG_TAG, "%s: Deleting idle peer: %s ", __func__,
             peer->PeerAddress().ToString().c_str());
    for (auto itr = addr_setid_pair.begin(); itr != addr_setid_pair.end(); ++itr) {
      if (itr->second == peer->SetId()) {
        addr_setid_pair.erase(itr);
        break;
      }
    }
    for (auto itr = set_cig_pair.begin(); itr != set_cig_pair.end(); ++itr) {
      if (itr->second == peer->SetId()) {
        set_cig_pair.erase(itr);
        break;
      }
    }
    bool found = false;
    for (auto itr = cig_cis_pair.begin(); itr != cig_cis_pair.end(); itr++) {
      for (auto ptr = itr->second.begin(); ptr != itr->second.end(); ptr++) {
        if (ptr->first == peer->CigId()) {
          if (ptr->second == peer->CisId()) {
            cig_cis_pair.erase(itr);
            found = true;
            break;
          }
        }
      }
      if (found)
        break;
    }
    peer->Cleanup();
    peers_.erase(prev_it);
    delete peer;
  }
}

void BtifAcmInitiator::CleanupAllPeers() {
  while (!peers_.empty()) {
    auto it = peers_.begin();
    BtifAcmPeer* peer = it->second;
    for (auto itr = addr_setid_pair.begin(); itr != addr_setid_pair.end(); ++itr) {
      if (itr->second == peer->SetId()) {
        addr_setid_pair.erase(itr);
        break;
      }
    }
    for (auto itr = set_cig_pair.begin(); itr != set_cig_pair.end(); ++itr) {
      if (itr->second == peer->SetId()) {
        set_cig_pair.erase(itr);
        break;
      }
    }
    bool found = false;
    for (auto itr = cig_cis_pair.begin(); itr != cig_cis_pair.end(); itr++) {
      for (auto ptr = itr->second.begin(); ptr != itr->second.end(); ptr++) {
        if (ptr->first == peer->CigId()) {
          if (ptr->second == peer->CisId()) {
            cig_cis_pair.erase(itr);
            found = true;
            break;
          }
        }
      }
      if (found)
        break;
    }
    peer->Cleanup();
    peers_.erase(it);
    delete peer;
  }
}

class PacsClientCallbacksImpl : public PacsClientCallbacks {
 public:
  ~PacsClientCallbacksImpl() = default;
  void OnInitialized(int status,
                     int client_id) override {
    LOG(WARNING) << __func__ << client_id;
    pacs_client_id = client_id;
  }
  void OnConnectionState(const RawAddress& address,
                         ConnectionState state) override {
    LOG(INFO) << __func__;
    BtifAcmPeer* peer = btif_acm_initiator.FindPeer(address);
    if (peer == nullptr) {
      BTIF_TRACE_DEBUG("%s: Peer is NULL", __PRETTY_FUNCTION__);
    }
    switch (state) {
      case ConnectionState::DISCONNECTED:
      case ConnectionState::DISCONNECTING: {
        tBTA_ACM_PACS_STATE_INFO data = {.bd_addr = address,
                                         .state = state
                                        };
        btif_acm_handle_evt(BTA_ACM_PACS_DISCONNECT_EVT, (char*)&data);
      } break;
      case ConnectionState::CONNECTING:
      case ConnectionState::CONNECTED: {
        tBTA_ACM_PACS_STATE_INFO data = {.bd_addr = address,
                                         .state = state
                                        };
        btif_acm_handle_evt(BTA_ACM_PACS_CONNECT_EVT, (char*)&data);
      } break;
      default:
        break;
    }
  }
  void OnAudioContextAvailable(const RawAddress& address,
                        uint32_t available_contexts) override {
    LOG(INFO) << __func__;
  }

  void OnSearchComplete(int status, const RawAddress& address,
                        std::vector<CodecConfig> sink_pac_records,
                        std::vector<CodecConfig> src_pac_records,
                        uint32_t sink_locations,
                        uint32_t src_locations,
                        uint32_t available_contexts,
                        uint32_t supported_contexts) override {
    LOG(INFO) << __func__;
    BtifAcmPeer* peer = btif_acm_initiator.FindPeer(address);
    if (peer == nullptr) {
      BTIF_TRACE_DEBUG("%s: Peer is NULL", __PRETTY_FUNCTION__);
    }
    tBTA_ACM_PACS_DATA_INFO data = {.status = status,
                                    .bd_addr = address,
                                    .sink_records = sink_pac_records,
                                    .src_records = src_pac_records
                                   };
    btif_acm_handle_evt(BTA_ACM_PACS_DISCOVERY_RES_EVT, (char*)&data);
  }
};

static PacsClientCallbacksImpl sPacsClientCallbacks;

class UcastClientCallbacksImpl : public UcastClientCallbacks {
 public:
  ~UcastClientCallbacksImpl() = default;
  void OnStreamState(const RawAddress& address,
                     std::vector<StreamStateInfo> streams_state_info) override {
    LOG(INFO) << __func__;
    BtifAcmPeer* peer = btif_acm_initiator.FindPeer(address);
    if (peer == nullptr) {
      BTIF_TRACE_DEBUG("%s: Peer is NULL", __PRETTY_FUNCTION__);
    }

    for (auto it = streams_state_info.begin();
                          it != streams_state_info.end(); ++it) {
      LOG(WARNING) << __func__ << ": address: " << address;
      LOG(WARNING) << __func__ << ": stream type:    "
                   << GetStreamType(it->stream_type.type);
      LOG(WARNING) << __func__ << ": stream context: "
                   << GetStreamType(it->stream_type.audio_context);
      LOG(WARNING) << __func__ << ": stream dir:     "
                   << GetStreamDirection(it->stream_type.direction);
      LOG(WARNING) << __func__ << ": stream state:   "
                   << GetStreamState(static_cast<int> (it->stream_state));

      switch (it->stream_state) {
        case StreamState::DISCONNECTED:
        case StreamState::DISCONNECTING: {
          tBTA_ACM_STATE_INFO data = {.bd_addr = address,
                                      .stream_type = it->stream_type,
                                      .stream_state = it->stream_state,
                                      .reason = it->reason,
                                      .stream_link_params = it->stream_link_params};
          btif_acm_handle_evt(BTA_ACM_DISCONNECT_EVT, (char*)&data);
        } break;

        case StreamState::CONNECTING:
        case StreamState::CONNECTED: {
          tBTA_ACM_STATE_INFO data = {.bd_addr = address,
                                      .stream_type = it->stream_type,
                                      .stream_state = it->stream_state,
                                      .reason = it->reason,
                                      .stream_link_params = it->stream_link_params};

          btif_acm_handle_evt(BTA_ACM_CONNECT_EVT, (char*)&data);
        } break;

        case StreamState::STARTING:
        case StreamState::STREAMING: {
          tBTA_ACM_STATE_INFO data = {.bd_addr = address,
                                      .stream_type = it->stream_type,
                                      .stream_state = it->stream_state,
                                      .reason = it->reason,
                                      .stream_link_params = it->stream_link_params};

          btif_acm_handle_evt(BTA_ACM_START_EVT, (char*)&data);
        } break;

        case StreamState::STOPPING: {
          tBTA_ACM_STATE_INFO data = {.bd_addr = address,
                                      .stream_type = it->stream_type,
                                      .stream_state = it->stream_state,
                                      .reason = it->reason,
                                      .stream_link_params = it->stream_link_params};

          btif_acm_handle_evt(BTA_ACM_STOP_EVT, (char*)&data);
        } break;

        case StreamState::RECONFIGURING: {
          tBTA_ACM_STATE_INFO data = {.bd_addr = address,
                                      .stream_type = it->stream_type,
                                      .stream_state = it->stream_state,
                                      .reason = it->reason,
                                      .stream_link_params = it->stream_link_params};

          btif_acm_handle_evt(BTA_ACM_RECONFIG_EVT, (char*)&data);
        } break;
        default:
          break;
      }
    }
  }

  void OnStreamConfig(const RawAddress& address,
                      std::vector<StreamConfigInfo> streams_config_info) override {
    LOG(INFO) << __func__;
    BtifAcmPeer* peer = btif_acm_initiator.FindPeer(address);
    if (peer == nullptr) {
      BTIF_TRACE_DEBUG("%s: Peer is NULL", __PRETTY_FUNCTION__);
    }

    for (auto it = streams_config_info.begin();
                            it != streams_config_info.end(); ++it) {
      print_codec_parameters(it->codec_config);
      print_qos_parameters(it->qos_config);
      BTIF_TRACE_DEBUG("%s: it->codec_config's codec_type = %d", __func__,
                       it->codec_config.codec_type);
      tBTA_ACM_CONFIG_INFO data = {.bd_addr = address,
                                   .stream_type = it->stream_type,
                                   .codec_config = it->codec_config,
                                   .audio_location = it->audio_location,
                                   .qos_config = it->qos_config,
                                   .codecs_selectable = it->codecs_selectable};
      btif_acm_handle_evt(BTA_ACM_CONFIG_EVT, (char*)&data);
    }
  }

  void OnStreamAvailable(const RawAddress& bd_addr,
                         uint16_t src_audio_contexts,
                         uint16_t sink_audio_contexts) override {
     LOG(INFO) << __func__;
     //Need to use during START of src and sink audio context
     BTIF_TRACE_DEBUG("%s: Peer %s, src_audio_context: 0x%x, "
                      "sink_audio_contexts: 0x%x", __func__,
                      bd_addr.ToString().c_str(),
                      src_audio_contexts, sink_audio_contexts);
  }

  const char* GetStreamType(uint16_t stream_type) {
    switch (stream_type) {
      CASE_RETURN_STR(CONTENT_TYPE_UNSPECIFIED)
      CASE_RETURN_STR(CONTENT_TYPE_CONVERSATIONAL)
      CASE_RETURN_STR(CONTENT_TYPE_MEDIA)
      CASE_RETURN_STR(CONTENT_TYPE_INSTRUCTIONAL)
      CASE_RETURN_STR(CONTENT_TYPE_NOTIFICATIONS)
      CASE_RETURN_STR(CONTENT_TYPE_ALERT)
      CASE_RETURN_STR(CONTENT_TYPE_MAN_MACHINE)
      CASE_RETURN_STR(CONTENT_TYPE_EMERGENCY)
      CASE_RETURN_STR(CONTENT_TYPE_RINGTONE)
      CASE_RETURN_STR(CONTENT_TYPE_SOUND_EFFECTS)
      CASE_RETURN_STR(CONTENT_TYPE_LIVE)
      CASE_RETURN_STR(CONTENT_TYPE_GAME)
      default:
       return "Unknown StreamType";
    }
  }

  const char* GetStreamDirection(uint8_t event) {
    switch (event) {
      CASE_RETURN_STR(ASE_DIRECTION_SINK)
      CASE_RETURN_STR(ASE_DIRECTION_SRC)
      default:
       return "Unknown StreamDirection";
    }
  }

  const char* GetStreamState(uint8_t event) {
    switch (event) {
      CASE_RETURN_STR(STREAM_STATE_DISCONNECTED)
      CASE_RETURN_STR(STREAM_STATE_CONNECTING)
      CASE_RETURN_STR(STREAM_STATE_CONNECTED)
      CASE_RETURN_STR(STREAM_STATE_STARTING)
      CASE_RETURN_STR(STREAM_STATE_STREAMING)
      CASE_RETURN_STR(STREAM_STATE_STOPPING)
      CASE_RETURN_STR(STREAM_STATE_DISCONNECTING)
      CASE_RETURN_STR(STREAM_STATE_RECONFIGURING)
      default:
       return "Unknown StreamState";
    }
  }
};

static UcastClientCallbacksImpl sUcastClientCallbacks;

bt_status_t BtifAcmInitiator::Init(
    btacm_initiator_callbacks_t* callbacks, int max_connected_acceptors,
    const std::vector<CodecConfig>& codec_priorities) {
  LOG_INFO(LOG_TAG, "%s: max_connected_acceptors=%d", __PRETTY_FUNCTION__,
           max_connected_acceptors);
  if (enabled_) return BT_STATUS_SUCCESS;
  CleanupAllPeers();
  max_connected_peers_ = max_connected_acceptors;
  alarm_free(music_set_lock_release_timer_);
  alarm_free(voice_set_lock_release_timer_);
  alarm_free(acm_group_procedure_timer_);
  alarm_free(acm_conn_interval_timer_);
  alarm_free(acm_in_call_tracker_timer_);
  music_set_lock_release_timer_ =
                alarm_new("btif_acm_initiator.music_set_lock_release_timer");
  voice_set_lock_release_timer_ =
                alarm_new("btif_acm_initiator.voice_set_lock_release_timer");
  acm_group_procedure_timer_ =
                alarm_new("btif_acm_initiator.acm_group_procedure_timer");
  acm_conn_interval_timer_ =
                alarm_new("btif_acm_initiator.acm_conn_interval_timer");
  acm_in_call_tracker_timer_ =
                alarm_new("btif_acm_initiator.acm_in_call_tracker_timer_");

  memset(&current_active_config, 0, sizeof(CodecConfig));
  memset(&current_media_config, 0, sizeof(CodecConfig));
  memset(&current_voice_config, 0, sizeof(CodecConfig));
  memset(&current_recording_config, 0, sizeof(CodecConfig));
  callbacks_ = callbacks;

  // Check local support based on property
  char lc3q_value[PROPERTY_VALUE_MAX] = "false";
  property_get("persist.vendor.service.bt.is_lc3q_supported", lc3q_value, "false");
  if (!strncmp("true", lc3q_value, 4)) {
    is_lc3q_supported_on_dut = true;
  } else {
    is_lc3q_supported_on_dut = false;
  }
  BTIF_TRACE_IMP("%s: is_lc3q_supported: %d", __func__, is_lc3q_supported_on_dut);

  char aptx_le_value[PROPERTY_VALUE_MAX] = "false";
  property_get("persist.vendor.service.bt.is_aptx_le_supported", aptx_le_value, "false");
  if (!strncmp("true", aptx_le_value, 4)) {
    is_aptx_supported_on_dut = true;
  } else {
    is_aptx_supported_on_dut = false;
  }
  BTIF_TRACE_IMP("%s: is_aptx_le_supported: %d", __func__, is_aptx_supported_on_dut);

  // Check local SoC codec related feature set supported
  const bt_device_qll_local_supported_features_t *qll_feature_list =
      controller_get_interface()->get_qll_features();
  if (HCI_QBCE_QLL_BN_VARIATION_BY_QHS_RATE(qll_feature_list->as_array)) {
    BTIF_TRACE_DEBUG("%s: BN Variation supported by controller", __func__);
    Is_BN_Variation_Supported = true;
  }

  if (HCI_QBCE_QLL_FT_CHNAGE(qll_feature_list->as_array)) {
    BTIF_TRACE_DEBUG("%s: FT Change supported by controller", __func__);
    Is_FT_Change_Supported = true;
  }

  bool is_lc3q_enhanced_gaming_enabled = false;
  char lc3q_ll_game_value[PROPERTY_VALUE_MAX] = "false";
  property_get("persist.vendor.service.bt.is_lc3q_enhanced_gaming", lc3q_ll_game_value, "false");
  if (!strncmp("true", lc3q_ll_game_value, 4)) {
    is_lc3q_enhanced_gaming_enabled = true;
  }
  BTIF_TRACE_IMP("%s: is_lc3q_supported: %d", __func__, is_lc3q_supported_on_dut);
  BTIF_TRACE_IMP("%s: lc3q_enhanced_gaming_enabled: %d", __func__, is_lc3q_enhanced_gaming_enabled);

  // deduce local encoder/decoder version based on local support
  if (is_lc3q_supported_on_dut) {
    dut_lc3_encoder_ver = LC3Q_CODEC_BER_SUPPORTED_VERSION;
    dut_lc3_decoder_ver = LC3Q_CODEC_BER_SUPPORTED_VERSION;
    if (Is_FT_Change_Supported && is_lc3q_enhanced_gaming_enabled) {
      dut_lc3_encoder_ver = LC3Q_CODEC_FT_CHANGE_SUPPORTED_VERSION;
    }
  }

  LOG_DEBUG(LOG_TAG, "%s: lc3 local encoder ver:%d, lc3 local decoder ver:%d,",
      __PRETTY_FUNCTION__, dut_lc3_encoder_ver, dut_lc3_decoder_ver);

  if (Is_BN_Variation_Supported && is_aptx_supported_on_dut) {
    dut_aptx_encoder_ver = APTX_LE_CODEC_BN_VARIATION_SUPPORTED_VERSION;
  }

  LOG_DEBUG(LOG_TAG, "%s: aptx local encoder ver:%d, aptx local decoder ver:%d,",
      __PRETTY_FUNCTION__, dut_aptx_encoder_ver, dut_aptx_decoder_ver);

  //init local capabilties
  init_local_capabilities();

  // register ACM with AHIM
  btif_register_cb();

  // Register callback for FT Changes
  btm_register_qle_cig_latency_changed_cback(btif_ft_change_event_cback);

  //CAS Service Discovery
  btif_register_uuid_srvc_disc(bluetooth::Uuid::FromString("00001853-0000-1000-8000-00805F9B34FB"));
  btif_vmcp_init();
  bt_status_t status1 = btif_acm_initiator_execute_service(true);
  if (status1 == BT_STATUS_SUCCESS) {
    BTIF_TRACE_EVENT("%s: status success", __func__);
  }

  if (sPacsClientInterface != nullptr) {
    LOG_INFO(LOG_TAG, "%s Cleaning up PACS client Interface before initializing...",
             __PRETTY_FUNCTION__);
    sPacsClientInterface = nullptr;
  }
  sPacsClientInterface = btif_pacs_client_get_interface();
  if (sPacsClientInterface == nullptr) {
    LOG_ERROR(LOG_TAG, "%s Failed to get PACS Interface", __PRETTY_FUNCTION__);
    return BT_STATUS_FAIL;
  }
  sPacsClientInterface->Init(&sPacsClientCallbacks);

  if (sUcastClientInterface != nullptr) {
    LOG_INFO(LOG_TAG, "%s Cleaning up BAP client Interface before initializing...",
             __PRETTY_FUNCTION__);
    sUcastClientInterface->Cleanup();
    sUcastClientInterface = nullptr;
  }
  sUcastClientInterface = bluetooth::bap::ucast::btif_bap_uclient_get_interface();

  if (sUcastClientInterface == nullptr) {
    LOG_ERROR(LOG_TAG, "%s Failed to get BAP Interface", __PRETTY_FUNCTION__);
    return BT_STATUS_FAIL;
  }
  char value[PROPERTY_VALUE_MAX];
  if(property_get("persist.vendor.service.bt.bap.conn_update", value, "false")
                     && !strcmp(value, "true")) {
    is_conn_update_enabled_ = true;
  } else {
    is_conn_update_enabled_ = false;
  }
  sUcastClientInterface->Init(&sUcastClientCallbacks);
  enabled_ = true;
  return BT_STATUS_SUCCESS;
}

void BtifAcmStateMachine::StateIdle::OnEnter() {
  BTIF_TRACE_DEBUG("%s: Peer %s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str());
  if(btif_acm_initiator.IsConnUpdateEnabled()) {
    if ((peer_.StateMachine().PreviousStateId() == BtifAcmStateMachine::kStateOpened) ||
       (peer_.StateMachine().PreviousStateId() == BtifAcmStateMachine::kStateStarted)) {
      if (alarm_is_scheduled(btif_acm_initiator.AcmConnIntervalTimer())) {
        btif_acm_check_and_cancel_conn_Interval_timer();
        peer_.SetConnUpdateMode(BtifAcmPeer::kFlagRelaxedMode);
      } else {
          LOG_ERROR(LOG_TAG, "%s Already in relaxed intervals",
                                             __PRETTY_FUNCTION__);
      }
    } else if (peer_.StateMachine().PreviousStateId() !=
                        BtifAcmStateMachine::kStateInvalid) {
      if (alarm_is_scheduled(btif_acm_initiator.AcmConnIntervalTimer())) {
        btif_acm_check_and_cancel_conn_Interval_timer();
      }
      peer_.SetConnUpdateMode(BtifAcmPeer::kFlagRelaxedMode);
    }
  }
  peer_.ClearConnUpdateMode();
  peer_.ClearAllFlags();
  peer_.SetProfileType(0);
  peer_.SetRcfgProfileType(0);
  peer_.SetPeerPacsState(ConnectionState::DISCONNECTED);

  // Delete peers that are re-entering the Idle state
  if (peer_.IsAcceptor()) {
    do_in_bta_thread(FROM_HERE, base::Bind(&BtifAcmInitiator::DeleteIdlePeers,
                                            base::Unretained(&btif_acm_initiator)));
  }
}

void BtifAcmStateMachine::StateIdle::OnExit() {
  BTIF_TRACE_DEBUG("%s: Peer %s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str());
}

bool BtifAcmStateMachine::StateIdle::ProcessEvent(uint32_t event,
                                                  void* p_data) {
  BTIF_TRACE_DEBUG("%s: Peer %s : event=%s flags=%s "
                   "music_active_peer=%s voice_active_peer=%s",
                   __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str(),
                   BtifAcmEvent::EventName(event).c_str(),
                   peer_.FlagsToString().c_str(),
                   logbool(peer_.IsPeerActiveForMusic()).c_str(),
                   logbool(peer_.IsPeerActiveForVoice()).c_str());

  switch (event) {
    case BTIF_ACM_STOP_STREAM_REQ_EVT: {
      tBTIF_ACM_STOP_REQ * p_bta_data = (tBTIF_ACM_STOP_REQ*)p_data;
      if(p_bta_data->wait_for_signal) {
        p_bta_data->peer_ready_promise->set_value();
      }
      LOG_ERROR(LOG_TAG, "%s BTIF_ACM_STOP_STREAM_REQ_EVT from BTIF ",
                                             __PRETTY_FUNCTION__);
    } break;

    case BTIF_ACM_SUSPEND_STREAM_REQ_EVT:
      break;

    case BTA_ACM_PACS_CONNECT_EVT: {
      tBTA_ACM_PACS_STATE_INFO* p_bta_data = (tBTA_ACM_PACS_STATE_INFO*)p_data;
      ConnectionState pacs_state = p_bta_data->state;
      if (pacs_state == ConnectionState::CONNECTING) {
        LOG_INFO(LOG_TAG, "%s: PACS connecting, ignore", __PRETTY_FUNCTION__);
      } else if (pacs_state == ConnectionState::CONNECTED) {
        peer_.SetPeerPacsState(pacs_state);
        LOG_INFO(LOG_TAG, "%s: PACS Connected, initiated PACS DIscovery",
                            __PRETTY_FUNCTION__);
        if (!sPacsClientInterface) break;
        sPacsClientInterface->StartDiscovery(pacs_client_id, peer_.PeerAddress());
      } else {
        LOG_INFO(LOG_TAG, "%s: PACS UNKNOWN state", __PRETTY_FUNCTION__);
      }
    } break;

    case BTA_ACM_PACS_DISCONNECT_EVT: {
      ConnectionState pacs_state = peer_.GetPeerPacsState();
      LOG_DEBUG(LOG_TAG, "%s: pacs_state: %d", __PRETTY_FUNCTION__, pacs_state);
      if(pacs_state == ConnectionState::DISCONNECTED) {
        LOG_DEBUG(LOG_TAG, "%s: PACS already disconnected ", __PRETTY_FUNCTION__);
        LOG_DEBUG(LOG_TAG, "%s: MusicTxState: %d, MusicRxState: %d,"
                           " VoiceTxState: %d, VoiceRxState: %d", __PRETTY_FUNCTION__,
                           peer_.GetPeerMusicTxState(), peer_.GetPeerMusicRxState(),
                           peer_.GetPeerVoiceTxState(), peer_.GetPeerVoiceRxState());
        if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
            peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
          btif_report_connection_state(peer_.PeerAddress(),
                                       BTACM_CONNECTION_STATE_DISCONNECTED,
                                       CONTEXT_TYPE_MUSIC);
        }

        if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED &&
            peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED) {
          btif_report_connection_state(peer_.PeerAddress(),
                                       BTACM_CONNECTION_STATE_DISCONNECTED,
                                       CONTEXT_TYPE_VOICE);
        }
      } else if(pacs_state == ConnectionState::CONNECTED) {
        if(sPacsClientInterface) {
          LOG_DEBUG(LOG_TAG, "%s: Issue PACS disconnect ", __PRETTY_FUNCTION__);
          sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
        }
      }
    } break;

    case BTIF_ACM_CONNECT_REQ_EVT: {
      //For below PTS cases set below prop to true
      //CAP/COM/ERR/BI-01-C
      //CAP/INI/ERR/BI-01-C
      char pts_lock[PROPERTY_VALUE_MAX] = "false";
      property_get("persist.vendor.btstack.pts_lock", pts_lock, "false");
      LOG_DEBUG(LOG_TAG, "%s: pts_lock: %s", __PRETTY_FUNCTION__, pts_lock);
      if (!strncmp("true", pts_lock, 4)) {
        LOG_DEBUG(LOG_TAG, "%s: Sleep for 20sec before PAC", __PRETTY_FUNCTION__);
        usleep(20000 * 1000);
        LOG_DEBUG(LOG_TAG, "%s: 20sec Sleep completed before PAC", __PRETTY_FUNCTION__);
      }

      ConnectionState pacs_state = peer_.GetPeerPacsState();
      LOG_INFO(LOG_TAG, "%s: BTIF_ACM_CONNECT_REQ_EVT, pacs_state=%d",
          __PRETTY_FUNCTION__, pacs_state);
      if (pacs_state == ConnectionState::DISCONNECTED ||
          pacs_state == ConnectionState::DISCONNECTING) {
        if (!sPacsClientInterface) break;
        sPacsClientInterface->Connect(pacs_client_id, peer_.PeerAddress(), true);
        peer_.SetPeerPacsState(ConnectionState::CONNECTING);
      }
    } break;

    case BTIF_ACM_DISCONNECT_REQ_EVT: {
      ConnectionState pacs_state = peer_.GetPeerPacsState();
      LOG_INFO(LOG_TAG, "%s: BTIF_ACM_DISCONNECT_REQ_EVT, pacs_state=%d",
          __PRETTY_FUNCTION__, pacs_state);
      if (pacs_state == ConnectionState::CONNECTING) {
        sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
        peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateIdle);
      }
    } break;

    case BTA_ACM_PACS_DISCOVERY_RES_EVT: {
      tBTA_ACM_PACS_DATA_INFO* pacs_discovery =  (tBTA_ACM_PACS_DATA_INFO *)p_data;
      if (pacs_discovery == nullptr || pacs_discovery->status) {
        LOG_INFO(LOG_TAG, "%s PACS Discovery failed or Empty PACS Discovery", __PRETTY_FUNCTION__);
        btif_report_connection_state(peer_.PeerAddress(),
                                     BTACM_CONNECTION_STATE_DISCONNECTED,
                                     peer_.GetContextType());
        break;
      }

      if (pacs_discovery->src_records.empty() &&
          pacs_discovery->sink_records.empty()) {
        ConnectionState pacs_state = peer_.GetPeerPacsState();
        LOG_DEBUG(LOG_TAG, "%s: pacs_state: %d", __PRETTY_FUNCTION__, pacs_state);
        if (pacs_state == ConnectionState::CONNECTED) {
          if(sPacsClientInterface) {
            LOG_DEBUG(LOG_TAG, "%s: Issue PACS disconnect ", __PRETTY_FUNCTION__);
            sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
            break;
          }
        }
      }
      peer_.SetPeerSrcPacsData(pacs_discovery->src_records);
      peer_.SetPeerSnkPacsData(pacs_discovery->sink_records);

      // prepare peer codec selectable capabilities
      std::vector<CodecConfig> snk_selectable_cap;
      for (auto local_snk_cap : unicast_codecs_capabilities_snk) {
        CodecConfig selectable_cap = {
                            CodecIndex::CODEC_INDEX_SOURCE_MAX,
                            CodecPriority::CODEC_PRIORITY_DEFAULT,
                            CodecSampleRate::CODEC_SAMPLE_RATE_NONE,
                            CodecBPS::CODEC_BITS_PER_SAMPLE_NONE,
                            CodecChannelMode::CODEC_CHANNEL_MODE_NONE,
                            0, 0, 0, 0};
        for (auto snk_pac : pacs_discovery->sink_records) {
          if (local_snk_cap.codec_type == snk_pac.codec_type) {
            selectable_cap.codec_type = snk_pac.codec_type;
            selectable_cap.codec_priority = local_snk_cap.codec_priority;
            if ((local_snk_cap.sample_rate & snk_pac.sample_rate) !=
                CodecSampleRate::CODEC_SAMPLE_RATE_NONE)
              selectable_cap.sample_rate |= snk_pac.sample_rate;
              selectable_cap.bits_per_sample = CodecBPS::CODEC_BITS_PER_SAMPLE_24;
            if ((local_snk_cap.channel_mode & snk_pac.channel_mode) !=
                CodecChannelMode::CODEC_CHANNEL_MODE_NONE)
              selectable_cap.channel_mode |= snk_pac.channel_mode;
          }
        }
        BTIF_TRACE_IMP("%s: selectable_snk sample rate: %d",
                       __func__, selectable_cap.sample_rate);
        if(selectable_cap.codec_type != CodecIndex::CODEC_INDEX_SOURCE_MAX)
          snk_selectable_cap.push_back(selectable_cap);
      }
      peer_.SetSnkSelectableCapability(snk_selectable_cap);

      std::vector<CodecConfig> src_selectable_cap;
      for (auto local_src_cap : unicast_codecs_capabilities_src) {
        CodecConfig selectable_cap = {
                            CodecIndex::CODEC_INDEX_SOURCE_MAX,
                            CodecPriority::CODEC_PRIORITY_DEFAULT,
                            CodecSampleRate::CODEC_SAMPLE_RATE_NONE,
                            CodecBPS::CODEC_BITS_PER_SAMPLE_NONE,
                            CodecChannelMode::CODEC_CHANNEL_MODE_NONE,
                            0, 0, 0, 0};
        for (auto src_pac : pacs_discovery->src_records) {
          if (local_src_cap.codec_type == src_pac.codec_type) {
            selectable_cap.codec_type = src_pac.codec_type;
            selectable_cap.codec_priority = local_src_cap.codec_priority;
            if ((local_src_cap.sample_rate & src_pac.sample_rate) !=
                CodecSampleRate::CODEC_SAMPLE_RATE_NONE)
              selectable_cap.sample_rate |= src_pac.sample_rate;
              selectable_cap.bits_per_sample = CodecBPS::CODEC_BITS_PER_SAMPLE_24;
            if ((local_src_cap.channel_mode & src_pac.channel_mode) !=
                CodecChannelMode::CODEC_CHANNEL_MODE_NONE)
              selectable_cap.channel_mode |= src_pac.channel_mode;
          }
        }
        BTIF_TRACE_IMP("%s: selectable_src sample rate: %d",
                       __func__, selectable_cap.sample_rate);
        if(selectable_cap.codec_type != CodecIndex::CODEC_INDEX_SOURCE_MAX)
          src_selectable_cap.push_back(selectable_cap);
      }
      peer_.SetSrcSelectableCapability(src_selectable_cap);

      tBTIF_ACM_CONN_DISC conn;
      conn.contextType = peer_.GetContextType();
      conn.profileType = peer_.GetProfileType();
      conn.bd_addr = peer_.PeerAddress();
      LOG_INFO(LOG_TAG, "%s PACS Discovery completed,"
                        " initiate BAP connect request", __PRETTY_FUNCTION__);
      peer_.StateMachine().ProcessEvent(BTIF_ACM_BAP_CONNECT_REQ_EVT, &conn);
    } break;

    case BTIF_ACM_BAP_CONNECT_REQ_EVT: {
      tBTIF_ACM_CONN_DISC* p_bta_data = (tBTIF_ACM_CONN_DISC*)p_data;
      bool can_connect = true;

      // Check whether connection is allowed
      if (peer_.IsAcceptor()) {
        //There is no char in current spec. Should we check VMCP role here?
        // shall we assume VMCP role would have been checked in apps and
        // no need to check here?
        can_connect = btif_acm_initiator.AllowedToConnect(peer_.PeerAddress());
        if (!can_connect) disconnect_acm_initiator(peer_.PeerAddress(),
                                                   p_bta_data->contextType);
      }

      if (!can_connect) {
        BTIF_TRACE_ERROR(
            "%s: Cannot connect to peer %s: too many connected "
            "peers",
            __PRETTY_FUNCTION__, peer_.PeerAddress().ToString().c_str());
        break;
      }

      BTIF_TRACE_ERROR("%s: contextType: %d, peer_.GetProfileType(): %d",
                        __PRETTY_FUNCTION__, p_bta_data->contextType,
                        peer_.GetProfileType());

      std::vector<StreamConnect> streams;
      if (p_bta_data->contextType == CONTEXT_TYPE_MUSIC) {
        StreamConnect conn_media;
        if (peer_.GetProfileType() & (BAP|GCP)) {
          bool is_m_tx_fetched = false;
          //keeping media tx as BAP/GCP config
          memset(&conn_media, 0, sizeof(conn_media));
          is_m_tx_fetched = fetch_media_tx_codec_qos_config(peer_.PeerAddress(),
                                          peer_.GetProfileType() & (BAP|GCP),
                                          &conn_media);
          BTIF_TRACE_ERROR("%s: is_m_tx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_m_tx_fetched);
          if (is_m_tx_fetched == false) {
            BTIF_TRACE_ERROR("%s: media tx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_media);
          }
        }

        if (peer_.GetProfileType() & WMCP) {
          bool is_m_rx_fetched = false;
          //keeping media rx as WMCP config
          memset(&conn_media, 0, sizeof(conn_media));
          is_m_rx_fetched = fetch_media_rx_codec_qos_config(peer_.PeerAddress(),
                                                            WMCP, &conn_media);
          BTIF_TRACE_ERROR("%s: is_m_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_m_rx_fetched);
          if (is_m_rx_fetched == false) {
            BTIF_TRACE_ERROR("%s: media rx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_media);
          }
        } else if (peer_.GetProfileType() & GCP_RX) {
          bool is_game_rx_fetched = false;
          //keeping media rx as GCP_RX config
          memset(&conn_media, 0, sizeof(conn_media));
          is_game_rx_fetched = fetch_media_rx_codec_qos_config(peer_.PeerAddress(),
                                                            GCP_RX, &conn_media);
          BTIF_TRACE_ERROR("%s: is_game_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_game_rx_fetched);
          if (is_game_rx_fetched == false) {
            BTIF_TRACE_ERROR("%s: game rx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_media);
          }
        }
      } else if (p_bta_data->contextType == CONTEXT_TYPE_MUSIC_VOICE) {
        StreamConnect conn_media, conn_voice;
        bool is_vm_v_tx_fetched = false;
        //keeping voice tx as BAP config
        memset(&conn_voice, 0, sizeof(conn_voice));
        is_vm_v_tx_fetched = fetch_voice_tx_codec_qos_config(peer_.PeerAddress(),
                                                             BAP, &conn_voice);
        BTIF_TRACE_ERROR("%s: is_vm_v_tx_fetched: %d",
                                 __PRETTY_FUNCTION__, is_vm_v_tx_fetched);
        if (is_vm_v_tx_fetched == false) {
          BTIF_TRACE_ERROR("%s: vm voice tx config not fetched", __PRETTY_FUNCTION__);
        } else {
          streams.push_back(conn_voice);
        }

        bool is_vm_v_rx_fetched = false;
        //keeping voice rx as BAP config
        memset(&conn_voice, 0, sizeof(conn_voice));
        is_vm_v_rx_fetched = fetch_voice_rx_codec_qos_config(peer_.PeerAddress(),
                                                             BAP, &conn_voice);
        BTIF_TRACE_ERROR("%s: is_vm_v_rx_fetched: %d",
                                 __PRETTY_FUNCTION__, is_vm_v_rx_fetched);
        if (is_vm_v_rx_fetched == false) {
          BTIF_TRACE_ERROR("%s: vm voice rx config not fetched", __PRETTY_FUNCTION__);
        } else {
          streams.push_back(conn_voice);
        }

        if (peer_.GetProfileType() & (BAP|GCP)) {
          //keeping media tx as BAP/GCP config
          bool is_vm_m_tx_fetched = false;
          memset(&conn_media, 0, sizeof(conn_media));
          is_vm_m_tx_fetched = fetch_media_tx_codec_qos_config(peer_.PeerAddress(),
                                          peer_.GetProfileType() & (BAP|GCP),
                                          &conn_media);
          BTIF_TRACE_ERROR("%s: is_vm_m_tx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_vm_m_tx_fetched);
          if (is_vm_m_tx_fetched == false) {
            BTIF_TRACE_ERROR("%s: vm media tx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_media);
          }
        }

        if (peer_.GetProfileType() & WMCP) {
          bool is_vm_m_rx_fetched = false;
          //keeping media rx as WMCP config
          memset(&conn_media, 0, sizeof(conn_media));
          is_vm_m_rx_fetched = fetch_media_rx_codec_qos_config(peer_.PeerAddress(), WMCP, &conn_media);
          BTIF_TRACE_ERROR("%s: is_vm_m_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_vm_m_rx_fetched);
          if (is_vm_m_rx_fetched == false) {
            BTIF_TRACE_ERROR("%s: vm media rx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_media);
          }
        } else if (peer_.GetProfileType() & GCP_RX) {
          bool is_vm_game_rx_fetched = false;
          //keeping media rx as GCP_RX config
          memset(&conn_media, 0, sizeof(conn_media));
          is_vm_game_rx_fetched = fetch_media_rx_codec_qos_config(peer_.PeerAddress(),
                                                               GCP_RX, &conn_media);
          BTIF_TRACE_ERROR("%s: is_vm_game_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_vm_game_rx_fetched);
          if (is_vm_game_rx_fetched == false) {
            BTIF_TRACE_ERROR("%s: vm game rx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_media);
          }
        }
      } else if (p_bta_data->contextType == CONTEXT_TYPE_VOICE) {
        StreamConnect conn_voice;
        //keeping voice tx as BAP config
        uint16_t num_sink_ases, num_src_ases;
        btif_bap_get_params(peer_.PeerAddress(), nullptr, nullptr, &num_sink_ases,
                            &num_src_ases, nullptr, nullptr);
        if(num_sink_ases) {
          bool is_v_tx_fetched = false;
          memset(&conn_voice, 0, sizeof(conn_voice));
          is_v_tx_fetched = fetch_voice_tx_codec_qos_config(peer_.PeerAddress(),
                                                            BAP, &conn_voice);
          BTIF_TRACE_ERROR("%s: is_v_tx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_v_tx_fetched);
          if (is_v_tx_fetched == false) {
            BTIF_TRACE_ERROR("%s: voice tx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_voice);
          }
        }
        //keeping voice rx as BAP config
        if(num_src_ases) {
          bool is_v_rx_fetched = false;
          memset(&conn_voice, 0, sizeof(conn_voice));
          is_v_rx_fetched = fetch_voice_rx_codec_qos_config(peer_.PeerAddress(),
                                                               BAP, &conn_voice);
          BTIF_TRACE_ERROR("%s: is_v_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_v_rx_fetched);
          if (is_v_rx_fetched == false) {
            BTIF_TRACE_ERROR("%s: voice rx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_voice);
          }
        }
      }

      LOG(WARNING) << __func__ << ": size of streams: " << streams.size();
      if (!sUcastClientInterface) break;
      if (streams.size() == 0) {
        ConnectionState pacs_state = peer_.GetPeerPacsState();
        LOG_DEBUG(LOG_TAG, "%s: pacs_state: %d", __PRETTY_FUNCTION__, pacs_state);

        //For 9(i) support, only SRC direcion.
        char acm_pts[PROPERTY_VALUE_MAX] = "false";
        property_get("persist.vendor.btstack.acm_pts", acm_pts, "false");
        LOG_DEBUG(LOG_TAG, "%s: acm_pts: %s", __PRETTY_FUNCTION__, acm_pts);

        if (strncmp("true", acm_pts, 4) &&
            pacs_state == ConnectionState::CONNECTED) {
          if(sPacsClientInterface) {
            LOG_DEBUG(LOG_TAG, "%s: Issue PACS disconnect ", __PRETTY_FUNCTION__);
            sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
          }
        }
        break;
      }
      // intiate background connection
      std::vector<RawAddress> address;
      address.push_back(peer_.PeerAddress());
      sUcastClientInterface->Connect(address, true, streams);
      peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpening);
    } break;

    case BTA_ACM_CONN_UPDATE_TIMEOUT_EVT:
      peer_.SetConnUpdateMode(BtifAcmPeer::kFlagRelaxedMode);
      break;

    default:
      BTIF_TRACE_WARNING("%s: Peer %s : Unhandled event=%s",
                         __PRETTY_FUNCTION__,
                         peer_.PeerAddress().ToString().c_str(),
                         BtifAcmEvent::EventName(event).c_str());
      return false;
  }

  return true;
}

void BtifAcmStateMachine::StateOpening::OnEnter() {
  BTIF_TRACE_DEBUG("%s: Peer %s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str());

  if(btif_acm_initiator.IsConnUpdateEnabled()) {
    //Cancel the timer if start streamng comes before
    // 5 seconds while moving the interval to relaxed mode.
    if (alarm_is_scheduled(btif_acm_initiator.AcmConnIntervalTimer())) {
       btif_acm_check_and_cancel_conn_Interval_timer();
    }
    else {
       peer_.SetConnUpdateMode(BtifAcmPeer::kFlagAggresiveMode);
    }
  }

}

void BtifAcmStateMachine::StateOpening::OnExit() {
  BTIF_TRACE_DEBUG("%s: Peer %s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str());
}

bool BtifAcmStateMachine::StateOpening::ProcessEvent(uint32_t event,
                                                     void* p_data) {
  BTIF_TRACE_DEBUG("%s: Peer %s : event=%s flags=%s "
                   "music_active_peer=%s voice_active_peer=%s",
                   __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str(),
                   BtifAcmEvent::EventName(event).c_str(),
                   peer_.FlagsToString().c_str(),
                   logbool(peer_.IsPeerActiveForMusic()).c_str(),
                   logbool(peer_.IsPeerActiveForVoice()).c_str());

  switch (event) {
    case BTIF_ACM_STOP_STREAM_REQ_EVT: {
      tBTIF_ACM_STOP_REQ * p_bta_data = (tBTIF_ACM_STOP_REQ*)p_data;
      if(p_bta_data->wait_for_signal) {
        p_bta_data->peer_ready_promise->set_value();
      }
      LOG_ERROR(LOG_TAG, "%s BTIF_ACM_STOP_STREAM_REQ_EVT from BTIF ",
                                             __PRETTY_FUNCTION__);
    } break;
    case BTIF_ACM_SUSPEND_STREAM_REQ_EVT:
      break;  // Ignore

    case BTA_ACM_CONNECT_EVT: {
      tBTIF_ACM* p_bta_data = (tBTIF_ACM*)p_data;
      btacm_connection_state_t state;
      uint8_t status = (uint8_t)p_bta_data->state_info.stream_state;
      uint16_t contextType = p_bta_data->state_info.stream_type.type;

      LOG_INFO(
          LOG_TAG, "%s: Peer %s : event=%s flags=%s status=%d contextType=%d",
          __PRETTY_FUNCTION__, peer_.PeerAddress().ToString().c_str(),
          BtifAcmEvent::EventName(event).c_str(), peer_.FlagsToString().c_str(),
          status, contextType);

      //Below property need set for TMAP PTS TCs of config B and C if required.
      char auto_start[PROPERTY_VALUE_MAX] = "false";
      property_get("persist.vendor.btstack.pts_auto_start_after_conn", auto_start, "false");
      LOG_DEBUG(LOG_TAG, "%s: auto_start: %s", __PRETTY_FUNCTION__, auto_start);

      if (contextType == CONTENT_TYPE_MEDIA) {
        if (p_bta_data->state_info.stream_state == StreamState::CONNECTED) {
          state = BTACM_CONNECTION_STATE_CONNECTED;
          // Report the connection state to the application
          btif_report_connection_state(peer_.PeerAddress(), state, CONTEXT_TYPE_MUSIC);
          if (p_bta_data->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerMusicTxState(p_bta_data->state_info.stream_state);
            BTIF_TRACE_DEBUG("%s: received connected state from BAP for mediaTx, "
                             "move in opened state", __func__);
          } else if (p_bta_data->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerMusicRxState(p_bta_data->state_info.stream_state);
            BTIF_TRACE_DEBUG("%s: received connected state from BAP for mediaRx, "
                             "move in opened state", __func__);
          }
          // Change state to OPENED
          peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);

          if (!strncmp("true", auto_start, 4)) {
            reconfig_acm_initiator(peer_.PeerAddress(), BAP, false);
            peer_.SetFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
          }
        } else if (p_bta_data->state_info.stream_state == StreamState::CONNECTING) {
          BTIF_TRACE_DEBUG("%s: received connecting state from BAP for "
                                         "MEDIA Tx or Rx, ignore", __func__);
          if (p_bta_data->state_info.stream_type.direction == ASE_DIRECTION_SINK)
            peer_.SetPeerMusicTxState(p_bta_data->state_info.stream_state);
          else if (p_bta_data->state_info.stream_type.direction == ASE_DIRECTION_SRC)
            peer_.SetPeerMusicRxState(p_bta_data->state_info.stream_state);
        }
      } else if (contextType == CONTENT_TYPE_CONVERSATIONAL) {
        if (p_bta_data->state_info.stream_state == StreamState::CONNECTED) {
          state = BTACM_CONNECTION_STATE_CONNECTED;
          uint16_t num_sink_ases, num_src_ases;
          btif_bap_get_params(peer_.PeerAddress(), nullptr, nullptr, &num_sink_ases,
                              &num_src_ases, nullptr, nullptr);
          if (p_bta_data->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerVoiceTxState(p_bta_data->state_info.stream_state);
            if (peer_.GetPeerVoiceRxState() == StreamState::CONNECTED || num_src_ases == 0) {
              btif_report_connection_state(peer_.PeerAddress(), state, CONTEXT_TYPE_VOICE);
              BTIF_TRACE_DEBUG("%s: received connected state from BAP for voice Tx, "
                               "move in opened state", __func__);
              peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
            }
          } else if (p_bta_data->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerVoiceRxState(p_bta_data->state_info.stream_state);
            if (peer_.GetPeerVoiceTxState() == StreamState::CONNECTED || num_sink_ases == 0) {
              btif_report_connection_state(peer_.PeerAddress(), state, CONTEXT_TYPE_VOICE);
              BTIF_TRACE_DEBUG("%s: received connected state from BAP for voice Rx, "
                               "move in opened state", __func__);
              peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
            }
          }

          if (!strncmp("true", auto_start, 4)) {
            reconfig_acm_initiator(peer_.PeerAddress(), BAP_CALL, false);
            peer_.SetFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
          }
        } else if (p_bta_data->state_info.stream_state == StreamState::CONNECTING) {
          BTIF_TRACE_DEBUG("%s: received connecting state from BAP for "
                            "CONVERSATIONAL Tx or Rx, ignore", __func__);
          if (p_bta_data->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerVoiceTxState(p_bta_data->state_info.stream_state);
          } else if (p_bta_data->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerVoiceRxState(p_bta_data->state_info.stream_state);
          }
        }
      }
    } break;

    case BTA_ACM_DISCONNECT_EVT: {
      tBTIF_ACM* p_acm = (tBTIF_ACM*)p_data;
      int context_type = p_acm->state_info.stream_type.type;
      if (p_acm->state_info.stream_state == StreamState::DISCONNECTED) {
        if (context_type == CONTENT_TYPE_MEDIA) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
          }
          if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
            BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnected state from BAP"
                      " when Voice Tx+Rx & Media Rx/Tx was disconnected, "
                      "send pacs disconnect", __func__);
            if (!sPacsClientInterface) break;
            sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
            peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
          } else {
            BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnected state from BAP"
                      " when either Voice Tx or Rx or Media Rx/Tx is connecting, "
                      "remain in opening state", __func__);
          }
        } else if (context_type == CONTENT_TYPE_CONVERSATIONAL) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED) {
              if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
                  peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Tx,"
                                 " voice Rx, music Tx+Rx are disconnected "
                                 "send pacs disconnect", __func__);
                if (!sPacsClientInterface) break;
                sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else if (peer_.GetPeerMusicTxState() == StreamState::CONNECTING ||
                         peer_.GetPeerMusicRxState() == StreamState::CONNECTING) {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Tx,"
                                 " voice Rx is disconnected but either music Tx or "
                                 "Rx still connecting, remain in opening state", __func__);
              }
            }
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED) {
              if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
                  peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                 " voice Tx, music Tx+Rx are disconnected "
                                 "move in idle state", __func__);
                if (!sPacsClientInterface) break;
                sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else if (peer_.GetPeerMusicTxState() == StreamState::CONNECTING ||
                         peer_.GetPeerMusicRxState() == StreamState::CONNECTING) {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                 " voice Tx is disconnected but music Tx or "
                                 "Rx still connecting, remain in opening state", __func__);
              }
            }
          }
        }
      } else if (p_acm->state_info.stream_state == StreamState::DISCONNECTING) {
          if (context_type == CONTENT_TYPE_MEDIA) {
            if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
              peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
            } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
              peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
            }
            btif_report_connection_state(peer_.PeerAddress(),
                                         BTACM_CONNECTION_STATE_DISCONNECTING,
                                         CONTEXT_TYPE_MUSIC);
            if ((peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED ||
                  peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTING) &&
                 (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED ||
                  peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTING) &&
                 (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED ||
                  peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING) &&
                 (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED ||
                  peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING)) {
              BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnecting state from BAP"
                               " when Voice Tx+Rx and Media Rx/Tx disconnected/ing, "
                               "move in closing state", __func__);
              peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
            } else {
              BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnecting state from BAP"
                               " when either Voice Tx or Rx or Media Rx/Tx is connected, "
                               "remain in opening state", __func__);
            }
          } else if (context_type == CONTENT_TYPE_CONVERSATIONAL) {
            if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
              peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
            } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
              peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
            }
            btif_report_connection_state(peer_.PeerAddress(),
                                         BTACM_CONNECTION_STATE_DISCONNECTING,
                                         CONTEXT_TYPE_VOICE);
            if ((peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED ||
                  peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTING) &&
                 (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED ||
                  peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTING) &&
                 (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED ||
                  peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING) &&
                 (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED ||
                  peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING)) {
              BTIF_TRACE_DEBUG("%s: received disconnecting state from BAP for voice Tx,"
                               " voice Rx, music Tx+Rx are disconnected/ing "
                               "move in closing state", __func__);
              peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
            } else {
              BTIF_TRACE_DEBUG("%s: received disconnecting state from BAP for voice Tx,"
                               " voice Rx is disconncted/ing but music Tx or "
                               "Rx still not disconnected/ing,"
                               " remain in opening state", __func__);
            }
          }
      }
    }
    break;

    case BTIF_ACM_DISCONNECT_REQ_EVT:{
      tBTIF_ACM_CONN_DISC* p_bta_data = (tBTIF_ACM_CONN_DISC*)p_data;
      std::vector<StreamType> disconnect_streams;
      if (p_bta_data->contextType == CONTEXT_TYPE_MUSIC) {
        StreamType type_1;
        if (p_bta_data->profileType & BAP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_MEDIA,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & WMCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_LIVE,
                    .direction = ASE_DIRECTION_SRC
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & GCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        StreamType type_2;
        if (p_bta_data->profileType & GCP_RX) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SRC
                   };
          type_2 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
          disconnect_streams.push_back(type_2);
        }
      } else if (p_bta_data->contextType == CONTEXT_TYPE_MUSIC_VOICE) {
        StreamType type_2 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SRC
                            };
        StreamType type_3 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SINK
                            };
        disconnect_streams.push_back(type_3);
        disconnect_streams.push_back(type_2);

        StreamType type_1;
        if (p_bta_data->profileType & BAP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_MEDIA,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & WMCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_LIVE,
                    .direction = ASE_DIRECTION_SRC
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & GCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        StreamType type_4;
        if (p_bta_data->profileType & GCP_RX) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SRC
                   };
          type_4 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
          disconnect_streams.push_back(type_4);
        }
      } else if (p_bta_data->contextType == CONTEXT_TYPE_VOICE) {
        StreamType type_2 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SRC
                            };
        StreamType type_3 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SINK
                            };
        disconnect_streams.push_back(type_3);
        disconnect_streams.push_back(type_2);
      }
      LOG(WARNING) << __func__
                   << " size of disconnect_streams " << disconnect_streams.size();
      if (!sUcastClientInterface) break;
      sUcastClientInterface->Disconnect(peer_.PeerAddress(), disconnect_streams);

      if ((p_bta_data->contextType == CONTEXT_TYPE_MUSIC) &&
          ((peer_.GetPeerVoiceRxState() == StreamState::CONNECTING) ||
           (peer_.GetPeerVoiceTxState() == StreamState::CONNECTING))) {
        LOG(WARNING) << __func__ << " voice connecting remain in opening ";
        btif_report_connection_state(peer_.PeerAddress(),
                                     BTACM_CONNECTION_STATE_DISCONNECTED,
                                     CONTEXT_TYPE_MUSIC);
      } else if ((p_bta_data->contextType == CONTEXT_TYPE_VOICE) &&
                 (peer_.GetPeerMusicTxState() == StreamState::CONNECTING ||
                  (peer_.GetPeerMusicRxState() == StreamState::CONNECTING))) {
        LOG(WARNING) << __func__ << " Music connecting remain in opening ";
        btif_report_connection_state(peer_.PeerAddress(),
                                     BTACM_CONNECTION_STATE_DISCONNECTED,
                                     CONTEXT_TYPE_VOICE);
      } else {
        LOG(WARNING) << __func__ << " Move in closing state ";
        peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
      }
    }
    break;

    case BTIF_ACM_CONNECT_REQ_EVT: {
      BTIF_TRACE_WARNING(
          "%s: Peer %s : event=%s : device is already connecting, "
          "ignore Connect request",
          __PRETTY_FUNCTION__, peer_.PeerAddress().ToString().c_str(),
          BtifAcmEvent::EventName(event).c_str());
    } break;

    case BTA_ACM_CONFIG_EVT: {
       tBTIF_ACM* p_acm_data = (tBTIF_ACM*)p_data;
       uint16_t contextType = p_acm_data->state_info.stream_type.type;
       uint16_t peer_latency_ms = 0;
       uint32_t presen_delay = 0;
       bool is_update_require = false;
       uint32_t audio_location = p_acm_data->config_info.audio_location;
       BTIF_TRACE_DEBUG("%s: audio location is %d", __PRETTY_FUNCTION__, audio_location);
       if (p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
         peer_.set_peer_audio_src_location(audio_location);
       } else if (p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
         peer_.set_peer_audio_sink_location(audio_location);
       }
       if (contextType == CONTENT_TYPE_MEDIA) {
         if (p_acm_data->state_info.stream_type.audio_context == CONTENT_TYPE_MEDIA) {
           BTIF_TRACE_DEBUG("%s: compare with current media config", __PRETTY_FUNCTION__);
           CodecConfig peer_config = peer_.get_peer_media_codec_config();
           is_update_require = compare_codec_config_(peer_config,
                                                     p_acm_data->config_info.codec_config);
         } else if (p_acm_data->state_info.stream_type.audio_context == CONTENT_TYPE_LIVE) {
           BTIF_TRACE_DEBUG("%s: cache current_recording_config", __PRETTY_FUNCTION__);
           current_recording_config = p_acm_data->config_info.codec_config;
         } else if (p_acm_data->state_info.stream_type.audio_context == CONTENT_TYPE_GAME) {
          //TODO
           if (p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
             BTIF_TRACE_DEBUG("%s: compare current gaming Tx config", __PRETTY_FUNCTION__);
             is_update_require = compare_codec_config_(current_media_config,
                                                       p_acm_data->config_info.codec_config);
           } else if (p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SRC){
             BTIF_TRACE_DEBUG("%s: cache current gaming Rx config", __PRETTY_FUNCTION__);
             current_gaming_rx_config = p_acm_data->config_info.codec_config;
           }
         }

         if (mandatory_codec_selected) {
           BTIF_TRACE_DEBUG("%s: Mandatory codec selected, do not store config", __PRETTY_FUNCTION__);
         } else {
           BTIF_TRACE_DEBUG("%s: store configuration", __PRETTY_FUNCTION__);
         }

         presen_delay = static_cast<uint32_t>(p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[0]) |
                        static_cast<uint32_t>(p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[1] << 8) |
                        static_cast<uint32_t>(p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[2] << 16);
         BTIF_TRACE_DEBUG("%s: presen_delay = %dus", __func__, presen_delay);
         peer_latency_ms = presen_delay/1000;
         peer_.peer_negotiated_pd_ = peer_latency_ms;
         //Cache the peer latency in WMCP case
         if (p_acm_data->state_info.stream_type.audio_context == CONTENT_TYPE_LIVE) {
           BTIF_TRACE_DEBUG("%s: presentation delay[0] = %x", __func__,
                            p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[0]);
           BTIF_TRACE_DEBUG("%s: presentation delay[1] = %x", __func__,
                            p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[1]);
           BTIF_TRACE_DEBUG("%s: presentation delay[2] = %x", __func__,
                            p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[2]);
           BTIF_TRACE_DEBUG("%s: s_to_m latency = %dms", __func__,
                           p_acm_data->config_info.qos_config.cig_config.max_tport_latency_s_to_m);
           peer_latency_ms += p_acm_data->config_info.qos_config.cig_config.max_tport_latency_s_to_m;
           peer_.SetPeerLatency(peer_latency_ms);
           BTIF_TRACE_DEBUG("%s: cached peer Latency = %dms", __func__, peer_.GetPeerLatency());
         }

         BTIF_TRACE_DEBUG("%s: is_update_require: %d",
                                        __PRETTY_FUNCTION__, is_update_require);
         if (is_update_require) {
           current_media_config = p_acm_data->config_info.codec_config;
           peer_.set_peer_media_codec_config(current_media_config);
           BTIF_TRACE_DEBUG("%s: current_media_config.codec_specific_3: %"
                                 PRIi64, __func__, current_media_config.codec_specific_3);

           btif_acm_report_source_codec_state(peer_.PeerAddress(), current_media_config,
                                              unicast_codecs_capabilities_snk,
                                              peer_.GetSnkSelectableCapability(), CONTEXT_TYPE_MUSIC);
         }
       } else if (contextType == CONTENT_TYPE_CONVERSATIONAL &&
                  p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
         BTIF_TRACE_DEBUG("%s: cache current_voice_config", __PRETTY_FUNCTION__);
         current_voice_config = p_acm_data->config_info.codec_config;
         BTIF_TRACE_DEBUG("%s: current_voice_config.codec_specific_3: %"
                               PRIi64, __func__, current_voice_config.codec_specific_3);
         btif_acm_update_lc3q_params(&current_voice_config.codec_specific_3, p_acm_data); //calling for voice context only
         btif_acm_report_source_codec_state(peer_.PeerAddress(), current_voice_config,
                                            unicast_codecs_capabilities_src,
                                            peer_.GetSrcSelectableCapability(), CONTEXT_TYPE_VOICE);
       }
      //Handle BAP START if reconfig comes in mid of streaming
      //peer_.SetStreamReconfigInfo(p_acm->acm_reconfig);
      //TODO: local capabilities
      //CodecConfig record = p_bta_data->acm_reconfig.codec_config;
      //saving codec config as negotiated parameter as true
      //btif_pacs_add_record(peer_.PeerAddress(), true, CodecDirection::CODEC_DIR_SRC, &record);

    } break;

    case BTA_ACM_CONN_UPDATE_TIMEOUT_EVT:
      peer_.SetConnUpdateMode(BtifAcmPeer::kFlagRelaxedMode);
      break;

    default:
      BTIF_TRACE_WARNING("%s: Peer %s : Unhandled event=%s",
                         __PRETTY_FUNCTION__,
                         peer_.PeerAddress().ToString().c_str(),
                         BtifAcmEvent::EventName(event).c_str());
      return false;
  }
  return true;
}

BtifAcmPeer *btif_get_peer_device(BtifAcmPeer *dev) {
    BtifAcmPeer *peer_dev = nullptr;
    tBTA_CSIP_CSET cset_info;
    memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
    cset_info = BTA_CsipGetCoordinatedSet(dev->SetId());
    if (cset_info.size == 0) {
      BTIF_TRACE_ERROR("%s: CSET info size is zero, return", __func__);
      return peer_dev;
    }
    std::vector<RawAddress>::iterator itr;
    BTIF_TRACE_DEBUG("%s: size of set members %d", __func__, (cset_info.set_members).size());
    if ((cset_info.set_members).size() > 0) {
      for (itr =(cset_info.set_members).begin(); itr != (cset_info.set_members).end(); itr++) {
        BtifAcmPeer* peer = btif_acm_initiator.FindPeer(*itr);
        if (peer != nullptr && peer->PeerAddress() != dev->PeerAddress()) {
          peer_dev = peer;
          break;
        }
      }
    }
    return peer_dev;
}

int btif_get_peer_streaming_context_type(uint8_t Id) {
    int context_type = 0;
    if (Id < INVALID_SET_ID) {
      tBTA_CSIP_CSET cset_info;
      memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
      cset_info = BTA_CsipGetCoordinatedSet(Id);
      if (cset_info.size == 0) {
        BTIF_TRACE_ERROR("%s: CSET info size is zero, return", __func__);
        return 0;
      }
      std::vector<RawAddress>::iterator itr;
      BTIF_TRACE_DEBUG("%s: size of set members %d", __func__, (cset_info.set_members).size());
      if ((cset_info.set_members).size() > 0) {
        for (itr =(cset_info.set_members).begin(); itr != (cset_info.set_members).end(); itr++) {
          BtifAcmPeer* peer = btif_acm_initiator.FindPeer(*itr);
          if (peer != nullptr && (peer->IsStreaming() ||
              peer->CheckFlags(BtifAcmPeer::kFlagPendingStart)) &&
              !peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend) &&
              !peer->CheckFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend)) {
            context_type = peer->GetStreamContextType();
            BTIF_TRACE_DEBUG("%s: fellow device %s is streaming for context type: %d ", __func__,
                              peer->PeerAddress().ToString().c_str(), context_type);
            break;
          }
        }
      }
    } else {
      context_type = 0;
      BTIF_TRACE_ERROR("%s: peer is TWM device, return context type %d", __func__, context_type);
    }
    return context_type;
}

bool btif_peer_device_is_streaming(uint8_t Id) {
    bool is_streaming = false;
    tBTA_CSIP_CSET cset_info;
    memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
    cset_info = BTA_CsipGetCoordinatedSet(Id);
    if (cset_info.size == 0) {
      BTIF_TRACE_ERROR("%s: CSET info size is zero, return", __func__);
      return false;
    }
    std::vector<RawAddress>::iterator itr;
    BTIF_TRACE_DEBUG("%s: size of set members %d",
                              __func__, (cset_info.set_members).size());
    if ((cset_info.set_members).size() > 0) {
      for (itr =(cset_info.set_members).begin();
                         itr != (cset_info.set_members).end(); itr++) {
        BtifAcmPeer* peer = btif_acm_initiator.FindPeer(*itr);
        if (peer != nullptr && (peer->IsStreaming() ||
            peer->CheckFlags(BtifAcmPeer::kFlagPendingStart)) &&
            !peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend) &&
            !peer->CheckFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend)) {
          BTIF_TRACE_DEBUG("%s: fellow device is streaming %s ",
                             __func__, peer->PeerAddress().ToString().c_str());
          is_streaming = true;
          break;
        }
      }
    }
    return is_streaming;
}

bool btif_peer_device_is_reconfiguring(uint8_t Id) {
    bool is_reconfigured = false;
    if (Id < INVALID_SET_ID) {
      tBTA_CSIP_CSET cset_info;
      memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
      cset_info = BTA_CsipGetCoordinatedSet(Id);
      if (cset_info.size == 0) {
        BTIF_TRACE_ERROR("%s: CSET info size is zero, return", __func__);
        return false;
      }
      std::vector<RawAddress>::iterator itr;
      BTIF_TRACE_DEBUG("%s: size of set members %d",
                             __func__, (cset_info.set_members).size());
      if ((cset_info.set_members).size() > 0) {
        for (itr =(cset_info.set_members).begin();
                                  itr != (cset_info.set_members).end(); itr++) {
          BtifAcmPeer* peer = btif_acm_initiator.FindPeer(*itr);
          if (peer != nullptr && peer->CheckFlags(BtifAcmPeer::kFlagPendingReconfigure)) {
            BTIF_TRACE_DEBUG("%s: peer is reconfiguring %s ",
                             __func__, peer->PeerAddress().ToString().c_str());
            is_reconfigured = true;
            break;
          }
        }
      }
    } else {
      is_reconfigured = true;
      BTIF_TRACE_ERROR("%s: peer is TWM device, return is_reconfigured %d",
                                                  __func__, is_reconfigured);
    }
    return is_reconfigured;
}

void BtifAcmStateMachine::StateOpened::OnEnter() {
  BTIF_TRACE_DEBUG("%s: Peer %s, Peer SetId = %d, MusicActiveSetId = %d, "
                   "ContextType = %d, active_context_type: %d, "
                   "old_active_profile_type: %d", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str(), peer_.SetId(),
                   btif_acm_initiator.MusicActiveCSetId(),
                   peer_.GetContextType(), active_context_type,
                   old_active_profile_type);

  //Starting the timer for 5 seconds before moving to relaxed state as
  //stop event or start streaming event moght immediately come
  //which requires aggresive interval
  if(btif_acm_initiator.IsConnUpdateEnabled()) {
    btif_acm_check_and_start_conn_Interval_timer(&peer_);
  }

  peer_.ClearFlags(BtifAcmPeer::kFlagPendingLocalSuspend |
                   BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend |
                   BtifAcmPeer::kFlagPendingStart |
                   BtifAcmPeer::kFlagPendingStop);

  BTIF_TRACE_DEBUG("%s: kFlagPendingReconfigure: %d, "
                   "kFLagPendingStartAfterReconfig: %d, "
                   "kFlagCallAndMusicSync: %d, "
                   "kFlagMediaSubUsecaseSync: %d "
                   "kFlagPendingAlsAndMetadataSync: %d ", __PRETTY_FUNCTION__,
          peer_.CheckFlags(BtifAcmPeer::kFlagPendingReconfigure),
          peer_.CheckFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig),
          peer_.CheckFlags(BtifAcmPeer::kFlagCallAndMusicSync),
          peer_.CheckFlags(BtifAcmPeer::kFlagMediaSubUsecaseSync),
          peer_.CheckFlags(BtifAcmPeer::kFlagPendingAlsAndMetadataSync));

  if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingReconfigure)) {
    BTIF_TRACE_DEBUG("%s: GetRcfgProfileType(): %d,"
                      " current_active_profile_type: %d",
                     __PRETTY_FUNCTION__, peer_.GetRcfgProfileType(),
                     current_active_profile_type);

    if ((peer_.GetRcfgProfileType() != BAP_CALL) &&
        (current_active_profile_type != peer_.GetRcfgProfileType())) {

      current_active_profile_type = peer_.GetRcfgProfileType();

      //TODO for gaming VBC
      BTIF_TRACE_DEBUG("%s: set current_active_profile_type: %d",
                           __PRETTY_FUNCTION__, current_active_profile_type);

      if (current_active_profile_type == GCP_RX) {
        current_active_config = current_media_config;
        print_codec_parameters(current_media_config);
        current_active_gaming_rx_config = current_gaming_rx_config;
        print_codec_parameters(current_gaming_rx_config);
      } else if (current_active_profile_type == WMCP) {
        current_active_config = current_recording_config;
      } else {
        current_active_config = current_media_config;
        BTIF_TRACE_DEBUG("%s: current_media_config's codec_type = : %d",
                            __PRETTY_FUNCTION__, current_media_config.codec_type);
      }

      if (btif_peer_device_is_reconfiguring(peer_.SetId()))
        btif_acm_source_restart_session(active_bda, active_bda);

      if (current_active_profile_type == WMCP) {
        uint16_t sink_latency = btif_acm_get_active_device_latency();
        BTIF_TRACE_EVENT("%s: sink_latency = %dms", __func__, sink_latency);
        if ((sink_latency > 0) && !btif_acm_update_sink_latency_change(sink_latency * 10)) {
        BTIF_TRACE_ERROR("%s: unable to update latency", __func__);
        }
      }

      if (current_active_profile_type == BAP) {
        peer_.ResetProfileType(GCP);
        peer_.ResetProfileType(GCP_RX);
        peer_.SetProfileType(BAP);
      } else if (current_active_profile_type == GCP) {
        peer_.ResetProfileType(BAP);
        peer_.ResetProfileType(GCP_RX);
        peer_.SetProfileType(GCP);
      } else if (current_active_profile_type == GCP_RX) {
        //TODO
        peer_.ResetProfileType(BAP);
        peer_.ResetProfileType(GCP);
        peer_.SetProfileType(GCP_RX);
      }

      BTIF_TRACE_DEBUG("%s: cummulative_profile_type %d",
                                  __func__, peer_.GetProfileType());
      BTIF_TRACE_DEBUG("%s: Reconfig + restart session completed for media,"
                                            " signal session ready", __func__);
      btif_acm_signal_session_ready();
    } else if (current_active_profile_type == peer_.GetRcfgProfileType()) {
      if(btif_ahim_is_aosp_aidl_hal_enabled()) {
        // update the configs properly
        if (current_active_profile_type == GCP_RX) {
          current_active_config = peer_.get_peer_media_codec_config();
          current_active_gaming_rx_config = current_gaming_rx_config;
        } else if (current_active_profile_type == WMCP) {
          current_active_config = current_recording_config;
        } else {
          current_active_config = peer_.get_peer_media_codec_config();
        }
        btif_acm_source_restart_session(active_bda, active_bda);
      }
      BTIF_TRACE_DEBUG("%s: Reconfig to remote is completed for media, "
                                    "restart session wasn't needed", __func__);
    } else {
      if(btif_ahim_is_aosp_aidl_hal_enabled()) {
        current_active_config = peer_.get_peer_voice_tx_codec_config();
        btif_acm_source_restart_session(active_bda, active_bda);
      }
      BTIF_TRACE_DEBUG("%s: Reconfig completed for BAP_CALL", __func__);
    }
    peer_.ClearFlags(BtifAcmPeer::kFlagPendingReconfigure);
  } else if(peer_.CheckFlags(BtifAcmPeer::kFlagCallAndMusicSync)) {
    peer_.ClearFlags(BtifAcmPeer::kFlagCallAndMusicSync);
    if(btif_ahim_is_aosp_aidl_hal_enabled()) {
      if (active_context_type == CONTEXT_TYPE_VOICE) {
        current_active_config = peer_.get_peer_voice_tx_codec_config();
      } else {
        if (current_active_profile_type == GCP_RX) {
          current_active_config = peer_.get_peer_media_codec_config();
          current_active_gaming_rx_config = current_gaming_rx_config;
        } else if (current_active_profile_type == WMCP) {
          current_active_config = current_recording_config;
        } else {
          BTIF_TRACE_DEBUG("%s: get_peer_media_codec_config", __func__);
          current_active_config = peer_.get_peer_media_codec_config();
        }
      }
      btif_acm_source_restart_session(active_bda, active_bda);
    }
    BTIF_TRACE_DEBUG("%s: Reconfig update completed for BAP_CALL", __func__);
  } else if ((active_context_type == CONTEXT_TYPE_MUSIC) &&
             peer_.CheckFlags(BtifAcmPeer::kFlagMediaSubUsecaseSync) &&
             (peer_.GetStreamProfileType() & old_active_profile_type)) {
    BTIF_TRACE_DEBUG("%s: Stream stopped for old profile: %d",
                                      __func__, old_active_profile_type);
    BTIF_TRACE_DEBUG("%s: GetStreamProfileType: %d",
                                    __func__, peer_.GetStreamProfileType());
    BTIF_TRACE_DEBUG("%s: Resume the new profile activity ", __func__);
    peer_.ClearFlags(BtifAcmPeer::kFlagMediaSubUsecaseSync);
    if (current_active_profile_type == GCP_RX) {
      current_active_config = peer_.get_peer_media_codec_config();
      current_active_gaming_rx_config = current_gaming_rx_config;
    } else if (current_active_profile_type == WMCP) {
      current_active_config = current_recording_config;
    } else {
      BTIF_TRACE_DEBUG("%s: get_peer_media_codec_config", __func__);
      current_active_config = peer_.get_peer_media_codec_config();
    }
    if(old_active_profile_type == GCP_RX) {
      // resume all first
      uint16_t temp_profile = current_active_profile_type;
      current_active_profile_type = old_active_profile_type;
      btif_acm_source_restart_session(active_bda, active_bda);
      current_active_profile_type = temp_profile;
      btif_acm_source_restart_session(active_bda, active_bda);
    } else {
      btif_acm_source_restart_session(active_bda, active_bda);
    }
  } else if ((active_context_type == CONTEXT_TYPE_MUSIC) &&
              peer_.CheckFlags(BtifAcmPeer::kFlagMediaSubUsecaseSync) &&
             (peer_.GetStreamProfileType() & current_active_profile_type)) {
    BTIF_TRACE_DEBUG("%s: Stream stopped for new profile ",
                            __func__, current_active_profile_type);
    BTIF_TRACE_DEBUG("%s: update config for old profile ",
                            __func__, old_active_profile_type);
    peer_.ClearFlags(BtifAcmPeer::kFlagMediaSubUsecaseSync);
    uint16_t temp_profile;
    if (old_active_profile_type == GCP_RX) {
      current_active_config = peer_.get_peer_media_codec_config();
      current_active_gaming_rx_config = current_gaming_rx_config;
    } else if (old_active_profile_type == WMCP) {
      current_active_config = current_recording_config;
    } else {
      BTIF_TRACE_DEBUG("%s: get_peer_media_codec_config", __func__);
      current_active_config = peer_.get_peer_media_codec_config();
    }

    if((current_active_profile_type == WMCP && old_active_profile_type == BAP) ||
       (current_active_profile_type == BAP && old_active_profile_type == WMCP)) {
      current_active_profile_type = old_active_profile_type;
      btif_acm_source_restart_session(active_bda, active_bda);
      btif_acm_update_acm_context(active_context_type, current_active_profile_type,
                                  current_active_profile_type, active_bda);
    }
  } else if( active_context_type == CONTEXT_TYPE_VOICE &&
             peer_.CheckFlags(BtifAcmPeer::kFlagMediaSubUsecaseSync)) {
    BTIF_TRACE_DEBUG("%s: clear kFlagMediaSubUsecaseSync for Voice context ", __func__);
    peer_.ClearFlags(BtifAcmPeer::kFlagMediaSubUsecaseSync);
  }

  //Start the lock release timer here.
  //check if peer device is in started state
  if (btif_peer_device_is_streaming(peer_.SetId()) ||
      peer_.CheckFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig)) {
    StreamType type_1, type_2;
    std::vector<StreamType> start_streams;

    // If peer device is in started state, use same context type with peer to start stream
    // Else check RcfgProfileType for stream start context type
    int start_stream_type;
    if (btif_peer_device_is_streaming(peer_.SetId())) {
      start_stream_type = btif_get_peer_streaming_context_type(peer_.SetId());
      peer_.SetStreamContextType(start_stream_type);
      peer_.SetStreamProfileType(current_active_profile_type);
    } else {
      if (peer_.GetRcfgProfileType() != BAP_CALL) {
        start_stream_type = CONTEXT_TYPE_MUSIC;
      } else {
        start_stream_type = CONTEXT_TYPE_VOICE;
      }
    }

    char value[PROPERTY_VALUE_MAX] = {'\0'};
    uint8_t pts_audio_context_ = 0;
    property_get("persist.vendor.btstack.pts_audio_context_", value, "0");

    int res = sscanf(value, "%hhu", &pts_audio_context_);
    BTIF_TRACE_DEBUG("%s: res: %d", __func__, res);
    BTIF_TRACE_DEBUG("%s: pts_audio_context_: %d", __func__, pts_audio_context_);

    BTIF_TRACE_DEBUG("%s: start_stream_type: %d", __func__, start_stream_type);
    if (start_stream_type != CONTEXT_TYPE_VOICE) {
      if ((current_active_profile_type == BAP) &&
              (peer_.GetPeerMusicTxState() == StreamState::CONNECTED)) {

        if ((res == 1) && (pts_audio_context_ != 0)) {
          if (pts_audio_context_ == 2) {
            type_1 = {.type = CONTENT_TYPE_MEDIA,
                      .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                      .direction = ASE_DIRECTION_SINK
                     };
          }
        } else {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_MEDIA,
                    .direction = ASE_DIRECTION_SINK
                   };
        }
        start_streams.push_back(type_1);
      } else if ((current_active_profile_type == GCP) &&
              (peer_.GetPeerMusicTxState() == StreamState::CONNECTED)) {
        type_1 = {.type = CONTENT_TYPE_MEDIA,
                  .audio_context = CONTENT_TYPE_GAME,
                  .direction = ASE_DIRECTION_SINK
                 };
        start_streams.push_back(type_1);
      } else if ((current_active_profile_type == WMCP) &&
              (peer_.GetPeerMusicRxState() == StreamState::CONNECTED)) {
        if ((res == 1) && (pts_audio_context_ != 0)) {
          if (pts_audio_context_ == 1) {
            type_1 = {.type = CONTENT_TYPE_MEDIA,
                      .audio_context = CONTENT_TYPE_MEDIA,
                      .direction = ASE_DIRECTION_SRC
                     };
          } else if (pts_audio_context_ == 2) {
            type_1 = {.type = CONTENT_TYPE_MEDIA,
                      .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                      .direction = ASE_DIRECTION_SRC
                     };
          }
        } else {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_LIVE,
                    .direction = ASE_DIRECTION_SRC
                   };
        }
        start_streams.push_back(type_1);
      } else if ((current_active_profile_type == GCP_RX) &&
              (peer_.GetPeerMusicTxState() == StreamState::CONNECTED) &&
              (peer_.GetPeerMusicRxState() == StreamState::CONNECTED)) {
        type_1 = {.type = CONTENT_TYPE_MEDIA,
                  .audio_context = CONTENT_TYPE_GAME,
                  .direction = ASE_DIRECTION_SRC
                 };
        type_2 = {.type = CONTENT_TYPE_MEDIA,
                  .audio_context = CONTENT_TYPE_GAME,
                  .direction = ASE_DIRECTION_SINK
                 };
        start_streams.push_back(type_1);
        start_streams.push_back(type_2);
      }
    } else {
      uint16_t num_sink_ases, num_src_ases;
      btif_bap_get_params(peer_.PeerAddress(), nullptr, nullptr, &num_sink_ases,
                            &num_src_ases, nullptr, nullptr);
      if (peer_.GetPeerVoiceTxState() == StreamState::CONNECTED) {
        if(num_sink_ases) {
          type_1 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                  .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                  .direction = ASE_DIRECTION_SINK
                 };
          start_streams.push_back(type_1);
        }
      }
      if(peer_.GetPeerVoiceRxState() == StreamState::CONNECTED) {
        //keeping voice rx as BAP config
        if(num_src_ases) {
          type_2 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                    .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                    .direction = ASE_DIRECTION_SRC
                   };
          start_streams.push_back(type_2);
        }
      }
    }

    if(btif_acm_initiator.IsConnUpdateEnabled()) {
      //Cancel the timer if start streamng comes before
      // 5 seconds while moving the interval to relaxed mode.
      if (alarm_is_scheduled(btif_acm_initiator.AcmConnIntervalTimer())) {
        btif_acm_check_and_cancel_conn_Interval_timer();
      } else {
        peer_.SetConnUpdateMode(BtifAcmPeer::kFlagAggresiveMode);
      }
    }
    // isMonoConfig will indicate the bap layer that send enable only 3 ASEs for vbc
    BTIF_TRACE_DEBUG(" current_active_profile_type = %d", current_active_profile_type);
    uint8_t version = GetVendorMetaDataCodecEncoderVer(&current_active_config);
    bool isMonoConfig = (current_active_profile_type == GCP_RX) &&
                        (version == LC3Q_CODEC_FT_CHANGE_SUPPORTED_VERSION);
    BTIF_TRACE_DEBUG(" isMonoConfig = %d", isMonoConfig);
    BtifAcmPeer *other_peer = btif_get_peer_device (&peer_);
    if(other_peer &&
       peer_.CheckFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig) &&
       other_peer->CheckFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig)) {
      peer_.ClearFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
      peer_.SetFlags(BtifAcmPeer::kFLagPendingStartAfterPeerReconfig);
      BTIF_TRACE_DEBUG("%s: pending for peer reconfig", __func__);
      return;
    } else if(other_peer &&
       peer_.CheckFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig) &&
       other_peer->CheckFlags(BtifAcmPeer::kFLagPendingStartAfterPeerReconfig)) {
      peer_.ClearFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
      other_peer->ClearFlags(BtifAcmPeer::kFLagPendingStartAfterPeerReconfig);
      peer_.SetRcfgProfileType(0);
      other_peer->SetRcfgProfileType(0);
      std::vector<RawAddress> address;
      address.push_back(peer_.PeerAddress());
      address.push_back(other_peer->PeerAddress());
      sUcastClientInterface->Start(address, start_streams, isMonoConfig);
      BTIF_TRACE_DEBUG("%s: Issued converged start ", __func__);
      peer_.SetFlags(BtifAcmPeer::kFlagPendingStart);
      other_peer->SetFlags(BtifAcmPeer::kFlagPendingStart);
    } else {
      std::vector<RawAddress> address;
      address.push_back(peer_.PeerAddress());
      peer_.SetRcfgProfileType(0);
      if (start_streams.size() != 0) {
        sUcastClientInterface->Start(address, start_streams, isMonoConfig);
        peer_.SetFlags(BtifAcmPeer::kFlagPendingStart);
        peer_.ClearFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
      }
    }
  }

  if (peer_.StateMachine().PreviousStateId() == BtifAcmStateMachine::kStateStarted) {
    BTIF_TRACE_DEBUG("%s: Entering Opened from Started State", __PRETTY_FUNCTION__);
    if ((btif_acm_initiator.GetGroupLockStatus(peer_.SetId()) !=
         BtifAcmInitiator::kFlagStatusUnknown) &&
         alarm_is_scheduled(btif_acm_initiator.AcmGroupProcedureTimer())) {
      BTIF_TRACE_DEBUG("%s: All locked and stop/suspend requested device have stopped,"
                                                              " ack mm audio", __func__);
      btif_acm_check_and_cancel_group_procedure_timer(
                                   btif_acm_initiator.MusicActiveCSetId());

      tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
      if (pending_cmd == A2DP_CTRL_CMD_STOP ||
         pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
        btif_acm_source_on_suspended(A2DP_CTRL_ACK_SUCCESS);
      } else if (pending_cmd == A2DP_CTRL_CMD_START) {
        btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
      } else {
        BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
      }

      btif_acm_check_and_start_lock_release_timer(btif_acm_initiator.MusicActiveCSetId());
    } else {
      tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
      if (pending_cmd == A2DP_CTRL_CMD_STOP ||
         pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
        btif_acm_source_on_suspended(A2DP_CTRL_ACK_SUCCESS);
      } else {
        BTIF_TRACE_DEBUG("%s: no pending suspend command to ack mm audio", __func__);
      }

    }
    BtifAcmPeer *other_peer = btif_get_peer_device(&peer_);
    if (other_peer &&
        other_peer->CheckFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend)) {
      BTIF_TRACE_DEBUG("%s: other peer is suspended and waiting, report STOP to apps"
                      " and move to Opened for it", __func__);
      btif_report_audio_state(other_peer->PeerAddress(),
                              BTACM_AUDIO_STATE_STOPPED,
                              other_peer->GetStreamContextType());
      other_peer->StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
      other_peer->ClearFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend);
    }
  }

  if (peer_.StateMachine().PreviousStateId() == BtifAcmStateMachine::kStateStarted) {
    if ((btif_acm_initiator.MusicActiveCSetId() > 0) &&
        (btif_acm_initiator.GetGroupLockStatus(
        btif_acm_initiator.MusicActiveCSetId()) == BtifAcmInitiator::kFlagStatusLocked)) {
      BTIF_TRACE_DEBUG("%s: Peer: %s", __PRETTY_FUNCTION__,
                                   peer_.PeerAddress().ToString().c_str());
      btif_acm_check_and_start_lock_release_timer(btif_acm_initiator.MusicActiveCSetId());
    }
  }
}

void BtifAcmStateMachine::StateOpened::OnExit() {
  BTIF_TRACE_DEBUG("%s: Peer %s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str());
}

bool BtifAcmStateMachine::StateOpened::ProcessEvent(uint32_t event,
                                                   void* p_data) {
  tBTIF_ACM* p_acm = (tBTIF_ACM*)p_data;

  BTIF_TRACE_DEBUG("%s: Peer %s : event=%s, flags=%s, music_active_peer=%s, "
                   "voice_active_peer=%s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str(),
                   BtifAcmEvent::EventName(event).c_str(),
                   peer_.FlagsToString().c_str(),
                   logbool(peer_.IsPeerActiveForMusic()).c_str(),
                   logbool(peer_.IsPeerActiveForVoice()).c_str());

  switch (event) {
    case BTIF_ACM_CONNECT_REQ_EVT: {
      ConnectionState pacs_state = peer_.GetPeerPacsState();
      if (pacs_state == ConnectionState::CONNECTED) {
        LOG_INFO(LOG_TAG, "%s PACS connected, initiate BAP connect", __PRETTY_FUNCTION__);
        bool can_connect = true;
        // Check whether connection is allowed
        if (peer_.IsAcceptor()) {
          //There is no char in current spec. Should we check VMCP role here?
          // shall we assume VMCP role would have been checked in apps and no need to check here?
          can_connect = btif_acm_initiator.AllowedToConnect(peer_.PeerAddress());
          if (!can_connect) disconnect_acm_initiator(peer_.PeerAddress(),
                                                     current_active_context_type);
        }
        if (!can_connect) {
          BTIF_TRACE_ERROR(
              "%s: Cannot connect to peer %s: too many connected "
              "peers",
              __PRETTY_FUNCTION__, peer_.PeerAddress().ToString().c_str());
          break;
        }

        BTIF_TRACE_ERROR("%s: current_active_context_type: %d,"
                         " peer_.GetProfileType(): %d", __PRETTY_FUNCTION__,
                         current_active_context_type, peer_.GetProfileType());
        std::vector<StreamConnect> streams;
        if (current_active_context_type == CONTEXT_TYPE_MUSIC) {
          StreamConnect conn_media;
          if (peer_.GetProfileType() & (BAP|GCP)) {
            bool is_m_tx_fetched = false;
            //keeping media tx as BAP/GCP config
            memset(&conn_media, 0, sizeof(conn_media));
            is_m_tx_fetched = fetch_media_tx_codec_qos_config(peer_.PeerAddress(),
                                            peer_.GetProfileType() & (BAP|GCP),
                                            &conn_media);
            BTIF_TRACE_ERROR("%s: is_m_tx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_m_tx_fetched);
            if (is_m_tx_fetched == false) {
              BTIF_TRACE_ERROR("%s: media tx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          }
          if (peer_.GetProfileType() & WMCP) {
            bool is_m_rx_fetched = false;
            //keeping media rx as WMCP config
            memset(&conn_media, 0, sizeof(conn_media));
            is_m_rx_fetched = fetch_media_rx_codec_qos_config(peer_.PeerAddress(),
                                                              WMCP, &conn_media);
            BTIF_TRACE_ERROR("%s: is_m_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_m_rx_fetched);
            if (is_m_rx_fetched == false) {
              BTIF_TRACE_ERROR("%s: media rx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          } else if (peer_.GetProfileType() & GCP_RX) {
            bool is_m_gcp_rx_fetched = false;
            //keeping media rx as GCP_RX config
            memset(&conn_media, 0, sizeof(conn_media));
            is_m_gcp_rx_fetched = fetch_media_rx_codec_qos_config(peer_.PeerAddress(),
                                                                  GCP_RX, &conn_media);
            BTIF_TRACE_ERROR("%s: is_m_gcp_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_m_gcp_rx_fetched);
            if (is_m_gcp_rx_fetched == false) {
              BTIF_TRACE_ERROR("%s: gcp rx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          }
        } else if (current_active_context_type == CONTEXT_TYPE_MUSIC_VOICE) {
          StreamConnect conn_media, conn_voice;
          //keeping voice tx as BAP config
          uint16_t num_sink_ases, num_src_ases;
          btif_bap_get_params(peer_.PeerAddress(), nullptr, nullptr,
                              &num_sink_ases, &num_src_ases,
                              nullptr, nullptr);

          if(num_sink_ases) {
            bool is_vm_v_tx_fetched = false;
            memset(&conn_voice, 0, sizeof(conn_voice));
            is_vm_v_tx_fetched = fetch_voice_tx_codec_qos_config(peer_.PeerAddress(),
                                                              BAP, &conn_voice);
            BTIF_TRACE_ERROR("%s: is_vm_v_tx_fetched: %d",
                                     __PRETTY_FUNCTION__, is_vm_v_tx_fetched);
            if (is_vm_v_tx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm voice tx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_voice);
            }
          }

          //keeping voice rx as BAP config
          if(num_src_ases) {
            bool is_vm_v_rx_fetched = false;
            memset(&conn_voice, 0, sizeof(conn_voice));
            is_vm_v_rx_fetched = fetch_voice_rx_codec_qos_config(peer_.PeerAddress(),
                                                                 BAP, &conn_voice);
            BTIF_TRACE_ERROR("%s: is_vm_v_rx_fetched: %d",
                                     __PRETTY_FUNCTION__, is_vm_v_rx_fetched);
            if (is_vm_v_rx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm voice rx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_voice);
            }
          }

          if (peer_.GetProfileType() & (BAP|GCP)) {
            bool is_vm_m_tx_fetched = false;
            //keeping media tx as BAP/GCP config
            memset(&conn_media, 0, sizeof(conn_media));
            is_vm_m_tx_fetched = fetch_media_tx_codec_qos_config(peer_.PeerAddress(),
                                            peer_.GetProfileType() & (BAP|GCP),
                                            &conn_media);
            BTIF_TRACE_ERROR("%s: is_vm_m_tx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_vm_m_tx_fetched);
            if (is_vm_m_tx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm media tx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          }
          if (peer_.GetProfileType() & WMCP) {
            bool is_vm_m_rx_fetched = false;
            //keeping media rx as WMCP config
            memset(&conn_media, 0, sizeof(conn_media));
            is_vm_m_rx_fetched = fetch_media_rx_codec_qos_config(peer_.PeerAddress(),
                                                              WMCP, &conn_media);
            BTIF_TRACE_ERROR("%s: is_vm_m_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_vm_m_rx_fetched);
            if (is_vm_m_rx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm media rx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          } else if (peer_.GetProfileType() & GCP_RX) {
            bool is_vm_gcp_rx_fetched = false;
            //keeping media rx as GCP_RX config
            memset(&conn_media, 0, sizeof(conn_media));
            is_vm_gcp_rx_fetched = fetch_media_rx_codec_qos_config(peer_.PeerAddress(),
                                                                  GCP_RX, &conn_media);
            BTIF_TRACE_ERROR("%s: is_vm_gcp_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_vm_gcp_rx_fetched);
            if (is_vm_gcp_rx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm gcp rx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          }
        } else if (current_active_context_type == CONTEXT_TYPE_VOICE) {
          StreamConnect conn_voice;
          bool is_v_tx_fetched = false;
          //keeping voice tx as BAP config
          memset(&conn_voice, 0, sizeof(conn_voice));
          is_v_tx_fetched = fetch_voice_tx_codec_qos_config(peer_.PeerAddress(),
                                                            BAP, &conn_voice);
          BTIF_TRACE_ERROR("%s: is_v_tx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_v_tx_fetched);
          if (is_v_tx_fetched == false) {
            BTIF_TRACE_ERROR("%s: voice tx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_voice);
          }

          bool is_v_rx_fetched = false;
          //keeping voice rx as BAP config
          memset(&conn_voice, 0, sizeof(conn_voice));
          is_v_rx_fetched = fetch_voice_rx_codec_qos_config(peer_.PeerAddress(),
                                                               BAP, &conn_voice);
          BTIF_TRACE_ERROR("%s: is_v_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_v_rx_fetched);
          if (is_v_rx_fetched == false) {
            BTIF_TRACE_ERROR("%s: voice rx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_voice);
          }
        }
        LOG(WARNING) << __func__ << ": size of streams: " << streams.size();
        if (!sUcastClientInterface) break;

        if (streams.size() == 0) {
          LOG_DEBUG(LOG_TAG, "%s: Streams null, Don't send connect req to BAP",
                                               __PRETTY_FUNCTION__);
          break;
        }

        std::vector<RawAddress> address;
        address.push_back(peer_.PeerAddress());
        sUcastClientInterface->Connect(address, true, streams);
        //peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpening);
      }
    } break;

    case BTIF_ACM_STOP_STREAM_REQ_EVT:
    case BTIF_ACM_SUSPEND_STREAM_REQ_EVT: {
      BTIF_TRACE_DEBUG("%s: Already in OPENED state, ACK success", __PRETTY_FUNCTION__);
      tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
      if (pending_cmd == A2DP_CTRL_CMD_STOP ||
         pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
        btif_acm_source_on_suspended(A2DP_CTRL_ACK_SUCCESS);
      } else if (pending_cmd == A2DP_CTRL_CMD_START) {
        btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
      } else {
        BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
      }
      if(event == BTIF_ACM_STOP_STREAM_REQ_EVT) {
        tBTIF_ACM_STOP_REQ * p_bta_data = (tBTIF_ACM_STOP_REQ*)p_data;
        if(p_bta_data->wait_for_signal) {
          p_bta_data->peer_ready_promise->set_value();
        }
        LOG_ERROR(LOG_TAG, "%s BTIF_ACM_STOP_STREAM_REQ_EVT from BTIF ",
                                               __PRETTY_FUNCTION__);
      }
    } break;

    case BTIF_ACM_START_STREAM_REQ_EVT: {
      LOG_INFO(LOG_TAG, "%s: Peer %s : event=%s flags=%s", __PRETTY_FUNCTION__,
              peer_.PeerAddress().ToString().c_str(),
              BtifAcmEvent::EventName(event).c_str(),
              peer_.FlagsToString().c_str());
      if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingStart) ||
          peer_.CheckFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig) ||
          peer_.CheckFlags(BtifAcmPeer::kFLagPendingStartAfterPeerReconfig)) {
        BTIF_TRACE_DEBUG("%s: Ignore Redundant Start req", __PRETTY_FUNCTION__);
        break;
      }

      LOG_INFO(LOG_TAG, "%s peer strm context type: %d, "
                        "current_active_profile_type: %d",
                        __PRETTY_FUNCTION__, peer_.GetStreamContextType(),
                        current_active_profile_type);
      std::string cached_config = peer_.get_peer_dev_requested_config();
      if (!cached_config.empty()) {
        LOG_ERROR(LOG_TAG, "%s Persist Dev UI Reconfig %s during stream start ",
                                               __PRETTY_FUNCTION__, (char *) cached_config.c_str());
        if (change_codec_config_acm_initiator_req(peer_.PeerAddress(),
            (char *) cached_config.c_str(), false) == BT_STATUS_SUCCESS)
          peer_.SetFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
      } else {
        if (peer_.GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
          if (reconfig_acm_initiator(peer_.PeerAddress(), current_active_profile_type, false)) {
            peer_.SetFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
          }
        } else if (peer_.GetStreamContextType() == CONTEXT_TYPE_VOICE) {
          peer_.SetFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
          if (!reconfig_acm_initiator(peer_.PeerAddress(), BAP_CALL, false)) {
            peer_.ClearFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
          }
        }
      }
    }
    break;

    case BTA_ACM_START_EVT: {
      tBTIF_ACM_STATUS status = (uint8_t)p_acm->state_info.stream_state;
      LOG_INFO(LOG_TAG,
               "%s: Peer %s : event=%s status=%d ",
               __PRETTY_FUNCTION__, peer_.PeerAddress().ToString().c_str(),
               BtifAcmEvent::EventName(event).c_str(),status);

      if (p_acm->state_info.stream_state == StreamState::STARTING) {
        //Check what to do in this case
        BTIF_TRACE_DEBUG("%s: BAP returned as starting, ignore", __PRETTY_FUNCTION__);
        break;
      } else if (p_acm->state_info.stream_state == StreamState::STREAMING) {
        int contentType = p_acm->acm_connect.streams_info.stream_type.type;
        if (contentType == CONTENT_TYPE_MEDIA) {
          peer_.SetStreamContextType(CONTEXT_TYPE_MUSIC);
          BTIF_TRACE_ERROR("%s: setting Music stream context", __PRETTY_FUNCTION__);
          //peer_.SetStreamProfileType(0);
        } else if (contentType == CONTENT_TYPE_CONVERSATIONAL) {
          peer_.SetStreamContextType(CONTEXT_TYPE_VOICE);
          BTIF_TRACE_ERROR("%s: setting Voice stream context", __PRETTY_FUNCTION__);
          //peer_.SetStreamProfileType(0);
        }
        peer_.ClearFlags(BtifAcmPeer::kFlagPendingStart);

        BTIF_TRACE_DEBUG("%s: kFlagPendingAlsAndMetadataSync: %d ", __PRETTY_FUNCTION__,
                  peer_.CheckFlags(BtifAcmPeer::kFlagPendingAlsAndMetadataSync));

        if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingAlsAndMetadataSync)) {
          peer_.ClearFlags(BtifAcmPeer::kFlagPendingAlsAndMetadataSync);
        }

        // latency related calculations
        peer_.cis_established_base_ft_ = p_acm->state_info.stream_link_params.ft_m_to_s;
        peer_.active_cig_id = p_acm->state_info.stream_link_params.cig_id;
        BTIF_TRACE_DEBUG("%s: base ft %d, Active CIG Id %d", __PRETTY_FUNCTION__,
            peer_.cis_established_base_ft_, peer_.active_cig_id);
        peer_.cis_handle_list.push_back(p_acm->state_info.stream_link_params.connection_handle);
        peer_.cis_established_transport_latency_ =
            p_acm->state_info.stream_link_params.transport_latency_m_to_s;
        uint16_t total_latency = peer_.peer_negotiated_pd_ + peer_.cis_established_transport_latency_;
        peer_.SetPeerLatency(total_latency);
        BTIF_TRACE_DEBUG("%s: peer pd %d, ATL %d, total_latency %d", __PRETTY_FUNCTION__,
            peer_.peer_negotiated_pd_, peer_.cis_established_transport_latency_, total_latency);

        peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateStarted);
      }
    } break;

    case BTIF_ACM_DISCONNECT_REQ_EVT:{
      tBTIF_ACM_CONN_DISC* p_bta_data = (tBTIF_ACM_CONN_DISC*)p_data;
      std::vector<StreamType> disconnect_streams;
      BtifAcmPeer *other_peer = btif_get_peer_device (&peer_);
      if (other_peer == nullptr || (!other_peer->CheckFlags(BtifAcmPeer::kFlagPendingReconfigure) &&
                          !other_peer->CheckFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig) &&
                          !other_peer->CheckFlags(BtifAcmPeer::kFlagPendingStart) &&
                          !other_peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend))) {
        tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
        if (pending_cmd == A2DP_CTRL_CMD_START) {
          btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
        } else if (pending_cmd == A2DP_CTRL_CMD_SUSPEND ||
                   pending_cmd == A2DP_CTRL_CMD_STOP) {
          btif_acm_source_on_suspended(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
        }
      }
      if (p_bta_data->contextType == CONTEXT_TYPE_MUSIC) {
        StreamType type_1;
        if (p_bta_data->profileType & BAP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_MEDIA,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & WMCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_LIVE,
                    .direction = ASE_DIRECTION_SRC
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & GCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        StreamType type_2;
        if (p_bta_data->profileType & GCP_RX) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SRC
                   };
          type_2 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
          disconnect_streams.push_back(type_2);
        }
      } else if (p_bta_data->contextType == CONTEXT_TYPE_MUSIC_VOICE) {
        StreamType type_2 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SRC
                            };
        StreamType type_3 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SINK
                            };
        disconnect_streams.push_back(type_3);
        disconnect_streams.push_back(type_2);
        StreamType type_1;
        if (p_bta_data->profileType & BAP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_MEDIA,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & WMCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_LIVE,
                    .direction = ASE_DIRECTION_SRC
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & GCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        StreamType type_4;
        if (p_bta_data->profileType & GCP_RX) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SRC
                   };
          type_4 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
          disconnect_streams.push_back(type_4);
        }
      } else if (p_bta_data->contextType == CONTEXT_TYPE_VOICE) {
        StreamType type_2 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SRC
                            };
        StreamType type_3 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SINK
                            };
        disconnect_streams.push_back(type_3);
        disconnect_streams.push_back(type_2);
      }

      LOG(WARNING) << __func__
                   << " size of disconnect_streams " << disconnect_streams.size();
      if (!sUcastClientInterface) break;
      sUcastClientInterface->Disconnect(peer_.PeerAddress(), disconnect_streams);

      if ((p_bta_data->contextType == CONTEXT_TYPE_MUSIC) &&
          ((peer_.GetPeerVoiceRxState() == StreamState::CONNECTED) ||
           (peer_.GetPeerVoiceTxState() == StreamState::CONNECTED))) {
        if (peer_.GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
          peer_.ClearFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
          peer_.ClearFlags(BtifAcmPeer::kFlagPendingStart);
          LOG(WARNING) << __func__ << " stream context music disconnected, clear flags ";
        }
        LOG(WARNING) << __func__ << " voice connected remain in opened ";
      } else if ((p_bta_data->contextType == CONTEXT_TYPE_VOICE) &&
                 ((peer_.GetPeerMusicTxState() == StreamState::CONNECTED) ||
                  (peer_.GetPeerMusicRxState() == StreamState::CONNECTED))) {
        if (peer_.GetStreamContextType() == CONTEXT_TYPE_VOICE) {
          peer_.ClearFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
          peer_.ClearFlags(BtifAcmPeer::kFlagPendingStart);
          LOG(WARNING) << __func__ << " stream context voice disconnected, clear flags ";
        }
        LOG(WARNING) << __func__ << " Music connected remain in opened ";
      } else {
        peer_.ClearFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
        peer_.ClearFlags(BtifAcmPeer::kFlagPendingStart);
        LOG(WARNING) << __func__ << " Move in closing state ";
        peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
      }
    } break;

    case BTA_ACM_CONNECT_EVT: {// above evnt can come and handle for voice/media case
      tBTIF_ACM_STATUS status = (uint8_t)p_acm->state_info.stream_state;
      int contextType = p_acm->state_info.stream_type.type;

      LOG_INFO(
          LOG_TAG, "%s: Peer %s : event=%s status=%d",
          __PRETTY_FUNCTION__, peer_.PeerAddress().ToString().c_str(),
          BtifAcmEvent::EventName(event).c_str(),status);

      if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend) ||
          peer_.CheckFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend)) {
        peer_.ClearFlags(BtifAcmPeer::kFlagPendingLocalSuspend);
        peer_.ClearFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend);
        BTIF_TRACE_DEBUG("%s: peer device is suspended, send MM any pending ACK",
                                                                 __func__);
        tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
        if (pending_cmd == A2DP_CTRL_CMD_STOP ||
         pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
         btif_acm_source_on_suspended(A2DP_CTRL_ACK_SUCCESS);
        } else if (pending_cmd == A2DP_CTRL_CMD_START) {
          btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
        } else {
         BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
        }
        break;
      }

      //TODO, below need to revisit when Gaming Rx moved to opened and then
      //Tx connect event comes from BAP
      if (p_acm->state_info.stream_state == StreamState::CONNECTED) {
        if (contextType == CONTENT_TYPE_MEDIA) {
          /*if ((btif_acm_initiator.MusicActivePeer() == peer_.PeerAddress()) &&
                  peer_.CheckFlags(BtifAcmPeer::kFlagPendingReconfigure)) { //recheck
            LOG(INFO) << __PRETTY_FUNCTION__ << " : Peer " << peer_.PeerAddress()
                  << " : Reconfig done - calling startSession() to audio HAL";
            std::promise<void> peer_ready_promise;
            std::future<void> peer_ready_future = peer_ready_promise.get_future();
            //TODO: cannot use peer addr here, must need group address.
            btif_acm_source_start_session(peer_.PeerAddress());
            //Perform group operation here
          } else */
         if (((peer_.GetPeerVoiceRxState() == StreamState::CONNECTED) ||
                      (peer_.GetPeerVoiceTxState() == StreamState::CONNECTED) ||
                      (peer_.GetPeerMusicRxState() == StreamState::CONNECTED)) &&
                     (peer_.GetPeerMusicTxState() == StreamState::CONNECTING) &&
                     (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK)) {
            BTIF_TRACE_DEBUG("%s: music Tx connected when either Voice Tx/Rx or"
                             " Music Rx was connected, remain in opened state", __func__);
            peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
            BTIF_TRACE_DEBUG("%s: received connected state from BAP for Music TX, "
                             "update state", __func__);
            btif_report_connection_state(peer_.PeerAddress(),
                                         BTACM_CONNECTION_STATE_CONNECTED,
                                         CONTEXT_TYPE_MUSIC);
          } else if ((peer_.GetPeerMusicRxState() == StreamState::CONNECTING) &&
                     (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC)) {
            peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
            if (p_acm->state_info.stream_type.audio_context != CONTENT_TYPE_GAME) {
              BTIF_TRACE_DEBUG("%s: received connected state from BAP for Music RX(recording), "
                                                                    "update state", __func__);
              btif_report_connection_state(peer_.PeerAddress(),
                                         BTACM_CONNECTION_STATE_CONNECTED, CONTEXT_TYPE_MUSIC);
            } else {
              //When Gaming Tx would get connets, then we can update to app as connected.
            }
          }
          if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingStart) &&
              peer_.GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
            peer_.ClearFlags(BtifAcmPeer::kFlagPendingStart);
            peer_.ClearFlags(BtifAcmPeer::kFlagPendingSetActiveDev);
            BTIF_TRACE_DEBUG("%s: BAP returned without streaming, ACK MM", __func__);
            tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
            if (pending_cmd == A2DP_CTRL_CMD_STOP || pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
              btif_acm_source_on_suspended(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
            } else if (pending_cmd == A2DP_CTRL_CMD_START) {
              btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
            } else {
             BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
            }
          }
        } else if (contextType == CONTENT_TYPE_CONVERSATIONAL) {
          BTIF_TRACE_DEBUG("%s: voice context connected, remain in opened state"
                  " peer_.GetPeerVoiceTxState() %d peer_.GetPeerVoiceRxState() %d",
                  __func__, peer_.GetPeerVoiceTxState(), peer_.GetPeerVoiceRxState());
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK &&
                  (peer_.GetPeerVoiceTxState() != StreamState::CONNECTED)) {
            peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceRxState() == StreamState::CONNECTED) {
              BTIF_TRACE_DEBUG("%s: received connected state from BAP for voice TX, "
                                                            "update state", __func__);
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_CONNECTED,
                                           CONTEXT_TYPE_VOICE);
            }
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC &&
                  (peer_.GetPeerVoiceRxState() != StreamState::CONNECTED)) {
            peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceTxState() == StreamState::CONNECTED) {
              BTIF_TRACE_DEBUG("%s: received connected state from BAP for voice RX, "
                                                             "update state", __func__);
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_CONNECTED,
                                           CONTEXT_TYPE_VOICE);
            }
          }
          if(btif_ahim_is_aosp_aidl_hal_enabled()) {
            if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingStart) &&
                peer_.GetStreamContextType() == CONTEXT_TYPE_VOICE) {
              peer_.ClearFlags(BtifAcmPeer::kFlagPendingStart);
              peer_.ClearFlags(BtifAcmPeer::kFlagPendingSetActiveDev);
              BTIF_TRACE_DEBUG("%s: BAP returned without streaming, ACK MM", __func__);
              tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
              if (pending_cmd == A2DP_CTRL_CMD_STOP || pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
                btif_acm_source_on_suspended(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
              } else if (pending_cmd == A2DP_CTRL_CMD_START) {
                btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
              } else {
               BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
              }
            }
          }
        }
      } else if (p_acm->state_info.stream_state == StreamState::CONNECTING){
          if (contextType == CONTENT_TYPE_MEDIA) {
            BTIF_TRACE_DEBUG("%s: received connecting state from BAP for MEDIA Tx "
                                                          "or Rx, ignore", __func__);
            if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK)
              peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
            else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC)
              peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
          } else if (contextType == CONTENT_TYPE_CONVERSATIONAL) {
            BTIF_TRACE_DEBUG("%s: received connecting state from BAP for CONVERSATIONAL Tx "
                                                                   "or Rx, ignore", __func__);
            if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
              peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
            } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
              peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
            }
          }
      }
      if (btif_peer_device_is_streaming(peer_.SetId()) &&
          !peer_.CheckFlags(BtifAcmPeer::kFlagPendingStart)) {
        if (btif_get_peer_streaming_context_type(peer_.SetId()) != CONTEXT_TYPE_VOICE) {
          LOG(INFO) << __PRETTY_FUNCTION__ << ": current_active_profile_type: "
                                           << current_active_profile_type;
          if ((current_active_profile_type == BAP &&
              peer_.GetPeerMusicTxState() == StreamState::CONNECTED) ||
              (current_active_profile_type == GCP &&
              peer_.GetPeerMusicTxState() == StreamState::CONNECTED) ||
              (current_active_profile_type == WMCP &&
              peer_.GetPeerMusicRxState() == StreamState::CONNECTED) ||
              (current_active_profile_type == GCP_RX &&
              peer_.GetPeerMusicTxState() == StreamState::CONNECTED &&
              peer_.GetPeerMusicRxState() == StreamState::CONNECTED)) {
              BTIF_TRACE_DEBUG("%s: Send ACM Start request for peer if other group mmeber "
                                               "is streaming with Media Context", __func__);
              peer_.SetStreamContextType(CONTEXT_TYPE_MUSIC);
              peer_.SetStreamProfileType(current_active_profile_type);
              btif_acm_initiator_dispatch_sm_event(peer_.PeerAddress(), BTIF_ACM_START_STREAM_REQ_EVT);
          }
        } else {
          if ((peer_.GetPeerVoiceTxState() == StreamState::CONNECTED) &&
              (peer_.GetPeerVoiceRxState() == StreamState::CONNECTED)) {
              BTIF_TRACE_DEBUG("%s: Send ACM Start request for peer if other group mmeber "
                                               "is streaming with Voice Context", __func__);
              peer_.SetStreamContextType(CONTEXT_TYPE_VOICE);
              peer_.SetStreamProfileType(current_active_profile_type);
              btif_acm_initiator_dispatch_sm_event(peer_.PeerAddress(), BTIF_ACM_START_STREAM_REQ_EVT);
          }
        }
      }
    } break;

    case BTA_ACM_DISCONNECT_EVT: {
      int context_type = p_acm->state_info.stream_type.type;
      if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingStart) &&
        (p_acm->state_info.stream_state == StreamState::DISCONNECTING ||
        p_acm->state_info.stream_state == StreamState::DISCONNECTED)) {
        BTIF_TRACE_DEBUG("%s: ACM is disconnected/disconnecting while pending start", __func__);
        peer_.ClearFlags(BtifAcmPeer::kFlagPendingStart);
        if (context_type == CONTENT_TYPE_MEDIA) {
          tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
          if (pending_cmd == A2DP_CTRL_CMD_START) {
            btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
          }
        }
      }

      if (p_acm->state_info.stream_state == StreamState::DISCONNECTED) {
        if (context_type == CONTENT_TYPE_MEDIA) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
          }
          if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
              peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
            btif_report_connection_state(peer_.PeerAddress(),
                BTACM_CONNECTION_STATE_DISCONNECTED, CONTEXT_TYPE_MUSIC);
          }
          if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
            BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnected state from BAP"
                      " when Voice Tx+Rx & Media Rx/Tx was disconnected, send pacs "
                      "disconnect", __func__);
            if (!sPacsClientInterface) break;
            sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
            peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
          } else {
            CodecConfig config_;
            memset(&config_, 0, sizeof(CodecConfig));
            peer_.set_peer_media_codec_config(config_);
            BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnected state from BAP"
                      " when either Voice Tx or Rx or Media Rx/Tx is connected, "
                      "remain in opened state", __func__);
          }
        } else if (context_type == CONTENT_TYPE_CONVERSATIONAL) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED) {
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_DISCONNECTED,
                                           CONTEXT_TYPE_VOICE);
              if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
                  peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Tx,"
                                 " voice Rx, Music Tx & Rx are disconnected send pacs "
                                 "disconnect", __func__);
                if (!sPacsClientInterface) break;
                sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Tx,"
                                 " voice Rx is disconnected but music Tx or Rx still not "
                                 "disconnected, remain in opened state", __func__);
              }
            }
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED) {
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_DISCONNECTED,
                                           CONTEXT_TYPE_VOICE);
              if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
                  peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                 " voice Tx, Music Tx & Rx are disconnected send pacs "
                                 "disconnect", __func__);
                if (!sPacsClientInterface) break;
                sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                 " voice Rx is disconnected but music Tx or Rx still not "
                                 "disconnected, remain in opened state", __func__);
              }
            }
          }
        }
      } else if (p_acm->state_info.stream_state == StreamState::DISCONNECTING) {
        if (context_type == CONTENT_TYPE_MEDIA) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
          }
          btif_report_connection_state(peer_.PeerAddress(),
                                       BTACM_CONNECTION_STATE_DISCONNECTING,
                                       CONTEXT_TYPE_MUSIC);

          if ((peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED ||
                peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTING) &&
               (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED ||
                peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTING) &&
               (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED ||
                peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING) &&
               (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED ||
                peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING)) {
              BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnecting state from BAP"
                               " when Voice Tx+Rx and Media Rx/Tx disconnected/ing, "
                               "move in closing state", __func__);
              peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
          } else {
            BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnecting state from BAP"
                             " when either Voice Tx or Rx or Media Rx/Tx is connected,"
                             " remain in opened state", __func__);

            std::vector<StreamType> disconnect_streams;
            if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING &&
                peer_.GetPeerMusicRxState() == StreamState::CONNECTED) {
              BTIF_TRACE_DEBUG("%s: Received disconnecting for Music-Tx, initiate for Rx also",
                                __func__);
              StreamType type_1;
              type_1 = {.type = CONTENT_TYPE_MEDIA,
                        .audio_context = CONTENT_TYPE_LIVE | CONTENT_TYPE_GAME,
                        .direction = ASE_DIRECTION_SRC
                       };
              disconnect_streams.push_back(type_1);

              if (!sUcastClientInterface) break;
              sUcastClientInterface->Disconnect(peer_.PeerAddress(), disconnect_streams);
            }

            if (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING &&
                peer_.GetPeerMusicTxState() == StreamState::CONNECTED) {
              BTIF_TRACE_DEBUG("%s: Received disconnecting for Music-Rx, initiate for Tx also",
                                __func__);
              StreamType type_1;
              type_1 = {.type = CONTENT_TYPE_MEDIA,
                        .audio_context = CONTENT_TYPE_MEDIA | CONTENT_TYPE_GAME,
                        .direction = ASE_DIRECTION_SINK
                       };
              disconnect_streams.push_back(type_1);

              if (!sUcastClientInterface) break;
              sUcastClientInterface->Disconnect(peer_.PeerAddress(), disconnect_streams);
            }
          }
        } else if (context_type == CONTENT_TYPE_CONVERSATIONAL) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTING ||
                peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED) {
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_DISCONNECTING,
                                           CONTEXT_TYPE_VOICE);
              if (((peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED) ||
                  (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING)) &&
                  ((peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) ||
                  (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING))) {
                BTIF_TRACE_DEBUG("%s: received disconnecting state from BAP for voice Tx,"
                                 " voice Rx, music Tx+Rx are disconnected/ing move in "
                                 "closing state", __func__);
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else {
                BTIF_TRACE_DEBUG("%s: received disconnecting state from BAP for voice Tx,"
                                 " voice Rx is disconncted/ing but music Tx or Rx still "
                                 "not disconnected/ing, remain in opened state", __func__);
              }
            }
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTING ||
                peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED) {
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_DISCONNECTING,
                                           CONTEXT_TYPE_VOICE);
              if (((peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED) ||
                  (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING)) &&
                  ((peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) ||
                  (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING))) {
                BTIF_TRACE_DEBUG("%s: received disconnecting state from BAP for voice Rx,"
                                 " voice Tx, music Tx+Rx are disconnected/ing move in "
                                 "closing state", __func__);
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                 " voice Tx is disconncted/ing but music Tx or Rx still "
                                 "not disconnected/ing, remain in opened state", __func__);
              }
            }
          }
        }
      }
    }
    break;

    case BTIF_ACM_RECONFIG_REQ_EVT: {
        BTIF_TRACE_DEBUG("%s: Stream_Context_type: %d, reconfig_context_type: %d, reconfig profile type: %d",
                         __PRETTY_FUNCTION__, peer_.GetStreamContextType(),
                         btif_acm_bap_to_acm_context(
                         p_acm->acm_reconfig.streams_info.stream_type.audio_context), peer_.GetRcfgProfileType());
        if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingStart) &&
            peer_.GetStreamContextType() == btif_acm_bap_to_acm_context(
            p_acm->acm_reconfig.streams_info.stream_type.audio_context)) {
          BTIF_TRACE_DEBUG("%s: Ignore Reconfig req, and handle "
                            "when move to started state", __PRETTY_FUNCTION__);
          peer_.SetFlags(BtifAcmPeer::kFLagPendingReconfigAfterStart);
          break;
        }

        std::vector<StreamReconfig> reconf_streams;
        StreamReconfig reconf_info;
        CodecQosConfig cfg;

        std::string config;
        memset(&cfg, 0, sizeof(CodecQosConfig));
        memset(&reconf_info, 0, sizeof(StreamReconfig));

        BTIF_TRACE_DEBUG("%s: prev reconfig profile: %d",
                         __PRETTY_FUNCTION__, peer_.GetRcfgProfileType());
        if (p_acm->acm_reconfig.streams_info.stream_type.type ==
                                                      CONTENT_TYPE_MEDIA &&
            p_acm->acm_reconfig.streams_info.stream_type.audio_context ==
                                                       CONTENT_TYPE_GAME &&
            p_acm->acm_reconfig.streams_info.stream_type.direction ==
                                                       ASE_DIRECTION_SRC) {

          bool is_gcp_rx_reconf_codec_qos_fetched = false;
          bool is_gcp_tx_reonf_codec_qos_fetched = false;
          BTIF_TRACE_DEBUG("%s: Reconfig for gaming VBC Rx.", __PRETTY_FUNCTION__);
          reconf_info.stream_type.type = CONTENT_TYPE_MEDIA;
          reconf_info.stream_type.audio_context = CONTENT_TYPE_GAME;
          reconf_info.stream_type.direction = ASE_DIRECTION_SRC;
          reconf_info.reconf_type =
                       bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
          is_gcp_rx_reconf_codec_qos_fetched =
                peer_.SetCodecQosConfig(peer_.PeerAddress(), GCP_RX, MEDIA_CONTEXT,
                                             SRC, true, config);
          BTIF_TRACE_DEBUG("%s: is_gcp_rx_reconf_codec_qos_fetched: %d",
                            __func__, is_gcp_rx_reconf_codec_qos_fetched);
          if (is_gcp_rx_reconf_codec_qos_fetched == false) {
            BTIF_TRACE_DEBUG("%s: reconfig gcp Rx codecQos config not fetched, return", __func__);
            return false;
          }

          cfg = peer_.get_peer_voice_rx_codec_qos_config();
          print_codec_parameters(cfg.codec_config);
          print_qos_parameters(cfg.qos_config);
          reconf_info.codec_qos_config_pair.push_back(cfg);
          reconf_streams.push_back(reconf_info);

          BTIF_TRACE_DEBUG("%s: Reconfig for gaming VBC Tx.", __PRETTY_FUNCTION__);
          memset(&cfg, 0, sizeof(CodecQosConfig));
          memset(&reconf_info, 0, sizeof(StreamReconfig));
          reconf_info.stream_type.type = CONTENT_TYPE_MEDIA;
          reconf_info.stream_type.audio_context = CONTENT_TYPE_GAME;
          reconf_info.stream_type.direction = ASE_DIRECTION_SINK;
          reconf_info.reconf_type =
                       bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
          is_gcp_tx_reonf_codec_qos_fetched =
                      peer_.SetCodecQosConfig(peer_.PeerAddress(), GCP_TX, MEDIA_CONTEXT,
                                               SNK, true, config);
          BTIF_TRACE_DEBUG("%s: is_gcp_tx_reonf_codec_qos_fetched: %d",
                            __func__, is_gcp_tx_reonf_codec_qos_fetched);
          if (is_gcp_tx_reonf_codec_qos_fetched == false) {
            BTIF_TRACE_DEBUG("%s: reconfig gcp Tx codecQos config not fetched, return", __func__);
            return false;
          }

          cfg = peer_.get_peer_media_codec_qos_config();
          print_codec_parameters(cfg.codec_config);
          print_qos_parameters(cfg.qos_config);
          reconf_info.codec_qos_config_pair.push_back(cfg);
          reconf_streams.push_back(reconf_info);
          peer_.SetRcfgProfileType(GCP_RX);
        } else if (p_acm->acm_reconfig.streams_info.stream_type.type ==
                                                          CONTENT_TYPE_MEDIA) {
          if (p_acm->acm_reconfig.streams_info.reconf_type ==
                bluetooth::bap::ucast::StreamReconfigType::APP_TRIGGERED_CONFIG) {
            BTIF_TRACE_IMP("%s: DEV-UI/ALS Triggered Reconfig for BAP media ", __PRETTY_FUNCTION__);
            reconf_info.reconf_type =
                             bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
            reconf_info.stream_type.type =
                             p_acm->acm_reconfig.streams_info.stream_type.type;
            reconf_info.stream_type.audio_context =
                             p_acm->acm_reconfig.streams_info.stream_type.audio_context;
            reconf_info.stream_type.direction =
                             p_acm->acm_reconfig.streams_info.stream_type.direction;
            cfg = peer_.get_peer_media_codec_qos_config();
            print_codec_parameters(cfg.codec_config);
            print_qos_parameters(cfg.qos_config);
            reconf_info.codec_qos_config_pair.push_back(cfg);
            reconf_streams.push_back(reconf_info);
            peer_.SetRcfgProfileType(BAP);
          } else if (p_acm->acm_reconfig.streams_info.stream_type.audio_context ==
                                                           CONTENT_TYPE_GAME &&
              p_acm->acm_reconfig.streams_info.stream_type.direction ==
                                                           ASE_DIRECTION_SINK) {
            bool is_gcp_tx_only_reconf_codec_qos_fetched = false;
            BTIF_TRACE_DEBUG("%s: Reconfig for gaming Tx only", __PRETTY_FUNCTION__);
            reconf_info.stream_type.type =
                             p_acm->acm_reconfig.streams_info.stream_type.type;
            reconf_info.stream_type.audio_context =
                             p_acm->acm_reconfig.streams_info.stream_type.audio_context;
            reconf_info.stream_type.direction =
                             p_acm->acm_reconfig.streams_info.stream_type.direction;
            reconf_info.reconf_type =
                             bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
            is_gcp_tx_only_reconf_codec_qos_fetched =
                            peer_.SetCodecQosConfig(peer_.PeerAddress(), GCP,
                                                    MEDIA_CONTEXT, SNK, false, config);
            BTIF_TRACE_DEBUG("%s: is_gcp_tx_only_reconf_codec_qos_fetched: %d",
                            __func__, is_gcp_tx_only_reconf_codec_qos_fetched);
            if (is_gcp_tx_only_reconf_codec_qos_fetched == false) {
              BTIF_TRACE_DEBUG("%s: reconfig gcp Tx only codecQos config not fetched,"
                                                       " return", __func__);
              return false;
            }

            cfg = peer_.get_peer_media_codec_qos_config();
            print_codec_parameters(cfg.codec_config);
            print_qos_parameters(cfg.qos_config);
            reconf_info.codec_qos_config_pair.push_back(cfg);
            reconf_streams.push_back(reconf_info);
            peer_.SetRcfgProfileType(GCP);
          } else if (p_acm->acm_reconfig.streams_info.stream_type.audio_context ==
                                                                CONTENT_TYPE_MEDIA &&
                     p_acm->acm_reconfig.streams_info.stream_type.direction ==
                                                                ASE_DIRECTION_SINK) {
            bool is_media_tx_reconf_codec_qos_fetched = false;
            BTIF_TRACE_DEBUG("%s: Reconfig for BAP media.", __PRETTY_FUNCTION__);
            reconf_info.stream_type.type =
                             p_acm->acm_reconfig.streams_info.stream_type.type;
            reconf_info.stream_type.audio_context =
                             p_acm->acm_reconfig.streams_info.stream_type.audio_context;
            reconf_info.stream_type.direction =
                             p_acm->acm_reconfig.streams_info.stream_type.direction;
            reconf_info.reconf_type =
                             bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
            is_media_tx_reconf_codec_qos_fetched =
                           peer_.SetCodecQosConfig(peer_.PeerAddress(), BAP,
                                                   MEDIA_CONTEXT, SNK, false, config);
            BTIF_TRACE_DEBUG("%s: is_media_tx_reconf_codec_qos_fetched: %d",
                            __func__, is_media_tx_reconf_codec_qos_fetched);
            if (is_media_tx_reconf_codec_qos_fetched == false) {
              BTIF_TRACE_DEBUG("%s: reconfig media Tx codecQos config not fetched,"
                                                                " return", __func__);
              return false;
            }

            cfg = peer_.get_peer_media_codec_qos_config();
            print_codec_parameters(cfg.codec_config);
            print_qos_parameters(cfg.qos_config);
            reconf_info.codec_qos_config_pair.push_back(cfg);
            reconf_streams.push_back(reconf_info);
            peer_.SetRcfgProfileType(BAP);
          } else if (p_acm->acm_reconfig.streams_info.stream_type.audio_context ==
                                                                 CONTENT_TYPE_LIVE &&
                     p_acm->acm_reconfig.streams_info.stream_type.direction ==
                                                                 ASE_DIRECTION_SRC) {
            bool is_media_rx_reconf_codec_qos_fetched = false;
            BTIF_TRACE_DEBUG("%s: Reconfig for live recording.", __PRETTY_FUNCTION__);
            reconf_info.stream_type.type =
                             p_acm->acm_reconfig.streams_info.stream_type.type;
            reconf_info.stream_type.audio_context =
                             p_acm->acm_reconfig.streams_info.stream_type.audio_context;
            reconf_info.stream_type.direction =
                             p_acm->acm_reconfig.streams_info.stream_type.direction;
            reconf_info.reconf_type =
                             bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
            is_media_rx_reconf_codec_qos_fetched =
                                peer_.SetCodecQosConfig(peer_.PeerAddress(), WMCP,
                                                        MEDIA_CONTEXT, SRC, false, config);
            BTIF_TRACE_DEBUG("%s: is_media_rx_reconf_codec_qos_fetched: %d",
                            __func__, is_media_rx_reconf_codec_qos_fetched);
            if (is_media_rx_reconf_codec_qos_fetched == false) {
              BTIF_TRACE_DEBUG("%s: reconfig media Rx codecQos config not fetched,"
                                                                  " return", __func__);
              return false;
            }

            cfg = peer_.get_peer_media_codec_qos_config();
            print_codec_parameters(cfg.codec_config);
            print_qos_parameters(cfg.qos_config);
            reconf_info.codec_qos_config_pair.push_back(cfg);
            reconf_streams.push_back(reconf_info);
            peer_.SetRcfgProfileType(WMCP);
          }
        } else {
          uint16_t num_sink_ases, num_src_ases;
          BTIF_TRACE_DEBUG("%s: Reconfig for BAP CALL Rx.", __PRETTY_FUNCTION__);
          btif_bap_get_params(peer_.PeerAddress(), nullptr, nullptr, &num_sink_ases,
                            &num_src_ases, nullptr, nullptr);
          if(num_src_ases) {
            bool is_voice_rx_reconf_codec_qos_fetched = false;
            reconf_info.stream_type.type = CONTENT_TYPE_CONVERSATIONAL;
            reconf_info.stream_type.audio_context = CONTENT_TYPE_CONVERSATIONAL;
            reconf_info.stream_type.direction = ASE_DIRECTION_SRC;
            reconf_info.reconf_type =
                      bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
            is_voice_rx_reconf_codec_qos_fetched =
                    peer_.SetCodecQosConfig(peer_.PeerAddress(), BAP,
                                    VOICE_CONTEXT, SRC, true, config);
            BTIF_TRACE_DEBUG("%s: is_voice_rx_reconf_codec_qos_fetched: %d",
                            __func__, is_voice_rx_reconf_codec_qos_fetched);
            if (is_voice_rx_reconf_codec_qos_fetched == false) {
              BTIF_TRACE_DEBUG("%s: reconfig voice Rx codecQos config not fetched,"
                                                               " return", __func__);
              return false;
            }
            cfg = peer_.get_peer_voice_rx_codec_qos_config();
            print_codec_parameters(cfg.codec_config);
            print_qos_parameters(cfg.qos_config);
            reconf_info.codec_qos_config_pair.push_back(cfg);
            reconf_streams.push_back(reconf_info);
            peer_.SetPeerVoiceRxState(StreamState::RECONFIGURING);
          }

          memset(&cfg, 0, sizeof(CodecQosConfig));
          memset(&reconf_info, 0, sizeof(StreamReconfig));

          BTIF_TRACE_DEBUG("%s: Reconfig for BAP CALL Tx.", __PRETTY_FUNCTION__);
          if(num_sink_ases) {
            bool is_voice_tx_reconf_codec_qos_fetched = false;
            reconf_info.stream_type.type = CONTENT_TYPE_CONVERSATIONAL;
            reconf_info.stream_type.audio_context = CONTENT_TYPE_CONVERSATIONAL;
            reconf_info.stream_type.direction = ASE_DIRECTION_SINK;
            reconf_info.reconf_type =
                     bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
            is_voice_tx_reconf_codec_qos_fetched =
                             peer_.SetCodecQosConfig(peer_.PeerAddress(), BAP,
                                    VOICE_CONTEXT, SNK, true, config);
            BTIF_TRACE_DEBUG("%s: is_voice_tx_reconf_codec_qos_fetched: %d",
                            __func__, is_voice_tx_reconf_codec_qos_fetched);
            if (is_voice_tx_reconf_codec_qos_fetched == false) {
              BTIF_TRACE_DEBUG("%s: reconfig voice Tx codecQos config not fetched,"
                                                                 "return", __func__);
              return false;
            }
            cfg = peer_.get_peer_voice_tx_codec_qos_config();
            print_codec_parameters(cfg.codec_config);
            print_qos_parameters(cfg.qos_config);
            reconf_info.codec_qos_config_pair.push_back(cfg);
            reconf_streams.push_back(reconf_info);
            peer_.SetPeerVoiceTxState(StreamState::RECONFIGURING);
          }
          peer_.SetRcfgProfileType(BAP_CALL);
        }

        if (!sUcastClientInterface) break;
          sUcastClientInterface->Reconfigure(peer_.PeerAddress(), reconf_streams);

        peer_.SetFlags(BtifAcmPeer::kFlagPendingReconfigure);
        peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateReconfiguring);
    }
    break;

    case BTA_ACM_CONFIG_EVT: {
       tBTIF_ACM* p_acm_data = (tBTIF_ACM*)p_data;
       uint16_t contextType = p_acm_data->state_info.stream_type.type;
       uint16_t peer_latency_ms = 0;
       uint32_t presen_delay = 0;
       bool is_update_require = false;
       uint32_t audio_location = p_acm_data->config_info.audio_location;
       BTIF_TRACE_DEBUG("%s: audio location is %d", __PRETTY_FUNCTION__, audio_location);
       if (p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
         peer_.set_peer_audio_src_location(audio_location);
       } else if (p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
         peer_.set_peer_audio_sink_location(audio_location);
       }
       if (contextType == CONTENT_TYPE_MEDIA) {
         if (p_acm_data->state_info.stream_type.audio_context == CONTENT_TYPE_MEDIA) {
           BTIF_TRACE_DEBUG("%s: compare with current media config", __PRETTY_FUNCTION__);
           CodecConfig peer_config = peer_.get_peer_media_codec_config();
           is_update_require = compare_codec_config_(peer_config,
                                                     p_acm_data->config_info.codec_config);
         } else if (p_acm_data->state_info.stream_type.audio_context == CONTENT_TYPE_LIVE) {
           BTIF_TRACE_DEBUG("%s: cache current_recording_config", __PRETTY_FUNCTION__);
           current_recording_config = p_acm_data->config_info.codec_config;
         } else if (p_acm_data->state_info.stream_type.audio_context == CONTENT_TYPE_GAME) {
          //TODO
           if (p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
             BTIF_TRACE_DEBUG("%s: compare current gaming Tx config", __PRETTY_FUNCTION__);
             CodecConfig peer_config = peer_.get_peer_media_codec_config();
             is_update_require = compare_codec_config_(peer_config,
                                                       p_acm_data->config_info.codec_config);
           } else if (p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SRC){
             BTIF_TRACE_DEBUG("%s: cache current gaming Rx config", __PRETTY_FUNCTION__);
             current_gaming_rx_config = p_acm_data->config_info.codec_config;
           }
         }

         if (mandatory_codec_selected) {
           BTIF_TRACE_DEBUG("%s: Mandatory codec selected, do not store config", __PRETTY_FUNCTION__);
         } else {
           BTIF_TRACE_DEBUG("%s: store configuration", __PRETTY_FUNCTION__);
         }

         //Cache the peer latency in WMCP case
         if (p_acm_data->state_info.stream_type.audio_context == CONTENT_TYPE_LIVE) {
           BTIF_TRACE_DEBUG("%s: presentation delay[0] = %x", __func__,
                            p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[0]);
           BTIF_TRACE_DEBUG("%s: presentation delay[1] = %x", __func__,
                            p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[1]);
           BTIF_TRACE_DEBUG("%s: presentation delay[2] = %x", __func__,
                            p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[2]);
           presen_delay = static_cast<uint32_t>(p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[0]) |
                          static_cast<uint32_t>(p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[1] << 8) |
                          static_cast<uint32_t>(p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[2] << 16);
           BTIF_TRACE_DEBUG("%s: presen_delay = %dus", __func__, presen_delay);
           peer_latency_ms = presen_delay/1000;
           BTIF_TRACE_DEBUG("%s: s_to_m latency = %dms", __func__,
                           p_acm_data->config_info.qos_config.cig_config.max_tport_latency_s_to_m);
           peer_latency_ms += p_acm_data->config_info.qos_config.cig_config.max_tport_latency_s_to_m;
           peer_.SetPeerLatency(peer_latency_ms);
           BTIF_TRACE_DEBUG("%s: cached peer Latency = %dms", __func__, peer_.GetPeerLatency());
         }

         BTIF_TRACE_DEBUG("%s: is_update_require: %d",
                                        __PRETTY_FUNCTION__, is_update_require);
         if (is_update_require) {
           current_media_config = p_acm_data->config_info.codec_config;
           BTIF_TRACE_DEBUG("%s: is_update_require is true, current_media_config's codec_type = %d ",
                                  __func__, current_media_config.codec_type);
           peer_.set_peer_media_codec_config(current_media_config);
           BTIF_TRACE_DEBUG("%s: current_media_config.codec_specific_3: %"
                                 PRIi64, __func__, current_media_config.codec_specific_3);
           btif_acm_report_source_codec_state(peer_.PeerAddress(), current_media_config,
                                              unicast_codecs_capabilities_snk,
                                              peer_.GetSnkSelectableCapability(), CONTEXT_TYPE_MUSIC);
         }
       } else if (contextType == CONTENT_TYPE_CONVERSATIONAL &&
                  p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
         BTIF_TRACE_DEBUG("%s: cache current_voice_config", __PRETTY_FUNCTION__);
         current_voice_config = p_acm_data->config_info.codec_config;
         BTIF_TRACE_DEBUG("%s: current_voice_config.codec_specific_3: %"
                               PRIi64, __func__, current_voice_config.codec_specific_3);
         btif_acm_update_lc3q_params(&current_voice_config.codec_specific_3, p_acm_data);
         btif_acm_report_source_codec_state(peer_.PeerAddress(), current_voice_config,
                                            unicast_codecs_capabilities_src,
                                            peer_.GetSrcSelectableCapability(), CONTEXT_TYPE_VOICE);
       }
      //Handle BAP START if reconfig comes in mid of streaming
      //peer_.SetStreamReconfigInfo(p_acm->acm_reconfig);
      //TODO: local capabilities
      //CodecConfig record = p_bta_data->acm_reconfig.codec_config;
      //saving codec config as negotiated parameter as true
      //btif_pacs_add_record(peer_.PeerAddress(), true, CodecDirection::CODEC_DIR_SRC, &record);

    } break;

    case BTA_ACM_CONN_UPDATE_TIMEOUT_EVT:
      peer_.SetConnUpdateMode(BtifAcmPeer::kFlagRelaxedMode);
      break;

    default:
      BTIF_TRACE_WARNING("%s: Peer %s : Unhandled event=%s",
                         __PRETTY_FUNCTION__,
                         peer_.PeerAddress().ToString().c_str(),
                         BtifAcmEvent::EventName(event).c_str());
      return false;
  }
  return true;
}

bool btif_acm_check_if_requested_devices_started() {
  std::vector<RawAddress>::iterator itr;
  if ((btif_acm_initiator.locked_devices).size() > 0) {
    for (itr = (btif_acm_initiator.locked_devices).begin();
                    itr != (btif_acm_initiator.locked_devices).end(); itr++) {
      BTIF_TRACE_DEBUG("%s: address =%s", __func__, *itr->ToString().c_str());
      BtifAcmPeer* peer = btif_acm_initiator.FindPeer(*itr);
      if ((peer == nullptr) || (peer != nullptr && !peer->IsStreaming())) {
        break;
      }
    }

    if (itr == (btif_acm_initiator.locked_devices).end()) {
      return true;
    }
  }
  return false;
}

bool btif_acm_check_if_requested_devices_stopped() {
  std::vector<RawAddress>::iterator itr;
  if ((btif_acm_initiator.locked_devices).size() > 0) {
    for (itr = (btif_acm_initiator.locked_devices).begin();
                     itr != (btif_acm_initiator.locked_devices).end(); itr++) {
      BTIF_TRACE_DEBUG("%s: address =%s", __func__, *itr->ToString().c_str());
      BtifAcmPeer* peer = btif_acm_initiator.FindPeer(*itr);
      if ((peer == nullptr) || (peer != nullptr /*&& !peer->IsSuspended()*/)) {
        break;
      }
    }

    if (itr == (btif_acm_initiator.locked_devices).end()) {
      return true;
    }
  }
  return false;
}

void BtifAcmStateMachine::StateStarted::OnEnter() {
  BTIF_TRACE_DEBUG("%s: Peer %s, Peer SetId = %d,"
                   " MusicActiveSetId = %d, ContextType = %d",
                   __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str(),
                   peer_.SetId(), btif_acm_initiator.MusicActiveCSetId(),
                   peer_.GetContextType());

  if(btif_acm_initiator.IsConnUpdateEnabled()) {
    //Starting the timer for 5 seconds before moving to relaxed state as
    //stop event or start streaming event moght immediately come
    //which requires aggresive interval
    btif_acm_check_and_start_conn_Interval_timer(&peer_);
  }
  bool start_ack = false;
  // Report that we have entered the Streaming stage. Usually, this should
  // be followed by focus grant. See update_audio_focus_state()
  btif_report_audio_state(peer_.PeerAddress(), BTACM_AUDIO_STATE_STARTED,
                                               peer_.GetStreamContextType());
  if (alarm_is_scheduled(btif_acm_initiator.AcmGroupProcedureTimer())) {
    btif_acm_check_and_cancel_group_procedure_timer(btif_acm_initiator.MusicActiveCSetId());
    tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
    if (pending_cmd == A2DP_CTRL_CMD_STOP ||
       pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
      btif_acm_source_on_suspended(A2DP_CTRL_ACK_SUCCESS);
    } else if (pending_cmd == A2DP_CTRL_CMD_START) {
      start_ack = true;
      btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
    } else {
      BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
    }
  } else {
    BTIF_TRACE_DEBUG("%s:no group procedure timer running ACK pending cmd", __func__);
    tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
    if (pending_cmd == A2DP_CTRL_CMD_STOP ||
       pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
      btif_acm_source_on_suspended(A2DP_CTRL_ACK_SUCCESS);
    } else if (pending_cmd == A2DP_CTRL_CMD_START) {
      start_ack = true;
      btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
    } else {
      BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
    }
  }

  BTIF_TRACE_DEBUG("%s: start_ack %d", __func__, start_ack);
  BTIF_TRACE_DEBUG("%s: current_active_context: %d, current_active_profile: %d",
      __func__, current_active_context_type, current_active_profile_type);
  if (start_ack) {
    std::vector<StreamUpdate> update_streams;
    uint16_t update_value = DO_NOT_CHANGE;
    if (current_active_context_type == CONTEXT_TYPE_MUSIC) {
      if (current_active_profile_type == BAP) {
        StreamType type = {.type = CONTENT_TYPE_MEDIA,
                .audio_context = CONTENT_TYPE_MEDIA,
                .direction = ASE_DIRECTION_SINK
               };
        update_value = MODE_HIGH_QUALITY;
        BTIF_TRACE_DEBUG("%s: update HQ mode", __func__);
        StreamUpdate stream = {.stream_type = type,
                               .update_type =
                                 bluetooth::bap::ucast::StreamUpdateType::ENCODER_MODE_UPDATE,
                               .update_value = update_value
                              };
        update_streams.push_back(stream);
      } else if (current_active_profile_type == GCP ||
                 current_active_profile_type == GCP_RX) {
        StreamType type = {.type = CONTENT_TYPE_MEDIA,
                .audio_context = CONTENT_TYPE_GAME,
                .direction = ASE_DIRECTION_SINK
               };
        update_value = MODE_LOW_LATENCY_0;
        BTIF_TRACE_DEBUG("%s: update HQ mode", __func__);
        StreamUpdate stream = {.stream_type = type,
                               .update_type =
                                 bluetooth::bap::ucast::StreamUpdateType::ENCODER_MODE_UPDATE,
                               .update_value = update_value
                              };
        update_streams.push_back(stream);
      }

      if (sUcastClientInterface)
        sUcastClientInterface->UpdateStream(peer_.PeerAddress(), update_streams);
    }
  }

  BTIF_TRACE_DEBUG("%s: kFlagPendingReconfigure: %d, ReconfigProfileType: %d",
                   __PRETTY_FUNCTION__,
                   peer_.CheckFlags(BtifAcmPeer::kFLagPendingReconfigAfterStart),
                   peer_.GetRcfgProfileType());

  if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingSetActiveDev)) {
    peer_.ClearFlags(BtifAcmPeer::kFlagPendingSetActiveDev);
  }

  if (peer_.CheckFlags(BtifAcmPeer::kFLagPendingReconfigAfterStart)) {
    peer_.ClearFlags(BtifAcmPeer::kFLagPendingReconfigAfterStart);
    reconfig_acm_initiator(peer_.PeerAddress(), peer_.GetRcfgProfileType(), false);
  }
}

void BtifAcmStateMachine::StateStarted::OnExit() {
  BTIF_TRACE_DEBUG("%s: Peer %s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str());
}

bool BtifAcmStateMachine::StateStarted::ProcessEvent(uint32_t event, void* p_data) {
  tBTIF_ACM* p_acm = (tBTIF_ACM*)p_data;

  BTIF_TRACE_DEBUG("%s: Peer: %s : event=%s, flags=%s, music_active_peer=%s,"
                   " voice_active_peer=%s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str(),
                   BtifAcmEvent::EventName(event).c_str(),
                   peer_.FlagsToString().c_str(),
                   logbool(peer_.IsPeerActiveForMusic()).c_str(),
                   logbool(peer_.IsPeerActiveForVoice()).c_str());

  switch (event) {
    case BTIF_ACM_CONNECT_REQ_EVT: {
      ConnectionState pacs_state = peer_.GetPeerPacsState();
      if (pacs_state == ConnectionState::CONNECTED) {
        LOG_INFO(LOG_TAG, "%s PACS connected, initiate BAP connect", __PRETTY_FUNCTION__);
        bool can_connect = true;
        // Check whether connection is allowed
        if (peer_.IsAcceptor()) {
          //There is no char in current spec. Should we check VMCP role here?
          // shall we assume VMCP role would have been checked in apps and no need to check here?
          can_connect = btif_acm_initiator.AllowedToConnect(peer_.PeerAddress());
          if (!can_connect) disconnect_acm_initiator(peer_.PeerAddress(),
                                                     current_active_context_type);
        }
        if (!can_connect) {
          BTIF_TRACE_ERROR(
              "%s: Cannot connect to peer %s: too many connected "
              "peers",
              __PRETTY_FUNCTION__, peer_.PeerAddress().ToString().c_str());
          break;
        }
        std::vector<StreamConnect> streams;
        if (current_active_context_type == CONTEXT_TYPE_MUSIC) {
          StreamConnect conn_media;
          if (peer_.GetProfileType() & (BAP|GCP)) {
            bool is_m_tx_fetched = false;
            //keeping media tx as BAP/GCP config
            memset(&conn_media, 0, sizeof(conn_media));
            is_m_tx_fetched = fetch_media_tx_codec_qos_config(peer_.PeerAddress(),
                                            peer_.GetProfileType() & (BAP|GCP),
                                            &conn_media);
            BTIF_TRACE_ERROR("%s: is_m_tx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_m_tx_fetched);
            if (is_m_tx_fetched == false) {
              BTIF_TRACE_ERROR("%s: media tx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          }
          if (peer_.GetProfileType() & WMCP) {
            bool is_m_rx_fetched = false;
            //keeping media rx as WMCP config
            memset(&conn_media, 0, sizeof(conn_media));
            is_m_rx_fetched = fetch_media_rx_codec_qos_config(peer_.PeerAddress(),
                                                              WMCP, &conn_media);
            BTIF_TRACE_ERROR("%s: is_m_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_m_rx_fetched);
            if (is_m_rx_fetched == false) {
              BTIF_TRACE_ERROR("%s: media rx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          }
        } else if (current_active_context_type == CONTEXT_TYPE_MUSIC_VOICE) {
          StreamConnect conn_media, conn_voice;
          //keeping voice tx as BAP config
          uint16_t num_sink_ases, num_src_ases;
          btif_bap_get_params(peer_.PeerAddress(), nullptr, nullptr,
                              &num_sink_ases, &num_src_ases,
                              nullptr, nullptr);

          if(num_sink_ases) {
            bool is_vm_v_tx_fetched = false;
            memset(&conn_voice, 0, sizeof(conn_voice));
            is_vm_v_tx_fetched = fetch_voice_tx_codec_qos_config(peer_.PeerAddress(),
                                                              BAP, &conn_voice);
            BTIF_TRACE_ERROR("%s: is_vm_v_tx_fetched: %d",
                                     __PRETTY_FUNCTION__, is_vm_v_tx_fetched);
            if (is_vm_v_tx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm voice tx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_voice);
            }
          }

          //keeping voice rx as BAP config
          if(num_src_ases) {
            bool is_vm_v_rx_fetched = false;
            memset(&conn_voice, 0, sizeof(conn_voice));
            is_vm_v_rx_fetched = fetch_voice_rx_codec_qos_config(peer_.PeerAddress(),
                                                                 BAP, &conn_voice);
            BTIF_TRACE_ERROR("%s: is_vm_v_rx_fetched: %d",
                                     __PRETTY_FUNCTION__, is_vm_v_rx_fetched);
            if (is_vm_v_rx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm voice rx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_voice);
            }
          }

          if (peer_.GetProfileType() & (BAP|GCP)) {
            bool is_vm_m_tx_fetched = false;
            //keeping media tx as BAP/GCP config
            memset(&conn_media, 0, sizeof(conn_media));
            is_vm_m_tx_fetched = fetch_media_tx_codec_qos_config(peer_.PeerAddress(),
                                            peer_.GetProfileType() & (BAP|GCP),
                                            &conn_media);
            BTIF_TRACE_ERROR("%s: is_vm_m_tx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_vm_m_tx_fetched);
            if (is_vm_m_tx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm media tx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          }
          if (peer_.GetProfileType() & WMCP) {
            bool is_vm_m_rx_fetched = false;
            //keeping media rx as WMCP config
            memset(&conn_media, 0, sizeof(conn_media));
            is_vm_m_rx_fetched = fetch_media_rx_codec_qos_config(peer_.PeerAddress(),
                                                              WMCP, &conn_media);
            BTIF_TRACE_ERROR("%s: is_vm_m_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_vm_m_rx_fetched);
            if (is_vm_m_rx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm media rx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          }
        } else if (current_active_context_type == CONTEXT_TYPE_VOICE) {
          bool is_v_tx_fetched = false;
          StreamConnect conn_voice;
          //keeping voice tx as BAP config
          memset(&conn_voice, 0, sizeof(conn_voice));
          is_v_tx_fetched = fetch_voice_tx_codec_qos_config(peer_.PeerAddress(),
                                                            BAP, &conn_voice);
          BTIF_TRACE_ERROR("%s: is_v_tx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_v_tx_fetched);
          if (is_v_tx_fetched == false) {
            BTIF_TRACE_ERROR("%s: voice tx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_voice);
          }

          bool is_v_rx_fetched = false;
          //keeping voice rx as BAP config
          memset(&conn_voice, 0, sizeof(conn_voice));
          is_v_rx_fetched = fetch_voice_rx_codec_qos_config(peer_.PeerAddress(),
                                                               BAP, &conn_voice);
          BTIF_TRACE_ERROR("%s: is_v_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_v_rx_fetched);
          if (is_v_rx_fetched == false) {
            BTIF_TRACE_ERROR("%s: voice rx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_voice);
          }
        }
        LOG(WARNING) << __func__ << ": size of streams: " << streams.size();
        if (!sUcastClientInterface) break;

        if (streams.size() == 0) {
          LOG_DEBUG(LOG_TAG, "%s: Streams null, Don't send connect req to BAP",
                                               __PRETTY_FUNCTION__);
          break;
        }

        std::vector<RawAddress> address;
        address.push_back(peer_.PeerAddress());
        sUcastClientInterface->Connect(address, true, streams);
      }
    } break;

    case BTIF_ACM_STOP_STREAM_REQ_EVT:
    case BTIF_ACM_SUSPEND_STREAM_REQ_EVT: {
      LOG_INFO(LOG_TAG, "%s: Peer %s : event=%s flags=%s", __PRETTY_FUNCTION__,
               peer_.PeerAddress().ToString().c_str(),
               BtifAcmEvent::EventName(event).c_str(),
               peer_.FlagsToString().c_str());
      if (peer_.CheckFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend)) {
        BTIF_TRACE_DEBUG("%s: peer is waiting the other peer suspend", __func__);
        break;
      }
      peer_.SetFlags(BtifAcmPeer::kFlagPendingLocalSuspend);

      StreamType type_1, type_2;
      std::vector<StreamType> stop_streams;
      tBTIF_ACM_STOP_REQ * p_bta_data = (tBTIF_ACM_STOP_REQ*)p_data;
      uint16_t stop_profile_type = p_bta_data->stop_profile_type;
      if (peer_.GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
        BTIF_TRACE_DEBUG("%s: stop_profile_type: %d",
                        __PRETTY_FUNCTION__, stop_profile_type);
        if (stop_profile_type == GCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          stop_streams.push_back(type_1);
        } else if (stop_profile_type == GCP_RX) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SRC
                   };
          type_2 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          stop_streams.push_back(type_1);
          stop_streams.push_back(type_2);
        } else if (stop_profile_type == WMCP){
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_LIVE,
                    .direction = ASE_DIRECTION_SRC
                   };
          stop_streams.push_back(type_1);
        } else {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_MEDIA,
                    .direction = ASE_DIRECTION_SINK
                   };
          stop_streams.push_back(type_1);
        }
      } else if (peer_.GetStreamContextType() == CONTEXT_TYPE_VOICE) {
        //StreamType type_2;
        type_1 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                  .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                  .direction = ASE_DIRECTION_SINK
                 };
        type_2 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                  .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                  .direction = ASE_DIRECTION_SRC
                 };
        stop_streams.push_back(type_2);
        stop_streams.push_back(type_1);
      }
      if(btif_acm_initiator.IsConnUpdateEnabled()) {
        //Cancel the timer if start streamng comes before
        // 5 seconds while moving the interval to relaxed mode.
        if (alarm_is_scheduled(btif_acm_initiator.AcmConnIntervalTimer())) {
           btif_acm_check_and_cancel_conn_Interval_timer();
        }
        else {
          peer_.SetConnUpdateMode(BtifAcmPeer::kFlagAggresiveMode);
        }
      }

      if (!sUcastClientInterface) break;
        sUcastClientInterface->Stop(peer_.PeerAddress(), stop_streams);

      if(event == BTIF_ACM_STOP_STREAM_REQ_EVT) {
        tBTIF_ACM_STOP_REQ * p_bta_data = (tBTIF_ACM_STOP_REQ*)p_data;
        if(p_bta_data->wait_for_signal) {
          p_bta_data->peer_ready_promise->set_value();
        }
        LOG_ERROR(LOG_TAG, "%s BTIF_ACM_STOP_STREAM_REQ_EVT from BTIF ",
                                               __PRETTY_FUNCTION__);
      }
    }
    break;

    case BTIF_ACM_DISCONNECT_REQ_EVT: {
      int contextType = p_acm->state_info.stream_type.type;
      LOG_INFO(LOG_TAG, "%s: Peer %s : event=%s flags=%s contextType=%d",
               __PRETTY_FUNCTION__,
               peer_.PeerAddress().ToString().c_str(),
               BtifAcmEvent::EventName(event).c_str(),
               peer_.FlagsToString().c_str(), contextType);

      tBTIF_ACM_CONN_DISC* p_bta_data = (tBTIF_ACM_CONN_DISC*)p_data;
      std::vector<StreamType> disconnect_streams;
      if (p_bta_data->contextType == CONTEXT_TYPE_MUSIC) {
        StreamType type_1;
        if (p_bta_data->profileType & BAP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_MEDIA,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & WMCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_LIVE,
                    .direction = ASE_DIRECTION_SRC
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & GCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        StreamType type_2;
        if (p_bta_data->profileType & GCP_RX) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SRC
                   };
          type_2 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
          disconnect_streams.push_back(type_2);
        }
      } else if (p_bta_data->contextType == CONTEXT_TYPE_MUSIC_VOICE) {
        StreamType type_2 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SRC
                            };
        StreamType type_3 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SINK
                            };
        disconnect_streams.push_back(type_3);
        disconnect_streams.push_back(type_2);
        StreamType type_1;
        if (p_bta_data->profileType & BAP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_MEDIA,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & WMCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_LIVE,
                    .direction = ASE_DIRECTION_SRC
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & GCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        StreamType type_4;
        if (p_bta_data->profileType & GCP_RX) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SRC
                   };
          type_4 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
          disconnect_streams.push_back(type_4);
        }
      } else if (p_bta_data->contextType == CONTEXT_TYPE_VOICE) {
        StreamType type_2 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SRC
                            };
        StreamType type_3 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SINK
                            };
        disconnect_streams.push_back(type_3);
        disconnect_streams.push_back(type_2);
      }
      LOG(WARNING) << __func__
                   << " size of disconnect_streams " << disconnect_streams.size();
      if (!sUcastClientInterface) break;
      sUcastClientInterface->Disconnect(peer_.PeerAddress(), disconnect_streams);

      // Inform the application that we are disconnecting
      if ((p_bta_data->contextType == CONTEXT_TYPE_MUSIC) &&
          ((peer_.GetPeerVoiceRxState() == StreamState::CONNECTED) ||
           (peer_.GetPeerVoiceTxState() == StreamState::CONNECTED))) {
        if (peer_.GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
          btif_report_audio_state(peer_.PeerAddress(),
                                  BTACM_AUDIO_STATE_STOPPED,
                                  CONTEXT_TYPE_MUSIC);
          LOG(WARNING) << __func__ << " voice connected move in opened state ";
          peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
        } else
          LOG(WARNING) << __func__ << " Voice is streaming, remain in started ";
      } else if ((p_bta_data->contextType == CONTEXT_TYPE_VOICE) &&
                 ((peer_.GetPeerMusicTxState() == StreamState::CONNECTED) ||
                  (peer_.GetPeerMusicRxState() == StreamState::CONNECTED))) {
        if (peer_.GetStreamContextType() == CONTEXT_TYPE_VOICE) {
          btif_report_audio_state(peer_.PeerAddress(),
                                  BTACM_AUDIO_STATE_STOPPED,
                                  CONTEXT_TYPE_VOICE);
          LOG(WARNING) << __func__ << " Music connected move in opened state ";
          peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
        } else
          LOG(WARNING) << __func__ << " Music is streaming, remain in started ";
      } else {
        btif_report_audio_state(peer_.PeerAddress(),
                                BTACM_AUDIO_STATE_STOPPED,
                                peer_.GetStreamContextType());
        LOG(WARNING) << __func__ << " Move in closing state ";
        peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
      }
    }
    break;

    case BTA_ACM_STOP_EVT: {
      int contextType = p_acm->state_info.stream_type.type;
      LOG_INFO(LOG_TAG, "%s: Peer %s : event=%s flags=%s", __PRETTY_FUNCTION__,
               peer_.PeerAddress().ToString().c_str(),
               BtifAcmEvent::EventName(event).c_str(),
               peer_.FlagsToString().c_str());
      if (contextType == CONTENT_TYPE_MEDIA) {
        BTIF_TRACE_DEBUG("%s: STOPPING event came from BAP for Media, ignore", __func__);
      } else if (contextType == CONTENT_TYPE_CONVERSATIONAL) {
        BTIF_TRACE_DEBUG("%s: STOPPING event came from BAP for Voice, ignore", __func__);
      }
    }
    break;
    case BTIF_ACM_START_STREAM_REQ_EVT:  {
      if(btif_ahim_is_aosp_aidl_hal_enabled()) {
        tA2DP_CTRL_CMD pending_cmd = A2DP_CTRL_CMD_START;
        //btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
        if (pending_cmd == A2DP_CTRL_CMD_START) {
          btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
        } else {
          BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
        }
      }
    } break;

    case BTA_ACM_DISCONNECT_EVT: {
      int context_type = p_acm->state_info.stream_type.type;
      if (p_acm->state_info.stream_state == StreamState::DISCONNECTED) {
        if (context_type == CONTENT_TYPE_MEDIA) {
          //Need to check whether below logic sufficient for VBC too.
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
          }
          if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
              peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
            btif_report_connection_state(peer_.PeerAddress(),
                BTACM_CONNECTION_STATE_DISCONNECTED, CONTEXT_TYPE_MUSIC);
          }
          if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
            BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnected state from BAP"
                      " when Voice Tx+Rx & Media Rx/Tx was disconnected, "
                      "send pacs disconnect", __func__);
            if (!sPacsClientInterface) break;
            sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
            peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
          } else {
            BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnected state from BAP"
                      " when either Voice Tx or Rx or Media Rx/Tx is connected, "
                      "remain in started state", __func__);
          }
        } else if (context_type == CONTENT_TYPE_CONVERSATIONAL) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED) {
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_DISCONNECTED,
                                           CONTEXT_TYPE_VOICE);

              if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
                  peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Tx,"
                                 " voice Rx, Music Tx & Rx are disconnected"
                                 " send pacs disconnect", __func__);
                if (!sPacsClientInterface) break;
                sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Tx,"
                                 " voice Rx is disconnected but music Tx or "
                                 "Rx still not disconnected,"
                                 " remain in started state", __func__);
              }
            }
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED) {
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_DISCONNECTED,
                                           CONTEXT_TYPE_VOICE);

              if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
                  peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                 " voice Tx, Music Tx & Rx are disconnected "
                                 "send pacs disconnect", __func__);
                if (!sPacsClientInterface) break;
                sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                 " voice Rx is disconnected but music Tx or "
                                 "Rx still not disconnected,"
                                 " remain in started state", __func__);
              }
            }
          }
        }
      } else if (p_acm->state_info.stream_state == StreamState::DISCONNECTING) {
        if (context_type == CONTENT_TYPE_MEDIA) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
          }

          if ((peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED ||
                peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTING) &&
               (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED ||
                peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTING) &&
               (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED ||
                peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING) &&
               (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED ||
                peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING)) {
              BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnecting state from BAP"
                               " when Voice Tx+Rx and Media Rx/Tx disconnected/ing, "
                               "move in closing state", __func__);
              peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
          } else {
              if (peer_.GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
                std::vector<StreamType> disconnect_streams;
                btif_report_audio_state(peer_.PeerAddress(),
                                        BTACM_AUDIO_STATE_STOPPED,
                                        CONTEXT_TYPE_MUSIC);

                BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnecting state "
                                 "from BAP while streaming when either "
                                 "Voice Tx or Rx or Media Rx/Tx is connected, "
                                 "move to opened state", __func__);

                if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING &&
                    peer_.GetPeerMusicRxState() == StreamState::CONNECTED) {
                  BTIF_TRACE_DEBUG("%s: Received disconnecting for Music-Tx, "
                                   "initiate for Rx also", __func__);
                  StreamType type_1;
                  type_1 = {.type = CONTENT_TYPE_MEDIA,
                            .audio_context = CONTENT_TYPE_LIVE | CONTENT_TYPE_GAME,
                            .direction = ASE_DIRECTION_SRC
                           };
                  disconnect_streams.push_back(type_1);

                  if (!sUcastClientInterface) break;
                  sUcastClientInterface->Disconnect(peer_.PeerAddress(), disconnect_streams);
                }

                if (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING &&
                    peer_.GetPeerMusicTxState() == StreamState::CONNECTED) {
                  BTIF_TRACE_DEBUG("%s: Received disconnecting for Music-Rx, "
                                   "initiate for Tx also", __func__);
                  StreamType type_1;
                  type_1 = {.type = CONTENT_TYPE_MEDIA,
                            .audio_context = CONTENT_TYPE_MEDIA | CONTENT_TYPE_GAME,
                            .direction = ASE_DIRECTION_SINK
                           };
                  disconnect_streams.push_back(type_1);

                  if (!sUcastClientInterface) break;
                  sUcastClientInterface->Disconnect(peer_.PeerAddress(), disconnect_streams);
                }
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
              } else {
                BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnecting state from BAP"
                    " when either Voice Tx or Rx or Media Rx/Tx is connected, "
                    "remain in started state", __func__);
              }
          }
          btif_report_connection_state(peer_.PeerAddress(),
                                       BTACM_CONNECTION_STATE_DISCONNECTING,
                                       CONTEXT_TYPE_MUSIC);
        } else if (context_type == CONTENT_TYPE_CONVERSATIONAL) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTING ||
                peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED) {
              if (((peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED) ||
                  (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING)) &&
                  ((peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) ||
                  (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING))) {
                BTIF_TRACE_DEBUG("%s: received disconnecting state from BAP for voice Tx,"
                                 " voice Rx, music Tx+Rx are disconnected/ing "
                                 "move in closing state", __func__);
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else {
                if (peer_.GetStreamContextType() == CONTEXT_TYPE_VOICE) {
                  btif_report_audio_state(peer_.PeerAddress(),
                                          BTACM_AUDIO_STATE_STOPPED,
                                          CONTEXT_TYPE_VOICE);
                  BTIF_TRACE_DEBUG("%s: received disconnecting state from BAP "
                                   "for voice Tx while streaming, voice Rx is disconncted/ing"
                                   " but music Tx or Rx still not disconnected/ing,"
                                   " move to opened state", __func__);
                  peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
                } else {
                  BTIF_TRACE_DEBUG("%s: received disconnecting state from BAP for voice Tx,"
                                   " voice Rx is disconncted/ing but music Tx or "
                                   "Rx still not disconnected/ing,"
                                   " remain in started state", __func__);
                }
              }
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_DISCONNECTING,
                                           CONTEXT_TYPE_VOICE);
            }
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTING ||
                peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED) {
              if (((peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED) ||
                  (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING)) &&
                  ((peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) ||
                  (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING))) {
                BTIF_TRACE_DEBUG("%s: received disconnecting state from BAP for "
                                 "voice Rx, voice Tx, music Tx+Rx are disconnected/ing "
                                 "move in closing state", __func__);
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else {
                if (peer_.GetStreamContextType() == CONTEXT_TYPE_VOICE) {
                  btif_report_audio_state(peer_.PeerAddress(),
                                          BTACM_AUDIO_STATE_STOPPED,
                                          CONTEXT_TYPE_VOICE);
                  BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx "
                                   "while streaming, voice Tx is disconncted/ing but music "
                                   "Tx or Rx still not disconnected/ing,"
                                   " move to Opened state", __func__);
                  peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
                } else {
                  BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                   " voice Tx is disconncted/ing but music Tx or "
                                   "Rx still not disconnected/ing,"
                                   " remain in started state", __func__);
                }
              }
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_DISCONNECTING,
                                           CONTEXT_TYPE_VOICE);
            }
          }
        }
      }
    }
    break;

    case BTA_ACM_CONNECT_EVT: {// above evnt can come and handle for voice/media case
      int contextType = p_acm->state_info.stream_type.type;
      LOG_INFO(
          LOG_TAG, "%s: Peer %s : event=%s context=%d",
          __PRETTY_FUNCTION__, peer_.PeerAddress().ToString().c_str(),
          BtifAcmEvent::EventName(event).c_str(), contextType);
      LOG_INFO(
          LOG_TAG, "%s: context=%d, converted=%d, Streaming context=%d",
          __PRETTY_FUNCTION__, contextType, btif_acm_bap_to_acm_context(contextType),
          peer_.GetStreamContextType());

      if (btif_acm_bap_to_acm_context(contextType) != peer_.GetStreamContextType()) {
        if (contextType == CONTENT_TYPE_MEDIA) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            if (p_acm->state_info.stream_state == StreamState::CONNECTED) {
              BTIF_TRACE_DEBUG("%s: received connected state from BAP for Music Rx, "
                                                              "update state", __func__);
              peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
            } else if (p_acm->state_info.stream_state == StreamState::CONNECTING){
              BTIF_TRACE_DEBUG("%s: received connecting state from BAP for Music Rx, "
                                                                    "ignore", __func__);
              //To check, why we are updating
              peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
            }
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            if (p_acm->state_info.stream_state == StreamState::CONNECTED) {
              BTIF_TRACE_DEBUG("%s: received connected state from BAP for Music Tx, "
                                                             "update state", __func__);
              peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
            } else if (p_acm->state_info.stream_state == StreamState::CONNECTING){
              BTIF_TRACE_DEBUG("%s: received connecting state from BAP for Music Tx, "
                                                                    "ignore", __func__);
              peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
            }

            //Both Game Rx and Tx has been connected update to App.
            if (p_acm->state_info.stream_type.audio_context == CONTENT_TYPE_GAME) {
              BTIF_TRACE_DEBUG("%s: received connecting state from BAP for Game Tx, "
                                                            "report to App", __func__);
              if (p_acm->state_info.stream_state == StreamState::CONNECTED)
                btif_report_connection_state(peer_.PeerAddress(),
                        BTACM_CONNECTION_STATE_CONNECTED, CONTEXT_TYPE_MUSIC);
            }
          }

          if (p_acm->state_info.stream_type.audio_context != CONTENT_TYPE_GAME) {
            if (p_acm->state_info.stream_state == StreamState::CONNECTED)
              btif_report_connection_state(peer_.PeerAddress(),
                      BTACM_CONNECTION_STATE_CONNECTED, CONTEXT_TYPE_MUSIC);
          } else {
            // For gaming need to update when Sink update comes from BAP
          }
        } else if (contextType == CONTENT_TYPE_CONVERSATIONAL) {
          if (p_acm->state_info.stream_state == StreamState::CONNECTED) {
            BTIF_TRACE_DEBUG("%s: voice context connected, remain in started state",
                                                                         __func__);
            if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
              peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
              if (peer_.GetPeerVoiceRxState() == StreamState::CONNECTED) {
                BTIF_TRACE_DEBUG("%s: received connected state from BAP for voice Tx, "
                                                              "update state", __func__);
                btif_report_connection_state(peer_.PeerAddress(),
                                             BTACM_CONNECTION_STATE_CONNECTED,
                                             CONTEXT_TYPE_VOICE);
              }
            } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
              peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
              if (peer_.GetPeerVoiceTxState() == StreamState::CONNECTED) {
                BTIF_TRACE_DEBUG("%s: received connected state from BAP for voice Rx, "
                                                              "update state", __func__);
                btif_report_connection_state(peer_.PeerAddress(),
                                             BTACM_CONNECTION_STATE_CONNECTED,
                                             CONTEXT_TYPE_VOICE);
              }
            }
          } else if (p_acm->state_info.stream_state == StreamState::CONNECTING) {
            BTIF_TRACE_DEBUG("%s: received connecting state from BAP "
                             "for voice Tx or Rx, ignore", __func__);
            if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
              peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
            } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
              peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
            }
          }
        }
      } else {
        BtifAcmPeer *other_peer = btif_get_peer_device (&peer_);
        if(other_peer &&
           peer_.CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend) &&
           other_peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend)) {
          peer_.ClearFlags(BtifAcmPeer::kFlagPendingLocalSuspend);
          peer_.SetFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend);
          BTIF_TRACE_DEBUG("%s: peer device is not suspended, wait for it", __func__);
          break;
        } else {
          if(other_peer &&
           peer_.CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend) &&
           other_peer->CheckFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend)) {
            BTIF_TRACE_DEBUG("%s: peer device also suspended", __func__);
            tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
            if (pending_cmd == A2DP_CTRL_CMD_STOP ||
              pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
              btif_acm_source_on_suspended(A2DP_CTRL_ACK_SUCCESS);
            } else if (pending_cmd == A2DP_CTRL_CMD_START) {
              btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
            } else {
              BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
            }
            BTIF_TRACE_DEBUG("%s: report STOP to apps and move to Opened", __func__);
            btif_report_audio_state(other_peer->PeerAddress(),
                                    BTACM_AUDIO_STATE_STOPPED,
                                    other_peer->GetStreamContextType());
            other_peer->StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
            other_peer->ClearFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend);
            btif_report_audio_state(peer_.PeerAddress(),
                                    BTACM_AUDIO_STATE_STOPPED,
                                    peer_.GetStreamContextType());
            peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
            peer_.ClearFlags(BtifAcmPeer::kFlagPendingLocalSuspend);
          } else if(peer_.CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend)) {
            peer_.ClearFlags(BtifAcmPeer::kFlagPendingLocalSuspend);
            BTIF_TRACE_DEBUG("%s: peer device is suspended, send MM any pending ACK", __func__);
            tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
            if (pending_cmd == A2DP_CTRL_CMD_STOP ||
              pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
              btif_acm_source_on_suspended(A2DP_CTRL_ACK_SUCCESS);
            } else if (pending_cmd == A2DP_CTRL_CMD_START) {
              btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
            } else {
              BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
            }
            BTIF_TRACE_DEBUG("%s: report STOP to apps and move to Opened", __func__);
            btif_report_audio_state(peer_.PeerAddress(),
                                    BTACM_AUDIO_STATE_STOPPED,
                                    peer_.GetStreamContextType());
            peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
          }
        }
      }

      if (alarm_is_scheduled(btif_acm_initiator.AcmGroupProcedureTimer()))
        btif_acm_check_and_cancel_group_procedure_timer(
                                     btif_acm_initiator.MusicActiveCSetId());

    } break;

    case BTIF_ACM_RECONFIG_REQ_EVT: {
        BTIF_TRACE_DEBUG("%s: sending stop to BAP before reconfigure", __func__);
        if(!btif_ahim_is_aosp_aidl_hal_enabled()) {
          btif_acm_source_end_session(active_bda);
        } else {
          btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_STREAM_SUSPENDED,
                                   current_active_profile_type);
        }
        peer_.SetFlags(BtifAcmPeer::kFlagPendingLocalSuspend);
        StreamType type_1, type_2;
        std::vector<StreamType> stop_streams;
        if (peer_.GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
          if (current_active_profile_type == GCP_RX) {
            type_1 = {.type = CONTENT_TYPE_MEDIA,
                      .audio_context = CONTENT_TYPE_GAME,
                      .direction = ASE_DIRECTION_SRC
                     };
            type_2 = {.type = CONTENT_TYPE_MEDIA,
                      .audio_context = CONTENT_TYPE_GAME,
                      .direction = ASE_DIRECTION_SINK
                     };
            stop_streams.push_back(type_1);
            stop_streams.push_back(type_2);
          } else if (current_active_profile_type == GCP) {
            type_1 = {.type = CONTENT_TYPE_MEDIA,
                      .audio_context = CONTENT_TYPE_GAME,
                      .direction = ASE_DIRECTION_SINK
                     };
            stop_streams.push_back(type_1);
          } else if (current_active_profile_type == WMCP) {
            type_1 = {.type = CONTENT_TYPE_MEDIA,
                      .audio_context = CONTENT_TYPE_LIVE,
                      .direction = ASE_DIRECTION_SRC
                     };
            stop_streams.push_back(type_1);
          } else {
            type_1 = {.type = CONTENT_TYPE_MEDIA,
                      .audio_context = CONTENT_TYPE_MEDIA,
                      .direction = ASE_DIRECTION_SINK
                     };
            stop_streams.push_back(type_1);
          }
        } else if (peer_.GetStreamContextType() == CONTEXT_TYPE_VOICE) {
          type_1 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                    .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                    .direction = ASE_DIRECTION_SINK
                   };
          type_2 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                    .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                    .direction = ASE_DIRECTION_SRC
                   };
          stop_streams.push_back(type_2);
          stop_streams.push_back(type_1);
        }

        if (!sUcastClientInterface) break;
          sUcastClientInterface->Stop(peer_.PeerAddress(), stop_streams);

        if (p_acm->acm_reconfig.streams_info.stream_type.type ==
                                                      CONTENT_TYPE_MEDIA &&
            p_acm->acm_reconfig.streams_info.stream_type.audio_context ==
                                                       CONTENT_TYPE_GAME &&
            p_acm->acm_reconfig.streams_info.stream_type.direction ==
                                                       ASE_DIRECTION_SRC) {
          BTIF_TRACE_DEBUG("%s: Set Reconfig_profile to GCP_RX", __PRETTY_FUNCTION__);
          peer_.SetRcfgProfileType(GCP_RX);
        } else if (p_acm->acm_reconfig.streams_info.stream_type.type ==
                                                          CONTENT_TYPE_MEDIA) {
          if (p_acm->acm_reconfig.streams_info.stream_type.audio_context ==
                                                           CONTENT_TYPE_GAME &&
              p_acm->acm_reconfig.streams_info.stream_type.direction ==
                                                           ASE_DIRECTION_SINK) {
            BTIF_TRACE_DEBUG("%s: Set Reconfig_profile to GCP", __PRETTY_FUNCTION__);
            peer_.SetRcfgProfileType(GCP);
          } else if (p_acm->acm_reconfig.streams_info.stream_type.audio_context ==
                                                                 CONTENT_TYPE_LIVE &&
                     p_acm->acm_reconfig.streams_info.stream_type.direction ==
                                                                 ASE_DIRECTION_SRC) {
            BTIF_TRACE_DEBUG("%s: Set Reconfig_profile to WMCP", __PRETTY_FUNCTION__);
            peer_.SetRcfgProfileType(WMCP);
          } else if (p_acm->acm_reconfig.streams_info.stream_type.audio_context ==
                                                                CONTENT_TYPE_MEDIA &&
                     p_acm->acm_reconfig.streams_info.stream_type.direction ==
                                                                ASE_DIRECTION_SINK) {
            BTIF_TRACE_DEBUG("%s: Set Reconfig_profile to BAP", __PRETTY_FUNCTION__);
            peer_.SetRcfgProfileType(BAP);
          }
        } else if (p_acm->acm_reconfig.streams_info.stream_type.type ==
                                                      CONTENT_TYPE_CONVERSATIONAL) {
          BTIF_TRACE_DEBUG("%s: Set Reconfig_profile to BAP_CALL", __PRETTY_FUNCTION__);
          peer_.SetRcfgProfileType(BAP_CALL);
        }

        peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateReconfiguring);
    }
    break;

    case BTA_ACM_CONFIG_EVT: {
       tBTIF_ACM* p_acm_data = (tBTIF_ACM*)p_data;
       uint16_t contextType = p_acm_data->state_info.stream_type.type;
       uint16_t peer_latency_ms = 0;
       uint32_t presen_delay = 0;
       bool is_update_require = false;
       uint32_t audio_location = p_acm_data->config_info.audio_location;
       BTIF_TRACE_DEBUG("%s: audio location is %d", __PRETTY_FUNCTION__, audio_location);
       if (p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
         peer_.set_peer_audio_src_location(audio_location);
       } else if (p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
         peer_.set_peer_audio_sink_location(audio_location);
       }

       if (contextType == CONTENT_TYPE_MEDIA) {
         if (p_acm_data->state_info.stream_type.audio_context == CONTENT_TYPE_MEDIA) {
           BTIF_TRACE_DEBUG("%s: compare with current media config", __PRETTY_FUNCTION__);
           is_update_require = compare_codec_config_(current_media_config, p_acm_data->config_info.codec_config);
         } else if (p_acm_data->state_info.stream_type.audio_context == CONTENT_TYPE_LIVE) {
           BTIF_TRACE_DEBUG("%s: cache current_recording_config", __PRETTY_FUNCTION__);
           current_recording_config = p_acm_data->config_info.codec_config;
         } else if (p_acm_data->state_info.stream_type.audio_context == CONTENT_TYPE_GAME) {
          //TODO
           if (p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
             BTIF_TRACE_DEBUG("%s: compare current gaming Tx config", __PRETTY_FUNCTION__);
             is_update_require = compare_codec_config_(current_media_config, p_acm_data->config_info.codec_config);
           } else if (p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SRC){
             BTIF_TRACE_DEBUG("%s: cache current gaming Rx config", __PRETTY_FUNCTION__);
             current_gaming_rx_config = p_acm_data->config_info.codec_config;
           }
         }

         if (mandatory_codec_selected) {
           BTIF_TRACE_DEBUG("%s: Mandatory codec selected, do not store config", __PRETTY_FUNCTION__);
         } else {
           BTIF_TRACE_DEBUG("%s: store configuration", __PRETTY_FUNCTION__);
         }

         //Cache the peer latency in WMCP case
         if (p_acm_data->state_info.stream_type.audio_context == CONTENT_TYPE_LIVE) {
           BTIF_TRACE_DEBUG("%s: presentation delay[0] = %x", __func__,
                            p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[0]);
           BTIF_TRACE_DEBUG("%s: presentation delay[1] = %x", __func__,
                            p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[1]);
           BTIF_TRACE_DEBUG("%s: presentation delay[2] = %x", __func__,
                            p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[2]);
           presen_delay = static_cast<uint32_t>(p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[0]) |
                          static_cast<uint32_t>(p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[1] << 8) |
                          static_cast<uint32_t>(p_acm_data->config_info.qos_config.ascs_configs[0].presentation_delay[2] << 16);
           BTIF_TRACE_DEBUG("%s: presen_delay = %dus", __func__, presen_delay);
           peer_latency_ms = presen_delay/1000;
           BTIF_TRACE_DEBUG("%s: s_to_m latency = %dms", __func__,
                           p_acm_data->config_info.qos_config.cig_config.max_tport_latency_s_to_m);
           peer_latency_ms += p_acm_data->config_info.qos_config.cig_config.max_tport_latency_s_to_m;
           peer_.SetPeerLatency(peer_latency_ms);
           BTIF_TRACE_DEBUG("%s: cached peer Latency = %dms", __func__, peer_.GetPeerLatency());
         }

         BTIF_TRACE_DEBUG("%s: is_update_require: %d",
                                        __PRETTY_FUNCTION__, is_update_require);
         //TODO, check whether to update about gaming rx config also to app
         if (is_update_require) {
           current_media_config = p_acm_data->config_info.codec_config;
           peer_.set_peer_media_codec_config(current_media_config);
           BTIF_TRACE_DEBUG("%s: current_media_config.codec_specific_3: %"
                                 PRIi64, __func__, current_media_config.codec_specific_3);
           btif_acm_report_source_codec_state(peer_.PeerAddress(), current_media_config,
                                              unicast_codecs_capabilities_snk,
                                              peer_.GetSnkSelectableCapability(), CONTEXT_TYPE_MUSIC);
         }
       } else if (contextType == CONTENT_TYPE_CONVERSATIONAL &&
                  p_acm_data->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
         BTIF_TRACE_DEBUG("%s: cache current_voice_config", __PRETTY_FUNCTION__);
         current_voice_config = p_acm_data->config_info.codec_config;
         BTIF_TRACE_DEBUG("%s: current_voice_config.codec_specific_3: %"
                               PRIi64, __func__, current_voice_config.codec_specific_3);
         btif_acm_update_lc3q_params(&current_voice_config.codec_specific_3, p_acm_data);
         btif_acm_report_source_codec_state(peer_.PeerAddress(), current_voice_config,
                                            unicast_codecs_capabilities_src,
                                            peer_.GetSrcSelectableCapability(), CONTEXT_TYPE_VOICE);
       }
      //Handle BAP START if reconfig comes in mid of streaming
      //peer_.SetStreamReconfigInfo(p_acm->acm_reconfig);
      //TODO: local capabilities
      //CodecConfig record = p_bta_data->acm_reconfig.codec_config;
      //saving codec config as negotiated parameter as true
      //btif_pacs_add_record(peer_.PeerAddress(), true, CodecDirection::CODEC_DIR_SRC, &record);

    } break;

    case BTA_ACM_CONN_UPDATE_TIMEOUT_EVT:
      peer_.SetConnUpdateMode(BtifAcmPeer::kFlagRelaxedMode);
      break;

    case BTA_ACM_LATENCY_UPDATE_EVT: {
      tBTA_ACM_LATENCY_CHANGE_INFO *p = (tBTA_ACM_LATENCY_CHANGE_INFO *)p_acm;
      //uint16_t conn_handle = p->conn_handle;
      uint8_t ft_c_to_p = p->ft_c_to_p;
      BTIF_TRACE_WARNING("%s: status %d ",__func__, p->status);
      BTIF_TRACE_WARNING("%s: conn_handle %d ",__func__, p->conn_handle);
      BTIF_TRACE_WARNING("%s: ft_c_to_p %d ",__func__, p->ft_c_to_p);
      BTIF_TRACE_WARNING("%s: ft_p_to_c %d ",__func__, p->ft_p_to_c);
      BTIF_TRACE_WARNING("%s: nse %d ",__func__, p->nse);
      // Calculate delta due to FT change
      int16_t drift = (peer_.cis_established_base_ft_ - ft_c_to_p) * 10;
      uint16_t updated_latency = btif_acm_get_active_device_latency() + drift;
      peer_.cis_established_base_ft_ = ft_c_to_p;
      peer_.SetPeerLatency(updated_latency);
    } break;
    default:
      BTIF_TRACE_WARNING("%s: Peer %s : Unhandled event=%s",
                         __PRETTY_FUNCTION__,
                         peer_.PeerAddress().ToString().c_str(),
                         BtifAcmEvent::EventName(event).c_str());
      return false;
  }

  return true;
}

void BtifAcmStateMachine::StateReconfiguring::OnEnter() {
  BTIF_TRACE_DEBUG("%s: Peer %s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str());
  if(btif_acm_initiator.IsConnUpdateEnabled()) {
    //Cancel the timer if running if  not, move to aggressive mode
    if (alarm_is_scheduled(btif_acm_initiator.AcmConnIntervalTimer())) {
       btif_acm_check_and_cancel_conn_Interval_timer();
    } else {
       BTIF_TRACE_DEBUG("%s: conn timer not running, push aggressive intervals", __func__);
       peer_.SetConnUpdateMode(BtifAcmPeer::kFlagAggresiveMode);
    }
  }
  uint16_t rcfg_context_type =
                  btif_acm_profile_to_context(peer_.GetRcfgProfileType());
  BTIF_TRACE_DEBUG("%s: rcfg_context_type: %d", __PRETTY_FUNCTION__,
                                                   rcfg_context_type);
  peer_.SetRcfgContextType(rcfg_context_type);
}

void BtifAcmStateMachine::StateReconfiguring::OnExit() {
  BTIF_TRACE_DEBUG("%s: Peer %s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str());
}

bool BtifAcmStateMachine::StateReconfiguring::ProcessEvent(uint32_t event,
                                                           void* p_data) {
  tBTIF_ACM* p_acm = (tBTIF_ACM*)p_data;
  BTIF_TRACE_DEBUG("%s: Peer %s : event=%s flags=%s music_active_peer=%s "
                   "voice_active_peer=%s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str(),
                   BtifAcmEvent::EventName(event).c_str(),
                   peer_.FlagsToString().c_str(),
                   logbool(peer_.IsPeerActiveForMusic()).c_str(),
                   logbool(peer_.IsPeerActiveForVoice()).c_str());

  switch (event) {
    case BTIF_ACM_CONNECT_REQ_EVT: {
      ConnectionState pacs_state = peer_.GetPeerPacsState();
      if (pacs_state == ConnectionState::CONNECTED) {
        LOG_INFO(LOG_TAG, "%s PACS connected, initiate BAP connect", __PRETTY_FUNCTION__);
        bool can_connect = true;
        // Check whether connection is allowed
        if (peer_.IsAcceptor()) {
          //There is no char in current spec. Should we check VMCP role here?
          // shall we assume VMCP role would have been checked in apps and no need to check here?
          can_connect = btif_acm_initiator.AllowedToConnect(peer_.PeerAddress());
          if (!can_connect) disconnect_acm_initiator(peer_.PeerAddress(),
                                                     current_active_context_type);
        }
        if (!can_connect) {
          BTIF_TRACE_ERROR(
              "%s: Cannot connect to peer %s: too many connected "
              "peers",
              __PRETTY_FUNCTION__, peer_.PeerAddress().ToString().c_str());
          break;
        }
        std::vector<StreamConnect> streams;
        if (current_active_context_type == CONTEXT_TYPE_MUSIC) {
          StreamConnect conn_media;
          if (peer_.GetProfileType() & (BAP|GCP)) {
            //keeping media tx as BAP/GCP config
            memset(&conn_media, 0, sizeof(conn_media));
            fetch_media_tx_codec_qos_config(peer_.PeerAddress(),
                                            peer_.GetProfileType() & (BAP|GCP),
                                            &conn_media);
            streams.push_back(conn_media);
          }
          if (peer_.GetProfileType() & WMCP) {
            bool is_m_rx_fetched = false;
            //keeping media rx as WMCP config
            memset(&conn_media, 0, sizeof(conn_media));
            is_m_rx_fetched = fetch_media_rx_codec_qos_config(peer_.PeerAddress(),
                                                              WMCP, &conn_media);
            BTIF_TRACE_ERROR("%s: is_m_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_m_rx_fetched);
            if (is_m_rx_fetched == false) {
              BTIF_TRACE_ERROR("%s: media rx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          }
        } else if (current_active_context_type == CONTEXT_TYPE_MUSIC_VOICE) {
          StreamConnect conn_media, conn_voice;
          //keeping voice tx as BAP config
          uint16_t num_sink_ases, num_src_ases;
          btif_bap_get_params(peer_.PeerAddress(), nullptr, nullptr,
                              &num_sink_ases, &num_src_ases,
                              nullptr, nullptr);

          if(num_sink_ases) {
            bool is_vm_v_tx_fetched = false;
            memset(&conn_voice, 0, sizeof(conn_voice));
            is_vm_v_tx_fetched = fetch_voice_tx_codec_qos_config(peer_.PeerAddress(),
                                                              BAP, &conn_voice);
            BTIF_TRACE_ERROR("%s: is_vm_v_tx_fetched: %d",
                                     __PRETTY_FUNCTION__, is_vm_v_tx_fetched);
            if (is_vm_v_tx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm voice tx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_voice);
            }
          }

          //keeping voice rx as BAP config
          if(num_src_ases) {
            bool is_vm_v_rx_fetched = false;
            memset(&conn_voice, 0, sizeof(conn_voice));
            is_vm_v_rx_fetched = fetch_voice_rx_codec_qos_config(peer_.PeerAddress(),
                                                                 BAP, &conn_voice);
            BTIF_TRACE_ERROR("%s: is_vm_v_rx_fetched: %d",
                                     __PRETTY_FUNCTION__, is_vm_v_rx_fetched);
            if (is_vm_v_rx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm voice rx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_voice);
            }
          }

          if (peer_.GetProfileType() & (BAP|GCP)) {
            bool is_vm_m_tx_fetched = false;
            //keeping media tx as BAP/GCP config
            memset(&conn_media, 0, sizeof(conn_media));
            is_vm_m_tx_fetched = fetch_media_tx_codec_qos_config(peer_.PeerAddress(),
                                            peer_.GetProfileType() & (BAP|GCP),
                                            &conn_media);
            BTIF_TRACE_ERROR("%s: is_vm_m_tx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_vm_m_tx_fetched);
            if (is_vm_m_tx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm media tx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          }
          if (peer_.GetProfileType() & WMCP) {
            bool is_vm_m_rx_fetched = false;
            //keeping media rx as WMCP config
            memset(&conn_media, 0, sizeof(conn_media));
            is_vm_m_rx_fetched = fetch_media_rx_codec_qos_config(peer_.PeerAddress(),
                                                              WMCP, &conn_media);
            BTIF_TRACE_ERROR("%s: is_vm_m_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_vm_m_rx_fetched);
            if (is_vm_m_rx_fetched == false) {
              BTIF_TRACE_ERROR("%s: vm media rx config not fetched", __PRETTY_FUNCTION__);
            } else {
              streams.push_back(conn_media);
            }
          }
        } else if (current_active_context_type == CONTEXT_TYPE_VOICE) {
          bool is_v_tx_fetched = false;
          StreamConnect conn_voice;
          //keeping voice tx as BAP config
          memset(&conn_voice, 0, sizeof(conn_voice));
          is_v_tx_fetched = fetch_voice_tx_codec_qos_config(peer_.PeerAddress(),
                                                            BAP, &conn_voice);
          BTIF_TRACE_ERROR("%s: is_v_tx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_v_tx_fetched);
          if (is_v_tx_fetched == false) {
            BTIF_TRACE_ERROR("%s: voice tx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_voice);
          }

          bool is_v_rx_fetched = false;
          //keeping voice rx as BAP config
          memset(&conn_voice, 0, sizeof(conn_voice));
          is_v_rx_fetched = fetch_voice_rx_codec_qos_config(peer_.PeerAddress(),
                                                               BAP, &conn_voice);
          BTIF_TRACE_ERROR("%s: is_v_rx_fetched: %d",
                                   __PRETTY_FUNCTION__, is_v_rx_fetched);
          if (is_v_rx_fetched == false) {
            BTIF_TRACE_ERROR("%s: voice rx config not fetched", __PRETTY_FUNCTION__);
          } else {
            streams.push_back(conn_voice);
          }
        }
        LOG(WARNING) << __func__ << ": size of streams: " << streams.size();
        if (!sUcastClientInterface) break;

        if (streams.size() == 0) {
          LOG_DEBUG(LOG_TAG, "%s: Streams null, Don't send connect req to BAP",
                                               __PRETTY_FUNCTION__);
          break;
        }

        std::vector<RawAddress> address;
        address.push_back(peer_.PeerAddress());
        sUcastClientInterface->Connect(address, true, streams);
      }
    } break;

    case BTIF_ACM_DISCONNECT_REQ_EVT:{
      tBTIF_ACM_CONN_DISC* p_bta_data = (tBTIF_ACM_CONN_DISC*)p_data;
      std::vector<StreamType> disconnect_streams;
      BtifAcmPeer *other_peer = btif_get_peer_device (&peer_);
      if (other_peer == nullptr || (!other_peer->CheckFlags(BtifAcmPeer::kFlagPendingReconfigure) &&
                          !other_peer->CheckFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig) &&
                          !other_peer->CheckFlags(BtifAcmPeer::kFlagPendingStart) &&
                          !other_peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend))) {
        tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
        if (pending_cmd == A2DP_CTRL_CMD_START) {
          btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
        } else if (pending_cmd == A2DP_CTRL_CMD_SUSPEND ||
                   pending_cmd == A2DP_CTRL_CMD_STOP) {
          btif_acm_source_on_suspended(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
        }
      }
      if (p_bta_data->contextType == CONTEXT_TYPE_MUSIC) {
        StreamType type_1;
        if (p_bta_data->profileType & BAP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_MEDIA,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & WMCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_LIVE,
                    .direction = ASE_DIRECTION_SRC
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & GCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        StreamType type_2;
        if (p_bta_data->profileType & GCP_RX) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SRC
                   };
          type_2 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
          disconnect_streams.push_back(type_2);
        }
      } else if (p_bta_data->contextType == CONTEXT_TYPE_MUSIC_VOICE) {
        StreamType type_2 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SRC
                            };
        StreamType type_3 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SINK
                            };
        disconnect_streams.push_back(type_3);
        disconnect_streams.push_back(type_2);
        StreamType type_1;
        if (p_bta_data->profileType & BAP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_MEDIA,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & WMCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_LIVE,
                    .direction = ASE_DIRECTION_SRC
                   };
          disconnect_streams.push_back(type_1);
        }

        if (p_bta_data->profileType & GCP) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
        }

        StreamType type_4;
        if (p_bta_data->profileType & GCP_RX) {
          type_1 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SRC
                   };
          type_4 = {.type = CONTENT_TYPE_MEDIA,
                    .audio_context = CONTENT_TYPE_GAME,
                    .direction = ASE_DIRECTION_SINK
                   };
          disconnect_streams.push_back(type_1);
          disconnect_streams.push_back(type_4);
        }
      } else if (p_bta_data->contextType == CONTEXT_TYPE_VOICE) {
        StreamType type_2 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SRC
                            };
        StreamType type_3 = {.type = CONTENT_TYPE_CONVERSATIONAL,
                             .audio_context = CONTENT_TYPE_CONVERSATIONAL,
                             .direction = ASE_DIRECTION_SINK
                            };
        disconnect_streams.push_back(type_3);
        disconnect_streams.push_back(type_2);
      }

      LOG(WARNING) << __func__
                   << " size of disconnect_streams " << disconnect_streams.size();
      if (!sUcastClientInterface) break;
      sUcastClientInterface->Disconnect(peer_.PeerAddress(), disconnect_streams);

      if ((p_bta_data->contextType == CONTEXT_TYPE_MUSIC) &&
          ((peer_.GetPeerVoiceRxState() == StreamState::CONNECTED) ||
           (peer_.GetPeerVoiceTxState() == StreamState::CONNECTED))) {
        if (peer_.GetRcfgProfileType() == BAP_CALL) {
          LOG(WARNING) << __func__ << " Voice is reconfiguring, remain in reconfiguring ";
        } else {
          peer_.ClearFlags(BtifAcmPeer::kFlagPendingReconfigure);
          peer_.ClearFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
          peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
          LOG(WARNING) << __func__ << " Voice is connected, move in open ";
        }
      } else if ((p_bta_data->contextType == CONTEXT_TYPE_VOICE) &&
                 ((peer_.GetPeerMusicTxState() == StreamState::CONNECTED) ||
                  (peer_.GetPeerMusicRxState() == StreamState::CONNECTED))) {
        if (peer_.GetRcfgProfileType() == BAP_CALL) {
          peer_.ClearFlags(BtifAcmPeer::kFlagPendingReconfigure);
          peer_.ClearFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
          peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
          LOG(WARNING) << __func__ << " Music is connected, move in open ";
        } else
          LOG(WARNING) << __func__ << " Music is reconfiguring, remain in reconfiguring";
      } else {
        peer_.ClearFlags(BtifAcmPeer::kFlagPendingReconfigure);
        peer_.ClearFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
        LOG(WARNING) << __func__ << " Move in closing state ";
        peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
      }
    } break;
    //MIUI ADD
    case BTIF_ACM_START_STREAM_REQ_EVT: {
        peer_.SetFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig);
    } break;
    //END
    case BTIF_ACM_SUSPEND_STREAM_REQ_EVT:

    case BTA_ACM_STOP_EVT: {
        BTIF_TRACE_DEBUG("%s: STOPPING event from BAP, Reset latency calculate params", __func__);
        peer_.cis_established_transport_latency_ = 0;
        peer_.cis_established_base_ft_ = 0;
        peer_.active_cig_id = -1;
        peer_.cis_handle_list.clear();
        peer_.SetPeerLatency(0);
    } break;

    case BTA_ACM_RECONFIG_EVT: {
        BTIF_TRACE_DEBUG("%s: received reconfiguring state from BAP, ignore", __func__);
    } break;

    case BTA_ACM_CONFIG_EVT: {
       uint16_t contextType = p_acm->state_info.stream_type.type;
       uint16_t peer_latency_ms = 0;
       uint32_t presen_delay = 0;
       bool is_update_require = false;
       uint32_t audio_location = p_acm->config_info.audio_location;
       BTIF_TRACE_DEBUG("%s: audio location is %d", __PRETTY_FUNCTION__, audio_location);
       if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
         peer_.set_peer_audio_src_location(audio_location);
       } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
         peer_.set_peer_audio_sink_location(audio_location);
       }
       if (contextType == CONTENT_TYPE_MEDIA) {
         if (p_acm->state_info.stream_type.audio_context == CONTENT_TYPE_MEDIA) {
           BTIF_TRACE_DEBUG("%s: compare with current media config",
                                                  __PRETTY_FUNCTION__);
           is_update_require = compare_codec_config_(current_media_config,
                                                     p_acm->config_info.codec_config);
         } else if (p_acm->state_info.stream_type.audio_context == CONTENT_TYPE_LIVE) {
           BTIF_TRACE_DEBUG("%s: cache current_recording_config", __PRETTY_FUNCTION__);
           current_recording_config = p_acm->config_info.codec_config;
         } else if (p_acm->state_info.stream_type.audio_context == CONTENT_TYPE_GAME) {
          //TODO
           if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
             BTIF_TRACE_DEBUG("%s: compare current gaming Tx config", __PRETTY_FUNCTION__);
             is_update_require = compare_codec_config_(current_media_config,
                                                       p_acm->config_info.codec_config);
             print_codec_parameters(p_acm->config_info.codec_config);
           } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC){
             BTIF_TRACE_DEBUG("%s: cache current gaming Rx config", __PRETTY_FUNCTION__);
             current_gaming_rx_config = p_acm->config_info.codec_config;
             print_codec_parameters(current_gaming_rx_config);
           }
         }

         //Cache the peer latency in WMCP case
         if (peer_.GetRcfgProfileType() == WMCP) {
           BTIF_TRACE_DEBUG("%s: presentation delay[0] = %x", __func__,
                            p_acm->config_info.qos_config.ascs_configs[0].presentation_delay[0]);
           BTIF_TRACE_DEBUG("%s: presentation delay[1] = %x", __func__,
                            p_acm->config_info.qos_config.ascs_configs[0].presentation_delay[1]);
           BTIF_TRACE_DEBUG("%s: presentation delay[2] = %x", __func__,
                            p_acm->config_info.qos_config.ascs_configs[0].presentation_delay[2]);
           presen_delay = static_cast<uint32_t>(p_acm->config_info.qos_config.ascs_configs[0].presentation_delay[0]) |
                          static_cast<uint32_t>(p_acm->config_info.qos_config.ascs_configs[0].presentation_delay[1] << 8) |
                          static_cast<uint32_t>(p_acm->config_info.qos_config.ascs_configs[0].presentation_delay[2] << 16);
           BTIF_TRACE_DEBUG("%s: presen_delay = %dus", __func__, presen_delay);
           peer_latency_ms = presen_delay/1000;
           BTIF_TRACE_DEBUG("%s: s_to_m latency = %dms", __func__,
                           p_acm->config_info.qos_config.cig_config.max_tport_latency_s_to_m);
           peer_latency_ms += p_acm->config_info.qos_config.cig_config.max_tport_latency_s_to_m;
           peer_.SetPeerLatency(peer_latency_ms);
           BTIF_TRACE_DEBUG("%s: cached peer Latency = %dms", __func__, peer_.GetPeerLatency());
         }

         BTIF_TRACE_DEBUG("%s: is_update_require: %d",
                                        __PRETTY_FUNCTION__, is_update_require);
         if (is_update_require) {
           current_media_config = p_acm->config_info.codec_config;
           peer_.set_peer_media_codec_config(current_media_config);
           BTIF_TRACE_DEBUG("%s: current_media_config.codec_specific_3: %"
                                 PRIi64, __func__, current_media_config.codec_specific_3);
           btif_acm_report_source_codec_state(peer_.PeerAddress(), current_media_config,
                                              unicast_codecs_capabilities_snk,
                                              peer_.GetSnkSelectableCapability(), CONTEXT_TYPE_MUSIC);
         }
       } else if (contextType == CONTENT_TYPE_CONVERSATIONAL &&
                  p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
         BTIF_TRACE_DEBUG("%s: cache current_voice_config", __PRETTY_FUNCTION__);
         current_voice_config = p_acm->config_info.codec_config;
         BTIF_TRACE_DEBUG("%s: current_voice_config.codec_specific_3: %"
                               PRIi64, __func__, current_voice_config.codec_specific_3);
         btif_acm_update_lc3q_params(&current_voice_config.codec_specific_3, p_acm);
         btif_acm_report_source_codec_state(peer_.PeerAddress(), current_voice_config,
                                            unicast_codecs_capabilities_src,
                                            peer_.GetSrcSelectableCapability(), CONTEXT_TYPE_VOICE);

       }
    } break;

    case BTA_ACM_CONNECT_EVT: {
        uint8_t status = (uint8_t)p_acm->state_info.stream_state;
        LOG_INFO(LOG_TAG, "%s: Peer %s : event=%s flags=%s status=%d,"
                          " rcfg_context_type: %d", __PRETTY_FUNCTION__,
                          peer_.PeerAddress().ToString().c_str(),
                          BtifAcmEvent::EventName(event).c_str(),
                          peer_.FlagsToString().c_str(),
                          status, peer_.GetRcfgContextType());

        if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingReconfigure) &&
            ((peer_.GetRcfgContextType() == CONTEXT_TYPE_VOICE &&
              p_acm->state_info.stream_type.type != CONTENT_TYPE_CONVERSATIONAL) ||
             (peer_.GetRcfgContextType() == CONTEXT_TYPE_MUSIC &&
              p_acm->state_info.stream_type.type != CONTENT_TYPE_MEDIA))) {
          BTIF_TRACE_DEBUG("%s:Reconfig context is not same as connected context"
                                                  "from BAP, ignore", __func__);
          break;
        }

        if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingReconfigure)) {
          if (p_acm->state_info.stream_state == StreamState::CONNECTED) {
            if (p_acm->state_info.stream_type.type == CONTENT_TYPE_MEDIA) {
              if (p_acm->state_info.stream_type.audio_context == CONTENT_TYPE_MEDIA ||
                  p_acm->state_info.stream_type.audio_context == CONTENT_TYPE_LIVE) {
                BTIF_TRACE_DEBUG("%s: Media Tx/Rx Reconfig complete, move in opened state",
                                                                             __func__);
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
              } else { //audio_context is Gaming
                if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
                  if (peer_.GetPeerMusicRxState() == StreamState::CONNECTED) {
                    BTIF_TRACE_DEBUG("%s: Move to opened state when Gaming Tx or"
                                                 " both Tx and Rx done", __func__);
                    BTIF_TRACE_DEBUG("%s: Received connected state from BAP for Gaming Tx,"
                                                       " move in opened state", __func__);
                  }
                  BTIF_TRACE_DEBUG("%s: Gaming Tx Reconfig complete, move in opened state",
                                                                                __func__);
                  peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
                } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
                  //TODO need to check this path whether VBC or not VBC
                  //Rx would come first in case of VBC, then wait for TX update from BAP
                  //to move to opened state
                  peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
                  BTIF_TRACE_DEBUG("%s: Gaming Rx Reconfig complete, wait for gaming Tx",
                                                                               __func__);
                  //peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
                }
              }
            } else {
              uint16_t num_sink_ases, num_src_ases;
              btif_bap_get_params(peer_.PeerAddress(), nullptr, nullptr, &num_sink_ases,
                            &num_src_ases, nullptr, nullptr);
              if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
                peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
                if (peer_.GetPeerVoiceRxState() == StreamState::CONNECTED ||
                    num_src_ases == 0) {
                  BTIF_TRACE_DEBUG("%s: Report Call audio config to apps? "
                         "move to opened when both Voice Tx and Rx done", __func__);
                  BTIF_TRACE_DEBUG("%s: received connected state from BAP for voice Tx, "
                                                "move in opened state", __func__);
                  peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
                }
              } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
                peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
                if (peer_.GetPeerVoiceTxState() == StreamState::CONNECTED ||
                    num_sink_ases == 0) {
                  BTIF_TRACE_DEBUG("%s: Report Call audio config to apps? "
                              "move to opened when both Voice Tx and Rx done", __func__);
                  BTIF_TRACE_DEBUG("%s: received connected state from BAP for voice Rx, "
                                                       "move in opened state", __func__);
                  peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
                }
              }
            }
          }
          break;
        }

        if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend)) {
          //When gaming app exited for gcp enabled, wait for both cbs from BAP.
          if (p_acm->state_info.stream_type.type == CONTENT_TYPE_MEDIA &&
              p_acm->state_info.stream_type.audio_context == CONTENT_TYPE_GAME &&
              p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
             BTIF_TRACE_DEBUG("%s:Stop came for GCP Rx, wait for GCP Tx from BAP ",
                                                                    __func__);
             break;
          } else if (p_acm->state_info.stream_type.type == CONTENT_TYPE_MEDIA &&
              p_acm->state_info.stream_type.audio_context == CONTENT_TYPE_GAME &&
              p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK){
             BTIF_TRACE_DEBUG("%s:Stop came for GCP Tx also from BAP, ack now ",
                                                                      __func__);
          }
          peer_.ClearFlags(BtifAcmPeer::kFlagPendingLocalSuspend);
          BTIF_TRACE_DEBUG("%s: peer device is suspended, send MM any pending ACK",
                                                                    __func__);
          tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);

          if (pending_cmd == A2DP_CTRL_CMD_STOP ||
              pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
            btif_acm_source_on_suspended(A2DP_CTRL_ACK_SUCCESS);
          } else if (pending_cmd == A2DP_CTRL_CMD_START) {
            btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
          } else {
           BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
          }

          if (alarm_is_scheduled(btif_acm_initiator.AcmGroupProcedureTimer())) {
            btif_acm_check_and_cancel_group_procedure_timer(
                                         btif_acm_initiator.MusicActiveCSetId());
          }

          btif_report_audio_state(peer_.PeerAddress(),
                                  BTACM_AUDIO_STATE_STOPPED,
                                  peer_.GetStreamContextType());

          std::vector<StreamReconfig> reconf_streams;
          StreamReconfig reconf_info;
          CodecQosConfig cfg;
          std::string config;
          memset(&reconf_info, 0, sizeof(StreamReconfig));
          memset(&cfg, 0, sizeof(CodecQosConfig));
          reconf_info.stream_type.type = CONTENT_TYPE_MEDIA;

          BTIF_TRACE_DEBUG("%s: reconfig profile: %d",
                             __PRETTY_FUNCTION__, peer_.GetRcfgProfileType());

          if (peer_.GetRcfgProfileType() == GCP) {
            bool is_media_tx_reconf_codec_qos_fetched = false;
            reconf_info.stream_type.audio_context = CONTENT_TYPE_GAME;
            reconf_info.stream_type.direction = ASE_DIRECTION_SINK;
            reconf_info.reconf_type =
                         bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
            is_media_tx_reconf_codec_qos_fetched =
                                    peer_.SetCodecQosConfig(peer_.PeerAddress(),
                                    GCP, MEDIA_CONTEXT, SNK, false, config);
            BTIF_TRACE_DEBUG("%s: is_media_tx_reconf_codec_qos_fetched: %d",
                            __func__, is_media_tx_reconf_codec_qos_fetched);
            if (is_media_tx_reconf_codec_qos_fetched == false) {
              BTIF_TRACE_DEBUG("%s: reconfig Game Tx only codecQos config not fetched,"
                                                                 " return", __func__);
              return false;
            }
            cfg = peer_.get_peer_media_codec_qos_config();
            print_codec_parameters(cfg.codec_config);
            print_qos_parameters(cfg.qos_config);
            reconf_info.codec_qos_config_pair.push_back(cfg);
            reconf_streams.push_back(reconf_info);
          } else if (peer_.GetRcfgProfileType() == GCP_RX) {
            //Get the config info from xml
            bool is_game_rx_reconf_codec_qos_fetched = false;
            BTIF_TRACE_DEBUG("%s: Reconfig for gaming Rx VBC.", __PRETTY_FUNCTION__);
            reconf_info.stream_type.type = CONTENT_TYPE_MEDIA;
            reconf_info.stream_type.audio_context = CONTENT_TYPE_GAME;
            reconf_info.stream_type.direction = ASE_DIRECTION_SRC;
            reconf_info.reconf_type =
                         bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
            is_game_rx_reconf_codec_qos_fetched =
                                    peer_.SetCodecQosConfig(peer_.PeerAddress(),
                                    GCP_RX, MEDIA_CONTEXT, SRC, true, config);
            BTIF_TRACE_DEBUG("%s: is_game_rx_reconf_codec_qos_fetched: %d",
                            __func__, is_game_rx_reconf_codec_qos_fetched);
            if (is_game_rx_reconf_codec_qos_fetched == false) {
              BTIF_TRACE_DEBUG("%s: reconfig game Rx codecQos config not fetched,"
                                                               " return", __func__);
              return false;
            }
            cfg = peer_.get_peer_voice_rx_codec_qos_config();
            print_codec_parameters(cfg.codec_config);
            print_qos_parameters(cfg.qos_config);
            reconf_info.codec_qos_config_pair.push_back(cfg);
            reconf_streams.push_back(reconf_info);

            bool is_game_tx_reconf_codec_qos_fetched = false;
            BTIF_TRACE_DEBUG("%s: Reconfig for gaming Tx.", __PRETTY_FUNCTION__);
            memset(&cfg, 0, sizeof(CodecQosConfig));
            memset(&reconf_info, 0, sizeof(StreamReconfig));
            reconf_info.stream_type.type = CONTENT_TYPE_MEDIA;
            reconf_info.stream_type.audio_context = CONTENT_TYPE_GAME;
            reconf_info.stream_type.direction = ASE_DIRECTION_SINK;
            reconf_info.reconf_type =
                         bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
            is_game_tx_reconf_codec_qos_fetched =
                                    peer_.SetCodecQosConfig(peer_.PeerAddress(),
                                    GCP_TX, MEDIA_CONTEXT, SNK, true, config);
            BTIF_TRACE_DEBUG("%s: is_game_tx_reconf_codec_qos_fetched: %d",
                            __func__, is_game_tx_reconf_codec_qos_fetched);
            if (is_game_tx_reconf_codec_qos_fetched == false) {
              BTIF_TRACE_DEBUG("%s: reconfig game Tx codecQos config not fetched,"
                                                            " return", __func__);
              return false;
            }
            cfg = peer_.get_peer_media_codec_qos_config();
            print_codec_parameters(cfg.codec_config);
            print_qos_parameters(cfg.qos_config);
            reconf_info.codec_qos_config_pair.push_back(cfg);
            reconf_streams.push_back(reconf_info);
          } else if (peer_.GetRcfgProfileType() == WMCP) {
            bool is_media_rx_reconf_codec_qos_fetched = false;
            reconf_info.stream_type.audio_context = CONTENT_TYPE_LIVE;
            reconf_info.stream_type.direction = ASE_DIRECTION_SRC;
            reconf_info.reconf_type =
                         bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
            is_media_rx_reconf_codec_qos_fetched =
                                    peer_.SetCodecQosConfig(peer_.PeerAddress(),
                                    WMCP, MEDIA_CONTEXT, SRC, false, config);
            BTIF_TRACE_DEBUG("%s: is_media_rx_reconf_codec_qos_fetched: %d",
                            __func__, is_media_rx_reconf_codec_qos_fetched);
            if (is_media_rx_reconf_codec_qos_fetched == false) {
              BTIF_TRACE_DEBUG("%s: reconfig media Rx codecQos config not fetched,"
                                                                 " return", __func__);
              return false;
            }
            cfg = peer_.get_peer_media_codec_qos_config();
            print_codec_parameters(cfg.codec_config);
            print_qos_parameters(cfg.qos_config);
            reconf_info.codec_qos_config_pair.push_back(cfg);
            reconf_streams.push_back(reconf_info);
          } else {
            bool is_media_tx_reconf_codec_qos_fetched = false;
            reconf_info.stream_type.audio_context = CONTENT_TYPE_MEDIA;
            reconf_info.stream_type.direction = ASE_DIRECTION_SINK;
            reconf_info.reconf_type =
                         bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
            peer_.SetCodecQosConfig(peer_.PeerAddress(),
                                    BAP, MEDIA_CONTEXT, SNK, false, config);
            BTIF_TRACE_DEBUG("%s: is_media_tx_reconf_codec_qos_fetched: %d",
                            __func__, is_media_tx_reconf_codec_qos_fetched);
            if (is_media_tx_reconf_codec_qos_fetched == false) {
              BTIF_TRACE_DEBUG("%s: reconfig media Tx codecQos config not fetched,"
                                                                 " return", __func__);
              return false;
            }
            cfg = peer_.get_peer_media_codec_qos_config();
            print_codec_parameters(cfg.codec_config);
            print_qos_parameters(cfg.qos_config);
            reconf_info.codec_qos_config_pair.push_back(cfg);
            reconf_streams.push_back(reconf_info);
          }

          peer_.SetFlags(BtifAcmPeer::kFlagPendingReconfigure);
          if (!sUcastClientInterface) break;
            sUcastClientInterface->Reconfigure(peer_.PeerAddress(), reconf_streams);
        }
    } break;

    case BTA_ACM_DISCONNECT_EVT: {
      int context_type = p_acm->state_info.stream_type.type;
      if (p_acm->state_info.stream_state == StreamState::DISCONNECTED) {
        if (context_type == CONTENT_TYPE_MEDIA) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
          }
          if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
              peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
            btif_report_connection_state(peer_.PeerAddress(),
                BTACM_CONNECTION_STATE_DISCONNECTED, CONTEXT_TYPE_MUSIC);
          }
          if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
            BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnected state from BAP"
                             " when Voice Tx+Rx & Media Rx/Tx was disconnected, "
                             "send pacs disconnect", __func__);
            if (!sPacsClientInterface) break;
            sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
            peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
          } else {
            BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnected state from BAP"
                             " when either Voice Tx or Rx or Media Rx/Tx is connected, "
                             "remain in reconfiguring state", __func__);
          }
        } else if (context_type == CONTENT_TYPE_CONVERSATIONAL) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED) {
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_DISCONNECTED,
                                           CONTEXT_TYPE_VOICE);
              if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
                  peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Tx,"
                                 " voice Rx, Music Tx & Rx are disconnected "
                                 "send pacs disconnect", __func__);
                if (!sPacsClientInterface) break;
                sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Tx,"
                                 " voice Rx is disconnected but music Tx or "
                                 "Rx still not disconnected,"
                                 " remain in reconfiguring state", __func__);
              }
            }
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED) {
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_DISCONNECTED,
                                           CONTEXT_TYPE_VOICE);
              if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
                  peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                 " voice Tx, Music Tx & Rx are disconnected "
                                 " send pacs disconnect", __func__);
                if (!sPacsClientInterface) break;
                sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                 " voice Rx is disconnected but music Tx or "
                                 "Rx still not disconnected,"
                                 " remain in reconfiguring state", __func__);
              }
            }
          }
        }
      } else if (p_acm->state_info.stream_state == StreamState::DISCONNECTING) {
        if (context_type == CONTENT_TYPE_MEDIA) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
          }
          btif_report_connection_state(peer_.PeerAddress(),
                                       BTACM_CONNECTION_STATE_DISCONNECTING,
                                       CONTEXT_TYPE_MUSIC);
          if ((peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED ||
                peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTING) &&
               (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED ||
                peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTING) &&
               (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED ||
                peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING) &&
               (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED ||
                peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING)) {
              BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnecting state from BAP"
                               " when Voice Tx+Rx and Media Rx/Tx disconnected/ing, "
                               "move in closing state", __func__);
              peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
          } else {
            if (peer_.GetRcfgProfileType() != BAP_CALL) {
              std::vector<StreamType> disconnect_streams;
              btif_report_audio_state(peer_.PeerAddress(),
                                      BTACM_AUDIO_STATE_STOPPED,
                                      CONTEXT_TYPE_MUSIC);

              if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING &&
                  peer_.GetPeerMusicRxState() == StreamState::CONNECTED) {
                BTIF_TRACE_DEBUG("%s: Received disconnecting for Music-Tx, "
                                 "initiate for Rx also", __func__);
                StreamType type_1;
                type_1 = {.type = CONTENT_TYPE_MEDIA,
                          .audio_context = CONTENT_TYPE_LIVE | CONTENT_TYPE_GAME,
                          .direction = ASE_DIRECTION_SRC
                         };
                disconnect_streams.push_back(type_1);

                if (!sUcastClientInterface) break;
                sUcastClientInterface->Disconnect(peer_.PeerAddress(), disconnect_streams);
              }

              if (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING &&
                  peer_.GetPeerMusicTxState() == StreamState::CONNECTED) {
                BTIF_TRACE_DEBUG("%s: Received disconnecting for Music-Rx, "
                                 "initiate for Tx also", __func__);
                StreamType type_1;
                type_1 = {.type = CONTENT_TYPE_MEDIA,
                          .audio_context = CONTENT_TYPE_MEDIA | CONTENT_TYPE_GAME,
                          .direction = ASE_DIRECTION_SINK
                         };
                disconnect_streams.push_back(type_1);

                if (!sUcastClientInterface) break;
                sUcastClientInterface->Disconnect(peer_.PeerAddress(), disconnect_streams);
              }
              BTIF_TRACE_DEBUG("%s: reconfiguring media is disconnected/ing, move to OPENED", __func__);
              peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
            } else {
              BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnecting state from BAP"
                               " when either Voice Tx or Rx or Media Rx/Tx is connected, "
                               "remain in reconfiguring state", __func__);
            }
          }
        } else if (context_type == CONTENT_TYPE_CONVERSATIONAL) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTING ||
                peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED) {
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_DISCONNECTING,
                                           CONTEXT_TYPE_VOICE);
              if (((peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED) ||
                  (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING)) &&
                  ((peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) ||
                  (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING))) {
                BTIF_TRACE_DEBUG("%s: received disconnecting state from BAP for voice Tx,"
                                 " voice Rx, music Tx+Rx are disconnected/ing "
                                 "move in closing state", __func__);
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else {
                if (peer_.GetRcfgProfileType() == BAP_CALL) {
                btif_report_audio_state(peer_.PeerAddress(),
                                        BTACM_AUDIO_STATE_STOPPED,
                                        CONTEXT_TYPE_VOICE);
                BTIF_TRACE_DEBUG("%s: reconfiguring voice is disconnected/ing, move to OPENED", __func__);
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
                } else {
                BTIF_TRACE_DEBUG("%s: received disconnecting state from BAP for voice Tx,"
                                 " voice Rx is disconncted/ing but music Tx or "
                                 "Rx still not disconnected/ing,"
                                 " remain in reconfiguring state", __func__);
                }
              }
            }
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTING ||
                peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED) {
              btif_report_connection_state(peer_.PeerAddress(),
                                           BTACM_CONNECTION_STATE_DISCONNECTING,
                                           CONTEXT_TYPE_VOICE);
              if (((peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED) ||
                  (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTING)) &&
                  ((peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) ||
                  (peer_.GetPeerMusicRxState() == StreamState::DISCONNECTING))) {
                BTIF_TRACE_DEBUG("%s: received disconnecting state from BAP for voice Rx,"
                                 " voice Tx, music Tx+Rx are disconnected/ing "
                                 "move in closing state", __func__);
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateClosing);
              } else {
                if(peer_.GetRcfgProfileType() == BAP_CALL) {
                btif_report_audio_state(peer_.PeerAddress(),
                                        BTACM_AUDIO_STATE_STOPPED,
                                        CONTEXT_TYPE_VOICE);
                BTIF_TRACE_DEBUG("%s: reconfiguring voice is disconnected/ing, move to OPENED", __func__);
                peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
                } else {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                 " voice Tx is disconncted/ing but music Tx or "
                                 "Rx still not disconnected/ing,"
                                 " remain in reconfiguring state", __func__);
              }
              }
            }
          }
        }
      }
    } break;

    case BTA_ACM_CONN_UPDATE_TIMEOUT_EVT:
      peer_.SetConnUpdateMode(BtifAcmPeer::kFlagRelaxedMode);
      break;

    default:
      BTIF_TRACE_WARNING("%s: Peer %s : Unhandled event=%s",
                         __PRETTY_FUNCTION__,
                         peer_.PeerAddress().ToString().c_str(),
                         BtifAcmEvent::EventName(event).c_str());
      return false;
  }
  return true;
}

void BtifAcmStateMachine::StateClosing::OnEnter() {
  BTIF_TRACE_DEBUG("%s: Peer %s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str());
  if(btif_acm_initiator.IsConnUpdateEnabled()) {
    //Cancel the timer if running if  not, move to aggressive mode
    if (alarm_is_scheduled(btif_acm_initiator.AcmConnIntervalTimer())) {
      btif_acm_check_and_cancel_conn_Interval_timer();
    }
    else {
      BTIF_TRACE_DEBUG("%s: conn timer not running, push aggressive intervals",
                                                              __func__);
      peer_.SetConnUpdateMode(BtifAcmPeer::kFlagAggresiveMode);
    }
  }
  if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingStart)) {
    peer_.ClearFlags(BtifAcmPeer::kFlagPendingStart);
    tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
    if (pending_cmd == A2DP_CTRL_CMD_START) {
      btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
    }
  }
  if (peer_.CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend)) {
    peer_.ClearFlags(BtifAcmPeer::kFlagPendingLocalSuspend);
    BtifAcmPeer *other_peer = btif_get_peer_device(&peer_);
    if(other_peer &&
          other_peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend)) {
      BTIF_TRACE_DEBUG("%s: other peer device is not suspended, wait for it", __func__);
    } else {
      BTIF_TRACE_DEBUG("%s: peer device is suspended, send MM any pending ACK", __func__);
      tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
      if (pending_cmd == A2DP_CTRL_CMD_STOP ||
        pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
        btif_acm_source_on_suspended(A2DP_CTRL_ACK_SUCCESS);
      } else if (pending_cmd == A2DP_CTRL_CMD_START) {
        btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
      } else {
        BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
      }
      if (other_peer &&
          other_peer->CheckFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend)) {
        BTIF_TRACE_DEBUG("%s: other peer is suspended and waiting, report STOP to apps"
                        " and move to Opened for it", __func__);
        btif_report_audio_state(other_peer->PeerAddress(),
                                BTACM_AUDIO_STATE_STOPPED,
                                other_peer->GetStreamContextType());
        other_peer->StateMachine().TransitionTo(BtifAcmStateMachine::kStateOpened);
        other_peer->ClearFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend);
      }
    }
  }
  if(peer_.CheckFlags(BtifAcmPeer::kFlagPendingSetActiveDev)) {
    peer_.ClearFlags(BtifAcmPeer::kFlagPendingSetActiveDev);
  }
}

void BtifAcmStateMachine::StateClosing::OnExit() {
  BTIF_TRACE_DEBUG("%s: Peer %s", __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str());
}


void BtifAcmStateMachine::StateClosing::CheckAndUpdateDisconnection() {
  ConnectionState pacs_state = peer_.GetPeerPacsState();
  if(pacs_state == ConnectionState::DISCONNECTED) {
    LOG_DEBUG(LOG_TAG, "%s PACS already disconnected ", __PRETTY_FUNCTION__);
    if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
        peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
      btif_report_connection_state(peer_.PeerAddress(),
                                   BTACM_CONNECTION_STATE_DISCONNECTED,
                                   CONTEXT_TYPE_MUSIC);
    }
    if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED &&
        peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED) {
      btif_report_connection_state(peer_.PeerAddress(),
                                   BTACM_CONNECTION_STATE_DISCONNECTED,
                                   CONTEXT_TYPE_VOICE);
    }
    peer_.StateMachine().TransitionTo(BtifAcmStateMachine::kStateIdle);
  } else if(pacs_state == ConnectionState::CONNECTED) {
    if(sPacsClientInterface) {
      LOG_DEBUG(LOG_TAG, "%s Issue PACS disconnect ", __PRETTY_FUNCTION__);
      sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
    }
  }
}

bool BtifAcmStateMachine::StateClosing::ProcessEvent(uint32_t event,
                                                     void* p_data) {
  tBTIF_ACM* p_acm = (tBTIF_ACM*)p_data;
  BTIF_TRACE_DEBUG("%s: Peer %s : event=%s flags=%s "
                   "music_active_peer=%s voice_active_peer=%s",
                   __PRETTY_FUNCTION__,
                   peer_.PeerAddress().ToString().c_str(),
                   BtifAcmEvent::EventName(event).c_str(),
                   peer_.FlagsToString().c_str(),
                   logbool(peer_.IsPeerActiveForMusic()).c_str(),
                   logbool(peer_.IsPeerActiveForVoice()).c_str());

  switch (event) {
    case BTIF_ACM_SUSPEND_STREAM_REQ_EVT:
    case BTIF_ACM_START_STREAM_REQ_EVT:
    case BTA_ACM_STOP_EVT:
      break;

    case BTIF_ACM_STOP_STREAM_REQ_EVT: {
      tBTIF_ACM_STOP_REQ * p_bta_data = (tBTIF_ACM_STOP_REQ*)p_data;
      if(p_bta_data->wait_for_signal) {
        p_bta_data->peer_ready_promise->set_value();
      }
      LOG_ERROR(LOG_TAG, "%s BTIF_ACM_STOP_STREAM_REQ_EVT from BTIF ",
                                             __PRETTY_FUNCTION__);
    } break;

    case BTA_ACM_DISCONNECT_EVT: {
      int context_type = p_acm->state_info.stream_type.type;
      if (p_acm->state_info.stream_state == StreamState::DISCONNECTED) {
        if (context_type == CONTENT_TYPE_MEDIA) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
          }
          if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
               peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
            BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnected state from BAP"
                      " when Voice Tx+Rx & Media Rx/Tx was disconnected, "
                      " send pacs disconnect", __func__);
            CheckAndUpdateDisconnection();

          } else {
            BTIF_TRACE_DEBUG("%s: received Media Tx/Rx disconnected state from BAP"
                      " when either Voice Tx or Rx or Media Rx/Tx is connected, "
                      "remain in closing state", __func__);
          }
        } else if (context_type == CONTENT_TYPE_CONVERSATIONAL) {
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceRxState() == StreamState::DISCONNECTED) {
              if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
                  peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Tx,"
                                 " voice Rx, Music Tx & Rx are disconnected "
                                 " send pacs disconnect", __func__);
                CheckAndUpdateDisconnection();
              } else {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Tx,"
                                 " voice Rx is disconnected but music Tx or "
                                 "Rx still not disconnected,"
                                 " remain in closing state", __func__);
              }
            }
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
            if (peer_.GetPeerVoiceTxState() == StreamState::DISCONNECTED) {
              if (peer_.GetPeerMusicTxState() == StreamState::DISCONNECTED &&
                  peer_.GetPeerMusicRxState() == StreamState::DISCONNECTED) {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                 " voice Tx, Music Tx & Rx are disconnected "
                                 " send pacs disconnect", __func__);
                CheckAndUpdateDisconnection();
              } else {
                BTIF_TRACE_DEBUG("%s: received disconnected state from BAP for voice Rx,"
                                 " voice Rx is disconnected but music Tx or "
                                 "Rx still not disconnected,"
                                 " remain in closing state", __func__);
              }
            }
          }
        }
      } else if (p_acm->state_info.stream_state == StreamState::DISCONNECTING) {
        if (context_type == CONTENT_TYPE_MEDIA) {
          BTIF_TRACE_DEBUG("%s: received Music Tx or Rx disconnecting state from BAP, "
                           "ignore", __func__);
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK)
            peer_.SetPeerMusicTxState(p_acm->state_info.stream_state);
          else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC)
            peer_.SetPeerMusicRxState(p_acm->state_info.stream_state);
        } else if (context_type == CONTENT_TYPE_CONVERSATIONAL) {
          BTIF_TRACE_DEBUG("%s: received voice Tx or Rx disconnecting state from BAP, "
                           "ignore", __func__);
          if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SINK) {
            peer_.SetPeerVoiceTxState(p_acm->state_info.stream_state);
          } else if (p_acm->state_info.stream_type.direction == ASE_DIRECTION_SRC) {
            peer_.SetPeerVoiceRxState(p_acm->state_info.stream_state);
          }
        }
      }
    } break;

    case BTA_ACM_PACS_DISCONNECT_EVT: {
      CheckAndUpdateDisconnection();
    } break;

    case BTA_ACM_CONN_UPDATE_TIMEOUT_EVT:
      peer_.SetConnUpdateMode(BtifAcmPeer::kFlagRelaxedMode);
      break;

    default:
      BTIF_TRACE_WARNING("%s: Peer %s : Unhandled event=%s",
                         __PRETTY_FUNCTION__,
                         peer_.PeerAddress().ToString().c_str(),
                         BtifAcmEvent::EventName(event).c_str());
      if (!sPacsClientInterface) break;
      sPacsClientInterface->Disconnect(pacs_client_id, peer_.PeerAddress());
      return false;
  }
  return true;
}

void btif_acm_update_lc3q_params(int64_t* cs3, tBTIF_ACM* p_data) {

  /* ==================================================================
   * CS3: Res  |LC3Q-len| QTI  | VMT | VML | ver/For_Als |LC3Q-support
   * ==================================================================
   *      0x00 |0B      | 000A | FF  | 0F  | 01/03       | 10
   * ==================================================================
   * CS4:    Res
   * ==============================
   *     0x00,00,00,00,00,00,00,00
   * ============================== */

  if (GetVendorMetaDataLc3QPref(
                         &p_data->config_info.codec_config)) {
    *cs3 &= ~((int64_t)0xFF << (LE_AUDIO_CS_3_1ST_BYTE_INDEX * 8));
    *cs3 |=  ((int64_t)0x10 << (LE_AUDIO_CS_3_1ST_BYTE_INDEX * 8));

    uint8_t lc3q_ver = GetVendorMetaDataCodecEncoderVer(&p_data->config_info.codec_config);
    BTIF_TRACE_DEBUG("%s: lc3q_ver: %d", __func__, lc3q_ver);
    *cs3 &= ~((int64_t)0xFF << (LE_AUDIO_CS_3_2ND_BYTE_INDEX * 8));
    *cs3 |=  ((int64_t)lc3q_ver << (LE_AUDIO_CS_3_2ND_BYTE_INDEX * 8));

    //*cs3 &= ~((int64_t)LE_AUDIO_MASK);
    *cs3 |=  (int64_t)LE_AUDIO_AVAILABLE_LICENSED;

    *cs3 &= ~((int64_t)0xFF << (LE_AUDIO_CS_3_3RD_BYTE_INDEX * 8));
    *cs3 |=  ((int64_t)0x0F << (LE_AUDIO_CS_3_3RD_BYTE_INDEX * 8));

    *cs3 &= ~((int64_t)0xFF << (LE_AUDIO_CS_3_4TH_BYTE_INDEX * 8));
    *cs3 |=  ((int64_t)0xFF << (LE_AUDIO_CS_3_4TH_BYTE_INDEX * 8));

    *cs3 &= ~((int64_t)0xFFFF << (LE_AUDIO_CS_3_5TH_BYTE_INDEX * 8));
    *cs3 |=  ((int64_t)0x000A << (LE_AUDIO_CS_3_5TH_BYTE_INDEX * 8));

    *cs3 &= ~((int64_t)0xFF << (LE_AUDIO_CS_3_7TH_BYTE_INDEX * 8));
    *cs3 |=  ((int64_t)0x0B << (LE_AUDIO_CS_3_7TH_BYTE_INDEX * 8));

    CodecConfig temp = unicast_codecs_capabilities_src.back();
    unicast_codecs_capabilities_src.pop_back();
    temp.codec_specific_3 = *cs3;
    unicast_codecs_capabilities_src.push_back(temp);
  }
  BTIF_TRACE_DEBUG("%s: cs3: %" PRIi64, __func__, *cs3);
  BTIF_TRACE_DEBUG("%s: cs3= 0x%" PRIx64, __func__, *cs3);
}

static void btif_report_connection_state(const RawAddress& peer_address,
                                         btacm_connection_state_t state,
                                         uint16_t contextType) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s state=%d contextType=%d", __func__,
           peer_address.ToString().c_str(), state, contextType);
  if (btif_acm_initiator.Enabled()) {
    do_in_jni_thread(FROM_HERE,
                     Bind(btif_acm_initiator.Callbacks()->connection_state_cb,
                          peer_address, state, contextType));
  }
}

void btif_report_metadata_context(const RawAddress& peer_address,
                                         uint16_t contextType) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s, contextType=%d", __func__,
           peer_address.ToString().c_str(), contextType);
  if (btif_acm_initiator.Enabled()) {
    do_in_jni_thread(FROM_HERE,
                     Bind(btif_acm_initiator.Callbacks()->metadata_cb,
                          peer_address, contextType));
  }
}

static void btif_report_audio_state(const RawAddress& peer_address,
                                    btacm_audio_state_t state,
                                    uint16_t contextType) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s state=%d contextType=%d",
           __func__, peer_address.ToString().c_str(), state, contextType);
  if (btif_acm_initiator.Enabled()) {
    do_in_jni_thread(FROM_HERE,
                     Bind(btif_acm_initiator.Callbacks()->audio_state_cb,
                          peer_address, state, contextType));
  }
}

void btif_acm_report_source_codec_state(
    const RawAddress& peer_address,
    const CodecConfig& codec_config,
    const std::vector<CodecConfig>& codecs_local_capabilities,
    const std::vector<CodecConfig>&
        codecs_selectable_capabilities, int contextType) {
  BTIF_TRACE_EVENT("%s: peer_address=%s contextType=%d", __func__,
                   peer_address.ToString().c_str(), contextType);
  print_codec_parameters(codec_config);
  if (btif_acm_initiator.Enabled()) {
    do_in_jni_thread(FROM_HERE,
                     Bind(btif_acm_initiator.Callbacks()->audio_config_cb,
                     peer_address,
                     codec_config, codecs_local_capabilities,
                     codecs_selectable_capabilities, contextType));
  }
}

static void btif_acm_handle_evt(uint16_t event, char* p_param) {
  BtifAcmPeer* peer = nullptr;
  BTIF_TRACE_DEBUG("Handle the ACM event = %d ", event);
  switch (event) {
    case BTIF_ACM_DISCONNECT_REQ_EVT: {
        if (p_param == NULL) {
          BTIF_TRACE_ERROR("%s: Invalid p_param, dropping event: %d",
                           __func__, event);
          return;
        }
        tBTIF_ACM_CONN_DISC* p_acm = (tBTIF_ACM_CONN_DISC*)p_param;
        peer = btif_acm_initiator.FindOrCreatePeer(p_acm->bd_addr);
        if (peer == nullptr) {
          BTIF_TRACE_ERROR(
              "%s: Cannot find peer for peer_address=%s"
              ": event dropped: %d",
              __func__, p_acm->bd_addr.ToString().c_str(),
              event);
          return;
        } else {
          BTIF_TRACE_EVENT(
              "%s: BTIF_ACM_DISCONNECT_REQ_EVT peer_address=%s"
              ": contextType=%d",
              __func__, p_acm->bd_addr.ToString().c_str(),
              p_acm->contextType);
        }
        break;
    }
    case BTIF_ACM_STOP_STREAM_REQ_EVT: {
        if (p_param == NULL) {
          BTIF_TRACE_ERROR("%s: Invalid p_param, dropping event: %d",
                           __func__, event);
          return;
        }
        tBTIF_ACM_STOP_REQ* p_acm = (tBTIF_ACM_STOP_REQ*)p_param;
        peer = btif_acm_initiator.FindOrCreatePeer(p_acm->bd_addr);
        if (peer == nullptr) {
          BTIF_TRACE_ERROR("%s: Cannot find peer for peer_address=%s"
                           ": event dropped: %d",
                           __func__, p_acm->bd_addr.ToString().c_str(),
                           event);
          return;
        }
    } break;
    case BTIF_ACM_START_STREAM_REQ_EVT:
    case BTIF_ACM_SUSPEND_STREAM_REQ_EVT: {
        if (p_param == NULL) {
          BTIF_TRACE_ERROR("%s: Invalid p_param, dropping event: %d",
                           __func__, event);
          return;
        }
        tBTIF_ACM_CONN_DISC* p_acm = (tBTIF_ACM_CONN_DISC*)p_param;
        peer = btif_acm_initiator.FindOrCreatePeer(p_acm->bd_addr);
        if (peer == nullptr) {
          BTIF_TRACE_ERROR("%s: Cannot find peer for peer_address=%s"
                           ": event dropped: %d",
                           __func__, p_acm->bd_addr.ToString().c_str(),
                           event);
          return;
        }
    } break;
    case BTA_ACM_DISCONNECT_EVT: {
        if (p_param == NULL) {
          BTIF_TRACE_ERROR("%s: Invalid p_param, dropping event: %d",
                           __func__, event);
          return;
        }
        tBTA_ACM_STATE_INFO* p_acm = (tBTA_ACM_STATE_INFO*)p_param;
        peer = btif_acm_initiator.FindPeer(p_acm->bd_addr);
        if (peer == nullptr) {
          BTIF_TRACE_ERROR("%s: Cannot find peer for peer_address=%s"
                           ": event dropped: %d",
                           __func__, p_acm->bd_addr.ToString().c_str(), event);
          return;
        }
    } break;
    case BTA_ACM_CONNECT_EVT:
    case BTA_ACM_START_EVT:
    case BTA_ACM_STOP_EVT:
    case BTA_ACM_RECONFIG_EVT: {
        if (p_param == NULL) {
          BTIF_TRACE_ERROR("%s: Invalid p_param, dropping event: %d",
                            __func__, event);
          return;
        }
        tBTA_ACM_STATE_INFO* p_acm = (tBTA_ACM_STATE_INFO*)p_param;
        peer = btif_acm_initiator.FindOrCreatePeer(p_acm->bd_addr);
        if (peer == nullptr) {
          BTIF_TRACE_ERROR("%s: Cannot find or create peer for peer_address=%s"
                           ": event dropped: %d",
                           __func__, p_acm->bd_addr.ToString().c_str(), event);
          return;
        }
    } break;
    case BTA_ACM_CONFIG_EVT: {
        if (p_param == NULL) {
          BTIF_TRACE_ERROR("%s: Invalid p_param, dropping event: %d",
                           __func__, event);
          return;
        }
        tBTA_ACM_CONFIG_INFO* p_acm = (tBTA_ACM_CONFIG_INFO*)p_param;
        peer = btif_acm_initiator.FindPeer(p_acm->bd_addr);
        if (peer == nullptr) {
          BTIF_TRACE_ERROR("%s: Cannot find or create peer for peer_address=%s"
                           ": event dropped: %d",
                           __func__, p_acm->bd_addr.ToString().c_str(), event);
          return;
        }
    } break;
    case BTIF_ACM_RECONFIG_REQ_EVT: {
        if (p_param == NULL) {
          BTIF_TRACE_ERROR("%s: Invalid p_param, dropping event: %d",
                             __func__, event);
          return;
        }
        tBTIF_ACM_RECONFIG* p_acm = (tBTIF_ACM_RECONFIG*)p_param;
        peer = btif_acm_initiator.FindPeer(p_acm->bd_addr);
        if (peer == nullptr) {
          BTIF_TRACE_ERROR("%s: Cannot find or create peer for peer_address=%s"
                           ": event dropped: %d",
                           __func__, p_acm->bd_addr.ToString().c_str(), event);
          return;
        }
    } break;

    case BTA_ACM_CONN_UPDATE_TIMEOUT_EVT: {
        if (p_param == NULL) {
          BTIF_TRACE_ERROR("%s: Invalid p_param, dropping event: %d",
                            __func__, event);
          return;
        }
        tBTA_ACM_CONN_UPDATE_TIMEOUT_INFO* p_acm =
                                  (tBTA_ACM_CONN_UPDATE_TIMEOUT_INFO *)p_param;
        peer = btif_acm_initiator.FindPeer(p_acm->bd_addr);
        if (peer == nullptr) {
          BTIF_TRACE_ERROR("%s: Cannot find or create peer for peer_address=%s"
                           ": event dropped: %d",
                           __func__, p_acm->bd_addr.ToString().c_str(), event);
          return;
        }
    } break;

    case BTA_ACM_PACS_DISCONNECT_EVT: {
        if (p_param == NULL) {
          BTIF_TRACE_ERROR("%s: Invalid p_param, dropping event: %d", __func__, event);
          return;
        }
        tBTA_ACM_PACS_STATE_INFO* p_acm = (tBTA_ACM_PACS_STATE_INFO *)p_param;
        peer = btif_acm_initiator.FindPeer(p_acm->bd_addr);
        if (peer == nullptr) {
          BTIF_TRACE_ERROR("%s: Cannot find or create peer for peer_address=%s"
                           ": event dropped: %d",
                           __func__, p_acm->bd_addr.ToString().c_str(), event);
          return;
        } else {
          ConnectionState pacs_state = p_acm->state;
          if (pacs_state == ConnectionState::DISCONNECTED ||
              pacs_state == ConnectionState::DISCONNECTING) {
            peer->SetPeerPacsState(pacs_state);
            LOG_ERROR(LOG_TAG, "%s PACS disconnected", __PRETTY_FUNCTION__);
            break;
          }
        }
    } break;

    case BTA_ACM_PACS_CONNECT_EVT: {
        if (p_param == NULL) {
          BTIF_TRACE_ERROR("%s: Invalid p_param, dropping event: %d", __func__, event);
          return;
        }
        tBTA_ACM_PACS_STATE_INFO* p_acm = (tBTA_ACM_PACS_STATE_INFO *)p_param;
        peer = btif_acm_initiator.FindPeer(p_acm->bd_addr);
        if (peer == nullptr) {
          BTIF_TRACE_ERROR("%s: Cannot find or create peer for peer_address=%s"
                           ": event dropped: %d",
                           __func__, p_acm->bd_addr.ToString().c_str(), event);
          return;
        }
    } break;

    case BTA_ACM_PACS_DISCOVERY_RES_EVT: {
        if (p_param == NULL) {
          BTIF_TRACE_ERROR("%s: Invalid p_param, dropping event: %d", __func__, event);
          return;
        }
        tBTA_ACM_PACS_DATA_INFO* p_acm =  (tBTA_ACM_PACS_DATA_INFO *)p_param;
        peer = btif_acm_initiator.FindPeer(p_acm->bd_addr);
        if (peer == nullptr) {
          BTIF_TRACE_ERROR("%s: Cannot find or create peer for peer_address=%s"
                           ": event dropped: %d",
                           __func__, p_acm->bd_addr.ToString().c_str(), event);
          return;
        }
    } break;

    case BTA_ACM_LATENCY_UPDATE_EVT: {
        if (p_param == NULL) {
          BTIF_TRACE_ERROR("%s: Invalid p_param, dropping event: %d", __func__, event);
          return;
        }
        BtifAcmPeer* peer = btif_acm_initiator.FindMusicActivePeer();
        if (!peer || !peer->IsStreaming()) {
          BTIF_TRACE_ERROR("%s: Peer not streaming ignore ft change evt: %d", __func__, event);
          return;
        }
    }   break;
    default :
        BTIF_TRACE_DEBUG("UNHandled ACM event = %d ", event);
        break;
  }
  peer->StateMachine().ProcessEvent(event, (void*)p_param);
}

/**
 * Process BTA CSIP events. The processing is done on the JNI
 * thread.
 */
static void btif_acm_handle_bta_csip_event(uint16_t evt, char* p_param) {
  BtifCsipEvent btif_csip_event(evt, p_param, sizeof(tBTA_CSIP_DATA));
  tBTA_CSIP_EVT event = btif_csip_event.Event();
  tBTA_CSIP_DATA* p_data = (tBTA_CSIP_DATA*)btif_csip_event.Data();
  BTIF_TRACE_DEBUG("%s: event=%s", __func__, btif_csip_event.ToString().c_str());

  switch (event) {
    case BTA_CSIP_LOCK_STATUS_CHANGED_EVT: {
      const tBTA_LOCK_STATUS_CHANGED& lock_status_param = p_data->lock_status_param;
      BTIF_TRACE_DEBUG("%s: app_id=%d, set_id=%d, status=%d ", __func__,
                       lock_status_param.app_id, lock_status_param.set_id,
                       lock_status_param.status);

      std::vector<RawAddress> set_members =lock_status_param.addr;

      for (int j = 0; j < (int)set_members.size(); j++) {
        BTIF_TRACE_DEBUG("%s: address =%s",
                         __func__, set_members[j].ToString().c_str());
      }

      BTIF_TRACE_DEBUG("%s: Get current lock status: %d ", __func__,
            btif_acm_initiator.GetGroupLockStatus(lock_status_param.set_id));

      if (btif_acm_initiator.GetGroupLockStatus(lock_status_param.set_id) ==
                                    BtifAcmInitiator::kFlagStatusPendingLock) {
        BTIF_TRACE_DEBUG("%s: lock was awaited for this set ", __func__);
      }

      if (btif_acm_initiator.GetGroupLockStatus(lock_status_param.set_id) ==
                                BtifAcmInitiator::kFlagStatusPendingUnlock) {
        BTIF_TRACE_DEBUG("%s: Unlock was awaited for this set ", __func__);
      }

      BTIF_TRACE_DEBUG("%s: Get CSIP app id: %d ", __func__,
                        btif_acm_initiator.GetCsipAppId());
      if (btif_acm_initiator.GetCsipAppId() != lock_status_param.app_id) {
        BTIF_TRACE_DEBUG("%s: app id mismatch ERROR!!! ", __func__);
        tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
        if (pending_cmd == A2DP_CTRL_CMD_STOP ||
           pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
          btif_acm_source_on_suspended(A2DP_CTRL_ACK_FAILURE);
        } else if (pending_cmd == A2DP_CTRL_CMD_START) {
          btif_acm_on_started(A2DP_CTRL_ACK_FAILURE);
        } else {
          BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
        }
        return;
      }

      switch (lock_status_param.status) {
        case LOCK_RELEASED:
            BTIF_TRACE_DEBUG("%s: unlocked attempt succeeded ", __func__);
            btif_acm_initiator.SetOrUpdateGroupLockStatus(lock_status_param.set_id,
                                              BtifAcmInitiator::kFlagStatusUnlocked);
            break;

        case LOCK_RELEASED_TIMEOUT:
            BTIF_TRACE_DEBUG("%s: peer unlocked due to timeout ", __func__);
            //in this case evaluate which device has sent TO and how to use it ?
            btif_acm_initiator.SetOrUpdateGroupLockStatus(lock_status_param.set_id,
                                              BtifAcmInitiator::kFlagStatusUnlocked);
            break;

        case ALL_LOCKS_ACQUIRED:
            btif_acm_initiator.SetOrUpdateGroupLockStatus(lock_status_param.set_id,
                                                BtifAcmInitiator::kFlagStatusLocked);
            btif_acm_handle_csip_status_locked(lock_status_param.addr,
                                               lock_status_param.set_id);
            BTIF_TRACE_DEBUG("%s: All locks acquired ", __func__);
            break;

        case SOME_LOCKS_ACQUIRED_REASON_TIMEOUT:
            //proceed to continue use case;
        case LOCK_DENIED: {
            //proceed to discontinue use case;
            tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
            if (pending_cmd == A2DP_CTRL_CMD_STOP ||
               pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
              btif_acm_source_on_suspended(A2DP_CTRL_ACK_FAILURE);
            } else if (pending_cmd == A2DP_CTRL_CMD_START) {
              btif_acm_on_started(A2DP_CTRL_ACK_FAILURE);
            } else {
              BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
            }
            btif_acm_check_and_cancel_group_procedure_timer(lock_status_param.set_id);
            btif_acm_initiator.SetOrUpdateGroupLockStatus(lock_status_param.set_id,
                                               BtifAcmInitiator::kFlagStatusUnlocked);
        } break;

        case INVALID_REQUEST_PARAMS: {
            BTIF_TRACE_DEBUG("%s: invalid lock request ", __func__);
            tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
            if (pending_cmd == A2DP_CTRL_CMD_STOP ||
               pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
              btif_acm_source_on_suspended(A2DP_CTRL_ACK_FAILURE);
            } else if (pending_cmd == A2DP_CTRL_CMD_START) {
              btif_acm_on_started(A2DP_CTRL_ACK_FAILURE);
            } else {
              BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
            }
            btif_acm_check_and_cancel_group_procedure_timer(lock_status_param.set_id);
            if (btif_acm_initiator.GetGroupLockStatus(
                   lock_status_param.set_id) == BtifAcmInitiator::kFlagStatusPendingLock)
              btif_acm_initiator.SetOrUpdateGroupLockStatus(lock_status_param.set_id,
                                                BtifAcmInitiator::kFlagStatusUnlocked);
            else
              btif_acm_initiator.SetOrUpdateGroupLockStatus(lock_status_param.set_id,
                                                  BtifAcmInitiator::kFlagStatusLocked);
        } break;

        default:
        break;
      }
    } break;

    case BTA_CSIP_SET_MEMBER_FOUND_EVT: {
      const tBTA_SET_MEMBER_FOUND& set_member_param = p_data->set_member_param;
      BTIF_TRACE_DEBUG("%s: set_id=%d, uuid=%d ", __func__,
                     set_member_param.set_id,
                     set_member_param.uuid);
    } break;

    case BTA_CSIP_LOCK_AVAILABLE_EVT: {
      const tBTA_LOCK_AVAILABLE& lock_available_param = p_data->lock_available_param;
      BTIF_TRACE_DEBUG("%s: app_id=%d, set_id=%d ", __func__,
                   lock_available_param.app_id, lock_available_param.set_id);
    } break;
  }
}

static void btif_acm_handle_csip_status_locked(std::vector<RawAddress> addr,
                                               uint8_t setId) {
  if (addr.empty()) {
    BTIF_TRACE_ERROR("%s: vector size is empty", __func__);
    return;
  }

  tA2DP_CTRL_CMD pending_cmd;// = A2DP_CTRL_CMD_START;//TODO: change to None
  pending_cmd =  btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
  std::vector<RawAddress>::iterator itr;
  int req = 0;

  if (pending_cmd == A2DP_CTRL_CMD_START) {
    req = BTIF_ACM_START_STREAM_REQ_EVT;
  } else if (pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
    req = BTIF_ACM_SUSPEND_STREAM_REQ_EVT;
  } else if (pending_cmd == A2DP_CTRL_CMD_STOP) {
    req = BTIF_ACM_STOP_STREAM_REQ_EVT;
  } else {
    BTIF_TRACE_EVENT("%s: No pending command, check if this list of peers belong "
                     "to MusicActive streaming started group", __func__);
  }
  if (req == BTIF_ACM_STOP_STREAM_REQ_EVT) {
    for (itr = addr.begin(); itr != addr.end(); itr++) {
      btif_acm_initiator_dispatch_stop_req(*itr, false, false);
    }
  } else if (req) {
    for (itr = addr.begin(); itr != addr.end(); itr++) {
      btif_acm_initiator_dispatch_sm_event(*itr,
                          static_cast<btif_acm_sm_event_t>(req));
    }
  }
}

static void btif_acm_check_and_start_conn_Interval_timer(BtifAcmPeer* peer) {

  btif_acm_check_and_cancel_conn_Interval_timer();
  BTIF_TRACE_DEBUG("%s: ", __func__);

  alarm_set_on_mloop(btif_acm_initiator.AcmConnIntervalTimer(),
                     BtifAcmInitiator::kTimeoutConnIntervalMs,
                     btif_acm_initiator_conn_Interval_timer_timeout,
                     (void *)peer);
}

static void btif_acm_check_and_cancel_conn_Interval_timer() {

  BTIF_TRACE_DEBUG("%s: ", __func__);
  if (alarm_is_scheduled(btif_acm_initiator.AcmConnIntervalTimer())) {
    alarm_cancel(btif_acm_initiator.AcmConnIntervalTimer());
  }
}

static void btif_acm_initiator_conn_Interval_timer_timeout(void *data) {

  BTIF_TRACE_DEBUG("%s: ", __func__);
  BtifAcmPeer *peer = (BtifAcmPeer *)data;
  tBTA_ACM_CONN_UPDATE_TIMEOUT_INFO p_data;
  p_data.bd_addr = peer->PeerAddress();
  btif_transfer_context(btif_acm_handle_evt, BTA_ACM_CONN_UPDATE_TIMEOUT_EVT,
                        (char*)&p_data,
                        sizeof(tBTA_ACM_CONN_UPDATE_TIMEOUT_INFO), NULL);
}

static void btif_acm_check_and_start_in_call_tracker_timer_(BtifAcmPeer* peer) {

  btif_acm_check_and_cancel_in_call_tracker_timer_();
  BTIF_TRACE_DEBUG("%s: ", __func__);

  alarm_set_on_mloop(btif_acm_initiator.AcmInCallTrackerTimer(),
                     BtifAcmInitiator::kTimeoutTrackInCallIntervalMs,
                     btif_acm_initiator_in_call_tracker_timer_timeout,
                     (void *)peer);
}

static void btif_acm_check_and_cancel_in_call_tracker_timer_() {

  BTIF_TRACE_DEBUG("%s: ", __func__);
  if (alarm_is_scheduled(btif_acm_initiator.AcmInCallTrackerTimer())) {
    BTIF_TRACE_DEBUG("%s: Cancel in call tracker timer.", __func__);
    alarm_cancel(btif_acm_initiator.AcmInCallTrackerTimer());
  }
}

static void btif_acm_initiator_in_call_tracker_timer_timeout(void *data) {

  BtifAcmPeer *peer = (BtifAcmPeer *)data;

  if (peer != nullptr) {
    BTIF_TRACE_DEBUG("%s: Peer: %s", __func__, peer->PeerAddress().ToString().c_str());
    stop_stream_acm_initiator_internal(peer->PeerAddress(), CONTEXT_TYPE_VOICE, true);
  }
}

static void btif_acm_check_and_start_group_procedure_timer(uint8_t setId) {
  uint8_t *arg = NULL;
  arg = (uint8_t *) osi_malloc(sizeof(uint8_t));
  BTIF_TRACE_DEBUG("%s: ", __func__);
  btif_acm_check_and_cancel_group_procedure_timer(setId);

  *arg = setId;
  alarm_set_on_mloop(btif_acm_initiator.AcmGroupProcedureTimer(),
                     BtifAcmInitiator::kTimeoutAcmGroupProcedureMs,
                     btif_acm_initiator_group_procedure_timer_timeout,
                     (void*) arg);

}

static void btif_acm_check_and_cancel_group_procedure_timer(uint8_t setId) {
  if (alarm_is_scheduled(btif_acm_initiator.AcmGroupProcedureTimer())) {
    BTIF_TRACE_ERROR("%s: acm group procedure already running for setId = %d, "
                     "cancel", __func__, setId);
    alarm_cancel(btif_acm_initiator.AcmGroupProcedureTimer());
  }
}

static void btif_acm_initiator_group_procedure_timer_timeout(void *data) {
  BTIF_TRACE_DEBUG("%s: ", __func__);
  tBTA_CSIP_CSET cset_info; // need to do memset ?
  memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
  std::vector<RawAddress> streaming_devices;
  std::vector<RawAddress> non_streaming_devices;
  uint8_t *arg = (uint8_t*) data;
  if (!arg) {
    BTIF_TRACE_ERROR("%s: coordinate arg is null, return", __func__);
    return;
  }
  uint8_t setId = *arg;
  if (setId == INVALID_SET_ID) {
    BTIF_TRACE_ERROR("%s: coordinate SetId is invalid, return", __func__);
    if (arg) osi_free(arg);
    return;
  }

  cset_info = BTA_CsipGetCoordinatedSet(setId);
  if (cset_info.size == 0) {
    BTIF_TRACE_ERROR("%s: CSET info size is zero, return", __func__);
    if (arg) osi_free(arg);
    return;
  }
  std::vector<RawAddress>::iterator itr;
  BTIF_TRACE_DEBUG("%s: size of set members %d",
                         __func__, (cset_info.set_members).size());
  if ((cset_info.set_members).size() > 0) {
    for (itr =(cset_info.set_members).begin();
                         itr != (cset_info.set_members).end(); itr++) {
      //BTIF_TRACE_DEBUG("%s: address = %s", __func__, itr->ToString().c_str());
      BtifAcmPeer* peer = btif_acm_initiator.FindPeer(*itr);
      if ((peer == nullptr) || (peer != nullptr && !peer->IsStreaming())) {
        non_streaming_devices.push_back(*itr);
      } else {
        streaming_devices.push_back(*itr);
      }
    }
  }

  if (streaming_devices.size() > 0) {
    tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
    if (pending_cmd == A2DP_CTRL_CMD_STOP ||
       pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
      btif_acm_source_on_suspended(A2DP_CTRL_ACK_SUCCESS);
    } else if (pending_cmd == A2DP_CTRL_CMD_START) {
      btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
    } else {
      BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
    }
    BTIF_TRACE_DEBUG("%s: Get music active setid: %d", __func__,
                        btif_acm_initiator.MusicActiveCSetId());
    btif_acm_check_and_start_lock_release_timer(btif_acm_initiator.MusicActiveCSetId());
    if (streaming_devices.size() < (cset_info.set_members).size()) {
      // this case should continue with mono mode since all set members are not streaming
    }
  } else {
    tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
    if (pending_cmd == A2DP_CTRL_CMD_STOP ||
       pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
      btif_acm_source_on_suspended(A2DP_CTRL_ACK_FAILURE);
    } else if (pending_cmd == A2DP_CTRL_CMD_START) {
      btif_acm_on_started(A2DP_CTRL_ACK_FAILURE);
    } else {
      BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
    }
  }

  if (non_streaming_devices.size() > 0) //do we need to unlock and then disconnect ??
   // le_Acl_disconnect (non_streaming_devices);

  if (arg) osi_free(arg);
}

static void btif_acm_check_and_start_lock_release_timer(uint8_t setId) {
  uint8_t *arg = NULL;
  arg = (uint8_t *) osi_malloc(sizeof(uint8_t));

  btif_acm_check_and_cancel_lock_release_timer(setId);

  *arg = setId;
  alarm_set_on_mloop(btif_acm_initiator.MusicSetLockReleaseTimer(),
                           BtifAcmPeer::kTimeoutLockReleaseMs,
                           btif_acm_initiator_lock_release_timer_timeout,
                           (void*) arg);
}

static void btif_acm_check_and_cancel_lock_release_timer(uint8_t setId) {
  if (alarm_is_scheduled(btif_acm_initiator.MusicSetLockReleaseTimer())) {
    BTIF_TRACE_ERROR("%s: lock release already running for setId = %d, "
                                             "cancel ", __func__, setId);
    alarm_cancel(btif_acm_initiator.MusicSetLockReleaseTimer());
  }
}

static void btif_acm_initiator_lock_release_timer_timeout(void *data) {
  uint8_t *arg = (uint8_t*) data;
  if (!arg) {
    BTIF_TRACE_ERROR("%s: coordinate arg is null, return", __func__);
    return;
  }
  uint8_t setId = *arg;
  if (setId == INVALID_SET_ID) {
    BTIF_TRACE_ERROR("%s: coordinate SetId is invalid, return", __func__);
    if (arg) osi_free(arg);
    return;
  }
  if ((btif_acm_initiator.GetGroupLockStatus(setId) !=
                   BtifAcmInitiator::kFlagStatusLocked) ||
      (btif_acm_initiator.GetGroupLockStatus(setId) !=
                   BtifAcmInitiator::kFlagStatusSubsetLocked)) {
    BTIF_TRACE_ERROR("%s: SetId = %d Lock Status = %d returning",
             __func__, setId, btif_acm_initiator.GetGroupLockStatus(setId));
    if (arg) osi_free(arg);
    return;
  }
  if (!btif_acm_request_csip_unlock(setId)) {
    BTIF_TRACE_ERROR("%s: error unlocking", __func__);
  }
  if (arg) osi_free(arg);
}

static void bta_csip_callback(tBTA_CSIP_EVT event, tBTA_CSIP_DATA* p_data) {
  BTIF_TRACE_DEBUG("%s: event: %d", __func__, event);
  btif_transfer_context(btif_acm_handle_bta_csip_event, event, (char*)p_data,
                        sizeof(tBTA_CSIP_DATA), NULL);
}

// Initializes the ACM interface for initiator mode
static bt_status_t init_acm_initiator(
    btacm_initiator_callbacks_t* callbacks, int max_connected_acceptors,
    const std::vector<CodecConfig>& codec_priorities) {
  BTIF_TRACE_EVENT("%s", __func__);
  return btif_acm_initiator.Init(callbacks, max_connected_acceptors,
                                 codec_priorities);
}

// Establishes the BAP connection with the remote acceptor device
static void connect_int(uint16_t uuid, char* p_param) {
    tBTIF_ACM_CONN_DISC connection;
    memset(&connection, 0, sizeof(tBTIF_ACM_CONN_DISC));
    memcpy(&connection, p_param, sizeof(connection));
    RawAddress peer_address = RawAddress::kEmpty;
    BtifAcmPeer* peer = nullptr;
    peer_address = connection.bd_addr;
    if (uuid == ACM_UUID) {
      peer = btif_acm_initiator.FindOrCreatePeer(peer_address);
    }
    if (peer == nullptr) {
      BTIF_TRACE_ERROR("%s: peer is NULL", __func__);
      return;
    }
    current_active_context_type = connection.contextType;
    peer->SetContextType(connection.contextType);
    peer->SetProfileType(connection.profileType);
    peer->SetPrefProfileType(connection.prefAcmProfile);
    BTIF_TRACE_DEBUG("%s: cummulative_profile_type %d",
                             __func__, peer->GetProfileType());
    peer->StateMachine().ProcessEvent(BTIF_ACM_CONNECT_REQ_EVT, &connection);
}

// Set the active peer for contexttype
static void set_acm_active_peer_int(const RawAddress& peer_address,
                                    uint16_t contextType, uint16_t profileType,
                                    std::promise<void> peer_ready_promise, bool ui_req) {
  BTIF_TRACE_EVENT("%s: peer_address=%s", __func__, peer_address.ToString().c_str());
  if (peer_address.IsEmpty()) {
    int setid = INVALID_SET_ID;
    if (contextType == CONTEXT_TYPE_MUSIC)
      setid = btif_acm_initiator.MusicActiveCSetId();
    else if (contextType == CONTEXT_TYPE_VOICE)
      setid = btif_acm_initiator.VoiceActiveCSetId();

    if (setid < INVALID_SET_ID) {
      tBTA_CSIP_CSET cset_info;
      memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
      cset_info = BTA_CsipGetCoordinatedSet(setid);
      if (cset_info.size != 0) {
        std::vector<RawAddress>::iterator itr;
        BTIF_TRACE_DEBUG("%s: size of set members %d",
                         __func__, (cset_info.set_members).size());
        if ((cset_info.set_members).size() > 0) {
          for (itr =(cset_info.set_members).begin();
                              itr != (cset_info.set_members).end(); itr++) {
            BtifAcmPeer* peer = btif_acm_initiator.FindPeer(*itr);
            if (peer != nullptr && peer->IsStreaming() &&
                    (contextType == peer->GetStreamContextType())) {
              BTIF_TRACE_DEBUG("%s: peer is streaming %s ",
                                __func__, peer->PeerAddress().ToString().c_str());
              btif_acm_initiator_dispatch_stop_req(*itr, false, ui_req);
            }
          }
        }
      }
    } else {
      BTIF_TRACE_DEBUG("%s: set active for twm device ", __func__);
      BtifAcmPeer* peer = nullptr;
      if (contextType == CONTEXT_TYPE_MUSIC)
        peer = btif_acm_initiator.FindPeer(btif_acm_initiator.MusicActivePeer());
      else if (contextType == CONTEXT_TYPE_VOICE)
        peer = btif_acm_initiator.FindPeer(btif_acm_initiator.VoiceActivePeer());

      if (peer != nullptr && peer->IsStreaming() &&
              (contextType == peer->GetStreamContextType())) {
        BTIF_TRACE_DEBUG("%s: peer is streaming %s ", __func__,
                                peer->PeerAddress().ToString().c_str());
        // push the set active dev req to BTIF thread
        if (contextType == CONTEXT_TYPE_MUSIC) {
          btif_acm_initiator_dispatch_stop_req(
                    btif_acm_initiator.MusicActivePeer(), true, ui_req);
        } else if (contextType == CONTEXT_TYPE_VOICE) {
          btif_acm_initiator_dispatch_stop_req(
                    btif_acm_initiator.VoiceActivePeer(), true, ui_req);
        }
      }
    }
  }
  if (!btif_acm_initiator.SetAcmActivePeer(peer_address, contextType, profileType,
                                           std::move(peer_ready_promise), ui_req)) {
    BTIF_TRACE_ERROR("%s: Error setting %s as active peer", __func__,
                     peer_address.ToString().c_str());
  }
}

static bt_status_t connect_acm_initiator(const RawAddress& peer_address,
                                         uint16_t contextType,
                                         uint16_t profileType,
                                         uint16_t preferredAcmProfile) {
  BTIF_TRACE_EVENT("%s: Peer %s contextType=%d profileType=%d "
                   "preferredContext=%d", __func__,
                   peer_address.ToString().c_str(),
                   contextType, profileType,
                   preferredAcmProfile);

  if (!btif_acm_initiator.Enabled()) {
    BTIF_TRACE_WARNING("%s: BTIF ACM Initiator is not enabled", __func__);
    return BT_STATUS_NOT_READY;
  }

  tBTIF_ACM_CONN_DISC conn;
  conn.contextType = contextType;
  conn.profileType = profileType;
  conn.bd_addr = peer_address;
  conn.prefAcmProfile = preferredAcmProfile;
  return btif_transfer_context(connect_int, ACM_UUID, (char*)&conn,
                               sizeof(tBTIF_ACM_CONN_DISC), NULL);
}

static bt_status_t disconnect_acm_initiator(const RawAddress& peer_address,
                                                        uint16_t contextType) {
  BTIF_TRACE_EVENT("%s: Peer %s contextType=%d", __func__,
                     peer_address.ToString().c_str(), contextType);

  if (!btif_acm_initiator.Enabled()) {
    BTIF_TRACE_WARNING("%s: BTIF ACM Initiator is not enabled", __func__);
    return BT_STATUS_NOT_READY;
  }

  BtifAcmPeer* peer = btif_acm_initiator.FindOrCreatePeer(peer_address);
  if (peer == nullptr) {
    BTIF_TRACE_ERROR("%s: peer is NULL", __func__);
    return BT_STATUS_FAIL;
  }

  tBTIF_ACM_CONN_DISC disc;
  peer->ResetContextType(contextType);
  if (contextType == CONTEXT_TYPE_MUSIC) {
    peer->ResetProfileType(BAP|GCP|WMCP|GCP_RX);
    disc.profileType = BAP|GCP|WMCP|GCP_RX;
  } else if (contextType == CONTEXT_TYPE_VOICE) {
    peer->ResetProfileType(BAP_CALL);
    disc.profileType = BAP_CALL;
  } else if (contextType == CONTEXT_TYPE_MUSIC_VOICE) {
    peer->ResetProfileType(BAP|GCP|WMCP|BAP_CALL|GCP_RX);
    disc.profileType = BAP|GCP|WMCP|BAP_CALL|GCP_RX;
  }
  BTIF_TRACE_DEBUG("%s: cummulative_profile_type %d",
                                 __func__, peer->GetProfileType());

  disc.bd_addr = peer_address;
  disc.contextType = contextType;
  btif_transfer_context(btif_acm_handle_evt, BTIF_ACM_DISCONNECT_REQ_EVT,
                        (char*)&disc, sizeof(tBTIF_ACM_CONN_DISC), NULL);
  return BT_STATUS_SUCCESS;
}

bool is_active_dev_streaming() {
  return btif_acm_initiator.IsActiveDevStreaming();
}
bt_status_t set_active_acm_initiator(const RawAddress& peer_address,
                                               uint16_t profileType) {

  BTIF_TRACE_IMP("%s: peer: %s, profile type = %d",
                __func__, peer_address.ToString().c_str(), profileType);
  return set_active_acm_initiator_internal(peer_address, profileType, true);
}

bt_status_t set_active_acm_initiator_internal(const RawAddress& peer_address,
                                            uint16_t profileType, bool ui_req) {
  std::unique_lock<std::mutex> guard(acm_set_active_wait_mutex_);
  bool bypass_context_update = false;
  if (!ui_req) {
    BtifAcmPeer *peer_ = btif_acm_initiator.FindPeer(peer_address);
    if (peer_ != nullptr && peer_->CheckFlags(BtifAcmPeer::kFlagPendingSetActiveDev)) {
      BTIF_TRACE_EVENT("%s: Device switch pending", __func__);
      return BT_STATUS_NOT_READY;
    }
  }
  uint16_t contextType = CONTEXT_TYPE_MUSIC;
  if (profileType == BAP || profileType == GCP || profileType == WMCP)
    contextType = CONTEXT_TYPE_MUSIC;
  else if (profileType == BAP_CALL)
    contextType = CONTEXT_TYPE_VOICE;

  BTIF_TRACE_EVENT("%s: Peer: %s contextType=%d profileType=%d "
                   "active_context_type: %d, current_active_profile_type: %d",
                   __func__, peer_address.ToString().c_str(), contextType,
                    profileType, active_context_type,
                    current_active_profile_type);
  if (!btif_acm_initiator.Enabled()) {
    LOG(WARNING) << __func__ << ": BTIF ACM Initiator is not enabled";
    return BT_STATUS_NOT_READY;
  }

  //UI req for same device profile change, usually for media
  if (ui_req &&
    (!peer_address.IsEmpty() &&
    (profileType != BAP_CALL &&
    peer_address == btif_acm_initiator.MusicActivePeer()))) {
    if (btif_ahim_is_aosp_aidl_hal_enabled()) {
      if (active_context_type != CONTEXT_TYPE_MUSIC) {
        bt_status_t status = BT_STATUS_SUCCESS;
        if (profileType & (GCP | GCP_RX | WMCP)) {
          status = BT_STATUS_NOT_READY;
        }
        LOG(WARNING) << __func__ << ": ALS update to media when voice context active";
        return status;
      }
      if (!btif_acm_update_acm_context(contextType, profileType,
                               current_active_profile_type, peer_address) &&
                               current_active_profile_type == profileType) {
        LOG(WARNING) << __func__ << ": ALS update, Already active for this profile";
        return BT_STATUS_SUCCESS;
      }
      bypass_context_update = true;
    }
  }

  BtifAcmPeer* peer = nullptr;
  if (contextType == CONTEXT_TYPE_MUSIC) {
    int setid = INVALID_SET_ID;
    setid = btif_acm_initiator.MusicActiveCSetId();
    if (setid < INVALID_SET_ID) {
      BTIF_TRACE_EVENT("%s: active music device is group device", __func__);
      tBTA_CSIP_CSET cset_info;
      memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
      cset_info = BTA_CsipGetCoordinatedSet(setid);
      if (cset_info.size != 0) {
        std::vector<RawAddress>::iterator itr;
        BTIF_TRACE_DEBUG("%s: size of set members %d",
                         __func__, (cset_info.set_members).size());
        if ((cset_info.set_members).size() > 0) {
          for (itr =(cset_info.set_members).begin();
                              itr != (cset_info.set_members).end(); itr++) {
            peer = btif_acm_initiator.FindPeer(*itr);
            if ((peer != nullptr) && (peer->GetStreamContextType() == CONTEXT_TYPE_MUSIC) &&
                (peer->CheckFlags(BtifAcmPeer::kFlagPendingStart |
                                  BtifAcmPeer::kFlagPendingLocalSuspend |
                                  BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend |
                                  BtifAcmPeer::kFlagPendingReconfigure |
                                  BtifAcmPeer::kFlagPendingAlsAndMetadataSync|
                                  BtifAcmPeer::kFLagPendingStartAfterReconfig))) {
              BTIF_TRACE_DEBUG("%s: kFlagPendingStart: %d, "
                   "kFlagPendingLocalSuspend: %d, "
                   "kFlagClearSuspendAfterPeerSuspend: %d, "
                   "kFlagPendingReconfigure: %d "
                   "kFlagPendingAlsAndMetadataSync: %d"
                   "kFLagPendingStartAfterReconfig = %d", __PRETTY_FUNCTION__,
                   peer->CheckFlags(BtifAcmPeer::kFlagPendingStart),
                   peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend),
                   peer->CheckFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend),
                   peer->CheckFlags(BtifAcmPeer::kFlagPendingReconfigure),
                   peer->CheckFlags(BtifAcmPeer::kFlagPendingAlsAndMetadataSync),
                   peer->CheckFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig));
              LOG(WARNING) << __func__
                           <<": Active music device is pending start or suspend or reconfig";
              return BT_STATUS_NOT_READY;
            }
          }
        }
      }
    } else if (setid == INVALID_SET_ID) {
      BTIF_TRACE_EVENT("%s: invalid set id.", __func__);
    } else {
      BTIF_TRACE_EVENT("%s: active music device is twm device", __func__);
      peer = btif_acm_initiator.FindPeer(btif_acm_initiator.MusicActivePeer());
      BtifAcmPeer *current_peer = btif_acm_initiator.FindPeer(peer_address);
      if (ui_req && current_peer != nullptr &&
          current_peer->CheckFlags(BtifAcmPeer::kFlagPendingSetActiveDev)) {
          LOG(WARNING) << __func__ << " Pending Active Device switch";
          //current_peer->ClearFlags(BtifAcmPeer::kFlagPendingSetActiveDev);
      } else if ((peer != nullptr) &&
          (peer->CheckFlags(BtifAcmPeer::kFlagPendingStart |
                            BtifAcmPeer::kFlagPendingLocalSuspend |
                            BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend |
                            BtifAcmPeer::kFlagPendingReconfigure |
                            BtifAcmPeer::kFlagPendingAlsAndMetadataSync |
                            BtifAcmPeer::kFLagPendingStartAfterReconfig))) {
        BTIF_TRACE_DEBUG("%s: kFlagPendingStart: %d, "
                   "kFlagPendingLocalSuspend: %d, "
                   "kFlagClearSuspendAfterPeerSuspend: %d, "
                   "kFlagPendingReconfigure: %d "
                   "kFlagPendingAlsAndMetadataSync: %d"
                   "kFLagPendingStartAfterReconfig = %d", __PRETTY_FUNCTION__,
                   peer->CheckFlags(BtifAcmPeer::kFlagPendingStart),
                   peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend),
                   peer->CheckFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend),
                   peer->CheckFlags(BtifAcmPeer::kFlagPendingReconfigure),
                   peer->CheckFlags(BtifAcmPeer::kFlagPendingAlsAndMetadataSync),
                   peer->CheckFlags(BtifAcmPeer::kFLagPendingStartAfterReconfig));
        LOG(WARNING) << __func__
                     <<": Active music device is pending start or suspend or reconfig";
        return BT_STATUS_NOT_READY;
      }
    }
  } else if (contextType == CONTEXT_TYPE_VOICE) {
    int setid = INVALID_SET_ID;
    setid = btif_acm_initiator.VoiceActiveCSetId();
    if (setid < INVALID_SET_ID) {
      BTIF_TRACE_EVENT("%s: active voice device is group device", __func__);
      tBTA_CSIP_CSET cset_info;
      memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
      cset_info = BTA_CsipGetCoordinatedSet(setid);
      if (cset_info.size != 0) {
        std::vector<RawAddress>::iterator itr;
        BTIF_TRACE_DEBUG("%s: size of set members %d",
                         __func__, (cset_info.set_members).size());
        if ((cset_info.set_members).size() > 0) {
          for (itr =(cset_info.set_members).begin();
                              itr != (cset_info.set_members).end(); itr++) {
            peer = btif_acm_initiator.FindPeer(*itr);
            if ((peer != nullptr) && (peer->GetStreamContextType() == CONTEXT_TYPE_VOICE) &&
                (peer->CheckFlags(BtifAcmPeer::kFlagPendingStart |
                                  BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend |
                                  BtifAcmPeer::kFlagPendingLocalSuspend))) {
              BTIF_TRACE_DEBUG("%s: kFlagPendingStart: %d, "
                   "kFlagPendingLocalSuspend: %d, "
                   "kFlagClearSuspendAfterPeerSuspend: %d ", __PRETTY_FUNCTION__,
                   peer->CheckFlags(BtifAcmPeer::kFlagPendingStart),
                   peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend),
                   peer->CheckFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend));
              LOG(WARNING) << __func__
                           << ": Active voice device is pending start or suspend";
              return BT_STATUS_NOT_READY;
            }
          }
        }
      }
    } else if (setid == INVALID_SET_ID) {
      BTIF_TRACE_EVENT("%s: invalid set id.", __func__);
    } else {
      BTIF_TRACE_EVENT("%s: active voice device is twm device", __func__);
      peer = btif_acm_initiator.FindPeer(btif_acm_initiator.VoiceActivePeer());
      BtifAcmPeer *current_peer = btif_acm_initiator.FindPeer(peer_address);
      if (ui_req && current_peer != nullptr &&
           current_peer->CheckFlags(BtifAcmPeer::kFlagPendingSetActiveDev)) {
        LOG(WARNING) << __func__ << " Pending Active Device switch";
        //current_peer->ClearFlags(BtifAcmPeer::kFlagPendingSetActiveDev);
      } else if ((peer != nullptr) &&
          (peer->CheckFlags(BtifAcmPeer::kFlagPendingStart |
                            BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend |
                            BtifAcmPeer::kFlagPendingLocalSuspend))) {
        BTIF_TRACE_DEBUG("%s: kFlagPendingStart: %d, "
                   "kFlagPendingLocalSuspend: %d, "
                   "kFlagClearSuspendAfterPeerSuspend: %d ", __PRETTY_FUNCTION__,
                   peer->CheckFlags(BtifAcmPeer::kFlagPendingStart),
                   peer->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend),
                   peer->CheckFlags(BtifAcmPeer::kFlagClearSuspendAfterPeerSuspend));
        LOG(WARNING) << __func__ << ": Active voice device is pending start or suspend";
        return BT_STATUS_NOT_READY;
      }
    }
  }
  if (btif_ahim_is_aosp_aidl_hal_enabled()) {
    if (!bypass_context_update && !peer_address.IsEmpty()) {
      if (!btif_acm_update_acm_context(contextType, profileType,
                                       current_active_profile_type,
                                       peer_address)) {
        LOG(WARNING) << __func__ << ": Already active for this profile";
        return BT_STATUS_SUCCESS;
      }
    }
  }

  std::promise<void> peer_ready_promise;
  std::future<void> peer_ready_future = peer_ready_promise.get_future();
  set_acm_active_peer_int(peer_address, contextType, profileType,
                          std::move(peer_ready_promise), ui_req);
  return BT_STATUS_SUCCESS;
}

static bt_status_t start_stream_acm_initiator(const RawAddress& peer_address,
                                              uint16_t contextType) {
  BTIF_TRACE_DEBUG("%s: Peer: %s, contextType: %d ", __func__,
                               peer_address.ToString().c_str(),contextType);

  if (!btif_acm_initiator.Enabled()) {
    BTIF_TRACE_WARNING("%s: BTIF ACM Initiator is not enabled", __func__);
    return BT_STATUS_NOT_READY;
  }
  int id = btif_acm_initiator.VoiceActiveCSetId();
  if (id < INVALID_SET_ID) {
    tBTA_CSIP_CSET cset_info;
    memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
    cset_info = BTA_CsipGetCoordinatedSet(id);
    std::vector<RawAddress>::iterator itr;
    BTIF_TRACE_DEBUG("%s: size of set members %d",
                        __func__, (cset_info.set_members).size());
    if ((cset_info.set_members).size() > 0) {
      bool start_sent = false;
      for (itr =(cset_info.set_members).begin();
                             itr != (cset_info.set_members).end(); itr++) {
         BTIF_TRACE_DEBUG("%s: Sending start request ", __func__);
         BtifAcmPeer* p = btif_acm_initiator.FindPeer(*itr);
         if (p && p->IsConnected() && p->GetPeerVoiceRxState() == StreamState::CONNECTED &&
                                      p->GetPeerVoiceTxState() == StreamState::CONNECTED &&
                      !(p->IsStreaming() && p->GetStreamContextType() == CONTEXT_TYPE_MUSIC)) {
           BTIF_TRACE_DEBUG("%s: Voice is connected, media is not streaming. ", __func__);
           if (btif_ahim_is_aosp_aidl_hal_enabled()) {
             btif_acm_check_and_cancel_in_call_tracker_timer_();
             if (p->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend)) {
               BTIF_TRACE_DEBUG("%s: pending local suspend, unable to start", __func__);
               btif_acm_on_started(A2DP_CTRL_ACK_FAILURE);
               return BT_STATUS_FAIL;
             }
           }
           p->SetStreamContextType(contextType);
           p->SetStreamProfileType(current_active_profile_type);
           start_sent = true;
           btif_acm_initiator_dispatch_sm_event(*itr, BTIF_ACM_START_STREAM_REQ_EVT);
         } else {
           BTIF_TRACE_DEBUG("%s: Unable to send start to group device ", __func__);
         }
      }
      if (!start_sent) {
        btif_acm_on_started(A2DP_CTRL_ACK_FAILURE);
      }
    }
    btif_acm_check_and_start_group_procedure_timer(btif_acm_initiator.VoiceActiveCSetId());
  } else {
    BTIF_TRACE_DEBUG("%s: Sending start to twm device ", __func__);
    BtifAcmPeer* p = btif_acm_initiator.FindPeer(btif_acm_initiator.VoiceActivePeer());
    if (p != nullptr && p->IsConnected() && p->GetPeerVoiceRxState() == StreamState::CONNECTED &&
                                            p->GetPeerVoiceTxState() == StreamState::CONNECTED &&
                        !(p->IsStreaming() && p->GetStreamContextType() == CONTEXT_TYPE_MUSIC)) {
      BTIF_TRACE_DEBUG("%s: p->GetStreamContextType(): %d", __func__, p->GetStreamContextType());
      if (btif_ahim_is_aosp_aidl_hal_enabled()) {
        btif_acm_check_and_cancel_in_call_tracker_timer_();
        if (p->CheckFlags(BtifAcmPeer::kFlagPendingLocalSuspend)) {
          BTIF_TRACE_DEBUG("%s: pending local suspend, unable to start", __func__);
          btif_acm_on_started(A2DP_CTRL_ACK_FAILURE);
          return BT_STATUS_FAIL;
        }
      }

      if (btif_ahim_is_aosp_aidl_hal_enabled() &&
          p->IsStreaming() && p->GetStreamContextType() == CONTEXT_TYPE_VOICE) {
         BTIF_TRACE_DEBUG("%s: Already voice is in streaming state, Ack success: %d", __func__);
         btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
         return BT_STATUS_SUCCESS;
      }
      BTIF_TRACE_DEBUG("%s: Voice is connected, media is not streaming. ", __func__);
      p->SetStreamContextType(CONTEXT_TYPE_VOICE);
      p->SetStreamProfileType(current_active_profile_type);

      btif_acm_initiator_dispatch_sm_event(btif_acm_initiator.VoiceActivePeer(),
                                           BTIF_ACM_START_STREAM_REQ_EVT);
    } else {
      BTIF_TRACE_DEBUG("%s: Unable to send start to twm device ", __func__);
      btif_acm_on_started(A2DP_CTRL_ACK_FAILURE);
      return BT_STATUS_FAIL;
    }
  }
  return BT_STATUS_SUCCESS;
}

static bt_status_t stop_stream_acm_initiator_internal(
                                             const RawAddress& peer_address,
                                             uint16_t contextType,
                                             bool in_call_tracker_tout) {
  BTIF_TRACE_DEBUG("%s: Peer: %s, contextType: %d, in_call_tracker_tout: %d",
                               __func__, peer_address.ToString().c_str(),
                               contextType, in_call_tracker_tout);

  if (!btif_acm_initiator.Enabled()) {
    BTIF_TRACE_WARNING("%s: BTIF ACM Initiator is not enabled", __func__);
    return BT_STATUS_NOT_READY;
  }

  int id = btif_acm_initiator.VoiceActiveCSetId();
  if (id < INVALID_SET_ID) {
    tBTA_CSIP_CSET cset_info;
    memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
    cset_info = BTA_CsipGetCoordinatedSet(id);
    std::vector<RawAddress>::iterator itr;
    BTIF_TRACE_DEBUG("%s: size of set members %d",
                            __func__, (cset_info.set_members).size());

    if ((cset_info.set_members).size() > 0) {
      bool stop_sent = false;
      for (itr =(cset_info.set_members).begin();
                        itr != (cset_info.set_members).end(); itr++) {
         BTIF_TRACE_DEBUG("%s: Sending stop request: %s",
                                    __func__, itr->ToString().c_str());
         BtifAcmPeer* p = btif_acm_initiator.FindPeer(*itr);
         if (p && p->IsConnected() &&
             p->GetPeerVoiceRxState() == StreamState::CONNECTED &&
             p->GetPeerVoiceTxState() == StreamState::CONNECTED &&
             p->GetStreamContextType() == CONTEXT_TYPE_VOICE) {
           if (p->IsStreaming()) {
             if (btif_ahim_is_aosp_aidl_hal_enabled() && !in_call_tracker_tout) {
               btif_acm_check_and_start_in_call_tracker_timer_(p);
             } else {
               stop_sent = true;
               btif_acm_initiator_dispatch_stop_req(*itr, false, false);
             }
           } else if (p->IsConnected()) {
             BTIF_TRACE_DEBUG("%s: not streaming, acking back sucess ", __func__);
           }
         } else {
           BTIF_TRACE_DEBUG("%s: Unable to send stop to group device ", __func__);
         }
      }
      if (!stop_sent) {
        btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_SUCCESS, BAP_CALL);
      }
    }
    btif_acm_check_and_start_group_procedure_timer(btif_acm_initiator.VoiceActiveCSetId());
  } else {
    BTIF_TRACE_DEBUG("%s: Sending stop to twm device ", __func__);
    BtifAcmPeer* p = btif_acm_initiator.FindPeer(btif_acm_initiator.VoiceActivePeer());
    if (p != nullptr && p->IsConnected() &&
        p->GetPeerVoiceRxState() == StreamState::CONNECTED &&
        p->GetPeerVoiceTxState() == StreamState::CONNECTED &&
        p->GetStreamContextType() == CONTEXT_TYPE_VOICE) {
      if (btif_ahim_is_aosp_aidl_hal_enabled() && !in_call_tracker_tout) {
        btif_acm_check_and_start_in_call_tracker_timer_(p);
        btif_acm_source_on_suspended(A2DP_CTRL_ACK_SUCCESS);
      } else {
        btif_acm_initiator_dispatch_stop_req(btif_acm_initiator.VoiceActivePeer(),
                                           false, false);
      }
    } else {
      BTIF_TRACE_DEBUG("%s: Unable to send stop to twm device ", __func__);
      if (p == nullptr)
        btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_SUCCESS, BAP_CALL);
      else
        btif_acm_source_profile_on_suspended(A2DP_CTRL_ACK_FAILURE, BAP_CALL);
      return BT_STATUS_FAIL;
    }
  }
  return BT_STATUS_SUCCESS;
}


static bt_status_t stop_stream_acm_initiator(const RawAddress& peer_address,
                                             uint16_t contextType) {

  BTIF_TRACE_DEBUG("%s: Peer: %s, contextType: %d", __func__,
                         peer_address.ToString().c_str(), contextType);
  return stop_stream_acm_initiator_internal(peer_address, contextType, true);

}


static bt_status_t codec_config_acm_initiator(const RawAddress& peer_address,
                                std::vector<CodecConfig> codec_preferences,
                                uint16_t contextType, uint16_t profileType) {
  BTIF_TRACE_EVENT("%s", __func__);

  if (!btif_acm_initiator.Enabled()) {
    BTIF_TRACE_IMP("%s: BTIF ACM Initiator is not enabled", __func__);
    return BT_STATUS_NOT_READY;
  }

  if (peer_address.IsEmpty()) {
    BTIF_TRACE_IMP("%s: BTIF ACM Initiator, peer empty", __func__);
    return BT_STATUS_PARM_INVALID;
  }

  std::promise<void> peer_ready_promise;
  std::future<void> peer_ready_future = peer_ready_promise.get_future();
  bt_status_t status = BT_STATUS_SUCCESS;
  if (status == BT_STATUS_SUCCESS) {
    peer_ready_future.wait();
  } else {
    BTIF_TRACE_IMP("%s: BTIF ACM Initiator, fails to config codec", __func__);
  }
  return status;
}

static bt_status_t change_codec_config_acm_initiator(const RawAddress& peer_address,
                                                     char* msg) {
  BTIF_TRACE_DEBUG("%s: codec change string: %s", __func__, msg);
  return change_codec_config_acm_initiator_req(peer_address, msg, true);
}

static bt_status_t change_codec_config_acm_initiator_req(const RawAddress& peer_address,
                                                     char* msg, bool isUIReq) {
  BTIF_TRACE_DEBUG("%s: codec change string: %s", __func__, msg);
  bool is_reconfig_codecQos_selected = false;
  tBTIF_ACM_RECONFIG data;
  if (!btif_acm_initiator.Enabled()) {
    BTIF_TRACE_IMP("%s: BTIF ACM Initiator is not enabled", __func__);
    return BT_STATUS_NOT_READY;
  }

  if (peer_address.IsEmpty()) {
    BTIF_TRACE_IMP("%s: BTIF ACM Initiator, peer empty", __func__);
    return BT_STATUS_PARM_INVALID;
  }
  BtifAcmPeer* peer = btif_acm_initiator.FindPeer(peer_address);
  if (peer == nullptr) {
    LOG(WARNING) << __func__ << ": BTIF ACM Initiator, peer is null";
    return BT_STATUS_FAIL;
  }
  std::string requested_cfg;
  requested_cfg.assign(msg);

  CodecQosConfig codec_qos_cfg;
  memset(&codec_qos_cfg, 0, sizeof(codec_qos_cfg));
  if (!strcmp(msg, "GCP_TX") && peer->GetContextType() == CONTEXT_TYPE_MUSIC) {
    data.bd_addr = peer_address;
    data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;
    data.streams_info.stream_type.audio_context = CONTENT_TYPE_MEDIA;
    data.streams_info.stream_type.direction = ASE_DIRECTION_SINK;
    data.streams_info.reconf_type = bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
    is_reconfig_codecQos_selected = peer->SetCodecQosConfig(peer_address, GCP,
                                          MEDIA_CONTEXT, SNK, false, requested_cfg);
    BTIF_TRACE_DEBUG("%s: Game Tx, is_reconfig_codecQos_selected: %d",
                                    __func__, is_reconfig_codecQos_selected);
    if (is_reconfig_codecQos_selected == false) {
      BTIF_TRACE_DEBUG("%s: reconfig codecQos not available for Game Tx, return", __func__);
      return BT_STATUS_FAIL;
    }
    codec_qos_cfg = peer->get_peer_media_codec_qos_config();
    data.streams_info.codec_qos_config_pair.push_back(codec_qos_cfg);
  } else if (!strcmp(msg, "GCP_RX") && peer->GetContextType() == CONTEXT_TYPE_MUSIC) {
    data.bd_addr = peer_address;
    data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;
    data.streams_info.stream_type.direction = ASE_DIRECTION_SINK;
    data.streams_info.reconf_type = bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
    is_reconfig_codecQos_selected = SelectCodecQosConfig(peer_address,
                                                         GCP, MEDIA_CONTEXT, SNK,
                                                         EB_CONFIG, false, requested_cfg);
    BTIF_TRACE_DEBUG("%s: Game Rx, is_reconfig_codecQos_selected: %d",
                                    __func__, is_reconfig_codecQos_selected);
    if (is_reconfig_codecQos_selected == false) {
      BTIF_TRACE_DEBUG("%s: reconfig codecQos not available for Game Rx, return", __func__);
      return BT_STATUS_FAIL;
    }

    codec_qos_cfg = peer->get_peer_media_codec_qos_config();
    codec_qos_cfg.qos_config.cig_config.cig_id++;
    codec_qos_cfg.qos_config.ascs_configs[0].cig_id++;
    peer->set_peer_media_qos_config(codec_qos_cfg.qos_config);
    peer->set_peer_media_codec_qos_config(codec_qos_cfg);
    data.streams_info.codec_qos_config_pair.push_back(codec_qos_cfg);
  } else if (!strcmp(msg, "MEDIA_TX")) {
    data.bd_addr = peer_address;
    data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;
    data.streams_info.reconf_type = bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
    data.streams_info.stream_type.direction = ASE_DIRECTION_SINK;
    is_reconfig_codecQos_selected = peer->SetCodecQosConfig(peer_address, BAP,
                                          MEDIA_CONTEXT, SNK, false, requested_cfg);
    BTIF_TRACE_DEBUG("%s: Media Tx, is_reconfig_codecQos_selected: %d",
                                    __func__, is_reconfig_codecQos_selected);
    if (is_reconfig_codecQos_selected == false) {
      BTIF_TRACE_DEBUG("%s: reconfig codecQos not available for Media Tx, return", __func__);
      return BT_STATUS_FAIL;
    }

    codec_qos_cfg = peer->get_peer_media_codec_qos_config();
    data.streams_info.codec_qos_config_pair.push_back(codec_qos_cfg);
  } else if (!strcmp(msg, "MEDIA_RX")) {
    data.bd_addr = peer_address;
    data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;
    data.streams_info.stream_type.audio_context = CONTENT_TYPE_LIVE; //Live Audio Context
    data.streams_info.reconf_type = bluetooth::bap::ucast::StreamReconfigType::CODEC_CONFIG;
    data.streams_info.stream_type.direction = ASE_DIRECTION_SRC;
    is_reconfig_codecQos_selected = SelectCodecQosConfig(peer_address, WMCP,
                                                         MEDIA_CONTEXT, SRC,
                                                         EB_CONFIG, false, requested_cfg);
    BTIF_TRACE_DEBUG("%s: Media Rx, is_reconfig_codecQos_selected: %d",
                                    __func__, is_reconfig_codecQos_selected);
    if (is_reconfig_codecQos_selected == false) {
      BTIF_TRACE_DEBUG("%s: reconfig codecQos not available for Media Rx, return", __func__);
      return BT_STATUS_FAIL;
    }
    codec_qos_cfg = peer->get_peer_media_codec_qos_config();
    data.streams_info.codec_qos_config_pair.push_back(codec_qos_cfg);
  } else { // Req from ALS OR Developer Options
    std::vector<std::string> cfg_array;
    CodecIndex codec_type = CodecIndex::CODEC_INDEX_SOURCE_LC3;
    char split_with = '_';
    string token;
    stringstream ss(requested_cfg);
    bool is_csip_device = (peer_address.address[0] == 0x9E &&
                           peer_address.address[1] == 0x8B &&
                           peer_address.address[2] == 0x00);

    while (getline(ss , token , split_with))
      cfg_array.push_back(token);
    if (cfg_array[0] == "APTXLE")
      codec_type = CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE;
    else
      codec_type = CodecIndex::CODEC_INDEX_SOURCE_LC3;
    int freq = std::stoi(cfg_array[1]);
    CodecSampleRate sample_rate = freq_to_sample_rate_map[freq];
    CodecConfig active_cfg = current_active_config;

    // TO-DO: Generalise for CSIP set devices later
    if (is_csip_device) {
      BTIF_TRACE_IMP("%s: Ignore DEV config requested for CSIP devices", __func__);
      return BT_STATUS_SUCCESS;
    }

    //if (dir == SRC && profile_type == GCP_RX)
    //  active_cfg = current_active_gaming_rx_config; TO-DO Check later
    if (((codec_type == active_cfg.codec_type && sample_rate == active_cfg.sample_rate) ||
        (codec_type == CodecIndex::CODEC_INDEX_SOURCE_LC3)) && isUIReq) {
      BTIF_TRACE_IMP("%s: Already set on same config ignore it", __func__);
      return BT_STATUS_SUCCESS;
    }

    std::string old_config = peer->get_peer_dev_requested_config();
    if (isUIReq && (requested_cfg.empty() || (requested_cfg.compare(old_config) == 0))) {
      BTIF_TRACE_IMP("%s: Old and new config are same ignore", __func__);
      return BT_STATUS_SUCCESS;
    }

    data.bd_addr = peer_address;
    data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;
    data.streams_info.stream_type.audio_context = CONTENT_TYPE_MEDIA;
    data.streams_info.reconf_type = bluetooth::bap::ucast::StreamReconfigType::APP_TRIGGERED_CONFIG;
    data.streams_info.stream_type.direction = ASE_DIRECTION_SINK;
    // To-do: Have profile type instead of BAP
    is_reconfig_codecQos_selected = peer->SetCodecQosConfig(peer_address, BAP,
                                               MEDIA_CONTEXT, SNK, false, requested_cfg);
    BTIF_TRACE_DEBUG("%s: ALS/Dev-UI, is_reconfig_codecQos_selected: %d",
                                    __func__, is_reconfig_codecQos_selected);
    if (is_reconfig_codecQos_selected == false) {
      BTIF_TRACE_DEBUG("%s: ALS/Dev-UI, reconfig codecQos not available for Media Tx,"
                                                             " return", __func__);
      return BT_STATUS_FAIL;
    }

    codec_qos_cfg = peer->get_peer_media_codec_qos_config();
    data.streams_info.codec_qos_config_pair.push_back(codec_qos_cfg);
    peer->set_peer_dev_requested_config(requested_cfg);
  }

  print_codec_parameters(codec_qos_cfg.codec_config);
  print_qos_parameters(codec_qos_cfg.qos_config);
  btif_transfer_context(btif_acm_handle_evt, BTIF_ACM_RECONFIG_REQ_EVT, (char*)&data,
                        sizeof(tBTIF_ACM_RECONFIG), NULL);
  return BT_STATUS_SUCCESS;
}

bool reconfig_acm_initiator(const RawAddress& peer_address, int profileType, bool ui_req) {
  BTIF_TRACE_DEBUG("%s: profileType: %d, ui_req = %d", __func__, profileType, ui_req);
  bool status = false;
  tBTIF_ACM_RECONFIG data;
  if (!btif_acm_initiator.Enabled()) {
    LOG(WARNING) << __func__ << ": BTIF ACM Initiator is not enabled";
    return false;
  }

  if (peer_address.IsEmpty()) {
    LOG(WARNING) << __func__ << ": BTIF ACM Initiator, peer empty";
    return false;
  }

  char value[PROPERTY_VALUE_MAX] = {'\0'};
  uint8_t pts_audio_context_ = 0;
  property_get("persist.vendor.btstack.pts_audio_context_", value, "0");

  int res = sscanf(value, "%hhu", &pts_audio_context_);
  APPL_TRACE_DEBUG("%s: res: %d", __func__, res);
  BTIF_TRACE_DEBUG("%s: pts_audio_context_: %d", __func__, pts_audio_context_);

  CodecQosConfig codec_qos_cfg;

  if (peer_address.address[0] == 0x9E && peer_address.address[1] == 0x8B &&
                                     peer_address.address[2] == 0x00) {
    BTIF_TRACE_DEBUG("%s: group address ", __func__);
    tBTA_CSIP_CSET cset_info;
    memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
    cset_info = BTA_CsipGetCoordinatedSet(peer_address.address[5]);
    if (cset_info.size != 0) {
      std::vector<RawAddress>::iterator itr;
      BTIF_TRACE_DEBUG("%s: size of set members %d",
                         __func__, (cset_info.set_members).size());
      if ((cset_info.set_members).size() > 0) {
        for (itr =(cset_info.set_members).begin();
                itr != (cset_info.set_members).end(); itr++) {
          status = false;
          memset(&codec_qos_cfg, 0, sizeof(codec_qos_cfg));
          BtifAcmPeer* grp_peer = btif_acm_initiator.FindPeer(*itr);
          if (grp_peer != nullptr) {
            if ((profileType == GCP) &&
                (grp_peer->GetContextType() & CONTEXT_TYPE_MUSIC)) {
              data.bd_addr = *itr;
              data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;
              data.streams_info.stream_type.audio_context = CONTENT_TYPE_GAME;
              data.streams_info.stream_type.direction = ASE_DIRECTION_SINK;
              status = true;
            } else if ((profileType == GCP_RX) &&
                       (grp_peer->GetContextType() & CONTEXT_TYPE_MUSIC)) {
              data.bd_addr = *itr;
              data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;
              data.streams_info.stream_type.audio_context = CONTENT_TYPE_GAME;
              data.streams_info.stream_type.direction = ASE_DIRECTION_SRC;
              status = true;
            } else if ((profileType == BAP) &&
                       (grp_peer->GetContextType() & CONTEXT_TYPE_MUSIC)) {
              data.bd_addr = *itr;
              data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;

              if ((res == 1) && (pts_audio_context_ != 0)) {
                if (pts_audio_context_ == 2) {
                  data.streams_info.stream_type.audio_context = CONTENT_TYPE_CONVERSATIONAL;
                }
              } else {
                data.streams_info.stream_type.audio_context = CONTENT_TYPE_MEDIA;
              }

              data.streams_info.stream_type.direction = ASE_DIRECTION_SINK;
              status = true;
            } else if ((profileType == WMCP) &&
                       (grp_peer->GetContextType() & CONTEXT_TYPE_MUSIC)) {
              //Live Audio Context
              data.bd_addr = *itr;
              data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;

              if ((res == 1) && (pts_audio_context_ != 0)) {
                if (pts_audio_context_ == 1) {
                  data.streams_info.stream_type.audio_context = CONTENT_TYPE_MEDIA;
                } else if (pts_audio_context_ == 2) {
                  data.streams_info.stream_type.audio_context = CONTENT_TYPE_CONVERSATIONAL;
                }
              } else {
                data.streams_info.stream_type.audio_context = CONTENT_TYPE_LIVE;
              }

              data.streams_info.stream_type.direction = ASE_DIRECTION_SRC;
              status = true;
            } else if ((profileType == BAP_CALL) &&
                       (grp_peer->GetContextType() & CONTEXT_TYPE_VOICE)) {
              data.bd_addr = *itr;
              data.streams_info.stream_type.type = CONTENT_TYPE_CONVERSATIONAL;
              status = true;
            }

            BTIF_TRACE_DEBUG("%s: status: %d", __PRETTY_FUNCTION__, status);

            if(status) {
              if (profileType != BAP_CALL) {
                print_codec_parameters(codec_qos_cfg.codec_config);
                print_qos_parameters(codec_qos_cfg.qos_config);
              }
              if (ui_req) {
                btif_transfer_context(btif_acm_handle_evt, BTIF_ACM_RECONFIG_REQ_EVT,
                                       (char*)&data, sizeof(tBTIF_ACM_RECONFIG), NULL);
              } else {
                btif_acm_handle_evt(BTIF_ACM_RECONFIG_REQ_EVT, (char*)&data);
              }
            } else {
              return status;
            }
          }
        }
      }
    } else {
      LOG(ERROR) << __func__ << ": BTIF ACM Initiator, group peer is null";
      return false;
    }
  } else {
    BTIF_TRACE_DEBUG("%s: not group address ", __func__);
    BtifAcmPeer* peer = btif_acm_initiator.FindPeer(peer_address);
    if (peer == nullptr) {
      LOG(ERROR) << __func__ << ": BTIF ACM Initiator, peer is null";
      return false;
    }

    memset(&codec_qos_cfg, 0, sizeof(codec_qos_cfg));
    if ((profileType == GCP) &&
        (peer->GetContextType() & CONTEXT_TYPE_MUSIC)) {
      data.bd_addr = peer_address;
      data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;
      data.streams_info.stream_type.audio_context = CONTENT_TYPE_GAME;
      data.streams_info.stream_type.direction = ASE_DIRECTION_SINK;
      status = true;
    } else if ((profileType == GCP_RX) &&
               (peer->GetContextType() & CONTEXT_TYPE_MUSIC)) {
      data.bd_addr = peer_address;
      data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;
      data.streams_info.stream_type.audio_context = CONTENT_TYPE_GAME;
      data.streams_info.stream_type.direction = ASE_DIRECTION_SRC;
      status = true;
    } else if ((profileType == BAP) &&
               (peer->GetContextType() & CONTEXT_TYPE_MUSIC)) {
      data.bd_addr = peer_address;
      data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;
      data.streams_info.stream_type.audio_context = CONTENT_TYPE_MEDIA;
      data.streams_info.stream_type.direction = ASE_DIRECTION_SINK;
      status = true;
    } else if ((profileType == WMCP) &&
               (peer->GetContextType() & CONTEXT_TYPE_MUSIC)) {
      //Live Audio Context
      data.bd_addr = peer_address;
      data.streams_info.stream_type.type = CONTENT_TYPE_MEDIA;
      data.streams_info.stream_type.audio_context = CONTENT_TYPE_LIVE;
      data.streams_info.stream_type.direction = ASE_DIRECTION_SRC;
      status = true;
    } else if ((profileType == BAP_CALL) &&
               (peer->GetContextType() & CONTEXT_TYPE_VOICE)) {
      data.bd_addr = peer_address;
      data.streams_info.stream_type.type = CONTENT_TYPE_CONVERSATIONAL;
      status = true;
    }

    BTIF_TRACE_DEBUG("%s: status: %d", __PRETTY_FUNCTION__, status);

    if(status) {
      if (profileType != BAP_CALL) {
        print_codec_parameters(codec_qos_cfg.codec_config);
        print_qos_parameters(codec_qos_cfg.qos_config);
      }
      if (ui_req) {
        btif_transfer_context(btif_acm_handle_evt, BTIF_ACM_RECONFIG_REQ_EVT,
                               (char*)&data, sizeof(tBTIF_ACM_RECONFIG), NULL);
      } else {
        btif_acm_handle_evt(BTIF_ACM_RECONFIG_REQ_EVT, (char*)&data);
      }
    }
  }
  return status;
}

static void cleanup_acm_initiator(void) {
  BTIF_TRACE_EVENT("%s", __func__);
  do_in_bta_thread(FROM_HERE, Bind(&BtifAcmInitiator::Cleanup,
                                   base::Unretained(&btif_acm_initiator)));
}

static const btacm_initiator_interface_t bt_acm_initiator_interface = {
    sizeof(btacm_initiator_interface_t),
    init_acm_initiator,
    connect_acm_initiator,
    disconnect_acm_initiator,
    set_active_acm_initiator,
    start_stream_acm_initiator,
    stop_stream_acm_initiator,
    codec_config_acm_initiator,
    change_codec_config_acm_initiator,
    cleanup_acm_initiator,
};

RawAddress btif_acm_initiator_music_active_peer(void) {
  return btif_acm_initiator.MusicActivePeer();
}

RawAddress btif_acm_initiator_voice_active_peer(void) {
  return btif_acm_initiator.VoiceActivePeer();
}

bool btif_acm_request_csip_lock(uint8_t setId) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  tBTA_CSIP_CSET cset_info; // need to do memset ?
  memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
  cset_info = BTA_CsipGetCoordinatedSet(setId);
  /*if (cset_info.p_srvc_uuid != ACM_UUID) {
    return false;
  }*/
  if (cset_info.size > cset_info.total_discovered) {
    LOG_INFO(LOG_TAG, "%s not complete set discovered yet. size = %d discovered = %d",
                    __func__, cset_info.size, cset_info.total_discovered);
  }
  if (setId == cset_info.set_id) {
    LOG_INFO(LOG_TAG, "%s correct set id", __func__);
  } else {
    return false;
  }

  btif_acm_check_and_cancel_lock_release_timer(setId);

  //Aquire lock for entire group.
  tBTA_SET_LOCK_PARAMS lock_params; //need to do memset ?
  lock_params.app_id = btif_acm_initiator.GetCsipAppId();
  lock_params.set_id = cset_info.set_id;
  lock_params.lock_value = LOCK_VALUE;//For lock
  lock_params.members_addr = cset_info.set_members;
  BTA_CsipSetLockValue (lock_params);
  btif_acm_initiator.SetLockFlags(BtifAcmInitiator::kFlagStatusPendingLock);
  btif_acm_initiator.SetOrUpdateGroupLockStatus(cset_info.set_id,
          btif_acm_initiator.CheckLockFlags(BtifAcmInitiator::kFlagStatusPendingLock));
  return true;
}

bool btif_acm_request_csip_unlock(uint8_t setId) {
  tBTA_CSIP_CSET cset_info; // need to do memset ?
  memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
  cset_info = BTA_CsipGetCoordinatedSet(setId);
  /*if (cset_info.p_srvc_uuid != Uuid::FromString("2B86")) {
    return false;
  }*/
  if (cset_info.size > cset_info.total_discovered) {
    LOG_INFO(LOG_TAG, "%s not complete set discovered yet. size = %d discovered = %d",
                    __func__, cset_info.size, cset_info.total_discovered);
  }
  if (setId == cset_info.set_id) {
    LOG_INFO(LOG_TAG, "%s correct app id", __func__);
  } else {
    return false;
  }
  //Aquire lock for entire group.
  tBTA_SET_LOCK_PARAMS lock_params; //need to do memset ?
  lock_params.app_id = btif_acm_initiator.GetCsipAppId();
  lock_params.set_id = cset_info.set_id;
  lock_params.lock_value = UNLOCK_VALUE;//For Unlock
  lock_params.members_addr = cset_info.set_members;
  BTA_CsipSetLockValue (lock_params);
  btif_acm_initiator.SetLockFlags(BtifAcmInitiator::kFlagStatusPendingUnlock);
  btif_acm_initiator.SetOrUpdateGroupLockStatus(cset_info.set_id,
                btif_acm_initiator.CheckLockFlags(BtifAcmInitiator::kFlagStatusPendingUnlock));
  return true;
}

bool btif_acm_is_call_active(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  BtifAcmPeer* peer = nullptr;
  int setid = INVALID_SET_ID;
  setid = btif_acm_initiator.VoiceActiveCSetId();
  if (setid < INVALID_SET_ID) {
    BTIF_TRACE_EVENT("%s: Call activedevice is group device", __func__);
    tBTA_CSIP_CSET cset_info;
    memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
    cset_info = BTA_CsipGetCoordinatedSet(setid);
    if (cset_info.size != 0) {
      std::vector<RawAddress>::iterator itr;
      BTIF_TRACE_DEBUG("%s: size of set members %d",
                       __func__, (cset_info.set_members).size());
      if ((cset_info.set_members).size() > 0) {
        for (itr =(cset_info.set_members).begin();
                            itr != (cset_info.set_members).end(); itr++) {
          peer = btif_acm_initiator.FindPeer(*itr);
          if (peer != nullptr &&
              (peer->IsStreaming() || peer->CheckFlags(BtifAcmPeer::kFlagPendingStart)) &&
              (peer->GetStreamContextType() == CONTEXT_TYPE_VOICE))
            return true;
        }
      }
    }
  } else {
    BTIF_TRACE_EVENT("%s: Call active device is twm device", __func__);
    peer = btif_acm_initiator.FindPeer(btif_acm_initiator.VoiceActivePeer());
    if (peer != nullptr &&
        (peer->IsStreaming() || peer->CheckFlags(BtifAcmPeer::kFlagPendingStart)) &&
        (peer->GetStreamContextType() == CONTEXT_TYPE_VOICE))
      return true;
  }

  return false;
}

bool btif_acm_get_is_inCall(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  bool is_call_active = false;
  if (btif_acm_initiator.Enabled()) {
    is_call_active = btif_acm_initiator.Callbacks()->is_call_active_cb();
  }
  LOG_INFO(LOG_TAG, "%s: is_call_active: %d", __func__, is_call_active);
  return is_call_active;
}


void btif_acm_stream_start(void) {
  LOG_INFO(LOG_TAG, "%s: active_context_type: %d get_cur_profile_type:%d", __func__, active_context_type, get_cur_profile_type());
  if (!btif_acm_initiator.Enabled())
    return;
  bool ret = false;
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_VOICE) {
      start_stream_acm_initiator(active_bda, CONTEXT_TYPE_VOICE);
      return;
    }
  }
  int id = btif_acm_initiator.MusicActiveCSetId();
  if (id < INVALID_SET_ID) {
    bool keep_pending_ack = false;
    bool send_pos_ack = false;
    bool send_neg_ack = false;
    tBTA_CSIP_CSET cset_info;
    memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
    cset_info = BTA_CsipGetCoordinatedSet(id);
    std::vector<RawAddress>::iterator itr;
    BTIF_TRACE_DEBUG("%s: size of set members %d",
                      __func__, (cset_info.set_members).size());
    if ((cset_info.set_members).size() > 0) {
      for (itr =(cset_info.set_members).begin();
                              itr != (cset_info.set_members).end(); itr++) {
         BTIF_TRACE_DEBUG("%s: Sending start request ", __func__);
         BtifAcmPeer* p = btif_acm_initiator.FindPeer(*itr);
         if (p == nullptr || (current_active_profile_type == WMCP &&
                              p->GetPeerMusicRxState() != StreamState::CONNECTED) ||
                              ((current_active_profile_type == BAP ||
                                current_active_profile_type == GCP ||
                                current_active_profile_type == GCP_RX) &&
                                p->GetPeerMusicTxState() != StreamState::CONNECTED)) {
           BTIF_TRACE_ERROR("%s: Music Rx/Tx is disconnected, returning start failure", __func__);
           send_neg_ack = true;
         } else if (p->IsStreaming() && p->GetStreamContextType() == CONTEXT_TYPE_MUSIC) {
           BTIF_TRACE_DEBUG("%s: Music already streaming ongoing", __func__);
           send_pos_ack = true;
         } else if (p->IsStreaming() && p->GetStreamContextType() == CONTEXT_TYPE_VOICE) {
           BTIF_TRACE_DEBUG("%s: Voice already streaming ongoing", __func__);
           send_neg_ack = true;
         } else if (p->IsConnected()) {
           keep_pending_ack = true;
           p->SetStreamContextType(CONTEXT_TYPE_MUSIC);
           p->SetStreamProfileType(current_active_profile_type);
           btif_acm_initiator_dispatch_sm_event(*itr, BTIF_ACM_START_STREAM_REQ_EVT);
         } else if (p->IsReconfinging() && p->GetStreamContextType() != CONTEXT_TYPE_VOICE && get_cur_profile_type() == GCP) {
           //MIUI ADD
           BTIF_TRACE_DEBUG("%s: peer device:%s is reconfiging.", __func__, itr->ToString().c_str());
           keep_pending_ack = true;
           p->SetStreamContextType(CONTEXT_TYPE_MUSIC);
           p->SetStreamProfileType(GCP);
           btif_acm_initiator_dispatch_sm_event(*itr, BTIF_ACM_START_STREAM_REQ_EVT);
           //END
         } else {
           send_neg_ack = true;
           BTIF_TRACE_DEBUG("%s: peer device is not connected.", __func__);
         }
      }
    } else if ((cset_info.set_members).size() == 0) {
      send_neg_ack = true;
    }
    if (!keep_pending_ack) {
      tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
      if (pending_cmd == A2DP_CTRL_CMD_STOP ||
          pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
        btif_acm_source_on_suspended(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
      } else if (pending_cmd == A2DP_CTRL_CMD_START) {
        if (send_pos_ack)
          btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
        else if (send_neg_ack)
          btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
      } else {
        BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
      }
      return;
    }
    btif_acm_check_and_start_group_procedure_timer(btif_acm_initiator.MusicActiveCSetId());
  } else {
    BTIF_TRACE_DEBUG("%s: Sending start to twm device ", __func__);
    BtifAcmPeer* p = btif_acm_initiator.FindPeer(btif_acm_initiator_music_active_peer());

    if (p != nullptr && p->IsStreaming()) {
      BTIF_TRACE_DEBUG("%s: Already streaming ongoing", __func__);
      btif_acm_on_started(A2DP_CTRL_ACK_SUCCESS);
      return;
    }

    if (p != nullptr && p->IsConnected()) {
      p->SetStreamContextType(CONTEXT_TYPE_MUSIC);
      p->SetStreamProfileType(current_active_profile_type);
      RawAddress peer_addr = btif_acm_initiator_music_active_peer();
      if (peer_addr != RawAddress::kEmpty) {
        BtifAcmPeer *peer_ = btif_acm_initiator.FindPeer(peer_addr);
        if (peer_ == nullptr) {
          BTIF_TRACE_WARNING("%s: peer is NULL", __func__);
          btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
          return;
        }
        if ((current_active_profile_type == WMCP &&
          peer_->GetPeerMusicRxState() == StreamState::DISCONNECTED) ||
          ((current_active_profile_type == BAP ||
          current_active_profile_type == GCP ||
          current_active_profile_type == GCP_RX) &&
          peer_->GetPeerMusicTxState() == StreamState::DISCONNECTED)) {
          tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
          if (pending_cmd == A2DP_CTRL_CMD_START) {
            BTIF_TRACE_ERROR("%s: Music Rx/Tx is disconnected, returning start failure", __func__);
            btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
            return;
          }
        }
      }
      btif_acm_initiator_dispatch_sm_event(btif_acm_initiator_music_active_peer(),
                                           BTIF_ACM_START_STREAM_REQ_EVT);
    } else {
      tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
      if (pending_cmd == A2DP_CTRL_CMD_STOP ||
          pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
        btif_acm_source_on_suspended(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
      } else if (pending_cmd == A2DP_CTRL_CMD_START) {
        btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
      } else {
        BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
      }
    }
  }
}

void btif_acm_stream_stop(void) {
  LOG_INFO(LOG_TAG, "%s ", __func__);
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_VOICE) {
      stop_stream_acm_initiator_internal(active_bda, CONTEXT_TYPE_VOICE, false);
      return;
    }
  }
  BTIF_TRACE_DEBUG("%s: Sending stop to twm device ", __func__);
  BtifAcmPeer* p = btif_acm_initiator.FindPeer(btif_acm_initiator_music_active_peer());
  if (p != nullptr && p->IsConnected()) {
    p->SetStreamContextType(CONTEXT_TYPE_MUSIC);
    btif_acm_initiator_dispatch_stop_req(btif_acm_initiator_music_active_peer(),
                                         false, false);
  } else {
    tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR);
    if (pending_cmd == A2DP_CTRL_CMD_STOP ||
        pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
      btif_acm_source_on_suspended(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
    } else if (pending_cmd == A2DP_CTRL_CMD_START) {
      btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
    } else {
      BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
    }
  }
}

bt_status_t btif_acm_stream_suspend(uint8_t direction) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  if (!btif_acm_initiator.Enabled())
    return BT_STATUS_FAIL;

  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_VOICE) {
      return stop_stream_acm_initiator_internal(active_bda, CONTEXT_TYPE_VOICE, false);
    }
  }

  int id = btif_acm_initiator.MusicActiveCSetId();
  bool keep_pending_ack = false;
  bool send_pos_ack = false;
  bool send_neg_ack = false;
  if (id < INVALID_SET_ID) {
    tBTA_CSIP_CSET cset_info; // need to do memset ?
    memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
    cset_info = BTA_CsipGetCoordinatedSet(id);
    std::vector<RawAddress>::iterator itr;
    BTIF_TRACE_DEBUG("%s: size of set members %d",
                     __func__, (cset_info.set_members).size());
    if ((cset_info.set_members).size() > 0) {
      for (itr =(cset_info.set_members).begin();
                               itr != (cset_info.set_members).end(); itr++) {
         BTIF_TRACE_DEBUG("%s: Sending suspend request ", __func__);
         BtifAcmPeer* p = btif_acm_initiator.FindPeer(*itr);
         if (p == nullptr || (p->GetPeerMusicTxState() != StreamState::CONNECTED &&
                              p->GetPeerMusicRxState() != StreamState::CONNECTED)) {
           BTIF_TRACE_DEBUG("%s: Didn't find peer device or media not connected", __func__);
           send_neg_ack = true;
         } else {
           BTIF_TRACE_DEBUG("%s: context is %d, stateId: %d, addr: %s", __func__,
                            p->GetStreamContextType(), p->StateMachine().StateId(),
                                                  p->PeerAddress().ToString().c_str());
           if (p->GetStreamContextType() == CONTEXT_TYPE_MUSIC && p->IsStreaming()) {
             BTIF_TRACE_DEBUG("%s: media is streaming, suspend it.",  __func__);
             keep_pending_ack = true;
             btif_acm_initiator_dispatch_stop_req(*itr, false, false);
           } else {
             send_pos_ack = true;
           }
         }
      }
    } else if ((cset_info.set_members).size() == 0) {
      send_neg_ack = true;
    }
    if (!keep_pending_ack) {
      tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR, direction);
      if (pending_cmd == A2DP_CTRL_CMD_STOP ||
          pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
        if (send_pos_ack)
          btif_acm_source_direction_on_suspended(A2DP_CTRL_ACK_SUCCESS, direction);
        else if (send_neg_ack)
          btif_acm_source_direction_on_suspended(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS, direction);
      } else if (pending_cmd == A2DP_CTRL_CMD_START) {
        if (send_neg_ack)
          // TODO: Add direction
          btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
      } else {
        BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
        return BT_STATUS_FAIL;
      }
      return BT_STATUS_SUCCESS;
    }
    btif_acm_check_and_start_group_procedure_timer(btif_acm_initiator.MusicActiveCSetId());
  } else {
    BTIF_TRACE_DEBUG("%s: Sending suspend to twm device ", __func__);
    BtifAcmPeer* p = btif_acm_initiator.FindPeer(btif_acm_initiator_music_active_peer());
    if (p != nullptr && p->IsStreaming()) {
      p->SetStreamContextType(CONTEXT_TYPE_MUSIC);
      //p->SetStreamProfileType(0);
      btif_acm_initiator_dispatch_stop_req(btif_acm_initiator_music_active_peer(),
                                         false, false);
    } else {
      tA2DP_CTRL_CMD pending_cmd = btif_ahim_get_pending_command(AUDIO_GROUP_MGR, direction);
      if (pending_cmd == A2DP_CTRL_CMD_STOP ||
          pending_cmd == A2DP_CTRL_CMD_SUSPEND) {
        btif_acm_source_direction_on_suspended(A2DP_CTRL_ACK_SUCCESS, direction);
      } else if (pending_cmd == A2DP_CTRL_CMD_START) {
        // TODO: Add direction
        btif_acm_on_started(A2DP_CTRL_ACK_DISCONNECT_IN_PROGRESS);
      } else {
        BTIF_TRACE_DEBUG("%s: no pending command to ack mm audio", __func__);
        return BT_STATUS_FAIL;
      }
    }
  }
  return BT_STATUS_SUCCESS;
}

void btif_acm_disconnect(const RawAddress& peer_address, int context_type) {
  LOG_INFO(LOG_TAG, "%s: peer %s", __func__, peer_address.ToString().c_str());
  disconnect_acm_initiator(peer_address, context_type);
}

static void btif_acm_initiator_dispatch_sm_event(const RawAddress& peer_address,
                                                 btif_acm_sm_event_t event) {
  BtifAcmEvent btif_acm_event(event, nullptr, 0);
  BTIF_TRACE_EVENT("%s: peer_address=%s event=%s", __func__,
                   peer_address.ToString().c_str(),
                   btif_acm_event.ToString().c_str());

  btif_transfer_context(btif_acm_handle_evt, event, (char *)&peer_address,
                        sizeof(RawAddress), NULL);
}

static void btif_acm_initiator_dispatch_stop_req(const RawAddress& peer_address,
                                                 bool wait_for_signal, bool ui_req) {
  std::promise<void> peer_ready;
  tBTIF_ACM_STOP_REQ stop_req;
  stop_req.wait_for_signal = wait_for_signal;
  stop_req.peer_ready_promise = &peer_ready;
  stop_req.bd_addr = peer_address;
  stop_req.stop_profile_type = current_active_profile_type;

  btif_acm_check_and_cancel_in_call_tracker_timer_();

  if (ui_req) {
    btif_transfer_context(btif_acm_handle_evt, BTIF_ACM_STOP_STREAM_REQ_EVT,
                         (char*)&stop_req,
                          sizeof(tBTIF_ACM_STOP_REQ), NULL);
  } else {
    btif_acm_handle_evt(BTIF_ACM_STOP_STREAM_REQ_EVT, (char*)&stop_req);
  }
  if(wait_for_signal) {
    std::future<void> peer_ready_future = peer_ready.get_future();
    peer_ready_future.wait();
    BTIF_TRACE_ERROR("%s: stop req completed", __func__);
  }
}

bt_status_t btif_acm_initiator_execute_service(bool enable) {
  BTIF_TRACE_EVENT("%s: service: %s", __func__,
                   (enable) ? "enable" : "disable");

  if (enable) {
    BTA_RegisterCsipApp(bta_csip_callback,
                        base::Bind([](uint8_t status, uint8_t app_id) {
                        if (status != BTA_CSIP_SUCCESS) {
                           LOG(ERROR) << "Can't register CSIP module ";
                           return;
                        }
                        BTIF_TRACE_DEBUG("App ID: %d", app_id);
                        btif_acm_initiator.SetCsipAppId(app_id);
                        btif_acm_initiator.SetCsipRegistration(true);} ));
    return BT_STATUS_SUCCESS;
  }

  // Disable the service
  //BTA_UnregisterCsipApp();
  return BT_STATUS_FAIL;
}

// Get the ACM callback interface for ACM Initiator profile
const btacm_initiator_interface_t* btif_acm_initiator_get_interface(void) {
  BTIF_TRACE_EVENT("%s", __func__);
  return &bt_acm_initiator_interface;
}

uint16_t btif_acm_get_active_device_latency() {
  BtifAcmPeer* peer = btif_acm_initiator.FindMusicActivePeer();
  if (peer == nullptr) {
    BTIF_TRACE_WARNING("%s: peer is NULL", __func__);
    return 0;
  } else {
    return peer->GetPeerLatency();
  }
}

static bool SelectCodecQosConfig(const RawAddress& bd_addr,
                                 int profile_type, int context_type,
                                 int direction, int config_type,
                                 bool is_both_tx_rx_support,
                                 std::string req_config) {

  BTIF_TRACE_DEBUG("%s: Peer %s , context type: %d, profile_type: %d,"
        " direction: %d config_type %d, is_both_tx_rx_support: %d",
        __func__, bd_addr.ToString().c_str(), context_type,
        profile_type, direction, config_type, is_both_tx_rx_support);

  BtifAcmPeer* peer = btif_acm_initiator.FindPeer(bd_addr);
  if (peer == nullptr) {
    BTIF_TRACE_WARNING("%s: peer is NULL", __func__);
    return false;
  }

  uint8_t CigId = peer->CigId();
  uint8_t set_size = 0;
  tBTA_CSIP_CSET cset_info;
  memset(&cset_info, 0, sizeof(cset_info));
  cset_info = BTA_CsipGetCoordinatedSet(peer->SetId());
  BTIF_TRACE_DEBUG("%s: cset members size: %d",
                    __func__, (uint8_t)(cset_info.size));

  DeviceType dev_type;
  btif_bap_get_device_type(bd_addr, &dev_type,
                           static_cast<CodecDirection>(direction));
  if(dev_type == DeviceType::EARBUD) {
    set_size = cset_info.size;
  } else if(dev_type == DeviceType::HEADSET_SPLIT_STEREO ||
            dev_type == DeviceType::EARBUD_NO_CSIP) {
    set_size = 2;
  } else if(dev_type == DeviceType::HEADSET_STEREO) {
    set_size = 1;
  }

  BTIF_TRACE_DEBUG("%s: device_type: %d",
                   __func__, static_cast<DeviceType>(dev_type));

  CodecConfig codec_config_;
  CodecQosConfig codec_qos_config;
  QosConfig qos_configs;
  CISConfig cis_config;
  std::vector<QoSConfig> vmcp_qos_config;
  std::vector<QoSConfig> cache_gcp_tx_qos_config, cache_gcp_rx_qos_config;
  std::vector<QoSConfig> cache_call_tx_qos_config, cache_call_rx_qos_config;
  int cache_profile_type = 0;
  CodecConfig cache_gcp_tx_codec_config_, cache_gcp_rx_codec_config_;
  CodecConfig cache_call_tx_codec_config_, cache_call_rx_codec_config_;
  int cache_tx_config_type = 0, cache_rx_config_type = 0;
  uint8_t result_num_of_blocks_per_sdu_m_to_s = 0;
  uint8_t result_num_of_blocks_per_sdu_s_to_m = 0;
  bool is_best_codec_config_found = false;

  BTIF_TRACE_WARNING("%s: going for best config", __func__);
  memset(&codec_config_, 0, sizeof(codec_config_));
  memset(&cache_gcp_tx_codec_config_, 0, sizeof(cache_gcp_tx_codec_config_));
  memset(&cache_gcp_rx_codec_config_, 0, sizeof(cache_gcp_rx_codec_config_));
  memset(&cache_call_tx_qos_config, 0, sizeof(cache_call_tx_qos_config));
  memset(&cache_call_rx_qos_config, 0, sizeof(cache_call_rx_qos_config));
  memset(&cache_call_tx_codec_config_, 0, sizeof(cache_call_tx_codec_config_));
  memset(&cache_call_rx_codec_config_, 0, sizeof(cache_call_rx_codec_config_));

  is_best_codec_config_found = select_best_codec_config(bd_addr, context_type, profile_type,
                            &codec_config_, direction, config_type, req_config);
  if (is_best_codec_config_found == false) {
    BTIF_TRACE_WARNING("%s: best codec config not found, return", __func__);
    return false;
  }

  BTIF_TRACE_DEBUG("%s: sample rate : %d, frame_duration: %d, octets: %d, "
                   "channel_mode: %d", __func__,
                   static_cast<uint16_t>(codec_config_.sample_rate),
                   GetFrameDuration(&codec_config_),
                   GetOctsPerFrame(&codec_config_),
                   static_cast<uint16_t>(codec_config_.channel_mode));

  if (is_both_tx_rx_support) {
    //DeviceType cache_device_type;
    if (profile_type == GCP_RX) {
      cache_profile_type = GCP_TX;

      cache_tx_config_type = btif_acm_get_device_config_type(bd_addr, SNK);
      is_best_codec_config_found = select_best_codec_config(bd_addr, context_type, cache_profile_type,
                              &cache_gcp_tx_codec_config_, SNK, cache_tx_config_type, req_config);

      if (is_best_codec_config_found == false) {
        BTIF_TRACE_WARNING("%s: best codec config not found for GCP Tx, return", __func__);
        return false;
      }

      BTIF_TRACE_DEBUG("%s: Gaming Tx: sample rate : %d, frame_duration: %d, "
                       "octets: %d, channel_mode: %d", __func__,
                       static_cast<uint16_t>(cache_gcp_tx_codec_config_.sample_rate),
                       GetFrameDuration(&cache_gcp_tx_codec_config_),
                       GetOctsPerFrame(&cache_gcp_tx_codec_config_),
                       static_cast<uint16_t>(cache_gcp_tx_codec_config_.channel_mode));
    } else if (profile_type == GCP_TX) {
      cache_profile_type = GCP_RX;

      cache_rx_config_type = btif_acm_get_device_config_type(bd_addr, SRC);
      is_best_codec_config_found = select_best_codec_config(bd_addr, context_type, cache_profile_type,
                              &cache_gcp_rx_codec_config_, SRC, cache_rx_config_type, req_config);

      if (is_best_codec_config_found == false) {
        BTIF_TRACE_WARNING("%s: best codec config not found for GCP Rx, return", __func__);
        return false;
      }

      BTIF_TRACE_DEBUG("%s: Gaming Rx: sample rate : %d, frame_duration: %d, "
                       "octets: %d, channel_mode: %d", __func__,
                       static_cast<uint16_t>(cache_gcp_rx_codec_config_.sample_rate),
                       GetFrameDuration(&cache_gcp_rx_codec_config_),
                       GetOctsPerFrame(&cache_gcp_rx_codec_config_),
                       static_cast<uint16_t>(cache_gcp_rx_codec_config_.channel_mode));
    }

    if (context_type == VOICE_CONTEXT) {
      //DeviceType cache_device_type;
      if (profile_type == BAP && direction == SRC) {
        cache_profile_type = BAP;

        cache_tx_config_type = btif_acm_get_device_config_type(bd_addr, SNK);
        is_best_codec_config_found = select_best_codec_config(bd_addr, context_type, cache_profile_type,
                                &cache_call_tx_codec_config_, SNK, cache_tx_config_type, req_config);

        if (is_best_codec_config_found == false) {
          BTIF_TRACE_WARNING("%s: best codec config not found for BAP Call Tx, return", __func__);
          return false;
        }

        BTIF_TRACE_DEBUG("%s: BAP Call Tx: sample rate : %d, frame_duration: %d, "
                         "octets: %d, channel_mode: %d", __func__,
                         static_cast<uint16_t>(cache_call_tx_codec_config_.sample_rate),
                         GetFrameDuration(&cache_call_tx_codec_config_),
                         GetOctsPerFrame(&cache_call_tx_codec_config_),
                         static_cast<uint16_t>(cache_call_tx_codec_config_.channel_mode));
      } else if (profile_type == BAP && direction == SNK) {
        cache_profile_type = BAP;

        cache_rx_config_type = btif_acm_get_device_config_type(bd_addr, SRC);
        is_best_codec_config_found = select_best_codec_config(bd_addr, context_type, cache_profile_type,
                                &cache_call_rx_codec_config_, SRC, cache_rx_config_type, req_config);

        if (is_best_codec_config_found == false) {
          BTIF_TRACE_WARNING("%s: best codec config not found for BAP Call Rx, return", __func__);
          return false;
        }

        BTIF_TRACE_DEBUG("%s: BAP Call Rx: sample rate : %d, frame_duration: %d, "
                         "octets: %d, channel_mode: %d", __func__,
                         static_cast<uint16_t>(cache_call_rx_codec_config_.sample_rate),
                         GetFrameDuration(&cache_call_rx_codec_config_),
                         GetOctsPerFrame(&cache_call_rx_codec_config_),
                         static_cast<uint16_t>(cache_call_rx_codec_config_.channel_mode));
      }
    }
  }

  CodecIndex codec_config_codec_index = codec_config_.codec_type;
  BTIF_TRACE_DEBUG("%s: Codec type of codec_config_: %d, ",__func__, codec_config_codec_index);
  CodecIndex cache_gcp_tx_codec_index = cache_gcp_tx_codec_config_.codec_type;

  if (context_type == MEDIA_CONTEXT) {
    if (profile_type != WMCP) {
      uint8_t target_context = MEDIA_LL_CONTEXT;
      if (profile_type == BAP || profile_type == TMAP) {
        target_context = MEDIA_HR_CONTEXT;
      }
      vmcp_qos_config = get_qos_params_for_codec(profile_type,
                                      target_context,
                                      codec_config_.sample_rate,
                                      GetFrameDuration(&codec_config_),
                                      GetOctsPerFrame(&codec_config_),
                                      codec_config_codec_index,
                                      GetVendorMetaDataCodecEncoderVer(&codec_config_),
                                      bd_addr);
      if (is_both_tx_rx_support) {
        if (profile_type == GCP_RX) {
          BTIF_TRACE_WARNING("%s: Cache gcp Tx config", __func__);
          cache_profile_type = GCP_TX;
          cache_gcp_tx_qos_config = get_qos_params_for_codec(cache_profile_type,
                                        MEDIA_LL_CONTEXT,
                                        cache_gcp_tx_codec_config_.sample_rate,
                                        GetFrameDuration(&cache_gcp_tx_codec_config_),
                                        GetOctsPerFrame(&cache_gcp_tx_codec_config_),
                                        cache_gcp_tx_codec_index,
                                        GetVendorMetaDataCodecEncoderVer(&cache_gcp_tx_codec_config_),
                                        bd_addr);
        } else if (profile_type == GCP_TX) {
          BTIF_TRACE_WARNING("%s: Cache gcp Rx config", __func__);
          CodecIndex cache_gcp_rx_codec_index = cache_gcp_rx_codec_config_.codec_type;
          cache_profile_type = GCP_RX;
          cache_gcp_rx_qos_config = get_qos_params_for_codec(cache_profile_type,
                                        MEDIA_LL_CONTEXT,
                                        cache_gcp_rx_codec_config_.sample_rate,
                                        GetFrameDuration(&cache_gcp_rx_codec_config_),
                                        GetOctsPerFrame(&cache_gcp_rx_codec_config_),
                                        cache_gcp_rx_codec_index,
                                        GetVendorMetaDataCodecEncoderVer(&cache_gcp_rx_codec_config_),
                                        bd_addr);
        }
      }
    } else {
      vmcp_qos_config = get_qos_params_for_codec(profile_type,
                                      MEDIA_HR_CONTEXT,
                                      codec_config_.sample_rate,
                                      GetFrameDuration(&codec_config_),
                                      GetOctsPerFrame(&codec_config_),
                                      codec_config_codec_index,
                                      GetVendorMetaDataCodecDecoderVer(&codec_config_),
                                      bd_addr);
    }
  } else if (context_type == VOICE_CONTEXT) {
    vmcp_qos_config = get_qos_params_for_codec(profile_type,
                                      VOICE_CONTEXT,
                                      codec_config_.sample_rate,
                                      GetFrameDuration(&codec_config_),
                                      GetOctsPerFrame(&codec_config_),
                                      codec_config_codec_index,
                                      GetVendorMetaDataCodecEncoderVer(&codec_config_),
                                      bd_addr);

    if (is_both_tx_rx_support) {
      if (profile_type == BAP && direction == SRC) {
        BTIF_TRACE_WARNING("%s: Cache BAP Call Tx config", __func__);
        cache_profile_type = BAP;
        cache_call_tx_qos_config = get_qos_params_for_codec(cache_profile_type,
                                      VOICE_CONTEXT,
                                      cache_call_tx_codec_config_.sample_rate,
                                      GetFrameDuration(&cache_call_tx_codec_config_),
                                      GetOctsPerFrame(&cache_call_tx_codec_config_),
                                      CodecIndex::CODEC_INDEX_SOURCE_LC3,
                                      GetVendorMetaDataCodecEncoderVer(&cache_call_tx_codec_config_),
                                      bd_addr);
      } else if (profile_type == BAP && direction == SNK) {
        BTIF_TRACE_WARNING("%s: Cache BAP Call Rx config", __func__);
        cache_profile_type = BAP;
        cache_call_rx_qos_config = get_qos_params_for_codec(cache_profile_type,
                                      VOICE_CONTEXT,
                                      cache_call_rx_codec_config_.sample_rate,
                                      GetFrameDuration(&cache_call_rx_codec_config_),
                                      GetOctsPerFrame(&cache_call_rx_codec_config_),
                                      CodecIndex::CODEC_INDEX_SOURCE_LC3,
                                      GetVendorMetaDataCodecEncoderVer(&cache_call_rx_codec_config_),
                                      bd_addr);
      }
    }
  }

  codec_qos_config.codec_config = codec_config_;
  BTIF_TRACE_DEBUG("%s: vmcp qos size: %d",
                           __func__, (uint8_t)vmcp_qos_config.size());

  if (vmcp_qos_config.empty()) {
    BTIF_TRACE_WARNING("%s: Qos config is empty, return", __func__);
    return false;
  }

  bool qhs_enable = false;
  char qhs_value[PROPERTY_VALUE_MAX] = "false";
  property_get("persist.vendor.btstack.qhs_enable", qhs_value, "false");
  if (!strncmp("true", qhs_value, 4)) {
    if (btm_acl_qhs_phy_supported(bd_addr, BT_TRANSPORT_LE)) {
      qhs_enable = true;
    }
  } else {
    qhs_enable = false;
  }

  //TODO: fill cig id and cis count from
  //Currently it is a single size vector
  for (uint8_t j = 0; j < (uint8_t)vmcp_qos_config.size(); j++) {
    if (vmcp_qos_config[j].mandatory == 0) {
      uint32_t sdu_interval = vmcp_qos_config[j].sdu_int_micro_secs;
      uint16_t max_tx_latency_m_to_s = 0;
      uint16_t max_tx_latency_s_to_m = 0;
      if (is_both_tx_rx_support) {
        if (profile_type == GCP_RX) {
          max_tx_latency_m_to_s = cache_gcp_tx_qos_config[j].max_trans_lat;
        } else {
          max_tx_latency_m_to_s = vmcp_qos_config[j].max_trans_lat;
        }

        if (profile_type == GCP_TX) {
          max_tx_latency_s_to_m = cache_gcp_rx_qos_config[j].max_trans_lat;
        } else {
          max_tx_latency_s_to_m = vmcp_qos_config[j].max_trans_lat;
        }

        if (context_type == VOICE_CONTEXT) {
          BTIF_TRACE_DEBUG("%s: Filling BAP Call mtl, direction: %d",
                                           __func__, direction);
          if (profile_type == BAP && direction == SRC) {
            max_tx_latency_m_to_s = cache_call_tx_qos_config[j].max_trans_lat;
          } else {
            max_tx_latency_m_to_s = vmcp_qos_config[j].max_trans_lat;
          }

          if (profile_type == BAP && direction == SNK) {
            max_tx_latency_s_to_m = cache_call_rx_qos_config[j].max_trans_lat;
          } else {
            max_tx_latency_s_to_m = vmcp_qos_config[j].max_trans_lat;
          }
        }
      } else {
        max_tx_latency_m_to_s = vmcp_qos_config[j].max_trans_lat;
        max_tx_latency_s_to_m = vmcp_qos_config[j].max_trans_lat;
      }

      codec_qos_config.qos_config.cig_config = {
                  .cig_id = CigId,
                  .cis_count = set_size,
                  .packing = 0x01, //interleaved
                  .framing =  vmcp_qos_config[j].framing, //unframed
                  .max_tport_latency_m_to_s = max_tx_latency_m_to_s,
                  .max_tport_latency_s_to_m = max_tx_latency_s_to_m,
                  .sdu_interval_m_to_s = {
                             static_cast<uint8_t>(sdu_interval & 0xFF),
                             static_cast<uint8_t>((sdu_interval >> 8)& 0xFF),
                             static_cast<uint8_t>((sdu_interval >> 16)& 0xFF)
                           },
                  .sdu_interval_s_to_m = {
                             static_cast<uint8_t>(sdu_interval & 0xFF),
                             static_cast<uint8_t>((sdu_interval >> 8)& 0xFF),
                             static_cast<uint8_t>((sdu_interval >> 16)& 0xFF)
                           }
                  };
      BTIF_TRACE_DEBUG("%s: framing: %d, transport latency: %d"
                       " sdu_interval: %d", __func__,
                        vmcp_qos_config[j].framing,
                        vmcp_qos_config[j].max_trans_lat,
                        vmcp_qos_config[j].sdu_int_micro_secs);
      BTIF_TRACE_DEBUG("%s: CIG: packing: %d, transport latency m to s: %d,"
              " transport latency s to m: %d", __func__,
              codec_qos_config.qos_config.cig_config.packing,
              codec_qos_config.qos_config.cig_config.max_tport_latency_m_to_s,
              codec_qos_config.qos_config.cig_config.max_tport_latency_s_to_m);
      BTIF_TRACE_DEBUG("%s: Filled CIG config ", __func__);
    }
  }

  uint8_t version = GetVendorMetaDataCodecEncoderVer(&codec_config_);
  bool is_mono_config = (profile_type == GCP_RX) &&
                        (version == LC3Q_CODEC_FT_CHANGE_SUPPORTED_VERSION);
  bool set_unidirectional_cis = false;
  BTIF_TRACE_DEBUG("%s: set_size: %d ", __func__, set_size);
  BTIF_TRACE_DEBUG("%s: is_mono_config: %d ", __func__, is_mono_config);
  for (uint8_t i = 0; i < set_size; i++) {
    //Currently it is a single size vector
    uint8_t check_memset = 0;
    for (uint8_t j = 0; j < (uint8_t)vmcp_qos_config.size(); j++) {
      memset(&cis_config, 0, sizeof(cis_config));
      if (!check_memset)
        check_memset = 1;

      cis_config.cis_id = i;

      if ((context_type == VOICE_CONTEXT && direction == SNK) ||
          (context_type == MEDIA_CONTEXT &&
           (profile_type != WMCP && profile_type != GCP_RX))) {
        if (config_type == STEREO_HS_CONFIG_2) {
          cis_config.max_sdu_m_to_s = 2 * vmcp_qos_config[j].max_sdu_size;
        } else {
          cis_config.max_sdu_m_to_s = vmcp_qos_config[j].max_sdu_size;
        }
        if(codec_config_.codec_type == CodecIndex::CODEC_INDEX_SOURCE_LC3) {
          result_num_of_blocks_per_sdu_m_to_s = GetLc3BlocksPerSdu(&codec_config_);
          BTIF_TRACE_DEBUG("%s: result_num_of_blocks_per_sdu_m_to_s: %d",
                             __func__, result_num_of_blocks_per_sdu_m_to_s);
          cis_config.max_sdu_m_to_s = result_num_of_blocks_per_sdu_m_to_s *
                                                    cis_config.max_sdu_m_to_s;
        }
        BTIF_TRACE_DEBUG("%s: cis_config.max_sdu_m_to_s =  %d",
                           __func__, cis_config.max_sdu_m_to_s);
      } else {
        if ((profile_type == GCP_RX) && is_both_tx_rx_support) {
          //Fill cached gcp Tx config
          BTIF_TRACE_DEBUG("%s: cached Game tx size: %d, "
                           " cache_tx_config_type: %d", __func__,
                           cache_gcp_tx_qos_config.size(),
                           cache_tx_config_type);
          if (cache_gcp_tx_qos_config.size()) {
            if(cache_tx_config_type == STEREO_HS_CONFIG_2) {
              cis_config.max_sdu_m_to_s = 2 *
                        cache_gcp_tx_qos_config[j].max_sdu_size;
            } else {
              cis_config.max_sdu_m_to_s =
                       cache_gcp_tx_qos_config[j].max_sdu_size;
            }
          }
          result_num_of_blocks_per_sdu_m_to_s =
                            GetLc3BlocksPerSdu(&cache_gcp_tx_codec_config_);
          BTIF_TRACE_DEBUG("%s: cached Game tx: "
                           "result_num_of_blocks_per_sdu_m_to_s: %d",
                             __func__, result_num_of_blocks_per_sdu_m_to_s);
          cis_config.max_sdu_m_to_s = result_num_of_blocks_per_sdu_m_to_s *
                                                    cis_config.max_sdu_m_to_s;
         BTIF_TRACE_DEBUG("%s: cis_config.max_sdu_m_to_s =  %d",
                            __func__, cis_config.max_sdu_m_to_s);
        } else if ((context_type == VOICE_CONTEXT) &&
                   is_both_tx_rx_support && direction == SRC) {
          //Fill cached gcp Tx config
          BTIF_TRACE_DEBUG("%s: cached BAP call tx size: %d,"
                           " cache_tx_config_type: %d", __func__,
                           cache_call_tx_qos_config.size(),
                           cache_tx_config_type);
          if (cache_call_tx_qos_config.size()) {
            if(cache_tx_config_type == STEREO_HS_CONFIG_2) {
              cis_config.max_sdu_m_to_s = 2 *
                        cache_call_tx_qos_config[j].max_sdu_size;
            } else {
              cis_config.max_sdu_m_to_s =
                       cache_call_tx_qos_config[j].max_sdu_size;
            }
          }
          result_num_of_blocks_per_sdu_m_to_s =
                            GetLc3BlocksPerSdu(&cache_call_tx_codec_config_);
          BTIF_TRACE_DEBUG("%s: cached BAP call tx: "
                           "result_num_of_blocks_per_sdu_m_to_s: %d",
                             __func__, result_num_of_blocks_per_sdu_m_to_s);
          cis_config.max_sdu_m_to_s = result_num_of_blocks_per_sdu_m_to_s *
                                                    cis_config.max_sdu_m_to_s;
          BTIF_TRACE_DEBUG("%s: cis_config.max_sdu_m_to_s =  %d",
                            __func__, cis_config.max_sdu_m_to_s);
        } else {
          cis_config.max_sdu_m_to_s = 0;
        }
      }

      if ((context_type == VOICE_CONTEXT && direction == SRC) ||
          (profile_type == WMCP) || (profile_type == GCP_RX)) {
        if(config_type == STEREO_HS_CONFIG_2) {
          cis_config.max_sdu_s_to_m = 2 * vmcp_qos_config[j].max_sdu_size;
        } else {
          cis_config.max_sdu_s_to_m = vmcp_qos_config[j].max_sdu_size;
        }
        if(codec_config_.codec_type == CodecIndex::CODEC_INDEX_SOURCE_LC3) {
          result_num_of_blocks_per_sdu_s_to_m = GetLc3BlocksPerSdu(&codec_config_);
          BTIF_TRACE_DEBUG("%s: result_num_of_blocks_per_sdu_m_to_s: %d",
                            __func__, result_num_of_blocks_per_sdu_s_to_m);
          cis_config.max_sdu_s_to_m = result_num_of_blocks_per_sdu_s_to_m *
                                                    cis_config.max_sdu_s_to_m;
        }

        if (set_unidirectional_cis) { // TO-DO - check other way to do this
          BTIF_TRACE_DEBUG("%s: temp, made sdu = 0, condition 1",__func__);
          cis_config.max_sdu_s_to_m = 0;
        }
        BTIF_TRACE_DEBUG("%s: cis_config.max_sdu_s_to_m =  %d",
                           __func__, cis_config.max_sdu_s_to_m);
      } else {
        if ((/*profile_type == GCP || */profile_type == GCP_TX) && is_both_tx_rx_support) {
          BTIF_TRACE_DEBUG("%s: cached Game rx size: %d"
                           ", cache_rx_config_type: %d", __func__,
                           cache_gcp_rx_qos_config.size(),
                           cache_rx_config_type);
          if (cache_gcp_rx_qos_config.size()) {
            // TO-DO - check other way to do this
            if(set_unidirectional_cis) { //2nd cis, make sdu_s_to_m because mono for vbc
              cis_config.max_sdu_s_to_m = 0;
            }
            else if(cache_rx_config_type == STEREO_HS_CONFIG_2) {
              cis_config.max_sdu_s_to_m = 2 *
                       cache_gcp_rx_qos_config[j].max_sdu_size;
            } else {
              cis_config.max_sdu_s_to_m =
                      cache_gcp_rx_qos_config[j].max_sdu_size;
            }
          }
          // TO-DO - Better way to do this?
          if (!set_unidirectional_cis) { //because otherwise max_sdu_s_to_m will get overwritten
            result_num_of_blocks_per_sdu_s_to_m =
                                GetLc3BlocksPerSdu(&cache_gcp_rx_codec_config_);
            BTIF_TRACE_DEBUG("%s: cached Game rx: "
                             "result_num_of_blocks_per_sdu_m_to_s: %d",
                               __func__, result_num_of_blocks_per_sdu_s_to_m);
            cis_config.max_sdu_s_to_m = result_num_of_blocks_per_sdu_s_to_m *
                                                      cis_config.max_sdu_s_to_m;
          }
          BTIF_TRACE_DEBUG("%s: cis_config.max_sdu_s_to_m =  %d",
                             __func__, cis_config.max_sdu_s_to_m);
        } else if ((context_type == VOICE_CONTEXT) &&
                   is_both_tx_rx_support && direction == SNK) {
          BTIF_TRACE_DEBUG("%s: cached BAP call rx size: %d,"
                           " cache_rx_config_type: %d", __func__,
                           cache_call_rx_qos_config.size(),
                           cache_rx_config_type);
          if (cache_call_rx_qos_config.size()) {
            if(cache_rx_config_type == STEREO_HS_CONFIG_2) {
              cis_config.max_sdu_s_to_m = 2 *
                       cache_call_rx_qos_config[j].max_sdu_size;
            } else {
              cis_config.max_sdu_s_to_m =
                      cache_call_rx_qos_config[j].max_sdu_size;
            }
          }
          result_num_of_blocks_per_sdu_s_to_m =
                              GetLc3BlocksPerSdu(&cache_call_rx_codec_config_);
          BTIF_TRACE_DEBUG("%s: cached BAP call rx: "
                           "result_num_of_blocks_per_sdu_m_to_s: %d",
                             __func__, result_num_of_blocks_per_sdu_s_to_m);
          cis_config.max_sdu_s_to_m = result_num_of_blocks_per_sdu_s_to_m *
                                                    cis_config.max_sdu_s_to_m;
          BTIF_TRACE_DEBUG("%s: cis_config.max_sdu_s_to_m =  %d",
                             __func__, cis_config.max_sdu_s_to_m);
        } else {
          cis_config.max_sdu_s_to_m = 0;
        }
      }

      BTIF_TRACE_DEBUG("%s: qhs_enable: %d", __func__, qhs_enable);

      if (qhs_enable) {
        cis_config.phy_m_to_s = LE_QHS_PHY;
        cis_config.phy_s_to_m = LE_QHS_PHY;
      } else {
        cis_config.phy_m_to_s = LE_2M_PHY;//2mbps
        cis_config.phy_s_to_m = LE_2M_PHY;
      }

      if (is_both_tx_rx_support) {
        if (profile_type == GCP_RX) {
          cis_config.rtn_m_to_s = cache_gcp_tx_qos_config[j].retrans_num;
        } else {
          cis_config.rtn_m_to_s = vmcp_qos_config[j].retrans_num;
        }

        if (profile_type == GCP || profile_type == GCP_TX) {
          cis_config.rtn_s_to_m = cache_gcp_rx_qos_config[j].retrans_num;
        } else {
          cis_config.rtn_s_to_m = vmcp_qos_config[j].retrans_num;
        }

        if (context_type == VOICE_CONTEXT) {
          BTIF_TRACE_DEBUG("%s: Filling BAP Call rtn, direction: %d",
                                                 __func__, direction);
          if (profile_type == BAP && direction == SRC) {
            cis_config.rtn_m_to_s = cache_call_tx_qos_config[j].retrans_num;
          } else {
            cis_config.rtn_m_to_s = vmcp_qos_config[j].retrans_num;
          }

          if (profile_type == BAP && direction == SNK) {
            cis_config.rtn_s_to_m = cache_call_rx_qos_config[j].retrans_num;
          } else {
            cis_config.rtn_s_to_m = vmcp_qos_config[j].retrans_num;
          }
        }
      } else {
        cis_config.rtn_m_to_s = vmcp_qos_config[j].retrans_num;
        cis_config.rtn_s_to_m = vmcp_qos_config[j].retrans_num;
      }
    }

    if (is_mono_config) {
      BTIF_TRACE_DEBUG("%s: Set Mono config for next CIS", __func__);
      set_unidirectional_cis = true;
    }

    if (!check_memset)
      memset(&cis_config, 0, sizeof(cis_config));
    codec_qos_config.qos_config.cis_configs.push_back(cis_config);
    BTIF_TRACE_DEBUG("%s: Filled CIS config for %d", __func__, i);
  }

  if (is_mono_config) {
    BTIF_TRACE_DEBUG("%s: Reset Mono config ", __func__);
    set_unidirectional_cis = false;
    is_mono_config = false;
  }

  for (uint8_t j = 0; j < (uint8_t)vmcp_qos_config.size(); j++) {
    if (vmcp_qos_config[j].mandatory == 0) {
      uint32_t presen_delay = vmcp_qos_config[j].presentation_delay;
      ASCSConfig ascs_config_1 = {
                      .cig_id = CigId,
                      .cis_id = peer->CisId(),
                      .target_latency = 0x03,//Target higher reliability
                      .bi_directional = false,
                      .presentation_delay = {
                           static_cast<uint8_t>(presen_delay & 0xFF),
                           static_cast<uint8_t>((presen_delay >> 8)& 0xFF),
                           static_cast<uint8_t>((presen_delay >> 16)& 0xFF)}
                      };
      codec_qos_config.qos_config.ascs_configs.push_back(ascs_config_1);
      BTIF_TRACE_DEBUG("%s: presentation delay = %d", __func__, presen_delay);
      BTIF_TRACE_DEBUG("%s: Filled ASCS config for %d",
                                         __func__, ascs_config_1.cis_id);
      if (config_type == STEREO_HS_CONFIG_1) {
        ASCSConfig ascs_config_2 = ascs_config_1;
        ascs_config_2.cis_id = peer->CisId() + 1;
        codec_qos_config.qos_config.ascs_configs.push_back(ascs_config_2);
        BTIF_TRACE_DEBUG("%s: Filled ASCS config for %d",
                                         __func__, ascs_config_2.cis_id);
      }
    }
  }

  if (profile_type == BAP) {
    if (context_type == VOICE_CONTEXT) {
      if (direction == SNK) {
        codec_qos_config.qos_config.cig_config.cig_id = CigId + 2;
        codec_qos_config.qos_config.ascs_configs[0].cig_id = CigId + 2;
        codec_qos_config.qos_config.ascs_configs[0].target_latency = 0x01;
        codec_qos_config.qos_config.ascs_configs[0].bi_directional = true;
        if (config_type == STEREO_HS_CONFIG_1) {
          codec_qos_config.qos_config.ascs_configs[1].cig_id = CigId + 2;
          codec_qos_config.qos_config.ascs_configs[1].target_latency = 0x01;
          codec_qos_config.qos_config.ascs_configs[1].bi_directional = true;
        }
        peer->set_peer_voice_tx_codec_config(codec_config_);
        peer->set_peer_voice_tx_qos_config(codec_qos_config.qos_config);
        peer->set_peer_voice_tx_codec_qos_config(codec_qos_config);
      } else if (direction == SRC) {
        codec_qos_config.qos_config.cig_config.cig_id = CigId + 2;
        codec_qos_config.qos_config.ascs_configs[0].cig_id = CigId + 2;
        codec_qos_config.qos_config.ascs_configs[0].target_latency = 0x01;
        codec_qos_config.qos_config.ascs_configs[0].bi_directional = true;
        if (config_type == STEREO_HS_CONFIG_1) {
          codec_qos_config.qos_config.ascs_configs[1].cig_id = CigId + 2;
          codec_qos_config.qos_config.ascs_configs[1].target_latency = 0x01;
          codec_qos_config.qos_config.ascs_configs[1].bi_directional = true;
        }
        peer->set_peer_voice_rx_codec_config(codec_config_);
        peer->set_peer_voice_rx_qos_config(codec_qos_config.qos_config);
        peer->set_peer_voice_rx_codec_qos_config(codec_qos_config);
      }
    } else {
      peer->set_peer_media_codec_config(codec_config_);
      peer->set_peer_media_qos_config(codec_qos_config.qos_config);
      peer->set_peer_media_codec_qos_config(codec_qos_config);
    }
  } else if (profile_type == WMCP) {
    if (context_type == MEDIA_CONTEXT) {
      codec_qos_config.qos_config.cig_config.cig_id = CigId + 3;
      codec_qos_config.qos_config.ascs_configs[0].cig_id = CigId + 3;
      if (config_type == STEREO_HS_CONFIG_1)
        codec_qos_config.qos_config.ascs_configs[1].cig_id = CigId + 3;
      peer->set_peer_media_codec_config(codec_config_);
      peer->set_peer_media_qos_config(codec_qos_config.qos_config);
      peer->set_peer_media_codec_qos_config(codec_qos_config);
    }
  } else if (profile_type == GCP_RX || profile_type == GCP ||
             profile_type == GCP_TX) {
    if (context_type == MEDIA_CONTEXT) {
      if (direction == SNK) {
        codec_qos_config.qos_config.cig_config.cig_id = CigId + 4;
        codec_qos_config.qos_config.ascs_configs[0].cig_id = CigId + 4;
        codec_qos_config.qos_config.ascs_configs[0].target_latency = 0x01;
        codec_qos_config.qos_config.ascs_configs[0].bi_directional = true;
        if (config_type == STEREO_HS_CONFIG_1) {
          codec_qos_config.qos_config.ascs_configs[1].cig_id = CigId + 4;
          codec_qos_config.qos_config.ascs_configs[1].target_latency = 0x01;
          codec_qos_config.qos_config.ascs_configs[1].bi_directional = true;
        }
        //TODO also check bidirectional
        peer->set_peer_media_codec_config(codec_config_);
        peer->set_peer_media_qos_config(codec_qos_config.qos_config);
        peer->set_peer_media_codec_qos_config(codec_qos_config);
      } else if (direction == SRC) {
        codec_qos_config.qos_config.cig_config.cig_id = CigId + 4;
        codec_qos_config.qos_config.ascs_configs[0].cig_id = CigId + 4;
        codec_qos_config.qos_config.ascs_configs[0].target_latency = 0x01;
        codec_qos_config.qos_config.ascs_configs[0].bi_directional = true;
        if (config_type == STEREO_HS_CONFIG_1) {
          codec_qos_config.qos_config.ascs_configs[1].cig_id = CigId + 4;
          codec_qos_config.qos_config.ascs_configs[1].target_latency = 0x01;
          codec_qos_config.qos_config.ascs_configs[1].bi_directional = true;
        }
        if (is_both_tx_rx_support) {
          //Gaming VBC use-case
          BTIF_TRACE_DEBUG("%s: Actual Gaming VBC Use-Case", __func__);
          peer->set_peer_voice_rx_codec_config(codec_config_);
          peer->set_peer_voice_rx_qos_config(codec_qos_config.qos_config);
          peer->set_peer_voice_rx_codec_qos_config(codec_qos_config);
        } else {
          BTIF_TRACE_DEBUG("%s: During Connection time case", __func__);
          peer->set_peer_media_codec_config(codec_config_);
          peer->set_peer_media_qos_config(codec_qos_config.qos_config);
          peer->set_peer_media_codec_qos_config(codec_qos_config);
        }
      }
    }
  }

  //print_codec_parameters(codec_config_);
  //print_qos_parameters(codec_qos_config.qos_config);
  return true;
}

CodecIndex select_codec_type(const RawAddress& bd_addr, uint16_t context_type,
                             uint16_t profile_type, uint8_t dir) {
  // To-Do: Have priority & classes for each codec for delegation
  CodecIndex codec_type = CodecIndex::CODEC_INDEX_SOURCE_LC3;
  if (context_type == VOICE_CONTEXT || dir == SRC)
    return codec_type;

  BtifAcmPeer* peer = btif_acm_initiator.FindPeer(bd_addr);
  if (peer == nullptr) return codec_type;
  bool is_peer_supports_aptx_le = false;
  std::vector<CodecConfig> selectable_capability = peer->GetSnkSelectableCapability();
  for (auto cap : selectable_capability) {
    if (cap.codec_type == CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE) {
      BTIF_TRACE_IMP(": select_codec_type found Aptx Adaptive LE in remote PAC");
      is_peer_supports_aptx_le = true;
      break;
    }
  }
  if (context_type == MEDIA_CONTEXT && profile_type == BAP && Is_BN_Variation_Supported
      && is_aptx_supported_on_dut && is_peer_supports_aptx_le) {
    BTIF_TRACE_IMP(": select_codec_type as Aptx Adaptive LE");
    codec_type = CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE;
  } else {
    BTIF_TRACE_IMP(": select_codec_type as LC3");
  }
  return codec_type;
}

bool select_best_codec_config(const RawAddress& bd_addr,
                              uint16_t context_type,
                              uint8_t profile_type,
                              CodecConfig *codec_config,
                              int dir, int config_type,
                              std::string req_config) {

    BTIF_TRACE_IMP("%s: select best codec config for context type: %d,"
                     " profile type %d, dir: %d", __func__,
                     context_type, profile_type, dir);

    CodecConfig result_codec_config;
    uint8_t peer_channel_mode = 0;
    uint8_t peer_fram_dur = 0;
    uint16_t peer_min_octets_per_frame = 0;
    uint16_t peer_max_octets_per_frame = 0;
    uint8_t peer_max_sup_lc3_frames = 0;
    uint16_t peer_preferred_context = 0;
    uint8_t peer_encoder_ver = 0;
    uint8_t peer_decoder_ver = 0;
    uint8_t peer_min_sup_frame_dur = 0;
    uint8_t peer_feature_map = 0;
    bool peer_lc3q_pref = 0;
    uint8_t dut_codec_encoder_ver = 0;
    uint8_t dut_codec_decoder_ver = 0;
    uint16_t vmcp_samp_freq = 0;
    uint8_t vmcp_fram_dur = 0;
    uint32_t vmcp_octets_per_frame = 0;
    bool best_codec_found = false;
    std::vector<CodecConfig> pac_record;
    std::vector<CodecConfig> local_codec_config;

    memset(&result_codec_config, 0, sizeof(result_codec_config));
    uint16_t audio_context_type = CONTENT_TYPE_UNSPECIFIED;

    BtifAcmPeer* peer = btif_acm_initiator.FindPeer(bd_addr);
    if (peer == nullptr) {
      BTIF_TRACE_WARNING("%s: peer is NULL", __func__);
      return false;
    }

    if (context_type == MEDIA_CONTEXT) {
      if (profile_type == WMCP) {
        audio_context_type |= CONTENT_TYPE_LIVE;
      } else if (profile_type == GCP || profile_type == GCP_RX ||
                 profile_type == GCP_TX) {
        audio_context_type |= CONTENT_TYPE_GAME;
      } else {
        audio_context_type |= CONTENT_TYPE_MEDIA;
      }
    } else if (context_type == VOICE_CONTEXT) {
      audio_context_type |= CONTENT_TYPE_CONVERSATIONAL;
    }
    BTIF_TRACE_IMP("%s: audio_context_type: %d", __func__, audio_context_type);

    if(dir == SNK) {
      pac_record = peer->GetPeerSnkPacsData();
    } else if(dir == SRC) {
      pac_record = peer->GetPeerSrcPacsData();
    }

    if (pac_record.empty()) {
      BTIF_TRACE_IMP("%s: PAC record is null for direction: %d, return", __func__, dir);
      return false;
    }

    std::vector<std::string> cfg_array;
    CodecIndex codec_type = CodecIndex::CODEC_INDEX_SOURCE_LC3;
    if (req_config.empty()) {
      codec_type = select_codec_type(bd_addr, context_type, profile_type, dir);
    } else {
      char split_with = '_';
      string token;
      stringstream ss(req_config);
      while (getline(ss , token , split_with))
        cfg_array.push_back(token);

      if (cfg_array[0] == "APTXLE")
        codec_type = CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE;
      else
        codec_type = CodecIndex::CODEC_INDEX_SOURCE_LC3;
    }

    BTIF_TRACE_IMP("%s: output of select_codec_type is = %d ", __func__, codec_type);

    local_codec_config = get_all_codec_configs(profile_type, context_type, codec_type, bd_addr);

    BTIF_TRACE_IMP("%s: size of local codec cfg is = %d ",
                                     __func__, local_codec_config.size());

    if (!req_config.empty()) {
      BTIF_TRACE_IMP("%s: Requested config not empty", __func__);
      int freq = std::stoi(cfg_array[1]);
      CodecSampleRate sample_rate = freq_to_sample_rate_map[freq];
      int oct_per_frame = std::stoi(cfg_array[2]);
      int frame_dur = std::stoi(cfg_array[3]);

      if (codec_type == CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE) {
        for (int i = 0; i < (int) local_codec_config.size(); ++i) {
          if (local_codec_config[i].sample_rate != sample_rate) {
            BTIF_TRACE_IMP("%s: Removing element %d from local cfg while choosing for best cfg",
                            __func__, local_codec_config[i].sample_rate);
            local_codec_config.erase(local_codec_config.begin() + i);
          }
        }
      } // To-Do: else for LC3 if any
    }

    BTIF_TRACE_IMP("%s: vmcp codec size: %d",
                         __func__, (uint8_t)local_codec_config.size());

    for (auto it = local_codec_config.begin(); it != local_codec_config.end(); ++it) {
      if (it->codec_type != codec_type) {
        continue;
      }
      if(it->codec_type == CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE) {
        dut_codec_encoder_ver = dut_aptx_encoder_ver;
        dut_codec_decoder_ver = dut_aptx_decoder_ver;
        BTIF_TRACE_IMP("%s: Codec_type is AAR3  ", __func__);
      } else if(it->codec_type == CodecIndex::CODEC_INDEX_SOURCE_LC3){
        dut_codec_encoder_ver = dut_lc3_encoder_ver;
        dut_codec_decoder_ver = dut_lc3_decoder_ver;
        BTIF_TRACE_IMP("%s: Codec_type is LC3  ", __func__);
      }
      BTIF_TRACE_IMP("%s: dut_codec_encoder_ver = %d, dut_codec_decoder_ver = %d ",
                       __func__, dut_codec_encoder_ver, dut_codec_decoder_ver);
      uint8_t local_codec_ver = GetTempVersionNumber(&(*it));
      BTIF_TRACE_IMP("%s: local_codec_ver = %d ",__func__, local_codec_ver);
      UpdateTempVersionNumber(&(*it), 0); //Resetting 8th Byte of cs4
      for (auto it_2 = pac_record.begin(); it_2 != pac_record.end(); ++it_2) {
        uint8_t negotiated_codec_encoder_ver = 0, negotiated_codec_decoder_ver = 0;
        if (it_2->codec_type != codec_type) {
          continue;
        }
        peer_encoder_ver = GetCapaVendorMetaDataCodecEncoderVer(&(*it_2));
        peer_decoder_ver = GetCapaVendorMetaDataCodecDecoderVer(&(*it_2));

        BTIF_TRACE_IMP("%s: peer_encoder_version = %d, peer_decoder_version = %d ",
                          __func__, peer_encoder_ver, peer_decoder_ver);

        negotiated_codec_encoder_ver = std::min(dut_codec_encoder_ver, peer_decoder_ver);
        negotiated_codec_decoder_ver = std::min(dut_codec_decoder_ver, peer_encoder_ver);

        BTIF_TRACE_IMP("%s: negotiated_encoder_ver = %d, negotiated_decoder_ver = %d ",
                         __func__, negotiated_codec_encoder_ver, negotiated_codec_decoder_ver);

        if ((context_type == MEDIA_CONTEXT &&
            (profile_type == BAP || profile_type == GCP ||
             profile_type == GCP_RX || profile_type == GCP_TX)) &&
            (local_codec_ver > negotiated_codec_encoder_ver)) {
          BTIF_TRACE_IMP("%s: local_ver > negotiated_ver,continue because unsupported", __func__);
          continue;
        }

        BTIF_TRACE_IMP("%s: local_codec_config sample_rate: %d and pac_record sample rate: %d ",
                                         __func__, it->sample_rate, it_2->sample_rate);
        peer_fram_dur = GetCapaSupFrameDurations(&(*it_2));
        vmcp_fram_dur = GetFrameDuration(&(*it));
        BTIF_TRACE_IMP("%s: VMCP parameters, peer_fram_dur = %d, vmcp_fram_dur = %d ",
                          __func__, peer_fram_dur, vmcp_fram_dur);
        switch (codec_type) {

          case CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE: {
            if (it_2->sample_rate == it->sample_rate) {
              vmcp_octets_per_frame = GetOctsPerFrame(&(*it));
              peer_channel_mode = static_cast<uint8_t>(it_2->channel_mode);
              peer_min_octets_per_frame = GetCapaSupOctsPerFrame(&(*it_2)) & 0xFFFF;
              peer_max_octets_per_frame = (GetCapaSupOctsPerFrame(&(*it_2)) & 0xFFFF0000) >> 16;
              peer_preferred_context = GetCapaPreferredContexts(&(*it_2));
              peer_min_sup_frame_dur = GetCapaMinSupFrameDur(&(*it_2));
              peer_feature_map = GetCapaFeatureMap(&(*it_2));
              vmcp_samp_freq = static_cast<uint16_t>(it->sample_rate);
              result_codec_config.sample_rate = it->sample_rate;
              BTIF_TRACE_IMP("%s: result_codec_config's sample_rate = %d",__func__, result_codec_config.sample_rate);
              result_codec_config.codec_type = codec_type;
              if (config_type == STEREO_HS_CONFIG_2) {
                result_codec_config.channel_mode = CodecChannelMode::CODEC_CHANNEL_MODE_STEREO;
              } else {
                result_codec_config.channel_mode = CodecChannelMode::CODEC_CHANNEL_MODE_MONO;
              }
              result_codec_config.codec_priority = CodecPriority::CODEC_PRIORITY_DEFAULT;
              UpdateOctsPerFrame(&result_codec_config, vmcp_octets_per_frame);
              //check below because it is not there in spec, (Keeping for now)
              UpdatePreferredAudioContext(&result_codec_config, audio_context_type);
              UpdateVendorMetaDataCodecEncoderVer(&result_codec_config, negotiated_codec_encoder_ver);
              BTIF_TRACE_IMP("%s: Codec is AAR3 and updated result_codec_version's encoder ver =  = %d",__func__,negotiated_codec_encoder_ver);
              UpdateVendorMetaDataCodecDecoderVer(&result_codec_config, negotiated_codec_decoder_ver);
              BTIF_TRACE_IMP("%s: Codec is AAR3 and updated result_codec_version's decoder ver =  = %d",__func__,negotiated_codec_decoder_ver);
              UpdateCapaMinSupFrameDur(&result_codec_config, peer_min_sup_frame_dur);
              UpdateCapaFeatureMap(&result_codec_config, peer_feature_map);
              best_codec_found = true;
              break;
            }
            break;
          }
          case CodecIndex::CODEC_INDEX_SOURCE_LC3: {
            peer_min_octets_per_frame = GetCapaSupOctsPerFrame(&(*it_2)) & 0xFFFF;
            peer_max_octets_per_frame = (GetCapaSupOctsPerFrame(&(*it_2)) & 0xFFFF0000) >> 16;
            vmcp_octets_per_frame = GetOctsPerFrame(&(*it));
            BTIF_TRACE_DEBUG("%s: PAC parameters, min_octets_per_frame=%d,"
                             " max_octets_per_frame=%d", __func__,
                             peer_min_octets_per_frame, peer_max_octets_per_frame);
            BTIF_TRACE_DEBUG("%s: VMCP parameters, octets=%d",
                                  __func__, vmcp_octets_per_frame);
            if ((it_2->sample_rate == it->sample_rate) &&
                (peer_fram_dur & (1 << vmcp_fram_dur)) &&
                (vmcp_octets_per_frame >= peer_min_octets_per_frame &&
                 vmcp_octets_per_frame <= peer_max_octets_per_frame)) {
              peer_channel_mode = static_cast<uint8_t>(it_2->channel_mode);
              peer_max_sup_lc3_frames = GetCapaMaxSupLc3Frames(&(*it_2));
              peer_lc3q_pref = GetCapaVendorMetaDataLc3QPref(&(*it_2));
              peer_preferred_context = GetCapaPreferredContexts(&(*it_2));

              vmcp_samp_freq = static_cast<uint16_t>(it->sample_rate);
              result_codec_config.sample_rate = it->sample_rate;
              result_codec_config.codec_type = CodecIndex::CODEC_INDEX_SOURCE_LC3;
              result_codec_config.codec_priority = CodecPriority::CODEC_PRIORITY_DEFAULT;

              UpdateOctsPerFrame(&result_codec_config, vmcp_octets_per_frame);

              if (is_lc3q_supported_on_dut) {
                UpdateLc3QPreference(&result_codec_config, true);
              }
              UpdateCapaMaxSupLc3Frames(&result_codec_config, peer_max_sup_lc3_frames);

              if (config_type == STEREO_HS_CONFIG_2) {
                result_codec_config.channel_mode = CodecChannelMode::CODEC_CHANNEL_MODE_STEREO;
              } else {
                result_codec_config.channel_mode = CodecChannelMode::CODEC_CHANNEL_MODE_MONO;
              }
              UpdateVendorMetaDataCodecEncoderVer(&result_codec_config, negotiated_codec_encoder_ver);
              BTIF_TRACE_IMP("%s: Codec is LC3 and updated result_codec_version's encoder ver =  = %d",__func__,negotiated_codec_encoder_ver);
              UpdateVendorMetaDataCodecDecoderVer(&result_codec_config, negotiated_codec_decoder_ver);
              BTIF_TRACE_IMP("%s: Codec is LC3 and updated result_codec_version's decoder ver =  = %d",__func__,negotiated_codec_decoder_ver);

              UpdateFrameDuration(&result_codec_config, vmcp_fram_dur);
              UpdateLc3BlocksPerSdu(&result_codec_config,
                                    btif_acm_update_lc3_blocks_per_sdu(
                                                        bd_addr, profile_type,
                                                        context_type, dir,
                                                        peer_max_sup_lc3_frames,
                                                        config_type,
                                                        &result_codec_config));
              UpdatePreferredAudioContext(&result_codec_config, audio_context_type);
              best_codec_found = true;
              break;
            }
            break;
          }

          default:
            break;
        }
        if (best_codec_found) break;
      }
      if (best_codec_found) break;
    }
    BTIF_TRACE_IMP("%s best_codec_found = %d", __func__, best_codec_found);
    BTIF_TRACE_IMP("%s: PAC parameters, min_octets_per_frame=%d,"
                     " max_octets_per_frame=%d, peer_max_sup_lc3_frames=%d",
                     __func__, peer_min_octets_per_frame, peer_max_octets_per_frame,
                     peer_max_sup_lc3_frames);
    BTIF_TRACE_IMP("%s: PAC parameters, peer_preferred_context=%d",
                     __func__, peer_preferred_context);
    BTIF_TRACE_IMP("%s: VMCP parameters, sample_freq=%d,"
                     " frame_duration=%d, octets=%d", __func__,
                      vmcp_samp_freq, vmcp_fram_dur, vmcp_octets_per_frame);

    BTIF_TRACE_IMP("Now printing result_codec_config: ");
    print_codec_parameters(result_codec_config);

    *codec_config = result_codec_config;
    return best_codec_found;
}

uint16_t btif_acm_get_sample_rate(uint8_t direction) {
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  BTIF_TRACE_DEBUG("[ACM]:%s: direction = %d", __func__, direction);

  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }

      if (active_config.sample_rate !=
                    CodecSampleRate::CODEC_SAMPLE_RATE_NONE) {
        BTIF_TRACE_DEBUG("[ACM]:%s: sample_rate = %d",
                         __func__, active_config.sample_rate);
        return static_cast<uint16_t>(active_config.sample_rate);
      }
    } else if( active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));

      if (active_config.sample_rate !=
                    CodecSampleRate::CODEC_SAMPLE_RATE_NONE) {
        BTIF_TRACE_DEBUG("[ACM]:%s: sample_rate = %d",
                         __func__, active_config.sample_rate);
        return static_cast<uint16_t>(active_config.sample_rate);
      }
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
    if (active_config.sample_rate !=
                  CodecSampleRate::CODEC_SAMPLE_RATE_NONE) {
      BTIF_TRACE_DEBUG("[ACM]:%s: sample_rate = %d",
                       __func__, active_config.sample_rate);
      return static_cast<uint16_t>(active_config.sample_rate);
    } else {
      BTIF_TRACE_DEBUG("[ACM]:%s: default sample_rate = %d",
                       __func__, default_config.sample_rate);
      return static_cast<uint16_t>(default_config.sample_rate);
    }
  }
  BTIF_TRACE_DEBUG("[ACM]:%s: default sample_rate = %d",
                     __func__, default_config.sample_rate);
  return static_cast<uint16_t>(default_config.sample_rate);
}

uint8_t btif_acm_get_ch_mode(uint8_t direction) {
  DeviceType src_dev_type, sink_dev_type;
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  BTIF_TRACE_DEBUG("[ACM]:%s: direction = %d, active_context_type: %d",
                                 __func__, direction, active_context_type);
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }

      btif_bap_get_device_type(active_bda, &src_dev_type,
                               CodecDirection::CODEC_DIR_SRC);
      btif_bap_get_device_type(active_bda, &sink_dev_type,
                               CodecDirection::CODEC_DIR_SINK);

      BTIF_TRACE_DEBUG("%s: src_dev_type: %d, sink_dev_type: %d",
                       __func__, static_cast<DeviceType>(src_dev_type),
                       static_cast<DeviceType>(sink_dev_type));

      BTIF_TRACE_DEBUG("[ACM]:%s: Music channel_mode = %d",
                                 __func__, active_config.channel_mode);
      if (active_config.channel_mode !=
             CodecChannelMode::CODEC_CHANNEL_MODE_NONE) {
        if(current_active_profile_type == WMCP) {
          if(src_dev_type == DeviceType::HEADSET_STEREO) {
            return static_cast<uint8_t>
               (CodecChannelMode::CODEC_CHANNEL_MODE_JOINT_STEREO);
          } else if(src_dev_type == DeviceType::EARBUD_NO_CSIP) {
            return static_cast<uint8_t>
               (CodecChannelMode::CODEC_CHANNEL_MODE_MONO);
          } else if(src_dev_type != DeviceType::NONE) {
            BTIF_TRACE_DEBUG("[ACM]:%s: Voice channel mode : Stereo", __func__ );
            return static_cast<uint8_t>
               (CodecChannelMode::CODEC_CHANNEL_MODE_STEREO);
          }
        } else {
          if(sink_dev_type == DeviceType::HEADSET_STEREO) {
            return static_cast<uint8_t>
               (CodecChannelMode::CODEC_CHANNEL_MODE_JOINT_STEREO);
          } else if(sink_dev_type == DeviceType::EARBUD_NO_CSIP) {
            return static_cast<uint8_t>
               (CodecChannelMode::CODEC_CHANNEL_MODE_MONO);
          } else if(sink_dev_type != DeviceType::NONE) {
            BTIF_TRACE_DEBUG("[ACM]:%s: Voice channel mode : Stereo", __func__ );
            return static_cast<uint8_t>
               (CodecChannelMode::CODEC_CHANNEL_MODE_STEREO);
          }
        }
      }
    } else if(active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
      btif_bap_get_device_type(active_bda, &src_dev_type,
                               CodecDirection::CODEC_DIR_SRC);
      btif_bap_get_device_type(active_bda, &sink_dev_type,
                               CodecDirection::CODEC_DIR_SINK);

      BTIF_TRACE_DEBUG("%s: src_dev_type: %d, sink_dev_type: %d",
                       __func__, static_cast<DeviceType>(src_dev_type),
                       static_cast<DeviceType>(sink_dev_type));
      BTIF_TRACE_DEBUG("[ACM]:%s: Voice direction = %d", __func__, direction);

      if (direction == TX_RX_BOTH) {
        direction = RX_ONLY;
      }
      BTIF_TRACE_DEBUG("[ACM]:%s: Voice channel_mode = %d, modified direction: %d",
                                 __func__, active_config.channel_mode, direction);

      if (active_config.channel_mode !=
             CodecChannelMode::CODEC_CHANNEL_MODE_NONE) {
        if (direction == TX_ONLY) {
          if(sink_dev_type == DeviceType::HEADSET_STEREO) {
            return static_cast<uint8_t>
               (CodecChannelMode::CODEC_CHANNEL_MODE_JOINT_STEREO);
          } else if(sink_dev_type == DeviceType::EARBUD_NO_CSIP) {
            return static_cast<uint8_t>
               (CodecChannelMode::CODEC_CHANNEL_MODE_MONO);
          } else if(sink_dev_type != DeviceType::NONE) {
            BTIF_TRACE_DEBUG("[ACM]:%s: Muisc channel mode : Stereo", __func__ );
            return static_cast<uint8_t>
               (CodecChannelMode::CODEC_CHANNEL_MODE_STEREO);
          }
        } else if(direction == RX_ONLY) {
          if(src_dev_type == DeviceType::HEADSET_STEREO) {
            return static_cast<uint8_t>
               (CodecChannelMode::CODEC_CHANNEL_MODE_JOINT_STEREO);
          } else if(src_dev_type == DeviceType::EARBUD_NO_CSIP) {
            return static_cast<uint8_t>
               (CodecChannelMode::CODEC_CHANNEL_MODE_MONO);
          } else if(src_dev_type != DeviceType::NONE) {
            BTIF_TRACE_DEBUG("[ACM]:%s: Muisc channel mode : Stereo", __func__ );
            return static_cast<uint8_t>
               (CodecChannelMode::CODEC_CHANNEL_MODE_STEREO);
          }
        }
      }
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
    btif_bap_get_device_type(active_bda, &src_dev_type,
                             CodecDirection::CODEC_DIR_SRC);
    btif_bap_get_device_type(active_bda, &sink_dev_type,
                             CodecDirection::CODEC_DIR_SINK);
    BTIF_TRACE_DEBUG("[ACM]:%s: channel_mode = %d",
                               __func__, active_config.channel_mode);
    if (active_config.channel_mode !=
           CodecChannelMode::CODEC_CHANNEL_MODE_NONE) {
      if(current_active_profile_type == WMCP) {
        if(src_dev_type == DeviceType::HEADSET_STEREO) {
          return static_cast<uint8_t>
             (CodecChannelMode::CODEC_CHANNEL_MODE_JOINT_STEREO);
        } else if(src_dev_type == DeviceType::EARBUD_NO_CSIP) {
          return static_cast<uint8_t>
             (CodecChannelMode::CODEC_CHANNEL_MODE_MONO);
        } else if(src_dev_type != DeviceType::NONE) {
          BTIF_TRACE_DEBUG("[ACM]:%s: channel mode : Stereo", __func__ );
          return static_cast<uint8_t>
             (CodecChannelMode::CODEC_CHANNEL_MODE_STEREO);
        }
      } else {
        if(sink_dev_type == DeviceType::HEADSET_STEREO) {
          return static_cast<uint8_t>
             (CodecChannelMode::CODEC_CHANNEL_MODE_JOINT_STEREO);
        } else if(sink_dev_type == DeviceType::EARBUD_NO_CSIP) {
          return static_cast<uint8_t>
             (CodecChannelMode::CODEC_CHANNEL_MODE_MONO);
        } else if(sink_dev_type != DeviceType::NONE) {
          BTIF_TRACE_DEBUG("[ACM]:%s: channel mode : Stereo", __func__ );
          return static_cast<uint8_t>
             (CodecChannelMode::CODEC_CHANNEL_MODE_STEREO);
        }
      }
    }
  }
  BTIF_TRACE_DEBUG("[ACM]:%s: default ch_mode = %d",
                   __func__, default_config.channel_mode);
  return static_cast<uint8_t> (CodecChannelMode::CODEC_CHANNEL_MODE_STEREO);
}

uint32_t btif_acm_get_bitrate(uint8_t direction) {
  //based on bitrate set (100(80kbps), 120 (96kbps), 155 (128kbps))
  uint32_t bitrate = 0;
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  BTIF_TRACE_DEBUG("[ACM]:%s: direction = %d", __func__, direction);
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }
    } else if(active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  }
  uint16_t octets = static_cast<int>(GetOctsPerFrame(&active_config));
  BTIF_TRACE_DEBUG("[ACM]:%s: octets = %d",__func__, octets);

  switch (octets) {
    case 26:
      bitrate = 27800;
      break;

    case 30:
      if (btif_acm_get_sample_rate(direction) ==
          (static_cast<uint16_t>(CodecSampleRate::CODEC_SAMPLE_RATE_8000))) {
        bitrate = 24000;
      } else {
        bitrate = 32000;
      }
      break;

    case 40:
      bitrate = 32000;
      break;

    case 45:
      bitrate = 48000;
      break;

    case 60:
      if (btif_acm_get_sample_rate(direction) ==
          (static_cast<uint16_t>(CodecSampleRate::CODEC_SAMPLE_RATE_24000))) {
        bitrate = 48000;
      } else {
        bitrate = 64000;
      }
      break;

    case 80:
      bitrate = 64000;
      break;

    case 98:
    case 130:
      bitrate = 95550;
      break;

    case 75:
    case 100:
      bitrate = 80000;
      break;

    case 90:
    case 120:
      bitrate = 96000;
      break;

    case 117:
    case 155:
      bitrate = 124000;
      break;

    default:
      bitrate = 124000;
      break;
  }
  BTIF_TRACE_DEBUG(" [ACM]%s: bitrate = %d",__func__,bitrate);
  return bitrate;
}

uint32_t btif_acm_get_octets(uint32_t bit_rate, uint8_t direction) {
  uint32_t octets = 0;
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  BTIF_TRACE_DEBUG(" [ACM]:%s: direction = %d", __func__, direction);
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }
    } else if(active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  }
  octets = GetOctsPerFrame(&active_config);
  BTIF_TRACE_DEBUG(" [ACM]%s: octets = %d",__func__,octets);
  return octets;
}

uint16_t btif_acm_get_framelength(uint8_t direction) {
  uint16_t frame_duration;
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  BTIF_TRACE_DEBUG(" [ACM]:%s: direction = %d", __func__, direction);
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }
    } else if(active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  }
  switch (GetFrameDuration(&active_config)) {
    case 0:
      frame_duration = 7500; //7.5msec
      break;

    case 1:
      frame_duration = 10000; //10msec
      break;

    default:
      frame_duration = 10000;
  }
  BTIF_TRACE_DEBUG(": [ACM]:%s: frame duration = %d",
                                 __func__,frame_duration);
  return frame_duration;
}

uint16_t btif_acm_get_current_active_profile() {
  BTIF_TRACE_DEBUG(": [ACM]:%s: current_active_profile_type = %d,"
                             " active_context_type: %d", __func__,
                              current_active_profile_type, active_context_type);
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if (active_context_type == CONTEXT_TYPE_MUSIC) {
      return current_active_profile_type;
    } else if (active_context_type == CONTEXT_TYPE_VOICE) {
      return BAP_CALL;
    }
  }
  return current_active_profile_type;
}

uint8_t btif_acm_get_ch_count() {//update channel mode based on device connection
  uint8_t ch_mode = 0;
  if (current_active_config.channel_mode ==
      CodecChannelMode::CODEC_CHANNEL_MODE_STEREO) {
    ch_mode = 0x02;
  } else if (current_active_config.channel_mode ==
             CodecChannelMode::CODEC_CHANNEL_MODE_MONO) {
    ch_mode = 0x01;
  }
  BTIF_TRACE_DEBUG(": [ACM]:%s: channel count = %d",__func__,ch_mode);
  return ch_mode;
}

bool btif_acm_is_codec_type_lc3q(uint8_t direction) {
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  BTIF_TRACE_DEBUG(": [ACM]:%s: direction = %d", __func__, direction);
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }
    } else if(active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  }
  return GetVendorMetaDataLc3QPref(&active_config);
}

uint8_t btif_acm_lc3q_ver(uint8_t direction) {
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  BTIF_TRACE_DEBUG("[ACM]:%s: direction = %d", __func__, direction);
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }
    } else if(active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  }
  uint8_t encoder_ver = GetVendorMetaDataCodecEncoderVer(&active_config);
  BTIF_TRACE_DEBUG("[ACM]:%s: encoder_ver = %d", __func__, encoder_ver);
  uint8_t decoder_ver = GetVendorMetaDataCodecDecoderVer(&active_config);
  BTIF_TRACE_DEBUG("[ACM]:%s: decoder_ver = %d", __func__, decoder_ver);
  return GetVendorMetaDataCodecEncoderVer(&active_config);
}

std::vector<uint8_t> btif_acm_codec_metadata(uint8_t direction) {
  std::vector<uint8_t> codec_metadata (META_DATA_SIZE, 0x00);
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  BTIF_TRACE_DEBUG("[ACM]:%s: direction = %d", __func__, direction);

  if (direction == TX_RX_BOTH) {
    memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
  } else {
    memcpy(&active_config, &current_active_config, sizeof(active_config));
  }

  if (active_config.codec_type == CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE) {
    codec_metadata[0] = GetVendorMetaDataCodecEncoderVer(&active_config);
    codec_metadata[1] = GetVendorMetaDataCodecDecoderVer(&active_config);
    codec_metadata[2] = GetCapaMinSupFrameDur(&active_config);
    codec_metadata[3] = GetCapaFeatureMap(&active_config);
  } else { // default LC3
    if (btif_acm_is_codec_type_lc3q(direction)) {
      codec_metadata[0] = GetVendorMetaDataCodecEncoderVer(&active_config);
      codec_metadata[1] = GetVendorMetaDataCodecDecoderVer(&active_config);
    }
  }

  for (int i = 0; i < META_DATA_SIZE; i++)
    BTIF_TRACE_DEBUG("[ACM]:%s: codec_metadata[%d] = %d", __func__,i, codec_metadata[i]);

  return codec_metadata;
}

uint8_t btif_acm_codec_encoder_version(uint8_t direction) {
  BTIF_TRACE_DEBUG("[ACM]:%s: direction = %d", __func__, direction);
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  uint8_t encoder_version;
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }
    } else if(active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  }
  encoder_version = GetVendorMetaDataCodecEncoderVer(&active_config);
  BTIF_TRACE_DEBUG("[ACM]:%s: encoder_version %d", __func__, encoder_version);
  return encoder_version;
}

uint8_t btif_acm_codec_decoder_version(uint8_t direction) {
  BTIF_TRACE_DEBUG("[ACM]:%s: direction = %d", __func__, direction);
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  uint8_t decoder_version;
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }
    } else if(active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  }
  decoder_version = GetVendorMetaDataCodecDecoderVer(&active_config);
  BTIF_TRACE_DEBUG("[ACM]:%s: decoder_version %d", __func__, decoder_version);
  return decoder_version;
}

uint8_t btif_acm_get_min_sup_frame_dur(uint8_t direction) {
  BTIF_TRACE_DEBUG("[ACM]:%s: direction = %d", __func__, direction);
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }
    } else if(active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  }
  return GetCapaMinSupFrameDur(&active_config);
}

uint8_t btif_acm_get_feature_map(uint8_t direction) {
  BTIF_TRACE_DEBUG("[ACM]:%s: direction = %d", __func__, direction);
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }
    } else if(active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  }
  BTIF_TRACE_DEBUG("[ACM]:%s: GetCapaFeatureMap = %d", __func__, GetCapaFeatureMap(&active_config));
  return GetCapaFeatureMap(&active_config);
}

uint8_t btif_acm_lc3_blocks_per_sdu(uint8_t direction) {
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  BTIF_TRACE_DEBUG("[ACM]:%s: direction = %d", __func__, direction);
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }
    } else if(active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  }

  return GetLc3BlocksPerSdu(&active_config);
}

uint8_t btif_acm_get_device_config_type(const RawAddress& bd_addr,
                                        int direction) {
  DeviceType cache_device_type;
  int dev_config_type = 0;

  btif_bap_get_device_type(bd_addr, &cache_device_type,
                           static_cast<CodecDirection>(direction));
  BTIF_TRACE_DEBUG("%s: cache_dev_type: %d, for direction: %d",
                         __func__, cache_device_type, direction);

  if (cache_device_type == DeviceType::EARBUD ||
      cache_device_type == DeviceType::EARBUD_NO_CSIP) {
    dev_config_type = EB_CONFIG;
  } else if (cache_device_type == DeviceType::HEADSET_SPLIT_STEREO) {
    dev_config_type = STEREO_HS_CONFIG_1;
  } else if (cache_device_type == DeviceType::HEADSET_STEREO) {
    dev_config_type = STEREO_HS_CONFIG_2;
  }

  BTIF_TRACE_DEBUG("%s: dev_config_type: %d",  __func__, dev_config_type);
  return dev_config_type;
}

uint8_t btif_acm_update_lc3_blocks_per_sdu(const RawAddress& bd_addr,
                                           uint8_t profile_type,
                                           uint16_t context_type, int dir,
                                           uint8_t peer_max_sup_lc3_frames,
                                           int config_type,
                                           CodecConfig* config_) {
  //Default value is 1, for all UCs
  uint8_t res_num_of_lc3_blocks = 1;
  BTIF_TRACE_DEBUG("%s: profile_type: %d, config_type: %d"
                   ", peer_max_sup_lc3_frames: %d, channel_mode: %d",
                   __func__, profile_type, config_type,
                   peer_max_sup_lc3_frames,
                   static_cast<uint16_t>(config_->channel_mode));

  if (profile_type == GCP ||
      profile_type == GCP_RX || profile_type == GCP_TX ||
      (profile_type == BAP && context_type == VOICE_CONTEXT)) {
    std::vector<QoSConfig> qos_config_;
    uint32_t sduInterval = 0;
    uint32_t frame_duration;
    if (profile_type == GCP ||
        profile_type == GCP_RX || profile_type == GCP_TX) {
      qos_config_ = get_qos_params_for_codec(profile_type,
                                    MEDIA_LL_CONTEXT,
                                    config_->sample_rate,
                                    GetFrameDuration(config_),
                                    GetOctsPerFrame(config_),
                                    CodecIndex::CODEC_INDEX_SOURCE_LC3,
                                    GetVendorMetaDataCodecEncoderVer(config_),
                                    bd_addr);
    } else if (profile_type == BAP && context_type == VOICE_CONTEXT) {
      qos_config_ = get_qos_params_for_codec(profile_type,
                                    VOICE_CONTEXT,
                                    config_->sample_rate,
                                    GetFrameDuration(config_),
                                    GetOctsPerFrame(config_),
                                    CodecIndex::CODEC_INDEX_SOURCE_LC3,
                                    GetVendorMetaDataCodecEncoderVer(config_),
                                    bd_addr);
    }

    for (uint8_t j = 0; j < (uint8_t)qos_config_.size(); j++) {
      if (qos_config_[j].mandatory == 0) {
        sduInterval = qos_config_[j].sdu_int_micro_secs;
      }
    }

    if (GetFrameDuration(config_) == 0) {
      frame_duration = FRAME_DUR_7_5_MSEC;
    } else {
      frame_duration = FRAME_DUR_10_MSEC;
    }

    //compute remote supported number of lc3 blocks
    uint8_t peer_compuated_sup_num_of_lc3_blocks =
         peer_max_sup_lc3_frames/(static_cast<uint8_t>(config_->channel_mode));

    //compute local(from xml) supported number of lc3 blocks
    uint8_t local_compuated_sup_num_of_lc3_blocks = sduInterval/frame_duration;

    res_num_of_lc3_blocks =
          std::min(peer_compuated_sup_num_of_lc3_blocks,
                                   local_compuated_sup_num_of_lc3_blocks);

    BTIF_TRACE_DEBUG("%s: frame_duration=%d, sduInterval=%d,"
                     " peer_compuated_sup_num_of_lc3_blocks: %d, "
                     "local_compuated_sup_num_of_lc3_blocks: %d,"
                     " res_num_of_lc3_blocks: %d",__func__, frame_duration,
                     sduInterval, peer_compuated_sup_num_of_lc3_blocks,
                     local_compuated_sup_num_of_lc3_blocks, res_num_of_lc3_blocks);

    return res_num_of_lc3_blocks;
  } else {
    return res_num_of_lc3_blocks;
  }
}

uint16_t btif_acm_get_pref_profile_type(const RawAddress& peer_address) {
  LOG(INFO) << __PRETTY_FUNCTION__ << ": peer: " << peer_address;

  int id = INVALID_SET_ID;
  BtifAcmPeer* peer = nullptr;
  if (peer_address.address[0] == 0x9E && peer_address.address[1] == 0x8B &&
          peer_address.address[2] == 0x00) {
    id = peer_address.address[5];
  }

  if (id < INVALID_SET_ID) {
    tBTA_CSIP_CSET cset_info;
    memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
    cset_info = BTA_CsipGetCoordinatedSet(id);
    std::vector<RawAddress>::iterator itr;
    BTIF_TRACE_DEBUG("%s: size of set members %d",
                     __func__, (cset_info.set_members).size());
    if ((cset_info.set_members).size() > 0) {
      for (itr =(cset_info.set_members).begin();
              itr != (cset_info.set_members).end(); itr++) {
        peer = btif_acm_initiator.FindPeer(*itr);
        if (peer != nullptr) {
          LOG(INFO) << __PRETTY_FUNCTION__
                << ": peer->GetPrefProfileType(): "
                << peer->GetPrefProfileType();
          return peer->GetPrefProfileType();
        }
      }
    }
  } else {
    peer = btif_acm_initiator.FindPeer(peer_address);
    if (peer != nullptr) {
      LOG(INFO) << __PRETTY_FUNCTION__
                << ": peer is not null, peer->GetPrefProfileType(): "
                << peer->GetPrefProfileType();
      return peer->GetPrefProfileType();
    }
  }
  return 0;
}

uint32_t btif_acm_get_audio_location(uint32_t stream_id, uint8_t direction) {
  std::vector<uint32_t> locations = {AUDIO_LOC_LEFT, AUDIO_LOC_RIGHT};
  int id = INVALID_SET_ID;
  BTIF_TRACE_DEBUG("[ACM]:%s: stream_id = %d, direction = %d", __func__, stream_id, direction);
  if (active_bda.address[0] == 0x9E && active_bda.address[1] == 0x8B &&
          active_bda.address[2] == 0x00) {
    id = active_bda.address[5];
  }
  if (id < INVALID_SET_ID) {
    tBTA_CSIP_CSET cset_info;
    memset(&cset_info, 0, sizeof(tBTA_CSIP_CSET));
    cset_info = BTA_CsipGetCoordinatedSet(id);
    std::vector<RawAddress>::iterator itr;
    BTIF_TRACE_DEBUG("%s: size of set members %d",
                     __func__, (cset_info.set_members).size());
    if ((cset_info.set_members).size() > 0) {
      for (itr =(cset_info.set_members).begin();
              itr != (cset_info.set_members).end(); itr++) {
        BtifAcmPeer* p = btif_acm_initiator.FindPeer(*itr);
        if (p != nullptr) {
          int audio_location = 0;
          if (direction == RX_ONLY) {
            audio_location = p->get_peer_audio_src_location() &
                        (AUDIO_LOC_LEFT | AUDIO_LOC_RIGHT);
          } else if (direction == TX_ONLY) {
            audio_location = p->get_peer_audio_sink_location() &
                        (AUDIO_LOC_LEFT | AUDIO_LOC_RIGHT);
          }
          if ((audio_location == AUDIO_LOC_LEFT && p->CisId()%2 == 0) ||
              (audio_location == AUDIO_LOC_RIGHT && p->CisId()%2 == 1)) {
            locations = {AUDIO_LOC_LEFT, AUDIO_LOC_RIGHT};
            break;
          } else if ((audio_location == AUDIO_LOC_RIGHT && p->CisId()%2 == 0) ||
              (audio_location == AUDIO_LOC_LEFT && p->CisId()%2 == 1)) {
            locations = {AUDIO_LOC_RIGHT, AUDIO_LOC_LEFT};
            break;
          }
        }
      }
    }
  }
  int idx = stream_id % 2;
  BTIF_TRACE_DEBUG("%s: audio location: %d",__func__, locations[idx]);
  return locations[idx];
}

uint8_t btif_acm_get_codec_type(uint8_t direction) {
  BTIF_TRACE_DEBUG("[ACM]:%s: direction = %d", __func__, direction);
  CodecConfig active_config;
  memset(&active_config, 0, sizeof(active_config));
  if(btif_ahim_is_aosp_aidl_hal_enabled()) {
    if(active_context_type == CONTEXT_TYPE_MUSIC) {
      if (direction == TX_RX_BOTH) {
        memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
      } else {
        memcpy(&active_config, &current_active_config, sizeof(active_config));
      }
    } else if(active_context_type == CONTEXT_TYPE_VOICE) {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  } else {
    if (direction == TX_RX_BOTH) {
      memcpy(&active_config, &current_active_gaming_rx_config, sizeof(active_config));
    } else {
      memcpy(&active_config, &current_active_config, sizeof(active_config));
    }
  }
  BTIF_TRACE_DEBUG("[ACM]:%s: GetCodecType returned = %d", __func__,
                   static_cast<uint8_t> (GetCodecType(&active_config)));
  return static_cast<uint8_t> (GetCodecType(&active_config));
}

uint16_t btif_acm_bap_to_acm_context(uint16_t bap_context) {
  switch (bap_context) {
    case CONTENT_TYPE_MEDIA:
    case CONTENT_TYPE_LIVE:
    case CONTENT_TYPE_GAME:
      return CONTEXT_TYPE_MUSIC;

    case CONTENT_TYPE_CONVERSATIONAL:
      return CONTEXT_TYPE_VOICE;

    default:
      BTIF_TRACE_DEBUG("%s: Unknown bap context",__func__);
      return CONTEXT_TYPE_UNKNOWN;
  }
}

uint16_t btif_acm_profile_to_context(uint16_t profile) {
  BTIF_TRACE_DEBUG("%s: profile: %d",__func__, profile);

  switch (profile) {
    case BAP:
    case GCP:
    case WMCP:
    case TMAP:
    case GCP_RX:
      return CONTEXT_TYPE_MUSIC;

    case BAP_CALL:
      return CONTEXT_TYPE_VOICE;

    default:
      BTIF_TRACE_DEBUG("%s: Unknown profile context",__func__);
      return CONTEXT_TYPE_UNKNOWN;
  }
}

static void btif_debug_acm_peer_dump(int fd, const BtifAcmPeer& peer) {
  std::string state_str;
  int state = peer.StateMachine().StateId();
  switch (state) {
    case BtifAcmStateMachine::kStateIdle:
      state_str = "Idle";
      break;

    case BtifAcmStateMachine::kStateOpening:
      state_str = "Opening";
      break;

    case BtifAcmStateMachine::kStateOpened:
      state_str = "Opened";
      break;

    case BtifAcmStateMachine::kStateStarted:
      state_str = "Started";
      break;

    case BtifAcmStateMachine::kStateReconfiguring:
      state_str = "Reconfiguring";
      break;

    case BtifAcmStateMachine::kStateClosing:
      state_str = "Closing";
      break;

    default:
      state_str = "Unknown(" + std::to_string(state) + ")";
      break;
  }

  dprintf(fd, "  Peer: %s\n", peer.PeerAddress().ToString().c_str());
  dprintf(fd, "    Connected: %s\n", peer.IsConnected() ? "true" : "false");
  dprintf(fd, "    Streaming: %s\n", peer.IsStreaming() ? "true" : "false");
  dprintf(fd, "    State Machine: %s\n", state_str.c_str());
  dprintf(fd, "    Flags: %s\n", peer.FlagsToString().c_str());

}

bool compare_codec_config_(CodecConfig &first, CodecConfig &second) {
    if (first.codec_type != second.codec_type) {
      BTIF_TRACE_DEBUG("[ACM] Codec type mismatch %s",__func__);
      return true;
    } else if (first.sample_rate != second.sample_rate) {
      BTIF_TRACE_DEBUG("[ACM] Sample rate mismatch %s",__func__);
      return true;
    } else if (first.bits_per_sample != second.bits_per_sample) {
      BTIF_TRACE_DEBUG("[ACM] Bits per sample mismatch %s",__func__);
      return true;
    } else if (first.channel_mode != second.channel_mode) {
      BTIF_TRACE_DEBUG("[ACM] Channel mode mismatch %s",__func__);
      return true;
    } else {
      uint8_t frame_first = GetFrameDuration(&first);
      uint8_t frame_second = GetFrameDuration(&second);
      if (frame_first != frame_second) {
        BTIF_TRACE_DEBUG("[ACM] frame duration mismatch %s",__func__);
        return true;
      }
      uint8_t lc3blockspersdu_first = GetLc3BlocksPerSdu(&first);
      uint8_t lc3blockspersdu_second = GetLc3BlocksPerSdu(&second);
      if (lc3blockspersdu_first != lc3blockspersdu_second) {
        BTIF_TRACE_DEBUG("[ACM] LC3blocks per SDU mismatch %s",__func__);
        return true;
      }
      uint16_t octets_first = GetOctsPerFrame(&first);
      uint16_t octets_second = GetOctsPerFrame(&second);
      if (octets_first != octets_second) {
        BTIF_TRACE_DEBUG("[ACM] LC3 octets mismatch %s",__func__);
        return true;
      }
      return false;
    }
}

void print_codec_parameters(CodecConfig config) {
  uint8_t frame = GetFrameDuration(&config);
  uint8_t lc3blockspersdu = GetLc3BlocksPerSdu(&config);
  uint16_t octets = GetOctsPerFrame(&config);
  bool vendormetadatalc3qpref = GetCapaVendorMetaDataLc3QPref(&config);
  uint8_t vendormetadataencoderver = GetCapaVendorMetaDataCodecEncoderVer(&config);
  uint8_t vendormetadatadecoderver = GetCapaVendorMetaDataCodecDecoderVer(&config);
  LOG_DEBUG(
    LOG_TAG,
    "codec_type=%d codec_priority=%d "
    "sample_rate=%d bits_per_sample=0x%x "
    "channel_mode=0x%x",
    config.codec_type, config.codec_priority,
    config.sample_rate, config.bits_per_sample,
    config.channel_mode);
  LOG_DEBUG(
    LOG_TAG,
    "frame_duration=%d, lc3_blocks_per_SDU=%d,"
    " octets_per_frame=%d, vendormetadatalc3qpref=%d,"
    " vendormetadataencoderver=%d, vendormetadatadecoderver=%d",
    frame, lc3blockspersdu, octets,
    vendormetadatalc3qpref, vendormetadataencoderver,
    vendormetadatadecoderver);
}

void print_qos_parameters(QosConfig qos) {
    LOG_DEBUG(
    LOG_TAG,
    "CIG --> cig_id=%d cis_count=%d "
    "packing=%d framing=%d "
    "max_tport_latency_m_to_s=%d "
    "max_tport_latency_s_to_m=%d "
    "sdu_interval_m_to_s[0] = %x "
    "sdu_interval_m_to_s[1] = %x "
    "sdu_interval_m_to_s[2] = %x ",
    qos.cig_config.cig_id, qos.cig_config.cis_count,
    qos.cig_config.packing, qos.cig_config.framing,
    qos.cig_config.max_tport_latency_m_to_s,
    qos.cig_config.max_tport_latency_s_to_m,
    qos.cig_config.sdu_interval_m_to_s[0],
    qos.cig_config.sdu_interval_m_to_s[1],
    qos.cig_config.sdu_interval_m_to_s[2]);
    for (auto it = qos.cis_configs.begin(); it != qos.cis_configs.end(); ++it) {
      LOG_DEBUG(
      LOG_TAG,
      "CIS --> cis_id = %d max_sdu_m_to_s = %d "
      "max_sdu_s_to_m=%d "
      "phy_m_to_s = %d "
      "phy_s_to_m = %d "
      "rtn_m_to_s = %d "
      "rtn_s_to_m = %d",
      it->cis_id, it->max_sdu_m_to_s,
      it->max_sdu_s_to_m,
      it->phy_m_to_s, it->phy_s_to_m,
      it->rtn_m_to_s, it->rtn_s_to_m);
    }
    for (auto it = qos.ascs_configs.begin(); it != qos.ascs_configs.end(); ++it) {
      LOG_DEBUG(
        LOG_TAG,
        "ASCS --> cig_id = %d cis_id = %d "
        "target_latency=%d "
        "bi_directional = %d "
        "presentation_delay[0] = %x "
        "presentation_delay[1] = %x "
        "presentation_delay[2] = %x ",
        it->cig_id,
        it->cis_id,
        it->target_latency,
        it->bi_directional,
        it->presentation_delay[0],
        it->presentation_delay[1],
        it->presentation_delay[2]);
    }
}

static void btif_debug_acm_initiator_dump(int fd) {
  bool enabled = btif_acm_initiator.Enabled();

  dprintf(fd, "\nA2DP Source State: %s\n", (enabled) ? "Enabled" : "Disabled");
  if (!enabled) return;
  //dprintf(fd, "  Active peer: %s\n",
    //      btif_acm_initiator.ActivePeer().ToString().c_str());
  for (auto it : btif_acm_initiator.Peers()) {
    const BtifAcmPeer* peer = it.second;
    btif_debug_acm_peer_dump(fd, *peer);
  }
}

void btif_debug_acm_dump(int fd) {
  btif_debug_acm_initiator_dump(fd);
}
