/******************************************************************************
 *
 *  Copyright (C) 2015 Google Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "btif/include/btif_debug_conn.h"
#include "osi/include/time.h"
#include "btif/include/btif_storage.h"
#include "stack/include/hcidefs.h"
#include "stack/include/l2cdefs.h"

#define NUM_CONNECTION_EVENTS 36
#define TEMP_BUFFER_SIZE 30

typedef struct conn_event_t {
  uint64_t ts;
  btif_debug_conn_state_t state;
  RawAddress bda;
  char dev_name[10];
  tCONN_STATUS conn_status;
} conn_event_t;

static conn_event_t connection_events[NUM_CONNECTION_EVENTS];
static uint8_t current_event = 0;

static char* format_ts(const uint64_t ts, char* buffer, int len) {
  const uint64_t ms = ts / 1000;
  const time_t secs = ms / 1000;
  struct tm* ptm = localtime(&secs);

  char tempbuff[20] = {0};
  if (ptm) {
    if (strftime(tempbuff, sizeof(tempbuff), "%m-%d %H:%M:%S", ptm)) {
      snprintf(buffer, len, "%s.%03u", tempbuff, (uint16_t)(ms % 1000));
    }
  }

  return buffer;
}

static const char* format_state(const btif_debug_conn_state_t state) {
  switch (state) {
    case BTIF_DEBUG_LE_CONNECTED:
      return "BLE-CONNECTED          ";
    case BTIF_DEBUG_LE_DISCONNECTED:
      return "BLE-DISCONNECTED       ";
    case BTIF_DEBUG_CLASSICAL_CONNECTED:
      return "CLASSICAL-CONNECTED    ";
    case BTIF_DEBUG_CLASSICAL_DISCONNECTED:
      return "CLASSICAL-DISCONNECTED ";
    case BTIF_DEBUG_SCO_CONNECTED:
      return "SCO-CONNECTED          ";
    case BTIF_DEBUG_SCO_DISCONNECTED:
      return "SCO-DISCONNECTED       ";
    case BTIF_DEBUG_ESCO_CONNECTED:
      return "eSCO-CONNECTED         ";
    case BTIF_DEBUG_ESCO_DISCONNECTED:
      return "eSCO-DISCONNECTED     ";
  }
  return "UNKNOWN";
}

static const char *format_conn_status(const tCONN_STATUS status) {
  switch (status) {
    /* spec specified status */
    case HCI_SUCCESS:
      return "Success";
    case HCI_ERR_ILLEGAL_COMMAND:
      return "Unknwon HCI Command";
    case HCI_ERR_NO_CONNECTION:
      return "Unknwon Conn Identifer";
    case HCI_ERR_HW_FAILURE:
      return "Hardware Failure";
    case HCI_ERR_PAGE_TIMEOUT:
      return "Page Timeout";
    case HCI_ERR_AUTH_FAILURE:
      return "Auth Failure";
    case HCI_ERR_KEY_MISSING:
      return "PIN or Key Missing";
    case HCI_ERR_MEMORY_FULL:
      return "Memory Exceeded";
    case HCI_ERR_CONNECTION_TOUT:
      return "Connection Timeout";
    case HCI_ERR_MAX_NUM_OF_CONNECTIONS:
      return "Conn Limit Exceeded";
    case HCI_ERR_MAX_NUM_OF_SCOS:
      return "SCO Limit Exceeded";
    case HCI_ERR_CONNECTION_EXISTS:
      return "Conn Already Exist";
    case HCI_ERR_COMMAND_DISALLOWED:
      return "Command Disallowed";
    case HCI_ERR_HOST_REJECT_RESOURCES:
      return "No Resource Reject";
    case HCI_ERR_HOST_REJECT_SECURITY:
      return "Security Reject";
    case HCI_ERR_HOST_REJECT_DEVICE:
      return "Unacceptable BD_ADDR";
    case HCI_ERR_HOST_TIMEOUT:
      return "Conn Accept Timeout";
    case HCI_ERR_UNSUPPORTED_VALUE:
      return "Unsupport Feature or Param";
    case HCI_ERR_ILLEGAL_PARAMETER_FMT:
      return "Invalid HCI Command Param";
    case HCI_ERR_PEER_USER:
      return "Remote User Terminated";
    case HCI_ERR_PEER_LOW_RESOURCES:
      return "Remote Low Resource";
    case HCI_ERR_PEER_POWER_OFF:
      return "Remote Power Off";
    case HCI_ERR_CONN_CAUSE_LOCAL_HOST:
      return "Local Host Terminated";
    case HCI_ERR_REPEATED_ATTEMPTS:
      return "Repeated Attempts";
    case HCI_ERR_PAIRING_NOT_ALLOWED:
      return "Paring Not Allowed";
    case HCI_ERR_UNKNOWN_LMP_PDU:
      return "Unknown LMP PDU";
    case HCI_ERR_UNSUPPORTED_REM_FEATURE:
      return "Unsupported Remote Feature";
    case HCI_ERR_SCO_OFFSET_REJECTED:
      return "SCO Offset Reject";
    case HCI_ERR_SCO_INTERVAL_REJECTED:
      return "SCO Interval Reject";
    case HCI_ERR_SCO_AIR_MODE:
      return "SCO Air Mode Reject";
    case HCI_ERR_INVALID_LMP_PARAM:
      return "Invalid LMP Param";
    case HCI_ERR_UNSPECIFIED:
      return "Unspecified Error";
    case HCI_ERR_UNSUPPORTED_LMP_FEATURE :
      return "Unsupport Remote LMP Feature";
    case HCI_ERR_ROLE_CHANGE_NOT_ALLOWED:
      return "Role Change Not Allowed";
    case HCI_ERR_LMP_RESPONSE_TIMEOUT:
      return "LMP Response Timeout";
    case HCI_ERR_LMP_ERR_TRANS_COLLISION:
      return "LMP Transaction Collison";
    case HCI_ERR_LMP_PDU_NOT_ALLOWED:
      return "LMP PDU Not Allowed";
    case HCI_ERR_ENCRY_MODE_NOT_ACCEPTABLE:
      return "Encryption Not Acceptable";
    case HCI_ERR_UNIT_KEY_USED:
      return "Link Key cannot be changed";
    case HCI_ERR_QOS_NOT_SUPPORTED:
      return "QoS not Support";
    case HCI_ERR_INSTANT_PASSED:
      return "Instant Passed";
    case HCI_ERR_PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED:
      return "Paring with Uint Key Not Support";
    case HCI_ERR_DIFF_TRANSACTION_COLLISION:
      return "Diff Transaction Collision";
    case HCI_ERR_UNDEFINED_0x2B:
      return "Reserved";
    case HCI_ERR_QOS_UNACCEPTABLE_PARAM:
      return "Qos Unacceptable Param";
    case HCI_ERR_QOS_REJECTED:
      return "Qos Reject";
    case HCI_ERR_CHAN_CLASSIF_NOT_SUPPORTED:
      return "Channel Classification Not Support";
    case HCI_ERR_INSUFFCIENT_SECURITY:
      return "Insufficient Security";
    case HCI_ERR_PARAM_OUT_OF_RANGE:
      return "Param Out of Range";
    case HCI_ERR_UNDEFINED_0x31:
      return "Reserved";
    case HCI_ERR_ROLE_SWITCH_PENDING:
      return "Role Switch Pending";
    case HCI_ERR_UNDEFINED_0x33:
      return "Reserved";
    case HCI_ERR_RESERVED_SLOT_VIOLATION:
      return "Reserved Slot Violation";
    case HCI_ERR_ROLE_SWITCH_FAILED:
      return "Role Switch Failed";
    case HCI_ERR_INQ_RSP_DATA_TOO_LARGE:
      return "EIR Too Large";
    case HCI_ERR_SIMPLE_PAIRING_NOT_SUPPORTED:
      return "SSP Not Support";
    case HCI_ERR_HOST_BUSY_PAIRING:
      return "Host Busy Paring";
    case HCI_ERR_REJ_NO_SUITABLE_CHANNEL:
      return "No Suitable Channel Reject";
    case HCI_ERR_CONTROLLER_BUSY:
      return "Controller Busy";
    case HCI_ERR_UNACCEPT_CONN_INTERVAL:
      return "Unacceptable Conn Param";
    case HCI_ERR_ADVERTISING_TIMEOUT:
      return "ADV Timeout";
    case HCI_ERR_CONN_TOUT_DUE_TO_MIC_FAILURE:
      return "MIC Failure";
    case HCI_ERR_CONN_FAILED_ESTABLISHMENT:
      return "Conn Failed to Establishment";
    case HCI_ERR_MAC_CONNECTION_FAILED:
      return "MAC Conn Failed";
    /* internal defined status */
    case L2CAP_CONN_CANCEL:
      return "L2cap Conn Cancelled";
  }
  return "UNKNOWN";
}

static void next_event() {
  ++current_event;
  if (current_event == NUM_CONNECTION_EVENTS) current_event = 0;
}

void btif_debug_conn_state(const RawAddress& bda,
                           const btif_debug_conn_state_t state,
                           tCONN_STATUS conn_status) {
  char remote_name[BTM_MAX_REM_BD_NAME_LEN] = "         \0";
  next_event();

  conn_event_t* evt = &connection_events[current_event];
  evt->ts = time_gettimeofday_us();
  evt->state = state;
  evt->conn_status = conn_status;
  evt->bda = bda;
  // Save device name from db
  if (btif_storage_get_stored_remote_name(bda, remote_name)) {
    memcpy(evt->dev_name, remote_name, sizeof(evt->dev_name)-1);
  }
}

void btif_debug_conn_dump(int fd) {
  const uint8_t current_event_local =
      current_event;  // Cache to avoid threading issues
  uint8_t dump_event = current_event_local;
  char ts_buffer[TEMP_BUFFER_SIZE] = {0};

  dprintf(fd, "\nConnection Events:\n");
  if (connection_events[dump_event].ts == 0) dprintf(fd, "  None\n");

  while (connection_events[dump_event].ts) {
    conn_event_t* evt = &connection_events[dump_event];
    dprintf(fd, "  %s %s %s %s", format_ts(evt->ts, ts_buffer, sizeof(ts_buffer)),
            format_state(evt->state), evt->bda.ToIncompleteString().c_str(),
            evt->dev_name);
    dprintf(fd, "  status = %s", format_conn_status(evt->conn_status));
    dprintf(fd,"\n");

    // Go to previous event; wrap if needed
    if (dump_event > 0)
      --dump_event;
    else
      dump_event = NUM_CONNECTION_EVENTS - 1;

    // Check if we dumped all events
    if (dump_event == current_event_local) break;
  }
}
