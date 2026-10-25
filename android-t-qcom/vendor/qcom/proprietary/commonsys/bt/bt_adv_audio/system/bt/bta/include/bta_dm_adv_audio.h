/*
 * Copyright (c) 2022 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
*/
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef BTA_DM_ADV_AUDIO_H
#define BTA_DM_ADV_AUDIO_H

#include "stack/include/bt_types.h"
#include "bta/dm/bta_dm_int.h"
#include <memory>
#include <map>
#include "bt_target.h"
#include "bta_sys.h"

#include "bta_gatt_api.h"

#define UUID_SERVCLASS_VCS 0x1844 /* Volume Control Service */
#define UUID_SERVCLASS_CSIS 0x1846 /* Coordinated Set Identification Service */
#define UUID_SERVCLASS_PACS 0x1850 /* LE AUDIO PACS */
#define UUID_SERVCLASS_ASCS 0x184E /* LE AUDIO ASCS */
#define UUID_SERVCLASS_BASS 0x184F /* LE AUDIO BASS */
#define UUID_SERVCLASS_BAAS 0x1851 /* LE AUDIO BAAS */
#define UUID_SERVCLASS_BRASS 0x1852 /* LE AUDIO BRASS */
#define UUID_SERVCLASS_TMAS 0x1855 /* LE AUDIO TMAP */
#define UUID_SERVCLASS_CAS 0x1853 /* LE AUDIO CAS */
#define UUID_SERVCLASS_CSIS_LOCK 0x2B86 /* LE AUDIO CSIS LOCK */
#define UUID_SERVCLASS_TMAP_ROLE_CHAR 0x2B51 /*TMAP ROLE  */
#define UUID_SERVCLASS_T_ADV_AUDIO_MEDIA_SINK 0x6AD0
#define UUID_SERVCLASS_T_ADV_AUDIO_VOICE 0x6AD5
#define UUID_SERVCLASS_T_ADV_AUDIO_CONN_LESS_MEDIA_SINK 0xFFA6
#define UUID_SERVCLASS_T_ADV_AUDIO_ASSIST 0xFFA7
#define UUID_SERVCLASS_T_ADV_AUDIO_DELEGATE 0xFFA8
#define UUID_SERVCLASS_HAS 0x6AD2
#define UUID_SERVCLASS_SOURCE_CONTEXT 0x2BCE
#define UUID_SERVCLASS_PACS_CT_SUPPORT 0x6AD4
#define UUID_SERVCLASS_PACS_UMR_SUPPORT 0x6AD1
#define UUID_SERVCLASS_PACS_SRC_SUPPORT_GAME_CONTEXT 0x6AD6

#define BTA_DM_LE_AUDIO_SEARCH_CMPL_EVT 7
#define BTA_DM_LE_AUDIO_CONN_FAIL_EVT 8

#define BTA_DM_GROUP_DATA_TYPE 0x2E

typedef struct {
  RawAddress peer_address;
  bool in_use =false;
  bool is_t_audio_srvc_found = false;
  bool is_has_found = false;
  std::vector<bluetooth::Uuid> uuids;
  int transport; //BTM_UseLeLink(remote_bda);
  int conn_id = 0;
  int8_t disc_progress = 0;
  uint8_t gatt_if;
  bool using_bredr_bonding = false;
  uint16_t t_role_handle = 0;
  uint16_t pacs_supp_context_handle = 0;
  uint16_t pacs_src_loc_handle = 0;
  uint16_t pacs_sink_loc_handle = 0;
  uint16_t pacs_src_pac_handle = 0;
  uint16_t pacs_sink_pac_handle = 0;
  bool csip_disc_progress = true;
  bool is_csip_support = false;
  bool gatt_disc_progress = true;
  bool enc_pending = true;
} tBTA_LE_AUDIO_DEV_INFO;

typedef struct {
  RawAddress p_addr;
  RawAddress p_id_addr;
  bool is_le_pairing = false;
  bool in_use = false;
  bool is_dumo_device = false;
  uint8_t dev_type;
  uint8_t transport;
  bool sdp_disc_status = true;
} tBTA_DEV_PAIRING_CB;

#define MAX_LEA_DEVICES 3
typedef struct {
  tBTA_DEV_PAIRING_CB bta_dev_pair_db[MAX_LEA_DEVICES];
  uint8_t num_devices;
  bool is_pairing_progress = false;
  std::map <RawAddress, RawAddress> dev_addr_map;
  std::map <RawAddress, RawAddress> dev_rand_addr_map;
  RawAddress pending_address;
  bool is_sdp_discover = true;
} tBTA_LEA_PAIRING_DB;


typedef struct {
  tBTA_LE_AUDIO_DEV_INFO bta_lea_dev_info[MAX_LEA_DEVICES];
  RawAddress pending_peer_addr;
  RawAddress gatt_op_addr = RawAddress::kEmpty;
  int num_lea_devices  = 0;
  bool bond_progress = false;
} tBTA_LE_AUDIO_DEV_CB;

extern std::unordered_map<RawAddress, uint32_t> adv_audio_device_db;
extern tBTA_LE_AUDIO_DEV_CB bta_le_audio_dev_cb;
extern bool is_remote_support_adv_audio(const RawAddress remote_bdaddr);
extern void bta_adv_audio_update_bond_db(RawAddress p_bd_addr, uint8_t transport);
extern bool is_le_audio_service(bluetooth::Uuid uuid);
extern int bta_is_adv_audio_valid_bdaddr(RawAddress p_bd_addr);
extern bool bta_is_le_audio_supported(RawAddress p_bd_addr);
extern bool bta_remote_device_is_dumo(RawAddress p_bd_addr);
extern RawAddress bta_get_rem_dev_id_addr(RawAddress p_bd_addr);
extern tBTA_DEV_PAIRING_CB* bta_get_lea_pair_cb(RawAddress peer_addr);
extern bool bta_lea_addr_match(RawAddress p_bd_addr);
extern void bta_dm_reset_lea_pairing_info(RawAddress p_addr);
extern void bta_dm_reset_adv_audio_pairing_info(RawAddress p_addr);
extern bool bta_is_bredr_primary_transport(RawAddress p_bd_addr);
extern bool bta_is_remote_support_lea(RawAddress p_addr);
extern bool bta_remote_dev_identity_addr_match(RawAddress p_addr);
extern void bta_dm_lea_disc_complete(RawAddress p_bd_addr);
extern void bta_dm_csis_disc_complete(RawAddress p_bd_addr, bool status);
extern tBTA_LE_AUDIO_DEV_INFO* bta_get_lea_ctrl_cb(RawAddress peer_addr);
extern void bta_gap_gatt_read_cb(uint16_t conn_id, tGATT_STATUS status,
                  uint16_t handle, uint16_t len,
                  uint8_t* value, void* data);
extern void bta_get_adv_audio_role(RawAddress peer_address, uint16_t conn_id,
                                  tGATT_STATUS status);
extern void bta_dm_csis_disc_complete(RawAddress p_bd_addr, bool status);
extern void bta_dm_lea_disc_complete(RawAddress p_bd_addr);
extern void bta_add_adv_audio_uuid(RawAddress peer_address,
                               tBTA_GATT_ID srvc_uuid);
extern tBTA_LE_AUDIO_DEV_INFO* bta_set_lea_ctrl_cb(RawAddress peer_addr);
extern void bta_dm_reset_adv_audio_dev_info(RawAddress p_addr);
extern void bta_dm_set_adv_audio_dev_info(tBTA_GATTC_OPEN* p_data);
extern void bta_dm_adv_audio_encryption_callback(const RawAddress* address,
                  tGATT_TRANSPORT transport,
                  void* p_ref_data, tBTM_STATUS result);
extern bool is_adv_audio_group_supported(RawAddress rem_bda, int conn_id);
extern void bta_dm_lea_gattc_callback(tBTA_GATTC_EVT event, tBTA_GATTC* p_data);
extern void bta_dm_adv_audio_gatt_conn(RawAddress p_bd_addr);
extern void bta_dm_adv_audio_close(RawAddress p_bd_addr);
extern tBTA_DEV_PAIRING_CB* bta_get_lea_pair_cb(RawAddress peer_addr);
extern tBTA_DEV_PAIRING_CB* bta_set_lea_pair_cb(RawAddress peer_addr);
extern void bta_dm_reset_lea_pairing_info(RawAddress p_addr);
extern void bta_dm_ble_adv_audio_idaddr_map(RawAddress p_bd_addr,
            RawAddress p_id_addr);
extern bool bta_remote_dev_identity_addr_match(RawAddress p_addr);
extern bool bta_lea_is_le_pairing();
extern bool bta_remote_device_is_dumo(RawAddress p_bd_addr);
extern RawAddress bta_get_rem_dev_id_addr(RawAddress p_bd_addr);
extern void bta_adv_audio_update_bond_db(RawAddress p_bd_addr, uint8_t transport);
extern int bta_is_adv_audio_valid_bdaddr(RawAddress p_bd_addr);
extern void bta_find_adv_audio_group_instance(uint16_t conn_id, tGATT_STATUS status,
    RawAddress p_addr);
extern bool bta_is_remote_support_lea(RawAddress p_addr);
extern bool is_gatt_srvc_disc_pending(RawAddress rem_bda);
extern RawAddress bta_get_pseudo_addr_with_id_addr(RawAddress p_addr);
extern void bta_dm_adv_audio_init();
extern void bta_dm_adv_audio_deinit();
extern void bta_le_audio_service_search_failed(void* data);
extern void bta_dm_update_adv_audio_db(const RawAddress& bd_addr);
#endif /* BTA_DM_ADV_AUDIO_H*/
