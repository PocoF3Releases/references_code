/*
 * Copyright (C) 2012-2019 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <log/log.h>
#include <phNxpLog.h>
#include <cutils/properties.h>
#include <phNxpUciHal.h>
#include <phNxpUciHal_Adaptation.h>
#include <phNxpUciHal_ext.h>
#include <phTmlUwb.h>
#include <phTmlUwb_spi.h>
#include <sys/stat.h>
#include <string.h>
#include "hal_nxpuwb.h"
#include "phNxpConfig.h"
#include <android-base/stringprintf.h>
using android::base::StringPrintf;
using namespace vendor::xiaomi::uwb::V1_0;

/*********************** Global Variables *************************************/
/* UCI HAL Control structure */
phNxpUciHal_Control_t nxpucihal_ctrl;

/* TML Context */
extern phTmlUwb_Context_t* gpphTmlUwb_Context;


bool uwb_debug_enabled = true;
bool uwb_device_initialized = false;
uint32_t timeoutTimerId = 0;
char persistant_log_path[120];

static uint8_t Rx_data[UCI_MAX_DATA_LEN];
/**************** local methods used in this file only ************************/

static void phNxpUciHal_open_complete(tHAL_UWB_STATUS status);
static void phNxpUciHal_write_complete(void* pContext,
                                       phTmlUwb_TransactInfo_t* pInfo);
static void phNxpUciHal_read_complete(void* pContext,
                                      phTmlUwb_TransactInfo_t* pInfo);
static void phNxpUciHal_close_complete(tHAL_UWB_STATUS status);
static void phNxpUciHal_kill_client_thread(
    phNxpUciHal_Control_t* p_nxpucihal_ctrl);
static void* phNxpUciHal_client_thread(void* arg);
extern int phNxpUciHal_fw_download();
static void phNxpUciHal_print_response_status(uint8_t* p_rx_data, uint16_t p_len);

/******************************************************************************
 * Function         phNxpUciHal_client_thread
 *
 * Description      This function is a thread handler which handles all TML and
 *                  UCI messages.
 *
 * Returns          void
 *
 ******************************************************************************/
static void* phNxpUciHal_client_thread(void* arg) {
  phNxpUciHal_Control_t* p_nxpucihal_ctrl = (phNxpUciHal_Control_t*)arg;
  phLibUwb_Message_t msg;

  NXPLOG_UCIHAL_D("thread started");

  p_nxpucihal_ctrl->thread_running = 1;

  while (p_nxpucihal_ctrl->thread_running == 1) {
    /* Fetch next message from the UWB stack message queue */
    if (phDal4Uwb_msgrcv(p_nxpucihal_ctrl->gDrvCfg.nClientId, &msg, 0, 0) ==
        -1) {
      NXPLOG_UCIHAL_E("UWB client received bad message");
      continue;
    }

    if (p_nxpucihal_ctrl->thread_running == 0) {
      break;
    }

    switch (msg.eMsgType) {
      case PH_LIBUWB_DEFERREDCALL_MSG: {
        phLibUwb_DeferredCall_t* deferCall =
            (phLibUwb_DeferredCall_t*)(msg.pMsgData);

        REENTRANCE_LOCK();
        deferCall->pCallback(deferCall->pParameter);
        REENTRANCE_UNLOCK();

        break;
      }

      case UCI_HAL_OPEN_CPLT_MSG: {
        REENTRANCE_LOCK();
        if (nxpucihal_ctrl.p_uwb_stack_cback != NULL) {
          /* Send the event */
          (*nxpucihal_ctrl.p_uwb_stack_cback)(HAL_UWB_OPEN_CPLT_EVT,
                                              HAL_UWB_STATUS_OK);
        }
        REENTRANCE_UNLOCK();
        break;
      }

      case UCI_HAL_CLOSE_CPLT_MSG: {
        REENTRANCE_LOCK();
        if (nxpucihal_ctrl.p_uwb_stack_cback != NULL) {
          /* Send the event */
          (*nxpucihal_ctrl.p_uwb_stack_cback)(HAL_UWB_CLOSE_CPLT_EVT,
                                              HAL_UWB_STATUS_OK);
          phNxpUciHal_kill_client_thread(&nxpucihal_ctrl);
        }
        REENTRANCE_UNLOCK();
        break;
      }

      case UCI_HAL_ERROR_MSG: {
        REENTRANCE_LOCK();
        if (nxpucihal_ctrl.p_uwb_stack_cback != NULL) {
          /* Send the event */
          (*nxpucihal_ctrl.p_uwb_stack_cback)(HAL_UWB_ERROR_EVT,
                                              HAL_UWB_ERROR_EVT);
        }
        REENTRANCE_UNLOCK();
        break;
      }
    }
  }

  NXPLOG_UCIHAL_D("NxpUciHal thread stopped");
  pthread_exit(NULL);
  return NULL;
}

/******************************************************************************
 * Function         phNxpUciHal_kill_client_thread
 *
 * Description      This function safely kill the client thread and clean all
 *                  resources.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpUciHal_kill_client_thread(
    phNxpUciHal_Control_t* p_nxpucihal_ctrl) {
  NXPLOG_UCIHAL_D("Terminating phNxpUciHal client thread...");

  p_nxpucihal_ctrl->p_uwb_stack_cback = NULL;
  p_nxpucihal_ctrl->p_uwb_stack_data_cback = NULL;
  p_nxpucihal_ctrl->thread_running = 0;

  return;
}

/******************************************************************************
 * Function         phNxpUciHal_open
 *
 * Description      This function is called by libuwb-uci during the
 *                  initialization of the UWBC. It opens the physical connection
 *                  with UWBC (SR100) and creates required client thread for
 *                  operation.
 *                  After open is complete, status is informed to libuwb-uci
 *                  through callback function.
 *
 * Returns          This function return UWBSTATUS_SUCCES (0) in case of success
 *                  In case of failure returns other failure value.
 *
 ******************************************************************************/
tHAL_UWB_STATUS phNxpUciHal_open(uwb_stack_callback_t* p_cback,
                     uwb_stack_data_callback_t* p_data_cback) {
  phOsalUwb_Config_t tOsalConfig;
  phTmlUwb_Config_t tTmlConfig;
  char* uwb_dev_node = NULL;
  const uint16_t max_len = 260;
  tHAL_UWB_STATUS wConfigStatus = UWBSTATUS_SUCCESS;
  pthread_attr_t attr;
  char FW_debug_log_path[100] = {0,};
  char Data_Logger_log_path[100] = {0,};

  if (nxpucihal_ctrl.halStatus == HAL_STATUS_OPEN) {
    NXPLOG_UCIHAL_E("phNxpUciHal_open already open");
    return UWBSTATUS_SUCCESS;
  }

  /* initialize trace level */
  phNxpLog_InitializeLogLevel();
  /*Create the timer for extns write response*/
  timeoutTimerId = phOsalUwb_Timer_Create();

  if (phNxpUciHal_init_monitor() == NULL) {
    NXPLOG_UCIHAL_E("Init monitor failed");
    return UWBSTATUS_FAILED;
  }

  CONCURRENCY_LOCK();

  memset(&nxpucihal_ctrl, 0x00, sizeof(nxpucihal_ctrl));
  memset(&tOsalConfig, 0x00, sizeof(tOsalConfig));
  memset(&tTmlConfig, 0x00, sizeof(tTmlConfig));
  uwb_dev_node = (char*)nxp_malloc(max_len * sizeof(char));
  if (uwb_dev_node == NULL) {
      NXPLOG_UCIHAL_E("malloc of uwb_dev_node failed ");
      goto clean_and_return;
    } else {
      NXPLOG_UCIHAL_E("Assinging the default helios Node: dev/sr100");
      strcpy(uwb_dev_node, "/dev/sr100");
    }
  /* By default HAL status is HAL_STATUS_OPEN */
  nxpucihal_ctrl.halStatus = HAL_STATUS_OPEN;

  nxpucihal_ctrl.p_uwb_stack_cback = p_cback;
  nxpucihal_ctrl.p_uwb_stack_data_cback = p_data_cback;

  nxpucihal_ctrl.IsFwDebugDump_enabled = false;
  nxpucihal_ctrl.IsCIRDebugDump_enabled = false;
  nxpucihal_ctrl.fw_dwnld_mode = false;
  sprintf(FW_debug_log_path, "%suwb_FW_debug.log", debug_log_path);
  sprintf(Data_Logger_log_path, "%suwb_data_logger.log", debug_log_path);
  if(NULL == (nxpucihal_ctrl.FwDebugLogFile = fopen(FW_debug_log_path, "wb"))) {
    NXPLOG_UCIHAL_E("unable to open log file");
  }
  if(NULL == (nxpucihal_ctrl.DataLoggerFile = fopen(Data_Logger_log_path, "wb"))) {
    NXPLOG_UCIHAL_E("unable to open log file");
  }

  /* Configure hardware link */
  nxpucihal_ctrl.gDrvCfg.nClientId = phDal4Uwb_msgget(0, 0600);
  nxpucihal_ctrl.gDrvCfg.nLinkType = ENUM_LINK_TYPE_SPI; /* For SR100 */
  tTmlConfig.pDevName = (int8_t*)uwb_dev_node;
  tOsalConfig.dwCallbackThreadId = (uintptr_t)nxpucihal_ctrl.gDrvCfg.nClientId;
  tOsalConfig.pLogFile = NULL;
  tTmlConfig.dwGetMsgThreadId = (uintptr_t)nxpucihal_ctrl.gDrvCfg.nClientId;

  /* Initialize TML layer */
  wConfigStatus = phTmlUwb_Init(&tTmlConfig);
  if (wConfigStatus != UWBSTATUS_SUCCESS) {
    NXPLOG_UCIHAL_E("phTmlUwb_Init Failed");
    goto clean_and_return;
  } else {
    if (uwb_dev_node != NULL) {
      free(uwb_dev_node);
      uwb_dev_node = NULL;
    }
  }

  /* Create the client thread */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (pthread_create(&nxpucihal_ctrl.client_thread, &attr,
                     phNxpUciHal_client_thread, &nxpucihal_ctrl) != 0) {
    NXPLOG_UCIHAL_E("pthread_create failed");
    wConfigStatus = phTmlUwb_Shutdown();
    goto clean_and_return;
  }

  CONCURRENCY_UNLOCK();

#if 0
  /* call read pending */
  status = phTmlUwb_Read(
      nxpucihal_ctrl.p_cmd_data, UCI_MAX_DATA_LEN,
      (pphTmlUwb_TransactCompletionCb_t)&phNxpUciHal_read_complete, NULL);
  if (status != UWBSTATUS_PENDING) {
    NXPLOG_UCIHAL_E("TML Read status error status = %x", status);
    wConfigStatus = phTmlUwb_Shutdown();
    wConfigStatus = UWBSTATUS_FAILED;
    goto clean_and_return;
  }
#endif
  pthread_attr_destroy(&attr);
  /* Call open complete */
  phNxpUciHal_open_complete(wConfigStatus);
  return wConfigStatus;

clean_and_return:
  CONCURRENCY_UNLOCK();
  if (uwb_dev_node != NULL) {
    free(uwb_dev_node);
    uwb_dev_node = NULL;
  }

  /* Report error status */
  (*nxpucihal_ctrl.p_uwb_stack_cback)(HAL_UWB_OPEN_CPLT_EVT, HAL_UWB_ERROR_EVT);

  nxpucihal_ctrl.p_uwb_stack_cback = NULL;
  nxpucihal_ctrl.p_uwb_stack_data_cback = NULL;
  phNxpUciHal_cleanup_monitor();
  nxpucihal_ctrl.halStatus = HAL_STATUS_CLOSE;
  pthread_attr_destroy(&attr);
  return wConfigStatus;
}

/******************************************************************************
 * Function         phNxpUciHal_open_complete
 *
 * Description      This function inform the status of phNxpUciHal_open
 *                  function to libuwb-uci.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpUciHal_open_complete(tHAL_UWB_STATUS status) {
  static phLibUwb_Message_t msg;

  if (status == UWBSTATUS_SUCCESS) {
    msg.eMsgType = UCI_HAL_OPEN_CPLT_MSG;
    nxpucihal_ctrl.hal_open_status = true;
    nxpucihal_ctrl.halStatus = HAL_STATUS_OPEN;
  } else {
    msg.eMsgType = UCI_HAL_ERROR_MSG;
  }

  msg.pMsgData = NULL;
  msg.Size = 0;

  phTmlUwb_DeferredCall(gpphTmlUwb_Context->dwCallbackThreadId,
                        (phLibUwb_Message_t*)&msg);

  return;
}

/******************************************************************************
 * Function         phNxpUciHal_write
 *
 * Description      This function write the data to UWBC through physical
 *                  interface (e.g. SPI) using the SR100 driver interface.
 *                  Before sending the data to UWBC, phNxpUciHal_write_ext
 *                  is called to check if there is any extension processing
 *                  is required for the UCI packet being sent out.
 *
 * Returns          It returns number of bytes successfully written to UWBC.
 *
 ******************************************************************************/
tHAL_UWB_STATUS phNxpUciHal_write(uint16_t data_len, const uint8_t* p_data) {
  if (nxpucihal_ctrl.halStatus != HAL_STATUS_OPEN) {
    return UWBSTATUS_FAILED;
  }

  CONCURRENCY_LOCK();
  uint16_t len = phNxpUciHal_write_unlocked(data_len, p_data);
  CONCURRENCY_UNLOCK();

  /* No data written */
  return len;
}

/******************************************************************************
 * Function         phNxpUciHal_write_unlocked
 *
 * Description      This is the actual function which is being called by
 *                  phNxpUciHal_write. This function writes the data to UWBC.
 *                  It waits till write callback provide the result of write
 *                  process.
 *
 * Returns          It returns number of bytes successfully written to UWBC.
 *
 ******************************************************************************/
tHAL_UWB_STATUS phNxpUciHal_write_unlocked(uint16_t data_len, const uint8_t* p_data) {
  tHAL_UWB_STATUS status;
  phNxpUciHal_Sem_t cb_data;
  /* Create the local semaphore */
  if (phNxpUciHal_init_cb_data(&cb_data, NULL) != UWBSTATUS_SUCCESS) {
    NXPLOG_UCIHAL_D("phNxpUciHal_write_unlocked Create cb data failed");
    data_len = 0;
    goto clean_and_return;
  }

  /* Create local copy of cmd_data */
  memcpy(nxpucihal_ctrl.p_cmd_data, p_data, data_len);
  nxpucihal_ctrl.cmd_len = data_len;

  data_len = nxpucihal_ctrl.cmd_len;
  status = phTmlUwb_Write(
      (uint8_t*)nxpucihal_ctrl.p_cmd_data, (uint16_t)nxpucihal_ctrl.cmd_len,
      (pphTmlUwb_TransactCompletionCb_t)&phNxpUciHal_write_complete,
      (void*)&cb_data);
  if (status != UWBSTATUS_PENDING) {
    NXPLOG_UCIHAL_E("write_unlocked status error");
    data_len = 0;
    goto clean_and_return;
  }

  /* Wait for callback response */
  if (SEM_WAIT(cb_data)) {
    NXPLOG_UCIHAL_E("write_unlocked semaphore error");
    data_len = 0;
    goto clean_and_return;
  }

clean_and_return:
  phNxpUciHal_cleanup_cb_data(&cb_data);
  return data_len;
}

/******************************************************************************
 * Function         phNxpUciHal_write_complete
 *
 * Description      This function handles write callback.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpUciHal_write_complete(void* pContext,
                                       phTmlUwb_TransactInfo_t* pInfo) {
  phNxpUciHal_Sem_t* p_cb_data = (phNxpUciHal_Sem_t*)pContext;

  if (pInfo->wStatus == UWBSTATUS_SUCCESS) {
    NXPLOG_UCIHAL_D("write successful status = 0x%x", pInfo->wStatus);
  } else {
    NXPLOG_UCIHAL_E("write error status = 0x%x", pInfo->wStatus);
  }
  p_cb_data->status = pInfo->wStatus;

  SEM_POST(p_cb_data);

  return;
}
/******************************************************************************
 * Function         phNxpUciHal_read_complete
 *
 * Description      This function is called whenever there is an UCI packet
 *                  received from UWBC. It could be RSP or NTF packet. This
 *                  function provide the received UCI packet to libuwb-uci
 *                  using data callback of libuwb-uci.
 *                  There is a pending read called from each
 *                  phNxpUciHal_read_complete so each a packet received from
 *                  UWBC can be provide to libuwb-uci.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpUciHal_read_complete(void* pContext,
                                      phTmlUwb_TransactInfo_t* pInfo) {
  tHAL_UWB_STATUS status;
  uint8_t gid = 0, oid = 0, pbf = 0;
  UNUSED(pContext);
  if (nxpucihal_ctrl.read_retry_cnt == 1) {
    nxpucihal_ctrl.read_retry_cnt = 0;
  }
  if (pInfo->wStatus == UWBSTATUS_SUCCESS) {
    NXPLOG_UCIHAL_D("read successful status = 0x%x", pInfo->wStatus);
    nxpucihal_ctrl.p_rx_data = pInfo->pBuff;
    nxpucihal_ctrl.rx_data_len = pInfo->wLength;

    gid = nxpucihal_ctrl.p_rx_data[0] & UCI_GID_MASK;
    oid = nxpucihal_ctrl.p_rx_data[1] & UCI_OID_MASK;
    pbf = (nxpucihal_ctrl.p_rx_data[0] & UCI_PBF_MASK) >> UCI_PBF_SHIFT;
    nxpucihal_ctrl.isSkipPacket = 0;
#if(NXP_UWB_EXTNS == TRUE)
    if((nxpucihal_ctrl.IsCIRDebugDump_enabled) || (nxpucihal_ctrl.IsFwDebugDump_enabled) || ((gid == UCI_GID_PROPRIETARY) && (oid == EXT_UCI_MSG_DBG_GET_ERROR_LOG))) {
      phNxpUciHal_dump_log(gid, oid, pbf);
    }
#endif
    if(!uwb_device_initialized) {
      if((gid == UCI_GID_CORE) && (oid == UCI_MSG_CORE_DEVICE_STATUS_NTF)) {
        nxpucihal_ctrl.uwbc_device_state = nxpucihal_ctrl.p_rx_data[UCI_RESPONSE_STATUS_OFFSET];
        if(nxpucihal_ctrl.uwbc_device_state == UWB_DEVICE_INIT || nxpucihal_ctrl.uwbc_device_state == UWB_DEVICE_READY) {
          nxpucihal_ctrl.isSkipPacket = 1;
          SEM_POST(&(nxpucihal_ctrl.dev_status_ntf_wait));
        }
      }
    }
    if (nxpucihal_ctrl.hal_ext_enabled == 1){
      if((nxpucihal_ctrl.p_rx_data[0x00] & 0xF0) == 0x40){
        nxpucihal_ctrl.isSkipPacket = 1;
        if(nxpucihal_ctrl.p_rx_data[UCI_RESPONSE_STATUS_OFFSET] == UCI_STATUS_OK){
          nxpucihal_ctrl.ext_cb_data.status = UWBSTATUS_SUCCESS;
        } else if ((gid == UCI_GID_CORE) && (oid == UCI_MSG_CORE_SET_CONFIG)){
          NXPLOG_UCIHAL_E(" status = 0x%x",nxpucihal_ctrl.p_rx_data[UCI_RESPONSE_STATUS_OFFSET]);
          /* check if any configurations are not supported then ignore the UWBSTATUS_FEATURE_NOT_SUPPORTED stastus code*/
          nxpucihal_ctrl.ext_cb_data.status = phNxpUciHal_process_ext_rsp(nxpucihal_ctrl.rx_data_len, nxpucihal_ctrl.p_rx_data);
        } else {
          nxpucihal_ctrl.ext_cb_data.status = UWBSTATUS_FAILED;
          NXPLOG_UCIHAL_E("command failed! status = 0x%x",nxpucihal_ctrl.p_rx_data[UCI_RESPONSE_STATUS_OFFSET]);
        }
        usleep(1);
        SEM_POST(&(nxpucihal_ctrl.ext_cb_data));
      } else if(gid == UCI_GID_CORE && oid == UCI_MSG_CORE_GENERIC_ERROR_NTF && nxpucihal_ctrl.p_rx_data[4] == UCI_STATUS_COMMAND_RETRY){
        nxpucihal_ctrl.ext_cb_data.status = UWBSTATUS_COMMAND_RETRANSMIT;
        SEM_POST(&(nxpucihal_ctrl.ext_cb_data));
      }
    }
    /* if Debug Notification, then skip sending to application */
    if(nxpucihal_ctrl.isSkipPacket == 0) {
      phNxpUciHal_print_response_status(nxpucihal_ctrl.p_rx_data, nxpucihal_ctrl.rx_data_len);
      /* Read successful, send the event to higher layer */
         if ((nxpucihal_ctrl.p_uwb_stack_data_cback != NULL) && (nxpucihal_ctrl.rx_data_len <= UCI_MAX_PAYLOAD_LEN)) {
        (*nxpucihal_ctrl.p_uwb_stack_data_cback)(nxpucihal_ctrl.rx_data_len,
                                                 nxpucihal_ctrl.p_rx_data);
      }
    }
  } else {
    NXPLOG_UCIHAL_E("read error status = 0x%x", pInfo->wStatus);
  }

  if (nxpucihal_ctrl.halStatus == HAL_STATUS_CLOSE) {
    return;
  }
  /* Disable junk data check for each UCI packet*/
  if(nxpucihal_ctrl.fw_dwnld_mode) {
    if((gid == UCI_GID_CORE) && (oid == UCI_MSG_CORE_DEVICE_STATUS_NTF)){
      nxpucihal_ctrl.fw_dwnld_mode = false;
    }
  }
  /* Read again because read must be pending always.*/
  status = phTmlUwb_Read(
      Rx_data, UCI_MAX_DATA_LEN,
      (pphTmlUwb_TransactCompletionCb_t)&phNxpUciHal_read_complete, NULL);
  if (status != UWBSTATUS_PENDING) {
    NXPLOG_UCIHAL_E("read status error status = %x", status);
    /* TODO: Not sure how to handle this ? */
  }
  return;
}

/******************************************************************************
 * Function         phNxpUciHal_close
 *
 * Description      This function close the UWBC interface and free all
 *                  resources.This is called by libuwb-uci on UWB service stop.
 *
 * Returns          Always return UWBSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
tHAL_UWB_STATUS phNxpUciHal_close() {
  tHAL_UWB_STATUS status;

  if (nxpucihal_ctrl.halStatus == HAL_STATUS_CLOSE) {
    NXPLOG_UCIHAL_E("phNxpUciHal_close is already closed, ignoring close");
    return UWBSTATUS_FAILED;
  }

/* FW debug log dump file closed */
  if(nxpucihal_ctrl.FwDebugLogFile != NULL){
    fclose(nxpucihal_ctrl.FwDebugLogFile);
  }
  if(nxpucihal_ctrl.DataLoggerFile != NULL){
    fclose(nxpucihal_ctrl.DataLoggerFile);
  }
  if(nxpucihal_ctrl.CIR0logFile != NULL){
    fclose(nxpucihal_ctrl.CIR0logFile);
  }
  if(nxpucihal_ctrl.CIR1logFile != NULL){
    fclose(nxpucihal_ctrl.CIR1logFile);
  }
  if(nxpucihal_ctrl.RngNtfFile != NULL){
    fclose(nxpucihal_ctrl.RngNtfFile);
  }
  nxpucihal_ctrl.IsFwDebugDump_enabled = false;
  nxpucihal_ctrl.IsCIRDebugDump_enabled = false;

  CONCURRENCY_LOCK();

  nxpucihal_ctrl.halStatus = HAL_STATUS_CLOSE;

  if (NULL != gpphTmlUwb_Context->pDevHandle) {
    phNxpUciHal_close_complete(UWBSTATUS_SUCCESS);
    /* Abort any pending read and write */
    status = phTmlUwb_ReadAbort();
    status = phTmlUwb_WriteAbort();

    phOsalUwb_Timer_Cleanup();

    status = phTmlUwb_Shutdown();

    phDal4Uwb_msgrelease(nxpucihal_ctrl.gDrvCfg.nClientId);

    memset(&nxpucihal_ctrl, 0x00, sizeof(nxpucihal_ctrl));

    NXPLOG_UCIHAL_D("phNxpUciHal_close - phOsalUwb_DeInit completed");
  }

  CONCURRENCY_UNLOCK();

  phNxpUciHal_cleanup_monitor();

  /* Return success always */
  return UWBSTATUS_SUCCESS;
}
/******************************************************************************
 * Function         phNxpUciHal_close_complete
 *
 * Description      This function inform libuwb-uci about result of
 *                  phNxpUciHal_close.
 *
 * Returns          void.
 *
 ******************************************************************************/
void phNxpUciHal_close_complete(tHAL_UWB_STATUS status) {
  static phLibUwb_Message_t msg;

  if (status == UWBSTATUS_SUCCESS) {
    msg.eMsgType = UCI_HAL_CLOSE_CPLT_MSG;
  } else {
    msg.eMsgType = UCI_HAL_ERROR_MSG;
  }
  msg.pMsgData = NULL;
  msg.Size = 0;

  phTmlUwb_DeferredCall(gpphTmlUwb_Context->dwCallbackThreadId, &msg);

  return;
}

/******************************************************************************
 * Function         phNxpUciHal_getVendorConfig
 *
 * Description      This function can be used by HAL to inform
 *                 to update vendor configuration parametres
 *
 * Returns          void.
 *
 ******************************************************************************/

void phNxpUciHal_getVendorConfig(vendor::xiaomi::uwb::V1_0::UwbConfig& config) {
  NXPLOG_UCIHAL_D(" phNxpUciHal_getVendorConfig Enter..");
  std::array<uint8_t, NXP_MAX_CONFIG_STRING_LEN> buffer;
  unsigned long num = 0;
  buffer.fill(0);
  memset(&config, 0x00, sizeof(vendor::xiaomi::uwb::V1_0::UwbConfig));

  if(GetNxpConfigNumValue(NAME_ANTENNA_PAIR_SELECTION_CONFIG1, &num, sizeof(num))){
    config.antennaPairSelectionConfig1 = (uint16_t)num;
    NXPLOG_UCIHAL_D("%s: NAME_ANTENNA_PAIR_SELECTION_CONFIG1: %x", __func__, config.antennaPairSelectionConfig1);
  } else {
    NXPLOG_UCIHAL_D("%s: NAME_ANTENNA_PAIR_SELECTION_CONFIG1: failed %x", __func__, (uint16_t)num);
  }
  if(GetNxpConfigNumValue(NAME_ANTENNA_PAIR_SELECTION_CONFIG2, &num, sizeof(num))){
    config.antennaPairSelectionConfig2 = (uint16_t)num;
    NXPLOG_UCIHAL_D("%s: NAME_ANTENNA_PAIR_SELECTION_CONFIG2: %x", __func__, config.antennaPairSelectionConfig2);
  } else {
    NXPLOG_UCIHAL_D("%s: NAME_ANTENNA_PAIR_SELECTION_CONFIG2: failed %x", __func__, (uint16_t)num);
  }
  if(GetNxpConfigNumValue(NAME_ANTENNA_PAIR_SELECTION_CONFIG3, &num, sizeof(num))){
    config.antennaPairSelectionConfig3 = (uint16_t)num;
    NXPLOG_UCIHAL_D("%s: NAME_ANTENNA_PAIR_SELECTION_CONFIG3: %x", __func__, config.antennaPairSelectionConfig3);
  } else {
    NXPLOG_UCIHAL_D("%s: NAME_ANTENNA_PAIR_SELECTION_CONFIG3: failed %x", __func__, (uint16_t)num);
  }
  if(GetNxpConfigNumValue(NAME_ANTENNA_PAIR_SELECTION_CONFIG4, &num, sizeof(num))){
    config.antennaPairSelectionConfig4 = (uint16_t)num;
    NXPLOG_UCIHAL_D("%s: NAME_ANTENNA_PAIR_SELECTION_CONFIG4: %x", __func__, config.antennaPairSelectionConfig4);
  } else {
    NXPLOG_UCIHAL_D("%s: NAME_ANTENNA_PAIR_SELECTION_CONFIG4: failed %x", __func__, (uint16_t)num);
  }
  if(GetNxpConfigNumValue(NAME_ANTENNA_PAIR_SELECTION_CONFIG5, &num, sizeof(num))){
    config.uwbXtalConfig.resize(2);
    config.uwbXtalConfig[0] = num & 0xff;
    config.uwbXtalConfig[1]=  (num >> 8);
    // TODO need to change this with configValue.antennaPairSelectionConfig5
    NXPLOG_UCIHAL_D("%s: NAME_ANTENNA_PAIR_SELECTION_CONFIG5: %x", __func__, (uint16_t)num);
  } else {
    NXPLOG_UCIHAL_D("%s: NAME_ANTENNA_PAIR_SELECTION_CONFIG5: failed %x", __func__, (uint16_t)num);
  }
}


static tHAL_UWB_STATUS phNxpUciHal_applyVendorConfig() {
  NXPLOG_UCIHAL_D(" phNxpUciHal_applyVendorConfig Enter..");
  std::array<uint8_t, NXP_MAX_CONFIG_STRING_LEN> buffer;
  uint8_t* vendorConfig = NULL;
  tHAL_UWB_STATUS status;
  buffer.fill(0);
  long retlen = 0;

  //xiaomi modify begin
  uint8_t ext_cmd_retry_cnt = 0;
  uint8_t cali_cap[9] = {0x2E,0x11,0x00,0x05,0x09,0x02,0xFF,0xFF,0x21};
  uint8_t Channel5[10] = {0x2E,0X11,0X00,0X06,0X05,0X01,0xFF,0xFF,0xFF,0xFF};
  uint8_t Channel9[10] = {0x2E,0X11,0X00,0X06,0X09,0X01,0xFF,0xFF,0xFF,0xFF};
  uint8_t pdoa_offset[14] = {0x2E,0x11,0x00,0x0A,0x09,0x0A,0X00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  int magiclen = 0;
  uint8_t magicStr[PROPERTY_VALUE_MAX] = {0};

  long retlen_tof = 0;
  uint8_t tofCali[8] = {0};
  uint8_t pdoaCali[8] = {0};
  //xiaomi modify end

  if (GetNxpConfigByteArrayValue(NAME_UWB_CORE_EXT_DEVICE_DEFAULT_CONFIG, (char*)buffer.data(), buffer.size(), &retlen)) {
    if (retlen > 0) {
      vendorConfig = buffer.data();

      //read tof calibration
      //todo : only for main antenna tof calibration.
      if (GetTofConfigByteArrayValue(NAME_UWB_FACTORY_TOF_CALIBRATION_CONFIG, (char*)tofCali, sizeof(tofCali), &retlen_tof)){
          if (retlen_tof == 8){
              vendorConfig[8] = tofCali[1];
              vendorConfig[9] = tofCali[0];
              vendorConfig[14] = tofCali[3];
              vendorConfig[15] = tofCali[2];
          }
      }
      status = phNxpUciHal_send_ext_cmd(retlen,vendorConfig);
      NXPLOG_UCIHAL_D(" phNxpUciHal_send_ext_cmd :: status value for %s is %d ", NAME_UWB_CORE_EXT_DEVICE_DEFAULT_CONFIG, status);
      if(status != UWBSTATUS_SUCCESS) {
        return status;
      }
    }
  }

  if (GetNxpConfigByteArrayValue(NAME_NXP_UWB_XTAL_38MHZ_CONFIG, (char*)buffer.data(), buffer.size(), &retlen)) {
    if (retlen > 0) {
      vendorConfig = buffer.data();
      status = phNxpUciHal_send_ext_cmd(retlen,vendorConfig);
      NXPLOG_UCIHAL_D(" phNxpUciHal_send_ext_cmd :: status value for %s is %d ", NAME_NXP_UWB_XTAL_38MHZ_CONFIG, status);
      if(status != UWBSTATUS_SUCCESS) {
        return status;
      }
    }
  }

  if (GetNxpConfigByteArrayValue(NAME_UWB_RSSI_CH5_CONFIG, (char*)buffer.data(), buffer.size(), &retlen)) {
    if (retlen > 0) {
      vendorConfig = buffer.data();
      status = phNxpUciHal_send_ext_cmd(retlen,vendorConfig);
      NXPLOG_UCIHAL_D(" phNxpUciHal_send_ext_cmd :: status value for %s is %d ", NAME_UWB_RSSI_CH5_CONFIG, status);
      if(status != UWBSTATUS_SUCCESS) {
        return status;
      }
    }
  }

  if (GetNxpConfigByteArrayValue(NAME_UWB_RSSI_CH9_CONFIG, (char*)buffer.data(), buffer.size(), &retlen)) {
    if (retlen > 0) {
      vendorConfig = buffer.data();
      status = phNxpUciHal_send_ext_cmd(retlen,vendorConfig);
      NXPLOG_UCIHAL_D(" phNxpUciHal_send_ext_cmd :: status value for %s is %d ", NAME_UWB_RSSI_CH9_CONFIG, status);
      if(status != UWBSTATUS_SUCCESS) {
        return status;
      }
    }
  }

  magiclen = property_get("persist.vendor.uwb.calibration",(char*)magicStr, "");
  NXPLOG_UCIHAL_D("magiclen is %d " , magiclen);
  if(magiclen == 5){  //从prop中读
    NXPLOG_UCIHAL_D("read calibration conf from prop");
    cali_cap[6] = magicStr[0];
    cali_cap[7] = magicStr[0];
    Channel5[6] = magicStr[1];
    Channel5[7] = magicStr[2];
    Channel5[8] = magicStr[1];
    Channel5[9] = magicStr[2];
    Channel9[6] = magicStr[3];
    Channel9[7] = magicStr[4];
    Channel9[8] = magicStr[3];
    Channel9[9] = magicStr[4];    
    NXPLOG_UCIHAL_D("prop data is %d " , cali_cap[7]);

    if(phNxpUciHal_send_ext_cmd(sizeof(cali_cap),cali_cap) <= 0)
    {
      NXPLOG_UCIHAL_D("SEND cali_cap failed !!!!");
    }
    if(phNxpUciHal_send_ext_cmd(sizeof(Channel5),Channel5) <= 0)
    {
      NXPLOG_UCIHAL_D("SEND Channel5 failed !!!!");
    }
    if(phNxpUciHal_send_ext_cmd(sizeof(Channel9),Channel9) <= 0)
    {
      NXPLOG_UCIHAL_D("SEND Channel9 failed !!!!");
    }
  }
  else{//从文件中读
    NXPLOG_UCIHAL_D("read calibration conf from path");
    if(GetCaliConfigByteArrayValue(NAME_UWB_FACTORY_CALIBRATION_CONFIG, (char*)buffer.data(), buffer.size(), &retlen)){
        NXPLOG_UCIHAL_D("parse caliconfig !");
        if(retlen > 0){
            vendorConfig = buffer.data();
            ext_cmd_retry_cnt = 0;
            NXPLOG_UCIHAL_D("len is %ld",retlen);
            if(retlen == 5){
                cali_cap[6] = vendorConfig[0];
                cali_cap[7] = vendorConfig[0];
                Channel5[6] = vendorConfig[1];
                Channel5[7] = vendorConfig[2];
                Channel5[8] = vendorConfig[1];
                Channel5[9] = vendorConfig[2];
                Channel9[6] = vendorConfig[3];
                Channel9[7] = vendorConfig[4];
                Channel9[8] = vendorConfig[3];
                Channel9[9] = vendorConfig[4];

                if(phNxpUciHal_send_ext_cmd(sizeof(cali_cap),cali_cap) <= 0)
                {
                  NXPLOG_UCIHAL_D("SEND cali_cap failed !!!!");
                }
                if(phNxpUciHal_send_ext_cmd(sizeof(Channel5),Channel5) <= 0)
                {
                  NXPLOG_UCIHAL_D("SEND Channel5 failed !!!!");
                }
                if(phNxpUciHal_send_ext_cmd(sizeof(Channel9),Channel9) <= 0)
                {
                  NXPLOG_UCIHAL_D("SEND Channel9 failed !!!!");
                }
	    }
        }
    }
  }

  if (GetPDOAConfigByteArrayValue(NAME_UWB_PDOA_CALIBRATION_CONFIG, (char*)pdoaCali, 8, &retlen)) {
    if (retlen > 0) {
      //send ch5 pdoa1
      pdoa_offset[0] = 0x2E;
      pdoa_offset[1] = 0x11;
      pdoa_offset[2] = 0x00;
      pdoa_offset[3] = 0x0A;
      pdoa_offset[4] = 0x05;
      pdoa_offset[5] = 0x07;
      pdoa_offset[6] = pdoaCali[1];
      pdoa_offset[7] = pdoaCali[0];
      ext_cmd_retry_cnt = 0;
      do {
        status = phNxpUciHal_send_ext_cmd(14,pdoa_offset);
        ext_cmd_retry_cnt++;
      } while(status == UWBSTATUS_COMMAND_RETRANSMIT && ext_cmd_retry_cnt <= MAX_COMMAND_RETRY_COUNT);
      NXPLOG_UCIHAL_D(" phNxpUciHal_send_ext_cmd :: status value for ch5 pdoa1_offset is %d ", status);
      if (status != UWBSTATUS_SUCCESS) {
        return status;
      }
      //ch5 pdoa2
      pdoa_offset[4] = 0x05;
      pdoa_offset[5] = 0x0A;
      pdoa_offset[6] = pdoaCali[3];
      pdoa_offset[7] = pdoaCali[2];
      ext_cmd_retry_cnt = 0;
      do {
        status = phNxpUciHal_send_ext_cmd(14,pdoa_offset);
        ext_cmd_retry_cnt++;
      } while(status == UWBSTATUS_COMMAND_RETRANSMIT && ext_cmd_retry_cnt <= MAX_COMMAND_RETRY_COUNT);
      NXPLOG_UCIHAL_D(" phNxpUciHal_send_ext_cmd :: status value for ch5 paod2_offset is %d ",  status);
      if(status != UWBSTATUS_SUCCESS) {
        return status;
      }
      //ch9 pdoa1_offset
      pdoa_offset[4] = 0x09;
      pdoa_offset[5] = 0x07;
      pdoa_offset[6] = pdoaCali[5];
      pdoa_offset[7] = pdoaCali[4];
      ext_cmd_retry_cnt = 0;
      do {
        status = phNxpUciHal_send_ext_cmd(14,pdoa_offset);
        ext_cmd_retry_cnt++;
      } while(status == UWBSTATUS_COMMAND_RETRANSMIT && ext_cmd_retry_cnt <= MAX_COMMAND_RETRY_COUNT);
      NXPLOG_UCIHAL_D(" phNxpUciHal_send_ext_cmd :: status value for ch9 pdoa1_offset is %d ", status);
      if (status != UWBSTATUS_SUCCESS) {
        return status;
      }
      //ch9 pdoa2_offset
      pdoa_offset[4] = 0x09;
      pdoa_offset[5] = 0x0A;
      pdoa_offset[6] = pdoaCali[7];
      pdoa_offset[7] = pdoaCali[6];
      ext_cmd_retry_cnt = 0;
      do {
        status = phNxpUciHal_send_ext_cmd(14,pdoa_offset);
        ext_cmd_retry_cnt++;
      } while(status == UWBSTATUS_COMMAND_RETRANSMIT && ext_cmd_retry_cnt <= MAX_COMMAND_RETRY_COUNT);
      NXPLOG_UCIHAL_D(" phNxpUciHal_send_ext_cmd :: status value for ch9 pdoa2_offst is %d ", status);
      if (status != UWBSTATUS_SUCCESS) {
        return status;
      }
    }
  }

  //xiaomi modify end
  for(int i = 1;i <= 10;i++) {
    std::string str = NAME_NXP_CORE_CONF_BLK;
    std::string value = std::to_string(i);
    std::string name = str + value;
    NXPLOG_UCIHAL_D(" phNxpUciHal_applyVendorConfig :: Name of the config block is %s", name.c_str());
    if (GetNxpConfigByteArrayValue(name.c_str(), (char*)buffer.data(), buffer.size(), &retlen)) {
      if (retlen <= 0) continue;
      vendorConfig = buffer.data();
      status = phNxpUciHal_send_ext_cmd(retlen,vendorConfig);
      NXPLOG_UCIHAL_D(" phNxpUciHal_send_ext_cmd :: status value for %s is %d ", name.c_str(),status);
      if(status != UWBSTATUS_SUCCESS) {
        return status;
      }
    }
    else {
      NXPLOG_UCIHAL_D(" phNxpUciHal_applyVendorConfig::%s not available in the config file", name.c_str());
    }
  }
  return UWBSTATUS_SUCCESS;
}

static tHAL_UWB_STATUS phNxpUciHal_uwb_reset() {
  tHAL_UWB_STATUS status;
  uint8_t buffer[] = {0x20, 0x00, 0x00, 0x01, 0x00};
  status = phNxpUciHal_send_ext_cmd(sizeof(buffer), buffer);
  if(status != UWBSTATUS_SUCCESS) {
    return status;
  }
  return UWBSTATUS_SUCCESS;
}

tHAL_UWB_STATUS phNxpUciHal_coreInitialization(uint8_t recovery) {
  tHAL_UWB_STATUS status;
  uint8_t fwd_retry_count = 0;

  NXPLOG_UCIHAL_D(" Start FW download");
  /* Create the local semaphore */
  if (phNxpUciHal_init_cb_data(&nxpucihal_ctrl.dev_status_ntf_wait, NULL) !=
      UWBSTATUS_SUCCESS) {
    NXPLOG_UCIHAL_E("Create dev_status_ntf_wait failed");
    return UWBSTATUS_FAILED;
  }
  nxpucihal_ctrl.fw_dwnld_mode = true; /* system in FW download mode*/
  uwb_device_initialized = false;
  if(recovery) {
    phTmlUwb_Spi_Reset();
  }
fwd_retry:
      nxpucihal_ctrl.uwbc_device_state = UWB_DEVICE_ERROR;
      status = phNxpUciHal_fw_download();
      if(status == UWBSTATUS_SUCCESS) {
          status = phTmlUwb_Read( Rx_data, UCI_MAX_DATA_LEN,
                    (pphTmlUwb_TransactCompletionCb_t)&phNxpUciHal_read_complete, NULL);
          if (status != UWBSTATUS_PENDING) {
            NXPLOG_UCIHAL_E("read status error status = %x", status);
            goto failure;
          }
          phNxpUciHal_sem_timed_wait(&nxpucihal_ctrl.dev_status_ntf_wait);
          if (nxpucihal_ctrl.dev_status_ntf_wait.status != UWBSTATUS_SUCCESS) {
            NXPLOG_UCIHAL_E("UWB_DEVICE_INIT dev_status_ntf_wait semaphore timed out");
            goto failure;
          }
          if(nxpucihal_ctrl.uwbc_device_state != UWB_DEVICE_INIT) {
            NXPLOG_UCIHAL_E("UWB_DEVICE_INIT not received uwbc_device_state = %x",nxpucihal_ctrl.uwbc_device_state);
            goto failure;
          }
          status = phNxpUciHal_set_board_config();
          if (status != UWBSTATUS_SUCCESS) {
            NXPLOG_UCIHAL_E("%s: Set Board Config Failed", __func__);
            goto failure;
          }
          phNxpUciHal_sem_timed_wait(&nxpucihal_ctrl.dev_status_ntf_wait);
          if (nxpucihal_ctrl.dev_status_ntf_wait.status != UWBSTATUS_SUCCESS) {
            NXPLOG_UCIHAL_E("UWB_DEVICE_READY dev_status_ntf_wait semaphore timed out");
            goto failure;
          }
          if(nxpucihal_ctrl.uwbc_device_state != UWB_DEVICE_READY) {
            NXPLOG_UCIHAL_E("UWB_DEVICE_READY not received uwbc_device_state = %x",nxpucihal_ctrl.uwbc_device_state);
            goto failure;
          }
          NXPLOG_UCIHAL_D("%s: Send device reset", __func__);
          status = phNxpUciHal_uwb_reset();
          if (status != UWBSTATUS_SUCCESS) {
            NXPLOG_UCIHAL_E("%s: device reset Failed", __func__);
            goto failure;
          }
          phNxpUciHal_sem_timed_wait(&nxpucihal_ctrl.dev_status_ntf_wait);
          if (nxpucihal_ctrl.dev_status_ntf_wait.status != UWBSTATUS_SUCCESS) {
            NXPLOG_UCIHAL_E("UWB_DEVICE_READY dev_status_ntf_wait semaphore timed out");
            goto failure;
          }
          if(nxpucihal_ctrl.uwbc_device_state != UWB_DEVICE_READY) {
            NXPLOG_UCIHAL_E("UWB_DEVICE_READY not received uwbc_device_state = %x",nxpucihal_ctrl.uwbc_device_state);
            goto failure;
          }
          status = phNxpUciHal_applyVendorConfig();
          if (status != UWBSTATUS_SUCCESS) {
            NXPLOG_UCIHAL_E("%s: Apply vendor Config Failed", __func__);
            goto failure;
          }
          uwb_device_initialized = true;
      } else if(status == UWBSTATUS_FILE_NOT_FOUND) {
        NXPLOG_UCIHAL_E("FW download File Not found: status= %x", status);
        goto failure;
      } else {
        NXPLOG_UCIHAL_E("FW download is failed FW download recovery starts: status= %x", status);
        fwd_retry_count++;
          if(fwd_retry_count <= FWD_MAX_RETRY_COUNT) {
            phTmlUwb_Chip_Reset();
            usleep(5000);
            goto fwd_retry;
          } else {
            goto failure;
          }
      }
      phNxpUciHal_cleanup_cb_data(&nxpucihal_ctrl.dev_status_ntf_wait);
      return status;
    failure:
        phNxpUciHal_cleanup_cb_data(&nxpucihal_ctrl.dev_status_ntf_wait);
        return UWBSTATUS_FAILED;
}

/******************************************************************************
 * Function         phNxpUciHal_ioctl
 *
 * Description      This function is called by jni when wired mode is
 *                  performed.First SR100 driver will give the access
 *                  permission whether wired mode is allowed or not
 *                  arg (0):
 * Returns          return uwb status, On success
 *                  update the acutual state of operation in arg pointer
 *
 ******************************************************************************/
tHAL_UWB_STATUS phNxpUciHal_ioctl(long arg, void* p_data) {
  NXPLOG_UCIHAL_D("%s : enter - arg = %ld", __func__, arg);

  tHAL_UWB_STATUS status = UWBSTATUS_FAILED;
  uwb_uci_IoctlInOutData_t* ioData = (uwb_uci_IoctlInOutData_t*)(p_data);
  switch (arg) {
#if (NXP_UWB_EXTNS == TRUE)
    case HAL_UWB_IOCTL_DUMP_FW_LOG: {
      if(p_data == NULL) {
          NXPLOG_UCIHAL_E("%s : p_data is NULL", __func__);
          return UWBSTATUS_FAILED;
      }
      switch(ioData->inp.data.fwLogType){
        case DUMP_FW_DEBUG_LOG:
          NXPLOG_UCIHAL_D("%s : Fw Dump Log is enabled", __func__);
          nxpucihal_ctrl.IsFwDebugDump_enabled = true;
          nxpucihal_ctrl.IsCIRDebugDump_enabled = false;
          break;
        case DUMP_FW_CIR_LOG:
          NXPLOG_UCIHAL_D("%s : Cir Dump Log is enabled", __func__);
          nxpucihal_ctrl.IsCIRDebugDump_enabled = true;
          nxpucihal_ctrl.IsFwDebugDump_enabled = false;
          break;
        case DUMP_FW_DEBUG_CIR_LOG:
          NXPLOG_UCIHAL_D("%s : Both FW and CIR Dump Log is enabled", __func__);
          nxpucihal_ctrl.IsFwDebugDump_enabled = true;
          nxpucihal_ctrl.IsCIRDebugDump_enabled = true;
          break;
        case DUMP_FW_CRASH_LOG:
          NXPLOG_UCIHAL_D("%s : FW Crash Log is enabled", __func__);
          status = phNxpUciHal_dump_fw_crash_log();
          if(status == UWBSTATUS_SUCCESS){
            NXPLOG_UCIHAL_D("phNxpUciHal_dump_fw_crash_log success");
          } else{
            NXPLOG_UCIHAL_E("phNxpUciHal_dump_fw_crash_log Failed");
          }
          break;
          case DUMP_FW_DISABLE_ALL_LOG:
          NXPLOG_UCIHAL_D("%s : Disable All FW Debug Log", __func__);
          nxpucihal_ctrl.IsFwDebugDump_enabled = false;
          nxpucihal_ctrl.IsCIRDebugDump_enabled = false;
          break;
        default:
          NXPLOG_UCIHAL_E("HAL_UWB_IOCTL_DUMP_FW_LOG subType not matched...");
          break;
      }
      status = UWBSTATUS_SUCCESS;
    }
      break;
    case HAL_UWB_IOCTL_ESE_RESET:
      phNxpUciHal_eSE_reset();
      break;
    case HAL_UWB_IOCTL_CORE_INIT:
      status = phNxpUciHal_coreInitialization(ioData->inp.data.recovery);
      break;
#endif
    default:
      NXPLOG_UCIHAL_E("%s : Wrong arg = %ld", __func__, arg);
      break;
  }
  return status;
}

static void phNxpUciHal_print_response_status(uint8_t* p_rx_data, uint16_t p_len) {
  uint8_t mt;
  int status_byte;
  const uint8_t response_buf[][30] = {"STATUS_OK",
                                       "STATUS_REJECTED",
                                       "STATUS_FAILED",
                                       "STATUS_SYNTAX_ERROR",
                                       "STATUS_INVALID_PARAM",
                                       "STATUS_INVALID_RANGE",
                                       "STATUS_INAVALID_MSG_SIZE",
                                       "STATUS_UNKNOWN_GID",
                                       "STATUS_UNKNOWN_OID",
                                       "STATUS_RFU",
                                       "STATUS_READ_ONLY",
                                       "STATUS_COMMAND_RETRY"};
  if(p_len > UCI_PKT_HDR_LEN) {
    mt = ((p_rx_data[0]) & UCI_MT_MASK) >> UCI_MT_SHIFT;
    status_byte = p_rx_data[UCI_RESPONSE_STATUS_OFFSET];
    if((mt == UCI_MT_RSP) && (status_byte <= MAX_RESPONSE_STATUS)) {
      NXPLOG_UCIHAL_D(" %s: Response Status = %s", __func__ , response_buf[status_byte]);
    }else{
      NXPLOG_UCIHAL_D(" %s: Response Status = %x", __func__ , status_byte);
    }
  }
}
