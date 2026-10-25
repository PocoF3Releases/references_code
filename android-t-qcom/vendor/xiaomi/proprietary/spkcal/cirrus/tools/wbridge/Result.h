////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2011-2015 Cirrus Logic International (UK) Ltd.  All rights reserved.
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
/// @file   Result.h
/// @brief  Return value enumerations.
///
/// @version \$Id: Result.h 17751 2015-09-09 14:08:13Z aangus $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
#ifndef __RESULT_H__
#define __RESULT_H__

//
// Include files
//

//
// Definitions
//
enum RESULT
{
    // Success results.
    WMT_SUCCESS             =  0,
    WMT_SOCKET_CLOSED       =  WMT_SUCCESS,     // The socket closed gracefully.

    // Error results.
    WMT_SOCKET_ERR          = -1,               // There was a problem with the socket.
    WMT_STRING_ERR          =  WMT_SOCKET_ERR,  // There was an error manipulating
                                                // strings.
    WMT_COMMS_ERR           =  WMT_STRING_ERR,  // There was an error communicating
                                                // with the device.
    WMT_WINSOCK_ERR         = -2,               // WinSock setup failed.
    WMT_CLOSE_SOCKET        = -3,               // Pseudo-error indicating the socket
                                                // needs to be closed.
    WMT_INVALID_COMMAND     = -30,              // The command was unrecognised.
    WMT_INVALID_PARAMETER   = -35,              // Invalid parameter passed to function.
    WMT_WRITE_FAILED        = -39,              // File write failed
    WMT_READ_FAILED         = -40,              // File read failed
    WMT_END_OF_FILE         = -41,              // End of file
    WMT_NO_DATA             = -42,              // No data available.
    WMT_RESOURCE_FAIL       = -50,              // Couldn't get resources (typically memory).
    WMT_UNSUPPORTED         = -51,              // This operation is not supported.
    WMT_OUT_OF_RANGE        = -53,              // Passed in a value which is out of range.
    WMT_NO_DEVICE           = -54,              // No device present.
    WMT_REG_NOT_PRESENT     = -55,              // Register not present.
    WMT_INVALID_HANDLE      = -60,              // An invalid handle has been passed in
    WMT_DUPLICATED          = -63,              // Data already exists.
    WMT_EXCLUDED            = -67,              // Data excluded.
    WMT_TRUNCATED           = -70,              // The string was truncated
};

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Inline functions
////////////////////////////////////////////////////////////////////////////////



#endif  // __RESULT_H__
///////////////////////////////// END OF FILE //////////////////////////////////