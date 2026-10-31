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

#ifndef _UWB_NATIVE_MANAGER_H_
#define _UWB_NATIVE_MANAGER_H_

namespace android {

class UwbEventManager {
public:
    static UwbEventManager& getInstance();
    void doLoadSymbols(JNIEnv* env, jobject o);

    void onDeviceStateNotificationReceived(uint8_t state);
    void onRangeDataNotificationReceived(tUWA_RANGE_DATA_NTF* ranging_ntf_data);
    void onSessionStatusNotificationReceived(uint32_t sessionId, uint8_t state, uint8_t reasonCode);
    void onCoreGenericErrorNotificationReceived(uint8_t state);
    void onMulticastListUpdateNotificationReceived(tUWA_SESSION_UPDATE_MULTICAST_LIST_NTF *multicast_list_ntf);
    void onBlinkDataTxNotificationReceived(uint8_t state);
    void onDataTransferStatusReceived(uint32_t sesssionID, uint8_t status);
    void onDataReceived(tUWA_RX_DATA_REVT *rcvd_data);

private:
    UwbEventManager();

    static UwbEventManager mObjUwbManager;

    JavaVM *mVm;

    jclass mClass;     // Reference to Java  class
    jobject mObject;    // Weak ref to Java object to call on

    jclass mRangeDataClass;
    jclass mRangingTwoWayMeasuresClass;
    jclass mRangeTdoaMeasuresClass;
    jclass mMulticastUpdateListDataClass;

    jmethodID mOnDeviceStateNotificationReceived;
    jmethodID mOnRangeDataNotificationReceived;
    jmethodID mOnSessionStatusNotificationReceived;
    jmethodID mOnCoreGenericErrorNotificationReceived;
    jmethodID mOnMulticastListUpdateNotificationReceived;
    jmethodID mOnBlinkDataTxNotificationReceived;
    jmethodID mOnDataTransferStatusReceived;
    jmethodID mOnDataReceived;
};

}
#endif