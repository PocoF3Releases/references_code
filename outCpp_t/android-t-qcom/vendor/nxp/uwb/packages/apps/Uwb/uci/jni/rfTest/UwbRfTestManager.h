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

#ifndef _UWB_RFTEST_NATIVE_MANAGER_H_
#define _UWB_RFTEST_NATIVE_MANAGER_H_
#define PER_RX_NTF_LEN      53   /* PER Ntf Len without vendor data */
#define LOOPBACK_NTF_LEN    21   /* LoopBack Ntf Len without vendor data & PSDU data */
#define TEST_RX_NTF_LEN     16   /* Test_Rx Ntf Len without vendor data & PSDU data */
namespace android {

typedef struct {
  uint8_t status;              ///< Status
  uint32_t attempts;           ///< No. of RX attempts
  uint32_t ACQ_detect;         ///< No. of times signal was detected
  uint32_t ACQ_rejects;        ///< No. of times signal was rejected
  uint32_t RX_fail;            ///< No. of times RX did not go beyond ACQ stage
  uint32_t sync_cir_ready;     ///< No. of times sync CIR ready event was received
  uint32_t sfd_fail;           ///< No. of time RX was stuck at either ACQ detect or sync CIR ready
  uint32_t sfd_found;          ///< No. of times SFD was found
  uint32_t phr_dec_error;      ///< No. of times PHR decode failed
  uint32_t phr_bit_error;      ///< No. of times PHR bits in error
  uint32_t psdu_dec_error;     ///< No. of times payload decode failed
  uint32_t psdu_bit_error;     ///< No. of times payload bits in error
  uint32_t sts_found;          ///< No. of times STS detection was successful
  uint32_t eof;                ///< No. of times end of frame event was triggered
  uint8_t vendor_specific_length;
  uint8_t vendor_specific_info[UCI_MAX_PAYLOAD_SIZE];
} tPER_RX_DATA;

typedef struct {
  uint8_t status;                   ///< Status
  uint32_t txts_int;                ///< Integer part of timestamp in 1/124.8 us resolution
  uint16_t txts_frac;               ///< Fractional part of timestamp in 1/124.8/512 µs resolution
  uint32_t rxts_int;                ///< Integer part of timestamp in 1/124.8 us resolution
  uint16_t rxts_frac;               ///< Fractional part of timestamp in 1/124.8/512 us resolution
  uint16_t aoa_azimuth;             ///< Angle of Arrival in degrees
  uint16_t aoa_elevation;           ///< Angle of Arrival in degrees
  uint16_t phr;                     ///< Received PHR
  uint16_t psdu_data_length;
  uint8_t psdu_data[UCI_PSDU_SIZE_4K];  ///< Received PSDU Data bytes
  uint8_t vendor_specific_length;
  uint8_t vendor_specific_info[UCI_MAX_PAYLOAD_SIZE];
} tUWB_LOOPBACK_DATA;

typedef struct {
  uint8_t status;
  uint32_t rx_done_ts_int;
  uint16_t rx_done_ts_frac;
  uint16_t aoa_azimuth;
  uint16_t aoa_elevation;
  uint8_t toa_gap;
  uint16_t phr;
  uint16_t psdu_data_length;
  uint8_t psdu_data[UCI_PSDU_SIZE_4K];
  uint8_t vendor_specific_length;
  uint8_t vendor_specific_info[UCI_MAX_PAYLOAD_SIZE];
} tUWB_RX_DATA;

typedef struct {
  uint8_t status;              ///< Status
} tPERIODIC_TX_DATA;

class UwbRfTestManager {
public:
    static UwbRfTestManager& getInstance();
    void doLoadSymbols(JNIEnv* env, jobject o);

    /* CallBack functions */
    void onPeriodicTxDataNotificationReceived(uint16_t len, uint8_t* data);
    void onPerRxDataNotificationReceived(uint16_t len, uint8_t* data);
    void onLoopBackTestDataNotificationReceived(uint16_t len, uint8_t* data);
    void onRxTestDataNotificationReceived(uint16_t len, uint8_t* data);

    /* API functions */
    jbyteArray setTestConfigurations(JNIEnv* env, jobject o, jint sessionId, jint noOfParams, jint testConfigLen, jbyteArray TestConfig);
    jbyteArray getTestConfigurations(JNIEnv* env, jobject o, jint sessionId, jint noOfParams, jint testConfigLen, jbyteArray TestConfig);
    jbyte startPerRxTest(JNIEnv* env, jobject o, jbyteArray refPsduData);
    jbyte startPeriodicTxTest(JNIEnv* env, jobject o, jbyteArray psduData);
    jbyte startUwbLoopBackTest(JNIEnv* env, jobject o, jbyteArray psduData);
    jbyte startRxTest(JNIEnv* env, jobject o);
    jbyte stopRfTest(JNIEnv* env, jobject o);
private:
    UwbRfTestManager();

    static UwbRfTestManager mObjTestManager;

    JavaVM *mVm;

    jclass mClass;     // Reference to Java  class
    jobject mObject;    // Weak ref to Java object to call on

    jclass mPeriodicTxDataClass;
    jclass mPerRxDataClass;
    jclass mUwbLoopBackDataClass;
    jclass mRxDataClass;

    jmethodID mOnPeriodicTxDataNotificationReceived;
    jmethodID mOnPerRxDataNotificationReceived;
    jmethodID mOnLoopBackTestDataNotificationReceived;
    jmethodID mOnRxTestDataNotificationReceived;
};

}
#endif