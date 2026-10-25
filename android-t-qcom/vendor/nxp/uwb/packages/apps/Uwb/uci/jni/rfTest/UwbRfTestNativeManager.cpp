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
#include "UwbRfTestManager.h"
#include "UwbAdaptation.h"
#include "SyncEvent.h"
#include "uwb_config.h"
#include "uwb_hal_int.h"
#include "JniLog.h"
#include "ScopedJniEnv.h"
#include "uci_ext_defs.h"

namespace android {

const char* UWB_RFTEST_NATIVE_MANAGER_CLASS_NAME = "com/android/uwb/dhimpl/UwbTestNativeManager";

static UwbRfTestManager &uwbRfTestManager = UwbRfTestManager::getInstance();

/*******************************************************************************
**
** Function:        uwbRfTestNativeManager_setTestConfigurations()
**
** Description:     application shall configure the Test configuration parameters
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  sessionId: All Test configurations belonging to this Session ID
**                  noOfParams : The number of Test Configuration fields to follow
**                  testConfigLen : Length of TestConfigData
**                  TestConfig : Test Configurations for session
**
** Returns:         Returns byte array
**
**
*******************************************************************************/
jbyteArray uwbRfTestNativeManager_setTestConfigurations(JNIEnv* env, jobject o, jint sessionId, jint noOfParams, jint testConfigLen, jbyteArray testConfigArray) {
    return uwbRfTestManager.setTestConfigurations(env, o, sessionId, noOfParams, testConfigLen, testConfigArray);
}

/*******************************************************************************
**
** Function:        uwbRfTestNativeManager_getTestConfigurations
**
** Description:     retrive the UWB device state..
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  session id : Session Id to which get All test Cofig list
**                  noOfParams: Number of Test Config Params
**                  testConfigLen: Total Test Config lentgh
**                  TestConfig: Test Config Id List
**
** Returns:         Returns App Config Info List in TLV.
**
*******************************************************************************/
jbyteArray uwbRfTestNativeManager_getTestConfigurations(JNIEnv* env, jobject o, jint sessionId, jint noOfParams, jint testConfigLen, jbyteArray testConfigArray) {
    return uwbRfTestManager.getTestConfigurations(env, o, sessionId, noOfParams, testConfigLen, testConfigArray);
}

/*******************************************************************************
**
** Function:        uwbRfTestNativeManager_startPerRxTest
**
** Description:     start PER RX performance test
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  refPsduData : Reference Psdu Data
**
** Returns:         if success UWb_STATUS_OK else returns
**                  UWA_STATUS_FAILED
*******************************************************************************/
jbyte uwbRfTestNativeManager_startPerRxTest (JNIEnv* env, jobject o, jbyteArray refPsduData) {
    return uwbRfTestManager.startPerRxTest(env, o, refPsduData);
}

/*******************************************************************************
**
** Function:        uwbRfTestNativeManager_startPeriodicTxTest
**
** Description:     start PERIODIC Tx Test
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  psduData : Reference Psdu Data
**
** Returns:         if success UWb_STATUS_OK else returns
**                  UWA_STATUS_FAILED
**
*******************************************************************************/
jbyte uwbRfTestNativeManager_startPeriodicTxTest(JNIEnv* env, jobject o, jbyteArray psduData) {
    return uwbRfTestManager.startPeriodicTxTest(env, o, psduData);
}

/*******************************************************************************
**
** Function:        uwbRfTestNativeManager_startUwbLoopBackTest
**
** Description:     start Rf Loop back test
**
** Params:          env: JVM environment.
**                  o: Java object.
**                  psduData : Reference Psdu Data
**
** Returns:         if success UWb_STATUS_OK else returns
**                  UWA_STATUS_FAILED
**
*******************************************************************************/
jbyte uwbRfTestNativeManager_startUwbLoopBackTest (JNIEnv* env, jobject o, jbyteArray psduData) {
    return uwbRfTestManager.startUwbLoopBackTest(env, o, psduData);
}

/*******************************************************************************
**
** Function:        uwbRfTestNativeManager_stopRfTest
**
** Description:     stop PER performance test
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         if success UWb_STATUS_OK else returns
**                  UWA_STATUS_FAILED
*******************************************************************************/
jbyte uwbRfTestNativeManager_stopRfTest (JNIEnv* env, jobject o) {
    return uwbRfTestManager.stopRfTest(env, o);
}

/*******************************************************************************
**
** Function:        uwbRfTestNativeManager_startRxTest
**
** Description:     start RX test
**
** Params:          env: JVM environment.
**                  o: Java object.
**
** Returns:         if success UWb_STATUS_OK else returns
**                  UWA_STATUS_FAILED
*******************************************************************************/
jbyte uwbRfTestNativeManager_startRxTest (JNIEnv* env, jobject o) {
    return uwbRfTestManager.startRxTest(env, o);
}

/*******************************************************************************
**
** Function:        uwbRfTestNativeManager_init
**
** Description:     Initialize variables.
**
** Params           env: JVM environment.
**                  o: Java object.
**
** Returns:         True if ok.
**
*******************************************************************************/
jboolean uwbRfTestNativeManager_init(JNIEnv* env, jobject o) {
    uwbRfTestManager.doLoadSymbols(env, o);
    return JNI_TRUE;
}


/*****************************************************************************
**
** JNI functions for android
** UWB service layer has to invoke these APIs to get required funtionality
**
*****************************************************************************/
static JNINativeMethod gMethods[] = {
    {"nativeInit", "()Z", (void*)uwbRfTestNativeManager_init},
    {"nativeSetTestConfigurations", "(III[B)[B", (void*)uwbRfTestNativeManager_setTestConfigurations},
    {"nativeGetTestConfigurations", "(III[B)[B", (void*)uwbRfTestNativeManager_getTestConfigurations},
    {"nativeStartPerRxTest", "([B)B", (void*)uwbRfTestNativeManager_startPerRxTest},
    {"nativeStartPeriodicTxTest", "([B)B", (void*)uwbRfTestNativeManager_startPeriodicTxTest},
    {"nativeStartUwbLoopBackTest", "([B)B", (void*)uwbRfTestNativeManager_startUwbLoopBackTest},
    {"nativeStartRxTest","()B",(void*)uwbRfTestNativeManager_startRxTest},
    {"nativeStopRfTest","()B",(void*)uwbRfTestNativeManager_stopRfTest}
};

/*******************************************************************************
**
** Function:        register_UwbRfTestNativeManager
**
** Description:     Regisgter JNI functions of UwbEventManager class with Java Virtual Machine.
**
** Params:          env: Environment of JVM.
**
** Returns:         Status of registration (JNI version).
**
*******************************************************************************/
int register_com_android_uwb_dhimpl_UwbRfTestNativeManager(JNIEnv* env) {
  JNI_TRACE_I("%s: enter", __func__);
  return jniRegisterNativeMethods(env, UWB_RFTEST_NATIVE_MANAGER_CLASS_NAME, gMethods,
                                  sizeof(gMethods)/sizeof(gMethods[0]));
}

}