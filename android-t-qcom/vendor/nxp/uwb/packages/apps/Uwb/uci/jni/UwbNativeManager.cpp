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

#include "UwbJniInternal.h"
#include "UwbEventManager.h"
#include "NxpUwbManager.h"
#include "UwbAdaptation.h"
#include "SyncEvent.h"
#include "uwb_config.h"
#include "uwb_hal_int.h"
#include "JniLog.h"
#include "ScopedJniEnv.h"
#include "uci_ext_defs.h"
#include <vector>

#define INVALID_SESSION_ID 0xFFFFFFFF

namespace android {

const char* UWB_NATIVE_MANAGER_CLASS_NAME = "com/android/uwb/dhimpl/UwbNativeManager";

bool uwb_debug_enabled = true;

static std::map<unsigned int, SessionRangingData> sAveragedRangingData;
static std::mutex sSessionMutex;

bool gIsUwaEnabled = false;
bool gIsCalibrationOngoing = false;
bool gIsMaxPpmValueAvailable = false;

static SyncEvent sUwaEnableEvent;                       // event for UWA_Enable()
static SyncEvent sUwaDisableEvent;                      // event for UWA_Disable
static SyncEvent sUwaSetConfigEvent;                    // event for Set_Config....
static SyncEvent sUwaSetAppConfigEvent;                 // event for Set_AppConfig....
static SyncEvent sUwaGetConfigEvent;                    // event for Get_Config....
static SyncEvent sUwaGetAppConfigEvent;                 // event for Get_AppConfig....
static SyncEvent sUwaDeviceResetEvent;                  // event for deviceResetEvent
static SyncEvent sUwaRngStartEvent;                     // event for ranging start
static SyncEvent sUwaRngStopEvent;                      // event for ranging stop
static SyncEvent sUwadeviceNtfEvent;                    // event for device status NTF
static SyncEvent sUwaSessionNtfEvent;                   // event for session status NTF
static SyncEvent sUwaSessionInitEvent;                  // event for sessionInit resp
static SyncEvent sUwaSessionDeInitEvent;                // event for sessionDeInit resp
static SyncEvent sUwaGetSessionCountEvent;              // event for get session count response
static SyncEvent sUwaGetDeviceInfoEvent;                // event for get Device Info
static SyncEvent sUwaGetRangingCountEvent;              // event for get ranging count response
static SyncEvent sUwaGetSessionStatusEvent;             // event for get the session status
static SyncEvent sUwaRegisterExtCallBackEvent;          // event for UWA_RegisterExtCallBack()
static SyncEvent sUwaMulticastListUpdateEvent;
static SyncEvent sUwaSendBlinkDataEvent;
static SyncEvent sErrNotify;
static SyncEvent sUwaSendDataEvent;

static deviceInfo_t sUwbDeviceInfo;
static uint8_t sSetAppConfig[UCI_MAX_PAYLOAD_SIZE];
static uint8_t sGetAppConfig[UCI_MAX_PAYLOAD_SIZE];
static uint8_t sNoOfAppConfigIds = 0x00;
static uint8_t sGetCoreConfig[UCI_MAX_PAYLOAD_SIZE];
static uint8_t sSetCoreConfig[UCI_MAX_PAYLOAD_SIZE];
static uint8_t sNoOfCoreConfigIds = 0x00;
static uint16_t sAntennaConfig[5] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
static uint8_t sSessionCount = -1;
static uint32_t sRangingCount = 0;
static uint16_t sGetCoreConfigLen;
static uint16_t sGetAppConfigLen;
static uint16_t sSetAppConfigLen;
static uint8_t sGetAppConfigStatus;
static uint8_t sSetAppConfigStatus;
static uint8_t sSendBlinkDataStatus;
static uint8_t sSendDataStatus;

/* command response status */
static tUWA_SESSION_STATUS_NTF_REVT sCurrentSessionStatus;
static bool sIsSessionStatusNtfReceived = false;
static bool sIsSessionOpInProgress = false;

static bool sSessionInitStatus = false;
static bool sSessionDeInitStatus = false;
static bool sIsDeviceResetDone = false; // whether Reset Performed is Successful is done or not
static bool sRangeStartStatus = false;
static bool sRangeStopStatus = false;
static bool sSetAppConfigRespStatus = false;
static bool sGetAppConfigRespStatus = false;
static bool sMulticastListUpdateStatus = false;

static uint8_t sSessionState = UWB_UNKNOWN_SESSION;

static eUWBD_DEVICE_STATUS_t sDeviceState = UWBD_STATUS_ERROR;
static eDEVICE_STATUS_t sDevState = STATUS_ERROR;

static uint8_t deviceNameLen = 0;
static uint8_t chipIdLen = 16;

static uint8_t sBindingStateNtf = ESE_BINDING_NTF_DEFAULT;
Mutex mBindingStatusCheckMutex;
static uint8_t sUwbMwState = UWB_MW_STATE_DISABLED;
static uint8_t sExpectedSessionState = UWB_UNKNOWN_SESSION;

static UwbEventManager &uwbEventManager = UwbEventManager::getInstance();
static NxpUwbManager &nxpUwbManager = NxpUwbManager::getInstance();

void setBindingStatusNtfRecievedFlag() {
  JNI_TRACE_I("Binding Status NTF is received");
  mBindingStatusCheckMutex.lock();
  sBindingStateNtf = ESE_BINDING_NTF_RECEIVED;
  mBindingStatusCheckMutex.unlock();
}

void setMwState(tMw_State state) {
  sUwbMwState = state;
}

bool IsUwbMwDisabled() {
  return sUwbMwState == UWB_MW_STATE_DISABLED ? true:false;
}

int getDeviceMaxPpmValue(){
  int maxPpmValue = 100;
  if(gIsMaxPpmValueAvailable){
    return sUwbDeviceInfo.nxpDeviceInfo.uwbsMaxPpmValue;
  } else{
    return maxPpmValue;
  }
}

static bool parseDeviceInfo(uint8_t* manfacturerData, uint8_t manufacturerLength) {
    static const char fn[] = "parseDeviceInfo";
    UNUSED(fn);
    uint8_t index = 0;

    if((manufacturerLength <= 0) || (manfacturerData == NULL)) {
        JNI_TRACE_I("%s: manufacturerLength is Zero or manfacturerData is NULL", fn);
        return false;
    }

    while (index < manufacturerLength) {
      uint8_t extParamId = manfacturerData[index++];
      UNUSED(extParamId);
      uint8_t paramId = manfacturerData[index++];
      uint8_t length = manfacturerData[index++];
      JNI_TRACE_I("%s: extParamId: %x, paramId: %x", fn, extParamId, paramId);

      switch(paramId) {
        case UWB_DEVICE_NAME_ID:
            deviceNameLen = length;
            memcpy(sUwbDeviceInfo.nxpDeviceInfo.deviceName, &manfacturerData[index], length);
            break;
        case UWB_FIRMWARE_VERSION_ID:
            {
              uint8_t* fwData = manfacturerData + index;
              STREAM_TO_UINT24(sUwbDeviceInfo.nxpDeviceInfo.fwVersion,fwData);
              break;
            }
        case UWB_NXP_UCI_VERSION_ID:
            {
              uint8_t* uciData = manfacturerData + index;
              STREAM_TO_UINT24(sUwbDeviceInfo.nxpDeviceInfo.vendorUciVersion,uciData);
              break;
            }
        case UWB_NXP_CHIP_ID_ID:
            memcpy(sUwbDeviceInfo.nxpDeviceInfo.chipId, &manfacturerData[index], length);
            break;
        case UWB_FW_BOOT_MODE:
            sUwbDeviceInfo.nxpDeviceInfo.fwBootMode = manfacturerData[index];
            break;
        case UWB_NXP_FIRA_UCI_GENERIC_VERSION:
            {
              uint8_t* firaUciData = manfacturerData + index;
              STREAM_TO_UINT24(sUwbDeviceInfo.nxpDeviceInfo.vendorFiraUciGenericVersion,firaUciData);
              break;
            }
        case UWB_NXP_FIRA_UCI_TEST_VERSION:
            {
              uint8_t* firaUciTestData = manfacturerData + index;
              STREAM_TO_UINT24(sUwbDeviceInfo.nxpDeviceInfo.vendorFiraUciTestVersion,firaUciTestData);
              break;
            }
        case UWB_MAX_PPM_VALUE:
            {
              uint8_t* maxPpmValue = manfacturerData + index;
              STREAM_TO_UINT8(sUwbDeviceInfo.nxpDeviceInfo.uwbsMaxPpmValue,maxPpmValue);
              gIsMaxPpmValueAvailable = true;
              break;
            }
        default:
            JNI_TRACE_I("%s: Unknown ID: %d", fn, paramId);
            break;
      }
      index = index + length;
    }
    return true;
}

/* Function to calculate and update ranging data averaging value into ranging data notification */
static void update_ranging_data_average(tUWA_RANGE_DATA_NTF *rangingDataNtf) {
  static const char fn[] = "update_ranging_data_average";
  UNUSED(fn);
  // Get Current Session Data
  SessionRangingData& sessionData = sAveragedRangingData[rangingDataNtf->session_id];

  // Calculate the Average of N Distances for every Anchor, Where N is Sampling Rate for that Anchor
  for (int i = 0; i < rangingDataNtf->no_of_measurements; i++) {
    // Get current Two way measures object
    tUWA_TWR_RANGING_MEASR& twr_range_measr = rangingDataNtf->ranging_measures.twr_range_measr[i];
    // Get the Current Anchor Distance Queue
    auto &anchorDistanceQueue = sessionData.anchors[i];
    JNI_TRACE_I("%s: Input Distance is: %d", fn, twr_range_measr.distance);
    // If Number of distances in Queue is more than Sampling Rate
    if(anchorDistanceQueue.size() >= sessionData.samplingRate) {
      // Remove items from the queue until items in the queue is one less than sampling rate
      while(anchorDistanceQueue.size() >= sessionData.samplingRate) {
        JNI_TRACE_I("%s: Distance Popped from Queue: %d", fn, anchorDistanceQueue.front());
        anchorDistanceQueue.pop_front();
      }
    }
    // Push the New distance item into the Anchor Distance Queue
    anchorDistanceQueue.push_back(twr_range_measr.distance);
    // Calculate average of items(Except where distance is FFFF)
    // in the Queue and update averaged distance into the distance field
    uint32_t divider = 0;
    uint32_t sum = 0;
    for (auto it = anchorDistanceQueue.begin(); it != anchorDistanceQueue.end(); ++it) {
        if(*it != 0xFFFF) {
            sum = (uint32_t)(sum + *it);
            divider++;
        }
    }
    if(divider > 0) {
        twr_range_measr.distance = sum/divider;
    } else {
        twr_range_measr.distance = 0xFFFF;
    }
    JNI_TRACE_I("%s: Averaged Distance is: %d", fn, twr_range_measr.distance);
  }
}

/*******************************************************************************
**
** Function:        notifyRangeDataNotification
**
** Description:     Notify the Range data  to application
**
** Returns:         void
**
*******************************************************************************/
void notifyRangeDataNotification(tUWA_RANGE_DATA_NTF *ranging_data) {
    static const char fn[] = "notifyRangeDataNotification";
    UNUSED(fn);
    JNI_TRACE_I("%s: Enter", fn);

    if(ranging_data->ranging_measure_type == ONE_WAY_RANGING){
        uwbEventManager.onRangeDataNotificationReceived(ranging_data);
    } else {
        {
            std::unique_lock<std::mutex> lock(sSessionMutex);
            unsigned int session_id = ranging_data->session_id;
            auto it = sAveragedRangingData.find(session_id);
            if (it != sAveragedRangingData.end()) {
                if(sAveragedRangingData[session_id].samplingRate > 1){
                  JNI_TRACE_I("%s: Before Averaging", fn);
                  update_ranging_data_average(ranging_data);
                  JNI_TRACE_I("%s: After Averaging", fn);
                }
            }
        }
        uwbEventManager.onRangeDataNotificationReceived(ranging_data);
    }
}

/*******************************************************************************
**
** Function:        uwaDeviceManagementCallback
**
** Description:     Receive device management events from UCI stack.
**                  dmEvent: Device-management event ID.
**                  eventData: Data associated with event ID.
**
** Returns:         None
**
*******************************************************************************/
static void uwaDeviceManagementCallback(uint8_t dmEvent,
                                 tUWA_DM_CBACK_DATA* eventData) {
  static const char fn[] = "uwaDeviceManagementCallback";
  UNUSED(fn);
  JNI_TRACE_I("%s: enter; event=0x%X", fn, dmEvent);

  switch (dmEvent) {
    case UWA_DM_ENABLE_EVT: /* Result of UWA_Enable */
    {
      SyncEventGuard guard(sUwaEnableEvent);
      JNI_TRACE_I("%s: uwa_dm_enable_EVT; status=0x%X", fn, eventData->status);
      gIsUwaEnabled = eventData->status == UWA_STATUS_OK;
      sUwaEnableEvent.notifyOne();
    } break;

    case UWA_DM_DISABLE_EVT: /* Result of UWA_Disable */
    {
      SyncEventGuard guard(sUwaDisableEvent);
      JNI_TRACE_I("%s: UWA_DM_DISABLE_EVT", fn);
      gIsUwaEnabled = false;
      sUwaDisableEvent.notifyOne();
    } break;
    case UWA_DM_REGISTER_EXT_CB_EVT: /* Result of UWA_Enable */
    {
      SyncEventGuard guard(sUwaRegisterExtCallBackEvent);
      JNI_TRACE_I("%s: uwa_dm_register_ext_cb_EVT; status=0x%X", fn, eventData->status);
      sUwaRegisterExtCallBackEvent.notifyOne();
    } break;
    case UWA_DM_DEVICE_RESET_RSP_EVT:  // result of UWA_SendDeviceReset
      {
        JNI_TRACE_I("%s: UWA_DM_DEVICE_RESET_RSP_EVT", fn);
        SyncEventGuard guard(sUwaDeviceResetEvent);
        if(eventData->status != UWA_STATUS_OK) {
            JNI_TRACE_E("%s: UWA_DM_DEVICE_RESET_RSP_EVT failed", fn);
        } else{
          sIsDeviceResetDone = true;
        }
        sUwaDeviceResetEvent.notifyOne();
      }
      break;
    case UWA_DM_DEVICE_STATUS_NTF_EVT:
      JNI_TRACE_I("%s: UWA_DM_DEVICE_STATUS_NTF_EVT", fn);
      {
        JNI_TRACE_I("device status = %x",eventData->dev_status.status);
        SyncEventGuard guard (sUwadeviceNtfEvent);
        sDeviceState =  (eUWBD_DEVICE_STATUS_t)eventData->dev_status.status;
        if (sDeviceState == UWBD_STATUS_ERROR)
          sErrNotify.notifyAll();
        else
          sUwadeviceNtfEvent.notifyOne();
        uwbEventManager.onDeviceStateNotificationReceived(sDeviceState);
      }
      break;
    case UWA_DM_CORE_GET_DEVICE_INFO_RSP_EVT:
      JNI_TRACE_I("%s: UWA_DM_CORE_GET_DEVICE_INFO_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaGetDeviceInfoEvent);
        if (eventData->status == UWA_STATUS_OK) {
          sUwbDeviceInfo.uciVersion = eventData->sGet_device_info.uci_version;
          sUwbDeviceInfo.macVersion = eventData->sGet_device_info.mac_version;
          sUwbDeviceInfo.phyVersion = eventData->sGet_device_info.phy_version;
          sUwbDeviceInfo.uciTestVersion = eventData->sGet_device_info.uciTest_version;
          if(parseDeviceInfo(eventData->sGet_device_info.vendor_info, eventData->sGet_device_info.vendor_info_len) == false) {
              JNI_TRACE_E("%s: Parsing Device Information Failed", fn);
          }
        } else {
          JNI_TRACE_E("%s: UWA_DM_CORE_GET_DEVICE_INFO_RSP_EVT failed", fn);
        }
        sUwaGetDeviceInfoEvent.notifyOne();
      }
      break;
    case UWA_DM_CORE_SET_CONFIG_RSP_EVT:  // result of UWA_SetCoreConfig
      JNI_TRACE_I("%s: UWA_DM_CORE_SET_CONFIG_RSP_EVT", fn);
      {
        if(eventData->status != UWA_STATUS_OK) {
            JNI_TRACE_E("%s: UWA_DM_CORE_SET_CONFIG_RSP_EVT failed", fn);
        }
        if(eventData->sCore_set_config.tlv_size > 0) {
          memcpy(sSetCoreConfig, eventData->sCore_set_config.param_ids,eventData->sCore_set_config.tlv_size);
        }
        SyncEventGuard guard(sUwaSetConfigEvent);
        sUwaSetConfigEvent.notifyOne();
      }
      break;
    case UWA_DM_CORE_GET_CONFIG_RSP_EVT: /* Result of UWA_GetCoreConfig */
      JNI_TRACE_I("%s: UWA_DM_CORE_GET_CONFIG_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaGetConfigEvent);
        if (eventData->status == UWA_STATUS_OK) {
          sGetCoreConfigLen = eventData->sCore_get_config.tlv_size;
          sNoOfCoreConfigIds = eventData->sCore_get_config.no_of_ids;
        } else {
          JNI_TRACE_E("%s: UWA_DM_GET_CONFIG failed", fn);
          /* As of now will cary the failed ids list till this point */
          sGetCoreConfigLen = 0;
          sNoOfCoreConfigIds = 0;
        }
        if(eventData->sCore_get_config.tlv_size > 0 && eventData->sCore_get_config.tlv_size <= sizeof(sGetCoreConfig)) {
          memcpy(sGetCoreConfig, eventData->sCore_get_config.param_tlvs, eventData->sCore_get_config.tlv_size);
        }
        sUwaGetConfigEvent.notifyOne();
      }
      break;
    case UWA_DM_SESSION_INIT_RSP_EVT:
      JNI_TRACE_I("%s: UWA_DM_SESSION_INIT_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaSessionInitEvent);
        if(eventData->status == UWA_STATUS_OK){
          sSessionInitStatus = true;
          JNI_TRACE_I("%s: UWA_DM_SESSION_INIT_RSP_EVT Success", fn);
        }
        else{
          JNI_TRACE_E("%s: UWA_DM_SESSION_INIT_RSP_EVT failed", fn);
        }
        sUwaSessionInitEvent.notifyOne();
      }
      break;
    case UWA_DM_SESSION_DEINIT_RSP_EVT:
      JNI_TRACE_I("%s: UWA_DM_SESSION_DEINIT_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaSessionDeInitEvent);
        if(eventData->status == UWA_STATUS_OK){
          sSessionDeInitStatus = true;
          JNI_TRACE_I("%s: UWA_DM_SESSION_DEINIT_RSP_EVT Success", fn);
        }
        else{
          JNI_TRACE_E("%s: UWA_DM_SESSION_DEINIT_RSP_EVT failed", fn);
        }
        sUwaSessionDeInitEvent.notifyOne();
      }
      break;
    case UWA_DM_SESSION_STATUS_NTF_EVT:
      JNI_TRACE_I("%s: UWA_DM_SESSION_STATUS_NTF_EVT", fn);
      {
        uwbEventManager.onSessionStatusNotificationReceived(eventData->sSessionStatus.session_id, eventData->sSessionStatus.state, eventData->sSessionStatus.reason_code);
        unsigned int session_id = eventData->sSessionStatus.session_id;
        if(sIsSessionOpInProgress) {
          bool found = false;
          found = nxpUwbManager.verifyExpectedReasonCode(sExpectedSessionState, eventData->sSessionStatus.reason_code);
          if((sCurrentSessionStatus.session_id == session_id) && found) {
            SyncEventGuard guard(sUwaSessionNtfEvent);
            sIsSessionStatusNtfReceived = true;
            sCurrentSessionStatus.state = eventData->sSessionStatus.state;
            sUwaSessionNtfEvent.notifyOne();
            sCurrentSessionStatus.session_id = INVALID_SESSION_ID;
          }
        }
        if(UWB_SESSION_DEINITIALIZED == eventData->sSessionStatus.state) {
          std::unique_lock<std::mutex> lock(sSessionMutex);
          auto it = sAveragedRangingData.find(session_id);
          if (it != sAveragedRangingData.end()) {
            sAveragedRangingData.erase(session_id);
            JNI_TRACE_E("%s: deinit: Averaging Disabled for Session %d", fn, session_id);
          }
        }
      }
      break;
    case UWA_DM_SESSION_SET_CONFIG_RSP_EVT:  // result of UWA_SetAppConfig
      JNI_TRACE_I("%s: UWA_DM_SESSION_SET_CONFIG_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaSetAppConfigEvent);
        sSetAppConfigRespStatus = true;
        sSetAppConfigStatus = eventData->status;
        sSetAppConfigLen = eventData->sApp_set_config.tlv_size;
        sNoOfAppConfigIds = eventData->sApp_set_config.num_param_id;
        if(eventData->sApp_set_config.tlv_size > 0){
          memcpy(sSetAppConfig, eventData->sApp_set_config.param_ids, eventData->sApp_set_config.tlv_size);
        }
        sUwaSetAppConfigEvent.notifyOne();
      }
      break;
    case UWA_DM_SESSION_GET_CONFIG_RSP_EVT: /* Result of UWA_GetAppConfig */
      JNI_TRACE_I("%s: UWA_DM_SESSION_GET_CONFIG_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaGetAppConfigEvent);
        sGetAppConfigRespStatus = true;
        sGetAppConfigStatus = eventData->status;
        sGetAppConfigLen = eventData->sApp_get_config.tlv_size;
        sNoOfAppConfigIds = eventData->sApp_get_config.no_of_ids;
        if(eventData->sApp_get_config.tlv_size > 0) {
          memcpy(sGetAppConfig, eventData->sApp_get_config.param_tlvs, eventData->sApp_get_config.tlv_size);
        }
        sUwaGetAppConfigEvent.notifyOne();
      }
      break;
    case UWA_DM_RANGE_START_RSP_EVT: /* result of range start command */
      JNI_TRACE_I("%s: UWA_DM_RANGE_START_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaRngStartEvent);
        if(eventData->status == UWA_STATUS_OK){
          sRangeStartStatus = true;
          JNI_TRACE_I("%s: UWA_DM_RANGE_START_RSP_EVT Success", fn);
        }
        else{
          sRangeStartStatus = false;
          JNI_TRACE_E("%s: UWA_DM_RANGE_START_RSP_EVT failed", fn);
        }
        sUwaRngStartEvent.notifyOne();
      }
      break;
    case UWA_DM_RANGE_STOP_RSP_EVT: /* result of range stop command */
      JNI_TRACE_I("%s: UWA_DM_RANGE_STOP_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaRngStopEvent);
        if(eventData->status == UWA_STATUS_OK){
          sRangeStopStatus = true;
          JNI_TRACE_I("%s: UWA_DM_RANGE_STOP_RSP_EVT Success", fn);
        }
        else{
          sRangeStopStatus = false;
          JNI_TRACE_E("%s: UWA_DM_RANGE_STOP_RSP_EVT failed", fn);
        }
        sUwaRngStopEvent.notifyOne();
      }
      break;
    case UWA_DM_GET_RANGE_COUNT_RSP_EVT:
      JNI_TRACE_I("%s: UWA_DM_GET_RANGE_COUNT_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaGetRangingCountEvent);
        if(eventData->status == UWA_STATUS_OK){
          sRangingCount = eventData->sGet_range_cnt.count;
        }else{
          JNI_TRACE_E("%s: get range count Request is failed", fn);
          sRangingCount = 0;
        }
        sUwaGetRangingCountEvent.notifyOne();
      }
      break;
    case UWA_DM_RANGE_DATA_NTF_EVT:
      JNI_TRACE_I("%s: UWA_DM_RANGE_DATA_NTF_EVT", fn);
      {
        notifyRangeDataNotification(&eventData->sRange_data);
      }
      break;
    case UWA_DM_SESSION_GET_COUNT_RSP_EVT:
      JNI_TRACE_I("%s: UWA_DM_SESSION_GET_COUNT_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaGetSessionCountEvent);
        if(eventData->status == UWA_STATUS_OK){
          sSessionCount = eventData->sGet_session_cnt.count;
        }else{
          JNI_TRACE_E("%s: get session count Request is failed", fn);
          sSessionCount = -1;
        }
        sUwaGetSessionCountEvent.notifyOne();
      }
      break;

    case UWA_DM_SESSION_GET_STATE_RSP_EVT:
      JNI_TRACE_I("%s: UWA_DM_SESSION_GET_STATE_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaGetSessionStatusEvent);
        if(eventData->status == UWA_STATUS_OK){
          sSessionState = eventData->sGet_session_state.session_state;
        }else{
          JNI_TRACE_E("%s: get session state Request is failed", fn);
          sSessionState = UWB_UNKNOWN_SESSION;
        }
        sUwaGetSessionStatusEvent.notifyOne();
      }
      break;

    case UWA_DM_SESSION_MC_LIST_UPDATE_RSP_EVT: /* result of session update multicast list */
      JNI_TRACE_I("%s: UWA_DM_SESSION_MC_LIST_UPDATE_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaMulticastListUpdateEvent);
        if(eventData->status == UWA_STATUS_OK){
          sMulticastListUpdateStatus = true;
          JNI_TRACE_I("%s: UWA_DM_SESSION_MC_LIST_UPDATE_RSP_EVT Success", fn);
        }
        else{
          JNI_TRACE_E("%s: UWA_DM_SESSION_MC_LIST_UPDATE_RSP_EVT failed", fn);
        }
        sUwaMulticastListUpdateEvent.notifyOne();
      }
      break;

    case UWA_DM_SESSION_MC_LIST_UPDATE_NTF_EVT:
      JNI_TRACE_I("%s: UWA_DM_SESSION_MC_LIST_UPDATE_NTF_EVT", fn);
      {
        uwbEventManager.onMulticastListUpdateNotificationReceived(&eventData->sMulticast_list_ntf);
      }
      break;

    case UWA_DM_SEND_BLINK_DATA_RSP_EVT:
      JNI_TRACE_I("%s: UWA_DM_SEND_BLINK_DATA_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaSendBlinkDataEvent);
        sSendBlinkDataStatus = eventData->status;
        sUwaSendBlinkDataEvent.notifyOne();
      }
      break;

    case UWA_DM_SEND_BLINK_DATA_NTF_EVT:
      JNI_TRACE_I("%s: UWA_DM_SEND_BLINK_DATA_NTF_EVT", fn);
      {
        uwbEventManager.onBlinkDataTxNotificationReceived(eventData->sBlink_data_ntf.repetition_count_status);
      }
      break;

    case UWA_DM_CORE_GEN_ERR_STATUS_EVT:
      JNI_TRACE_I("%s: UWA_DM_CORE_GEN_ERR_STATUS_EVT", fn);
      {
        if (eventData->sCore_gen_err_status.status ==
            UCI_STATUS_DEVICE_TEMP_REACHED_THERMAL_RUNAWAY) {
          sErrNotify.notifyAll(); //signal upper layer in case of over temp ntf
        }

        uwbEventManager.onCoreGenericErrorNotificationReceived(
            eventData->sCore_gen_err_status.status);
      }
      break;
    case UWA_DM_UWBD_RESP_TIMEOUT_EVT:
      JNI_TRACE_I("%s: UWA_DM_UWBD_RESP_TIMEOUT_EVT", fn);
      {
        sErrNotify.notifyAll();
        sDevState = STATUS_TIMEOUT;
        uwbEventManager.onDeviceStateNotificationReceived(sDevState);
      }
      break;

    case UWA_DM_SEND_DATA_PACKET_RSP_EVT:
      JNI_TRACE_I("%s: UWA_DM_SEND_DATA_PACKET_RSP_EVT", fn);
      {
        SyncEventGuard guard(sUwaSendDataEvent);
        sSendDataStatus = eventData->status;
        sUwaSendDataEvent.notifyOne();
      }
      break;

    case UWA_DM_DATA_TRANSFER_STATUS_NTF_EVT:
      JNI_TRACE_I("%s: UWA_DM_DATA_TRANSFER_STATUS_NTF_EVT", fn);
      {
        uwbEventManager.onDataTransferStatusReceived(eventData->sData_xfer_status.session_id, eventData->sData_xfer_status.status);
      }
      break;

    case UWA_DM_DATA_RECV_EVT:
      JNI_TRACE_I("%s: UWA_DM_DATA_RECV_EVT", fn);
      {
        uwbEventManager.onDataReceived(&eventData->sRcvd_data);
      }
       break;

    default:
      JNI_TRACE_I("%s: unhandled event", fn);
      break;
  }
}

/*******************************************************************************
**
** Function:        setAppConfiguration
**
** Description:     Set the session specific App Config
**
** Params:          session_id: Session Id of the required App Config
**                  noOfParams: Number of Params need to configure
**                  paramLen: Total Params Lentgh
**                  appConfigParams: AppConfigs List in TLV format
**
** Returns:         If Success UWA_STATUS_OK  else UWA_STATUS_FAILED
**
*******************************************************************************/
static tUWA_STATUS setAppConfiguration(uint32_t session_id, uint8_t noOfParams, uint8_t paramLen , uint8_t appConfigParams[]){
  static const char fn[] = "setAppConfiguration";
  UNUSED(fn);
  tUWA_STATUS status;
  sSetAppConfigRespStatus = false;
  SyncEventGuard guard(sUwaSetAppConfigEvent);
  status = UWA_SetAppConfig(session_id, noOfParams, paramLen, appConfigParams);
  if(status == UWA_STATUS_OK) {
      sUwaSetAppConfigEvent.wait(UWB_CMD_TIMEOUT);
      JNI_TRACE_I("%s: Success UWA_SetAppConfig Command", fn);
  } else {
    JNI_TRACE_E("%s: Failed UWA_SetAppConfig Command", fn);
    return UWA_STATUS_FAILED;
  }
  return (sSetAppConfigRespStatus) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}
/*******************************************************************************
**
** Function:        SetCoreDeviceConfigurations
**
** Description:     Set the Core Device Config
**
**
** Returns:         If Success UWA_STATUS_OK  else UWA_STATUS_FAILED
**
*******************************************************************************/
static tUWA_STATUS SetCoreDeviceConfigurations(){
  uint8_t coreConfigsCount = 1;
  static const char fn[] = "SetCoreDeviceConfigurations";
  UNUSED(fn);
  tUWA_STATUS status;
  uint8_t configParam[] = {0x00, 0x00, 0x00};
  uint16_t config = 0;
  JNI_TRACE_I("%s: Enter ", fn);

  config = UwbConfig::getUnsigned(NAME_UWB_LOW_POWER_MODE, 0x00);
  JNI_TRACE_I("%s: NAME_UWB_LOW_POWER_MODE value %d ", fn,(uint8_t)config);

  configParam[0] = (uint8_t)config;

  {
    SyncEventGuard guard(sUwaSetConfigEvent);
    status = UWA_SetCoreConfig(UCI_PARAM_ID_LOW_POWER_MODE, coreConfigsCount, &configParam[0]);
    if(status == UWA_STATUS_OK) {
      sUwaSetConfigEvent.wait(UWB_CMD_TIMEOUT);
      JNI_TRACE_I("%s: low power mode config is success", fn);
    } else {
      JNI_TRACE_E("%s: low power mode config is failed", fn);
      return UWA_STATUS_FAILED;
    }
  }

  JNI_TRACE_I("%s: Exit ", fn);
  return status;
}

/*******************************************************************************
**
** Function:        clearAllSessionContext
**
** Description:     This API is invoked before Init and during DeInit to clear
**                  All the Session specific context.
**
** Returns:         Nothing
**
*******************************************************************************/
void clearAllSessionContext(){
#if 1//WA waiting binding status NTF/ SE comm error NTF
  gBindStatus = 0;
#endif
  {
    std::unique_lock<std::mutex> lock(sSessionMutex);
    sAveragedRangingData.clear();
  }
  clearRfTestContext();
}

/*******************************************************************************
**
** Function:        UwbDeviceReset
**
** Description:     Send Device Reset Command.
**
** Params:          resetConfig: Manufacturer/Vendor Specific Reset Data

** Returns:         If Success UWA_STATUS_OK  else UWA_STATUS_FAILED
**
*******************************************************************************/
bool UwbDeviceReset(uint8_t resetConfig) {
  static const char fn[] = "UwbDeviceReset";
  UNUSED(fn);
  tUWA_STATUS status;
  JNI_TRACE_I("%s: Enter", fn);

  sIsDeviceResetDone = false;
  mBindingStatusCheckMutex.lock();
  sBindingStateNtf = ESE_BINDING_NTF_PENDING;
  mBindingStatusCheckMutex.unlock();
  {
    SyncEventGuard guard(sUwaDeviceResetEvent);
    status = UWA_SendDeviceReset((uint8_t)resetConfig);
    if (status == UWA_STATUS_OK)
          sUwaDeviceResetEvent.wait(UWB_CMD_TIMEOUT); /* wait for callback */
  }
  if (status == UWA_STATUS_OK) {
    JNI_TRACE_E("%s: Success UWA_SendDeviceReset", fn);
    if(sIsDeviceResetDone) {
      SyncEventGuard guard (sUwadeviceNtfEvent);
      sUwadeviceNtfEvent.wait(UWB_CMD_TIMEOUT);
      switch(sDeviceState) {
        case UWBD_STATUS_READY:
        {
          clearAllSessionContext();
          JNI_TRACE_I("%s: Device Reset is success %d", fn, sDeviceState);
        } break;
        default: {
          JNI_TRACE_E("%s: Device state is = %d", fn, sDeviceState);
        } break;
      }
    }
  } else {
    JNI_TRACE_E("%s: Failed UWA_SendDeviceReset", fn);
  }
  JNI_TRACE_I("%s: Exit", fn);
  return sIsDeviceResetDone ? TRUE : FALSE;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_doInitialize
**
** Description:     Turn on UWB. intialize the GKI module and HAL module for UWB device.
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         True if UWB device initilization is success.
**
*******************************************************************************/
jboolean uwbNativeManager_doInitialize(JNIEnv* env, jobject o) {
  static const char fn[] = "uwbNativeManager_doInitialize";
  UNUSED(fn);
  tUWA_MW_VERSION mwVersion;
  tUWA_STATUS status;
  uint8_t resetConfig = 0;
  JNI_TRACE_I("%s: enter", fn);

  if(gIsUwaEnabled) {
    JNI_TRACE_I("%s: Already Initialized", fn);
    UwbDeviceReset(resetConfig);
    setMwState(UWB_MW_STATE_ENABLED);
    return JNI_TRUE;
  }

  sDeviceState = UWBD_STATUS_ERROR;
  mwVersion = UWA_GetMwVersion();
  if(mwVersion.rc_version) { /* for RC release*/
    JNI_TRACE_I("MW Version: UWB01_SW_Android_R_HKY_D%02x.%02x_RC%02x",
       mwVersion.major_version, mwVersion.minor_version, mwVersion.rc_version);
  } else if(mwVersion.mw_drop) { /* For Drops */
    JNI_TRACE_I("MW Version: UWB01_SW_Android_R_HKY_D%02x.%02x_DROP%02x",
       mwVersion.major_version, mwVersion.minor_version, mwVersion.mw_drop);
  } else { /* for Major Releases*/
    JNI_TRACE_I("MW Version: UWB01_SW_Android_R_HKY_D%02x.%02x",
       mwVersion.major_version, mwVersion.minor_version);
  }
  sUwbDeviceInfo.mwVersion[0] = mwVersion.major_version;
  sUwbDeviceInfo.mwVersion[1] = mwVersion.minor_version;
  UwbAdaptation& theInstance = UwbAdaptation::GetInstance();
  theInstance.Initialize();  // start GKI, UCI task, UWB task
  tHAL_UWB_ENTRY* halFuncEntries = theInstance.GetHalEntryFuncs();
  UWA_Init(halFuncEntries);
  clearAllSessionContext();
  setMwState(UWB_MW_STATE_ENABLING);
  {
    SyncEventGuard guard(sUwaEnableEvent);
    status = UWA_Enable(uwaDeviceManagementCallback, uwaRfTestDeviceManagementCallback);
    if (status == UWA_STATUS_OK)
      sUwaEnableEvent.wait(UWB_CMD_TIMEOUT);
  }
  if (status == UWA_STATUS_OK) {
    if(!gIsUwaEnabled)
    {
      JNI_TRACE_E("%s: UWB Enable failed", fn);
      goto error;
    }
    {
      SyncEventGuard guard(sUwaRegisterExtCallBackEvent);
      (void)UWA_RegisterExtCallback(extDeviceManagementCallback);
      sUwaRegisterExtCallBackEvent.wait(UWB_CMD_TIMEOUT);
    }
    status = theInstance.CoreInitialization(false);
    JNI_TRACE_I("%s: CoreInitialization status: %d", fn, status);

    if (status == UWA_STATUS_OK) {
      #if 1//WA waiting binding status NTF/ SE comm error NTF
      {
          SyncEventGuard guard (gBindStatusNtfEvt);
          gBindStatusNtfEvt.wait(200);
      }
      JNI_TRACE_I("%s: Binding Status: %d", fn, gBindStatus);
      if(gBindStatus == 0x03) // Unknown Binding Status.
      {
          SyncEventGuard guard (gSeCommErrNtfEvt);
          gSeCommErrNtfEvt.wait(50);
      }
      #endif
        {
          loadAntennaConfigs();

          if (UwbConfig::hasKey(NAME_ANTENNA_PAIR_SELECTION_CONFIG5))
             sAntennaConfig[4] = (uint16_t)UwbConfig::getUnsigned(NAME_ANTENNA_PAIR_SELECTION_CONFIG5);
          /* Get Device Info */
          {
            SyncEventGuard guard(sUwaGetDeviceInfoEvent);
            status = UWA_GetDeviceInfo();

            if (status == UWA_STATUS_OK) {
              sUwaGetDeviceInfoEvent.wait();
              JNI_TRACE_I("Firmware Version : %x.%x.%x", (sUwbDeviceInfo.nxpDeviceInfo.fwVersion & MSB_BITMASK),
                                                         ((sUwbDeviceInfo.nxpDeviceInfo.fwVersion >> 8) & MSB_BITMASK),
                                                         ((sUwbDeviceInfo.nxpDeviceInfo.fwVersion >> 16 ) & MSB_BITMASK));
              JNI_TRACE_I("UCI Version : %x.%x", (sUwbDeviceInfo.uciVersion & 0X00FF), (sUwbDeviceInfo.uciVersion >> 8));
              if(sUwbDeviceInfo.nxpDeviceInfo.fwBootMode == FACTORY_FW_BOOT_MODE) {
                JNI_TRACE_I("Factory FW Version, no need to check binding status");
                mBindingStatusCheckMutex.lock();
                sBindingStateNtf = ESE_BINDING_NTF_RECEIVED;
                mBindingStatusCheckMutex.unlock();
              }
            }
          }

          if(status == UWA_STATUS_OK)
          {
            gIsUwaEnabled = true;
            status = SetCoreDeviceConfigurations();
            if (status == UWA_STATUS_OK) {
              JNI_TRACE_I("%s: SetCoreDeviceConfigurations is SUCCESS %d", fn,status);
            } else {
              JNI_TRACE_I("%s: SetCoreDeviceConfigurations is Failed %d", fn,status);
              goto error;
            }
            status = SetExtDeviceConfigurations();
            if (status == UWA_STATUS_OK) {
              JNI_TRACE_I("%s: setInitExtDeviceConfigurations is SUCCESS %d", __func__,status);
            } else {
              JNI_TRACE_I("%s: setInitExtDeviceConfigurations is Failed %d", __func__,status);
              goto error;
            }
            goto end;
          }
        }
    }
  }
error:
  JNI_TRACE_E("%s: device status is failed %d", fn, sDeviceState);
  gIsUwaEnabled = false;
  mBindingStatusCheckMutex.lock();
  sBindingStateNtf = ESE_BINDING_NTF_DEFAULT;
  mBindingStatusCheckMutex.unlock();
  status = UWA_Disable(false); /* gracefull exit */
  if (status == UWA_STATUS_OK) {
    JNI_TRACE_I("%s: UWA_Disable(false) SUCCESS %d", fn,status);
  } else {
    JNI_TRACE_E("%s: UWA_Disable(false) is failed %d", fn,status);
  }
  theInstance.Finalize(false);  // disable GKI, UCI task, UWB task
end:
  mBindingStatusCheckMutex.lock();
  if(sBindingStateNtf != ESE_BINDING_NTF_RECEIVED) {
    sBindingStateNtf = ESE_BINDING_NTF_PENDING;
  }
  mBindingStatusCheckMutex.unlock();
  if(gIsUwaEnabled){
    setMwState(UWB_MW_STATE_ENABLED);
    sDeviceState = UWBD_STATUS_ACTIVE;
  }
  JNI_TRACE_I("%s: exit", fn);
  return gIsUwaEnabled ? JNI_TRUE : JNI_FALSE;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_doDeinitialize
**
** Description:     Turn off UWB. Deinitilize the GKI and HAL module, power
**                  of the UWB device.
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         True if UWB device De-initilization is success.
**
*******************************************************************************/
jboolean uwbNativeManager_doDeinitialize(JNIEnv* env, jobject obj) {
  static const char fn[] = "uwbNativeManager_doDeinitialize";
  UNUSED(fn);
  tUWA_STATUS status;
  JNI_TRACE_I("%s: Enter", fn);
  UwbAdaptation& theInstance = UwbAdaptation::GetInstance();

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is already De-initialized", fn);
    return JNI_TRUE;
  }

/* do not disable the MW stack if binding status NTf is not received */
  mBindingStatusCheckMutex.lock();
  if(sBindingStateNtf == ESE_BINDING_NTF_PENDING) {
    JNI_TRACE_E("%s: Binding Status NTF is pending", fn);
    setMwState(UWB_MW_STATE_DISABLED);
    mBindingStatusCheckMutex.unlock();
    return JNI_TRUE;
  }
  mBindingStatusCheckMutex.unlock();

  setMwState(UWB_MW_STATE_DISABLING);
  SyncEventGuard guard(sUwaDisableEvent);
  status = UWA_Disable(true); /* gracefull exit */
  if (status == UWA_STATUS_OK) {
    JNI_TRACE_I("%s: wait for de-init completion:", fn);
    sUwaDisableEvent.wait();
  } else {
    JNI_TRACE_E("%s: De-Init is failed:", fn);
  }
  clearAllSessionContext();
  gIsUwaEnabled = false;
  theInstance.Finalize(true);  // disable GKI, UCI task, UWB task
  JNI_TRACE_I("%s: Exit", fn);
  mBindingStatusCheckMutex.lock();
  sBindingStateNtf = ESE_BINDING_NTF_DEFAULT;
  mBindingStatusCheckMutex.unlock();
  setMwState(UWB_MW_STATE_DISABLED);
  return JNI_TRUE;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_getDeviceInfo
**
** Description:     retrive the UWB device information etc.
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         device info class object or NULL.
**
*******************************************************************************/
jobject uwbNativeManager_getDeviceInfo(JNIEnv* env, jobject obj) {
  static const char fn[] = "uwbNativeManager_getDeviceInfo";
  UNUSED(fn);
  JNI_TRACE_I("%s: Enter", fn);

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", fn);
    return NULL;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return NULL;
  }


  const char* DEVICE_DATA_CLASS_NAME = "com/android/uwb/UwbDeviceData";
  jclass deviceDataClass = env->FindClass(DEVICE_DATA_CLASS_NAME);
  jmethodID constructor = env->GetMethodID(deviceDataClass, "<init>", "(IIII[BIIII[BIII)V");
  if (constructor == JNI_NULL) {
    JNI_TRACE_E("%s: jni cannot find the method deviceInfoClass", fn);
    return NULL;
  }

  jbyteArray deviceName = env->NewByteArray(deviceNameLen);
  env->SetByteArrayRegion (deviceName, 0, deviceNameLen, (jbyte *)sUwbDeviceInfo.nxpDeviceInfo.deviceName);

  jint uciVersion = sUwbDeviceInfo.uciVersion;
  jint uciTestVersion = sUwbDeviceInfo.uciTestVersion;
  jint mwMinorVersion = sUwbDeviceInfo.mwVersion[1];
  jint mwMajorVersion = sUwbDeviceInfo.mwVersion[0];
  jint fwVersion = sUwbDeviceInfo.nxpDeviceInfo.fwVersion;
  jint vendorUciVersion = sUwbDeviceInfo.nxpDeviceInfo.vendorUciVersion;
  jint macVersion = sUwbDeviceInfo.macVersion;
  jint phyVersion = sUwbDeviceInfo.phyVersion;
  jint firaUciGenVersion = sUwbDeviceInfo.nxpDeviceInfo.vendorFiraUciGenericVersion;
  jint firaUciTestVersion = sUwbDeviceInfo.nxpDeviceInfo.vendorFiraUciTestVersion;
  jint uwbsMaxPpmValue = sUwbDeviceInfo.nxpDeviceInfo.uwbsMaxPpmValue;

  jbyteArray chipId = env->NewByteArray(chipIdLen);
  env->SetByteArrayRegion (chipId, 0, chipIdLen, (jbyte *)sUwbDeviceInfo.nxpDeviceInfo.chipId);

  JNI_TRACE_I("%s: Exit", fn);

  return env->NewObject(deviceDataClass, constructor, uciVersion, macVersion, phyVersion, uciTestVersion, deviceName, mwMajorVersion, mwMinorVersion, fwVersion,
              vendorUciVersion, chipId, firaUciGenVersion, firaUciTestVersion, uwbsMaxPpmValue);

}

/*******************************************************************************
**
** Function:        uwbNativeManager_getUwbDeviceState
**
** Description:     Retrive the UWB device state..
**
** Params :         env: JVM environment.
**                  o: Java object.
**
** Returns:         device state.
**
*******************************************************************************/
jint uwbNativeManager_getUwbDeviceState(JNIEnv* env, jobject obj) {
  static const char fn[] = "uwbNativeManager_getUwbDeviceState";
  UNUSED(fn);
  eUWBD_DEVICE_STATUS_t sDeviceState = UWBD_STATUS_ERROR;
  JNI_TRACE_I("%s: Enter", fn);

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", fn);
    return sDeviceState;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return sDeviceState;
  }

  tUWA_PMID configParam[] = { UCI_PARAM_ID_DEVICE_STATE };
  SyncEventGuard guard(sUwaGetConfigEvent);
  tUWA_STATUS status = UWA_GetCoreConfig(sizeof(configParam), configParam);
  if (status == UWA_STATUS_OK) {
    sUwaGetConfigEvent.wait(UWB_CMD_TIMEOUT);
    if(sGetCoreConfigLen > 0){
      if (sGetCoreConfig[0] == UCI_PARAM_ID_DEVICE_STATE) {
        sDeviceState = (eUWBD_DEVICE_STATUS_t)sGetCoreConfig[2];
      }
    }
  }
  JNI_TRACE_I("%s: Exit", fn);
  return sDeviceState;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_deviceReset
**
** Description:     Send Device Reset Command.
**
** Params:          env: JVM environment.
**                  obj: Java object.
**                  resetConfig: Manufacturer/Vendor Specific Reset Data
**
** Returns:         If Success UWA_STATUS_OK  else UWA_STATUS_FAILED
**
*******************************************************************************/
jbyte uwbNativeManager_deviceReset(JNIEnv* env, jobject obj, jbyte resetConfig) {
  static const char fn[] = "uwbNativeManager_deviceReset";
  UNUSED(fn);
  bool status;
  JNI_TRACE_I("%s: Enter", fn);

  //WA: commented reset functionality as this wiill trigger ESE communication and Helios will send binding status NTF again
  // if Helos is turned off without reading response from ESE, then this makes ESE unresponsive sitiation
  //Sending reset command as part of MW enable every time to reset both Helios and SUS applet from ESE
  status = true; // true always
#if 0
  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", fn);
    return UWA_STATUS_FAILED;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return UWA_STATUS_FAILED;
  }

  status = UwbDeviceReset((uint8_t)resetConfig);
 #endif
  JNI_TRACE_I("%s: Exit", fn);
  return status ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        initializeSessionStatus
**
** Description:     Function to Intialize Session Status and set Session Status Ntf Received to false
**
*******************************************************************************/
void initializeSessionStatus(uint32_t session_id, uint8_t expectedState) {
    sCurrentSessionStatus.session_id = session_id;
    sCurrentSessionStatus.state = UWB_UNKNOWN_SESSION;
    sCurrentSessionStatus.reason_code = 0;
    sIsSessionStatusNtfReceived = false;
    sIsSessionOpInProgress = true;
    sExpectedSessionState = expectedState;
}

/*******************************************************************************
**
** Function:        waitForSessionStatusNotification
**
** Description:     Function to wait for Notification and update Uppwer layer if notification is not received
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         If Success true else false
**
*******************************************************************************/
static bool waitForSessionStatusNotification() {
  static const char fn[] = "waitForSessionStatusNotification";
  UNUSED(fn);
  std::vector<tUWA_SESSION_STATUS_NTF_REVT> sessionList;
  if(sIsSessionStatusNtfReceived == false) {
    /* If session status is not received, wait for notification */
    SyncEventGuard guard(sUwaSessionNtfEvent);
    sUwaSessionNtfEvent.wait(UWB_CMD_TIMEOUT);
  }

  if(sDeviceState == UWBD_STATUS_ERROR){
    JNI_TRACE_E("%s: UWBD is in error state %d", fn, sDeviceState);
    return false;
  }

  if(sIsSessionStatusNtfReceived == false) {
    uint32_t session_id = sCurrentSessionStatus.session_id;
    /* if Session Status notification is not received, then get the session details
       for the session id update upper layer */
    nxpUwbManager.getAllUwbSessions(sessionList);
    auto it = find_if(sessionList.begin(), sessionList.end(), [&](auto item) { return (session_id == item.session_id); });
    if(it != sessionList.end()) {
      sCurrentSessionStatus.state = (*it).state;
      sCurrentSessionStatus.reason_code = (*it).reason_code;
    } else {
      std::unique_lock<std::mutex> lock(sSessionMutex);
      auto it = sAveragedRangingData.find(session_id);
      if (it != sAveragedRangingData.end()) {
        sAveragedRangingData.erase(session_id);
        JNI_TRACE_E("%s: deinit: Averaging Disabled for Session %d", fn, session_id);
      }
      /* if entry for the given session id is not found in the list, set state as deinitialized */
      sCurrentSessionStatus.state = UWB_SESSION_DEINITIALIZED;
      sCurrentSessionStatus.reason_code = 0;
    }
    uwbEventManager.onSessionStatusNotificationReceived(session_id, sCurrentSessionStatus.state, sCurrentSessionStatus.reason_code);
  }
  sIsSessionOpInProgress = false;
  return (sCurrentSessionStatus.state == sExpectedSessionState);
}

/*******************************************************************************
**
** Function:        uwbNativeManager_sessionInit
**
** Description:     Initialize the session for the particular activity.
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         If Success UWA_STATUS_OK  else UWA_STATUS_FAILED
**
*******************************************************************************/
jbyte uwbNativeManager_sessionInit(JNIEnv* env, jobject o, jint sessionId, jbyte sessionType) {
  static const char fn[] = "uwbNativeManager_sessionInit";
  UNUSED(fn);
  tUWA_STATUS status = UWA_STATUS_FAILED;
  JNI_TRACE_I("%s: Enter", fn);
  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", fn);
    return status;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return status;
  }

  sSessionInitStatus = false;
  initializeSessionStatus(sessionId, UWB_SESSION_INITIALIZED);
  {
    SyncEventGuard guard(sUwaSessionInitEvent);
    status = UWA_SendSessionInit(sessionId, sessionType);
    if(UWA_STATUS_OK == status)
      sUwaSessionInitEvent.wait(UWB_CMD_TIMEOUT);
  }
  if(sSessionInitStatus) {
    sSessionInitStatus = waitForSessionStatusNotification();
  }
  JNI_TRACE_I("%s: exit status = 0x%x sSessionInitStatus = %s", fn, status, sSessionInitStatus?"true":"false");
  return (sSessionInitStatus) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_sessionDeInit
**
** Description:     This API is invoked from the application to DeInitialize
**                  Session Specific context.
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         If Success UWA_STATUS_OK  else UWA_STATUS_FAILED
**
*******************************************************************************/
jbyte uwbNativeManager_sessionDeInit(JNIEnv* env, jobject o, jint sessionId) {
  static const char fn[] = "uwbNativeManager_sessionDeInit";
  UNUSED(fn);
  tUWA_STATUS status = UWA_STATUS_FAILED;
  JNI_TRACE_I("%s: Enter", fn);
  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", fn);
    return status;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return status;
  }

  initializeSessionStatus(sessionId, UWB_SESSION_DEINITIALIZED);
  sSessionDeInitStatus = false;
  {
    SyncEventGuard guard(sUwaSessionDeInitEvent);
    status = UWA_SendSessionDeInit(sessionId);
    if(UWA_STATUS_OK == status)
      sUwaSessionDeInitEvent.wait(UWB_CMD_TIMEOUT);
  }
  if(sSessionDeInitStatus) {
    sSessionDeInitStatus = waitForSessionStatusNotification();
  }
  JNI_TRACE_I("%s: exit status = 0x%x sSessionDeInitStatus = %s", fn, status, sSessionDeInitStatus?"true":"false");
  return (sSessionDeInitStatus)?UWA_STATUS_OK:UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_setAppConfigurations()
**
** Description:     Invoked this API to set the session specific Application
**                  Configuration.
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  sessionId: All APP configurations belonging to this Session ID
**                  noOfParams : The number of APP Configuration fields to follow
**                  appConfigLen : Length of AppConfigData
**                  AppConfig : App Configurations for session
**
** Returns:         Returns byte array
**
*******************************************************************************/
jbyteArray uwbNativeManager_setAppConfigurations(JNIEnv* env, jobject o, jint sessionId, jint noOfParams, jint appConfigLen, jbyteArray AppConfig) {
  static const char fn[] = "uwbNativeManager_setAppConfigurations";
  UNUSED(fn);
  jbyteArray appConfigArray = NULL;
  uint8_t* appConfigData = NULL;
  tUWA_STATUS status = UWA_STATUS_FAILED;
  JNI_TRACE_I("%s: Enter", fn);
  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", fn);
    return appConfigArray;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return appConfigArray;
  }

  appConfigData = (uint8_t*) malloc(sizeof(uint8_t) * appConfigLen);
  if(appConfigData != NULL) {
    memset(appConfigData, 0, (sizeof(uint8_t) * appConfigLen));
    env->GetByteArrayRegion(AppConfig,0,appConfigLen,(jbyte*)appConfigData);
    JNI_TRACE_I("%d: appConfigLen", appConfigLen);
    status = setAppConfiguration(sessionId, noOfParams,appConfigLen, appConfigData);
    free(appConfigData);
    appConfigArray = env->NewByteArray(sSetAppConfigLen + sizeof(sSetAppConfigStatus) + sizeof(sNoOfAppConfigIds));
    env->SetByteArrayRegion (appConfigArray, 0, 1, (jbyte *)&sSetAppConfigStatus);
    env->SetByteArrayRegion (appConfigArray, 1, 1, (jbyte *)&sNoOfAppConfigIds);
    if(sSetAppConfigLen > 0){
      env->SetByteArrayRegion (appConfigArray, 2, sSetAppConfigLen, (jbyte *)&sSetAppConfig[0]);
    }
  } else {
    JNI_TRACE_E("%s: Unable to Allocate Memory", fn);
  }
  JNI_TRACE_I("%s: Exit", fn);
  return appConfigArray;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_getAppConfigurations
**
** Description:     retrive the UWB device state..
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  session id : Session Id for the given set of App params
**                  noOfParams: Number of Params
**                  appConfigLen: Total App config Lentgh
**                  AppConfig: App Config List to retreive
**
** Returns:         Returns App Config Info list.
**
*******************************************************************************/
jbyteArray uwbNativeManager_getAppConfigurations(JNIEnv* env, jobject o, jint sessionId, jint noOfParams, jint appConfigLen, jbyteArray AppConfig) {
  static const char fn[] = "uwbNativeManager_getAppConfigurations";
  UNUSED(fn);
  tUWA_STATUS status;
  jbyteArray appConfigArray = NULL;
  uint8_t* appConfigData = NULL;
  JNI_TRACE_I("%s: Enter", fn);

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", fn);
    return appConfigArray;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return appConfigArray;
  }

  sGetAppConfigRespStatus = false;
  appConfigData = (uint8_t*) malloc(sizeof(uint8_t) * appConfigLen);
  if(appConfigData != NULL) {
    memset(appConfigData, 0, (sizeof(uint8_t) * appConfigLen));
    env->GetByteArrayRegion(AppConfig, 0, appConfigLen, (jbyte*)appConfigData);
    SyncEventGuard guard(sUwaGetAppConfigEvent);
    status = UWA_GetAppConfig(sessionId, noOfParams, appConfigLen, appConfigData);
    if (status == UWA_STATUS_OK) {
      sUwaGetAppConfigEvent.wait(UWB_CMD_TIMEOUT);
      if(sGetAppConfigRespStatus) {
        appConfigArray = env->NewByteArray(sGetAppConfigLen + sizeof(sGetAppConfigStatus) + sizeof(sNoOfAppConfigIds));
        env->SetByteArrayRegion (appConfigArray, 0, 1, (jbyte *)&sGetAppConfigStatus);
        env->SetByteArrayRegion (appConfigArray, 1, 1, (jbyte *)&sNoOfAppConfigIds);
        env->SetByteArrayRegion (appConfigArray, 2, sGetAppConfigLen, (jbyte *)&sGetAppConfig[0]);
      } else {
        JNI_TRACE_E("%s: Failed getAppConfigurations, Status = %d", fn, sGetAppConfigRespStatus);
      }
    } else {
      JNI_TRACE_E("%s: Failed UWA_GetAppConfig", fn);
    }
    free(appConfigData);
  } else {
    JNI_TRACE_E("%s: Unable to Allocate Memory", fn);
  }
  JNI_TRACE_I("%s: Exit", fn);
  return appConfigArray;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_startRanging
**
** Description:     start the ranging session with required configs and notify
**                  the peer device information.
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  sessionId :  Session ID for which ranging shall start
**
** Returns:         If Success UWA_STATUS_OK  else UWA_STATUS_FAILED
**
*******************************************************************************/
jbyte uwbNativeManager_startRanging(JNIEnv* env, jobject obj, jint sessionId) {
  static const char fn[] = "uwbNativeManager_startRanging";
  UNUSED(fn);
  tUWA_STATUS status = UWA_STATUS_FAILED;
  JNI_TRACE_I("%s: enter", fn);

  if (!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not enabled", fn);
    return status;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return status;
  }

  sRangeStartStatus = false;
  initializeSessionStatus(sessionId, UWB_SESSION_ACTIVE);
  {
    SyncEventGuard guard(sUwaRngStartEvent);
    status = UWA_StartRangingSession(sessionId);
    if (status == UWA_STATUS_OK)
      sUwaRngStartEvent.wait(UWB_CMD_TIMEOUT);
  }
  if(sRangeStartStatus) {
    sRangeStartStatus = waitForSessionStatusNotification();
  }
  JNI_TRACE_I("%s: exit status = 0x%x sRangeStartStatus = %s", fn, status, sRangeStartStatus?"true":"false");
  return (sRangeStartStatus)? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_stopRanging
**
** Description:     stop the ranging session.
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  sessionId :  Session ID for which ranging shall start
**
** Returns:         UWA_STATUS_OK if ranging session stop is success.
**
*******************************************************************************/
jbyte uwbNativeManager_stopRanging(JNIEnv* env, jobject obj, jint sessionId) {
  static const char fn[] = "uwbNativeManager_stopRanging";
  UNUSED(fn);
  tUWA_STATUS status = UWA_STATUS_FAILED;
  JNI_TRACE_I("%s: enter", fn);
  if (!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not enabled", fn);
    return status;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return status;
  }

  sRangeStopStatus = false;
  initializeSessionStatus(sessionId, UWB_SESSION_IDLE);
  {
    SyncEventGuard guard(sUwaRngStopEvent);
    status = UWA_StopRangingSession(sessionId);
    if(status == UWA_STATUS_OK)
      sUwaRngStopEvent.wait(UWB_CMD_TIMEOUT);
  }
  if(sRangeStopStatus) {
    sRangeStopStatus = waitForSessionStatusNotification();
  }
  JNI_TRACE_I("%s: exit status = 0x%x sRangeStopStatus = %s", fn, status, sRangeStopStatus?"true":"false");
  return (sRangeStopStatus)? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_getSessionCount
**
** Description:     Get session count.
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         session count on success
**
*******************************************************************************/
jbyte uwbNativeManager_getSessionCount(JNIEnv* env, jobject obj) {
  static const char fn[] = "uwbNativeManager_getSessionCount";
  UNUSED(fn);
  tUWA_STATUS status;
  sSessionCount = -1;
  JNI_TRACE_I("%s: Enter", fn);

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", fn);
    return sSessionCount;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return sSessionCount;
  }

  SyncEventGuard guard(sUwaGetSessionCountEvent);
  status = UWA_GetSessionCount();
  if(UWA_STATUS_OK == status){
    sUwaGetSessionCountEvent.wait(UWB_CMD_TIMEOUT);
  } else {
    JNI_TRACE_E("%s: get session count command is  failed", fn);
  }
  JNI_TRACE_I("%s: Exit", fn);
  return sSessionCount;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_getSessionState
**
** Description:     get the current session status for the given session id
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  sessionId :  Session ID for which to get the session status
**
** Returns:         current session status if UWb_STATUS_OK else returns
**                  UWA_STATUS_FAILED.
**
*******************************************************************************/
jbyte uwbNativeManager_getSessionState(JNIEnv* env, jobject obj, jint sessionId) {
  static const char fn[] = "uwbNativeManager_getSessionState";
  UNUSED(fn);
  tUWA_STATUS status;
  JNI_TRACE_I("%s: enter", fn);
  sSessionState = UWB_UNKNOWN_SESSION;

  if (!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not enabled", fn);
    return sSessionState;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return sSessionState;
  }

  SyncEventGuard guard(sUwaGetSessionStatusEvent);
  status = UWA_GetSessionStatus(sessionId);
  if (status == UWA_STATUS_OK) {
    sUwaGetSessionStatusEvent.wait(UWB_CMD_TIMEOUT);
  }
  JNI_TRACE_I("%s: exit", fn);
  return sSessionState;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_enableRangeDataNtf
**
** Description:     Enable or disable the range data ntf.
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  sessionId: Session Id To which enable/Disable the Ranging
**                  enable: Option to set enable/disable
**
** Returns:         if success UWb_STATUS_OK else returns
**                  UWA_STATUS_FAILED.
**
********************************************************************************/
jbyte uwbNativeManager_enableRangeDataNtf(JNIEnv* env, jobject o, jint sessionId, jbyte enable) {
  static const char fn[] = "uwbNativeManager_enableRangeDataNtf";
  UNUSED(fn);
  tUWA_STATUS status = UWA_STATUS_FAILED;
  uint8_t appConfigData[] = { UCI_PARAM_ID_RNG_DATA_NTF, UCI_PARAM_LEN_RNG_DATA_NTF, (uint8_t)enable};
  uint8_t numOfConfigIds = 1;
  JNI_TRACE_I("%s: Enter: sessionId = %d, enable=%d", fn, sessionId, enable);

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", fn);
    return status;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return status;
  }

  status = setAppConfiguration(sessionId, numOfConfigIds, sizeof(appConfigData), appConfigData);
  JNI_TRACE_I("%s: Exit", fn);
  return status;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_setRangingDataSamplingRate
**
** Description:     set the sampling rate to get the Average ranging data.
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  sessionId: Session Id to which set the sampling rate
**                  samplingRate: Required ranging sampling rate
**
** Returns:         if success UWb_STATUS_OK else returns
**                  UWA_STATUS_FAILED
**
*******************************************************************************/
jbyte uwbNativeManager_setRangingDataSamplingRate(JNIEnv* env, jobject o, jint sessionId, jbyte samplingRate) {
  static const char fn[] = "uwbNativeManager_setRangingDataSamplingRate";
  UNUSED(fn);
  JNI_TRACE_I("%s: Enter", fn);

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", fn);
    return UWA_STATUS_FAILED;
  }

  JNI_TRACE_E("%s: sessionId: %x, samplingRate set is %d", fn, sessionId, samplingRate);
  {
    std::unique_lock<std::mutex> lock(sSessionMutex);
    if(samplingRate > 1) {
        auto it = sAveragedRangingData.find(sessionId);
        if (it == sAveragedRangingData.end()) {
            SessionRangingData sessionData;
            memset(&sessionData, 0, sizeof(sessionData));
            sAveragedRangingData.insert({sessionId, sessionData});
        }
        sAveragedRangingData[sessionId].samplingRate = samplingRate;
        JNI_TRACE_E("%s: Averaging Enabled for session Id %d", fn, sessionId);
    } else {
        sAveragedRangingData.erase(sessionId);
        JNI_TRACE_E("%s: Averaging Disabled for session Id %d since sampling rate is %d", fn, sessionId, samplingRate);
    }
  }
  JNI_TRACE_I("%s: Exit", fn);
  return UWA_STATUS_OK;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_getRangingCount()
**
** Description:     application can use this API to get Ranging Count
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  sessionId : Session Id for which get the Ranging Count
**
** Returns:         ranging count on success else 0 on Failure
**
*******************************************************************************/
jint uwbNativeManager_getRangingCount(JNIEnv* env, jobject o, int sessionId) {
  static const char fn[] = "uwbNativeManager_getRangingCount";
  UNUSED(fn);
  sRangingCount = 0;
  tUWA_STATUS status;
  JNI_TRACE_I("%s: Enter", fn);

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", fn);
    return sRangingCount;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return sRangingCount;
  }

  SyncEventGuard guard(sUwaGetRangingCountEvent);
  status = UWA_GetRangingCount(sessionId);
  if(UWA_STATUS_OK == status){
    sUwaGetRangingCountEvent.wait(UWB_CMD_TIMEOUT);
  } else {
    JNI_TRACE_E("%s: get ranging count command is  failed", fn);
  }
  JNI_TRACE_I("%s: Exit", fn);
  return sRangingCount;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_doRecovery()
**
** Description:     UWB service shall apply defaut configurations on UWBD soft reset
**
** Returns:         UWb_STATUS_OK on success else UWA_STATUS_FAILED
**
*******************************************************************************/
jbyte uwbNativeManager_doRecovery(){
  static const char fn[] = "uwbNativeManager_doRecovery";
  UNUSED(fn);
  tUWA_STATUS status = UWA_STATUS_FAILED;
  JNI_TRACE_I("%s: Enter; ", fn);

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", fn);
    return status;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return status;
  }

  clearAllSessionContext();
  status = SetCoreDeviceConfigurations();
  if (status == UWA_STATUS_OK) {
    JNI_TRACE_I("%s: CoreDeviceConfigs are Success", fn);
    status = SetExtDeviceConfigurations();
    if (status == UWA_STATUS_OK) {
      JNI_TRACE_I("%s: setInitExtDeviceConfigurations is SUCCESS %d", fn,status);
    } else {
      JNI_TRACE_I("%s: setInitExtDeviceConfigurations is Failed %d", fn,status);
    }
  } else {
    JNI_TRACE_I("%s: CoreDeviceConfigs are Failed ", fn);
  }
  JNI_TRACE_I("%s: Exit status=%d; ", fn, status);
  return (status == UWA_STATUS_OK) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_getAntennaPairConfig()
**
** Description:     read the antenna pair config from the config file
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  antennaConfig: Optionto set the Antenna Config
**
** Returns:         config value
**
*******************************************************************************/
jshort uwbNativeManager_getAntennaPairConfig(JNIEnv* env, jobject o, jbyte antennaConfig) {
  static const char fn[] = "uwbNativeManager_getAntennaPairConfig";
  UNUSED(fn);
  jshort num = 0;
  JNI_TRACE_I("%s: enter; ", fn);
  if(antennaConfig == (jbyte)ANTENNA_CONFIG_1){
    return sAntennaConfig[0];
  } else if (antennaConfig == (jbyte)ANTENNA_CONFIG_2){
    return sAntennaConfig[1];
  } else if (antennaConfig == (jbyte)ANTENNA_CONFIG_3){
    return sAntennaConfig[2];
  } else if (antennaConfig == (jbyte)ANTENNA_CONFIG_4){
    return sAntennaConfig[3];
  } else if (antennaConfig == (jbyte)ANTENNA_CONFIG_5){
    return sAntennaConfig[4];
  } else {
    JNI_TRACE_E("%s: antenna Config is not matched ", fn);
  }
  return num;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_ControllerMulticastListUpdate()
**
** Description:     API to set Controller Multicast List Update
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  sessionId: Session Id to which update the list
**                  action: Required Action to be taken
**                  noOfControlees: Number of Responders
**                  shortAddressList: Short Address list for each responder
**                  subSessionIdList: Sub session Id list of each responder
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte uwbNativeManager_ControllerMulticastListUpdate(JNIEnv* env, jobject o, jint sessionId, jbyte action,
       jbyte noOfControlees, jshortArray shortAddressList, jintArray subSessionIdList) {
  static const char fn[] = "uwbNativeManager_ControllerMulticastListUpdate";
  UNUSED(fn);
  tUWA_STATUS status = UWA_STATUS_FAILED;
  uint16_t* shortAddressArray = NULL;
  uint32_t* subSessionIdArray = NULL;
  JNI_TRACE_E("%s: enter; ", fn);

  if(!gIsUwaEnabled) {
      JNI_TRACE_E("%s: UWB device is not initialized", fn);
      return status;
  }

  if(gIsCalibrationOngoing) {
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return status;
  }

  if((shortAddressList == NULL) || (subSessionIdList == NULL)) {
    JNI_TRACE_E("%s: subSessionIdList or shortAddressList value is NULL", fn);
    return status;
  }
  uint8_t shortAddressLen = env->GetArrayLength(shortAddressList);
  uint8_t subSessionIdLen = env->GetArrayLength(subSessionIdList);
  if (noOfControlees > MAX_NUM_CONTROLLEES) {
      JNI_TRACE_E("%s: no Of Controlees %d exceeded than %d ", fn, shortAddressLen, MAX_NUM_CONTROLLEES);
      return status;
  }

  if((shortAddressLen > 0) && (subSessionIdLen > 0)) {
    shortAddressArray = (uint16_t*) malloc(SHORT_ADDRESS_LEN * shortAddressLen);
    if (shortAddressArray == NULL) {
        JNI_TRACE_E("%s: malloc failure for shortAddressArray", fn);
        return status;
    }
    memset(shortAddressArray, 0, (SHORT_ADDRESS_LEN * shortAddressLen));
    env->GetShortArrayRegion(shortAddressList, 0, shortAddressLen, (jshort*)shortAddressArray);

    subSessionIdArray = (uint32_t*) malloc(SESSION_ID_LEN * subSessionIdLen);
    if (subSessionIdArray == NULL) {
        free(shortAddressArray);
        JNI_TRACE_E("%s: malloc failure for subSessionIdArray", fn);
        return status;
    }
    memset(subSessionIdArray, 0, (SESSION_ID_LEN * subSessionIdLen));
    env->GetIntArrayRegion(subSessionIdList, 0, subSessionIdLen, (jint*)subSessionIdArray);

    sMulticastListUpdateStatus = false;
    SyncEventGuard guard(sUwaMulticastListUpdateEvent);
    status = UWA_ControllerMulticastListUpdate(sessionId, action, noOfControlees, shortAddressArray, subSessionIdArray);
    if (status == UWA_STATUS_OK) {
      sUwaMulticastListUpdateEvent.wait(UWB_CMD_TIMEOUT);
    }
    free(shortAddressArray);
    free(subSessionIdArray);
  } else {
    JNI_TRACE_E("%s: controleeListArray length is not valid", fn);
  }
  JNI_TRACE_I("%s: exit", fn);
  return (sMulticastListUpdateStatus)? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        uwbManager_sendBlinkData()
**
** Description:     API to test uwb UART connectivity
**
** Params:          e: JVM environment.
**                  o: Java object.
**                  sessionId: Session Id
**                  repetitionCount: Number of times Application Data is added
**                  in Payload of Blink Message.
**                  appData: Application specific data.
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte uwbManager_sendBlinkData(JNIEnv* env, jobject o, jint sessionId, jbyte repetitionCount, jbyteArray appData) {
  static const char fn[] = "uwbManager_sendBlinkData";
  UNUSED(fn);
  tUWA_STATUS status = UWA_STATUS_FAILED;
  JNI_TRACE_I("%s: enter; ", fn);

  if(!gIsUwaEnabled) {
      JNI_TRACE_E("%s: UWB device is not initialized", fn);
      return status;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return status;
  }

  uint8_t len = env->GetArrayLength(appData);

  if (len > UCI_MAX_PAYLOAD_SIZE) {
      JNI_TRACE_E("%s: len %d is beyond max allowed range %d", fn, len, UCI_MAX_PAYLOAD_SIZE);
      status = UWA_STATUS_DATA_MAX_TX_PSDU_SIZE_EXCEEDED;
      return status;
  }

  if(len > 0) {
    uint8_t* appDataArray = (uint8_t*) malloc(sizeof(uint8_t) * len);
    if (appDataArray == NULL) {
        JNI_TRACE_E("%s: malloc failure for appDataArray", fn);
        return status;
    }
    memset(appDataArray, 0, (sizeof(uint8_t) * len));
    env->GetByteArrayRegion(appData, 0, len, (jbyte*)appDataArray);

    sSendBlinkDataStatus = UWA_STATUS_FAILED;
    SyncEventGuard guard(sUwaSendBlinkDataEvent);
    status = UWA_SendBlinkData(sessionId, repetitionCount, len, appDataArray);
    if (status == UWA_STATUS_OK) {
      sUwaSendBlinkDataEvent.wait(UWB_CMD_TIMEOUT);
    }
    free(appDataArray);
  } else {
    JNI_TRACE_E("%s: appData length is not valid", fn);
  }

  JNI_TRACE_I("%s: exit status= 0x%x", fn, status);
  return sSendBlinkDataStatus;
}

/*******************************************************************************
**
** Function:        uwbManager_sendData()
**
** Description:     API to send application data over UWB to remote UWB device
**
** Params:          e: JVM environment.
**                  o: Java object.
**                  sessionId: Session Id
**                  address: Remote UWB device adddress
**                  appData: Application data.
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte uwbManager_sendData(JNIEnv* env, jobject o, jint sessionId, jbyteArray address, jbyteArray appData) {
  static const char fn[] = "uwbManager_sendData";
  UNUSED(fn);
  tUWA_STATUS status = UWA_STATUS_FAILED;
  uint8_t* pData = NULL;
  uint8_t* pAddress = NULL;
  JNI_TRACE_I("%s: enter; ", fn);

  if(!gIsUwaEnabled) {
      JNI_TRACE_E("%s: UWB device is not initialized", fn);
      return status;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", fn);
    return status;
  }

  uint16_t dataLen = env->GetArrayLength(appData);
  uint8_t addrLen = env->GetArrayLength(address);

  if((dataLen > 0) && (addrLen == EXTENDED_ADDRESS_LEN)) {
    pData = (uint8_t*) malloc(sizeof(uint8_t) * dataLen);
    if (pData == NULL) {
        JNI_TRACE_E("%s: malloc failure for pData", fn);
        return status;
    }
    pAddress = (uint8_t*) malloc(sizeof(uint8_t) * addrLen);
    if (pAddress == NULL) {
        JNI_TRACE_E("%s: malloc failure for pAddress", fn);
        free(pData);
        return status;
    }
  }else{
    JNI_TRACE_E("%s: data or address is null", fn);
    return status;
  }
  memset(pData, 0, (sizeof(uint8_t) * dataLen));
  env->GetByteArrayRegion(appData, 0, dataLen, (jbyte*)pData);
  memset(pAddress, 0, (sizeof(uint8_t) * addrLen));
  env->GetByteArrayRegion(address, 0, addrLen, (jbyte*)pAddress);

  sSendDataStatus = UWA_STATUS_FAILED;
  SyncEventGuard guard(sUwaSendDataEvent);
  status = UWA_SendUwbData(sessionId, addrLen, pAddress, dataLen, pData);
  if (status == UWA_STATUS_OK) {
    sUwaSendDataEvent.wait(UWB_CMD_TIMEOUT);
  }
  free(pData);
  free(pAddress);
  JNI_TRACE_I("%s: exit status= 0x%x", fn, sSendDataStatus);
  return sSendDataStatus;
}

/*******************************************************************************
**
** Function:        uwbNativeManager_init
**
** Description:     Initialize variables.
**
** Params           env: JVM environment.
**                  o: Java object.
**
** Returns:         True if ok.
**
*******************************************************************************/
jboolean uwbNativeManager_init(JNIEnv* env, jobject o) {
    uwbEventManager.doLoadSymbols(env, o);
    return JNI_TRUE;
}

/*****************************************************************************
**
** JNI functions for android
** UWB service layer has to invoke these APIs to get required funtionality
**
*****************************************************************************/
static JNINativeMethod gMethods[] = {
    {"nativeInit", "()Z", (void*)uwbNativeManager_init},
    {"nativeDoInitialize", "()Z", (void*)uwbNativeManager_doInitialize},
    {"nativeDoDeinitialize", "()Z", (void*)uwbNativeManager_doDeinitialize},
    {"nativeDeviceReset","(B)B", (void*)uwbNativeManager_deviceReset},
    {"nativeGetUwbDeviceState", "()I", (void*)uwbNativeManager_getUwbDeviceState},
    {"nativeGetDeviceInfo", "()Lcom/android/uwb/UwbDeviceData;",(void*)uwbNativeManager_getDeviceInfo},
    {"nativeGetAppConfigurations", "(III[B)[B", (void*)uwbNativeManager_getAppConfigurations},
    {"nativeSessionInit","(IB)B", (void*)uwbNativeManager_sessionInit},
    {"nativeSessionDeInit","(I)B", (void*)uwbNativeManager_sessionDeInit},
    {"nativeSetAppConfigurations", "(III[B)[B", (void*)uwbNativeManager_setAppConfigurations},
    {"nativeRangingStart", "(I)B", (void*)uwbNativeManager_startRanging},
    {"nativeRangingStop", "(I)B", (void*)uwbNativeManager_stopRanging},
    {"nativeGetSessionCount","()B", (void*)uwbNativeManager_getSessionCount},
    {"nativeEnableRangeDataNtf","(IB)B", (void*)uwbNativeManager_enableRangeDataNtf},
    {"nativeSetRangingDataSamplingRate","(IB)B", (void*)uwbNativeManager_setRangingDataSamplingRate},
    {"nativeDoRecovery","()B", (void*)uwbNativeManager_doRecovery},
    {"nativeGetRangingCount","(I)I", (void*)uwbNativeManager_getRangingCount},
    {"nativeGetSessionState", "(I)B", (void*)uwbNativeManager_getSessionState},
    {"nativeControllerMulticastListUpdate", "(IBB[S[I)B", (void*)uwbNativeManager_ControllerMulticastListUpdate},
    {"nativeSendBlinkData", "(IB[B)B", (void*)uwbManager_sendBlinkData},
    {"nativeSendData", "(I[B[B)B", (void*)uwbManager_sendData}
};


/*******************************************************************************
**
** Function:        register_UwbNativeManager
**
** Description:     Regisgter JNI functions of UwbEventManager class with Java Virtual Machine.
**
** Params:          env: Environment of JVM.
**
** Returns:         Status of registration (JNI version).
**
*******************************************************************************/
int register_com_android_uwb_dhimpl_UwbNativeManager(JNIEnv* env) {
  static const char fn[] = "register_com_android_uwb_dhimpl_UwbNativeManager";
  UNUSED(fn);
  JNI_TRACE_I("%s: enter", fn);
  return jniRegisterNativeMethods(env, UWB_NATIVE_MANAGER_CLASS_NAME, gMethods,
                                  sizeof(gMethods)/sizeof(gMethods[0]));
}

}
