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
 *  This file contains the Near Field Communication (UWB) API function
 *  external definitions.
 *
 ******************************************************************************/

#ifndef UWB_API_H
#define UWB_API_H

#include "uwb_types.h"
#include "uwb_target.h"
#include "uci_defs.h"
#include "uwb_hal_api.h"
#include "uwb_gki.h"

#define NXP_CHIP_SR100 1
#define NXP_ANDROID_VERSION (11U) /* Android version */
#define UWB_NXP_MW_VERSION_MAJ (0x27) /* MW major version */
#define UWB_NXP_MW_VERSION_MIN (0x00) /* MS Nibble is MW minor version and LS Nibble is Test Object/Patch*/
#define UWB_NXP_ANDROID_MW_RC_VERSION  (0x00) /* Android MW RC Version */
#define UWB_NXP_ANDROID_MW_DROP_VERSION  (0x00) /* Android MW early drops */

typedef uint8_t tUWB_STATUS;


/* UWB application return status codes */
/* Command succeeded    */
#define UWB_STATUS_OK UCI_STATUS_OK
/* Command is rejected. */
#define UWB_STATUS_REJECTED UCI_STATUS_REJECTED
/* failed               */
#define UWB_STATUS_FAILED UCI_STATUS_FAILED
/* Syntax error         */
#define UWB_STATUS_SYNTAX_ERROR UCI_STATUS_SYNTAX_ERROR
/* Invalid Parameter    */
#define UWB_STATUS_INVALID_PARAM UCI_STATUS_INVALID_PARAM
/* Invalid Parameter    */
#define UWB_STATUS_INVALID_RANGE UCI_STATUS_INVALID_RANGE
/* Unknown UCI Group ID */
#define UWB_STATUS_UNKNOWN_GID UCI_STATUS_UNKNOWN_GID
/* Unknown UCI Opcode   */
#define UWB_STATUS_UNKNOWN_OID UCI_STATUS_UNKNOWN_OID
/* Read Only   */
#define UWB_STATUS_READ_ONLY UCI_STATUS_READ_ONLY
/* retry the command */
#define UWB_STATUS_COMMAND_RETRY UCI_STATUS_COMMAND_RETRY

/* UWB session Specific status code */
/* session is not exist in UWBD */
#define UWB_STATUS_SESSSION_NOT_EXIST UCI_STATUS_SESSSION_NOT_EXIST
/* Session is already exist/duplicate */
#define UWB_STATUS_SESSSION_DUPLICATE UCI_STATUS_SESSSION_DUPLICATE
/* Session is in active state */
#define UWB_STATUS_SESSSION_ACTIVE UCI_STATUS_SESSSION_ACTIVE
/* Maximum sessions are reached */
#define UWB_STATUS_MAX_SESSSIONS_EXCEEDED UCI_STATUS_MAX_SESSSIONS_EXCEEDED
/*session not configured */
#define UWB_STATUS_SESSION_NOT_CONFIGURED UCI_STATUS_SESSION_NOT_CONFIGURED

/* Ranging specific error code */
/* Ranging tx failed */
#define UWB_STATUS_RANGING_TX_FAILED UCI_STATUS_RANGING_TX_FAILED
/* Ranging rx timeout */
#define UWB_STATUS_RANGING_RX_TIMEOUT UCI_STATUS_RANGING_RX_TIMEOUT
/* Physical layer decoding failed */
#define UWB_STATUS_RANGING_RX_PHY_DEC_FAILED UCI_STATUS_RANGING_RX_PHY_DEC_FAILED
/* Physical layer TOA failed */
#define UWB_STATUS_RANGING_RX_PHY_TOA_FAILED UCI_STATUS_RANGING_RX_PHY_TOA_FAILED
/* Physical layer STS failed */
#define UWB_STATUS_RANGING_RX_PHY_STS_FAILED UCI_STATUS_RANGING_RX_PHY_STS_FAILED
/* MAC decoding failed */
#define UWB_STATUS_RANGING_RX_MAC_DEC_FAILED UCI_STATUS_RANGING_RX_MAC_DEC_FAILED
/* MAC information decoding failed */
#define UWB_STATUS_RANGING_RX_MAC_IE_DEC_FAILED UCI_STATUS_RANGING_RX_MAC_IE_DEC_FAILED
/* MAC information missing */
#define UWB_STATUS_RANGING_RX_MAC_IE_MISSING  UCI_STATUS_RANGING_RX_MAC_IE_MISSING

/* UWB Data Session Specific Status Codes */
#define UWB_STATUS_DATA_MAX_TX_PSDU_SIZE_EXCEEDED UCI_STATUS_DATA_MAX_TX_PSDU_SIZE_EXCEEDED
#define UWB_STATUS_DATA_RX_CRC_ERROR UCI_STATUS_DATA_RX_CRC_ERROR


/* all UWB Manager Callback functions have prototype like void (cback) (uint8_t
 * event, void *p_data)
 * tUWB_RESPONSE_CBACK uses tNFC_RESPONSE_EVT; range  0x4000 ~
 * tUWB_TEST_RESPONSE_CBACK uses tNFC_RESPONSE_EVT; range  0x5000 ~
 */

#define UWB_FIRST_REVT 0x4000
#define UWB_FIRST_TEVT 0x5000

//RFU size for tdoa Ranging

#define TDOA_RANGE_MEASR_RFU  12
#define TWR_RANGE_MEASR_RFU   12

/* the events reported on tUWB_RESPONSE_CBACK */
enum {
  UWB_ENABLE_REVT = UWB_FIRST_REVT,            /* 0  Enable event                        */
  UWB_DISABLE_REVT,                            /* 1  Disable event                       */
  UWB_REGISTER_EXT_CB_REVT,                    /* 2  Register Ext Callback               */
  UWB_DEVICE_STATUS_REVT,                      /* 3  device status notification          */
  UWB_SET_CORE_CONFIG_REVT,                    /* 4  Set Config Response                 */
  UWB_GET_CORE_CONFIG_REVT,                    /* 5  Get Config Response                 */
  UWB_GET_DEVICE_INFO_REVT,                    /* 6  Get Device Info response            */
  UWB_DEVICE_RESET_REVT,                       /* 7  device Reset response               */
  UWB_CORE_GEN_ERR_STATUS_REVT,                /* 8  Generic Error Notification          */
  UWB_SESSION_INIT_REVT,                       /* 9  session Init  request               */
  UWB_SESSION_STATUS_NTF_REVT,                 /* 10 SESSION STATUS NTF response         */
  UWB_SESSION_DEINIT_REVT,                     /* 11  session DeInit request             */
  UWB_SESSION_GET_COUNT_REVT,                  /* 12 get session count response          */
  UWB_GET_APP_CONFIG_REVT,                     /* 13  Get App Config response            */
  UWB_SET_APP_CONFIG_REVT,                     /* 14  Get App Config response            */
  UWB_START_RANGE_REVT,                        /* 15 range start response                */
  UWB_STOP_RANGE_REVT,                         /* 16 range stop response                 */
  UWB_RANGE_DATA_REVT,                         /* 17 range data notofication             */
  UWB_GET_RANGE_COUNT_REVT,                    /* 23  get Range Count response           */
  UWB_SESSION_GET_STATE_REVT,                  /* 24 get session status response         */
  UWB_UWBD_TRANSPORT_ERR_REVT,                 /* 25  UCI Tranport error                 */
  UWB_UWBD_RESP_TIMEOUT_REVT,                  /* 26  UWB device is not responding       */
  UWB_CORE_GET_DEVICE_CAPABILITY_REVT,         /* 27  Get Device Capability              */
  UWB_SESSION_UPDATE_MULTICAST_LIST_REVT,      /* 28 Session Update Multicast List resp  */
  UWB_SESSION_UPDATE_MULTICAST_LIST_NTF_REVT,  /* 29 Session Update Multicast List ntf   */
  UWB_BLINK_DATA_TX_REVT,                      /* 30 Blink Data Tx resp                  */
  UWB_BLINK_DATA_TX_NTF_REVT,                  /* 31 Blink Data Tx ntf                   */
  UWB_SEND_DATA_PACKET_RSP_REVT,                /* 32 UWB data reception status by UWBS   */
  UWB_DATA_TRANSFER_STATUS_NTF_REVT,           /* 33 UWB data transfer status over UWB   */
  UWB_DATA_RECV_REVT,                          /* 34  received data over UWB */
  UWB_SEGMENT_SENT_EVT                          /* 35 send raw command event when pbf is set */
};
typedef uint16_t tUWB_RESPONSE_EVT;

/* the events reported on tUWB_TEST_RESPONSE_CBACK */
enum {
  UWB_TEST_GET_CONFIG_REVT = UWB_FIRST_TEVT,   /* 0  Get test Config response......*/
  UWB_TEST_SET_CONFIG_REVT,                    /* 1  Set test Config response           */
  UWB_TEST_PERIODIC_TX_DATA_REVT,              /* 2  PERIODIC TX ntf results response   */
  UWB_TEST_PER_RX_DATA_REVT,                   /* 3  PER RX ntf results response        */
  UWB_TEST_PERIODIC_TX_REVT,                   /* 4  PERIODIC tx test results response  */
  UWB_TEST_PER_RX_REVT,                        /* 5  PER rx test results response       */
  UWB_TEST_STOP_SESSION_REVT,                  /* 6  Stop PER test Session              */
  UWB_TEST_LOOPBACK_DATA_REVT,                 /* 7  Test Rf loop back ntf              */
  UWB_TEST_LOOPBACK_REVT,                      /* 8  Test Rf loop back resp             */
  UWB_TEST_RX_REVT,                            /* 9  RX Test resp                        */
  UWB_TEST_RX_DATA_REVT                        /* 10  RX Test ntf                         */
};
typedef uint16_t tUWB_TEST_RESPONSE_EVT;

typedef uint8_t tUWB_RAW_EVT;  /* proprietary events */
typedef uint8_t tUWB_STATUS;


typedef struct {
  tUWB_STATUS status; /* The event status.                */
} tUWB_ENABLE_REVT;

#define UWB_MAX_NUM_IDS 125
/* the data type associated with UWB_SET_CORE_CONFIG_REVT */
typedef struct {
  tUWB_STATUS status;                 /* The event status.                */
  uint8_t num_param_id;               /* Number of rejected UCI Param ID  */
  uint8_t param_ids[UWB_MAX_NUM_IDS]; /* UCI Param ID          */
  uint16_t tlv_size;                  /* The length of TLV    */
} tUWB_SET_CORE_CONFIG_REVT;

typedef struct {
  tUWB_STATUS status;                 /* The event status.                */
  uint8_t num_param_id;               /* Number of rejected UCI Param ID  */
  uint8_t param_ids[UWB_MAX_NUM_IDS]; /* UCI Param ID          */
  uint16_t tlv_size;                  /* The length of TLV    */
} tUWB_SET_APP_CONFIG_REVT;

/* the data type associated with UWB_GET_CORE_CONFIG_REVT */
typedef struct {
  tUWB_STATUS status;    /* The event status.    */
  uint8_t no_of_ids;
  uint8_t p_param_tlvs[UCI_MAX_PAYLOAD_SIZE]; /* TLV                  */
  uint16_t tlv_size;     /* The length of TLV    */
} tUWB_GET_CORE_CONFIG_REVT;

/* the data type associated with UWB_GET_APP_CONFIG_REVT */
typedef struct {
  tUWB_STATUS status;    /* The event status.    */
  uint8_t no_of_ids;
  uint8_t p_param_tlvs[UCI_MAX_PAYLOAD_SIZE]; /* TLV                  */
  uint16_t tlv_size;     /* The length of TLV    */
} tUWB_GET_APP_CONFIG_REVT;
/* the data type associated with UWB_DEVICE_RESET_REVT */
typedef struct {
  tUWB_STATUS status;                 /* The event status.                */
  uint8_t resetConfig;               /* Vendor Specific Reset Config */
} tUWB_DEVICE_RESET_REVT;

/* the data type associated with UWB_DEVICE_STATUS_REVT */
typedef struct {
  uint8_t status;
}tUWB_DEVICE_STATUS_REVT;

/* the data type associated with UWB_CORE_GEN_ERR_STATUS_REVT */
typedef struct {
  uint8_t status;
}tUWB_CORE_GEN_ERR_STATUS_REVT;

/* the data type associated with UWB_SESSION_GET_COUNT_REVT */
typedef struct {
  uint8_t status;     /* device status             */
  uint8_t count;      /* active session count      */
} tUWB_SESSION_GET_COUNT_REVT;

/* the data type associated with UWB_SESSION_GET_STATE_REVT */
typedef struct {
  uint8_t status;     /* device status             */
  uint8_t session_state;      /* current session state */
} tUWB_SESSION_GET_STATE_REVT;

/* the data type associated with UWB_SESSION_NTF_REVT */
typedef struct {
  uint32_t session_id;
  uint8_t  state;
  uint8_t reason_code;
}tUWB_SESSION_NTF_REVT;

/* the data type associated with tUWB_GET_DEVICE_INFO_REVT */
typedef struct {
  tUWB_STATUS status;             /* The event status.                  */
  uint16_t uci_version;           /* UCI version                        */
  uint16_t mac_version;           /* MAC version                        */
  uint16_t phy_version;           /* PHY version                        */
  uint16_t uciTest_version;       /* UCI Test specification version     */
  uint8_t vendor_info_len;        /* Device Information length          */
  uint8_t vendor_info[UCI_VENDOR_INFO_MAX_SIZE];     /* Manufacturer specific information  */
}tUWB_GET_DEVICE_INFO_REVT;

typedef struct {
  uint8_t mac_addr[8];
  uint8_t  status;
  uint8_t nLos; /* non line of sight */
  uint16_t distance;
  uint16_t aoa_azimuth;
  uint8_t aoa_azimuth_FOM;
  uint16_t aoa_elevation;
  uint8_t aoa_elevation_FOM;
  uint16_t aoa_dest_azimuth;
  uint8_t aoa_dest_azimuth_FOM;
  uint16_t aoa_dest_elevation;
  uint8_t aoa_dest_elevation_FOM;
  uint8_t slot_index;
  uint8_t rfu[TWR_RANGE_MEASR_RFU];
} tUWB_TWR_RANGING_MEASR;

typedef struct {
  uint8_t mac_addr[8];
  uint8_t  frame_type;
  uint8_t nLos; /* non line of sight */
  uint16_t aoa_azimuth;
  uint8_t aoa_azimuth_FOM;
  uint16_t aoa_elevation;
  uint8_t aoa_elevation_FOM;
  uint64_t timeStamp;  /* Time stamp */
  uint32_t  blink_frame_number;  /* blink frame number received from tag/master anchor */
  uint8_t rfu[TDOA_RANGE_MEASR_RFU];
  uint8_t device_info_size;  /* Size of Device Specific Information */
  uint8_t* device_info;  /* Device Specific Information */
  uint8_t blink_payload_size;  /* Size of Blink Payload Data */
  uint8_t* blink_payload_data;  /* Blink Payload Data */
} tUWB_TDoA_RANGING_MEASR;

typedef struct {
  uint16_t vendor_specific_length;
  uint8_t vendor_specific_info[MAX_VENDOR_INFO_LENGTH];
  uint8_t* range_data_ntf_buff;
  uint16_t range_data_ntf_buff_len;
}tUWB_RANGE_DATA_VENDOR_EXTENSION;

typedef union {
  tUWB_TWR_RANGING_MEASR twr_range_measr[MAX_NUM_RESPONDERS];
  tUWB_TDoA_RANGING_MEASR tdoa_range_measr[MAX_NUM_OF_TDOA_MEASURES];
}tUWB_RANGING_MEASR;

/* the data type associated with UWB_RANGE_DATA_REVT */
typedef struct {
  uint16_t range_data_len;
  uint32_t seq_counter;
  uint32_t session_id;
  uint8_t rcr_indication;
  uint32_t curr_range_interval;
  uint8_t ranging_measure_type;
  uint8_t rfu;
  uint8_t mac_addr_mode_indicator;
  uint8_t reserved[8];
  uint8_t no_of_measurements;
  tUWB_RANGING_MEASR ranging_measures;
  tUWB_RANGE_DATA_VENDOR_EXTENSION vendor_info;
}tUWB_RANGE_DATA_REVT;

/* the data type associated with UWB_RANGING_GET_COUNT_REVT */
typedef struct {
  uint8_t status;     /* response status */
  uint32_t count;      /* ranging count in particular session duration */
} tUWB_GET_RANGE_COUNT_REVT;

/* the data type associated with UWB_CORE_GET_DEVICE_CAPABILITY_REVT */
typedef struct {
  tUWB_STATUS status;    /* The event status.    */
  uint8_t no_of_tlvs;
  uint8_t tlv_buffer[UCI_MAX_PAYLOAD_SIZE]; /* TLV                  */
  uint16_t tlv_buffer_len;     /* The length of TLV    */
} tUWB_CORE_GET_DEVICE_CAPABILITY_REVT;

/* the data type associated with UWB_SESSION_UPDATE_MULTICAST_LIST_NTF_REVT */
typedef struct {
  uint32_t session_id;
  uint8_t remaining_list;
  uint8_t no_of_controlees;
  uint32_t subsession_id_list[MAX_NUM_CONTROLLEES];
  uint8_t  status_list[MAX_NUM_CONTROLLEES];
} tUWB_SESSION_UPDATE_MULTICAST_LIST_NTF_REVT;

/* the data type associated with UWB_BLINK_DATA_TX_NTF_REVT */
typedef struct {
  uint8_t repetition_count_status;     /* repetition count status */
} tUWB_SEND_BLINK_DATA_NTF_REVT;

/* the data type associated with UWB_DATA_TRANSFER_STATUS_NTF_REVT */
typedef struct {
  uint32_t session_id;
  uint8_t status;
}tUWB_DATA_TRANSFER_STATUS_NTF_REVT;

/* the data type associated with UWB_DATA_RECV_REVT */
typedef struct {
  uint32_t session_id;
  uint8_t address[EXTENDED_ADDRESS_LEN];
  uint16_t data_len;
  uint8_t data[4196];
}tUWB_RX_DATA_REVT;


typedef union {
  tUWB_STATUS status; /* The event status. */
  tUWB_ENABLE_REVT enable;
  tUWB_GET_DEVICE_INFO_REVT sGet_device_info;
  tUWB_DEVICE_STATUS_REVT sDevice_status;
  tUWB_CORE_GEN_ERR_STATUS_REVT sCore_gen_err_status;
  tUWB_DEVICE_RESET_REVT sDevice_reset;
  tUWB_SET_CORE_CONFIG_REVT sCore_set_config;
  tUWB_GET_CORE_CONFIG_REVT sCore_get_config;
  tUWB_SESSION_NTF_REVT sSessionStatus;
  tUWB_GET_APP_CONFIG_REVT sApp_get_config;
  tUWB_SET_APP_CONFIG_REVT sApp_set_config;
  tUWB_SESSION_GET_COUNT_REVT sGet_session_cnt;
  tUWB_SESSION_GET_STATE_REVT sGet_session_state;
  tUWB_RANGE_DATA_REVT  sRange_data;
  tUWB_GET_RANGE_COUNT_REVT sGet_range_cnt;
  tUWB_CORE_GET_DEVICE_CAPABILITY_REVT sGet_device_capability;
  tUWB_SESSION_UPDATE_MULTICAST_LIST_NTF_REVT sMulticast_list_ntf;
  tUWB_SEND_BLINK_DATA_NTF_REVT sSend_blink_data_ntf;
  tUWB_DATA_TRANSFER_STATUS_NTF_REVT sData_xfer_status;
  tUWB_RX_DATA_REVT sRcvd_data;
} tUWB_RESPONSE;

/* Data types associated with all RF test Events */

/* the data type associated with UWB_GET_TEST_CONFIG_REVT */
typedef struct {
  tUWB_STATUS status;    /* The event status.    */
  uint8_t no_of_ids;
  uint8_t p_param_tlvs[UCI_MAX_PAYLOAD_SIZE]; /* TLV                  */
  uint16_t tlv_size;     /* The length of TLV    */
} tUWB_TEST_GET_CONFIG_REVT;

typedef struct {
  tUWB_STATUS status;                 /* The event status.                */
  uint8_t num_param_id;               /* Number of rejected UCI Param ID  */
  uint8_t param_ids[UWB_MAX_NUM_IDS]; /* UCI Param ID          */
  uint16_t tlv_size;                  /* The length of TLV    */
} tUWB_TEST_SET_CONFIG_REVT;


/* The data type associated with  UWB_RF_TEST_DATA_REVT*/
typedef struct {
  uint16_t length;
  uint8_t data[255];
} tUWB_RF_TEST_DATA;

typedef union {
  tUWB_STATUS status; /* The event status. */
  tUWB_TEST_GET_CONFIG_REVT sTest_get_config;
  tUWB_TEST_SET_CONFIG_REVT sTest_set_config;
  tUWB_RF_TEST_DATA  sRf_test_result;
} tUWB_TEST_RESPONSE;


/*************************************
**  RESPONSE Callback Functions
**************************************/
typedef void(tUWB_RESPONSE_CBACK)(tUWB_RESPONSE_EVT event,
                                  tUWB_RESPONSE* p_data);
typedef void(tUWB_TEST_RESPONSE_CBACK)(tUWB_TEST_RESPONSE_EVT event,
                                  tUWB_TEST_RESPONSE* p_data);
typedef void(tUWB_RAW_CBACK)(tUWB_RAW_EVT event, uint16_t data_len,
                             uint8_t* p_data);

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
extern tUWB_STATUS UWB_Enable(tUWB_RESPONSE_CBACK* p_cback, tUWB_TEST_RESPONSE_CBACK* p_test_cback);

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
tUWB_STATUS UWB_RegisterExtCallBack(tUWB_RAW_CBACK* p_ext_cback);

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
extern void UWB_Disable(void);

/*******************************************************************************
**
** Function         UWB_Init
**
** Description      This function initializes control blocks for UWB
**
** Returns          nothing
**
*******************************************************************************/

extern void UWB_Init(tHAL_UWB_CONTEXT* p_hal_entry_cntxt);

/*******************************************************************************
**
** Function         UWB_GetDeviceInfo
**
** Description      This function is called to get Device Info
**
** Parameters       None
**
** Returns          none
**
*******************************************************************************/
extern tUWB_STATUS UWB_GetDeviceInfo();

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
extern tUWB_STATUS UWB_DeviceResetCommand(uint8_t resetConfig);

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
extern tUWB_STATUS UWB_SetCoreConfig(uint8_t tlv_size, uint8_t* p_param_tlvs);

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
extern tUWB_STATUS UWB_GetCoreConfig(uint8_t num_ids, uint8_t* p_param_ids);

/*******************************************************************************
**
** Function         UWB_SessionInit
**
** Description      This function is called to send  session init command
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_SessionInit(uint32_t session_id,uint8_t sessionType);

/*******************************************************************************
**
** Function         UWB_SessionDeInit
**
** Description      This function is called to send  session DeInit command
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_SessionDeInit(uint32_t session_id);

/*******************************************************************************
**
** Function         UWB_GetSessionCount
**
** Description      This function is called to send get session count command
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_GetSessionCount();

/*******************************************************************************
**
** Function         UWB_GetAppConfig
**
** Description      This function is called to retrieve the parameter TLV from
**                  UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_GET_APP_CONFIG_REVT.
**
** Parameters       session_id - All APP configurations belonging to this Session ID
**                  num_ids - the number of parameter IDs
**                  length - Length of app parameter ID
**                  p_param_ids - the parameter ID list.
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_GetAppConfig(uint32_t session_id, uint8_t num_ids, uint8_t length, uint8_t* p_param_ids);

/*******************************************************************************
**
** Function         UWB_SetAppConfig
**
** Description      This function is called to set the parameter TLV from
**                  UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_GET_APP_CONFIG_REVT.
**
** Parameters       session_id - All APP configurations belonging to this Session ID
**                  num_ids - the number of parameter IDs
**                  length - Length of app parameter data
**                  p_data - SetAppConfig TLV data
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_SetAppConfig(uint32_t session_id, uint8_t num_ids, uint8_t length, uint8_t* p_data);

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
extern tUWB_STATUS UWB_StartRanging(uint32_t session_id);

/*******************************************************************************
**
** Function         UWB_StopRanging
**
** Description      This function is called to send the range stop command to
**                  UWBC. The response from UWBC is reported to the given
**                  tUWB_RESPONSE_CBACK.
**
** Parameters       session_id -  Session ID for which ranging shall stop
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_StopRanging(uint32_t session_id);

/*******************************************************************************
**
** Function         UWB_GetRangingCount
**
** Description      This function is called to send get ranging count command
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_GetRangingCount(uint32_t session_id);

/*******************************************************************************
**
** Function         UWB_GetSessionStatus
**
** Description      This function is called to send get session status command
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_GetSessionStatus(uint32_t session_id);

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
extern tUWB_STATUS UWB_CoreGetDeviceCapability(void);

/*******************************************************************************
**
** Function         UWB_GetDebugLog
**
** Description      This function is called to ge th debug log
**
** Parameters       None
**
** Returns          none
**
*******************************************************************************/
extern void UWB_GetDebugLog();

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
extern tUWB_STATUS UWB_MulticastListUpdate(uint32_t session_id, uint8_t action, uint8_t noOfControlees, uint16_t* shortAddressList, uint32_t* subSessionIdList);

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
extern tUWB_STATUS UWB_SendBlinkData(uint32_t session_id, uint8_t repetition_count, uint8_t app_data_len, uint8_t* app_data);

/*******************************************************************************
**
** Function         UWB_SendRawCommand
**
** Description      This function is called to send the given raw command to
**                  UWBC. The response from UWBC is reported to the given
**                  tUWB_RAW_CBACK.
**
** Parameters       p_data - The command buffer
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_SendRawCommand(UWB_HDR* p_data, tUWB_RAW_CBACK* p_cback);


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
                                           uint16_t data_len, uint8_t* p_data);

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
void UWB_EnableConformanceTest(uint8_t enable);

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
extern const uint8_t* UWB_GetStatusName(tUWB_STATUS status);




/****************** RF TEST FUNCTIONS********************************************/


/*******************************************************************************
**
** Function         UWB_TestGetConfig
**
** Description      This function is called to retrieve the parameter TLV from
**                  UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_GET_TEST_CONFIG_REVT.
**
** Parameters       session_id - All TEST configurations belonging to this Session ID
**                  num_ids - the number of parameter IDs
**                  length - Length of test parameter ID
**                  p_param_ids - the parameter ID list.
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_TestGetConfig(uint32_t session_id, uint8_t num_ids, uint8_t length, uint8_t* p_param_ids);

/*******************************************************************************
**
** Function         UWB_SetTestConfig
**
** Description      This function is called to set the parameter TLV from
**                  UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_SET_APP_CONFIG_REVT.
**
** Parameters       session_id - All TEST configurations belonging to this Session ID
**                  num_ids - the number of parameter IDs
**                  length - Length of test parameter data
**                  p_data - SetAppConfig TLV data
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_SetTestConfig(uint32_t session_id, uint8_t num_ids, uint8_t length, uint8_t* p_data);


/*******************************************************************************
**
** Function         UWB_TestPeriodicTx
**
** Description      This function is called send per rx command from
**                  UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_TEST_PERIODIC_TX_REVT.
**
** Parameters       length - Length of psdu data.
**                  p_data - psdu data
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_TestPeriodicTx(uint16_t length, uint8_t* p_data);

/*******************************************************************************
**
** Function         UWB_TestPerRx
**
** Description      This function is called send per rx command from
**                  UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_TEST_PER_RX_REVT
**
** Parameters       length - Length of psdu data.
**                  p_data - psdu data
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_TestPerRx(uint16_t length, uint8_t* p_data);

/*******************************************************************************
**
** Function         UWB_TestUwbLoopBack
**
** Description      This function is called send uwb loop back command to
**                  UWBC.
**
** Parameters       length - Length of psdu data.
**                  p_data - psdu data
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_TestUwbLoopBack(uint16_t length, uint8_t* p_data);

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
extern tUWB_STATUS UWB_TestRx(void);

/*******************************************************************************
**
** Function         UWB_TestStopSession
**
** Description      This function is called send per rx/tx stop command from
**                  UWBC. The response from UWBC is reported by
**                  tUWB_RESPONSE_CBACK as UWB_TEST_STOP_SESSION_REVT.
**
** Parameters       None
**
** Returns          tUWB_STATUS
**
*******************************************************************************/
extern tUWB_STATUS UWB_TestStopSession(void);

#endif /* UWB_API_H */
