/*
*
* Copyright 2019-2020 NXP.
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
 *  This file contains the definition from UCI specification
 *
 ******************************************************************************/

#ifndef UWB_UCI_EXT_DEFS_H
#define UWB_UCI_EXT_DEFS_H

#include <stdint.h>


#define EXT_UCI_PAYLOAD_SUPPORT  1
#define EXT_MAX_UCI_PAYLOAD_SIZE 4096

#define EXT_LENGTH_INDCATOR_BIT 0x80 //1000 0000
#define EXT_UCI_LENGTH_SHIFT 8


/* UCI Data Format:
 * byte 0: MT(0) PBF RFU
 * byte 1: Extended payload Length indicator
 * byte 2: Data Length
 * byte 3: Data Length */
#define EXT_UCI_DATA_PBLD_HDR(p, pbf, len)             \
  *(p)++ = (uint8_t)(((pbf) << UCI_PBF_SHIFT));       \
  *(p)++ = EXT_LENGTH_INDCATOR_BIT;                    \
   *(p)++ = (uint8_t)(len);\
  *(p)++ = (uint8_t)(((len) >> EXT_UCI_LENGTH_SHIFT));


/**********************************************
 * UCI proprietary group(UCI_GID_PROPRIETARY)- 7: Opcodes and size of commands
 **********************************************/
#define EXT_UCI_MSG_CORE_DEVICE_INIT                       0x00
#define EXT_UCI_MSG_SE_DO_BIND                             0x01
#define EXT_UCI_MSG_DBG_DATA_LOGGER_NTF                    0x02
#define EXT_UCI_MSG_DBG_BIN_LOG_NTF                        0x03
#define EXT_UCI_MSG_DBG_CIR0_LOG_NTF                       0x04
#define EXT_UCI_MSG_DBG_CIR1_LOG_NTF                       0x05
#define EXT_UCI_MSG_DBG_GET_ERROR_LOG                      0x06
#define EXT_UCI_MSG_DBG_PSDU_LOG_NTF                       0x09
#define EXT_UCI_MSG_SE_GET_BINDING_COUNT                   0x0A
#define EXT_UCI_MSG_DBG_RFRAME_LOG_NTF                     0x0B
#define EXT_UCI_MSG_SE_GET_BINDING_STATUS                  0x0C
#define EXT_UCI_MSG_SE_DO_TEST_LOOP                        0x0D
#define EXT_UCI_MSG_SE_DO_TEST_CONNECTIVITY                0x0E
#define EXT_UCI_MSG_GET_ALL_UWB_SESSIONS                   0x0F
#define EXT_UCI_MSG_SE_COMM_ERROR_NTF                      0x10
#define EXT_UCI_MSG_SET_CALIBRATION                        0x11
#define EXT_UCI_MSG_GET_CALIBRATION                        0x12
#define EXT_UCI_MSG_BINDING_STATUS_NTF                     0x13
#define EXT_UCI_MSG_SCHEDULER_STATUS_NTF                   0x14
#define EXT_UCI_MSG_UWB_SESSION_KDF_NTF                    0x15
#define EXT_UCI_MSG_UWB_WIFI_COEX_IND_NTF                  0x16
#define EXT_UCI_MSG_WLAN_UWB_IND_ERR_NTF                   0x17
#define EXT_UCI_MSG_DO_CALIBRATION                         0x18
#define EXT_UCI_MSG_QUERY_TEMPERATURE                      0x19
#define EXT_UCI_MSG_UART_CONNECTIVITY_TEST_CMD             0x1B
#define EXT_UCI_MSG_GENERATE_TAG_CMD                       0x1C
#define EXT_UCI_MSG_VERIFY_TAG_CMD                         0x1D
#define EXT_UCI_MSG_CONFIGURE_AUTH_TAG_OPTIONS_CMD         0x1E
#define EXT_UCI_MSG_CONFIGURE_AUTH_TAG_VERSION_CMD         0x1F
#define EXT_UCI_MSG_CALIBRATION_INTEGRATION_PROTECTION_CMD 0x20
#define EXT_UCI_MSG_QUERY_UWB_TIMESTAMP_CMD                0x23
#define EXT_UCI_MSG_URSK_DELETION_REQ_CMD                  0x24

/**********************************************
 * UWB DATA CONTROL Group Opcode-9 Opcodes and size of command
 **********************************************/
#define EXT_UCI_MSG_DATA_CREDIT_NTF                        0x00
#define EXT_UCI_MSG_DATA_TRANSFER_STATUS_NTF               0x01
/**********************************************************
 * UCI Extention Parameter IDs : Device Information
 *********************************************************/
#define EXTENDED_DEVICE_INFO_ID             0xE3

#define UCI_EXT_PARAM_ID_DEVICE_NAME        0x00
#define UCI_EXT_PARAM_ID_FW_VERSION         0x01
#define UCI_EXT_PARAM_ID_PLL_VCO_CODE       0x02

/**********************************************************
 * UCI Extention Parameter IDs : Device Configurations
 *********************************************************/
#define EXTENDED_DEVICE_CONFIG_ID           0xE4

#define UCI_EXT_PARAM_ID_DELAY_CALIBRATION        0x00
#define UCI_EXT_PARAM_ID_POS_ALGO_CFG6            0x01
#define UCI_EXT_PARAM_ID_DPD_WAKEUP_SRC           0x02
#define UCI_EXT_PARAM_ID_WTX_COUNT                0x03
#define UCI_EXT_PARAM_ID_DPD_ENTRY_TIMEOUT        0x04
#define UCI_EXT_PARAM_ID_NXP_EXTENDED_NTF_CONFIG  0x33

#define UCI_EXT_PARAM_ID_WTX_COUNT_LEN                 0x01
#define UCI_EXT_PARAM_ID_DPD_ENTRY_TIMEOUT_LEN         0x02
#define UCI_EXT_PARAM_ID_NXP_EXTENDED_NTF_CONFIG_LEN   0x01

/***********************************************************
 * UCI Extention Parameter IDs : Application Configurations
 **********************************************************/
/* Extended ID for Extended Application configurations */
#define EXTENDED_APP_CONFIG_ID  0xE3

#define UCI_EXT_PARAM_ID_TOA_MODE                       0x00
#define UCI_EXT_PARAM_ID_CIR_CAPTURE_MODE               0x01
#define UCI_EXT_PARAM_ID_MAC_PAYLOAD_ENCRYPTION         0x02
#define UCI_EXT_PARAM_ID_RX_ANTENNA_POLARIZATION_OPTION 0x03
#define UCI_EXT_PARAM_ID_RFU_1                          0x04
#define UCI_EXT_PARAM_ID_SESSION_SYNC_ATTEMPTS          0x05
#define UCI_EXT_PARAM_ID_SESSION_SCHED_ATTEMPTS         0x06
#define UCI_EXT_PARAM_ID_SCHED_STATUS_NTF               0x07
#define UCI_EXT_PARAM_ID_TX_POWER_DELTA_FCC             0x08
#define UCI_EXT_PARAM_ID_TEST_KDF_FEATURE               0x09
#define UCI_EXT_PARAM_ID_DUAL_AOA_PREAMBLE_STS          0x0A
#define UCI_EXT_PARAM_ID_TX_POWER_TEMP_COMPENSATION     0x0B

/***********************************************************
 * UCI Extention Parameter IDs : Debug Configurations
 **********************************************************/
#define EXTENDED_DEBUG_CONFIG_ID  0xE4

#define UCI_EXT_PARAM_ID_THREAD_SECURE           0x00
#define UCI_EXT_PARAM_ID_THREAD_SECURE_ISR       0x01
#define UCI_EXT_PARAM_ID_THREAD_NON_SECURE_ISR   0x02
#define UCI_EXT_PARAM_ID_THREAD_SHELL            0x03
#define UCI_EXT_PARAM_ID_THREAD_PHY              0x04
#define UCI_EXT_PARAM_ID_THREAD_RANGING          0x05
#define UCI_EXT_PARAM_ID_CIR_LOG_NTF             0x11
#define UCI_EXT_PARAM_ID_DATA_LOGGER_NTF         0x10
#define UCI_EXT_PARAM_ID_PSDU_LOG_NTF            0x12
#define UCI_EXT_PARAM_ID_RFRAME_LOG_NTF          0x13

/***********************************************************
 * UCI Extention Test Parameter IDs : Test Configurations
 **********************************************************/
#define EXTENDED_TEST_CONFIG_ID  0xE5

#define UCI_EXT_TEST_PARAM_ID_RSSI_AVG_FILT_CNT         0x00
#define UCI_EXT_TEST_PARAM_ID_RSSI_CALIBRATION_OPTION   0x01
#define UCI_EXT_TEST_PARAM_ID_AGC_GAIN_VAL_RX           0x02
#define UCI_EXT_TEST_PARAM_ID_TEST_SESSION_STS_KEY_OPT  0x03

/* Binary log levels*/
#define DBG_LOG_LEVEL_DISABLE             0x0000
#define DBG_LOG_LEVEL_ERROR               0x0001
#define DBG_LOG_LEVEL_WARNG               0x0002
#define DBG_LOG_LEVEL_TMSTAMP             0x0004
#define DBG_LOG_LEVEL_SEQNUM              0x0008
#define DBG_LOG_LEVEL_INFO1               0x0010
#define DBG_LOG_LEVEL_INFO2               0x0020
#define DBG_LOG_LEVEL_INFO3               0x0040

/* UWB proprietary status codes */
#define UCI_STATUS_BINDING_SUCCESS        0x50
#define UCI_STATUS_BINDING_FAILURE        0x51
#define UCI_STATUS_BINDING_LIMIT_REACHED  0x52
#define UCI_STATUS_NO_ESE                 0x70
#define UCI_STATUS_ESE_RSP_TIMEOUT        0x71
#define UCI_STATUS_ESE_RECOVERY_FAILURE   0x72
#define UCI_STATUS_ESE_RECOVERY_SUCCESS   0x73
#define UCI_STATUS_SE_APDU_CMD_FAIL       0x74
#define UCI_STATUS_SE_AUTH_FAIL           0x75

/* UWB proprietary calibration parameter codes */
#define UCI_CALIB_VCO_PLL               0x00
#define UCI_CALIB_TX_POWER_ID           0x01
#define UCI_CALIB_XTAL_CAP_38_4_MHZ     0x02
#define UCI_CALIB_RSSI_CALIB_CONSTANT1  0x03
#define UCI_CALIB_RSSI_CALIB_CONSTANT2  0x04
#define UCI_CALIB_SNR_CALIB_CONSTANT    0x05
#define UCI_CALIB_MANUAL_TX_POW_CTRL    0x06
#define UCI_CALIB_PDOA_OFFSET           0x07
#define UCI_CALIB_PA_PPA_CALIB_CTRL     0x08
#endif /* UWB_UCI_EXT_DEFS_H */
