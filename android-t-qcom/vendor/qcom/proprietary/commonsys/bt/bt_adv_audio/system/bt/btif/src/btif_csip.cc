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

/*******************************************************************************
 *
 *  Filename:      btif_csip.c
 *
 *  Description:   CSIP client implementation (Set Coordinator)
 *
 ******************************************************************************/

#define LOG_TAG "bt_btif_csip"

#include <base/at_exit.h>
#include <base/bind.h>
#include <base/threading/thread.h>
#include <bluetooth/uuid.h>
#include <errno.h>
#include <hardware/bluetooth.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "device/include/controller.h"
#include "bta_csip_api.h"
#include "btm_ble_api.h"
#include "btif_api.h"
#include "btif_common.h"
#include "btif_util.h"

#include <hardware/bt_csip.h>

using base::Bind;
using bluetooth::Uuid;

static btcsip_callbacks_t* bt_csip_callbacks = NULL;

static std::vector<RawAddress> vector_addr;

void btif_new_set_found_cb(tBTA_CSIP_NEW_SET_FOUND params) {
  HAL_CBACK(bt_csip_callbacks, new_set_found_cb, params.set_id, params.addr,
            params.size, params.sirk, params.including_srvc_uuid,
            params.lock_support);
}

void btif_conn_state_changed_cb(tBTA_CSIP_CONN_STATE_CHANGED params) {
  HAL_CBACK(bt_csip_callbacks, conn_state_cb, params.app_id, params.addr,
            params.state, params.status);
}

void btif_new_set_member_found_cb(tBTA_SET_MEMBER_FOUND params) {
  HAL_CBACK(bt_csip_callbacks, new_set_member_cb, params.set_id, params.addr);
}

void btif_lock_status_changed_cb(tBTA_LOCK_STATUS_CHANGED params) {
  HAL_CBACK(bt_csip_callbacks, lock_status_cb, params.app_id, params.set_id,
            params.value, params.status, params.addr);
}

void btif_lock_available_cb(tBTA_LOCK_AVAILABLE params) {
  HAL_CBACK(bt_csip_callbacks, lock_available_cb, params.app_id, params.set_id,
            params.addr);
}

void btif_set_size_changed_cb (tBTA_CSIP_SET_SIZE_CHANGED params) {
  HAL_CBACK(bt_csip_callbacks, size_changed_cb, params.set_id, params.size,
            params.addr);
}

void btif_set_sirk_changed_cb (tBTA_CSIP_SET_SIRK_CHANGED params) {
  HAL_CBACK(bt_csip_callbacks, sirk_changed_cb, params.set_id, params.sirk,
            params.addr);
}

void btif_rsi_data_found_cb (tBTA_CSIP_RSI_DATA_FOUND params) {
  HAL_CBACK(bt_csip_callbacks, rsi_data_found_cb, params.rsi,
            params.addr);
}

void btif_csip_opportunistic_result_cb (const RawAddress &bda, uint8_t* data) {
  std::vector<RawAddress>::iterator it;
  it = std::find(vector_addr.begin(), vector_addr.end(), bda);
  if (it != vector_addr.end()) {
    return;
  }
  vector_addr.push_back(bda);
  tBTA_CSIP_RSI_DATA_FOUND rsi_param = {bda, {}};
  memcpy(rsi_param.rsi, data, 6);
  do_in_jni_thread(Bind(btif_rsi_data_found_cb, rsi_param));
}

void  btif_csip_set_opportunistic_scan(bool is_start) {
  vector_addr.clear();
  BTIF_TRACE_DEBUG("%s  %d ", __func__, is_start);
  tBTM_CSIP_OPPORTUNISTIC_SCAN_CB * pcb = (tBTM_CSIP_OPPORTUNISTIC_SCAN_CB*)
          &btif_csip_opportunistic_result_cb;
  BTM_BleEnableCsipOpportunisticScan(is_start, pcb);
}

const char* btif_csip_get_event_name(tBTA_CSIP_EVT event) {
  switch(event) {
    case BTA_CSIP_LOCK_STATUS_CHANGED_EVT:
      return "BTA_CSIP_LOCK_STATUS_CHANGED_EVT";
    case BTA_CSIP_SET_MEMBER_FOUND_EVT:
      return "BTA_CSIP_SET_MEMBER_FOUND_EVT";
    case BTA_CSIP_LOCK_AVAILABLE_EVT:
      return "BTA_CSIP_LOCK_AVAILABLE_EVT";
    case BTA_CSIP_NEW_SET_FOUND_EVT:
      return "BTA_CSIP_NEW_SET_FOUND_EVT";
    case BTA_CSIP_CONN_STATE_CHG_EVT:
      return "BTA_CSIP_CONN_STATE_CHG_EVT";
    case BTA_CSIP_SET_SIZE_CHANGED:
      return "BTA_CSIP_SET_SIZE_CHANGED";
    case BTA_CSIP_SET_SIRK_CHANGED:
      return "BTA_CSIP_SET_SIRK_CHANGED";
    default:
      return "UNKNOWN_EVENT";
  }
}

void btif_csip_evt (tBTA_CSIP_EVT event, tBTA_CSIP_DATA* p_data) {
  BTIF_TRACE_EVENT("%s: Event = %02x (%s)", __func__, event, btif_csip_get_event_name(event));

  switch (event) {
    case BTA_CSIP_LOCK_STATUS_CHANGED_EVT: {
        tBTA_LOCK_STATUS_CHANGED lock_status_params = p_data->lock_status_param;
        do_in_jni_thread(Bind(btif_lock_status_changed_cb, lock_status_params));
      }
      break;

    case BTA_CSIP_LOCK_AVAILABLE_EVT: {
        tBTA_LOCK_AVAILABLE lock_avl_param = p_data->lock_available_param;
        do_in_jni_thread(Bind(btif_lock_available_cb, lock_avl_param));
      }
      break;

    case BTA_CSIP_NEW_SET_FOUND_EVT: {
        tBTA_CSIP_NEW_SET_FOUND new_set_params = p_data->new_set_params;
        memcpy(new_set_params.sirk, p_data->new_set_params.sirk, SIRK_SIZE);
        do_in_jni_thread(Bind(btif_new_set_found_cb, new_set_params));
      }
      break;

    case BTA_CSIP_SET_MEMBER_FOUND_EVT: {
        tBTA_SET_MEMBER_FOUND new_member_params = p_data->set_member_param;
        do_in_jni_thread(Bind(btif_new_set_member_found_cb, new_member_params));
      }
      break;

    case BTA_CSIP_CONN_STATE_CHG_EVT: {
        tBTA_CSIP_CONN_STATE_CHANGED conn_params = p_data->conn_params;
        do_in_jni_thread(Bind(btif_conn_state_changed_cb, conn_params));
      }
      break;

   case BTA_CSIP_SET_SIZE_CHANGED: {
        tBTA_CSIP_SET_SIZE_CHANGED size_chg_param = p_data->size_chg_params;
        do_in_jni_thread(Bind(btif_set_size_changed_cb, size_chg_param));
      }
      break;

   case BTA_CSIP_SET_SIRK_CHANGED: {
          tBTA_CSIP_SET_SIRK_CHANGED sirk_chg_param = p_data->sirk_chg_params;
          do_in_jni_thread(Bind(btif_set_sirk_changed_cb, sirk_chg_param));
      }
      break;

    default:
      BTIF_TRACE_ERROR("%s: Unknown event %d", __func__, event);
  }
}

/* Initialization of CSIP module on BT ON*/
bt_status_t btif_csip_init( btcsip_callbacks_t* callbacks ) {
  bt_csip_callbacks = callbacks;

  do_in_jni_thread(Bind(BTA_CsipEnable, btif_csip_evt));
  btif_register_uuid_srvc_disc(Uuid::FromString("1846"));

  return BT_STATUS_SUCCESS;
}

/* Connect call from upper layer for GATT Connecttion to a given Set Member */
bt_status_t btif_csip_connect (uint8_t app_id, RawAddress *bd_addr) {
  BTIF_TRACE_EVENT("%s: Address: %s", __func__, bd_addr->ToString().c_str());

  do_in_jni_thread(Bind(BTA_CsipConnect, app_id, *bd_addr));

  return BT_STATUS_SUCCESS;
}

/* Call from upper layer to disconnect GATT Connection for given Set Member */
bt_status_t btif_csip_disconnect (uint8_t app_id, RawAddress *bd_addr ) {
  BTIF_TRACE_EVENT("%s", __func__);

  do_in_jni_thread(Bind(BTA_CsipDisconnect, app_id, *bd_addr));

  return BT_STATUS_SUCCESS;
}

/** register app/module with CSIP profile */
bt_status_t btif_csip_app_register (const bluetooth::Uuid& uuid) {
  BTIF_TRACE_EVENT("%s", __func__);
  return do_in_jni_thread(Bind(
    [](const Uuid& uuid) {
      BTA_RegisterCsipApp(
          btif_csip_evt,
          base::Bind(
              [](const Uuid& uuid, uint8_t status, uint8_t app_id) {
                do_in_jni_thread(Bind(
                    [](const Uuid& uuid, uint8_t status, uint8_t app_id) {
                      HAL_CBACK(bt_csip_callbacks, app_registered_cb,
                                status, app_id, uuid);
                    },
                    uuid, status, app_id));
              },
              uuid));
    }, uuid));
}

/** unregister csip App/Module */
bt_status_t btif_csip_app_unregister (uint8_t app_id) {
  BTIF_TRACE_EVENT("%s", __func__);
  return do_in_jni_thread(Bind(BTA_UnregisterCsipApp, app_id));
}

/** change lock value */
bt_status_t btif_csip_set_lock_value (uint8_t app_id, uint8_t set_id, uint8_t lock_value,
                                             std::vector<RawAddress> devices) {
  BTIF_TRACE_EVENT("%s appId = %d setId = %d Lock Value = %02x ", __func__,
                    app_id, set_id, lock_value);
  tBTA_SET_LOCK_PARAMS lock_params = {app_id, set_id, lock_value, devices};
  do_in_jni_thread(Bind(BTA_CsipSetLockValue, lock_params));
  return BT_STATUS_SUCCESS;
}

void  btif_csip_cleanup() {
  BTIF_TRACE_EVENT("%s", __func__);
  do_in_jni_thread(Bind(BTA_CsipDisable));
}

const btcsip_interface_t btcsipInterface = {
    sizeof(btcsipInterface),
    btif_csip_init,
    btif_csip_connect,
    btif_csip_disconnect,
    btif_csip_app_register,
    btif_csip_app_unregister,
    btif_csip_set_lock_value,
    btif_csip_set_opportunistic_scan,
    btif_csip_cleanup,
};

/*******************************************************************************
 *
 * Function         btif_csip_get_interface
 *
 * Description      Get the csip callback interface
 *
 * Returns          btcsip_interface_t
 *
 ******************************************************************************/
const btcsip_interface_t* btif_csip_get_interface() {
  BTIF_TRACE_EVENT("%s", __func__);
  return &btcsipInterface;
}
