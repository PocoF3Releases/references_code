/*
 * Copyright (c) 2022 Qualcomm Technologies, Inc.
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

#define LOG_TAG "bt_bta_dm_adv_audio"

#include <base/bind.h>
#include <base/callback.h>
#include <base/logging.h>
#include <string.h>

#include "bt_common.h"
#include "bt_target.h"
#include "bt_types.h"
#include "bta_api.h"
#include "bta_dm_api.h"
#include "bta_dm_co.h"
#include "bta/dm/bta_dm_int.h"
#include "bta_csip_api.h"
#include "bta_sys.h"
#include "btif/include/btif_storage.h"
#include "btm_api.h"
#include "btm_int.h"
#include "btu.h"
#include "gap_api.h" /* For GAP_BleReadPeerPrefConnParams */
#include "l2c_api.h"
#include "osi/include/log.h"
#include "osi/include/osi.h"
#include "sdp_api.h"
#include "bta_sdp_api.h"
#include "stack/gatt/connection_manager.h"
#include "stack/include/gatt_api.h"
#include "utl.h"
#include "device/include/interop_config.h"
#include "device/include/profile_config.h"
#include "device/include/interop.h"
#include "stack/sdp/sdpint.h"
#include <inttypes.h>
#include "btif/include/btif_config.h"
#include "device/include/device_iot_config.h"
#include <btcommon_interface_defs.h>
#include <controller.h>
#include "bta_gatt_queue.h"
#include "bta_dm_adv_audio.h"
#include "btif/include/btif_dm_adv_audio.h"
#include "btif/include/btif_bap_config.h"
#include <hardware/bt_pacs_client.h>
#include <hardware/bt_bap_uclient.h>
#include "osi/include/properties.h"

#if (GAP_INCLUDED == TRUE)
#include "gap_api.h"
#endif

using bluetooth::Uuid;
using bluetooth::bap::pacs::CodecDirection;
using bluetooth::bap::pacs::CodecCapChnlCount;
using bluetooth::bap::ucast::codec_type_t;
using bluetooth::bap::ucast::AUDIO_LOC_RIGHT;
using bluetooth::bap::ucast::AUDIO_LOC_LEFT;

#define ADV_AUDIO_VOICE_ROLE_BIT 2
#define ADV_AUDIO_MEDIA_ROLE_BIT 8
#define CONN_LESS_MEDIA_SINK_ROLE_BIT 32
#define CONN_LESS_ASSIST_ROLE_BIT 64
#define CONN_LESS_DELEGATE_ROLE_BIT 128
#define PACS_CT_SUPPORT_VALUE 2
#define PACS_UMR_SUPPORT_VALUE 4
#define PACS_CONVERSATIONAL_ROLE_VALUE 0x20002
#define PACS_MEDIA_ROLE_VALUE 0x40004
#define PACS_GAME_AUDIO_CONTEXT_SUPPORT 8

#define BTA_DM_ADV_AUDIO_GATT_CLOSE_DELAY_TOUT 1000

Uuid UUID_SERVCLASS_WMCP =
      Uuid::FromString("2587db3c-ce70-4fc9-935f-777ab4188fd7");
Uuid UUID_SERVCLASS_GMAP =
      Uuid::FromString("12994B7E-6d47-4215-8C9E-AAE9A1095BA3");

#define ASE_COUNT_ONE                   0x01
#define LTV_TYPE_CHNL_COUNTS            0x03

#define ASCS_SINK_ASE_UUID              0x2BC4
#define PACS_SINK_LOC_UUID              0x2BCA
#define ASCS_SRC_ASE_UUID               0x2BC5
#define PACS_SRC_LOC_UUID               0x2BCC
#define PACS_SINK_PAC_UUID              0x2BC9
#define PACS_SRC_PAC_UUID               0x2BCB

std::vector<uint8_t> codec_caps;
std::vector<bluetooth::Uuid> uuid_srv_disc_search;
tBTA_LE_AUDIO_DEV_CB bta_le_audio_dev_cb;
tBTA_LEA_PAIRING_DB bta_lea_pairing_cb;
extern void bta_dm_proc_open_evt(tBTA_GATTC_OPEN* p_data);
bool is_adv_audio_unicast_supported(RawAddress rem_bda, int conn_id);

#define BTA_DM_GATT_CONN_TOUT_TIMER_MS 2000

typedef struct {
  RawAddress bd_addr;
  alarm_t *gatt_le_conn_tout_alarm;
} bta_dm_le_pending_conn;

static bta_dm_le_pending_conn dm_le_conn_alarm;

void bta_dm_adv_audio_init() {
  APPL_TRACE_DEBUG("%s ", __func__);
  dm_le_conn_alarm.gatt_le_conn_tout_alarm
    = alarm_new("gatt_le_conn_tout_alarm");
}

void bta_dm_adv_audio_deinit() {
  APPL_TRACE_DEBUG("%s ", __func__);
  alarm_free(dm_le_conn_alarm.gatt_le_conn_tout_alarm);
}

void bta_le_audio_service_search_failed(void* data) {
  tBTA_DM_SEARCH result;
  RawAddress* p_bd_addr = (RawAddress*)data;
  APPL_TRACE_DEBUG("%s %s", __func__, p_bd_addr->ToString().c_str());

  result.adv_audio_disc_cmpl.num_uuids = 0;
  result.adv_audio_disc_cmpl.bd_addr = *p_bd_addr;
  APPL_TRACE_DEBUG("%s Sending Call back with 0 ADV Audio uuids's", __func__);
  bta_dm_search_cb.p_search_cback(BTA_DM_LE_AUDIO_SEARCH_CMPL_EVT, &result);
  btif_le_audio_conn_failed(*p_bd_addr);
}

bool bta_dm_le_set_alarm(RawAddress& bd_addr) {
  dm_le_conn_alarm.bd_addr = bd_addr;
  if (!dm_le_conn_alarm.gatt_le_conn_tout_alarm) {
    APPL_TRACE_ERROR("%s:unable to allocate le_conn_tout",__func__);
    return false;
  }
  alarm_set(dm_le_conn_alarm.gatt_le_conn_tout_alarm, BTA_DM_GATT_CONN_TOUT_TIMER_MS,
            bta_le_audio_service_search_failed, &dm_le_conn_alarm.bd_addr);
  APPL_TRACE_DEBUG("%s: bta_dm le_conn_timer started ", __func__);
  return true;
}

/***************************************************************************
 *
 * Function         bta_get_lea_ctrl_cb
 *
 * Description      Gets the control block of LE audio device
 *
 * Parameters:      tBTA_LE_AUDIO_DEV_INFO*
 *
 ****************************************************************************/

tBTA_LE_AUDIO_DEV_INFO* bta_get_lea_ctrl_cb(RawAddress peer_addr) {
  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb = NULL;
  p_lea_cb = &bta_le_audio_dev_cb.bta_lea_dev_info[0];

  for (int i = 0; i < MAX_LEA_DEVICES ; i++) {
      if (p_lea_cb[i].in_use &&
        (p_lea_cb[i].peer_address == peer_addr)) {
        APPL_TRACE_DEBUG(" %s Control block Found for addr %s",
          __func__, peer_addr.ToString().c_str());
        return &p_lea_cb[i];
      }
  }
  APPL_TRACE_DEBUG(" %s Control block Not Found for addr %s",
          __func__, peer_addr.ToString().c_str());
  return NULL;
}

void update_device_type(RawAddress p_bd_addr) {
  uint16_t sink_aud_loc = 0;
  uint16_t src_aud_loc = 0;
  uint16_t sink_num_ase = 0;
  uint16_t src_num_ase = 0;
  uint16_t sink_chan_cnt = 0;
  uint16_t src_chan_cnt = 0;
  DeviceType sink_device_type = DeviceType::NONE;
  DeviceType src_device_type = DeviceType::NONE;
  bool csip_supported = false;

  btif_bap_get_params(p_bd_addr, &sink_aud_loc, &src_aud_loc,
                      &sink_num_ase, &src_num_ase, &sink_chan_cnt,
                      &src_chan_cnt);

  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb = bta_get_lea_ctrl_cb(p_bd_addr);
  if (p_lea_cb == NULL) {
    APPL_TRACE_ERROR(" %s Control block didnt find for peer address %s", __func__,
        p_bd_addr.ToString().c_str());
    return;
  }
  csip_supported = p_lea_cb->is_csip_support;

  APPL_TRACE_DEBUG("%s: CSIP supported: %d", __func__, csip_supported);
  APPL_TRACE_DEBUG("%s: Sink Audio Location: %d", __func__, sink_aud_loc);
  APPL_TRACE_DEBUG("%s: Source Audio Location: %d", __func__, src_aud_loc);
  APPL_TRACE_DEBUG("%s: Num of Sink ASEs: %d", __func__, sink_num_ase);
  APPL_TRACE_DEBUG("%s: Num of Source ASEs: %d", __func__, src_num_ase);
  APPL_TRACE_DEBUG("%s: Sink Channel Count: %d", __func__, sink_chan_cnt);
  APPL_TRACE_DEBUG("%s: Source Channel Count: %d", __func__, src_chan_cnt);

  if (csip_supported) {
      sink_device_type = DeviceType::EARBUD;
      src_device_type = DeviceType::EARBUD;
  } else {
      if (sink_chan_cnt &
          static_cast<uint16_t>(CodecCapChnlCount::CHNL_COUNT_TWO)) {
          sink_device_type = DeviceType::HEADSET_STEREO;
      } else if (sink_chan_cnt &
                 static_cast<uint16_t>(CodecCapChnlCount::CHNL_COUNT_ONE)) {
          if ((sink_aud_loc == AUDIO_LOC_LEFT) ||
               sink_aud_loc == AUDIO_LOC_RIGHT) {
              sink_device_type = DeviceType::EARBUD_NO_CSIP;
          } else {
              if (sink_num_ase == ASE_COUNT_ONE) {
                  sink_device_type = DeviceType::EARBUD_NO_CSIP;
              } else if ((sink_num_ase % 2) == 0){
                  sink_device_type = DeviceType::HEADSET_SPLIT_STEREO;
              }
          }
      }
      if (src_chan_cnt &
          static_cast<uint16_t>(CodecCapChnlCount::CHNL_COUNT_TWO)) {
          src_device_type = DeviceType::HEADSET_STEREO;
      } else if (src_chan_cnt &
                 static_cast<uint16_t>(CodecCapChnlCount::CHNL_COUNT_ONE)) {
          if ((src_aud_loc == AUDIO_LOC_LEFT) ||
               src_aud_loc == AUDIO_LOC_RIGHT) {
              src_device_type = DeviceType::EARBUD_NO_CSIP;
          } else {
              if (src_num_ase == ASE_COUNT_ONE) {
                  src_device_type = DeviceType::EARBUD_NO_CSIP;
              } else if ((src_num_ase % 2) == 0){
                  src_device_type = DeviceType::HEADSET_SPLIT_STEREO;
              }
          }
      }
  }
  APPL_TRACE_DEBUG("%s: Sink device type: %d", __func__, sink_device_type);
  APPL_TRACE_DEBUG("%s: Source device type: %d", __func__, src_device_type);

  btif_bap_add_device_type(p_bd_addr, sink_device_type, src_device_type);
}

void read_pac_record(RawAddress peer_address, uint8_t* p,
                                        CodecDirection dir) {
  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb =
                 bta_get_lea_ctrl_cb(bta_le_audio_dev_cb.gatt_op_addr);
  if (p_lea_cb == NULL) {
    APPL_TRACE_ERROR(" %s Control block didnt find for peer address %s",
                      __func__, peer_address.ToString().c_str());
    return;
  }

  codec_type_t codec_id;
  uint8_t num_pac_recs;
  uint8_t codec_cap_len;
  STREAM_TO_UINT8(num_pac_recs, p);
  LOG(WARNING) << __func__ << ": num_pac_recs: " << num_pac_recs;

  if (num_pac_recs) {
    STREAM_TO_ARRAY(&codec_id, p, static_cast<int> (sizeof(codec_id)));
    STREAM_TO_UINT8(codec_cap_len, p);
    codec_caps.resize(codec_cap_len);
    STREAM_TO_ARRAY(codec_caps.data(), p, codec_cap_len);
    uint8_t *pp = codec_caps.data();

    while (codec_cap_len) {
      uint8_t ltv_len = *pp++;
      codec_cap_len--;
      uint8_t ltv_type = *pp++;
      if (ltv_type == LTV_TYPE_CHNL_COUNTS) {
        uint8_t chnl_allocation;
        STREAM_TO_UINT8(chnl_allocation, pp);
        if (dir == CodecDirection::CODEC_DIR_SINK) {
          LOG(WARNING) << __func__ << ": Sink Val: ";
          btif_bap_add_channel_count(peer_address, chnl_allocation,
                                     CodecDirection::CODEC_DIR_SINK);
          break;
        } else if (dir == CodecDirection::CODEC_DIR_SRC) {
          LOG(WARNING) << __func__ << ": Src Val: ";
          btif_bap_add_channel_count(peer_address, chnl_allocation,
                                     CodecDirection::CODEC_DIR_SRC);
          break;
        }
      } else {
        uint8_t rem_len = ltv_len - 1;
        while (rem_len--) { pp++;};
      }
    }
  } else {
    if (dir == CodecDirection::CODEC_DIR_SINK) {
      p_lea_cb->pacs_sink_pac_handle = 0;
    } else if (dir == CodecDirection::CODEC_DIR_SRC) {
      p_lea_cb->pacs_src_pac_handle = 0;
    }
  }
}

/* Callback received when remote device Coordinated Sets SIRK is read */
void bta_gap_gatt_read_cb(uint16_t conn_id, tGATT_STATUS status,
                  uint16_t handle, uint16_t len,
                  uint8_t* value, void* data) {

  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb =
    bta_get_lea_ctrl_cb(bta_le_audio_dev_cb.gatt_op_addr);
  uint32_t role = 0;
  uint32_t aud_loc = 0;
  uint8_t *p_val = value;

  STREAM_TO_ARRAY(&role, p_val, len);
  STREAM_TO_UINT32(aud_loc, p_val);

  if (p_lea_cb) {
    APPL_TRACE_DEBUG("%s: Addr: %s ", __func__,
      p_lea_cb->peer_address.ToString().c_str());
    if (status == GATT_SUCCESS) {
      if (p_lea_cb->t_role_handle == handle) {
        LOG(INFO) << __func__
                  << ": Role derived by T_ADV_AUDIO: " << +role;
        if (role != 0)  {
          if (role & ADV_AUDIO_VOICE_ROLE_BIT)
            p_lea_cb->uuids.push_back(Uuid::From16Bit
                (UUID_SERVCLASS_T_ADV_AUDIO_VOICE));
          if (role & ADV_AUDIO_MEDIA_ROLE_BIT)
            p_lea_cb->uuids.push_back(Uuid::From16Bit
                (UUID_SERVCLASS_T_ADV_AUDIO_MEDIA_SINK));
          if (role & CONN_LESS_MEDIA_SINK_ROLE_BIT)
            p_lea_cb->uuids.push_back(Uuid::From16Bit
                (UUID_SERVCLASS_T_ADV_AUDIO_CONN_LESS_MEDIA_SINK));
          if (role & CONN_LESS_ASSIST_ROLE_BIT)
            p_lea_cb->uuids.push_back(Uuid::From16Bit
                (UUID_SERVCLASS_T_ADV_AUDIO_ASSIST));
          if (role & CONN_LESS_DELEGATE_ROLE_BIT)
            p_lea_cb->uuids.push_back(Uuid::From16Bit
                (UUID_SERVCLASS_T_ADV_AUDIO_DELEGATE));
        }
        p_lea_cb->disc_progress--;
      } else if(handle == p_lea_cb->pacs_supp_context_handle) {
        LOG(INFO) << __func__
                  << ": Supported Audio Context derived by PACS: " << +role;
        if (role == 0) {
          LOG(INFO) << __func__ << ": Invalid Information ";
        } else {
          if (is_adv_audio_unicast_supported(bta_le_audio_dev_cb.gatt_op_addr, conn_id)) {
            LOG(INFO) << __func__ << ": ASCS Supported by the remote: ";
            if (role & PACS_CONVERSATIONAL_ROLE_VALUE) {
              p_lea_cb->uuids.push_back(Uuid::From16Bit(UUID_SERVCLASS_PACS_CT_SUPPORT));
            }

            if (role & PACS_MEDIA_ROLE_VALUE) {
              p_lea_cb->uuids.push_back(Uuid::From16Bit(UUID_SERVCLASS_PACS_UMR_SUPPORT));
            }

            //check for Game context support in Supported_Sink_Contexts
            //in PACS Supported_Audio_Contexts. If remote doesn't support
            //GMAP service and show Game contect support in PACS, then add it
            //to uuids list.
            if (role & PACS_GAME_AUDIO_CONTEXT_SUPPORT) {
              std::vector<bluetooth::Uuid>::iterator itr;
              itr = std::find(p_lea_cb->uuids.begin(), p_lea_cb->uuids.end(),
                                                UUID_SERVCLASS_GMAP);
              if (itr == p_lea_cb->uuids.end()) {
                LOG(INFO) << __func__ << ": GMAP uuid not found in added uuids, add it.";
                p_lea_cb->uuids.push_back(UUID_SERVCLASS_GMAP);
              } else {
                LOG(INFO) << __func__ << ": GMAP uuid already found in added uuids";
              }
            }

            bool is_src_supported_game_context_enabled = false;
            char src_game_context_value[PROPERTY_VALUE_MAX] = "false";
            property_get("persist.vendor.btstack.is_src_supported_game_context_enable",
                                                   src_game_context_value, "true");
            if (!strncmp("true", src_game_context_value, 4)) {
              is_src_supported_game_context_enabled = true;
            }
            LOG(INFO) << __func__ << ": is_src_supported_game_context_enable: "
                                  << is_src_supported_game_context_enabled;
            //check for Game context support in Supported_Source_Contexts
            //in PACS Supported_Audio_Contexts
            if (((role >> 16) & PACS_GAME_AUDIO_CONTEXT_SUPPORT) &&
                (is_src_supported_game_context_enabled)) {
              LOG(INFO) << __func__
                        << ": Supported_Source_Contexts has GAME context support.";
              p_lea_cb->uuids.push_back(
                 Uuid::From16Bit(UUID_SERVCLASS_PACS_SRC_SUPPORT_GAME_CONTEXT));
            }
          }
          //TODO LEA_DBG Call API which will be provided by BAP
        }
        p_lea_cb->disc_progress--;
      } else if (handle == p_lea_cb->pacs_src_loc_handle) {
        LOG(INFO) << __func__
                  << ": Src Audio Location derived by PACS " << +aud_loc;
        if (aud_loc == 0) {
          LOG(INFO) << __func__ << ": Invalid Information ";
        } else {
          btif_bap_add_audio_loc(p_lea_cb->peer_address,
                                 CodecDirection::CODEC_DIR_SRC, aud_loc);
        }
        p_lea_cb->disc_progress--;
      } else if (handle == p_lea_cb->pacs_sink_loc_handle) {
        LOG(INFO) << __func__
                  << ": Sink Audio Location derived by PACS: " << +aud_loc;
        if (aud_loc == 0) {
          LOG(INFO) << __func__ << ": Invalid Information ";
        } else {
          btif_bap_add_audio_loc(p_lea_cb->peer_address,
                                 CodecDirection::CODEC_DIR_SINK, aud_loc);
        }
        p_lea_cb->disc_progress--;
      } else if (handle == p_lea_cb->pacs_sink_pac_handle) {
        LOG(INFO) << __func__ << ": Sink Channel Count derived by PACS: ";
        read_pac_record(p_lea_cb->peer_address,
                        value, CodecDirection::CODEC_DIR_SINK);
        p_lea_cb->disc_progress--;
      } else if (handle == p_lea_cb->pacs_src_pac_handle) {
        LOG(INFO) << __func__ << ": Src Channel Count derived by PACS ";
        read_pac_record(p_lea_cb->peer_address,
                        value, CodecDirection::CODEC_DIR_SRC);
        p_lea_cb->disc_progress--;
      } else {
        LOG(INFO) << __func__ << ": Invalid Handle for LE AUDIO";
      }
    } else {
      p_lea_cb->disc_progress--;
      LOG(INFO) << __func__ << ": GATT READ FAILED" ;
    }

    LOG(INFO) << __func__
              << ": Discovery progress:" << p_lea_cb->disc_progress;
    if (p_lea_cb->disc_progress <= 0) {
      LOG(INFO) << __func__ << ": Discovery Complete";
      bta_dm_lea_disc_complete(p_lea_cb->peer_address);
      update_device_type(p_lea_cb->peer_address);
    }
  } else {
    LOG(INFO) << __func__ << ": INVALID CONTROL BLOCK" ;
  }
}

/*******************************************************************************
 *
 * Function         bta_get_adv_audio_role
 *
 * Description      This API gets role for LE Audio Device after all services
 *                  discovered
 *
 * Parameters:      none
 *
 ******************************************************************************/
void bta_get_adv_audio_role(RawAddress peer_address, uint16_t conn_id,
                                  tGATT_STATUS status) {
  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb = bta_get_lea_ctrl_cb(peer_address);

  bta_le_audio_dev_cb.gatt_op_addr = peer_address;

  if (p_lea_cb == NULL) {
    APPL_TRACE_ERROR("%s: Control block didnt find for peer address: %s",
                     __func__, peer_address.ToString().c_str());
    return;
  }

  // Fetch remote device gatt services from database
  const std::vector<gatt::Service>* services = BTA_GATTC_GetServices(conn_id);

  if (services) {
    APPL_TRACE_DEBUG("%s: SIZE: %d, addr: %s, conn_id: %d",
                      __func__, (*services).size(),
                      bta_le_audio_dev_cb.gatt_op_addr.ToString().c_str(),
      conn_id);

    // Search for CSIS service in the database
    for (const gatt::Service& service : *services) {
      LOG(INFO) << __func__ << ": SERVICES IN REMOTE DEVICE: "
                <<  service.uuid.ToString().c_str();
      if (is_le_audio_service(service.uuid)) {
        size_t len = service.uuid.GetShortestRepresentationSize();
        uint16_t uuid_val = 0;
        if (len == Uuid::kNumBytes16) {
          uuid_val = service.uuid.As16Bit();
        } else if(len == Uuid::kNumBytes128) {
          if (service.uuid == UUID_SERVCLASS_WMCP) {
            APPL_TRACE_DEBUG("%s: WMCP Service UUId found", __func__);
            std::vector<bluetooth::Uuid>::iterator itr;
            itr = std::find(p_lea_cb->uuids.begin(), p_lea_cb->uuids.end(),
                UUID_SERVCLASS_WMCP);
            if (itr == p_lea_cb->uuids.end()) {
              p_lea_cb->uuids.push_back(UUID_SERVCLASS_WMCP);
            }
          }

          if (service.uuid == UUID_SERVCLASS_GMAP) {
            APPL_TRACE_DEBUG("%s: GMAP Service UUId found", __func__);
            std::vector<bluetooth::Uuid>::iterator itr;
            itr = std::find(p_lea_cb->uuids.begin(), p_lea_cb->uuids.end(),
                                                UUID_SERVCLASS_GMAP);
            if (itr == p_lea_cb->uuids.end()) {
              p_lea_cb->uuids.push_back(UUID_SERVCLASS_GMAP);
            }
          }
        }

        switch (uuid_val) {
          case UUID_SERVCLASS_CSIS: {
            APPL_TRACE_DEBUG("%s: CSIS service found Uuid: %s ", __func__,
                              service.uuid.ToString().c_str());

            p_lea_cb->is_csip_support = true;
            bta_dm_csis_disc_complete(bta_dm_search_cb.peer_bdaddr, false);
            // Get Characteristic and CCCD handle
            for (const gatt::Characteristic& charac : service.characteristics) {
              Uuid lock_uuid = charac.uuid;
              if (lock_uuid.As16Bit() == UUID_SERVCLASS_CSIS_LOCK) {
                APPL_TRACE_DEBUG("%s: CSIS rank found Uuid: %s ", __func__,
                    lock_uuid.ToString().c_str());
                if (p_lea_cb != NULL) {
                  Uuid csip_lock_uuid = Uuid::FromString("6AD8");
                  std::vector<bluetooth::Uuid>::iterator itr;
                  itr = std::find(p_lea_cb->uuids.begin(), p_lea_cb->uuids.end(),
                    csip_lock_uuid);
                  if (itr == p_lea_cb->uuids.end()) {
                    p_lea_cb->uuids.push_back(csip_lock_uuid);
                  }
                } else {
                  APPL_TRACE_DEBUG(" %s No Control Block", __func__);
                }
              }
            }
          } break;

          case UUID_SERVCLASS_TMAS: {
              APPL_TRACE_DEBUG("%s: T_ADV_AUDIO service found Uuid: %s ",
                                __func__, service.uuid.ToString().c_str());
              std::vector<bluetooth::Uuid>::iterator itr;
              itr = std::find(p_lea_cb->uuids.begin(), p_lea_cb->uuids.end(),
                service.uuid);

              if (itr == p_lea_cb->uuids.end()) {
                p_lea_cb->uuids.push_back(service.uuid);
              }

              // Get Characteristic and CCCD handle
              for (const gatt::Characteristic& charac : service.characteristics) {
                Uuid role_uuid = charac.uuid;
                if (role_uuid.As16Bit() == UUID_SERVCLASS_TMAP_ROLE_CHAR) {
                  APPL_TRACE_DEBUG("%s: T_ADV_AUDIO_ROLE_CHAR service found Uuid: %s ",
                                   __func__, role_uuid.ToString().c_str());
                  if (p_lea_cb != NULL) {
                    p_lea_cb->is_t_audio_srvc_found = true;
                    p_lea_cb->disc_progress++;
                    p_lea_cb->t_role_handle = charac.value_handle;
                  } else {
                    APPL_TRACE_DEBUG("%s: No Control Block", __func__);
                  }
                }
              }

              if (p_lea_cb->t_role_handle) {
                APPL_TRACE_DEBUG("%s: t_role_handle: %d", __func__,
                  p_lea_cb->t_role_handle);
                BtaGattQueue::ReadCharacteristic(conn_id, p_lea_cb->t_role_handle,
                  bta_gap_gatt_read_cb, NULL);
              }
          } break;

          case UUID_SERVCLASS_HAS: {
              p_lea_cb->is_has_found = true;
              APPL_TRACE_DEBUG("%s: HAS service found Uuid: %s ", __func__,
                service.uuid.ToString().c_str());
              std::vector<bluetooth::Uuid>::iterator itr;
              itr = std::find(p_lea_cb->uuids.begin(), p_lea_cb->uuids.end(),
                service.uuid);
              if (itr == p_lea_cb->uuids.end()) {
                p_lea_cb->uuids.push_back(service.uuid);
              }

           } break;
          case UUID_SERVCLASS_PACS: {
             LOG(INFO) << __func__ << ": SERVCLASS_PACS";
              APPL_TRACE_DEBUG("%s: PACS service found Uuid: %s ", __func__,
                service.uuid.ToString().c_str());

              std::vector<bluetooth::Uuid>::iterator itr;
              itr = std::find(p_lea_cb->uuids.begin(), p_lea_cb->uuids.end(),
                service.uuid);
              if (itr == p_lea_cb->uuids.end()) {
                p_lea_cb->uuids.push_back(service.uuid);
              }

              // Get Characteristic and CCCD handle
              for (const gatt::Characteristic& charac : service.characteristics) {
                LOG(INFO) << __func__ << ": charac";
                uint16_t uuid_char = charac.uuid.As16Bit();

                switch (uuid_char) {
                  case UUID_SERVCLASS_SOURCE_CONTEXT: {
                     LOG(INFO) << __func__ << ": PACS Source context CHAR found Uuid: "
                                << charac.uuid.ToString().c_str();
                     if ((p_lea_cb != NULL) && (!p_lea_cb->pacs_supp_context_handle)) {
                       p_lea_cb->disc_progress++;
                       p_lea_cb->pacs_supp_context_handle = charac.value_handle;
                       BtaGattQueue::ReadCharacteristic(conn_id,
                          p_lea_cb->pacs_supp_context_handle,
                          bta_gap_gatt_read_cb, NULL);
                     } else {
                       APPL_TRACE_DEBUG("%s: No Control Block", __func__);
                     }
                  } break;

                  case PACS_SINK_PAC_UUID: {
                     LOG(INFO) << __func__ << ": PACS Sink Pac CHAR found Uuid: "
                               << charac.uuid.ToString().c_str();
                     if ((p_lea_cb != NULL) && (!p_lea_cb->pacs_sink_pac_handle)) {
                       p_lea_cb->disc_progress++;
                       p_lea_cb->pacs_sink_pac_handle = charac.value_handle;
                       BtaGattQueue::ReadCharacteristic(conn_id,
                         p_lea_cb->pacs_sink_pac_handle,
                         bta_gap_gatt_read_cb, NULL);
                     } else {
                       APPL_TRACE_DEBUG("%s: No Control Block", __func__);
                     }
                  } break;

                  case PACS_SRC_PAC_UUID: {
                     LOG(INFO) << __func__ << ": PACS Src Pac CHAR found Uuid: "
                               << charac.uuid.ToString().c_str();
                     if ((p_lea_cb != NULL) && (!p_lea_cb->pacs_src_pac_handle)) {
                       p_lea_cb->disc_progress++;
                       p_lea_cb->pacs_src_pac_handle = charac.value_handle;
                       BtaGattQueue::ReadCharacteristic(conn_id,
                         p_lea_cb->pacs_src_pac_handle,
                         bta_gap_gatt_read_cb, NULL);
                     } else {
                       APPL_TRACE_DEBUG("%s: No Control Block", __func__);
                     }
                  } break;

                  case PACS_SINK_LOC_UUID: {
                     LOG(INFO) << __func__ << ": PACS Sink Location CHAR found Uuid: "
                                << charac.uuid.ToString().c_str();
                     if ((p_lea_cb != NULL) && (!p_lea_cb->pacs_sink_loc_handle)) {
                       p_lea_cb->disc_progress++;
                       p_lea_cb->pacs_sink_loc_handle = charac.value_handle;
                       BtaGattQueue::ReadCharacteristic(conn_id,
                         p_lea_cb->pacs_sink_loc_handle,
                         bta_gap_gatt_read_cb, NULL);
                     } else {
                       APPL_TRACE_DEBUG("%s: No Control Block", __func__);
                     }
                  } break;

                  case PACS_SRC_LOC_UUID: {
                     LOG(INFO) << __func__ << ": PACS Source Location CHAR found Uuid: "
                                << charac.uuid.ToString().c_str();
                     if ((p_lea_cb != NULL) && (!p_lea_cb->pacs_src_loc_handle)) {
                       p_lea_cb->disc_progress++;
                       p_lea_cb->pacs_src_loc_handle = charac.value_handle;
                       BtaGattQueue::ReadCharacteristic(conn_id,
                          p_lea_cb->pacs_src_loc_handle,
                          bta_gap_gatt_read_cb, NULL);
                     } else {
                       APPL_TRACE_DEBUG("%s: No Control Block", __func__);
                     }
                  } break;

                  default:
                     LOG(ERROR) << __func__
                                << ": No Match found for: " << uuid_char;
                }
              }
          } break;

          case UUID_SERVCLASS_ASCS: {
            LOG(INFO) << __func__ << ": SERVICE_CLASS_ASCS found";
            uint8_t num_src_ase = 0;
            uint8_t num_sink_ase = 0;
            for (const gatt::Characteristic& charac : service.characteristics) {
              if (charac.uuid.As16Bit() == ASCS_SINK_ASE_UUID){
                LOG(INFO) << __func__ << ": ASCS Sink ASE CHAR found Uuid: "
                          << charac.uuid.ToString().c_str();
                num_sink_ase++;
              } else if (charac.uuid.As16Bit() == ASCS_SRC_ASE_UUID){
                LOG(INFO) << __func__ << ": ASCS Source ASE CHAR found Uuid: "
                          << charac.uuid.ToString().c_str();
                num_src_ase++;
              }
            }
            btif_bap_add_num_src_sink(peer_address, num_sink_ase, num_src_ase);
          } break;

          default:
            APPL_TRACE_DEBUG("%s: Not a LE AUDIO SERVICE-- IGNORE: %s ",
                             __func__, service.uuid.ToString().c_str());
        }
      }
    }
  }

  if (p_lea_cb->disc_progress == 0) {
    bta_dm_lea_disc_complete(peer_address);
  }
}

/*****************************************************************************
 *
 * Function         bta_dm_csis_disc_complete
 *
 * Description      This API updates csis discovery complete status
 *
 * Parameters:      none
 *****************************************************************************/
void bta_dm_csis_disc_complete(RawAddress p_bd_addr, bool status) {
  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb = bta_get_lea_ctrl_cb(p_bd_addr);
  APPL_TRACE_DEBUG("%s %s %d", __func__, p_bd_addr.ToString().c_str(),
      status);

  if (p_lea_cb) {
    p_lea_cb->csip_disc_progress = status;
  } else {
    RawAddress pseudo_addr = bta_get_pseudo_addr_with_id_addr(p_bd_addr);
    if (pseudo_addr != RawAddress::kEmpty) {
      p_lea_cb = bta_get_lea_ctrl_cb(pseudo_addr);
      if (p_lea_cb) {
        p_lea_cb->csip_disc_progress = status;
        APPL_TRACE_DEBUG(" %s Pseudo addr disc_progress resetted", __func__);
      } else {
        APPL_TRACE_DEBUG(" %s No Control Block for pseudo addr", __func__);
      }
    } else {
      APPL_TRACE_DEBUG(" %s No Control Block", __func__);
    }
  }
}

/*****************************************************************************
 *
 * Function         bta_dm_lea_disc_complete
 *
 * Description      This API sends the event to upper layer that LE audio
 *                  gatt operations are complete.
 *
 * Parameters:      none
 *
 ****************************************************************************/
void bta_dm_lea_disc_complete(RawAddress p_bd_addr) {
  tBTA_DM_SEARCH result;
  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb = bta_get_lea_ctrl_cb(p_bd_addr);
  APPL_TRACE_DEBUG("%s %s", __func__, p_bd_addr.ToString().c_str());

  if (p_lea_cb == NULL) {
    RawAddress pseudo_addr = bta_get_pseudo_addr_with_id_addr(p_bd_addr);
    p_lea_cb = bta_get_lea_ctrl_cb(pseudo_addr);
    p_bd_addr = pseudo_addr;
  }

  if (p_lea_cb) {
  APPL_TRACE_DEBUG("csip_disc_progress %d", p_lea_cb->csip_disc_progress);
    if ((p_lea_cb->disc_progress == 0) &&
        (p_lea_cb->csip_disc_progress)) { //Add CSIS check also
      result.adv_audio_disc_cmpl.num_uuids = 0;
      for (uint16_t i = 0; i < p_lea_cb->uuids.size(); i++) {
        result.adv_audio_disc_cmpl.adv_audio_uuids[i] = p_lea_cb->uuids[i];
        result.adv_audio_disc_cmpl.num_uuids++;
      }

      result.adv_audio_disc_cmpl.bd_addr = p_bd_addr;
      APPL_TRACE_DEBUG("Sending Call back with  no of uuids's"
        "p_lea_cb->uuids.size() %d", p_lea_cb->uuids.size());
      bta_dm_search_cb.p_search_cback(BTA_DM_LE_AUDIO_SEARCH_CMPL_EVT, &result);
    } else {
      APPL_TRACE_DEBUG("%s Discovery in progress", __func__);
    }
  } else {
    APPL_TRACE_DEBUG(" %s No Control Block", __func__);
  }
}


/*****************************************************************************
 *
 * Function         bta_add_adv_audio_uuid
 *
 * Description      This is GATT client callback function used in DM.
 *
 * Parameters:
 *
 ******************************************************************************/
void bta_add_adv_audio_uuid(RawAddress peer_address,
                               tBTA_GATT_ID srvc_uuid) {
  auto itr = find(uuid_srv_disc_search.begin(),
                  uuid_srv_disc_search.end(), srvc_uuid.uuid);

  if(itr != uuid_srv_disc_search.end()) {
    tBTA_LE_AUDIO_DEV_INFO *p_lea_cb = bta_get_lea_ctrl_cb(peer_address);
    if (p_lea_cb != NULL) {
      APPL_TRACE_DEBUG(" %s Control Block Found", __func__);

      std::vector<bluetooth::Uuid>::iterator itr;
      itr = std::find(p_lea_cb->uuids.begin(), p_lea_cb->uuids.end(), srvc_uuid.uuid);
      if (itr == p_lea_cb->uuids.end()) {
        p_lea_cb->uuids.push_back(srvc_uuid.uuid);
      }
    } else {
      APPL_TRACE_DEBUG(" %s No Control Block", __func__);
    }
  }
}




/*******************************************************************************
 *
 * Function         bta_set_lea_ctrl_cb
 *
 * Description      This is GATT client callback function used in DM.
 *
 * Parameters:
 *
 ******************************************************************************/

tBTA_LE_AUDIO_DEV_INFO* bta_set_lea_ctrl_cb(RawAddress peer_addr) {
  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb = NULL;

  p_lea_cb = bta_get_lea_ctrl_cb(peer_addr);

  if (p_lea_cb == NULL) {
    APPL_TRACE_DEBUG("%s Control block create ", __func__);

    for (int i = 0; i < MAX_LEA_DEVICES ; i++) {
      if (!bta_le_audio_dev_cb.bta_lea_dev_info[i].in_use) {
        bta_le_audio_dev_cb.bta_lea_dev_info[i].peer_address = peer_addr;
        bta_le_audio_dev_cb.bta_lea_dev_info[i].in_use = true;
        bta_le_audio_dev_cb.bta_lea_dev_info[i].csip_disc_progress = true;
        bta_le_audio_dev_cb.bta_lea_dev_info[i].is_csip_support = false;
        bta_le_audio_dev_cb.bta_lea_dev_info[i].gatt_disc_progress = true;
        bta_le_audio_dev_cb.bta_lea_dev_info[i].enc_pending = true;
        bta_le_audio_dev_cb.num_lea_devices++;
        return (&(bta_le_audio_dev_cb.bta_lea_dev_info[i]));
      }
    }
  } else {
    return p_lea_cb;
  }
  return NULL;
}

/*******************************************************************************
 *
 * Function         bta_dm_reset_adv_audio_dev_info
 *
 * Description      This is GATT client callback function used in DM.
 *
 * Parameters:
 *
 ******************************************************************************/
void bta_dm_reset_adv_audio_dev_info(RawAddress p_addr) {
  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb = bta_get_lea_ctrl_cb(p_addr);

  if (p_lea_cb != NULL) {
    p_lea_cb->peer_address = RawAddress::kEmpty;
    p_lea_cb->disc_progress = 0;
    p_lea_cb->conn_id = 0;
    p_lea_cb->transport = 0;
    p_lea_cb->in_use = false;
    p_lea_cb->t_role_handle = 0;
    p_lea_cb->is_has_found = false;
    p_lea_cb->is_t_audio_srvc_found = false;
    p_lea_cb->pacs_sink_loc_handle = 0;
    p_lea_cb->pacs_src_loc_handle = 0;
    p_lea_cb->pacs_sink_pac_handle = 0;
    p_lea_cb->pacs_src_pac_handle = 0;
    p_lea_cb->pacs_supp_context_handle = 0;
    p_lea_cb->using_bredr_bonding = 0;
    p_lea_cb->gatt_disc_progress = true;
    p_lea_cb->enc_pending = true;
    p_lea_cb->uuids.clear();
    bta_le_audio_dev_cb.gatt_op_addr = RawAddress::kEmpty;
    bta_le_audio_dev_cb.pending_peer_addr = RawAddress::kEmpty;
    bta_le_audio_dev_cb.num_lea_devices--;
    bta_le_audio_dev_cb.bond_progress = false;
    APPL_TRACE_DEBUG("bta_dm_reset_adv_audio_dev_info %s  transport %d ",
      p_lea_cb->peer_address.ToString().c_str(), p_lea_cb->transport);
  }
}

/*******************************************************************************
 *
 * Function         bta_dm_set_adv_audio_dev_info
 *
 * Description      This is GATT client callback function used in DM.
 *
 * Parameters:
 *
 ******************************************************************************/
void bta_dm_set_adv_audio_dev_info(tBTA_GATTC_OPEN* p_data) {
  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb = bta_set_lea_ctrl_cb(p_data->remote_bda);

  if (p_lea_cb != NULL) {
    p_lea_cb->peer_address = p_data->remote_bda;
    p_lea_cb->disc_progress = 0;
    p_lea_cb->conn_id = p_data->conn_id;
    p_lea_cb->transport = p_data->transport;//BTM_UseLeLink(p_data->remote_bda);
    APPL_TRACE_DEBUG("bta_dm_set_adv_audio_dev_info %s  transport %d ",
      p_lea_cb->peer_address.ToString().c_str(), p_lea_cb->transport);
  }

  /* verify bond */
  uint8_t sec_flag = 0;
  BTM_GetSecurityFlagsByTransport(p_data->remote_bda, &sec_flag, BT_TRANSPORT_LE);
  APPL_TRACE_DEBUG(" sec_flag = 0x%x ", sec_flag);

  if (sec_flag & BTM_SEC_FLAG_ENCRYPTED) {
    /* if link has been encrypted */
    APPL_TRACE_DEBUG("link has been encrypted for device: %s ",
            p_data->remote_bda.ToString().c_str());
    bta_dm_proc_open_evt(p_data);
    return;
  }

  if (sec_flag & BTM_SEC_FLAG_LKEY_KNOWN) {
    /* if bonded and link not encrypted */
    APPL_TRACE_DEBUG("trying to encrypt now for device: %s ",
             p_data->remote_bda.ToString().c_str());
    sec_flag = BTM_BLE_SEC_ENCRYPT;
    BTM_SetEncryption(p_data->remote_bda, BTA_TRANSPORT_LE,
             bta_dm_adv_audio_encryption_callback, nullptr, sec_flag);
    return;
  }

  /* otherwise let it go through */
  APPL_TRACE_DEBUG("encryption failed for device : %s ",
            p_data->remote_bda.ToString().c_str());
  bta_le_audio_service_search_failed(&p_data->remote_bda);
}

void bta_dm_adv_audio_encryption_callback(const RawAddress* address,
                                        tGATT_TRANSPORT transport,
                                        void* p_ref_data, tBTM_STATUS result) {
  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb = bta_get_lea_ctrl_cb(*address);
  tBTA_GATTC_OPEN data;
  memset(&data, 0, sizeof(data));
  APPL_TRACE_DEBUG("bta_dm_adv_audio_encryption_callback: address %s, result %d ",
          (*address).ToString().c_str(), result);

  if (p_lea_cb != NULL && result == BTM_SUCCESS) {
    p_lea_cb->enc_pending = false;
    data.status = GATT_SUCCESS;
    data.conn_id = p_lea_cb->conn_id;
    data.client_if = p_lea_cb->gatt_if;
    data.remote_bda = p_lea_cb->peer_address;
    data.transport = p_lea_cb->transport;
    if (!p_lea_cb->gatt_disc_progress) {
        bta_get_adv_audio_role(p_lea_cb->peer_address,p_lea_cb->conn_id,
                               BTM_SUCCESS);
    }
    bta_dm_proc_open_evt(&data);
  } else {
    bta_le_audio_service_search_failed((void*)address);
  }
}

/*******************************************************************************
 *
 * Function         is_adv_audio_unicast_supported
 *
 * Description      This function checks whether unicast support is there or not on
 *                  remote side
 *
 * Parameters:
 *
 ******************************************************************************/

bool is_adv_audio_unicast_supported(RawAddress rem_bda, int conn_id) {
  const std::vector<gatt::Service>* services = BTA_GATTC_GetServices(conn_id);

  if (services) {
    for (const gatt::Service& service : *services) {
      uint16_t uuid_val = service.uuid.As16Bit();
      if (uuid_val == UUID_SERVCLASS_ASCS)
        return true;
    }
  }

  return false;
}

/*******************************************************************************
 *
 * Function         is_adv_audio_group_supported
 *
 * Description      This function checks whether csip support is there or not on
 *                  remote side
 *
 * Parameters:
 *
 ******************************************************************************/

bool is_adv_audio_group_supported(RawAddress rem_bda, int conn_id) {
  const std::vector<gatt::Service>* services = BTA_GATTC_GetServices(conn_id);

  if (services) {
    for (const gatt::Service& service : *services) {
      if (is_le_audio_service(service.uuid)) {
        uint16_t uuid_val = service.uuid.As16Bit();
        if (uuid_val == UUID_SERVCLASS_CSIS)
          return true;
      }
    }
  }

  return false;
}

/*******************************************************************************
 *
 * Function         bta_dm_lea_gattc_callback
 *
 * Description      This is GATT client callback function used in DM.
 *
 * Parameters:
 *
 ******************************************************************************/

void bta_dm_lea_gattc_callback(tBTA_GATTC_EVT event, tBTA_GATTC* p_data) {
  APPL_TRACE_DEBUG("bta_dm_lea_gattc_callback event = %d", event);

  switch (event) {
    case BTA_GATTC_OPEN_EVT:
      if (alarm_is_scheduled(dm_le_conn_alarm.gatt_le_conn_tout_alarm))
        alarm_cancel(dm_le_conn_alarm.gatt_le_conn_tout_alarm);
      if (p_data->status != GATT_SUCCESS) {
        btif_dm_release_action_uuid(bta_le_audio_dev_cb.pending_peer_addr);
        if (is_remote_support_adv_audio(bta_le_audio_dev_cb.pending_peer_addr)) {
          btif_le_audio_conn_failed(bta_le_audio_dev_cb.pending_peer_addr);
        }
      } else {
        if (is_remote_support_adv_audio(bta_le_audio_dev_cb.pending_peer_addr)) {
          bta_dm_set_adv_audio_dev_info(&p_data->open);
        } else {
          bta_dm_proc_open_evt(&p_data->open);
        }
      }
      break;

    case BTA_GATTC_SEARCH_RES_EVT:
      if (is_remote_support_adv_audio(bta_le_audio_dev_cb.pending_peer_addr)) {
        bta_add_adv_audio_uuid(bta_le_audio_dev_cb.pending_peer_addr,
                           p_data->srvc_res.service_uuid);
      }
      break;

    case BTA_GATTC_SEARCH_CMPL_EVT:
      if (p_data->search_cmpl.status == 0) {
        tBTA_LE_AUDIO_DEV_INFO *p_lea_cb =
          bta_get_lea_ctrl_cb(bta_le_audio_dev_cb.pending_peer_addr);
        if (p_lea_cb) {
          p_lea_cb->gatt_disc_progress = false;
          if (!p_lea_cb->enc_pending) {
            bta_get_adv_audio_role(bta_le_audio_dev_cb.pending_peer_addr,
                p_data->search_cmpl.conn_id,
                p_data->search_cmpl.status);
            if (is_adv_audio_group_supported(bta_le_audio_dev_cb.pending_peer_addr,
                  p_data->search_cmpl.conn_id)) {
              RawAddress p_id_addr =
                bta_get_rem_dev_id_addr(bta_le_audio_dev_cb.pending_peer_addr);
              if (p_id_addr != RawAddress::kEmpty) {
                BTA_CsipFindCsisInstance(p_data->search_cmpl.conn_id,
                    p_data->search_cmpl.status,
                    p_id_addr);
              } else {
                BTA_CsipFindCsisInstance(p_data->search_cmpl.conn_id,
                    p_data->search_cmpl.status,
                    bta_le_audio_dev_cb.pending_peer_addr);
              }
            }
          } else {
            APPL_TRACE_WARNING("%s ENCRYPTION PENDING so no role discovery",
                __func__);
          }
        } else {
          APPL_TRACE_WARNING("%s CONTROL BLOCK IS NOT PRESENT", __func__);
        }
      } else {
          APPL_TRACE_DEBUG("%s Discovery Failure ", __func__);
          bta_le_audio_service_search_failed
            (&bta_le_audio_dev_cb.pending_peer_addr);
      }
      break;

    case BTA_GATTC_CLOSE_EVT:
      APPL_TRACE_DEBUG("BTA_GATTC_CLOSE_EVT reason = %d, data conn_id %d,"
          "search conn_id %d", p_data->close.reason, p_data->close.conn_id,
          bta_dm_search_cb.conn_id);

      if (is_remote_support_adv_audio(bta_le_audio_dev_cb.pending_peer_addr)) {
        bta_dm_reset_adv_audio_dev_info(bta_le_audio_dev_cb.pending_peer_addr);
      }
      break;

    default:
      break;
  }
}

/******************************************************************************
 *
 * Function         bta_dm_adv_audio_gatt_conn
 *
 * Description      This API opens the gatt conn after finding sdp record
 *                  during BREDR Discovery
 *
 * Parameters:      none
 *
 ******************************************************************************/
void bta_dm_adv_audio_gatt_conn(RawAddress p_bd_addr) {
  APPL_TRACE_DEBUG("bta_dm_adv_audio_gatt_conn ");

  bta_le_audio_dev_cb.pending_peer_addr = p_bd_addr;

  tBTA_LE_AUDIO_DEV_INFO *tmp_lea_cb = bta_get_lea_ctrl_cb(p_bd_addr);
  if (tmp_lea_cb && tmp_lea_cb->in_use) {
    APPL_TRACE_DEBUG("bta_dm_adv_audio_gatt_conn Already exists %d",
      tmp_lea_cb->conn_id);
    return;
  }
  if (!bta_dm_le_set_alarm(p_bd_addr)) {
    bta_le_audio_service_search_failed(&p_bd_addr);
    return;
  }

  BTA_GATTC_AppRegister(bta_dm_lea_gattc_callback,
      base::Bind([](uint8_t client_id, uint8_t status) {
        if (status == GATT_SUCCESS) {
          tBTA_LE_AUDIO_DEV_INFO *p_lea_cb =
            bta_set_lea_ctrl_cb(bta_le_audio_dev_cb.pending_peer_addr);
            if (p_lea_cb) {
              APPL_TRACE_DEBUG("bta_dm_adv_audio_gatt_conn Client Id: %d",
                  client_id);
              p_lea_cb->gatt_if = client_id;
              p_lea_cb->using_bredr_bonding = true;
              BTA_GATTC_Open(client_id, bta_le_audio_dev_cb.pending_peer_addr,
                  true, GATT_TRANSPORT_LE, false);
            }
        }
        }), false);

}

/******************************************************************************
 *
 * Function         bta_dm_adv_audio_close
 *
 * Description      This API closes the gatt conn with was opened by dm layer
 *                  for service discovery (or) opened after finding sdp record
 *                  during BREDR Discovery
 *
 * Parameters:      none
 *
 ******************************************************************************/
void bta_dm_adv_audio_close(RawAddress p_bd_addr) {
  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb = bta_get_lea_ctrl_cb(p_bd_addr);
  APPL_TRACE_DEBUG("%s", __func__);

  if (p_lea_cb) {
    APPL_TRACE_DEBUG("%s %d", __func__, p_lea_cb->gatt_if);
    if (p_lea_cb->using_bredr_bonding) {
      APPL_TRACE_DEBUG("%s closing LE conn est due to bredr bonding  %d", __func__,
          p_lea_cb->gatt_if);
      alarm_cancel(bta_dm_search_cb.gatt_close_timer);
      BTA_GATTC_AppDeregister(p_lea_cb->gatt_if);
      bta_dm_reset_adv_audio_dev_info(p_bd_addr);
    } else {
      bta_sys_start_timer(bta_dm_search_cb.gatt_close_timer,
          BTA_DM_ADV_AUDIO_GATT_CLOSE_DELAY_TOUT,
          BTA_DM_DISC_CLOSE_TOUT_EVT, 0);
    }
  }
}

/*******************************************************************************
 *
 * Function         bta_get_lea_ctrl_cb
 *
 * Description      This API returns pairing control block of LE AUDIO DEVICE
 *
 * Parameters:      tBTA_DEV_PAIRING_CB
 *
 ******************************************************************************/
tBTA_DEV_PAIRING_CB* bta_get_lea_pair_cb(RawAddress peer_addr) {
  tBTA_DEV_PAIRING_CB *p_lea_pair_cb = NULL;
  p_lea_pair_cb = &bta_lea_pairing_cb.bta_dev_pair_db[0];
  APPL_TRACE_DEBUG("%s %s ", __func__, peer_addr.ToString().c_str());

  for (int i = 0; i < MAX_LEA_DEVICES; i++) {
      if ((p_lea_pair_cb[i].in_use) &&
        (p_lea_pair_cb[i].p_addr == peer_addr)) {
        APPL_TRACE_DEBUG("%s Found %s index i %d ", __func__,
          p_lea_pair_cb[i].p_addr.ToString().c_str(), i);
        return &p_lea_pair_cb[i];
      }
    }
  return NULL;
}



/*******************************************************************************
 *
 * Function         bta_set_lea_ctrl_cb
 *
 * Description      This is GATT client callback function used in DM.
 *
 * Parameters:
 *
 ******************************************************************************/

tBTA_DEV_PAIRING_CB* bta_set_lea_pair_cb(RawAddress peer_addr) {
  tBTA_DEV_PAIRING_CB *p_lea_pair_cb = NULL;
  APPL_TRACE_DEBUG("bta_set_lea_ctrl_cb %s", peer_addr.ToString().c_str());

  p_lea_pair_cb = bta_get_lea_pair_cb(peer_addr);

  if (p_lea_pair_cb == NULL) {
    APPL_TRACE_DEBUG("bta_set_lea_ctrl_cb Control block create ");

    for (int i = 0; i < MAX_LEA_DEVICES ; i++) {
      if (!bta_lea_pairing_cb.bta_dev_pair_db[i].in_use) {
        bta_lea_pairing_cb.bta_dev_pair_db[i].p_addr = peer_addr;
        bta_lea_pairing_cb.bta_dev_pair_db[i].in_use = true;
        bta_lea_pairing_cb.is_pairing_progress = true;
        bta_lea_pairing_cb.num_devices++;
        return (&(bta_lea_pairing_cb.bta_dev_pair_db[i]));
      }
    }
  } else {
    return p_lea_pair_cb;
  }
  return NULL;
}

/*******************************************************************************
 *
 * Function         bta_dm_reset_adv_audio_dev_info
 *
 * Description      This API resets all the pairing information related to le
 *                  audio remote device.
 * Parameters:      none
 *
 ******************************************************************************/
void bta_dm_reset_lea_pairing_info(RawAddress p_addr) {

  APPL_TRACE_DEBUG("%s Addr %s", __func__, p_addr.ToString().c_str());

  auto itr = bta_lea_pairing_cb.dev_addr_map.find(p_addr);
  if (itr != bta_lea_pairing_cb.dev_addr_map.end()) {
    bta_lea_pairing_cb.dev_addr_map.erase(p_addr);
  }

  itr = bta_lea_pairing_cb.dev_rand_addr_map.find(p_addr);
  if (itr != bta_lea_pairing_cb.dev_rand_addr_map.end()) {
    bta_lea_pairing_cb.dev_rand_addr_map.erase(p_addr);
  }

  tBTA_DEV_PAIRING_CB *p_lea_pair_cb = NULL;
  p_lea_pair_cb = bta_get_lea_pair_cb(p_addr);
  if (p_lea_pair_cb) {
    APPL_TRACE_DEBUG("%s RESETTING VALUES", __func__);
    p_lea_pair_cb->in_use = false;
    p_lea_pair_cb->is_dumo_device = false;
    p_lea_pair_cb->is_le_pairing = false;
    p_lea_pair_cb->dev_type = 0;
    if (p_lea_pair_cb->p_addr != RawAddress::kEmpty) {
      itr = bta_lea_pairing_cb.dev_addr_map.find(p_lea_pair_cb->p_id_addr);
      APPL_TRACE_DEBUG("%s RESETTING Addr %s", __func__,
        p_lea_pair_cb->p_addr.ToString().c_str());
      if (itr != bta_lea_pairing_cb.dev_addr_map.end()) {
        APPL_TRACE_DEBUG("%s Clearing INSIDE LEA ADDR DB MAP",
          __func__);
        bta_lea_pairing_cb.dev_addr_map.erase(p_lea_pair_cb->p_id_addr);
      }
      p_lea_pair_cb->p_id_addr = RawAddress::kEmpty;
      p_lea_pair_cb->transport = 0;
      p_lea_pair_cb->p_addr = RawAddress::kEmpty;
    }
    bta_lea_pairing_cb.is_pairing_progress = false;
    bta_lea_pairing_cb.num_devices--;
    bta_lea_pairing_cb.is_sdp_discover = true;
  } else {
    APPL_TRACE_DEBUG("%s INVALID CONTROL BLOCK", __func__);
  }
}
/*****************************************************************************
 *
 * Function        bta_dm_reset_adv_audio_pairing_info
 *
 * Description      This API resets all the pairing information related to le
 *                  audio remote device.
 *
 * Returns         none
 *
 *****************************************************************************/
void bta_dm_reset_adv_audio_pairing_info(RawAddress p_addr) {

  APPL_TRACE_DEBUG("%s Addr %s", __func__, p_addr.ToString().c_str());
  tBTA_DEV_PAIRING_CB *p_lea_pair_cb = NULL;
  p_lea_pair_cb = bta_get_lea_pair_cb(p_addr);
  if (p_lea_pair_cb ) {
   bta_dm_reset_lea_pairing_info(p_addr);
 }
}
/*****************************************************************************
 *
 * Function        bta_dm_ble_adv_audio_idaddr_map
 *
 * Description     storing the identity address information in the device
 *                 control block. It will used for DUMO devices
 *
 * Returns         none
 *
 *****************************************************************************/
void bta_dm_ble_adv_audio_idaddr_map(RawAddress p_bd_addr,
  RawAddress p_id_addr) {
  int device_type = 0;

  btif_get_device_type(p_bd_addr, &device_type);
  APPL_TRACE_DEBUG("%s p_bd_addr %s id_addr %s device type %d ", __func__,
    p_bd_addr.ToString().c_str(), p_id_addr.ToString().c_str(), device_type);

  if (device_type != BT_DEVICE_TYPE_DUMO) {
    APPL_TRACE_DEBUG("%s %s not DUAL Mode device. So skip it", __func__,
        p_bd_addr.ToString().c_str());
    return;
  }

  if (is_remote_support_adv_audio(p_bd_addr)) {
    bta_lea_pairing_cb.dev_addr_map[p_id_addr] = p_bd_addr;
    bta_lea_pairing_cb.dev_rand_addr_map[p_bd_addr] = p_id_addr;

    tBTA_DEV_PAIRING_CB *p_lea_pair_cb = NULL;
    p_lea_pair_cb = bta_get_lea_pair_cb(p_bd_addr);
    if (p_lea_pair_cb) {
      if (p_id_addr != p_bd_addr) {
        APPL_TRACE_DEBUG("%s is_dumo_device %s", __func__,
          p_id_addr.ToString().c_str());
        p_lea_pair_cb->p_id_addr = p_id_addr;

        if (device_type == BT_DEVICE_TYPE_DUMO)
          p_lea_pair_cb->is_dumo_device = true;
        else
          p_lea_pair_cb->is_dumo_device = false;
      }
    }
  }
}

bool bta_remote_dev_identity_addr_match(RawAddress p_addr) {
  APPL_TRACE_DEBUG("%s ", __func__);

  auto itr = bta_lea_pairing_cb.dev_addr_map.find(p_addr);

  if (itr != bta_lea_pairing_cb.dev_addr_map.end()) {
    APPL_TRACE_DEBUG("%s Identity BD_ADDR %s", __func__,
      p_addr.ToString().c_str());
      return true;
  }
  return false;
}
bool bta_lea_is_le_pairing(){
  return bta_lea_pairing_cb.is_pairing_progress;
}
bool bta_is_bredr_primary_transport(RawAddress p_bd_addr) {

  tBTA_DEV_PAIRING_CB *p_lea_pair_cb = NULL;

  p_lea_pair_cb = bta_get_lea_pair_cb(p_bd_addr);
  APPL_TRACE_DEBUG("%s ", __func__);
  if (p_lea_pair_cb) {
    APPL_TRACE_DEBUG("%s Transport %d ", __func__, p_lea_pair_cb->transport);
    if (p_lea_pair_cb->transport == BT_TRANSPORT_BR_EDR) {
      return true;
    }
  }

  return false;
}

bool bta_remote_device_is_dumo(RawAddress p_bd_addr) {
  int device_type = 0;

  btif_get_device_type(p_bd_addr, &device_type);
  if (device_type != BT_DEVICE_TYPE_DUMO) {
    APPL_TRACE_DEBUG("%s Addr is not dumo device %s", __func__,
        p_bd_addr.ToString().c_str());
    return false;
  }

  tBTA_DEV_PAIRING_CB *p_lea_pair_cb = NULL;
  p_lea_pair_cb = bta_get_lea_pair_cb(p_bd_addr);
  if (p_lea_pair_cb) {
    if (!p_lea_pair_cb->is_dumo_device) {
      APPL_TRACE_DEBUG("%s is_dumo_device flag is 0 %s", __func__,
                        p_bd_addr.ToString().c_str());
      return false;
    }
  }

  auto itr = bta_lea_pairing_cb.dev_addr_map.find(p_bd_addr);
  APPL_TRACE_DEBUG("%s Addr %s", __func__, p_bd_addr.ToString().c_str());

  if (itr != bta_lea_pairing_cb.dev_addr_map.end()) {
    APPL_TRACE_DEBUG("%s DUMO DEVICE Identity BD_ADDR %s", __func__,
      p_bd_addr.ToString().c_str());
      return true;
  }

  auto itr2 = bta_lea_pairing_cb.dev_rand_addr_map.find(p_bd_addr);
  if (itr2 != bta_lea_pairing_cb.dev_rand_addr_map.end()) {
    APPL_TRACE_DEBUG("%s Dumo addressed %s %s ", __func__,
      itr2->first.ToString().c_str(), itr2->second.ToString().c_str());
    return true;
  }
  return false;
}

RawAddress bta_get_rem_dev_id_addr(RawAddress p_bd_addr) {
  tBTA_DEV_PAIRING_CB *p_lea_pair_cb = NULL;
  APPL_TRACE_DEBUG("%s ", __func__);

  p_lea_pair_cb = bta_get_lea_pair_cb(p_bd_addr);
  if (p_lea_pair_cb) {
    APPL_TRACE_DEBUG("%s %s", __func__,
      p_lea_pair_cb->p_id_addr.ToString().c_str());
    return p_lea_pair_cb->p_id_addr;
  }
  return RawAddress::kEmpty;
}

/*****************************************************************************
 *
 * Function        bta_adv_audio_update_bond_db
 *
 * Description     Updates pairing control block of the device and the bonding
 *                 is initiated using LE transport or not.
 *
 * Returns         void
 *
 *****************************************************************************/
void bta_adv_audio_update_bond_db(RawAddress p_bd_addr, uint8_t transport) {
  tBTA_DEV_PAIRING_CB *p_dev_pair_cb = bta_set_lea_pair_cb(p_bd_addr);

  APPL_TRACE_DEBUG("%s", __func__);
  if (p_dev_pair_cb) {
    APPL_TRACE_DEBUG("%s Addr %s Transport %d", __func__,
      p_bd_addr.ToString().c_str(),  transport);
    p_dev_pair_cb->p_addr = p_bd_addr;
    p_dev_pair_cb->transport = transport;
    if (transport == BT_TRANSPORT_LE) {
      if (is_remote_support_adv_audio(p_dev_pair_cb->p_addr))
        p_dev_pair_cb->is_le_pairing = true;
      else
        p_dev_pair_cb->is_le_pairing = false;
    } else
      p_dev_pair_cb->is_le_pairing = false;
  }
}

/*****************************************************************************
 *
 * Function        is_le_audio_service
 *
 * Description     It checks whether the given service is related to the LE
 *                 Audio service or not.
 *
 * Returns         true for LE Audio service which are registered.
                   false by default
 *
 *****************************************************************************/
bool is_le_audio_service(Uuid uuid) {

  uint16_t uuid_val = 0;
  bool status = false;

  size_t len = uuid.GetShortestRepresentationSize();
  if (len == Uuid::kNumBytes16) {
    uuid_val = uuid.As16Bit();
    APPL_TRACE_DEBUG("%s: 0x%X, 0x%X ", __func__, uuid.As16Bit(), uuid_val);
    //TODO check the service contains any LE AUDIO service or not
    switch (uuid_val) {
      case UUID_SERVCLASS_VCS:
        FALLTHROUGH_INTENDED; /* FALLTHROUGH */
      case UUID_SERVCLASS_CSIS:
        FALLTHROUGH_INTENDED; /* FALLTHROUGH */
      case UUID_SERVCLASS_BASS:
        FALLTHROUGH_INTENDED; /* FALLTHROUGH */
      case UUID_SERVCLASS_TMAS:
        FALLTHROUGH_INTENDED; /* FALLTHROUGH */
      case UUID_SERVCLASS_ASCS:
        FALLTHROUGH_INTENDED; /* FALLTHROUGH */
      case UUID_SERVCLASS_BAAS:
        FALLTHROUGH_INTENDED; /* FALLTHROUGH */
      case UUID_SERVCLASS_CAS:
        FALLTHROUGH_INTENDED; /* FALLTHROUGH */
      case UUID_SERVCLASS_PACS:
        {
          auto itr = find(uuid_srv_disc_search.begin(),
              uuid_srv_disc_search.end(), uuid);
          if (itr != uuid_srv_disc_search.end())
            status = true;
        }
        break;
      default:
        APPL_TRACE_DEBUG("%s : Not a LEA service ", __func__);
    }
  } else if(len == Uuid::kNumBytes128) {
    if (uuid == UUID_SERVCLASS_WMCP) {
      APPL_TRACE_DEBUG("%s: WMCP Service UUId found", __func__);
      auto itr = find(uuid_srv_disc_search.begin(),
          uuid_srv_disc_search.end(), uuid);
      if (itr != uuid_srv_disc_search.end())
        status = true;
    }
    if (uuid == UUID_SERVCLASS_GMAP) {
      APPL_TRACE_DEBUG("%s: GMAP Service UUId found", __func__);
      auto itr = find(uuid_srv_disc_search.begin(),
          uuid_srv_disc_search.end(), uuid);
      if (itr != uuid_srv_disc_search.end())
        status = true;
    }
  }

  return status;
}

/*****************************************************************************
 *
 * Function        bta_is_adv_audio_valid_bdaddr
 *
 * Description     This API is used for DUMO device. If the device contains
 *                 two address (random and public), it checks for valid
 *                 address.
 *
 * Returns         0 - for random address in dumo device
 *                 1 - for public address in dumo device
 *
 ****************************************************************************/
int bta_is_adv_audio_valid_bdaddr(RawAddress p_bd_addr) {
  tBTA_DEV_PAIRING_CB *p_lea_pair_cb = NULL;
  p_lea_pair_cb = bta_get_lea_pair_cb(p_bd_addr);

  if (p_lea_pair_cb) {
    APPL_TRACE_DEBUG("%s p_lea_pair_cb %s", __func__,
      p_lea_pair_cb->p_addr.ToString().c_str());
    auto itr = bta_lea_pairing_cb.dev_addr_map.find(p_bd_addr);
    if (itr == bta_lea_pairing_cb.dev_addr_map.end() &&
        (p_lea_pair_cb->is_dumo_device)) {
      APPL_TRACE_DEBUG("%s Ignore BD_ADDR because of ID %s", __func__,
        p_lea_pair_cb->p_id_addr.ToString().c_str());
        return 0;
    }
  }
  return 1;
}

/*****************************************************************************
 *
 * Function        devclass2uint
 *
 * Description     This API is to derive the class of device based of dev_class
 *
 * Returns         uint32_t - class of device
 *
 ****************************************************************************/
static uint32_t devclass2uint(DEV_CLASS dev_class) {
  uint32_t cod = 0;

  if (dev_class != NULL) {
    /* if COD is 0, irrespective of the device type set it to Unclassified
     * device */
    cod = (dev_class[2]) | (dev_class[1] << 8) | (dev_class[0] << 16);
  }
  return cod;
}

/*****************************************************************************
 *
 * Function        bta_is_remote_support_lea
 *
 * Description     This API is to check the remote device contains LEA service
 *                 or not. It checks in Inquiry database initially.
 *                 If the address is Public identity address then it will
 *                 check in the pairing database of that remote device.
 *
 * Returns         true - if remote device inquiry db contains LEA service
 *
 ****************************************************************************/
bool bta_is_remote_support_lea(RawAddress p_addr) {
  tBTM_INQ_INFO* p_inq_info;

  p_inq_info = BTM_InqDbRead(p_addr);
  if (p_inq_info != NULL) {
    uint32_t cod = devclass2uint(p_inq_info->results.dev_class);
    BTIF_TRACE_DEBUG("%s cod is 0x%06x", __func__, cod);
    if ((cod & MAJOR_LE_AUDIO_VENDOR_COD)
          == MAJOR_LE_AUDIO_VENDOR_COD) {
      return true;
    }
  }

  /* check the address is public identity address and its related to random
   * address which supports to LEA then that Public ID address should return
   * true.
   */
  auto itr = bta_lea_pairing_cb.dev_addr_map.find(p_addr);
  if (itr != bta_lea_pairing_cb.dev_addr_map.end()) {
    BTIF_TRACE_DEBUG("%s Idenity address mapping", __func__);
    return true;
  }

  return false;
}

void bta_find_adv_audio_group_instance(uint16_t conn_id, tGATT_STATUS status,
    RawAddress p_addr) {
    BTA_CsipFindCsisInstance(conn_id, status, p_addr);
}

/*******************************************************************************
 *
 * Function         is_gatt_srvc_disc_pending
 *
 * Description      This function checks whether gatt_srvc_disc is processing
 *                  or not
 *
 * Parameters:
 *
 ******************************************************************************/
bool is_gatt_srvc_disc_pending(RawAddress rem_bda) {
  tBTA_LE_AUDIO_DEV_INFO *p_lea_cb = bta_get_lea_ctrl_cb(rem_bda);

  APPL_TRACE_DEBUG("%s ", __func__);
  if (p_lea_cb == NULL) {
    return false;
  } else {
    APPL_TRACE_DEBUG("%s gatt_disc_progress %d ", __func__,
        p_lea_cb->gatt_disc_progress);
    return p_lea_cb->gatt_disc_progress;
  }
}

/******************************************************************************
 *
 * Function         bta_get_pseudo_addr_with_id_addr
 *
 * Description      This function returns the mapping id_addr(if present) to
 *                  pseudo addr
 *
 * Parameters:
 *
 *****************************************************************************/
RawAddress bta_get_pseudo_addr_with_id_addr(RawAddress p_addr) {
  auto itr = bta_lea_pairing_cb.dev_addr_map.find(p_addr);

  APPL_TRACE_DEBUG("%s p_addr %s ", __func__, p_addr.ToString().c_str());
  if (itr != bta_lea_pairing_cb.dev_addr_map.end()) {
    APPL_TRACE_DEBUG("%s addr is mapped to %s ", __func__,
        itr->second.ToString().c_str());
    if (itr->second != RawAddress::kEmpty) {
      return itr->second;
    }
  }
  return p_addr;
}

/******************************************************************************
 *
 * Function         bta_dm_update_adv_audio_db
 *
 * Description      This function adds the bd_addr to adv_audio_device_db
 *
 * Parameters:
 *
 *****************************************************************************/
void bta_dm_update_adv_audio_db(const RawAddress& bd_addr) {
  adv_audio_device_db.emplace(bd_addr, MAJOR_LE_AUDIO_VENDOR_COD);
}
