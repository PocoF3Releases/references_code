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

#ifndef _NXP_UWB_NATIVE_MANAGER_H_
#define _NXP_UWB_NATIVE_MANAGER_H_

#include <vector>

namespace android {
jint getDeviceMaxPpmValue();
class NxpUwbManager {
public:
    static NxpUwbManager& getInstance();
    void doLoadSymbols(JNIEnv* env, jobject o);

    /* Notifications to Service Layer */
    void onExtendedNotificationReceived(uint8_t notificationType, uint8_t *data, uint16_t length);
    void onRawUciNotificationReceived(uint8_t *data, uint16_t length);
    jbyteArray getExtDeviceCapability(JNIEnv* env, jobject o);
    jbyteArray setExtDeviceConfigurations(JNIEnv* env, jobject o, int noOfParams, int deviceConfigLen, jbyteArray extDeviceConfigArray);
    jbyteArray getExtDeviceConfigurations(JNIEnv* env, jobject o, jint noOfParams, int deviceConfigLen, jbyteArray extDeviceConfigArray);
    jbyteArray setDebugConfigurations(JNIEnv* env, jobject o, int sessionId, int noOfParams, int debugConfigLen, jbyteArray debugConfigArray, jboolean enableCirDump, jboolean enableFwDump);
    jbyte bind(JNIEnv* env, jobject o);
    jbyte getBindingStatus(JNIEnv* env, jobject o);
    jbyteArray getBindingCount(JNIEnv* env, jobject o);
    jbyte startLoopBackTest(JNIEnv* env, jobject o, int loopCount, int interval);
    jbyte startConnectivityTest(JNIEnv* env, jobject o);
    void getAllUwbSessions(std::vector<tUWA_SESSION_STATUS_NTF_REVT> &outSessionList);
    jbyteArray getAllUwbSessions(JNIEnv* env, jobject o);
    jbyte setCalibration(JNIEnv* env, jobject o, jbyte channelNo, jbyte paramType, jbyteArray calibrationValue);
    jbyteArray getCalibration(JNIEnv* env, jobject o, jbyte channelNo, jbyte paramType);
    jbyteArray performCalibration(JNIEnv* env, jobject o, jbyte channelNo, jbyte paramType);
    jbyteArray getUwbDeviceTemperature(JNIEnv* env, jobject o);
    jbyte setWtxCount(JNIEnv* env, jobject o,jbyte count);
    jbyte uartConnectivityTest(JNIEnv* env, jobject o, jbyteArray payload);
    jbyte verifySecureTag(JNIEnv* e, jobject o, jbyteArray cmacTag, jbyte tagOption, jshort tagVersion);
    jbyteArray generateSecureTag(JNIEnv* e, jobject o,jbyte tagOption);
    jbyte setCalibrationIntegrityProtection(JNIEnv* e, jobject o, jbyte tagOption,jint calibParamBitmask);
    jbyte configureTagVersion(JNIEnv* e, jobject o, jshort version);
    jbyteArray sendRawUci(JNIEnv* env, jobject o, jbyteArray rawUci, jint cmdLen);
    bool setExtDeviceConfigurations(uint8_t noOfParams, uint8_t extDeviceConfigLen, const uint8_t* pInExtDeviceConfig, uint8_t *pOutExtDeviceConfig, uint8_t *configLen);
    tUWA_STATUS SetDefaultDebugConfigurations(uint32_t sessionId);
    jbyte configureAuthTagOption(JNIEnv* env, jobject o, jbyte deviceTag, jbyte modelTag, jbyteArray label);
    jbyteArray getUwbDeviceTimeStamp(JNIEnv* env, jobject o);
    jbyte getRanMultiplier(JNIEnv* env, jobject o);
    jbyte eraseURSK(JNIEnv* env, jobject o,jbyte numOfSessionIds, jintArray sessionIdsList);
    jbyte enableConformanceTest(JNIEnv* env, jobject o,jboolean enable);
    bool verifyExpectedReasonCode(uint8_t expectedState, uint8_t reasonCode);

private:
    NxpUwbManager();
    /* Internal Functions */
    bool getExtDeviceCapability(uint8_t *pOutExtDeviceCapability, uint8_t *configLen);
    bool getExtDeviceConfigurations(uint8_t noOfParams, uint8_t extDeviceConfigLen, const uint8_t *pInExtDeviceConfig, uint8_t *pOutExtDeviceConfig, uint8_t *configLen);
    tUWA_STATUS setDebugConfigurations(uint32_t sessionId, uint8_t noOfParams, uint8_t configLen, const uint8_t* pDebugConfig, uint8_t *pOutDebugConfig, uint8_t *debugConfigLen);
    bool performBind();
    bool getBindingStatus();
    tUWA_STATUS startConnectivityTest();
    tUWA_STATUS setCalibration(uint8_t channelNo, uint8_t paramType, uint8_t *calibrationValue, uint8_t length);
    tUWA_STATUS performCalibration(uint8_t channelNo, uint8_t paramType, uint8_t *calibrationValue, uint8_t *length);
    tUWA_STATUS getUwbDeviceTemperature(int8_t *temperatureData);
    tUWA_STATUS startLoopBackTest(uint16_t loopCount, uint16_t interval);
    tUWA_STATUS getCalibration(uint8_t channelNo, uint8_t paramType, uint8_t *calibrationValue, uint8_t *length);
    tUWA_STATUS setWtxCount(uint8_t  Count);
    tUWA_STATUS uartConnectivityTest(uint8_t length, uint8_t *payload);
    tUWA_STATUS verifySecureTag(uint8_t tagOption, uint8_t *cmacTag, uint8_t cmacTagLen, uint16_t tagVersion);
    tUWA_STATUS generateSecureTag(uint8_t tagOption, uint8_t *pCmacTag, uint8_t * pCmacTagLen);
    tUWA_STATUS configureTagVersion(uint16_t version);
    tUWA_STATUS sendRawUci(uint8_t *rawCmd, uint8_t cmdLen, uint8_t *rspData, uint8_t *rspLen );
    tUWA_STATUS configureAuthTagOption(uint8_t deviceTag, uint8_t modelTag, uint8_t *label, uint8_t length);
    tUWA_STATUS sendSetCalibrationIntegrityProtection(uint8_t tagOption, uint32_t calibParams);
    tUWA_STATUS sendGetUwbDeviceTimeStamp(uint8_t *rspData, uint8_t *rspLen);
    tUWA_STATUS getRanMultiplier(uint8_t *ranMultiplierVal);
    tUWA_STATUS sendEraseUrskCommand(uint8_t noOfSessionIds, uint32_t *sessionIdsList);
    tUWA_STATUS enableConformanceTest(bool enable);
    static NxpUwbManager mObjNxpManager;

    JavaVM *mVm;

    jclass mClass;     // Reference to Java  class
    jobject mObject;    // Weak ref to Java object to call on

    jmethodID mOnExtendedNotificationReceived;
    jmethodID mOnRawUciNotificationReceived;
};

void extDeviceManagementCallback(uint8_t event, uint16_t payloadLength, uint8_t* pResponseBuffer);
void handle_generate_tag_ntf(uint8_t* p, uint16_t len);
void handle_verify_tag_ntf(uint8_t* p, uint16_t len);
void handle_configure_auth_tag_option_ntf(uint8_t* p, uint16_t len);
void loadAntennaConfigs();
tUWA_STATUS SetExtDeviceConfigurations();
}
#endif