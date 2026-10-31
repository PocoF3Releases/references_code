////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2016 Cirrus Logic International (UK) Ltd.  All rights reserved.
//
// This software as well as any related documentation is furnished under 
// license and may only be used or copied in accordance with the terms of the 
// license. The information in this file is furnished for informational use 
// only, is subject to change without notice, and should not be construed as 
// a commitment by Cirrus Logic International (UK) Ltd.  Cirrus Logic International
// (UK) Ltd assumes no responsibility or liability for any errors or inaccuracies
// that may appear in this document or any software that may be provided in
// association with this document. 
//
// Except as permitted by such license, no part of this document may be 
// reproduced, stored in a retrieval system, or transmitted in any form or by 
// any means without the express written consent of Cirrus Logic International
// (UK) Ltd or affiliated companies.
//
/// @file   QDSPInfo.h
/// @brief  QDSP-specific information.
///
/// @version \$Id: QDSPInfo.h 18539 2016-08-01 10:46:15Z lbogdan $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
#ifndef __QDSPINFO_H__
#define __QDSPINFO_H__

//
// Include files
//

//
// Definitions
//

#define QDSP_NAME           "WMB001"
#define QDSP_DRIVER_FILE    "/dev/CIRRUS_SPEAKER_PROTECTION"
#define QDSP_DATA_SIZE_BITS 32

#define XM_START_REGISTER               0x190000
#define XM_LAST_REGISTER                0x197FFF
#define XM_FIRMWARE_OFFSET              0x00000100
#define XM_INITIAL_ALGO_OFFSET          XM_FIRMWARE_OFFSET + 0x00000100

//
// The DSP XM Header info is as follows:
//
//      DATA_HEADER_COREID
//      DATA_HEADER_COREREVISION
//      DATA_HEADER_FIRMWAREID
//      DATA_HEADER_FIRMWAREREVISION
//      DATA_HEADER_FIRMWAREZMBASE
//      DATA_HEADER_FIRMWAREXMBASE
//      DATA_HEADER_FIRMWAREYMBASE
//      DATA_HEADER_FIRMWAREALGORITHMCOUNT
//      ALGORITHM_HEADER1_ID
//      ALGORITHM_HEADER1_REVISION
//      ALGORITHM_HEADER1_ZMBASE
//      ALGORITHM_HEADER1_XMBASE
//      ALGORITHM_HEADER1_YMBASE
//      ...
//      ALGORITHM_LIST_TERMINATOR
//
#define DSP_HEADER_ALGORITHM_COUNT_IDX  7
#define DSP_FIRST_ALGO_ID_IDX           8
#define DSP_FIRST_ALGO_REVISION_IDX     9
#define DSP_FIRST_ALGO_ZM_BASE_IDX      10
#define DSP_FIRST_ALGO_XM_BASE_IDX      11
#define DSP_FIRST_ALGO_YM_BASE_IDX      12
#define DSP_ALGO_HEADER_NUM_ELEMENTS    5
#define DSP_ALGORITHM_LIST_TERMINATOR   0x00BEDEAD

//
// Comms limits
//
#define MAX_REG_PER_OPERATION 12

// 
// Algorithms to spoof
//
#define COMP                    1
#define QDSP_SPEAKER_PROTECTION 2
#define AUTO_DETECT             3

struct QDSPAlgoInfo
{
    unsigned int    ID;
    unsigned int    revision;
    unsigned int    registerCount;
};

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
///
//  Function: IsQDSPDevice
///
/// @brief  Checks whether the given device is the QDSP device.
///
/// @param  deviceName  Device name to check.
///
/// @return True if the device is the QDSP device; false otherwise.
///
////////////////////////////////////////////////////////////////////////////////
bool IsQDSPDevice( const char *deviceName );

#endif  // __QDSPINFO_H__
///////////////////////////////// END OF FILE //////////////////////////////////