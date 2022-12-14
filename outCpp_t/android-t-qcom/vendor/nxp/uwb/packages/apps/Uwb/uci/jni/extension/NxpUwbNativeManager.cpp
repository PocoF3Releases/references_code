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
#include "NxpUwbManager.h"
#include "JniLog.h"
#include "uwb_config.h"
#include "uci_ext_defs.h"

namespace android {

const char* NXP_UWB_NATIVE_MANAGER_CLASS_NAME = "com/android/uwb/dhimpl/NxpUwbNativeManager";
static uint16_t sAntennaConfig[4] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };

static NxpUwbManager &nxpUwbManager = NxpUwbManager::getInstance();

/*******************************************************************************
**
** Function:        loadAntennaConfigs()
**
** Description:     read the antenna pair config from the config file
**
** Params:          None
**
** Returns:         None
**
*******************************************************************************/
void loadAntennaConfigs()
{
    JNI_TRACE_I("%s: enter; ", __func__);
    if (UwbConfig::hasKey(NAME_ANTENNA_PAIR_SELECTION_CONFIG1))
        sAntennaConfig[0] = (uint16_t)UwbConfig::getUnsigned(NAME_ANTENNA_PAIR_SELECTION_CONFIG1);

    if (UwbConfig::hasKey(NAME_ANTENNA_PAIR_SELECTION_CONFIG2))
     sAntennaConfig[1] = (uint16_t)UwbConfig::getUnsigned(NAME_ANTENNA_PAIR_SELECTION_CONFIG2);

    if (UwbConfig::hasKey(NAME_ANTENNA_PAIR_SELECTION_CONFIG3))
     sAntennaConfig[2] = (uint16_t)UwbConfig::getUnsigned(NAME_ANTENNA_PAIR_SELECTION_CONFIG3);

    if (UwbConfig::hasKey(NAME_ANTENNA_PAIR_SELECTION_CONFIG4))
     sAntennaConfig[3] = (uint16_t)UwbConfig::getUnsigned(NAME_ANTENNA_PAIR_SELECTION_CONFIG4);

    JNI_TRACE_I("%s: exit; ", __func__);

}

/*******************************************************************************
**
** Function:        SetExtDeviceConfigurations
**
** Description:     Set the Ext Core Device Config
**
**
** Returns:         If Success UWA_STATUS_OK  else UWA_STATUS_FAILED
**
*******************************************************************************/
tUWA_STATUS SetExtDeviceConfigurations(){
  uint8_t extDeviceConfigsCount = 2;
  tUWA_STATUS status;
  uint8_t configParam[] = {0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00};
  uint16_t config = 0;
  uint8_t VendorExtensionEnable = 0x01; /* NXP_EXTENDED_NTF_CONFIG to be 1 always except on conformance test */
  JNI_TRACE_I("%s: Enter ", __func__);

  uint8_t *configPtr = &configParam[0];
  config = UwbConfig::getUnsigned(NAME_UWB_DPD_ENTRY_TIMEOUT, 0x00);
  JNI_TRACE_I("%s: NAME_UWB_DPD_ENTRY_TIMEOUT value %d ", __func__,(uint8_t)config);

  if((config >= DPD_TIMEOUT_MIN) && (config <= DPD_TIMEOUT_MAX)) {
    UINT8_TO_STREAM(configPtr, EXTENDED_DEVICE_CONFIG_ID);
    UINT8_TO_STREAM(configPtr, UCI_EXT_PARAM_ID_DPD_ENTRY_TIMEOUT);
    UINT8_TO_STREAM(configPtr, UCI_EXT_PARAM_ID_DPD_ENTRY_TIMEOUT_LEN);
    UINT16_TO_STREAM(configPtr, config);
  } else {
    JNI_TRACE_E("%s: Invalid Range for DPD Entry Timeout in ConfigFile", __func__);
  }

  JNI_TRACE_I("%s: NXP_EXTENDED_NTF value %d ", __func__,(uint8_t)VendorExtensionEnable);

  UINT8_TO_STREAM(configPtr, EXTENDED_DEVICE_CONFIG_ID);
  UINT8_TO_STREAM(configPtr, UCI_EXT_PARAM_ID_NXP_EXTENDED_NTF_CONFIG);
  UINT8_TO_STREAM(configPtr, UCI_EXT_PARAM_ID_NXP_EXTENDED_NTF_CONFIG_LEN);
  UINT8_TO_STREAM(configPtr, VendorExtensionEnable);

  uint8_t deviceConfig[UCI_MAX_PAYLOAD_SIZE];
  uint8_t configLen;
  bool ret = nxpUwbManager.setExtDeviceConfigurations(extDeviceConfigsCount, sizeof(configParam), &configParam[0], &deviceConfig[0], &configLen);
  status = ret ? UWA_STATUS_OK : UWA_STATUS_FAILED;
  if(status == UWA_STATUS_OK) {
    JNI_TRACE_I("%s: setExtDeviceConfigurations Succes", __func__);
  } else {
      JNI_TRACE_E("%s: setExtDeviceConfigurations failed", __func__);
  }

  JNI_TRACE_I("%s: Exit ", __func__);
  return status;
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_setCalibratationIntegrityProtection()
**
** Description:     API to do Set Calibration Integrity Protection
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         Returns byte
**
*******************************************************************************/
jbyte nxpUwbNativeManager_setCalibrationIntegrityProtection(JNIEnv* env, jobject o, jbyte tagOption, jshort calibParamBitmask) {
    return nxpUwbManager.setCalibrationIntegrityProtection(env, o, tagOption, calibParamBitmask);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_getExtDeviceCapability()
**
** Description:     API to get device capability info
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         Returns byte array
**
*******************************************************************************/
jbyteArray nxpUwbNativeManager_getExtDeviceCapability(JNIEnv* env, jobject o) {
    return nxpUwbManager.getExtDeviceCapability(env, o);
}

/*********************************************************************************
**
** Function:        nxpUwbNativeManager_setExtDeviceConfigurations()
**
** Description:     This API is invoked to set the Extended Device specific Config
** Params:          env: JVM environment.
**                  o: Java object.
**                  noOfParams: Number of Params
**                  deviceConfigLen: Total Device config Lentgh
**                  extDeviceConfigArray:  Extended Device Config List to retreive
**
** Returns:         Returns byte array
**
**********************************************************************************/
jbyteArray nxpUwbNativeManager_setExtDeviceConfigurations(JNIEnv* env, jobject o, int noOfParams, int deviceConfigLen, jbyteArray extDeviceConfigArray) {
    return nxpUwbManager.setExtDeviceConfigurations(env, o, noOfParams, deviceConfigLen, extDeviceConfigArray);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_getExtDeviceConfigurations
**
** Description:     retrive the UWB Ext Device Config information etc.
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         ExtDeviceConfigInfo class object or NULL.
**
*******************************************************************************/
jbyteArray nxpUwbNativeManager_getExtDeviceConfigurations(JNIEnv* env, jobject o, jint noOfParams,
       jint deviceConfigLen, jbyteArray deviceConfigArray) {
    return nxpUwbManager.getExtDeviceConfigurations(env, o, noOfParams, deviceConfigLen, deviceConfigArray);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_setDebugConfigurations()
**
** Description:     application can use this API to configure Debug parameters
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  sessionId : Session Id To which set the Debug Config
**                  noOfParams: Number of Debug cofnfig
**                  debugConfigLen: Total Debug Config list length
**                  debugConfigArray: Debug Config Data
**                  enableCirDump: Option to enable/Disable CIR dump
**                  enableFwDump: Option to enable/Disable FW dump
**
** Returns:         UWb_STATUS_OK on success else UWA_STATUS_FAILED
**
*******************************************************************************/
jbyteArray nxpUwbNativeManager_setDebugConfigurations(JNIEnv* env, jobject o, int sessionId, int noOfParams,
       int debugConfigLen, jbyteArray debugConfigArray, jboolean enableCirDump, jboolean enableFwDump) {
    return nxpUwbManager.setDebugConfigurations(env, o, sessionId, noOfParams, debugConfigLen, debugConfigArray, enableCirDump, enableFwDump);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_bind()
**
** Description:     API to performing binding
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         UWb_STATUS_OK on success else UWA_STATUS_FAILED
**
*******************************************************************************/
jbyte nxpUwbNativeManager_bind(JNIEnv* env, jobject o) {
    return nxpUwbManager.bind(env, o);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_getBindingStatus()
**
** Description:     API to get Binding Status
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         UWb_STATUS_OK on success else UWA_STATUS_FAILED
**
*******************************************************************************/
jbyte nxpUwbNativeManager_getBindingStatus(JNIEnv* env, jobject o) {
    return nxpUwbManager.getBindingStatus(env, o);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_getBindingCount()
**
** Description:     application can use this API to get Binding Count
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         binding count on success else 0 on Failure
**
*******************************************************************************/
jbyteArray nxpUwbNativeManager_getBindingCount(JNIEnv* env, jobject o) {
    return nxpUwbManager.getBindingCount(env, o);
}


/*******************************************************************************
**
** Function:        nxpUwbNativeManager_startLoopBackTest()
**
** Description:     application can use this API to start SE Loopback Test
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  loopCount: Number of loop
**                  interval: Interval in ms
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte nxpUwbNativeManager_startLoopBackTest(JNIEnv* env, jobject o, int loopCount, int interval) {
    return nxpUwbManager.startLoopBackTest(env, o, loopCount, interval);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_startConnectivityTest()
**
** Description:     API to check SE connectivity
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte nxpUwbNativeManager_startConnectivityTest(JNIEnv* env, jobject o) {
    return nxpUwbManager.startConnectivityTest(env, o);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_setCalibration()
**
** Description:     API to set calibration value
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  channelNo: Channel number
**                  paramType: Parameter Type
**                  calibrationValue: Required Calibration value
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte nxpUwbNativeManager_setCalibration(JNIEnv* env, jobject o, jbyte channelNo, jbyte paramType,
       jbyteArray calibrationValue) {
    return nxpUwbManager.setCalibration(env, o, channelNo, paramType, calibrationValue);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_configureAuthTagOption()
**
** Description:     API to configure tag options
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  deviceTag: Device Tag
**                  modelTag: Model Tag
**                  label: Label used for Model Specefic Tag Generation.
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/

jbyte nxpUwbNativeManager_configureAuthTagOption(JNIEnv* env, jobject o, jbyte deviceTag, jbyte modelTag,
       jbyteArray label) {
    return nxpUwbManager.configureAuthTagOption(env, o, deviceTag, modelTag, label);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_getCalibration()
**
** Description:     API to get calibration value
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  channelNo: Channel number
**                  paramType: Parameter type
**
** Returns:         calibration value
**
*******************************************************************************/
jbyteArray nxpUwbNativeManager_getCalibration(JNIEnv* env, jobject o, jbyte channelNo, jbyte paramType) {
    return nxpUwbManager.getCalibration(env, o, channelNo, paramType);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_performCalibration()
**
** Description:     API to perform calibration
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  channelNo: Channel number
**                  paramType: Parameter type
**
** Returns:         returns calibration value in byte array form if calibration is success, otherwise NULL
**
*******************************************************************************/
jbyteArray nxpUwbNativeManager_performCalibration(JNIEnv* env, jobject o, jbyte channelNo, jbyte paramType) {
    return nxpUwbManager.performCalibration(env, o, channelNo, paramType);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_getUwbDeviceTemperature()
**
** Description:     API to read UWBS temperature
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         returns temperature value in byte array if it is success
**                  otherwise NULL
**
*******************************************************************************/
jbyteArray nxpUwbNativeManager_getUwbDeviceTemperature(JNIEnv* env, jobject o) {
    return nxpUwbManager.getUwbDeviceTemperature(env, o);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_setWtxCount()
**
** Description:     API to set WtxCount
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  count: Required wtx count
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte nxpUwbNativeManager_setWtxCount(JNIEnv* env, jobject o,jbyte count) {
    return nxpUwbManager.setWtxCount(env, o, count);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_uartConnectivityTest()
**
** Description:     API to test uwb UART connectivity
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  payload: Required data to send to test uart connectivity
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte nxpUwbNativeManager_uartConnectivityTest(JNIEnv* env, jobject o, jbyteArray payload) {
    return nxpUwbManager.uartConnectivityTest(env, o, payload);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_generateSecureTag()
**
** Description:     API to generate the cmac tag
**
** Params:          e: JVM environment.
**                  o: Java object.
**                  tagOption: Device Specific or model specific config option
**
** Returns:         status + cmacTag on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyteArray nxpUwbNativeManager_generateSecureTag(JNIEnv* env, jobject o,jbyte tagOption) {
    return nxpUwbManager.generateSecureTag(env, o, tagOption);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_verifySecureTag()
**
** Description:     API to verify cmac tag
**
** Params:          e: JVM environment.
**                  o: Java object.
**                  cmacTag: cmac tag required to be verify
**                  tagOption: Device Specific or model specific config option
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte nxpUwbNativeManager_verifySecureTag(JNIEnv* env, jobject o, jbyteArray cmacTag, jbyte tagOption, jshort tagVersion) {
    return nxpUwbManager.verifySecureTag(env, o, cmacTag, tagOption, tagVersion);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_configureTagVersion()
**
** Description:     API to configure Tag version
**
** Params:          e: JVM environment.
**                  o: Java object.
**                  version: Config Tag version number
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte nxpUwbNativeManager_configureTagVersion(JNIEnv* env, jobject o, jshort version) {
    return nxpUwbManager.configureTagVersion(env, o, version);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_nativeSetCalibrationIntegrityProtection()
**
** Description:     API to configure the tag option and calibration parameters to considered
**                  during tag generation and verification process.
**
** Params:          e: JVM environment.
**                  o: Java object.
**                  tagOption: Device Specific or model specific Tag (1 byte)
**                  calibParamBitmask: calibration parameters  (2 bytes)
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte nxpUwbNativeManager_nativeSetCalibrationIntegrityProtection(JNIEnv* env, jobject o, jbyte tagOption, jint calibParamBitmask) {
    return nxpUwbManager.setCalibrationIntegrityProtection(env,o,tagOption,calibParamBitmask);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_nativeGetUwbDeviceTimeStamp()
**
** Description:     API to get the Uwb Device TimeStamp Information
**
** Params:          e: JVM environment.
**                  o: Java object.
**
** Returns:         UwbTimeStamp Info Class Object on success
**
*******************************************************************************/
jbyteArray nxpUwbNativeManager_nativeGetUwbDeviceTimeStamp(JNIEnv* env, jobject o) {
    return nxpUwbManager.getUwbDeviceTimeStamp(env,o);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_nativeGetRanMultiplier()
**
** Description:     API to get Ran Multiplier Value
**
** Params:          e: JVM environment.
**                  o: Java object.
**
** Returns:         Ran Multiplier Value on success
**
*******************************************************************************/
jbyte nxpUwbNativeManager_nativeGetRanMultiplier(JNIEnv* env, jobject o) {
    return nxpUwbManager.getRanMultiplier(env,o);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_eraseURSK()
**
** Description:     API to Delete RDS/URSK from the SecureElement
**
** Params:          e: JVM environment.
**                  o: Java object.
**                  numberSessionIds: Number of Session Id's URSK to be deleted
**                  sessionIdsList: List Of Session Ids for which URSK to be deleted
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte nxpUwbNativeManager_eraseURSK(JNIEnv* env, jobject o, jbyte numberSessionIds, jintArray sessionIdsList) {
    return nxpUwbManager.eraseURSK(env,o,numberSessionIds,sessionIdsList);
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_getAllUwbSessions()
**
** Description:     API to get All Uwb session data
**
** Params:          e: JVM environment.
**                  o: Java object.
**
** Returns:         jbyteArray on success else null on Failure
**
*******************************************************************************/
jbyteArray nxpUwbNativeManager_getAllUwbSessions(JNIEnv* env, jobject o) {
    return nxpUwbManager.getAllUwbSessions(env, o);
}

/*******************************************************************************
**
** Function:        uwbNativeManager_enableConformanceTest
**
** Description:     Enable or disable MCTT mode of operation.
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  enable: enable/disable MCTT mode
**
** Returns:         if success UWb_STATUS_OK else returns
**                  UWA_STATUS_FAILED.
**
********************************************************************************/
jbyte uwbNativeManager_enableConformanceTest(JNIEnv* env, jobject o, jboolean enable) {
    return nxpUwbManager.enableConformanceTest(env, o, enable);
}

/*******************************************************************************
** Function:        nxpUwbNativeManager_sendRawUci()
**
** Description:     API to send raw uci cmds
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  rawUci: Uci data to send to controller
**                  cmdLen: uci data lentgh
**
** Returns:         raw uci rsp
**
*******************************************************************************/
jbyteArray nxpUwbNativeManager_sendRawUci(JNIEnv* env, jobject o, jbyteArray rawUci, jint cmdLen) {
    return nxpUwbManager.sendRawUci(env, o, rawUci, cmdLen);
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
jshort nxpUwbNativeManager_getAntennaPairConfig(JNIEnv* env, jobject o, jbyte antennaConfig) {
  jshort num = 0;
  JNI_TRACE_I("%s: enter; ", __func__);
  if(antennaConfig == (jbyte)ANTENNA_CONFIG_1){
    return sAntennaConfig[0];
  } else if (antennaConfig == (jbyte)ANTENNA_CONFIG_2){
    return sAntennaConfig[1];
  } else if (antennaConfig == (jbyte)ANTENNA_CONFIG_3){
    return sAntennaConfig[2];
  } else if (antennaConfig == (jbyte)ANTENNA_CONFIG_4){
    return sAntennaConfig[3];
  } else {
    JNI_TRACE_E("%s: antenna Config is not matched ", __func__);
  }
  return num;
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_nativeGetDeviceMaxPPM()
**
** Description:     get the Uwbs Max Ppm Value
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         config value
**
*******************************************************************************/
jint nxpUwbNativeManager_nativeGetDeviceMaxPPM(JNIEnv* env, jobject o) {
  JNI_TRACE_I("%s: enter; ", __func__);
  return android::getDeviceMaxPpmValue();
}

/*******************************************************************************
**
** Function:        nxpUwbNativeManager_init
**
** Description:     Initialize variables.
**
** Params           env: JVM environment.
**                  o: Java object.
**
** Returns:         True if ok.
**
*******************************************************************************/
jboolean nxpUwbNativeManager_init(JNIEnv* env, jobject o) {
    nxpUwbManager.doLoadSymbols(env, o);
    return JNI_TRUE;
}

/*****************************************************************************
**
** JNI functions for android
** UWB service layer has to invoke these APIs to get required funtionality
**
*****************************************************************************/
static JNINativeMethod gMethods[] = {
    {"nativeInit", "()Z", (void*)nxpUwbNativeManager_init},
    {"nativeGetExtDeviceCapability","()[B", (void*)nxpUwbNativeManager_getExtDeviceCapability},
    {"nativeSetExtDeviceConfigurations", "(II[B)[B", (void*)nxpUwbNativeManager_setExtDeviceConfigurations},
    {"nativeGetExtDeviceConfigurations", "(II[B)[B", (void*)nxpUwbNativeManager_getExtDeviceConfigurations},
    {"nativeSetDebugConfigurations", "(III[BZZ)[B", (void*)nxpUwbNativeManager_setDebugConfigurations},
    {"nativeBind", "()B", (void*)nxpUwbNativeManager_bind},
    {"nativeGetBindingStatus", "()B", (void*)nxpUwbNativeManager_getBindingStatus},
    {"nativeGetBindingCount", "()[B", (void*)nxpUwbNativeManager_getBindingCount},
    {"nativeStartLoopBackTest", "(II)B", (void*)nxpUwbNativeManager_startLoopBackTest},
    {"nativeStartConnectivityTest", "()B", (void*)nxpUwbNativeManager_startConnectivityTest},
    {"nativeSetCalibration", "(BB[B)B", (void*)nxpUwbNativeManager_setCalibration},
    {"nativeGetCalibration", "(BB)[B", (void*)nxpUwbNativeManager_getCalibration},
    {"nativeSetWtxCount","(B)B",(void*)nxpUwbNativeManager_setWtxCount},
    {"nativeSendRawUci", "([BI)[B", (void*)nxpUwbNativeManager_sendRawUci},
    {"nativePerformCalibration", "(BB)[B", (void*)nxpUwbNativeManager_performCalibration},
    {"nativeGetUwbDeviceTemperature", "()[B", (void*)nxpUwbNativeManager_getUwbDeviceTemperature},
    {"nativeUartConnectivityTest", "([B)B", (void*)nxpUwbNativeManager_uartConnectivityTest},
    {"nativeVerifySecureTag", "([BBS)B", (void*)nxpUwbNativeManager_verifySecureTag},
    {"nativeGenerateSecureTag", "(B)[B", (void*)nxpUwbNativeManager_generateSecureTag},
    {"nativeConfigureTagVersion", "(S)B", (void*)nxpUwbNativeManager_configureTagVersion},
    {"nativeConfigureAuthTagOption", "(BB[B)B", (void*)nxpUwbNativeManager_configureAuthTagOption},
    {"nativeGetAllUwbSessions", "()[B", (void*)nxpUwbNativeManager_getAllUwbSessions},
    {"nativeSetCalibrationIntegrityProtection", "(BI)B", (void*)nxpUwbNativeManager_nativeSetCalibrationIntegrityProtection},
    {"nativeGetUwbDeviceTimeStamp", "()[B", (void*)nxpUwbNativeManager_nativeGetUwbDeviceTimeStamp},
    {"nativeGetRanMultiplier", "()B", (void*)nxpUwbNativeManager_nativeGetRanMultiplier},
    {"nativeEraseURSK", "(B[I)B", (void*)nxpUwbNativeManager_eraseURSK},
    {"nativeEnableConformanceTest", "(Z)B", (void*)uwbNativeManager_enableConformanceTest},
    {"nativeGetAntennaPairConfig", "(B)S", (void*)nxpUwbNativeManager_getAntennaPairConfig},
    {"nativeGetDeviceMaxPPM", "()I", (void*)nxpUwbNativeManager_nativeGetDeviceMaxPPM}
};

/*******************************************************************************
**
** Function:        register_NxpUwbNativeManager
**
** Description:     Regisgter JNI functions of NxpUwbManager class with Java Virtual Machine.
**
** Params:          env: Environment of JVM.
**
** Returns:         Status of registration (JNI version).
**
*******************************************************************************/
int register_com_android_uwb_dhimpl_NxpUwbNativeManager(JNIEnv* env) {
  JNI_TRACE_I("%s: enter", __func__);
  return jniRegisterNativeMethods(env, NXP_UWB_NATIVE_MANAGER_CLASS_NAME, gMethods,
                                  sizeof(gMethods)/sizeof(gMethods[0]));
}

}