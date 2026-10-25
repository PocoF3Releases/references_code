/*
 * Copyright (c) 2021-2022 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
*/
/******************************************************************************
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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

#include "bta_api.h"
#include "bt_target.h"
#include "bta_sys.h"
#include "bta_tmap_api.h"
#include "gatts_ops_queue.h"
#include "btif_util.h"

#include <base/bind.h>
#include <base/callback.h>
#include <base/strings/string_number_conversions.h>
#include <vector>
#include <string.h>

using bluetooth::Uuid;
using bluetooth::bap::GattsOpsQueue;
tmasServiceInfo_t tmasServiceInfo;

class TmapImpl;
static TmapImpl *instance;

#define     CALL_GATEWAY_ROLE           1
#define     CALL_TERMINAL_ROLE          2
#define     UNICAST_MEDIA_SENDER        4
#define     UNICAST_MEDIA_RECEIVER      8
#define     BROADCAST_MEDIA_SENDER     16
#define     BROADCAST_MEDIA_RECEIVER   32

Uuid TMAS_UUID  = Uuid::FromString("1855");
Uuid TMAP_ROLE_UUID   = Uuid::FromString("2B51");
Uuid TMAP_DESCRIPTOR_UUID = Uuid::FromString("2902");

#define TMAP_INIT_EVENT            199
#define TMAP_CLEANUP_EVENT         200

void HandleTmapEvent(uint32_t event, void* param);

static void OnTmapServiceAddedCb(uint8_t status, int serverIf,
                      std::vector<btgatt_db_element_t> service);

const char* get_tmap_event_name(uint32_t event) {
   switch (event) {
      CASE_RETURN_STR(TMAP_INIT_EVENT)
      CASE_RETURN_STR(TMAP_CLEANUP_EVENT)

      // Gatt callback events
      CASE_RETURN_STR(BTA_GATTS_REG_EVT)
      CASE_RETURN_STR(BTA_GATTS_DEREG_EVT)
      CASE_RETURN_STR(BTA_GATTS_CONF_EVT)
      CASE_RETURN_STR(BTA_GATTS_CONGEST_EVT)
      CASE_RETURN_STR(BTA_GATTS_MTU_EVT)
      CASE_RETURN_STR(BTA_GATTS_CONNECT_EVT)
      CASE_RETURN_STR(BTA_GATTS_DISCONNECT_EVT)
      CASE_RETURN_STR(BTA_GATTS_CLOSE_EVT)
      CASE_RETURN_STR(BTA_GATTS_STOP_EVT)
      CASE_RETURN_STR(BTA_GATTS_DELELTE_EVT)
      CASE_RETURN_STR(BTA_GATTS_READ_CHARACTERISTIC_EVT)
      CASE_RETURN_STR(BTA_GATTS_READ_DESCRIPTOR_EVT)
      CASE_RETURN_STR(BTA_GATTS_WRITE_CHARACTERISTIC_EVT)
      CASE_RETURN_STR(BTA_GATTS_WRITE_DESCRIPTOR_EVT)
      CASE_RETURN_STR(BTA_GATTS_EXEC_WRITE_EVT)
      CASE_RETURN_STR(BTA_GATTS_CONN_UPDATE_EVT)
      CASE_RETURN_STR(BTA_GATTS_PHY_UPDATE_EVT)

      default:
        return ": Unknown Event";
      }
}


class TmapImpl : public TmapServer {
  Uuid app_uuid;

  public:
     virtual ~TmapImpl() = default;


  TmapImpl(Uuid uuid)
     :app_uuid(uuid) {
    LOG(INFO) << ": TmapImpl gatts app register";
    HandleTmapEvent(TMAP_INIT_EVENT, &app_uuid);
  }
};

  void TmapServer::CleanUp() {
     HandleTmapEvent(TMAP_CLEANUP_EVENT, NULL);
     delete instance;
     instance = nullptr;
  }

  TmapServer* TmapServer::Get() {
    CHECK(instance);
    return instance;
  }

  void  TmapServer::Initialize( Uuid uuid) {
     LOG(INFO) << __func__ << ": initialize is called";
    if (instance) {
    LOG(ERROR) << ": Already initialized!";
    } else {
       instance = new TmapImpl(uuid);
    }
  }

static void OnTmapServiceAddedCb(uint8_t status, int serverIf,
                                std::vector<btgatt_db_element_t> service) {

  LOG(INFO) << __func__ << ": on tmap service added cb";
  if (service[0].uuid == Uuid::From16Bit(UUID_SERVCLASS_GATT_SERVER) ||
      service[0].uuid == Uuid::From16Bit(UUID_SERVCLASS_GAP_SERVER)) {
    LOG(INFO) << "%s: Attempt to register restricted service"<< __func__;
    return;
  }

  for(int i = 0; i < (int)service.size(); i++) {

    if (service[i].uuid == TMAS_UUID) {
      LOG(INFO) << __func__ << ": tmas service added attr handle:  " << service[i].attribute_handle;
    } else if (service[i].uuid == TMAP_ROLE_UUID) {
      tmasServiceInfo.tmap_role_handle = service[i++].attribute_handle;
      tmasServiceInfo.tmap_role_desc = service[i].attribute_handle;
      LOG(INFO) << __func__ << ": tmap role handle: " <<
      tmasServiceInfo.tmap_role_handle;
      LOG(INFO) << __func__ << ": tmap role handle desc: " <<
      tmasServiceInfo.tmap_role_desc;
   }
  }
}

static std::vector<btgatt_db_element_t> TmapAddService(int server_if) {

  LOG(INFO) << __func__ << ": tmap add service";

  std::vector<btgatt_db_element_t> tmas_services;
  tmas_services.clear();
  //service
  btgatt_db_element_t service = {.uuid = TMAS_UUID, .type = BTGATT_DB_PRIMARY_SERVICE, 0};
  tmas_services.push_back(service);
  tmasServiceInfo.tmas_service_uuid = service.uuid;

  //TMAP_ROLE_UUID
  btgatt_db_element_t tmas_tmap_role_char = {.uuid = TMAP_ROLE_UUID,
                                              .type = BTGATT_DB_CHARACTERISTIC,
                                              .properties = GATT_CHAR_PROP_BIT_READ,
                                              .permissions = GATT_PERM_READ};
  tmas_services.push_back(tmas_tmap_role_char);
  tmasServiceInfo.tmap_role_uuid = tmas_tmap_role_char.uuid;

  //1st desc
  btgatt_db_element_t desc1 = {.uuid = TMAP_DESCRIPTOR_UUID,
                               .type = BTGATT_DB_DESCRIPTOR,
                               .permissions = GATT_PERM_READ};
  tmas_services.push_back(desc1);
  return tmas_services;
}


void BTTmapCback(tBTA_GATTS_EVT event, tBTA_GATTS* param) {
   LOG(INFO) << __func__ << ": bttmap cback";
   HandleTmapEvent((uint32_t)event, param);
}

void HandleTmapEvent(uint32_t event, void* param) {
  LOG(INFO) << __func__ << ": Tmap handle event: " << get_tmap_event_name(event);

  tBTA_GATTS* p_data = NULL;
  tGATTS_RSP rsp_struct;
  switch (event) {
    case TMAP_INIT_EVENT: {
      Uuid *app_uuid = (Uuid*) param;
      //Add APP register Code here with callback
      if(app_uuid != NULL)
        BTA_GATTS_AppRegister(*app_uuid, BTTmapCback, true);
      break;
    }

    case BTA_GATTS_REG_EVT: {
       p_data = (tBTA_GATTS*)param;
       if (p_data->reg_oper.status == BT_STATUS_SUCCESS) {
           tmasServiceInfo.server_if = p_data->reg_oper.server_if;
         std::vector<btgatt_db_element_t> service;
         service = TmapAddService(tmasServiceInfo.server_if);
         if (service[0].uuid == Uuid::From16Bit(UUID_SERVCLASS_GATT_SERVER) ||
                 service[0].uuid == Uuid::From16Bit(UUID_SERVCLASS_GAP_SERVER)) {
           LOG(INFO) << __func__ << ": service app register uuid is not valid";
           break;
         }
         LOG(INFO) << __func__ << ": service app register";
         BTA_GATTS_AddService(tmasServiceInfo.server_if, service,
                              base::Bind(&OnTmapServiceAddedCb));
       }
       break;
    }

    case BTA_GATTS_READ_CHARACTERISTIC_EVT: {
      if(!instance) {
        LOG(INFO) << __func__ << ": instance is NULL ";
        break;
      }
      p_data = (tBTA_GATTS*)param;
      std::vector<uint8_t> value;
      LOG(INFO) << __func__ << ": charateristcs read handle: " <<
          p_data->req_data.p_data->read_req.handle <<", trans_id : " <<
          p_data->req_data.trans_id;

      LOG(INFO) <<": offset: " << p_data->req_data.p_data->read_req.offset <<
          " , long : " << p_data->req_data.p_data->read_req.is_long;

      rsp_struct.attr_value.auth_req  = 0;
      rsp_struct.attr_value.handle = p_data->req_data.p_data->read_req.handle;
      rsp_struct.attr_value.offset = p_data->req_data.p_data->read_req.offset;
      rsp_struct.attr_value.len = sizeof(uint16_t);
      //KW: Array '&tmap_role' of size 1 may use index value(s) 1
      uint16_t tmap_role =
           (uint16_t)(CALL_GATEWAY_ROLE | UNICAST_MEDIA_SENDER | BROADCAST_MEDIA_SENDER);
      rsp_struct.attr_value.value[1] = (uint8_t)(tmap_role & 0xFF);
      rsp_struct.attr_value.value[0] = (uint8_t)((tmap_role>>8) & 0xFF);

      if(p_data->req_data.p_data->read_req.handle ==
          tmasServiceInfo.tmap_role_handle) {
        LOG(INFO) << __func__ << ": TMAP_ROLE read";
      }
        //Directly post to gatts operations queue.
      GattsOpsQueue::SendResponse(p_data->req_data.conn_id, p_data->req_data.trans_id,
           BT_STATUS_SUCCESS, &rsp_struct);
      break;
    }

    case TMAP_CLEANUP_EVENT: {
       //unregister app
       BTA_GATTS_AppDeregister(tmasServiceInfo.server_if);
       break;
    }

    default:
      LOG(INFO) << __func__ << ": event not matched !!";
      break;
  }
}

