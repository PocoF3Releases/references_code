/*
 *
 * Copyright 2019-2020 NXP.
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only be
 * used strictly in accordance with the applicable license terms. By expressly
 * accepting such terms or by downloading,installing, activating and/or
 * otherwise using the software, you are agreeing that you have read,and that
 * you agree to comply with and are bound by, such license terms. If you do not
 * agree to be bound by the applicable license terms, then you may not retain,
 * install, activate or otherwise use the software.
 *
 */
#ifndef _UWBAPI_INTERNAL_H_
#define _UWBAPI_INTERNAL_H_

#include "UwbJniTypes.h"
#include "UwbJniExtTypes.h"
#include <nativehelper/ScopedLocalRef.h>
#include "UwbJniUtil.h"

namespace android {

/*Antenna pair selection config*/
#define ANTENNA_CONFIG_1 0x01
#define ANTENNA_CONFIG_2 0x02
#define ANTENNA_CONFIG_3 0x03
#define ANTENNA_CONFIG_4 0x04
#define ANTENNA_CONFIG_5 0x05

#define UWB_CMD_TIMEOUT            4000  // UWB command time out

/* rf performance test types */
#define UWB_PERIODIC_TX_TEST_MODE  0x00
#define UWB_PER_RX_TEST_MODE       0x01
#define UWB_LOOPBACK_TEST_MODE     0x02

/*DPD_ENTRY_TIMEOUT Time Range*/
#define DPD_TIMEOUT_MIN            100
#define DPD_TIMEOUT_MAX            2000

#define UWB_DEVICE_TEMPERATURE_PAYLOAD_LEN        2
#define UWB_MAX_UART_PAYLOAD_LENGTH             253
#define UWB_CMAC_TAG_DERIVE_KEY_LENGTH            8
#define UWB_CMAC_TAG_LENGTH                      16
#define UWB_CMAC_TAG_OPTION_LENGTH                1
#define UCI_AUTH_TAG_LABEL_LENGTH                 2
#define UCI_TAG_VERSION_LENGTH                    2
#define MODEL_SPECEFIC_TAG                     0x01

/* CALIBRATION_INTEGRITY_PROCTECTION */
#define UCI_CALIB_PARAM_BITMASK_LENGTH            2
#define UCI_CALIB_PARAM_BITMASK_MASK              0x1FFF

/*Manufacture data ID'S*/
#define UWB_DEVICE_NAME_ID                  0x00
#define UWB_FIRMWARE_VERSION_ID             0x01
#define UWB_NXP_UCI_VERSION_ID              0x02
#define UWB_NXP_CHIP_ID_ID                  0x03
#define UWB_FW_BOOT_MODE                    0x04
#define UWB_NXP_FIRA_UCI_GENERIC_VERSION    0x05
#define UWB_NXP_FIRA_UCI_TEST_VERSION       0x06
#define UWB_MAX_PPM_VALUE                  0x07

#define FACTORY_FW_BOOT_MODE          0x00
#define USER_FW_BOOT_MODE             0x01

#define MSB_BITMASK  0x000000FF


/*Binding status NTF recepriont status 8=*/
enum {
    ESE_BINDING_NTF_DEFAULT = 0x00,
    ESE_BINDING_NTF_PENDING = 0x01,
    ESE_BINDING_NTF_RECEIVED = 0x02
};

typedef enum {
    UWB_MW_STATE_DISABLED = 0x00,
    UWB_MW_STATE_DISABLING = 0x01,
    UWB_MW_STATE_ENABLED = 0x02,
    UWB_MW_STATE_ENABLING = 0x03
} tMw_State;

/* extern declarations */
extern bool uwb_debug_enabled;
extern bool gIsUwaEnabled;
extern bool gIsCalibrationOngoing;
extern SyncEvent gBindStatusNtfEvt;
extern SyncEvent gSeCommErrNtfEvt;
extern SyncEvent gGenCmacTagNtfEvt;
extern SyncEvent gVerifyTagNtfEvt;
extern uint8_t gBindStatus;
extern void setBindingStatusNtfRecievedFlag();

void clearRfTestContext();
void uwaRfTestDeviceManagementCallback(uint8_t dmEvent,
                                 tUWA_DM_TEST_CBACK_DATA* eventData);
}
#endif