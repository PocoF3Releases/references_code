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
/******************************************************************************
 *
 *  This file contains function of the UWB unit to receive/process UCI
 *  commands.
 *
 ******************************************************************************/

#include "uwb_target.h"
#include "uwb_gki.h"
#include "uci_defs.h"
#include "uci_test_defs.h"
#include "uci_hmsgs.h"
#include "uwb_api.h"
#include "uwb_int.h"
#include "uwa_sys.h"
#include "uwb_osal_common.h"
#include "uwb_hal_int.h"
#include "uci_log.h"
#if(NXP_UWB_EXTNS == TRUE)
#include "uci_ext_defs.h"
#endif

/*******************************************************************************
 **
 ** Function         uwb_proc_core_rsp
 **
 ** Description      Process UCI responses in the CORE group
 **
 ** Returns          true-caller of this function to free the GKI buffer p_msg
 **
 *******************************************************************************/
bool uwb_proc_core_rsp(uint8_t op_code, uint8_t* p_buf, uint16_t len) {
  bool free = true;

  /* process the message based on the opcode and message type */
  switch (op_code) {
  case UCI_MSG_CORE_DEVICE_RESET:
    uwb_ucif_proc_core_device_reset_rsp_status(p_buf, len);
    break;
  case UCI_MSG_CORE_DEVICE_INFO:
    uwb_ucif_proc_get_device_info_rsp(p_buf, len);
    break;
  case UCI_MSG_CORE_GET_CAPS_INFO:
    uwb_ucif_proc_get_device_capability_rsp(p_buf, len);
    break;
  case UCI_MSG_CORE_GET_CONFIG:
    uwb_ucif_proc_core_get_config_rsp(p_buf, len);
    break;
  case UCI_MSG_CORE_SET_CONFIG:
    uwb_ucif_proc_core_set_config_status(p_buf, len);
    break;
  default:
    UCI_TRACE_E("%s: unknown opcode:0x%x", __func__, op_code);
    break;
  }

  return free;
}
/*******************************************************************************
 **
 ** Function         uci_proc_core_management_ntf
 **
 ** Description      Process UCI notifications in the core Management group
 **
 ** Returns          void
 **
 *******************************************************************************/
void uci_proc_core_management_ntf(uint8_t op_code, uint8_t* p_buf,
    uint16_t len) {
  switch (op_code) {
  case UCI_MSG_CORE_GENERIC_ERROR_NTF:
    uwb_ucif_proc_core_generic_error_ntf(p_buf, len);
    break;
  case UCI_MSG_CORE_DEVICE_STATUS_NTF:
    uwb_ucif_proc_core_device_status(p_buf, len);
    break;
  default:
    UCI_TRACE_E("%s: unknown opcode:0x%x", __func__, op_code);
    break;
  }
}

/*******************************************************************************
 **
 ** Function         uci_proc_session_management_rsp
 **
 ** Description      Process UCI responses in the Session Management group
 **
 ** Returns          void
 **
 *******************************************************************************/
void uci_proc_session_management_rsp(uint8_t op_code, uint8_t* p_buf,
    uint16_t len) {
  switch (op_code) {
  case UCI_MSG_SESSION_INIT:
    uwb_ucif_session_management_status(UWB_SESSION_INIT_REVT, p_buf, len);
    break;
  case UCI_MSG_SESSION_DEINIT:
    uwb_ucif_session_management_status(UWB_SESSION_DEINIT_REVT, p_buf, len);
    break;
  case UCI_MSG_SESSION_GET_APP_CONFIG:
    uwb_ucif_proc_app_get_config_status(p_buf, len);
    break;
  case UCI_MSG_SESSION_SET_APP_CONFIG:
    uwb_ucif_proc_app_set_config_status(p_buf, len);
    break;
  case UCI_MSG_SESSION_GET_COUNT:
    uwb_ucif_session_management_status(UWB_SESSION_GET_COUNT_REVT, p_buf,
        len);
    break;
  case UCI_MSG_SESSION_GET_STATE:
    uwb_ucif_session_management_status(UWB_SESSION_GET_STATE_REVT, p_buf,
        len);
    break;
  case UCI_MSG_SESSION_UPDATE_CONTROLLER_MULTICAST_LIST:
    uwb_ucif_session_management_status(UWB_SESSION_UPDATE_MULTICAST_LIST_REVT, p_buf,
        len);
    break;
  default:
    UCI_TRACE_E("%s: unknown opcode:0x%x", __func__, op_code);
    break;
  }
}

/*******************************************************************************
 **
 ** Function         uci_proc_test_management_rsp
 **
 ** Description      Process UCI responses in the APP DATA Management group
 **
 ** Returns          void
 **
 *******************************************************************************/
void uci_proc_test_management_rsp(uint8_t op_code, uint8_t* p_buf,
    uint16_t len) {
  switch (op_code) {
  case UCI_MSG_TEST_GET_CONFIG:
    uwb_ucif_proc_test_get_config_status(p_buf, len);
    break;
  case UCI_MSG_TEST_SET_CONFIG:
    uwb_ucif_proc_test_set_config_status(p_buf, len);
    break;
  case UCI_MSG_TEST_PERIODIC_TX:
    uwb_ucif_test_management_status(UWB_TEST_PERIODIC_TX_REVT, p_buf, len);
    break;
  case UCI_MSG_TEST_PER_RX:
    uwb_ucif_test_management_status(UWB_TEST_PER_RX_REVT, p_buf, len);
    break;
  case UCI_MSG_TEST_LOOPBACK:
    uwb_ucif_test_management_status(UWB_TEST_LOOPBACK_REVT, p_buf, len);
    break;
  case UCI_MSG_TEST_RX:
    uwb_ucif_test_management_status(UWB_TEST_RX_REVT, p_buf, len);
    break;
  case UCI_MSG_TEST_STOP_SESSION:
    uwb_ucif_test_management_status(UWB_TEST_STOP_SESSION_REVT, p_buf, len);
    break;
  default:
    UCI_TRACE_E("%s: unknown opcode:0x%x", __func__, op_code);
    break;
  }
}

/*******************************************************************************
 **
 ** Function         uci_proc_session_management_ntf
 **
 ** Description      Process UCI notifications in the Ranging Management group
 **
 ** Returns          void
 **
 *******************************************************************************/
void uci_proc_session_management_ntf(uint8_t op_code, uint8_t* p_buf,
    uint16_t len) {
  switch (op_code) {
  case UCI_MSG_SESSION_STATUS_NTF:
    uwb_ucif_proc_session_status(p_buf, len);
    break;
  case UCI_MSG_SESSION_UPDATE_CONTROLLER_MULTICAST_LIST:
    uwb_ucif_proc_multicast_list_update_ntf(p_buf, len);
    break;
  default:
    UCI_TRACE_E("%s: unknown opcode:0x%x", __func__, op_code);
    break;
  }
}

/*******************************************************************************
 **
 ** Function         uci_proc_app_data_management_ntf
 **
 ** Description      Process UCI notifications in the Ranging Management group
 **
 ** Returns          void
 **
 *******************************************************************************/
void uci_proc_app_data_management_ntf(uint8_t op_code, uint8_t* p_buf,
    uint16_t len) {
  switch (op_code) {
  case EXT_UCI_MSG_DATA_CREDIT_NTF:
    uwb_ucif_proc_data_credit_ntf(p_buf, len);
    break;
  case EXT_UCI_MSG_DATA_TRANSFER_STATUS_NTF:
    uwb_ucif_proc_data_transfer_status_ntf(p_buf, len);
    break;
  default:
    UCI_TRACE_E("%s: unknown opcode:0x%x", __func__, op_code);
    break;
  }
}


/*******************************************************************************
 **
 ** Function         uci_proc_rang_management_rsp
 **
 ** Description      Process UCI responses in the Ranging Management group
 **
 ** Returns          void
 **
 *******************************************************************************/
void uci_proc_rang_management_rsp(uint8_t op_code, uint8_t* p_buf,
    uint16_t len) {
  switch (op_code) {
  case UCI_MSG_RANGE_START:
    uwb_ucif_range_management_status(UWB_START_RANGE_REVT, p_buf, len);
    break;
  case UCI_MSG_RANGE_STOP:
    uwb_ucif_range_management_status(UWB_STOP_RANGE_REVT, p_buf, len);
    break;
  case UCI_MSG_RANGE_GET_RANGING_COUNT:
    uwb_ucif_get_range_count_status(UWB_GET_RANGE_COUNT_REVT, p_buf, len);
    break;
  case UCI_MSG_RANGE_BLINK_DATA_TX:
    uwb_ucif_range_management_status(UWB_BLINK_DATA_TX_REVT, p_buf, len);
    break;
  default:
    UCI_TRACE_E("%s: unknown opcode:0x%x", __func__, op_code);
    break;
  }
}

/*******************************************************************************
 **
 ** Function         uci_proc_rang_management_ntf
 **
 ** Description      Process UCI notifications in the Ranging Management group
 **
 ** Returns          void
 **
 *******************************************************************************/
void uci_proc_rang_management_ntf(uint8_t op_code, uint8_t* p_buf,
    uint16_t len) {
  switch (op_code) {
  case UCI_MSG_RANGE_DATA_NTF:
    uwb_ucif_proc_ranging_data(p_buf, len);
    break;
  case UCI_MSG_RANGE_BLINK_DATA_TX_NTF:
    uwb_ucif_proc_send_blink_data_ntf(p_buf, len);
    break;
  default:
    UCI_TRACE_E("%s: unknown opcode:0x%x", __func__, op_code);
    break;
  }
}

/*******************************************************************************
 **
 ** Function         uci_proc_raw_cmd_rsp
 **
 ** Description      Process RAW CMD responses
 **
 ** Returns          void
 **
 *******************************************************************************/
void uci_proc_raw_cmd_rsp(uint8_t* p_buf, uint16_t len) {
  tUWB_RAW_CBACK* p_cback = (tUWB_RAW_CBACK*) uwb_cb.p_raw_cmd_cback;

  UCI_TRACE_I(" uci_proc_raw_cmd_rsp:"); // for debug

  /* If there's a pending/stored command, restore the associated address of the
   * callback function */
  if (p_cback == NULL) {
    UCI_TRACE_E("p_raw_cmd_cback is null");
  } else {
    (*p_cback)(0 /*unused in this case*/, len, p_buf);
    uwb_cb.p_raw_cmd_cback = NULL;
  }
  uwb_cb.rawCmdCbflag = false;
  uwb_ucif_update_cmd_window();
}

/*******************************************************************************
 **
 ** Function         uci_proc_test_management_ntf
 **
 ** Description      Process UCI notifications in the proprietary Management group
 **
 ** Returns          void
 **
 *******************************************************************************/
void uci_proc_test_management_ntf(uint8_t op_code, uint8_t* p_buf,
    uint16_t len) {
  switch (op_code) {
  case UCI_MSG_TEST_PERIODIC_TX:
    uwb_ucif_proc_rf_test_data(UWB_TEST_PERIODIC_TX_DATA_REVT, p_buf, len);
    break;
  case UCI_MSG_TEST_PER_RX:
    uwb_ucif_proc_rf_test_data(UWB_TEST_PER_RX_DATA_REVT, p_buf, len);
    break;
  case UCI_MSG_TEST_LOOPBACK:
    uwb_ucif_proc_rf_test_data(UWB_TEST_LOOPBACK_DATA_REVT, p_buf, len);
    break;
  case UCI_MSG_TEST_RX:
    uwb_ucif_proc_rf_test_data(UWB_TEST_RX_DATA_REVT, p_buf, len);
    break;
  default:
    UCI_TRACE_E("%s: unknown opcode:0x%x", __func__, op_code);
    break;
  }
}

/*******************************************************************************
 **
 ** Function         uci_proc_proprietary_ntf
 **
 ** Description      Process UCI notifications in the proprietary Management group
 **
 ** Returns          void
 **
 *******************************************************************************/
void uci_proc_proprietary_ntf(uint8_t op_code, uint8_t* p_buf, uint16_t len) {
  uint8_t stat;
  if (len > 0) {
    UCI_TRACE_I(" uci_proc_raw_cmd_rsp:"); // for debug

    if (uwb_cb.p_ext_resp_cback == NULL) {
      UCI_TRACE_E("ext response callback is null");
    } else {
      switch (op_code) {
      /* Perform Reset incase of SE Communication Failure*/
      case EXT_UCI_MSG_SE_COMM_ERROR_NTF: {

        uint8_t *status_ptr = p_buf + UCI_RESPONSE_STATUS_OFFSET;
        uint8_t status = *status_ptr;
        if (status == UCI_STATUS_ESE_RECOVERY_FAILURE) {
          uwb_uci_IoctlInOutData_t inpOutData;
          stat = uwb_cb.p_hal->ioctl(HAL_UWB_IOCTL_ESE_RESET,
              (void*) &inpOutData);
          if (stat == UWA_STATUS_OK) {
            UCI_TRACE_I("%s: Set ESE RESET successful", __func__);
          } else {
            UCI_TRACE_E("%s: Set ESE RESET Failed", __func__);
          }
        }
      }
        break;
      default:
        break;
      }
      /* Invokes extDeviceManagementCallback with Raw UCI Data in JNI */
      (*uwb_cb.p_ext_resp_cback)((tUWB_RAW_EVT)(op_code), len, p_buf);
    }
  } else {
    UCI_TRACE_E("%s: len is zero", __func__);
  }
}
