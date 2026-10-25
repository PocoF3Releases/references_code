/*
*
* Copyright 2018-2020 NXP.
*
* NXP Confidential. This software is owned or controlled by NXP and may only be
* used strictly in accordance with the applicable license terms. By expressly
* accepting such terms or by downloading,installing, activating and/or otherwise
* using the software, you are agreeing that you have read,and that you agree to
* comply with and are bound by, such license terms. If you do not agree to be
* bound by the applicable license terms, then you may not retain, install, activate
* or otherwise use the software.
*
*/
#include <string.h>
#include <stdlib.h>
#include "uwb_gki.h"
#include "uwb_target.h"
#include "uwa_sys.h"
#include "uwb_hal_api.h"
#include "uwb_hal_int.h"
#include "uwb_api.h"
#include "uwb_int.h"
#include "uci_hmsgs.h"
#include "uwa_dm_int.h"
#include "uwb_osal_common.h"
#include "uci_log.h"

/****************************************************************************
** Declarations
****************************************************************************/
tUWB_CB uwb_cb;

/*******************************************************************************
**
** Function         uwb_state_name
**
** Description      This function returns the state name.
**
** NOTE             conditionally compiled to save memory.
**
** Returns          pointer to the name
**
*******************************************************************************/
std::string uwb_state_name(uint8_t state) {
  switch (state) {
    case UWB_STATE_NONE:
      return "NONE";
    case UWB_STATE_W4_HAL_OPEN:
      return "W4_HAL_OPEN";
    case UWB_STATE_IDLE:
      return "IDLE";
    case UWB_STATE_ACTIVE:
      return "ACTIVE";
    case UWB_STATE_CLOSING:
      return "CLOSING";
    case UWB_STATE_W4_HAL_CLOSE:
      return "W4_HAL_CLOSE";
    default:
      return "???? UNKNOWN STATE";
  }
}

/*******************************************************************************
**
** Function         uwb_hal_event_name
**
** Description      This function returns the HAL event name.
**
** NOTE             conditionally compiled to save memory.
**
** Returns          pointer to the name
**
*******************************************************************************/

std::string uwb_hal_event_name(uint8_t event) {
  switch (event) {
    case HAL_UWB_OPEN_CPLT_EVT:
      return "HAL_UWB_OPEN_CPLT_EVT";

    case HAL_UWB_CLOSE_CPLT_EVT:
      return "HAL_UWB_CLOSE_CPLT_EVT";

    case HAL_UWB_ERROR_EVT:
      return "HAL_UWB_ERROR_EVT";

    default:
      return "???? UNKNOWN EVENT";
  }
}

/*******************************************************************************
**
** Function         uwb_main_notify_enable_status
**
** Description      Notify status of Enable/PowerOffSleep/PowerCycle
**
*******************************************************************************/
static void uwb_main_notify_enable_status(tUWB_STATUS uwb_status) {
  tUWB_RESPONSE evt_data;

  evt_data.status = uwb_status;

  if (uwb_cb.p_resp_cback) {
    (*uwb_cb.p_resp_cback)(UWB_ENABLE_REVT, &evt_data);
  }
}

/*******************************************************************************
**
** Function         uwb_enabled
**
** Description      UWBC enabled, proceed with stack start up.
**
** Returns          void
**
*******************************************************************************/
void uwb_enabled(tUWB_STATUS uwb_status,
                 __attribute__((unused)) UWB_HDR* p_init_rsp_msg) {
  tUWB_RESPONSE evt_data;

  memset(&evt_data, 0, sizeof(tUWB_RESPONSE));

  if (uwb_status == UCI_STATUS_OK) {
    uwb_set_state(UWB_STATE_IDLE);
  }
  uwb_main_notify_enable_status(uwb_status);
}

/*******************************************************************************
**
** Function         uwb_set_state
**
** Description      Set the state of UWB stack
**
** Returns          void
**
*******************************************************************************/
void uwb_set_state(tUWB_STATE uwb_state) {
    UCI_TRACE_I("uwb_set_state %d (%s)->%d (%s)", uwb_cb.uwb_state,
                      uwb_state_name(uwb_cb.uwb_state).c_str(), uwb_state,
                      uwb_state_name(uwb_state).c_str());
  uwb_cb.uwb_state = uwb_state;
}

/*******************************************************************************
**
** Function         uwb_gen_cleanup
**
** Description      Clean up for both going into low power mode and disabling
**                  UWB
**
*******************************************************************************/
void uwb_gen_cleanup(void) {
  /* clear any pending CMD/RSP */
  uwb_main_flush_cmd_queue();
  uwb_main_flush_data_queue();
}

/*******************************************************************************
**
** Function         uwb_main_handle_hal_evt
**
** Description      Handle BT_EVT_TO_UWB_MSGS
**
*******************************************************************************/
void uwb_main_handle_hal_evt(tUWB_HAL_EVT_MSG* p_msg) {
  // uint8_t* ps;

  UCI_TRACE_I("HAL event=0x%x", p_msg->hal_evt);

  switch (p_msg->hal_evt) {
    case HAL_UWB_OPEN_CPLT_EVT: /* only for failure case */
      uwb_enabled(UWB_STATUS_FAILED, NULL);
      break;

    case HAL_UWB_CLOSE_CPLT_EVT:
      if (uwb_cb.p_resp_cback) {
        if (uwb_cb.uwb_state == UWB_STATE_W4_HAL_CLOSE) {
          uwb_set_state(UWB_STATE_NONE);
          (*uwb_cb.p_resp_cback)(UWB_DISABLE_REVT, NULL);
          uwb_cb.p_resp_cback = NULL;
        } else {
          /* found error during initialization */
          uwb_set_state(UWB_STATE_NONE);
          uwb_main_notify_enable_status(UWB_STATUS_FAILED);
        }
      }
      break;

    case HAL_UWB_ERROR_EVT:
      switch (p_msg->status) {
        case HAL_UWB_STATUS_ERR_TRANSPORT:
          /* Notify app of transport error */
          if (uwb_cb.p_resp_cback) {
            (*uwb_cb.p_resp_cback)(UWB_UWBD_TRANSPORT_ERR_REVT, NULL);

            /* if enabling UWB, notify upper layer of failure after closing HAL
             */
            if (uwb_cb.uwb_state < UWB_STATE_IDLE) {
              uwb_enabled(UWB_STATUS_FAILED, NULL);
            }
          }
          break;
        default:
          break;
      }
      break;
    default:
      UCI_TRACE_E("unhandled event (0x%x).", p_msg->hal_evt);
      break;
  }
}

/*******************************************************************************
**
** Function         uwb_main_flush_cmd_queue
**
** Description      This function is called when setting power off sleep state.
**
** Returns          void
**
*******************************************************************************/
void uwb_main_flush_cmd_queue(void) {
  UWB_HDR* p_msg;

  UCI_TRACE_I(__func__);

  /* initialize command window */
  uwb_cb.uci_cmd_window = UCI_MAX_CMD_WINDOW;

  /* Stop command-pending timer */
  uwb_stop_quick_timer(&uwb_cb.uci_wait_rsp_timer);
  uwb_cb.is_resp_pending = false;
  uwb_cb.cmd_retry_count = 0;

  /* dequeue and free buffer */
  while ((p_msg = (UWB_HDR*)phUwb_GKI_dequeue(&uwb_cb.uci_cmd_xmit_q)) != NULL) {
    phUwb_GKI_freebuf(p_msg);
  }
}

/*******************************************************************************
**
** Function         uwb_main_flush_data_queue
**
** Description      This function is called when setting power off sleep state.
**
** Returns          void
**
*******************************************************************************/
void uwb_main_flush_data_queue(void) {
  UWB_HDR* p_msg;

  UCI_TRACE_I(__func__);

  /* Stop command-pending timer */
  uwb_stop_quick_timer(&uwb_cb.uci_wait_credit_ntf_timer);
  uwb_cb.is_credit_ntf_pending = false;
  uwb_cb.data_pkt_retry_count = 0;

  /* dequeue and free buffer */
  while ((p_msg = (UWB_HDR*)phUwb_GKI_dequeue(&uwb_cb.tx_data_pkt_q)) != NULL) {
    phUwb_GKI_freebuf(p_msg);
  }
}
/*******************************************************************************
**
** Function         uwb_main_post_hal_evt
**
** Description      This function posts HAL event to UWB_TASK
**
** Returns          void
**
*******************************************************************************/
void uwb_main_post_hal_evt(uint8_t hal_evt, tUWB_STATUS status) {
  tUWB_HAL_EVT_MSG* p_msg;

  p_msg = (tUWB_HAL_EVT_MSG*)phUwb_GKI_getbuf(sizeof(tUWB_HAL_EVT_MSG));
  if (p_msg != NULL) {
    /* Initialize UWB_HDR */
    p_msg->hdr.len = 0;
    p_msg->hdr.event = BT_EVT_TO_UWB_MSGS;
    p_msg->hdr.offset = 0;
    p_msg->hdr.layer_specific = 0;
    p_msg->hal_evt = hal_evt;
    p_msg->status = status;
    phUwb_GKI_send_msg(UWB_TASK, UWB_MBOX_ID, p_msg);
  } else {
    UCI_TRACE_E("No buffer");
  }
}

/*******************************************************************************
**
** Function         uwb_main_hal_cback
**
** Description      HAL event handler
**
** Returns          void
**
*******************************************************************************/
static void uwb_main_hal_cback(uint8_t event, tUWB_STATUS status) {
  UCI_TRACE_I("uwb_main_hal_cback event: %s(0x%x), status=%d",
                      uwb_hal_event_name(event).c_str(), event, status);
  switch (event) {
    case HAL_UWB_OPEN_CPLT_EVT:
      /*
      ** if UWB_Disable() is called before receiving HAL_UWB_OPEN_CPLT_EVT,
      ** then wait for HAL_UWB_CLOSE_CPLT_EVT.
      */
      if (uwb_cb.uwb_state == UWB_STATE_W4_HAL_OPEN) {
        if (status == HAL_UWB_STATUS_OK) {
          /* Notify UWB_TASK that UCI tranport is initialized */
          phUwb_GKI_send_event(UWB_TASK, UWB_TASK_EVT_TRANSPORT_READY);
        } else {
          uwb_main_post_hal_evt(event, status);
        }
      }
      break;

    case HAL_UWB_CLOSE_CPLT_EVT:
    case HAL_UWB_ERROR_EVT:
      uwb_main_post_hal_evt(event, status);
      break;

    default:
      UCI_TRACE_E("uwb_main_hal_cback unhandled event %x", event);
      break;
  }
}

/*******************************************************************************
**
** Function         uwb_main_hal_data_cback
**
** Description      HAL data event handler
**
** Returns          void
**
*******************************************************************************/
static void uwb_main_hal_data_cback(uint16_t data_len, uint8_t* p_data) {
  UWB_HDR* p_msg;

  /* ignore all data while shutting down Helio */
  if (uwb_cb.uwb_state == UWB_STATE_W4_HAL_CLOSE ||
      uwb_cb.uwb_state == UWB_STATE_W4_HAL_OPEN) {
    return;
  }
  if (p_data) {
    p_msg = (UWB_HDR*)phUwb_GKI_getpoolbuf(UWB_UCI_POOL_ID);
    if (p_msg != NULL) {
      /* Initialize UWB_HDR */
      p_msg->len = data_len;
      p_msg->event = BT_EVT_TO_UWB_UCI;
      p_msg->offset = UWB_RECEIVE_MSGS_OFFSET;
#if (NXP_UWB_DEBUG_LOG == TRUE)
      if(data_len >= 5) {
        UCI_TRACE_D("uwb_main_hal_data_cback Response : 0x%x 0x%x 0x%x 0x%x 0x%x",
          p_data[0],p_data[1],p_data[2],p_data[3],p_data[4]);
      }
#endif
      /* no need to check length, it always less than pool size */
      memcpy((uint8_t*)(p_msg + 1) + p_msg->offset, p_data, p_msg->len);

      phUwb_GKI_send_msg(UWB_TASK, UWB_MBOX_ID, p_msg);
    } else {
      UCI_TRACE_E("No buffer");
    }
  }
}

/*******************************************************************************
**
** Function         UWB_Enable
**
** Description      This function enables UWB. Prior to calling UWB_Enable:
**                  - the UWBC must be powered up, and ready to receive
**                    commands.
**                  - GKI must be enabled
**                  - UWB_TASK must be started
**                  - UCIT_TASK must be started (if using dedicated UCI
**                    transport)
**
**                  This function opens the UCI transport (if applicable),
**                  resets the UWB controller, and initializes the UWB
**                  subsystems.
**
**                  When the UWB startup procedure is completed, an
**                  UWB_ENABLE_REVT is returned to the application using the
**                  tUWB_RESPONSE_CBACK.
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_Enable(tUWB_RESPONSE_CBACK* p_cback, tUWB_TEST_RESPONSE_CBACK* p_test_cback) {
  UCI_TRACE_I(__func__);
  /* Validate callback */
  if (!p_cback) {
    return (UWB_STATUS_INVALID_PARAM);
  }
  uwb_set_state(UWB_STATE_W4_HAL_OPEN);
  uwb_cb.p_resp_cback = p_cback;
  uwb_cb.p_test_resp_cback = p_test_cback;
  uwb_cb.p_hal->open(uwb_main_hal_cback, uwb_main_hal_data_cback);
  return (UWB_STATUS_OK);
}

/*******************************************************************************
**
** Function         UWB_RegisterExtCallBack
**
** Description      This function should be called only once during initialization
**                  all proprietary events are routed throught this callback.
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_RegisterExtCallBack(tUWB_RAW_CBACK* p_ext_cback) {
  UCI_TRACE_I(__func__);
  /* Validate callback */
  if (!p_ext_cback) {
    return (UWB_STATUS_INVALID_PARAM);
  }
  uwb_cb.p_ext_resp_cback = p_ext_cback;
  return (UWB_STATUS_OK);
}

/*******************************************************************************
**
** Function         UWB_Disable
**
** Description      This function performs clean up routines for shutting down
**                  UWB and closes the UCI transport (if using dedicated UCI
**                  transport).
**
**                  When the UWB shutdown procedure is completed, an
**                  UWB_DISABLED_REVT is returned to the application using the
**                  tUWB_RESPONSE_CBACK.
**
** Returns          nothing
**
*******************************************************************************/
void UWB_Disable(void) {
  UCI_TRACE_I("uwb_state = %d", uwb_cb.uwb_state);

  if (uwb_cb.uwb_state == UWB_STATE_NONE) {
    uwb_set_state(UWB_STATE_NONE);
    if (uwb_cb.p_resp_cback) {
      (*uwb_cb.p_resp_cback)(UWB_DISABLE_REVT, NULL);
      uwb_cb.p_resp_cback = NULL;
      uwb_cb.p_test_resp_cback = NULL;
    }
    return;
  }

  /* Close transport and clean up */
  uwb_task_shutdown_uwbc();
}

/*******************************************************************************
**
** Function         UWB_Init
**
** Description      This function initializes control block for UWB
**
** Returns          nothing
**
*******************************************************************************/
void UWB_Init(tHAL_UWB_CONTEXT* p_hal_entry_cntxt) {
  /* Clear uwb control block */
  memset(&uwb_cb, 0, sizeof(tUWB_CB));
  uwb_cb.p_hal = p_hal_entry_cntxt->hal_entry_func;
  uwb_cb.uwb_state = UWB_STATE_NONE;
  uwb_cb.uci_cmd_window = UCI_MAX_CMD_WINDOW;
  uwb_cb.retry_rsp_timeout = ((UWB_CMD_RETRY_TIMEOUT * QUICK_TIMER_TICKS_PER_SEC) / 1000);
  uwb_cb.uci_wait_rsp_tout = ((UWB_CMD_CMPL_TIMEOUT * QUICK_TIMER_TICKS_PER_SEC) / 1000);
  uwb_cb.pLast_cmd_buf = NULL;
  uwb_cb.pLast_data_buf = NULL;
  uwb_cb.is_resp_pending = false;
  uwb_cb.is_credit_ntf_pending = false;
  uwb_cb.cmd_retry_count = 0;
  uwb_cb.data_pkt_retry_count = 0;
  uwb_cb.is_recovery_in_progress = false;
  uwb_cb.uci_credit_ntf_timeout = ((UWB_CMD_CMPL_TIMEOUT * QUICK_TIMER_TICKS_PER_SEC) / 1000); // currently used same timeout value as cmd timeout
  uwb_cb.data_credits = 1;
  uwb_cb.IsConformaceTestEnabled = false;
  uwb_cb.is_first_frgmnt_done = false;
  phUwb_GKI_init_q(&uwb_cb.tx_data_pkt_q);
}

/*******************************************************************************
**
** Function         UWB_GetDeviceInfo
**
** Description      This function is called to send the get UWB device info.
**                  The response from UWBC is reported to the given
**                  tUWB_RESPONSE_CBACK.
**
** Parameters       Nill
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_GetDeviceInfo() {
  return uci_snd_get_device_info_cmd();
}

/*******************************************************************************
**
** Function         UWB_DeviceResetCommand
**
** Description      This function is called to send Device Reset Command.
**                       The response from UWBC is reported by
**                       tUWB_RESPONSE_CBACK as UWB_DEVICE_RESET_REVT.
**
** Parameters       resetConfig - Vendor Specific Reset Config to be sent
**
** Returns          tUWB_STATUS
**
*******************************************************************************/

tUWB_STATUS UWB_DeviceResetCommand(uint8_t resetConfig) {
  return uci_snd_device_reset_cmd(resetConfig);
}

/*******************************************************************************
**
** Function         UWB_SetCoreConfig
**
** Description      This function is called to send the configuration parameter
**                  TLV to UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_SET_CORE_CONFIG_REVT.
**
** Parameters       tlv_size - the length of p_param_tlvs.
**                  p_param_tlvs - the parameter ID/Len/Value list
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_SetCoreConfig(uint8_t tlv_size, uint8_t* p_param_tlvs) {
  return uci_snd_core_set_config_cmd(p_param_tlvs, tlv_size);
}

/*******************************************************************************
**
** Function         UWB_GetCoreConfig
**
** Description      This function is called to retrieve the parameter TLV from
**                  UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_GET_CORE_CONFIG_REVT.
**
** Parameters       num_ids - the number of parameter IDs
**                  p_param_ids - the parameter ID list.
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_GetCoreConfig(uint8_t num_ids, uint8_t* p_param_ids) {
  return uci_snd_core_get_config_cmd(p_param_ids, num_ids);
}

/*******************************************************************************
**
** Function         UWB_SessionInit
**
** Description      This function is called to send RCR command
**
** Parameters       interval-update - interval update  for the ranging
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_SessionInit(uint32_t session_id,uint8_t session_type) {
  return uci_snd_session_init_cmd(session_id,session_type);
}
/*******************************************************************************
**
** Function         UWB_SessionDeInit
**
** Description      This function is called to send RCR command
**
** Parameters       interval-update - interval update  for the ranging
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_SessionDeInit(uint32_t session_id) {
  return uci_snd_session_deinit_cmd(session_id);
}

/*******************************************************************************
**
** Function         UWB_GetAppConfig
**
** Description      This function is called to retrieve the parameter TLV from
**                  UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_GET_APP_CONFIG_REVT.
**
** Parameters       session_id : All APP configurations belonging to this Session ID
**                  num_ids - the number of parameter IDs
**                  length - Length of app parameter IDs
**                  p_param_ids - the parameter ID list.
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_GetAppConfig(uint32_t session_id, uint8_t num_ids, uint8_t length, uint8_t* p_param_ids) {
  return uci_snd_app_get_config_cmd(session_id, num_ids, length, p_param_ids);
}

/*******************************************************************************
**
** Function         UWB_SetAppConfig
**
** Description      This function is called to set the parameter TLV from
**                  UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_GET_APP_CONFIG_REVT.
**
** Parameters       session_id : All APP configurations belonging to this Session ID
**                  num_ids - the number of parameter IDs
**                  length - Length of TLV data.
**                  p_data - AppConfig TLV data
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_SetAppConfig(uint32_t session_id, uint8_t num_ids, uint8_t length, uint8_t* p_data) {
  return uci_snd_app_set_config_cmd(session_id, num_ids, length, p_data);
}

/*******************************************************************************
**
** Function         UWB_GetSessionCount
**
** Description      This function is called to send the get session count command to
**                  UWBC. The response from UWBC is reported to the given
**                  tUWB_RESPONSE_CBACK.
**
** Parameters       Nill
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_GetSessionCount() {
  return uci_snd_get_session_count_cmd();
}

/*******************************************************************************
**
** Function         UWB_StartRanging
**
** Description      This function is called to send the range start command to
**                  UWBC. The response from UWBC is reported to the given
**                  tUWB_RESPONSE_CBACK.
**
** Parameters       session_id -  Session ID for which ranging shall start
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_StartRanging(uint32_t session_id) {
  uwb_cb.data_credits = 1;
  return uci_snd_range_start_cmd(session_id);
}

/*******************************************************************************
**
** Function         UWB_StopRanging
**
** Description      This function is called to send the range stop command to
**                  UWBC. The response from UWBC is reported to the given
**                  tUWB_RESPONSE_CBACK.
**
** Parameters       session_id -  Session ID for which ranging shall start
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_StopRanging(uint32_t session_id) {
  return uci_snd_range_stop_cmd(session_id);
}

/*******************************************************************************
**
** Function         UWB_GetRangingCount
**
** Description      This function is called to send the get ranging count command to
**                  UWBC. The response from UWBC is reported to the given
**                  tUWB_RESPONSE_CBACK.
**
** Parameters       session_id - session_id
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_GetRangingCount(uint32_t session_id) {
  return uci_snd_get_range_count_cmd(session_id);;
}

/*******************************************************************************
**
** Function         UWB_GetSessionStatus
**
** Description      This function is called to send the get session status command to
**                  UWBC. The response from UWBC is reported to the given
**                  tUWB_RESPONSE_CBACK.
**
** Parameters       Nill
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_GetSessionStatus(uint32_t session_id) {
  return uci_snd_get_session_status_cmd(session_id);
}

/*******************************************************************************
**
** Function         UWB_MulticastListUpdate
**
** Description      This function is called send controlee session Multicast
**                  List Update command to UWBC.
**
** Parameters       session_id - Session ID
**                  action - action
**                  noOfControlees - No Of Controlees
**                  shortAddress - array of short address
**                  subSessionId - array of sub session ID
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_MulticastListUpdate(uint32_t session_id, uint8_t action, uint8_t noOfControlees, uint16_t* shortAddressList, uint32_t* subSessionIdList) {
  return uci_snd_multicast_list_update_cmd(session_id, action, noOfControlees, shortAddressList, subSessionIdList);
}

/*******************************************************************************
**
** Function         UWB_CoreGetDeviceCapability
**
** Description      This function is called to send the Core Get Capability
**                  Info command to UWBC. The response from UWBC is reported
**                  to the given tUWB_RESPONSE_CBACK.
**
** Parameters       Nill
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_CoreGetDeviceCapability(void) {
  return uci_snd_core_get_device_capability();
}

/*******************************************************************************
**
** Function         UWB_SendBlinkData
**
** Description      This function is called to send blink data tx
**                  command to UWBC.
**
** Parameters       session_id - Session ID
**                  repetition_count - repetition count
**                  app_data_len - size of application data
**                  app_data - application data
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_SendBlinkData(uint32_t session_id, uint8_t repetition_count, uint8_t app_data_len, uint8_t* app_data) {
  return uci_snd_blink_data_cmd(session_id, repetition_count, app_data_len, app_data);
}


/* APIs for UWB RF test functionality */

/*******************************************************************************
**
** Function         UWB_TestGetConfig
**
** Description      This function is called to retrieve the parameter TLV from
**                  UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_TEST_GET_CONFIG_REVT.
**
** Parameters       session_id : All TEST configurations belonging to this Session ID
**                  num_ids - the number of parameter IDs
**                  length - Length of test parameter IDs
**                  p_param_ids - the parameter ID list.
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_TestGetConfig(uint32_t session_id, uint8_t num_ids, uint8_t length, uint8_t* p_param_ids) {
  return uci_snd_test_get_config_cmd(session_id, num_ids, length, p_param_ids);
}

/*******************************************************************************
**
** Function         UWB_SetTestConfig
**
** Description      This function is called to set the parameter TLV from
**                  UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_TEST_SET_CONFIG_REVT.
**
** Parameters       session_id : All APP configurations belonging to this Session ID
**                  num_ids - the number of parameter IDs
**                  length - Length of TLV data.
**                  p_data - AppConfig TLV data
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_SetTestConfig(uint32_t session_id, uint8_t num_ids, uint8_t length, uint8_t* p_data) {
  return uci_snd_test_set_config_cmd(session_id, num_ids, length, p_data);
}

/*******************************************************************************
**
** Function         UWB_TestPeriodicTx
**
** Description      This function is called send per rx command from
**                  UWBC. The response from UWBC is reported by
**                  tUWA_DM_API_PER_TX as UWA_PER_TX_REVT.
**
** Parameters       length - Length of psdu data.
**                  p_data - psdu data
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_TestPeriodicTx(uint16_t length, uint8_t* p_data) {
  return uci_snd_test_periodic_tx_cmd(length, p_data);
}

/*******************************************************************************
**
** Function         UWB_TestPerRx
**
** Description      This function is called send per rx command from
**                  UWBC. The response from UWBC is reported by
**                  tUWA_DM_API_PER_RX as UWA_PER_RX_REVT.
**
** Parameters       length - Length of psdu data.
**                  p_data - psdu data
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_TestPerRx(uint16_t length, uint8_t* p_data) {
  return uci_snd_test_per_rx_cmd(length, p_data);
}

/*******************************************************************************
**
** Function         UWB_TestUwbLoopBack
**
** Description      This function is called send uwb loopback command to
**                  UWBC.
**
** Parameters       length - Length of psdu data.
**                  p_data - psdu data
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_TestUwbLoopBack(uint16_t length, uint8_t* p_data) {
  return uci_snd_test_uwb_loopback_cmd(length, p_data);
}


/********************************************************************************
**
** Function         UWB_TestStopSession
**
** Description      This function is called send per rx/tx stop command from
**                  UWBC. The response from UWBC is reported by
**                  tUWA_DM_API_PER_RX as UWA_PER_RX_REVT.
**
** Parameters       None
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_TestStopSession(void) {
  return uci_snd_test_stop_session_cmd();
}

/********************************************************************************
**
** Function         UWB_TestRx
**
** Description      This function is called send test rx command to
**                  UWBC.
**
** Parameters       None
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_TestRx(void) {
  return uci_snd_test_rx_cmd();
}

/*******************************************************************************
**
** Function         UWB_SendRawCommand
**
** Description      This function is called to send the raw vendor specific
**                  command to UWBC. The response from UWBC is reported to the
**                  given tUWB_VS_CBACK.
**
** Parameters       p_data - The command buffer
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_SendRawCommand(UWB_HDR* p_data, tUWB_RAW_CBACK* p_cback) {
  /* Validate parameters */
  if (p_data == NULL) {
    return UWB_STATUS_INVALID_PARAM;
  }

  p_data->event = BT_EVT_TO_UWB_UCI;
  p_data->layer_specific = UWB_WAIT_RSP_RAW_CMD;
  /* save the callback function in the BT_HDR, to receive the response */
  ((tUWB_UCI_RAW_MSG*)p_data)->p_cback = p_cback;

  uwb_ucif_check_cmd_queue(p_data);
  return UWB_STATUS_OK;
}

/*******************************************************************************
**
** Function         UWB_SendData
**
** Description      This function is called to send the data packet over UWB.
**
** Parameters       p_data - The data  buffer
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
tUWB_STATUS UWB_SendData(uint32_t session_id,uint8_t addr_len, uint8_t* p_addr,
                                           uint16_t data_len, uint8_t* p_data) {
  return uci_send_data_frame(session_id, addr_len, p_addr, data_len, p_data);
}

/*******************************************************************************
**
** Function         UWB_EnableConformanceTest
**
** Description      This function is called to set MCTT mode.
**
** Parameters       p_data - The data  buffer
**
** Returns          None
**
*******************************************************************************/
void UWB_EnableConformanceTest(uint8_t enable) {
  uwb_cb.IsConformaceTestEnabled = enable;
}

/*******************************************************************************
**
** Function         UWB_GetStatusName
**
** Description      This function returns the status name.
**
** NOTE             conditionally compiled to save memory.
**
** Returns          pointer to the name
**
*******************************************************************************/
const uint8_t* UWB_GetStatusName(tUWB_STATUS status) {
  switch (status) {
    case UWB_STATUS_OK:
      return (uint8_t*)"OK";
    case UWB_STATUS_REJECTED:
      return (uint8_t*)"REJECTED";
    case UWB_STATUS_FAILED:
      return (uint8_t*)"FAILED";
    case UWB_STATUS_SYNTAX_ERROR:
      return (uint8_t*)"SYNTAX_ERROR";
    case UWB_STATUS_UNKNOWN_GID:
      return (uint8_t*)"UNKNOWN_GID";
    case UWB_STATUS_UNKNOWN_OID:
      return (uint8_t*)"UNKNOWN_OID";
    case UWB_STATUS_INVALID_PARAM:
      return (uint8_t*)"INVALID_PARAM";
    case UWB_STATUS_INVALID_RANGE:
      return (uint8_t*)"INVALID_RANGE";
    case UWB_STATUS_READ_ONLY:
      return (uint8_t*)"READ_ONLY";
    case UWB_STATUS_COMMAND_RETRY:
      return (uint8_t*)"COMMAND_RETRY";
    case UWB_STATUS_SESSSION_NOT_EXIST:
      return (uint8_t*)"SESSION_NOT_EXIST";
    case UWB_STATUS_SESSSION_DUPLICATE:
      return (uint8_t*)"SESSION_DUPLICATE";
    case UWB_STATUS_SESSSION_ACTIVE:
      return (uint8_t*)"SESSION_IN_ACTIVE";
    case UWB_STATUS_MAX_SESSSIONS_EXCEEDED:
      return (uint8_t*)"MAX_SESSION_REACHED";
    case UWB_STATUS_SESSION_NOT_CONFIGURED:
      return (uint8_t*)"SESSION_NOT_CONFIGURED";
    case UWB_STATUS_RANGING_TX_FAILED:
      return (uint8_t*)"RANGING TX FAILED";
    case UWB_STATUS_RANGING_RX_TIMEOUT:
      return (uint8_t*)"RANGING RX TIMEOUT";
    case UWB_STATUS_RANGING_RX_PHY_DEC_FAILED:
      return (uint8_t*)"PHYSICAL DECODING FAILED";
    case UWB_STATUS_RANGING_RX_PHY_TOA_FAILED:
      return (uint8_t*)"PHYSICAL TOA FAILED";
    case UWB_STATUS_RANGING_RX_PHY_STS_FAILED:
      return (uint8_t*)"PHYSICAL STS FAILED";
    case UWB_STATUS_RANGING_RX_MAC_DEC_FAILED:
      return (uint8_t*)"MAC DECODING FAILED";
    case UWB_STATUS_RANGING_RX_MAC_IE_DEC_FAILED:
      return (uint8_t*)"MAC INFORMATION DECODING FAILED";
    case UWB_STATUS_RANGING_RX_MAC_IE_MISSING:
      return (uint8_t*)"MAC INFORMATION MISSING";
    case UWB_STATUS_DATA_MAX_TX_PSDU_SIZE_EXCEEDED:
      return (uint8_t*)"MAX_PSDU_SIZE_EXCEEDED";
    case UWB_STATUS_DATA_RX_CRC_ERROR:
      return (uint8_t*)"DATA_RX_CRC_ERROR";
    default:
      return (uint8_t*)"UNKNOWN";
  }
}
