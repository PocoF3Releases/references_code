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

#ifndef _UWB_JNI_EXT_TYPES_
#define _UWB_JNI_EXT_TYPES_

#include "SyncEvent.h"
#include "uwa_api.h"

#define UWB_CMD_TIMEOUT       4000  // UWB command time out

typedef struct {
  uint8_t  mapping;
  uint8_t dec_status;
  uint8_t n_los; /* non line of sight */
  uint16_t first_path_index;
  uint16_t main_path_index;
  uint8_t  snr_main_path; /* snr Main path */
  uint8_t  snr_first_path; /* snr First path */
  uint16_t snr_total;
  uint16_t rssi;
  uint8_t cir_main_power[4];
  uint8_t cir_first_path_power[4];
  uint16_t noise_variance;
  uint16_t cfo;
  uint16_t aoaPhase;
} tRFRAME_MEASURES;

typedef struct {
  uint8_t len;
  uint32_t session_id;
  uint8_t no_of_meas;
  tRFRAME_MEASURES *rframe_meas;
}tRFRAME_LOG_NTF;

typedef struct {
  uint32_t session_id;
  uint8_t  sched_status;
  uint32_t no_of_successful_sched;
  uint32_t no_of_unsuccessful_sched;
  uint8_t  priority;
} tSCHED_SESSION_DATA;

typedef struct {
  uint8_t len;
  uint8_t session_count;
  tSCHED_SESSION_DATA *sched_session_data;
} tSCHED_STATUS_NTF;

typedef struct {
  uint8_t status;
  uint8_t count_remaining;
  uint8_t binding_state;
} tSE_BIND_NTF;

typedef struct {
  uint8_t status;
  uint16_t se_instruction_code;
  uint16_t se_error_code;
} tSE_COMM_ERROR_NTF;

typedef struct {
  uint8_t status;
  uint8_t se_binding_count;
  uint8_t uwbs_binding_count;
} tSE_GET_BINDING_STATUS_NTF;

typedef struct {
  uint8_t status;
} tBINDING_STATUS_NTF;

typedef struct {
  uint8_t status;
  uint16_t loop_count;
  uint16_t loop_pass_count;
} tSE_TEST_LOOP_NTF;

typedef struct {
  uint8_t status;
  uint8_t wtx_count;
} tSE_CONNECTIVITY_TEST_NTF;

typedef struct {
  uint8_t status;
  uint8_t payload_len;
  uint8_t* payload;
} tUART_CONNECTIVITY_TEST_NTF;

enum {
    CALIB_PARAM_VCO_PLL_ID = 0x00,
    CALIB_PARAM_TX_POWER_ID = 0x01,
    CALIB_PARAM_XTAL_CAP_38_4_MHZ = 0x02,
    CALIB_PARAM_RSSI_CALIB_CONSTANT1 = 0x03,
    CALIB_PARAM_RSSI_CALIB_CONSTANT2 = 0x04,
    CALIB_PARAM_SNR_CALIB_CONSTANT = 0x05,
    CALIB_PARAM_MANUAL_TX_POW_CTRL = 0x06,
    CALIB_PARAM_PDOA_OFFSET1 = 0x07,
    CALIB_PARAM_PA_PPA_CALIB_CTRL = 0x08,
    CALIB_PARAM_TX_TEMPARATURE_COMP = 0x09,
    CALIB_PARAM_PDOA_OFFSET2 = 0x0A
};

enum {
    CHANNEL_NUM_5 = 5,
    CHANNEL_NUM_6 = 6,
    CHANNEL_NUM_8 = 8,
    CHANNEL_NUM_9 = 9
};

enum {
    DO_CALIB_PARAM_VCO_PLL_ID = 0x00,
    DO_CALIB_PARAM_PA_PPA_CALIB_CTRL = 0x01
};

typedef struct NxpFeatureData {
    SyncEvent NxpConfigEvt;
    tUWA_STATUS wstatus;
    uint8_t rsp_data[UCI_MAX_PAYLOAD_SIZE];
    uint8_t rsp_len;
    uint8_t ntf_data[UCI_MAX_PAYLOAD_SIZE];
    uint8_t ntf_len;
}NxpFeatureData_t;

enum {
    SESSION_RSN_TERMINATION_ON_MAX_RR_RETRY = 0x01,
    SESSION_RSN_MAX_RANGING_BLOCK_REACHED,
    SESSION_RSN_URSK_TTL_MAX_VALUE_REACHED,
    SESSION_RSN_IN_BAND_STOP_RCVD = 0x83
};

#endif