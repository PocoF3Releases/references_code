/******************************************************************************
 *
 *  Copyright 2018-2020 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
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
#ifndef ANDROID_HARDWARE_HAL_NXPUWB_V1_0_H
#define ANDROID_HARDWARE_HAL_NXPUWB_V1_0_H
#include <vector>
#include <string>

#define UWB_UCI_NXP_HELIOS_HARDWARE_MODULE_ID "uwb_uci.helios"
#define MAX_IOCTL_TRANSCEIVE_CMD_LEN 256
#define MAX_IOCTL_TRANSCEIVE_RESP_LEN 256
#define MAX_ATR_INFO_LEN 128

enum {
  HAL_UWB_STATUS_OK = 0x00,
  HAL_UWB_STATUS_ERR_TRANSPORT = 0x01,
  HAL_UWB_STATUS_ERR_CMD_TIMEOUT = 0x02
};
enum {
  HAL_UWB_IOCTL_DUMP_FW_LOG = 0,
  HAL_UWB_IOCTL_ESE_RESET,
  HAL_UWB_IOCTL_CORE_INIT
};

/*
 * Data structures provided below are used of Hal Ioctl calls
 */
/*
 * uwb_uci_ExtnCmd_t shall contain data for commands used for transceive command
 * in ioctl
 */
typedef struct {
  uint16_t cmd_len;
  uint8_t p_cmd[MAX_IOCTL_TRANSCEIVE_CMD_LEN];
} uwb_uci_ExtnCmd_t;

/*
 * uwb_uci_ExtnRsp_t shall contain response for command sent in transceive
 * command
 */
typedef struct {
  uint16_t rsp_len;
  uint8_t p_rsp[MAX_IOCTL_TRANSCEIVE_RESP_LEN];
} uwb_uci_ExtnRsp_t;

/*
 * dumpFwLog_t contains Fw Log enable/Disable Flag status
 *
 */
typedef enum dumpFwLog {
  DUMP_FW_DEBUG_LOG = 1,
  DUMP_FW_CIR_LOG,
  DUMP_FW_DEBUG_CIR_LOG,
  DUMP_FW_CRASH_LOG,
  DUMP_FW_DISABLE_ALL_LOG
} dumpFwLog_t;

/*
 * InputData_t :ioctl has multiple subcommands
 * Each command has corresponding input data which needs to be populated in this
 */
typedef union {
  uint8_t           status;
  uint8_t           halType;
  dumpFwLog_t       fwLogType;
  uwb_uci_ExtnCmd_t uciCmd;
  uint32_t          timeoutMilliSec;
  bool              recovery;
}InputData_t;

/*
 * uwb_uci_ExtnInputData_t :Apart from InputData_t, there are context data
 * which is required during callback from stub to proxy.
 * To avoid additional copy of data while propagating from libuwb to Adaptation
 * and Uwbstub to ucihal, common structure is used. As a sideeffect, context
 * data
 * is exposed to libuwb (Not encapsulated).
 */
typedef struct {
  /*context to be used/updated only by users of proxy & stub of Uwb.hal
  * i.e, UwbAdaptation & hardware/interface/Uwb.
  */
  void* context;
  InputData_t data;
  uint8_t data_source;
  long level;
} uwb_uci_ExtnInputData_t;

/*
 * outputData_t :ioctl has multiple commands/responses
 * This contains the output types for each ioctl.
 */
typedef union {
  uint32_t status;
  uwb_uci_ExtnRsp_t uciRsp;
  uint8_t nxpUciAtrInfo[MAX_ATR_INFO_LEN];
} outputData_t;

/*
 * uwb_uci_ExtnOutputData_t :Apart from outputData_t, there are other
 * information
 * which is required during callback from stub to proxy.
 * For ex (context, result of the operation , type of ioctl which was
 * completed).
 * To avoid additional copy of data while propagating from libuwb to Adaptation
 * and Uwbstub to ucihal, common structure is used. As a sideeffect, these data
 * is exposed(Not encapsulated).
 */
typedef struct {
  /*ioctlType, result & context to be used/updated only by users of
   * proxy & stub of Uwb.hal.
   * i.e, UwbAdaptation & hardware/interface/Uwb
   * These fields shall not be used by libuwb or halimplementation*/
  uint64_t ioctlType;
  uint32_t result;
  void* context;
  outputData_t data;
} uwb_uci_ExtnOutputData_t;

/*
 * uwb_uci_IoctlInOutData_t :data structure for input & output
 * to be sent for ioctl command. input is populated by client/proxy side
 * output is provided from server/stub to client/proxy
 */
typedef struct {
  uwb_uci_ExtnInputData_t inp;
  uwb_uci_ExtnOutputData_t out;
} uwb_uci_IoctlInOutData_t;

enum NxpUwbHalStatus {
    /** In case of an error, HCI network needs to be re-initialized */
    HAL_STATUS_OK = 0x00,
};

#endif  // ANDROID_HARDWARE_HAL_NXPUWB_V1_0_H
