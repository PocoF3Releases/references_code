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

#ifndef _UWB_JNI_TYPES_
#define _UWB_JNI_TYPES_

#include <map>
#include <mutex>
#include <deque>
#include <array>
#include <numeric>
#include <map>
#include "uci_defs.h"

/* device status */
typedef enum {
  STATUS_INIT = 0,       /* UWBD is idle */
  STATUS_READY = 1,      /* UWBD is ready to establish UWB session*/
  STATUS_ACTIVE,         /* UWBD is Acive in performing UWB session*/
  STATUS_TIMEOUT = 0xFE, /* error occured in UWBD*/
  STATUS_ERROR
} eDEVICE_STATUS_t;

typedef struct NxpUwbDeviceInfo {
  uint8_t deviceName[UCI_MAX_PAYLOAD_SIZE];
  uint32_t fwVersion;
  uint32_t vendorUciVersion;
  uint8_t chipId[16];
  uint8_t fwBootMode;
  uint32_t vendorFiraUciGenericVersion;
  uint32_t vendorFiraUciTestVersion;
  uint8_t uwbsMaxPpmValue;
} nxpDeviceInfo_t;

typedef struct UwbDeviceInfo {
  uint16_t uciVersion;
  uint16_t macVersion;
  uint16_t phyVersion;
  uint16_t uciTestVersion;
  uint8_t mwVersion[2];
  nxpDeviceInfo_t nxpDeviceInfo;
} deviceInfo_t;



/* Session Data contains M distance samples of N Anchors in order to provide averaged distance for every anchor */
/* N is Maximum Number of Anchors(MAX_NUM_RESPONDERS) */
/* Where M is sampling Rate, the Max value is defined by Service */
typedef struct sessionRangingData {
  uint8_t samplingRate;
  std::array<std::deque<uint32_t>, MAX_NUM_RESPONDERS> anchors;
} SessionRangingData;

#endif