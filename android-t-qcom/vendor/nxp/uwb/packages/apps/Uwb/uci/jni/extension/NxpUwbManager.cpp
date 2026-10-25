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
#include "UwbAdaptation.h"
#include "SyncEvent.h"
#include "uwb_config.h"
#include "uwb_hal_int.h"
#include "JniLog.h"
#include "ScopedJniEnv.h"
#include "uci_ext_defs.h"

namespace android {

SyncEvent gBindStatusNtfEvt;
SyncEvent gSeCommErrNtfEvt;
SyncEvent gGenCmacTagNtfEvt;
SyncEvent gVerifyTagNtfEvt;
SyncEvent gConfigureAuthTagNtfEvt;
uint8_t gBindStatus;

static NxpFeatureData_t sNxpFeatureConf;
static bool isCalibNtfReceived;
static bool isConfigureAuthTagNtfReceived;
static bool isCmacTagNtfReceived;
static bool isVerifyTagNtfReceived;
static bool gIsConformaceTestEnabled;
static SyncEvent gCalibNtfEvt;
static NxpUwbManager &nxpUwbManager = NxpUwbManager::getInstance();

extern bool IsUwbMwDisabled();

NxpUwbManager NxpUwbManager::mObjNxpManager;

NxpUwbManager& NxpUwbManager::getInstance() {
    return mObjNxpManager;
}

NxpUwbManager::NxpUwbManager() {
    mVm = NULL;
    mClass = NULL;
    mObject = NULL;
    mOnExtendedNotificationReceived = NULL;
    mOnRawUciNotificationReceived = NULL;
    gIsConformaceTestEnabled = false;
}

void NxpUwbManager::onExtendedNotificationReceived(uint8_t notificationType, uint8_t* data, uint16_t length) {
    JNI_TRACE_I("%s: Enter", __func__);

    ScopedJniEnv env(mVm);
    if (env == NULL) {
        JNI_TRACE_E("%s: jni env is null", __func__);
        return;
    }

    if(length == 0 || data == NULL) {
        JNI_TRACE_E("%s: length is zero or data is NULL, skip sending notifications", __func__);
        return;
    }

    jbyteArray dataArray = env->NewByteArray(length);
    env->SetByteArrayRegion (dataArray, 0, length, (jbyte *)data);

    if(mOnExtendedNotificationReceived != NULL) {
        env->CallVoidMethod(mObject, mOnExtendedNotificationReceived, notificationType, dataArray);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            JNI_TRACE_E("%s: fail to send notification", __func__);
        }
    } else {
        JNI_TRACE_E("%s: onExtendedNotificationReceived MID is NULL", __func__);
    }
    JNI_TRACE_I("%s: exit", __func__);
}

void NxpUwbManager::onRawUciNotificationReceived(uint8_t* data, uint16_t length) {
    JNI_TRACE_I("%s: Enter", __func__);

    ScopedJniEnv env(mVm);
    if (env == NULL) {
        JNI_TRACE_E("%s: jni env is null", __func__);
        return;
    }

    if(length == 0 || data == NULL) {
        JNI_TRACE_E("%s: length is zero or data is NULL, skip sending notifications", __func__);
        return;
    }

    jbyteArray dataArray = env->NewByteArray(length);
    env->SetByteArrayRegion (dataArray, 0, length, (jbyte *)data);

    if(mOnRawUciNotificationReceived != NULL) {
        env->CallVoidMethod(mObject, mOnRawUciNotificationReceived, dataArray);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            JNI_TRACE_E("%s: fail to send notification", __func__);
        }
    } else {
        JNI_TRACE_E("%s: onRawUciNotificationReceived MID is NULL", __func__);
    }
    JNI_TRACE_I("%s: exit", __func__);
}

void NxpUwbManager::doLoadSymbols(JNIEnv* env, jobject thiz) {
    JNI_TRACE_I("%s: enter", __func__);
    env->GetJavaVM(&mVm);

    jclass clazz = env->GetObjectClass(thiz);
    if (clazz != NULL) {
        mClass = (jclass) env->NewGlobalRef(clazz);
        // The reference is only used as a proxy for callbacks.
        mObject = env->NewGlobalRef(thiz);
        mOnExtendedNotificationReceived = env->GetMethodID(clazz, "onExtendedNotificationReceived", "(B[B)V");
        mOnRawUciNotificationReceived = env->GetMethodID(clazz, "onRawUciNotificationReceived", "([B)V");
    }
    JNI_TRACE_I("%s: exit", __func__);
}

/*******************************************************************************
**
** Function:        getExtDeviceCapability
**
** Description:     retrive the UWB device capability information.
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         jbyteArray or NULL.
**
*******************************************************************************/
jbyteArray NxpUwbManager::getExtDeviceCapability(JNIEnv* env, jobject o) {
    bool result = false;
    jbyteArray DeviceCapabilityArray = NULL;
    uint8_t configLen = 0;
    JNI_TRACE_I("%s: Enter", __func__);
    if (!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not enabled", __func__);
        return DeviceCapabilityArray;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return DeviceCapabilityArray;
    }

    uint8_t deviceCapabilityInfo[UCI_MAX_PAYLOAD_SIZE];
    result = getExtDeviceCapability(&deviceCapabilityInfo[0], &configLen);
    if(result) {
        DeviceCapabilityArray = env->NewByteArray(configLen); // Extract Length
        env->SetByteArrayRegion (DeviceCapabilityArray, 0, configLen, (jbyte *)&deviceCapabilityInfo[0]);
    }

    JNI_TRACE_I("%s: Exit", __func__);
    return DeviceCapabilityArray;
}

/*********************************************************************************
**
** Function:        setExtDeviceConfigurations()
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
jbyteArray NxpUwbManager::setExtDeviceConfigurations(JNIEnv* env, jobject o, int noOfParams, int deviceConfigLen, jbyteArray extDeviceConfigArray) {
    jbyteArray DeviceConfigArray = NULL;
    uint8_t configLen = 0;
    bool status = false;
    uint8_t* extDeviceConfigData = NULL;
    JNI_TRACE_I("%s: enter; ", __func__);

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return DeviceConfigArray;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return DeviceConfigArray;
    }

    extDeviceConfigData = (uint8_t*) malloc(sizeof(uint8_t) * deviceConfigLen);
    if(extDeviceConfigData != NULL) {
        memset(extDeviceConfigData, 0, (sizeof(uint8_t) * deviceConfigLen));
        env->GetByteArrayRegion(extDeviceConfigArray, 0, deviceConfigLen, (jbyte*)extDeviceConfigData);
        uint8_t extDeviceConfig[UCI_MAX_PAYLOAD_SIZE];
        status = setExtDeviceConfigurations(noOfParams, deviceConfigLen, extDeviceConfigData, &extDeviceConfig[0], &configLen);
        DeviceConfigArray = env->NewByteArray(configLen); // Extract Length
        env->SetByteArrayRegion (DeviceConfigArray, 0, configLen, (jbyte *)&extDeviceConfig[0]); // Copy Data from 1st Index
        free(extDeviceConfigData);
    } else {
        JNI_TRACE_E("%s: Unable to Allocate Memory", __func__);
    }
    JNI_TRACE_I("%s: exit status= 0x%x", __func__, status);
    return DeviceConfigArray;
}

/*******************************************************************************
**
** Function:        getExtDeviceConfigurations
**
** Description:     retrive the UWB Ext Device Config information etc.
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         ExtDeviceConfigInfo class object or NULL.
**
*******************************************************************************/
jbyteArray NxpUwbManager::getExtDeviceConfigurations(JNIEnv* env, jobject o, jint noOfParams,
       jint deviceConfigLen, jbyteArray DeviceConfig) {
    bool result = false;
    jbyteArray DeviceConfigArray = NULL;
    uint8_t configLen = 0;
    uint8_t* DeviceConfigData = NULL;
    JNI_TRACE_I("%s: Enter", __func__);
    if (!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not enabled", __func__);
        return DeviceConfigArray;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return DeviceConfigArray;
    }

    DeviceConfigData = (uint8_t*) malloc(sizeof(uint8_t) * deviceConfigLen);
    if(DeviceConfigData != NULL) {
        memset(DeviceConfigData, 0, (sizeof(uint8_t) * deviceConfigLen));
        env->GetByteArrayRegion(DeviceConfig, 0, deviceConfigLen, (jbyte*)DeviceConfigData);
        uint8_t extDeviceConfig[UCI_MAX_PAYLOAD_SIZE];
        result = getExtDeviceConfigurations(noOfParams, deviceConfigLen, DeviceConfigData, &extDeviceConfig[0], &configLen);
        DeviceConfigArray = env->NewByteArray(configLen); // Extract Length
        env->SetByteArrayRegion (DeviceConfigArray, 0, configLen, (jbyte *)&extDeviceConfig[0]); // Copy Data from 1st Index
        free(DeviceConfigData);
    } else {
        JNI_TRACE_E("%s: Unable to Allocate Memory", __func__);
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return DeviceConfigArray;
}

/*******************************************************************************
**
** Function:        setDebugConfigurations()
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
jbyteArray NxpUwbManager::setDebugConfigurations(JNIEnv* env, jobject o, int sessionId, int noOfParams,
       int debugConfigLen, jbyteArray debugConfigArray, jboolean enableCirDump, jboolean enableFwDump) {
    tUWA_STATUS status = UWA_STATUS_FAILED;
    jbyteArray DebugConfigArray = NULL;
    uint8_t* debugConfigData = NULL;
    uint8_t configLen = 0;
    JNI_TRACE_I("%s: Enter; ", __func__);
    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return DebugConfigArray;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return DebugConfigArray;
    }

    debugConfigData = (uint8_t*) malloc(sizeof(uint8_t) * debugConfigLen);
    if(debugConfigData != NULL) {
        memset(debugConfigData, 0, (sizeof(uint8_t) * debugConfigLen));
        env->GetByteArrayRegion(debugConfigArray, 0, debugConfigLen, (jbyte*)debugConfigData);
        uint8_t debugConfig[UCI_MAX_PAYLOAD_SIZE];
        status = setDebugConfigurations(sessionId, noOfParams, debugConfigLen, debugConfigData, &debugConfig[0], &configLen);
        DebugConfigArray = env->NewByteArray(configLen); // Extract Length
        env->SetByteArrayRegion (DebugConfigArray, 0, configLen, (jbyte *)&debugConfig[0]); // Copy Data from 1st Index
        if(status == UWA_STATUS_OK) {
            UwbAdaptation& theInstance = UwbAdaptation::GetInstance();
            theInstance.DumpFwDebugLog(enableFwDump,enableCirDump);
        }
        free(debugConfigData);
    } else {
        JNI_TRACE_E("%s: Unable to Allocate Memory", __func__);
    }
    JNI_TRACE_I("%s: Exit status= 0x%x", __func__, status);
    return DebugConfigArray;
}

/*******************************************************************************
**
** Function:        bind()
**
** Description:     API to performing binding
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         UWb_STATUS_OK on success else UWA_STATUS_FAILED
**
*******************************************************************************/
jbyte NxpUwbManager::bind(JNIEnv* env, jobject o) {
    tUWA_STATUS status = UWA_STATUS_FAILED;
    JNI_TRACE_I("%s: enter; ", __func__);

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return status;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return status;
    }

    status = performBind();
    JNI_TRACE_I("%s: exit status= 0x%x", __func__, status);
    return (status) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        getBindingStatus()
**
** Description:     API to get Binding Status
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         UWb_STATUS_OK on success else UWA_STATUS_FAILED
**
*******************************************************************************/
jbyte NxpUwbManager::getBindingStatus(JNIEnv* env, jobject o) {
    tUWA_STATUS status = UWA_STATUS_FAILED;
    JNI_TRACE_I("%s: enter; ", __func__);

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return status;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return status;
    }

    status = getBindingStatus();
    JNI_TRACE_I("%s: exit status= 0x%x", __func__, status);
    return (status) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        startLoopBackTest()
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
jbyte NxpUwbManager::startLoopBackTest(JNIEnv* env, jobject o, int loopCount, int interval) {
    tUWA_STATUS status = UWA_STATUS_FAILED;
    JNI_TRACE_I("%s: Enter; ", __func__);
    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return status;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return status;
    }

    status = startLoopBackTest(loopCount, interval);
    JNI_TRACE_I("%s: Exit status=%d; ", __func__, status);

    return (status == UWA_STATUS_OK) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        startConnectivityTest()
**
** Description:     API to check SE connectivity
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte NxpUwbManager::startConnectivityTest(JNIEnv* env, jobject o) {
    tUWA_STATUS status = UWA_STATUS_FAILED;
    JNI_TRACE_I("%s: Enter; ", __func__);

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return status;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return status;
    }

    status = startConnectivityTest();
    JNI_TRACE_I("%s: Exit status=%d; ", __func__, status);

    return (status == UWA_STATUS_OK) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        setCalibration()
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
jbyte NxpUwbManager::setCalibration(JNIEnv* env, jobject o, jbyte channelNo, jbyte paramType,
       jbyteArray calibrationValue) {
    tUWA_STATUS status = UWA_STATUS_FAILED;
    uint8_t* calibValue = NULL;
    JNI_TRACE_I("%s: enter; ", __func__);

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return status;
    }

    if(gIsCalibrationOngoing){
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return status;
    }

    uint8_t len = env->GetArrayLength(calibrationValue);

    if (len > UCI_MAX_PAYLOAD_SIZE) {
        JNI_TRACE_E("%s: len %d is beyond max allowed range %d", __func__, len, UCI_MAX_PAYLOAD_SIZE);
        return status;
    }

    if(len > 0) {
        calibValue = (uint8_t*) malloc(sizeof(uint8_t) * len);
        if (calibValue == NULL) {
            JNI_TRACE_E("%s: malloc failure for calibValue", __func__);
            return status;
        }
        memset(calibValue, 0, (sizeof(uint8_t) * len));
        env->GetByteArrayRegion(calibrationValue, 0, len, (jbyte*)calibValue);

        status = setCalibration((uint8_t)channelNo, (uint8_t)paramType, calibValue, len);
        free(calibValue);
    } else {
        JNI_TRACE_E("%s: calibrationValue length is not valid", __func__);
    }

    JNI_TRACE_I("%s: exit status= 0x%x", __func__, status);
    return (status == UWA_STATUS_OK) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        configureAuthTagOption()
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
jbyte NxpUwbManager::configureAuthTagOption(JNIEnv* env, jobject o, jbyte deviceTag, jbyte modelTag,
       jbyteArray label) {
    tUWA_STATUS status = UWA_STATUS_FAILED;
    uint8_t* labelValue = NULL;
    JNI_TRACE_I("%s: enter; ", __func__);

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return status;
    }

    if(gIsCalibrationOngoing){
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return status;
    }

    uint8_t len = env->GetArrayLength(label);

    if (len != UCI_AUTH_TAG_LABEL_LENGTH) {
        JNI_TRACE_E("%s: len %d is beyond allowed value %d", __func__, len, UCI_AUTH_TAG_LABEL_LENGTH);
        return status;
    } else {
        labelValue = (uint8_t*) malloc(sizeof(uint8_t) * len);
        if (labelValue == NULL) {
            JNI_TRACE_E("%s: malloc failure for labelValue", __func__);
            return status;
        }
        memset(labelValue, 0, (sizeof(uint8_t) * len));
        env->GetByteArrayRegion(label, 0, len, (jbyte*)labelValue);

        status = configureAuthTagOption((uint8_t)deviceTag, (uint8_t)modelTag, labelValue, len);
        free(labelValue);
    }

    JNI_TRACE_I("%s: exit status= 0x%x", __func__, status);
    return (status == UWA_STATUS_OK) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        getCalibration()
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
jbyteArray NxpUwbManager::getCalibration(JNIEnv* env, jobject o, jbyte channelNo, jbyte paramType) {
    tUWA_STATUS status;
    jbyteArray calibrationArray = NULL;
    uint8_t calibration[UCI_MAX_PAYLOAD_SIZE];
    uint8_t calibLen = 0;
    JNI_TRACE_I("%s: enter; ", __func__);

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return calibrationArray;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return calibrationArray;
    }

    status = getCalibration((uint8_t)channelNo, (uint8_t)paramType, &calibration[0], &calibLen);
    if(status == UWA_STATUS_OK) {
        calibrationArray = env->NewByteArray(calibLen);
        env->SetByteArrayRegion (calibrationArray, 0, calibLen, (jbyte *)&calibration[0]);
    }
    return calibrationArray;
}

/*******************************************************************************
**
** Function:        performCalibration()
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
jbyteArray NxpUwbManager::performCalibration(JNIEnv* env, jobject o, jbyte channelNo, jbyte paramType) {
    tUWA_STATUS status;
    jbyteArray calibrationArray = NULL;
    uint8_t calibration[UCI_MAX_PAYLOAD_SIZE];
    uint8_t calibLen = 0;
    JNI_TRACE_I("%s: enter; ", __func__);

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return calibrationArray;
    }

    gIsCalibrationOngoing = true;
    status = performCalibration((uint8_t)channelNo, (uint8_t)paramType, &calibration[0], &calibLen);
    gIsCalibrationOngoing = false;
    if(status == UWA_STATUS_OK) {
        calibrationArray = env->NewByteArray(calibLen);
        env->SetByteArrayRegion (calibrationArray, 0, calibLen, (jbyte *)&calibration[0]);
    }
    return calibrationArray;
}

/*******************************************************************************
**
** Function:        getUwbDeviceTemperature()
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
jbyteArray NxpUwbManager::getUwbDeviceTemperature(JNIEnv* env, jobject o) {
    tUWA_STATUS status;
    jbyteArray temperatureDataArray = NULL;
    int8_t temperatureData;

    JNI_TRACE_I("%s: enter; ", __func__);

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return temperatureDataArray;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return temperatureDataArray;
    }

    status = getUwbDeviceTemperature(&temperatureData);

    if(status == UWA_STATUS_OK) {
        temperatureDataArray = env->NewByteArray(UWB_DEVICE_TEMPERATURE_PAYLOAD_LEN);
        env->SetByteArrayRegion (temperatureDataArray, 0, sizeof(status), (jbyte *)&status);
        env->SetByteArrayRegion (temperatureDataArray, 1, sizeof(temperatureData), (jbyte *)&temperatureData);
    }

    return temperatureDataArray;
}

/*******************************************************************************
**
** Function:        setWtxCount()
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
jbyte NxpUwbManager::setWtxCount(JNIEnv* env, jobject o,jbyte count) {
    tUWA_STATUS status = UWA_STATUS_FAILED;
    JNI_TRACE_I("%s: enter; ", __func__);

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return status;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return status;
    }

    status = setWtxCount(count);
    JNI_TRACE_I("%s: exit status= 0x%x", __func__, status);
    return (status == UWA_STATUS_OK) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        uartConnectivityTest()
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
jbyte NxpUwbManager::uartConnectivityTest(JNIEnv* env, jobject o, jbyteArray payload) {
    tUWA_STATUS status = UWA_STATUS_FAILED;
    JNI_TRACE_I("%s: enter; ", __func__);

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return status;
    }

    if(gIsCalibrationOngoing){
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return status;
    }

    uint8_t len = env->GetArrayLength(payload);

    if (len > UWB_MAX_UART_PAYLOAD_LENGTH) {
        JNI_TRACE_E("%s: len %d is beyond max allowed range %d", __func__, len, UCI_MAX_PAYLOAD_SIZE);
        return status;
    }

    if(len > 0) {
        uint8_t* pBuff = (uint8_t*) malloc(sizeof(uint8_t) * len);
        if (pBuff == NULL) {
            JNI_TRACE_E("%s: malloc failure for pBuff", __func__);
            return status;
        }
        memset(pBuff, 0, (sizeof(uint8_t) * len));
        env->GetByteArrayRegion(payload, 0, len, (jbyte*)pBuff);

        status = uartConnectivityTest(len, pBuff);
        free(pBuff);
    } else {
        JNI_TRACE_E("%s: payload length is not valid", __func__);
    }

    JNI_TRACE_I("%s: exit status= 0x%x", __func__, status);
    return (status == UWA_STATUS_OK) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        generateSecureTag()
**
** Description:     API to generate the cmac tag
**
** Params:          e: JVM environment.
**                  o: Java object.
**                  keyData: Required data to send to UWBS to generate cmac tag
**                  tagOption: Config Option to select Device Specific or Model Specific
**
** Returns:         cmac tag in byte array on success or UFA_STATUS_FAILED
**                  on failure
**
*******************************************************************************/
jbyteArray NxpUwbManager::generateSecureTag(JNIEnv* e, jobject o,jbyte tagOption) {
  tUWA_STATUS status = UWA_STATUS_FAILED;
  uint8_t rspData[UCI_MAX_PAYLOAD_SIZE];
  uint8_t rspLen = 0;
  jbyteArray cmacTagData = NULL;
  jbyteArray cmacTagStatus= e->NewByteArray(1);
  e->SetByteArrayRegion (cmacTagStatus, 0, 1, (jbyte *)&status);
  JNI_TRACE_I("%s: enter; ", __func__);

  if(!gIsUwaEnabled) {
      JNI_TRACE_E("%s: UWB device is not initialized", __func__);
      return cmacTagStatus;
  }

  if(gIsCalibrationOngoing) {
      JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
      return cmacTagStatus;
  }

  status = generateSecureTag((uint8_t)tagOption, &rspData[0], &rspLen);
  if(status == UWA_STATUS_OK) {
      cmacTagData = e->NewByteArray(UWB_CMAC_TAG_LENGTH + 1);
      e->SetByteArrayRegion (cmacTagData, 0, rspLen, (jbyte *)&rspData[0]);
  } else {
      JNI_TRACE_E("%s: generate tag ntf status failed", __func__);
      cmacTagData = cmacTagStatus;
  }

  JNI_TRACE_I("%s: exit", __func__);
  return cmacTagData;
}

/*******************************************************************************
**
** Function:        verifySecureTag()
**
** Description:     API to verify cmac tag
**
** Params:          e: JVM environment.
**                  o: Java object.
**                  cmacTag: cmac tag required to be verify
**                  tagOption: Config Option to select Device Specific or Model Specific
**
** Returns:         UFA_STATUS_OK on success or UFA_STATUS_FAILED on failure
**
*******************************************************************************/
jbyte NxpUwbManager::verifySecureTag(JNIEnv* e, jobject o, jbyteArray cmacTag, jbyte tagOption, jshort tagVersion) {
  tUWA_STATUS status = UWA_STATUS_FAILED;
  JNI_TRACE_I("%s: enter; ", __func__);

  if(!gIsUwaEnabled) {
      JNI_TRACE_E("%s: UWB device is not initialized", __func__);
      return status;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
    return status;
  }

  uint8_t cmacTagLen = e->GetArrayLength(cmacTag);

  if (cmacTagLen > UWB_CMAC_TAG_LENGTH) {
      JNI_TRACE_E("%s: len is beyond max allowed range cmacTagLen:%d", __func__, cmacTagLen);
      return status;
  }

  if(cmacTagLen > 0) {
    uint8_t* pCmacBuff = (uint8_t*) malloc(sizeof(uint8_t) * cmacTagLen);
    if (pCmacBuff == NULL) {
        JNI_TRACE_E("%s: malloc failure for pCmacBuff", __func__);
        return status;
    }
    memset(pCmacBuff, 0, (sizeof(uint8_t) * cmacTagLen));
    e->GetByteArrayRegion(cmacTag, 0, cmacTagLen, (jbyte*)pCmacBuff);

    status = verifySecureTag((uint8_t)tagOption, pCmacBuff, cmacTagLen, tagVersion);
    free(pCmacBuff);
  } else {
    JNI_TRACE_E("%s: payload length is not valid", __func__);
  }

  JNI_TRACE_I("%s: exit", __func__);
  return (status == UWA_STATUS_OK) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        configureTagVersion()
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
jbyte NxpUwbManager::configureTagVersion(JNIEnv* e, jobject o, jshort version) {
  tUWA_STATUS status = UWA_STATUS_FAILED;
  JNI_TRACE_I("%s: enter; ", __func__);

  if(!gIsUwaEnabled) {
      JNI_TRACE_E("%s: UWB device is not initialized", __func__);
      return status;
  }

  if(gIsCalibrationOngoing) {
    JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
    return status;
  }

  status = configureTagVersion(version);

  JNI_TRACE_I("%s: exit", __func__);
  return (status == UWA_STATUS_OK) ? UWA_STATUS_OK : UWA_STATUS_FAILED;
}

/*******************************************************************************
** Function:        sendRawUci()
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
jbyteArray NxpUwbManager::sendRawUci(JNIEnv* env, jobject o, jbyteArray rawUci, jint cmdLen) {
    jbyteArray rspArray = NULL;
    uint8_t* cmd = NULL;
    tUWA_STATUS status;
    uint8_t rspData[UCI_MAX_PAYLOAD_SIZE];
    uint8_t rspLen = 0;

    JNI_TRACE_I("%s: enter; ", __func__);
    if (cmdLen > UCI_PSDU_SIZE_4K) {
        JNI_TRACE_E("%s: CmdLen %d is beyond max allowed range %d", __func__, cmdLen, UCI_PSDU_SIZE_4K);
        return rspArray;
    }

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return rspArray;
    }

    if(gIsCalibrationOngoing){
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return rspArray;
    }

    cmd = (uint8_t*) malloc(sizeof(uint8_t) * cmdLen);
    if (cmd == NULL) {
        JNI_TRACE_E("%s: malloc failure for raw cmd", __func__);
        return rspArray;
    }
    memset(cmd, 0, (sizeof(uint8_t) * cmdLen));
    env->GetByteArrayRegion(rawUci,0,cmdLen,(jbyte*)cmd);

    status = sendRawUci(cmd, cmdLen, &rspData[0], &rspLen);
    if(rspLen > 0){
        rspArray = env->NewByteArray(rspLen);
        env->SetByteArrayRegion (rspArray, 0, rspLen, (jbyte *)&rspData[0]);
    }
    free(cmd);
    JNI_TRACE_I("%s: exit sendRawUCi= 0x%x", __func__, status);
    return rspArray;
}
/*******************************************************************************
**
** Function:        setCalibratationIntegrityProtection()
**
** Description:     Set Calibratation Integrity Protection
**
** Returns:         Status
**
*******************************************************************************/
jbyte NxpUwbManager::setCalibrationIntegrityProtection(JNIEnv* env, jobject o, jbyte tagOption, jint calibParamBitmask) {
  tUWA_STATUS status = UWA_STATUS_FAILED;
  JNI_TRACE_I("%s: enter; ", __func__);

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", __func__);
    return status;
  }

  if(gIsCalibrationOngoing){
    JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
    return status;
  }

  status = sendSetCalibrationIntegrityProtection(tagOption, calibParamBitmask);

  JNI_TRACE_I("%s: exit status= 0x%x", __func__, status);
  return (status == UWA_STATUS_OK) ? UWA_STATUS_OK : UWA_STATUS_FAILED;

}

/*******************************************************************************
**
** Function:        getUwbDeviceTimeStamp()
**
** Description:     Get the Uwb Device TimeStamp Info
**
** Returns:         Status
**
*******************************************************************************/
jbyteArray NxpUwbManager::getUwbDeviceTimeStamp(JNIEnv* env, jobject o) {
  tUWA_STATUS status;
  jbyteArray rspArray = NULL;
  uint8_t rspData[UCI_MAX_PAYLOAD_SIZE];
  uint8_t rspLen = 0;
  JNI_TRACE_I("%s: enter; ", __func__);

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", __func__);
    return rspArray;
  }

  status = sendGetUwbDeviceTimeStamp(&rspData[0], &rspLen);
  if (status == UWA_STATUS_OK) {
      rspArray = env->NewByteArray(rspLen);
      env->SetByteArrayRegion (rspArray, 0, rspLen, (jbyte *)&rspData[0]);
  } else {
    JNI_TRACE_E("%s: Failed getUwbDeviceTimeStamp", __func__);
  }

  JNI_TRACE_I("%s: exit status= 0x%x", __func__, status);
  return rspArray;

}

/*******************************************************************************
**
** Function:        getRanMultiplier()
**
** Description:     API to get Ran Multiplier
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         get Ran Multiplier Value on success and 0 on failure.
**
*******************************************************************************/
jbyte NxpUwbManager::getRanMultiplier(JNIEnv* env, jobject o) {
    tUWA_STATUS status;
    uint8_t ranMultiplierVal = 0;
    JNI_TRACE_I("%s: enter; ", __func__);

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return ranMultiplierVal;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return ranMultiplierVal;
    }

    status = getRanMultiplier(&ranMultiplierVal);
    JNI_TRACE_I("%s: exit status= 0x%x", __func__, status);
    return ranMultiplierVal;
}

/*******************************************************************************
**
** Function:        eraseURSK()
**
** Description:     Request to Delete the RDS/URSK from SecueElement
**
** Returns:         Status
**
*******************************************************************************/
jbyte NxpUwbManager::eraseURSK(JNIEnv* e, jobject o, jbyte numberOfSessionIds, jintArray sessionIdsList) {
  tUWA_STATUS status = UWA_STATUS_FAILED;
  JNI_TRACE_I("%s: enter; ", __func__);

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", __func__);
    return status;
  }

  if(sessionIdsList == NULL) {
    JNI_TRACE_E("%s: sessionIdsList value is NULL", __func__);
    return status;
  }
  uint8_t sessionIdsListLen = e->GetArrayLength(sessionIdsList);

  if(sessionIdsListLen > 0) {
    uint32_t* pSessionIdsList = (uint32_t*) malloc(SESSION_ID_LEN * sessionIdsListLen);
    if (pSessionIdsList == NULL) {
        JNI_TRACE_E("%s: malloc failure for pSessionIdsList", __func__);
        return status;
    }
    memset(pSessionIdsList, 0, (SESSION_ID_LEN * sessionIdsListLen));
    e->GetIntArrayRegion(sessionIdsList, 0, sessionIdsListLen, (jint*)pSessionIdsList);

    status = sendEraseUrskCommand(numberOfSessionIds, pSessionIdsList);
    if (status == UWA_STATUS_OK) {
      JNI_TRACE_E("%s: sendEraseUrskCommand Success", __func__);
    } else {
      JNI_TRACE_E("%s: Failed sendEraseUrskCommand", __func__);
    }
    free(pSessionIdsList);
  } else {
    JNI_TRACE_E("%s: payload length is not valid", __func__);
  }

  JNI_TRACE_I("%s: exit status= 0x%x", __func__, status);
  return (status == UWA_STATUS_OK) ? UWA_STATUS_OK : UWA_STATUS_FAILED;

}

/*******************************************************************************
**
** Function:        enableConformanceTest
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
jbyte NxpUwbManager::enableConformanceTest(JNIEnv* e, jobject o, jboolean enable) {
  tUWA_STATUS status = UWA_STATUS_FAILED;
  JNI_TRACE_I("%s: Enter: enable=%d", __func__, enable);

  if(!gIsUwaEnabled) {
    JNI_TRACE_E("%s: UWB device is not initialized", __func__);    return status;
  }

  /* Enable/Disable the NxpExtendedConfig */
  uint8_t vendorExtnEnableDisable = 0x00;
  if(!enable) vendorExtnEnableDisable = 0x01;

  uint8_t configParam[] = {0x00, 0x00, 0x00, 0x00};
  uint8_t *configPtr = &configParam[0];

  JNI_TRACE_I("%s: NXP_EXTENDED_NTF value %d ", __func__,(uint8_t)vendorExtnEnableDisable);

  UINT8_TO_STREAM(configPtr, EXTENDED_DEVICE_CONFIG_ID);
  UINT8_TO_STREAM(configPtr, UCI_EXT_PARAM_ID_NXP_EXTENDED_NTF_CONFIG);
  UINT8_TO_STREAM(configPtr, UCI_EXT_PARAM_ID_NXP_EXTENDED_NTF_CONFIG_LEN);
  UINT8_TO_STREAM(configPtr, vendorExtnEnableDisable);

  uint8_t deviceConfig[UCI_MAX_PAYLOAD_SIZE];
  uint8_t configLen;
  bool ret = setExtDeviceConfigurations(1, sizeof(configParam), &configParam[0], &deviceConfig[0], &configLen);
  status = ret ? UWA_STATUS_OK : UWA_STATUS_FAILED;
  if(status == UWA_STATUS_OK) {
    JNI_TRACE_I("%s: setExtDeviceConfigurations Success", __func__);
     UWB_EnableConformanceTest((bool)(enable == JNI_TRUE));
     gIsConformaceTestEnabled = enable;
     android::setBindingStatusNtfRecievedFlag();
  } else {
      JNI_TRACE_E("%s: setExtDeviceConfigurations failed", __func__);
  }

  JNI_TRACE_I("%s: Exit", __func__);
  return status;
}

/*******************************************************************************
**
** Function:        extDeviceManagementCallback
**
** Description:     Receive device management events from stack for Properiatary Extensions
**                  dmEvent: Device-management event ID.
**                  eventData: Data associated with event ID.
**
** Returns:         None
**
*******************************************************************************/
void extDeviceManagementCallback(uint8_t event, uint16_t payloadLength, uint8_t* pResponseBuffer) {
    JNI_TRACE_I("%s: Entry", __func__);
    uint8_t *payloadPtr = NULL;
    uint16_t packetLength = payloadLength + UCI_MSG_HDR_SIZE;

    if (pResponseBuffer != NULL) {
        JNI_TRACE_I("NxpResponse_Cb Received length data = 0x%x status = 0x%x", \
                            payloadLength,
                            pResponseBuffer[UCI_RESPONSE_STATUS_OFFSET]);
        payloadPtr = &pResponseBuffer[UCI_RESPONSE_STATUS_OFFSET];

        if (gIsConformaceTestEnabled == true) {
          nxpUwbManager.onRawUciNotificationReceived(pResponseBuffer, packetLength);
          return;
        }
        switch (event) {
            case EXT_UCI_MSG_SE_COMM_ERROR_NTF: /* SE_COMM_ERROR_NTF */
            {
                #if 1//WA waiting binding status NTF/ SE comm error NTF
                if(!gIsUwaEnabled)
                {
                    SyncEventGuard guard(gSeCommErrNtfEvt);
                    gSeCommErrNtfEvt.notifyOne();
                }
                #endif
            } break;
            case EXT_UCI_MSG_BINDING_STATUS_NTF: /* BINDING_STATUS_NTF */
            {
                #if 1//WA waiting binding status NTF/ SE comm error NTF
                gBindStatus = payloadPtr[0];
                if(!gIsUwaEnabled)
                {
                    SyncEventGuard guard(gBindStatusNtfEvt);
                    gBindStatusNtfEvt.notifyOne();
                }
                #endif
                android::setBindingStatusNtfRecievedFlag();
                if(!IsUwbMwDisabled()) return;
            } break;
            case EXT_UCI_MSG_DO_CALIBRATION: /* DO_CALIBRATION */
            {
                sNxpFeatureConf.ntf_len = payloadLength;
                memcpy(&sNxpFeatureConf.ntf_data[0], payloadPtr, payloadLength);
                isCalibNtfReceived = true;
                SyncEventGuard guard(gCalibNtfEvt);
                gCalibNtfEvt.notifyOne();
            } break;
            case EXT_UCI_MSG_DBG_RFRAME_LOG_NTF: /* rframe log data notification */
            case EXT_UCI_MSG_SE_DO_BIND: /* SE_DO_BIND_EVT */
            case EXT_UCI_MSG_SE_GET_BINDING_STATUS: /* SE_GET_BINDING_STATUS */
            case EXT_UCI_MSG_SE_DO_TEST_LOOP: /* SE_DO_TEST_LOOP */
            case EXT_UCI_MSG_SE_DO_TEST_CONNECTIVITY:
            case EXT_UCI_MSG_SCHEDULER_STATUS_NTF:
            case EXT_UCI_MSG_UART_CONNECTIVITY_TEST_CMD:
            case EXT_UCI_MSG_URSK_DELETION_REQ_CMD:
            {
            } break;
            case EXT_UCI_MSG_GENERATE_TAG_CMD: /* GENERATE_TAG_NTF */
            {
                handle_generate_tag_ntf(payloadPtr, payloadLength);
            } break;
            case EXT_UCI_MSG_VERIFY_TAG_CMD: /* VERIFY_TAG_NTF */
            {
                handle_verify_tag_ntf(payloadPtr, payloadLength);
            } break;
            case EXT_UCI_MSG_CONFIGURE_AUTH_TAG_OPTIONS_CMD: /* CONFIGURE AUTH TAG OPTION NTF */
            {
                handle_configure_auth_tag_option_ntf(payloadPtr, payloadLength);
            } break;
            default:
                JNI_TRACE_E("%s: unhandled event", __func__);
                break;
        }
        /* Send Notification received to upper Layer */
        nxpUwbManager.onExtendedNotificationReceived(event, payloadPtr, payloadLength);
    } else {
        JNI_TRACE_E("%s: pResponseBuffer is NULL\n", __func__);
    }
    JNI_TRACE_I("%s: Exit", __func__);
}
/*******************************************************************************
**
** Function:        SetCbStatus
**
** Description:     Set ExtApp Response Status
**                  tUWA_STATUS:  status to be set
**
** Returns:         None
**
*******************************************************************************/
static void SetCbStatus(tUWA_STATUS status) {
    sNxpFeatureConf.wstatus = status;
}

/*******************************************************************************
**
** Function:        GetCbStatus
**
** Description:     Get ExtApp Response Status
**
** Returns:         None
**
*******************************************************************************/
static tUWA_STATUS GetCbStatus() {
    return sNxpFeatureConf.wstatus;
}

/*******************************************************************************
**
** Function:        CommandResponse_Cb
**
** Description:     Receive response from the stack for raw command sent from
*jni.
**                  event:  event ID.
**                  paramLength: length of the response
**                  pResponseBuffer: pointer to data
**
** Returns:         None
**
*******************************************************************************/
static void CommandResponse_Cb(uint8_t event, uint16_t paramLength, uint8_t* pResponseBuffer) {
    JNI_TRACE_I("%s: Entry", __func__);

    if ((paramLength > UCI_RESPONSE_STATUS_OFFSET) && (pResponseBuffer != NULL)) {
        JNI_TRACE_I("NxpResponse_Cb Received length data = 0x%x status = 0x%x", \
                            paramLength,
                            pResponseBuffer[UCI_RESPONSE_STATUS_OFFSET]);

        sNxpFeatureConf.rsp_len = (uint8_t)paramLength;
        memcpy(sNxpFeatureConf.rsp_data, pResponseBuffer, paramLength);
        if (pResponseBuffer[UCI_RESPONSE_STATUS_OFFSET] == 0x00) {
            SetCbStatus(UWA_STATUS_OK);
        } else {
            SetCbStatus(UWA_STATUS_FAILED);
        }
    } else if (event == (uint8_t)UWB_SEGMENT_SENT_EVT) {
        JNI_TRACE_E("%s:NxpResponse_Cb responseBuffer status is UWB_SEGMENT_SENT_EVT", __func__);
        SetCbStatus(UWA_STATUS_UCI_SEGMENT_SENT);
    } else {
        JNI_TRACE_E("%s:NxpResponse_Cb responseBuffer is NULL or Length < UCI_RESPONSE_STATUS_OFFSET", __func__);
        SetCbStatus(UWA_STATUS_FAILED);
    }
    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    sNxpFeatureConf.NxpConfigEvt.notifyOne();

    JNI_TRACE_I("%s: Exit", __func__);
}

/*******************************************************************************
**
** Function:        getExtDeviceCapability
**
** Description:     Get device capability info
**                  etc.
**                  uint8_t*: Output config tlv
**
** Returns:         None
**
*******************************************************************************/
bool NxpUwbManager::getExtDeviceCapability(uint8_t *pOutExtDeviceCapability, uint8_t *configLen) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    uint8_t* pExtDeviceCapabilityCommand = NULL;
    uint8_t ExtDeviceCapabilityBuffer[UCI_MAX_PAYLOAD_SIZE];

    if ((configLen == NULL) || (pOutExtDeviceCapability == NULL)) {
        JNI_TRACE_E("%s: pOutExtDeviceCapability is NULL", __func__);
        return false;
    }

    pExtDeviceCapabilityCommand = ExtDeviceCapabilityBuffer;
    UCI_MSG_BLD_HDR0(pExtDeviceCapabilityCommand, UCI_MT_CMD, UCI_GID_CORE);
    UCI_MSG_BLD_HDR1(pExtDeviceCapabilityCommand, UCI_MSG_CORE_GET_CAPS_INFO);
    UINT8_TO_STREAM(pExtDeviceCapabilityCommand, 0x00);
    UINT8_TO_STREAM(pExtDeviceCapabilityCommand, 0x00);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand(UCI_MSG_HDR_SIZE, ExtDeviceCapabilityBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return false;
    }

    status = GetCbStatus();
    uint8_t *deviceCapabilityRsp = &sNxpFeatureConf.rsp_data[UCI_RESPONSE_PAYLOAD_OFFSET];
    *configLen = sNxpFeatureConf.rsp_len - UCI_RESPONSE_PAYLOAD_OFFSET;
    memcpy(pOutExtDeviceCapability, deviceCapabilityRsp, *configLen);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Get Ext Device capability Success", __func__);
    } else {
        JNI_TRACE_E("%s: Get Ext Device capability Failure", __func__);
        return false;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return true;
}

/*******************************************************************************
**
** Function:        setExtDeviceConfigurations()
**
** Description:     Send Command to set Device Configuration parameters
**                  uint8_t: No Of Parameters
**                  uint8_t: Length of ExtDevice Config TLV
**                  uint8_t*: Device configuration structure
**
** Returns:         None
**
*******************************************************************************/
bool NxpUwbManager::setExtDeviceConfigurations(uint8_t noOfParams, uint8_t ExtDeviceConfigLen, const uint8_t* pInExtDeviceConfig, uint8_t *pOutExtDeviceConfig, uint8_t *configLen) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    uint8_t* pExtDeviceConfigCommand = NULL;
    uint8_t ExtDeviceConfigBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint32_t payloadLen = (uint32_t)(sizeof(noOfParams) + ExtDeviceConfigLen);

    if (pInExtDeviceConfig == NULL) {
        JNI_TRACE_E("%s: pInExtDeviceConfig is NULL", __func__);
        return false;
    }

    if (payloadLen > (UCI_MAX_PAYLOAD_SIZE - UCI_MSG_HDR_SIZE)) {
        JNI_TRACE_E("%s: payloadLen is more than (UCI_MAX_PAYLOAD_SIZE - UCI_MSG_HDR_SIZE)", __func__);
        return false;
    }

    pExtDeviceConfigCommand = ExtDeviceConfigBuffer;
    UCI_MSG_BLD_HDR0(pExtDeviceConfigCommand, UCI_MT_CMD, UCI_GID_CORE);
    UCI_MSG_BLD_HDR1(pExtDeviceConfigCommand, UCI_MSG_CORE_SET_CONFIG);
    UINT8_TO_STREAM(pExtDeviceConfigCommand, 0x00);
    UINT8_TO_STREAM(pExtDeviceConfigCommand, payloadLen);
    UINT8_TO_STREAM(pExtDeviceConfigCommand, noOfParams);
    ARRAY_TO_STREAM(pExtDeviceConfigCommand, pInExtDeviceConfig, ExtDeviceConfigLen);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), ExtDeviceConfigBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return false;
    }
    status = GetCbStatus();
    uint8_t *appConfigRsp = &sNxpFeatureConf.rsp_data[UCI_RESPONSE_STATUS_OFFSET];
    *configLen = sNxpFeatureConf.rsp_len - UCI_RESPONSE_STATUS_OFFSET;
    memcpy(pOutExtDeviceConfig, appConfigRsp, *configLen);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: PHY configs is applied succeessfully", __func__);
    } else {
        JNI_TRACE_E("%s: PHY configs is failed", __func__);
        return false;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return true;
}


/*******************************************************************************
**
** Function:        getExtDeviceConfigurations
**
** Description:     Get theCurrent ExtDevice Config
**                  etc.
**                  uint8_t*: Output config tlv
**
** Returns:         None
**
*******************************************************************************/
bool NxpUwbManager::getExtDeviceConfigurations(uint8_t noOfParams, uint8_t ExtDeviceConfigLen, const uint8_t *pInExtDeviceConfig, uint8_t *pOutExtDeviceConfig, uint8_t *configLen) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    uint8_t* pExtDeviceConfigCommand = NULL;
    uint8_t ExtDeviceConfigBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint32_t payloadLen =  (uint32_t)(sizeof(noOfParams) + ExtDeviceConfigLen);

    if ((pInExtDeviceConfig == NULL) || (configLen == NULL) || (pOutExtDeviceConfig == NULL)) {
        JNI_TRACE_E("%s: pInExtDeviceConfig is NULL", __func__);
        return false;
    }

    if (payloadLen > (UCI_MAX_PAYLOAD_SIZE - UCI_MSG_HDR_SIZE)) {
        JNI_TRACE_E("%s: payloadLen is more than (UCI_MAX_PAYLOAD_SIZE - UCI_MSG_HDR_SIZE)", __func__);
        return false;
    }

    pExtDeviceConfigCommand = ExtDeviceConfigBuffer;
    UCI_MSG_BLD_HDR0(pExtDeviceConfigCommand, UCI_MT_CMD, UCI_GID_CORE);
    UCI_MSG_BLD_HDR1(pExtDeviceConfigCommand, UCI_MSG_CORE_GET_CONFIG);
    UINT8_TO_STREAM(pExtDeviceConfigCommand, 0x00);
    UINT8_TO_STREAM(pExtDeviceConfigCommand, payloadLen);
    UINT8_TO_STREAM(pExtDeviceConfigCommand, noOfParams);
    ARRAY_TO_STREAM(pExtDeviceConfigCommand, pInExtDeviceConfig, ExtDeviceConfigLen);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), ExtDeviceConfigBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return false;
    }

    status = GetCbStatus();
    uint8_t *appConfigRsp = &sNxpFeatureConf.rsp_data[UCI_RESPONSE_STATUS_OFFSET];
    *configLen = sNxpFeatureConf.rsp_len - UCI_RESPONSE_STATUS_OFFSET;
    memcpy(pOutExtDeviceConfig, appConfigRsp, *configLen);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Get Ext Device Config Success", __func__);
    } else {
        JNI_TRACE_E("%s: Get Ext Device Config Failure", __func__);
        return false;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return true;
}

/*******************************************************************************
**
** Function:        setDebugConfigurations()
**
** Description:     Set the default Debug Config
**
** Returns:         None
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::setDebugConfigurations(uint32_t sessionId, uint8_t noOfParams, uint8_t debugConfigLen, const uint8_t* pDebugConfig, uint8_t *pOutDebugConfig, uint8_t *configLen) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status = UWA_STATUS_FAILED;
    uint8_t* pDebugConfigCommand = NULL;
    uint8_t debugConfigBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint32_t payloadLen =  (uint32_t)(sizeof(sessionId) + sizeof(noOfParams) + debugConfigLen);

    if (pDebugConfig == NULL) {
        JNI_TRACE_E("%s: pDebugConfig is NULL", __func__);
        return status;
    }

    if (payloadLen > (UCI_MAX_PAYLOAD_SIZE - UCI_MSG_HDR_SIZE)) {
        JNI_TRACE_E("%s: payloadLen is more than (UCI_MAX_PAYLOAD_SIZE - UCI_MSG_HDR_SIZE)", __func__);
        return status;
    }

    pDebugConfigCommand = debugConfigBuffer;
    UCI_MSG_BLD_HDR0(pDebugConfigCommand, UCI_MT_CMD, UCI_GID_SESSION_MANAGE);
    UCI_MSG_BLD_HDR1(pDebugConfigCommand, UCI_MSG_SESSION_SET_APP_CONFIG);
    UINT8_TO_STREAM(pDebugConfigCommand, 0x00);
    UINT8_TO_STREAM(pDebugConfigCommand, payloadLen);
    UINT32_TO_STREAM(pDebugConfigCommand, sessionId);
    UINT8_TO_STREAM(pDebugConfigCommand, noOfParams);
    ARRAY_TO_STREAM(pDebugConfigCommand, pDebugConfig, debugConfigLen);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), debugConfigBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
    }
    status = GetCbStatus();
    uint8_t *appConfigRsp = &sNxpFeatureConf.rsp_data[UCI_RESPONSE_STATUS_OFFSET];
    *configLen = sNxpFeatureConf.rsp_len - UCI_RESPONSE_STATUS_OFFSET;
    memcpy(pOutDebugConfig, appConfigRsp, *configLen);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Debug Config applied succeessfully", __func__);
    } else {
        JNI_TRACE_E("%s: Debug Config is failed", __func__);
        return status;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
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
jbyteArray NxpUwbManager::getAllUwbSessions(JNIEnv* env, jobject o) {
  JNI_TRACE_I("%s: Entry", __func__);

  tUWA_STATUS status;
  jbyteArray sessionList = NULL;
  uint8_t* pGetAllUwbSessionCommand = NULL;
  uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];
  uint8_t sessionData[UCI_MAX_PAYLOAD_SIZE];
  uint8_t payloadLen =  0;
  uint8_t sessionDataLen =  0;

  if(!gIsUwaEnabled) {
      JNI_TRACE_E("%s: UWB device is not initialized", __func__);
      return sessionList;
  }

  if(gIsCalibrationOngoing) {
      JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
      return sessionList;
  }

  pGetAllUwbSessionCommand = commandBuffer;
  UCI_MSG_BLD_HDR0(pGetAllUwbSessionCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
  UCI_MSG_BLD_HDR1(pGetAllUwbSessionCommand, EXT_UCI_MSG_GET_ALL_UWB_SESSIONS);
  UINT8_TO_STREAM(pGetAllUwbSessionCommand, 0x00);
  UINT8_TO_STREAM(pGetAllUwbSessionCommand, payloadLen);

  SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
  SetCbStatus(UWA_STATUS_FAILED);
  status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
  if (status == UWA_STATUS_OK) {
      JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
      sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
  } else {
      JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
      return sessionList;
  }

  status = GetCbStatus();
  if (status == UWA_STATUS_OK) {
      uint8_t *sessionDataPtr = &sNxpFeatureConf.rsp_data[UCI_RESPONSE_STATUS_OFFSET];
      sessionDataLen = sNxpFeatureConf.rsp_len - UCI_RESPONSE_STATUS_OFFSET;
      memcpy(&sessionData[0], sessionDataPtr, sessionDataLen);
      sessionList = env->NewByteArray(sessionDataLen);
      env->SetByteArrayRegion (sessionList, 0, sessionDataLen, (jbyte *)&sessionData[0]);
      JNI_TRACE_I("%s: Get All Uwb Session Success", __func__);
  } else {
      JNI_TRACE_E("%s: Get All Uwb Session Failure", __func__);
  }
  JNI_TRACE_I("%s: Exit", __func__);
  return sessionList;
}

/*******************************************************************************
**
** Function:        sendRawUci
**
** Description:     send raw uci
**
** Returns:         raw uci response
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::sendRawUci(uint8_t *rawCmd, uint8_t cmdLen, uint8_t *rspData, uint8_t *rspLen ) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand(cmdLen, rawCmd, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
    }
    status = GetCbStatus();
    if (status != UWA_STATUS_UCI_SEGMENT_SENT){
        JNI_TRACE_E("%s: status in sendRawUci is %d", __func__, status);
        uint8_t *rsp = sNxpFeatureConf.rsp_data;
        *rspLen = sNxpFeatureConf.rsp_len;
        memcpy(rspData, rsp, *rspLen);
    } else {
        *rspLen = 0;
        status = UWA_STATUS_FAILED;
    }

    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        performBind()
**
** Description:     Send Binding Command to Helios
**
** Returns:         true if API is success, otherwise false
**
*******************************************************************************/
bool NxpUwbManager::performBind() {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    uint8_t* pBindCommand = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint8_t payloadLen =  0;

    pBindCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pBindCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pBindCommand, EXT_UCI_MSG_SE_DO_BIND);
    UINT8_TO_STREAM(pBindCommand, 0x00);
    UINT8_TO_STREAM(pBindCommand, payloadLen);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return false;
    }
    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Binding is succeessful", __func__);
    } else {
        JNI_TRACE_E("%s: Binding is failed", __func__);
        return false;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return true;
}

/*******************************************************************************
**
** Function:        getBindingStatus
**
** Description:     Get Binding Status
**
** Returns:         Binding Status
**
*******************************************************************************/
bool NxpUwbManager::getBindingStatus() {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    uint8_t* pGetBindingStatusCommand = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint8_t payloadLen =  0;

    pGetBindingStatusCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pGetBindingStatusCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pGetBindingStatusCommand, EXT_UCI_MSG_SE_GET_BINDING_STATUS);
    UINT8_TO_STREAM(pGetBindingStatusCommand, 0x00);
    UINT8_TO_STREAM(pGetBindingStatusCommand, payloadLen);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return false;
    }

    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Get Binding Status Success", __func__);
    } else {
        JNI_TRACE_E("%s: Get Binding Status Failure", __func__);
        return false;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return true;
}

/*******************************************************************************
**
** Function:        getBindingCount
**
** Description:     Get Binding Count
**
** Returns:         Binding Count on success, NULL on failure
**
*******************************************************************************/
jbyteArray NxpUwbManager::getBindingCount(JNIEnv* env, jobject o) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    jbyteArray bindingCountArray = NULL;
    uint8_t* pGetBindingCountCommand = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint8_t bindingCountData[UCI_MAX_PAYLOAD_SIZE];
    uint8_t payloadLen =  0;
    uint8_t bindingCountDataLen =  0;

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return bindingCountArray;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return bindingCountArray;
    }

    pGetBindingCountCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pGetBindingCountCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pGetBindingCountCommand, EXT_UCI_MSG_SE_GET_BINDING_COUNT);
    UINT8_TO_STREAM(pGetBindingCountCommand, 0x00);
    UINT8_TO_STREAM(pGetBindingCountCommand, payloadLen);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return bindingCountArray;
    }

    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        uint8_t *bindingCountPtr = &sNxpFeatureConf.rsp_data[UCI_RESPONSE_STATUS_OFFSET];
        bindingCountDataLen = sNxpFeatureConf.rsp_len - UCI_RESPONSE_STATUS_OFFSET;
        memcpy(&bindingCountData[0], bindingCountPtr, bindingCountDataLen);
        bindingCountArray = env->NewByteArray(bindingCountDataLen);
        env->SetByteArrayRegion (bindingCountArray, 0, bindingCountDataLen, (jbyte *)&bindingCountData[0]);
        JNI_TRACE_I("%s: Get Binding Count Success", __func__);
    } else {
        JNI_TRACE_E("%s: Get Binding Count Failure", __func__);
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return bindingCountArray;
}

/*******************************************************************************
**
** Function:        startLoopBackTest()
**
** Description:     Start Loopback Test
**
** Returns:         Status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::startLoopBackTest(uint16_t loopCount, uint16_t interval) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    uint8_t* pStartLoopBackTestCommand = NULL;
    uint32_t payloadLen =  sizeof(loopCount) + sizeof(interval);
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];

    pStartLoopBackTestCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pStartLoopBackTestCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pStartLoopBackTestCommand, EXT_UCI_MSG_SE_DO_TEST_LOOP);
    UINT8_TO_STREAM(pStartLoopBackTestCommand, 0x00);
    UINT8_TO_STREAM(pStartLoopBackTestCommand, payloadLen);
    UINT16_TO_STREAM(pStartLoopBackTestCommand, loopCount);
    UINT16_TO_STREAM(pStartLoopBackTestCommand, interval);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
    }
    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Loop Back Test started succeessfully", __func__);
    } else {
        JNI_TRACE_E("%s: Loop Back Test start is failed", __func__);
        return status;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        startConnectivityTest()
**
** Description:     Send se connectivity test Command to Helios
**
** Returns:         Connectivity test data object on success, NULL on failure
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::startConnectivityTest() {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    uint8_t* pStartConnectivity = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint8_t payloadLen =  0;

    pStartConnectivity = commandBuffer;
    UCI_MSG_BLD_HDR0(pStartConnectivity, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pStartConnectivity, EXT_UCI_MSG_SE_DO_TEST_CONNECTIVITY);
    UINT8_TO_STREAM(pStartConnectivity, 0x00);
    UINT8_TO_STREAM(pStartConnectivity, payloadLen);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
    }
    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Connectivity Test started succeessfully", __func__);
    } else {
        JNI_TRACE_E("%s: Connectivity Test start is failed", __func__);
        return status;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:       getAllUwbSessions
**
** Description:    Get Uwb session information from firmware and update the session data structure
**
** Parameters:     in: sessionId
**                 out: Session Data for the given Session Id
**
** Returns:        None
**
*******************************************************************************/
void NxpUwbManager::getAllUwbSessions(vector<tUWA_SESSION_STATUS_NTF_REVT> &outSessionList) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    uint8_t* pGetAllUwbSessionsCommand = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint8_t payloadLen =  0;

    if(!gIsUwaEnabled) {
        JNI_TRACE_E("%s: UWB device is not initialized", __func__);
        return;
    }

    if(gIsCalibrationOngoing) {
        JNI_TRACE_E("%s: UWB device Calibration is being performed", __func__);
        return;
    }

    pGetAllUwbSessionsCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pGetAllUwbSessionsCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pGetAllUwbSessionsCommand, EXT_UCI_MSG_GET_ALL_UWB_SESSIONS);
    UINT8_TO_STREAM(pGetAllUwbSessionsCommand, 0x00);
    UINT8_TO_STREAM(pGetAllUwbSessionsCommand, payloadLen);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return;
    }

    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        uint8_t *sessionInfoPtr = &sNxpFeatureConf.rsp_data[UCI_RESPONSE_STATUS_OFFSET];
        uint8_t status = 0x00;

        STREAM_TO_UINT8(status, sessionInfoPtr);
        if(status == UWA_STATUS_OK) {
            uint8_t session_count = 0x00;
            STREAM_TO_UINT8(session_count, sessionInfoPtr);
            /* Copy the data from structure to Java Object */
            for(int i = 0; i < session_count; i++) {
                uint8_t session_type = 0x00;
                tUWA_SESSION_STATUS_NTF_REVT outSessionData;
                memset(&outSessionData, 0, sizeof(outSessionData));

                STREAM_TO_UINT32(outSessionData.session_id, sessionInfoPtr);
                STREAM_TO_UINT8(session_type, sessionInfoPtr);
                STREAM_TO_UINT8(outSessionData.state, sessionInfoPtr);

                outSessionList.push_back(outSessionData);
            }
            JNI_TRACE_I("%s: Get Uwb sessions Success", __func__);
        } else {
            JNI_TRACE_E("%s: Get Uwb sessions Failure", __func__);
        }
    }
    JNI_TRACE_I("%s: Exit", __func__);
}


/*******************************************************************************
**
** Function:        configureAuthTagOption()
**
** Description:     Configure the auth tag option
**
** Returns:         Status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::configureAuthTagOption(uint8_t deviceTag, uint8_t modelTag, uint8_t *label, uint8_t length) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status = UWA_STATUS_FAILED;
    uint8_t* pConfigureAuthTagOptionCommand = NULL;
    uint32_t payloadLen =  (uint32_t)(sizeof(deviceTag) + sizeof(modelTag) + length);
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];

    if(label == NULL) {
        JNI_TRACE_E("%s: label is NULL", __func__);
        return status;
    }

    pConfigureAuthTagOptionCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pConfigureAuthTagOptionCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pConfigureAuthTagOptionCommand, EXT_UCI_MSG_CONFIGURE_AUTH_TAG_OPTIONS_CMD);
    UINT8_TO_STREAM(pConfigureAuthTagOptionCommand, 0x00);
    UINT8_TO_STREAM(pConfigureAuthTagOptionCommand, payloadLen);
    UINT8_TO_STREAM(pConfigureAuthTagOptionCommand, deviceTag);
    UINT8_TO_STREAM(pConfigureAuthTagOptionCommand, modelTag);
    ARRAY_TO_STREAM(pConfigureAuthTagOptionCommand, label, length);

    {
        isConfigureAuthTagNtfReceived = false;
        SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
        SetCbStatus(UWA_STATUS_FAILED);
        status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
        if (status == UWA_STATUS_OK) {
            JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
            sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
        } else {
            JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
            return status;
        }
    }

    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
      JNI_TRACE_I("%s: Response -> Configure Auth Tag Option success", __func__);
      if(!isConfigureAuthTagNtfReceived) {
          SyncEventGuard guard (gConfigureAuthTagNtfEvt);
          gConfigureAuthTagNtfEvt.wait(UWB_CMD_TIMEOUT);
      }
      if(isConfigureAuthTagNtfReceived) {
          if(sNxpFeatureConf.ntf_len > 0) {
              uint8_t ntf_status = UWA_STATUS_FAILED;
              uint8_t *configAuthTagPtr = &sNxpFeatureConf.ntf_data[0];
              STREAM_TO_UINT8(ntf_status, configAuthTagPtr);
              if(ntf_status == UWA_STATUS_OK) {
                  status = UWA_STATUS_OK;
              } else {
                  JNI_TRACE_E("%s: Configure Auth Tag Option Notification Not Received", __func__);
                  status = UWA_STATUS_FAILED;
              }
          } else {
              JNI_TRACE_E("%s: Configure Auth Tag Option Notification Length is zero", __func__);
              status = UWA_STATUS_FAILED;
          }
      } else {
          JNI_TRACE_E("%s: Configure Auth Tag Option Notification Not Received", __func__);
      }
    } else {
        JNI_TRACE_E("%s: Response -> Configure Auth Tag Option failed", __func__);
        return status;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        setCalibration()
**
** Description:     set calibration
**
** Returns:         Status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::setCalibration(uint8_t channelNo, uint8_t paramType, uint8_t *calibrationValue, uint8_t length) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status = UWA_STATUS_FAILED;
    uint8_t* pSetCalibrationCommand = NULL;
    uint32_t payloadLen =  (uint32_t)(sizeof(channelNo) + sizeof(paramType) + length);
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];

    if(calibrationValue == NULL) {
        JNI_TRACE_E("%s: calibrationValue is NULL", __func__);
        return status;
    }

    switch(channelNo) {
        case CHANNEL_NUM_5:
        case CHANNEL_NUM_6:
        case CHANNEL_NUM_8:
        case CHANNEL_NUM_9:
            break;
        default:
           JNI_TRACE_E("%s:    Invalid channel number ", __func__);
        return status;
    }

    switch(paramType) {
        case CALIB_PARAM_VCO_PLL_ID:
        case CALIB_PARAM_XTAL_CAP_38_4_MHZ:
        case CALIB_PARAM_RSSI_CALIB_CONSTANT1:
        case CALIB_PARAM_RSSI_CALIB_CONSTANT2:
        case CALIB_PARAM_SNR_CALIB_CONSTANT:
        case CALIB_PARAM_TX_POWER_ID:
        case CALIB_PARAM_MANUAL_TX_POW_CTRL:
        case CALIB_PARAM_PDOA_OFFSET1:
        case CALIB_PARAM_PA_PPA_CALIB_CTRL:
        case CALIB_PARAM_TX_TEMPARATURE_COMP:
        case CALIB_PARAM_PDOA_OFFSET2:

            break;
        default:
           JNI_TRACE_E("%s: Invalid calibration parameter ", __func__);
        return status;
    }
    pSetCalibrationCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pSetCalibrationCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pSetCalibrationCommand, EXT_UCI_MSG_SET_CALIBRATION);
    UINT8_TO_STREAM(pSetCalibrationCommand, 0x00);
    UINT8_TO_STREAM(pSetCalibrationCommand, payloadLen);
    UINT8_TO_STREAM(pSetCalibrationCommand, channelNo);
    UINT8_TO_STREAM(pSetCalibrationCommand, paramType);
    ARRAY_TO_STREAM(pSetCalibrationCommand, calibrationValue, length);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
    }
    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: set calibration succeess", __func__);
    } else {
        JNI_TRACE_E("%s: set calibration failed", __func__);
        return status;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        getCalibration
**
** Description:     Get calibration value
**
** Returns:         status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::getCalibration(uint8_t channelNo, uint8_t paramType, uint8_t *calibrationValue, uint8_t *length) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status = UWA_STATUS_FAILED;
    uint8_t* pGetCalibrationCommand = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint8_t payloadLen =  sizeof(channelNo) + sizeof(paramType);

    if(calibrationValue == NULL || length == NULL) {
        JNI_TRACE_E("%s: calibrationValue or length is NULL", __func__);
        return status;
    }

    switch(channelNo) {
        case CHANNEL_NUM_5:
        case CHANNEL_NUM_6:
        case CHANNEL_NUM_8:
        case CHANNEL_NUM_9:
            break;
        default:
            JNI_TRACE_E("%s:    Invalid channel number ", __func__);
            return status;
    }

    switch(paramType) {
        case CALIB_PARAM_VCO_PLL_ID:
        case CALIB_PARAM_XTAL_CAP_38_4_MHZ:
        case CALIB_PARAM_RSSI_CALIB_CONSTANT1:
        case CALIB_PARAM_RSSI_CALIB_CONSTANT2:
        case CALIB_PARAM_SNR_CALIB_CONSTANT:
        case CALIB_PARAM_TX_POWER_ID:
        case CALIB_PARAM_MANUAL_TX_POW_CTRL:
        case CALIB_PARAM_PDOA_OFFSET1:
        case CALIB_PARAM_PA_PPA_CALIB_CTRL:
        case CALIB_PARAM_TX_TEMPARATURE_COMP:
        case CALIB_PARAM_PDOA_OFFSET2:
            break;
        default:
            JNI_TRACE_E("%s:  Invalid calibration parameter ", __func__);
            return status;
    }

    pGetCalibrationCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pGetCalibrationCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pGetCalibrationCommand, EXT_UCI_MSG_GET_CALIBRATION);
    UINT8_TO_STREAM(pGetCalibrationCommand, 0x00);
    UINT8_TO_STREAM(pGetCalibrationCommand, payloadLen);
    UINT8_TO_STREAM(pGetCalibrationCommand, channelNo);
    UINT8_TO_STREAM(pGetCalibrationCommand, paramType);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
    }

    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        uint8_t *calibrationPtr = &sNxpFeatureConf.rsp_data[UCI_RESPONSE_PAYLOAD_OFFSET];
        *length = sNxpFeatureConf.rsp_len - UCI_RESPONSE_PAYLOAD_OFFSET;
        memcpy(&calibrationValue[0], calibrationPtr, *length);
    } else {
        JNI_TRACE_E("%s: Get calibration value Failure", __func__);
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        performCalibration
**
** Description:     Perform calibration and update the value into calibrationValue parameter if success
**
** Returns:         status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::performCalibration(uint8_t channelNo, uint8_t paramType, uint8_t *calibrationValue, uint8_t *length) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status = UWA_STATUS_FAILED;
    uint8_t* pPerformCalibrationCommand = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint8_t payloadLen =  sizeof(channelNo) + sizeof(paramType);

    if(calibrationValue == NULL || length == NULL) {
        JNI_TRACE_E("%s: calibrationValue or length is NULL", __func__);
        return status;
    }

    switch(channelNo) {
        case CHANNEL_NUM_5:
        case CHANNEL_NUM_6:
        case CHANNEL_NUM_8:
        case CHANNEL_NUM_9:
            break;
        default:
            JNI_TRACE_E("%s:    Invalid channel number ", __func__);
            return status;
    }

    switch(paramType) {
        case DO_CALIB_PARAM_VCO_PLL_ID:
        case DO_CALIB_PARAM_PA_PPA_CALIB_CTRL:
            break;
        default:
            JNI_TRACE_E("%s:  Invalid calibration parameter ", __func__);
            return status;
    }

    pPerformCalibrationCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pPerformCalibrationCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pPerformCalibrationCommand, EXT_UCI_MSG_DO_CALIBRATION);
    UINT8_TO_STREAM(pPerformCalibrationCommand, 0x00);
    UINT8_TO_STREAM(pPerformCalibrationCommand, payloadLen);
    UINT8_TO_STREAM(pPerformCalibrationCommand, channelNo);
    UINT8_TO_STREAM(pPerformCalibrationCommand, paramType);

    {
        isCalibNtfReceived = false;
        SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
        SetCbStatus(UWA_STATUS_FAILED);
        status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
        if (status == UWA_STATUS_OK) {
            JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
            sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
        } else {
            JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
            return status;
        }
    }

    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        if(!isCalibNtfReceived) {
            SyncEventGuard guard (gCalibNtfEvt);
            gCalibNtfEvt.wait(UWB_CMD_TIMEOUT);
        }
        if(isCalibNtfReceived) {
            if(sNxpFeatureConf.ntf_len > 0) {
                uint8_t ntf_status = UWA_STATUS_FAILED;
                uint8_t *calibrationPtr = &sNxpFeatureConf.ntf_data[0];
                STREAM_TO_UINT8(ntf_status, calibrationPtr);
                if(ntf_status == UWA_STATUS_OK) {
                    *length = sNxpFeatureConf.ntf_len - sizeof(ntf_status);
                    STREAM_TO_ARRAY(calibrationValue, calibrationPtr, (*length));
                    status = UWA_STATUS_OK;
                } else {
                    JNI_TRACE_E("%s: Ntf Status is NOT STATUS_OK", __func__);
                    status = UWA_STATUS_FAILED;
                }
            } else {
                JNI_TRACE_E("%s: Ntf len is zero", __func__);
                status = UWA_STATUS_FAILED;
            }
        } else {
            JNI_TRACE_E("%s: Calib Notiication Not Received", __func__);
        }
    } else {
        JNI_TRACE_E("%s: calibration value Failure", __func__);
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        getUwbDeviceTemperature
**
** Description:     get the UWBS temperature if command is success
**
** Returns:         status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::getUwbDeviceTemperature(int8_t *temperatureData) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    uint8_t* pTemperatureData = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint8_t payloadLen =  0;

    pTemperatureData = commandBuffer;
    UCI_MSG_BLD_HDR0(pTemperatureData, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pTemperatureData, EXT_UCI_MSG_QUERY_TEMPERATURE);
    UINT8_TO_STREAM(pTemperatureData, 0x00);
    UINT8_TO_STREAM(pTemperatureData, payloadLen);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
    }
    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        /* Uwb device Temperature byte value available next to response status byte*/
        *temperatureData = sNxpFeatureConf.rsp_data[UCI_RESPONSE_STATUS_OFFSET+1];
    } else {
        JNI_TRACE_E("%s: Query temperature Failure", __func__);
        status = UWA_STATUS_FAILED;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        uartConnectivityTest()
**
** Description:     uart Connectivity Test
**
** Returns:         Status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::uartConnectivityTest(uint8_t length, uint8_t *payload) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status = UWA_STATUS_FAILED;
    uint8_t* pUartConnectivityCommand = NULL;
    uint32_t payloadLen =  (uint32_t)(sizeof(length) + length);
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];

    if(payload == NULL) {
        JNI_TRACE_E("%s: payload is NULL", __func__);
        return status;
    }

    pUartConnectivityCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pUartConnectivityCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pUartConnectivityCommand, EXT_UCI_MSG_UART_CONNECTIVITY_TEST_CMD);
    UINT8_TO_STREAM(pUartConnectivityCommand, 0x00);
    UINT8_TO_STREAM(pUartConnectivityCommand, payloadLen);
    UINT8_TO_STREAM(pUartConnectivityCommand, length);
    ARRAY_TO_STREAM(pUartConnectivityCommand, payload, length);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
    }
    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Uart Connectivity Test succeess", __func__);
    } else {
        JNI_TRACE_E("%s: Uart Connectivity Test failed", __func__);
        return status;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        generateSecureTag()
**
** Description:     generate secure tag
**
** Returns:         Status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::generateSecureTag(uint8_t tagOption, uint8_t *ntfData, uint8_t *ntfLen) {
    JNI_TRACE_I("%s: Entry", __func__);
    uint8_t payLoadLen =  (uint8_t)(UWB_CMAC_TAG_OPTION_LENGTH);
    tUWA_STATUS status;
    uint8_t* pGenCmacTagCmd = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];

    pGenCmacTagCmd = commandBuffer;
    UCI_MSG_BLD_HDR0(pGenCmacTagCmd, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pGenCmacTagCmd, EXT_UCI_MSG_GENERATE_TAG_CMD);
    UINT8_TO_STREAM(pGenCmacTagCmd, 0x00);
    UINT8_TO_STREAM(pGenCmacTagCmd, payLoadLen);
    UINT8_TO_STREAM(pGenCmacTagCmd, tagOption);

    {
      isCmacTagNtfReceived = false;
      SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
      SetCbStatus(UWA_STATUS_FAILED);
      status = UWA_SendRawCommand((payLoadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
      if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
      } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
      }
    }

    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
      JNI_TRACE_I("%s: Response -> Tag generation request is success", __func__);
      if(!isCmacTagNtfReceived) {
          SyncEventGuard guard (gGenCmacTagNtfEvt);
          gGenCmacTagNtfEvt.wait(UWB_CMD_TIMEOUT);
      }
      if(isCmacTagNtfReceived) {
          if(sNxpFeatureConf.ntf_len > 0) {
              uint8_t *cmacTagPtr = &sNxpFeatureConf.ntf_data[0];
              if(sNxpFeatureConf.ntf_data[0] == UWA_STATUS_OK) {
                  *ntfLen = sNxpFeatureConf.ntf_len ;
                  STREAM_TO_ARRAY(ntfData, cmacTagPtr, (*ntfLen));
                  status = UWA_STATUS_OK;
              } else {
                  JNI_TRACE_E("%s: Generate Tag Notification Not received", __func__);
                  status = UWA_STATUS_FAILED;
              }
          } else {
              JNI_TRACE_E("%s: Notification -> Length is zero", __func__);
              status = UWA_STATUS_FAILED;
          }
      } else {
          JNI_TRACE_E("%s: Generate Tag Notification Not Received", __func__);
      }
    } else {
        JNI_TRACE_E("%s: Response -> Tag generation request is failed", __func__);
        return status;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        verifySecureTag()
**
** Description:     verify secure tag
**
** Returns:         Status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::verifySecureTag(uint8_t tagOption, uint8_t *cmacTag, uint8_t cmacTagLen, uint16_t tagVersion) {
    JNI_TRACE_I("%s: Entry", __func__);
    tUWA_STATUS status = UWA_STATUS_FAILED;
    uint8_t* pVerifySecureTagCommand = NULL;
    uint32_t payloadLen =  (uint32_t)(cmacTagLen + sizeof(tagVersion) + UWB_CMAC_TAG_OPTION_LENGTH);
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];

    if(cmacTag == NULL) {
        JNI_TRACE_E("%s: cmacTag is NULL", __func__);
        return status;
    }

    pVerifySecureTagCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pVerifySecureTagCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pVerifySecureTagCommand, EXT_UCI_MSG_VERIFY_TAG_CMD);
    UINT8_TO_STREAM(pVerifySecureTagCommand, 0x00);
    UINT8_TO_STREAM(pVerifySecureTagCommand, payloadLen);
    ARRAY_TO_STREAM(pVerifySecureTagCommand, cmacTag, cmacTagLen);
    UINT8_TO_STREAM(pVerifySecureTagCommand, tagOption);
    UINT16_TO_STREAM(pVerifySecureTagCommand, tagVersion);


    isVerifyTagNtfReceived =  false;
    {
      SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
      SetCbStatus(UWA_STATUS_FAILED);
      status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
      if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
      } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
      }
    }
    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
      JNI_TRACE_I("%s: Response -> verify cmacTag succeess", __func__);
      if(!isVerifyTagNtfReceived) {
          SyncEventGuard guard (gVerifyTagNtfEvt);
          gVerifyTagNtfEvt.wait(UWB_CMD_TIMEOUT);
      }
      if(isVerifyTagNtfReceived) {
          if(sNxpFeatureConf.ntf_len > 0) {
              uint8_t ntf_status = UWA_STATUS_FAILED;
              uint8_t *verifyTagPtr = &sNxpFeatureConf.ntf_data[0];
              STREAM_TO_UINT8(ntf_status, verifyTagPtr);
              if(ntf_status == UWA_STATUS_OK) {
                  status = UWA_STATUS_OK;
              } else {
                  JNI_TRACE_E("%s: Verify Tag Notification Not Received", __func__);
                  status = UWA_STATUS_FAILED;
              }
          } else {
              JNI_TRACE_E("%s: Verify Tag Notification Lentgh is zero", __func__);
              status = UWA_STATUS_FAILED;
          }
      } else {
          JNI_TRACE_E("%s: Verif Tag Notification Not Received", __func__);
      }
    } else {
        JNI_TRACE_E("%s: Response -> Verify Tag failed", __func__);
        return status;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        configureTagVersion()
**
** Description:     configure tag version
**
** Returns:         Status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::configureTagVersion(uint16_t version) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    uint8_t* pConfigureTagVersionCommand = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];

    pConfigureTagVersionCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pConfigureTagVersionCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pConfigureTagVersionCommand, EXT_UCI_MSG_CONFIGURE_AUTH_TAG_VERSION_CMD);
    UINT8_TO_STREAM(pConfigureTagVersionCommand, 0x00);
    UINT8_TO_STREAM(pConfigureTagVersionCommand, sizeof(version));
    UINT16_TO_STREAM(pConfigureTagVersionCommand, version);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((sizeof(version) + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
    }
    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Response -> configure tag version succeess", __func__);
    } else {
        JNI_TRACE_E("%s: Response -> configure tag version failed", __func__);
        return status;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function         handle_generate_tag_ntf
**
** Description      Copies the NTF to the ntf_fata and signales the gGenCmacTagNtfEvt
**
** Returns          void
**
*******************************************************************************/
void handle_generate_tag_ntf(uint8_t* p, uint16_t len) {
  isCmacTagNtfReceived = true;
  if(p == NULL) {
    JNI_TRACE_E("%s: data is null", __func__);
  } else {
    if(len != 0) {
      sNxpFeatureConf.ntf_len = len;
      memcpy(&sNxpFeatureConf.ntf_data[0], p, len);
    }
  }
  SyncEventGuard guard(gGenCmacTagNtfEvt);
  gGenCmacTagNtfEvt.notifyOne();
}

/*******************************************************************************
**
** Function         handle_verify_tag_ntf
**
** Description      Copies the NTF to the ntf_fata and signales the gVerifyTagNtfEvt
**
** Returns          void
**
*******************************************************************************/
void handle_verify_tag_ntf(uint8_t* p, uint16_t len) {
  isVerifyTagNtfReceived = true;
  if(p == NULL) {
    JNI_TRACE_E("%s: data is null", __func__);
  } else {
    if(len != 0) {
      sNxpFeatureConf.ntf_len = len;
      memcpy(&sNxpFeatureConf.ntf_data[0], p, len);
    }
  }
  SyncEventGuard guard(gVerifyTagNtfEvt);
  gVerifyTagNtfEvt.notifyOne();
}

/*******************************************************************************
**
** Function         handle_configure_auth_tag_option_ntf
**
** Description      Copies the NTF to the ntf_data and signales the gConfigureAuthTagNtfEvt
**
** Returns          void
**
*******************************************************************************/
void handle_configure_auth_tag_option_ntf(uint8_t* p, uint16_t len) {
  isConfigureAuthTagNtfReceived = true;
  if(p == NULL) {
    JNI_TRACE_E("%s: data is null", __func__);
  } else {
    if(len != 0) {
      sNxpFeatureConf.ntf_len = len;
      memcpy(&sNxpFeatureConf.ntf_data[0], p, len);
    }
  }
  SyncEventGuard guard(gConfigureAuthTagNtfEvt);
  gConfigureAuthTagNtfEvt.notifyOne();
}

/*******************************************************************************
**
** Function:        setwtxcount
**
** Description:     This functoion is called to set the WtxCount
**
** Returns:         status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::setWtxCount(uint8_t count) {
    JNI_TRACE_I("%s: Entry", __func__);
    tUWA_STATUS status;
    uint8_t* psetwtxcountCommand = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint8_t payloadLen =  5;

    psetwtxcountCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(psetwtxcountCommand, UCI_MT_CMD, UCI_GID_CORE);
    UCI_MSG_BLD_HDR1(psetwtxcountCommand, UCI_MSG_CORE_SET_CONFIG);
    UINT8_TO_STREAM(psetwtxcountCommand, 0x00);
    UINT8_TO_STREAM(psetwtxcountCommand, payloadLen);
    UINT8_TO_STREAM(psetwtxcountCommand, 1);
    UINT8_TO_STREAM(psetwtxcountCommand, EXTENDED_DEVICE_CONFIG_ID);
    UINT8_TO_STREAM(psetwtxcountCommand, UCI_EXT_PARAM_ID_WTX_COUNT);
    UINT8_TO_STREAM(psetwtxcountCommand, UCI_EXT_PARAM_ID_WTX_COUNT_LEN);
    UINT8_TO_STREAM(psetwtxcountCommand, count);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT);
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
    }
    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: set WtxCount succeess", __func__);
    } else {
        JNI_TRACE_E("%s: set WtxCount failed", __func__);
    }
    return status;
}
/*******************************************************************************
**
** Function:        sendSetCalibrationIntegrityProtection()
**
** Description:     Send Command to UWBS
**                  uint8_t: TAG Option
**                  uint8_t *: Calibratatio Paramaters
**
** Returns:         tUWA_STATUS: sendSetCalibrationIntegrityProtection() Status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::sendSetCalibrationIntegrityProtection(uint8_t tagOption, uint32_t calibParams)
{
    JNI_TRACE_I("%s: Entry", __func__);
    tUWA_STATUS status;
    uint8_t* pSetCalIntProtCommand = NULL;
    uint8_t payloadLen =  (uint8_t)(UWB_CMAC_TAG_OPTION_LENGTH + UCI_CALIB_PARAM_BITMASK_LENGTH);
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint16_t calibParamBitmask = calibParams & UCI_CALIB_PARAM_BITMASK_MASK;

    pSetCalIntProtCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pSetCalIntProtCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pSetCalIntProtCommand, EXT_UCI_MSG_CALIBRATION_INTEGRATION_PROTECTION_CMD);
    UINT8_TO_STREAM(pSetCalIntProtCommand, 0x00);
    UINT8_TO_STREAM(pSetCalIntProtCommand, payloadLen);
    UINT8_TO_STREAM(pSetCalIntProtCommand, tagOption);
    UINT16_TO_STREAM(pSetCalIntProtCommand, calibParamBitmask);


    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
      JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
      sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
      JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
      return status;
    }
    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
      JNI_TRACE_I("%s:Set Calibratation Integrity Protection", __func__);
    } else {
      JNI_TRACE_E("%s:Set Calibratation Integrity Protection failed", __func__);
      return status;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        sendGetUwbDeviceTimeStamp()
**
** Description:     Send Command to UWBS
**
** Returns:         tUWA_STATUS: sendGetUwbDeviceTimeStamp() Status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::sendGetUwbDeviceTimeStamp(uint8_t *pUwbTimeStamp, uint8_t *respLen)
{
    JNI_TRACE_I("%s: Entry", __func__);
    tUWA_STATUS status;
    uint8_t* pSendTimeStampCommand = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];

    pSendTimeStampCommand = commandBuffer;
    UCI_MSG_BLD_HDR0(pSendTimeStampCommand, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pSendTimeStampCommand, EXT_UCI_MSG_QUERY_UWB_TIMESTAMP_CMD);
    UINT8_TO_STREAM(pSendTimeStampCommand, 0x00);
    UINT8_TO_STREAM(pSendTimeStampCommand, 0x00);


    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand(UCI_MSG_HDR_SIZE, commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
      JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
      sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
      JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
      return status;
    }

    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
      uint8_t *getTimeStampRsp = &sNxpFeatureConf.rsp_data[UCI_RESPONSE_PAYLOAD_OFFSET];
      *respLen = sNxpFeatureConf.rsp_len - UCI_RESPONSE_PAYLOAD_OFFSET;
      memcpy(pUwbTimeStamp, getTimeStampRsp, *respLen);
      JNI_TRACE_I("%s:Get Uwb TimeStamp Success", __func__);
    } else {
      JNI_TRACE_E("%s:Get Uwb TimeStamp failed", __func__);
      return false;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        getRanMultiplier
**
** Description:     get the Ran Multiplier value
**
** Returns:         status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::getRanMultiplier(uint8_t *ranMultiplierVal) {
    JNI_TRACE_I("%s: Entry", __func__);

    tUWA_STATUS status;
    uint8_t* pRanMultiplierCmd = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];
    uint8_t payloadLen =  0;

    pRanMultiplierCmd = commandBuffer;
    UCI_MSG_BLD_HDR0(pRanMultiplierCmd, UCI_MT_CMD, UCI_GID_SESSION_MANAGE);
    UCI_MSG_BLD_HDR1(pRanMultiplierCmd, UCI_MSG_SESSION_GET_POSSIBLE_RAN_MULTIPLIER_VALUE);
    UINT8_TO_STREAM(pRanMultiplierCmd, 0x00);
    UINT8_TO_STREAM(pRanMultiplierCmd, payloadLen);

    SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
    SetCbStatus(UWA_STATUS_FAILED);
    status = UWA_SendRawCommand((payloadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
    if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
    } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
    }
    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
        *ranMultiplierVal = sNxpFeatureConf.rsp_data[UCI_RESPONSE_PAYLOAD_OFFSET];
    } else {
        JNI_TRACE_E("%s: get Ran Multiplier failed", __func__);
        status = UWA_STATUS_FAILED;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:        sendEraseUrskCommand()
**
** Description:     Send Command to UWBS
**
** Returns:         tUWA_STATUS: sendEraseUrskCommand() Status
**
*******************************************************************************/
tUWA_STATUS NxpUwbManager::sendEraseUrskCommand(uint8_t numOfSessionIds, uint32_t *sessionIdsList) {
    JNI_TRACE_I("%s: Entry", __func__);
    uint8_t payLoadLen =  (uint8_t)(sizeof(numOfSessionIds) + numOfSessionIds*SESSION_ID_LEN);
    tUWA_STATUS status;
    uint8_t* pEraseUrskCmd = NULL;
    uint8_t commandBuffer[UCI_MAX_PAYLOAD_SIZE];

    pEraseUrskCmd = commandBuffer;
    UCI_MSG_BLD_HDR0(pEraseUrskCmd, UCI_MT_CMD, UCI_GID_PROPRIETARY);
    UCI_MSG_BLD_HDR1(pEraseUrskCmd, EXT_UCI_MSG_URSK_DELETION_REQ_CMD);
    UINT8_TO_STREAM(pEraseUrskCmd, 0x00);
    UINT8_TO_STREAM(pEraseUrskCmd, payLoadLen);
    UINT8_TO_STREAM(pEraseUrskCmd, numOfSessionIds);
    for (uint8_t i = 0; i < numOfSessionIds; i++) {
        UINT32_TO_STREAM(pEraseUrskCmd, sessionIdsList[i]);
    }

    {
      SyncEventGuard guard(sNxpFeatureConf.NxpConfigEvt);
      SetCbStatus(UWA_STATUS_FAILED);
      status = UWA_SendRawCommand((payLoadLen + UCI_MSG_HDR_SIZE), commandBuffer, CommandResponse_Cb);
      if (status == UWA_STATUS_OK) {
        JNI_TRACE_I("%s: Success UWA_SendRawCommand", __func__);
        sNxpFeatureConf.NxpConfigEvt.wait(UWB_CMD_TIMEOUT); /* wait for callback */
      } else {
        JNI_TRACE_E("%s: Failed UWA_SendRawCommand", __func__);
        return status;
      }
    }
    status = GetCbStatus();
    if (status == UWA_STATUS_OK) {
      JNI_TRACE_I("%s: Delete Request for URSK is Success", __func__);
    } else {
      JNI_TRACE_E("%s:Delete Request for URSK is  failed", __func__);
      return status;
    }
    JNI_TRACE_I("%s: Exit", __func__);
    return status;
}

bool NxpUwbManager::verifyExpectedReasonCode(uint8_t expectedState, uint8_t reasonCode) {
    bool status = true;
    switch(reasonCode) {
        case SESSION_RSN_TERMINATION_ON_MAX_RR_RETRY:
        case SESSION_RSN_MAX_RANGING_BLOCK_REACHED:
        case SESSION_RSN_IN_BAND_STOP_RCVD:
          status = false;
          break;
        case SESSION_RSN_URSK_TTL_MAX_VALUE_REACHED:
          if(expectedState == UWB_SESSION_ACTIVE) status = false;
          break;
    }
    return status;
}


}
