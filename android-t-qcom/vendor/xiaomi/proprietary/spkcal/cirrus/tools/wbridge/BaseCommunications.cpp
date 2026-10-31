////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2015 Cirrus Logic International (UK) Ltd.  All rights reserved.
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
/// @file   BaseCommunications.cpp
/// @brief  Base communication class.
///
/// @version \$Id: BaseCommunications.cpp 16820 2015-01-09 13:47:56Z shackman $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////

//
// Include files
//
#include "BaseCommunications.h"
#ifndef _WIN32
#include <sys/utsname.h>
#include <stdio.h>
#include <string.h>
#endif

#ifdef _WIN32
//
// We have to build on Linux as well, so we can't use the secure versions
// of the CRTL functions.
//
#pragma warning( disable: 4996 )
#endif

//
// Global definitions
//

//
// Class data
//

//
// Function prototypes
//

///////////////////////////////////////////////////////////////////////////////
///
//  Class:  CBaseCommunications
///
/// @brief  Base communication class.
///
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//  Constructor: CBaseCommunications
///
/// @brief   Constructor for CBaseCommunications
///
/// <Detailed description>
/// 
/// @param  
/// (no parameters)
/// 
///////////////////////////////////////////////////////////////////////////////
CBaseCommunications::CBaseCommunications()
{
}

///////////////////////////////////////////////////////////////////////////////
///
//  Destructor: ~CBaseCommunications
///
/// @brief Destructor for CBaseCommunications - cleans up when it is destroyed.
///
/////////////////////////////////////////////////////////////////////////////
CBaseCommunications::~CBaseCommunications()
{
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetOSInfo
///
/// @brief  Gets the os and osversion for the Info command.
///
/// @param  buffer          Buffer to receive the text.
/// @param  bufferLength    Length of the buffer.
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_STRING_ERR  There was a problem manipulating strings.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CBaseCommunications::GetOSInfo( char *buffer, size_t bufferLength )
{
    RESULT          result = WMT_SUCCESS;
    char            osName[MAX_OS_STRING];
    char            osVersion[MAX_OS_STRING];
    int             length = MAX_OS_STRING;
    int             status = 0;
#ifndef _WIN32
    struct utsname  name;
#endif

    memset( osName, 0, length );
    memset( osVersion, 0, length );

#ifdef _WIN32
    OSVERSIONINFOEX osInfo;

    memset( &osInfo, 0, sizeof( OSVERSIONINFOEX ) );
    osInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

    GetVersionEx( (OSVERSIONINFO *) &osInfo );

    // Get the OS name.
    const char *windowsType;
    windowsType =
            ( VER_NT_WORKSTATION == osInfo.wProductType ) ? "Workstation" :
            ( VER_NT_SERVER == osInfo.wProductType ) ? "Server" :
            ( VER_NT_DOMAIN_CONTROLLER == osInfo.wProductType ) ? "Domain controller" :
            "unknown";

    status = snprintf( osName, length, "Windows %s", windowsType );
    if ( WMT_STRING_ERR == status )
        goto error;

    // Get the OS version.
    status = snprintf( osVersion,
                       length,
                       "%d.%d.%d (SP %d.%d)",
                       osInfo.dwMajorVersion,
                       osInfo.dwMinorVersion,
                       osInfo.dwBuildNumber,
                       osInfo.wServicePackMajor,
                       osInfo.wServicePackMinor
                     );
    if ( WMT_STRING_ERR == status )
        goto error;
#else
    status = uname( &name );
    if ( status < 0 )
        goto error;

    status = snprintf( osName, length, "%s", name.sysname );
    if ( WMT_STRING_ERR == status )
        goto error;

    status = snprintf( osVersion, length, "%s", name.release );
    if ( WMT_STRING_ERR == status )
        goto error;
#endif

    status = snprintf( buffer,
                       bufferLength,
                       "os=\"%s\" osversion=\"%s\"",
                       osName,
                       osVersion
                     );
    if ( WMT_STRING_ERR == status )
        goto error;

    return result;

error:
    return WMT_STRING_ERR;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: BlockRead
///
/// @brief  Performs a block read on the device.
///
/// @param  addr    The address to read from.
/// @param  length  The number of bytes to read.
/// @param  data    Receives the data.
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_NO_DEVICE   No device present.
/// @retval WMT_COMM_ERR    There was a comm failure.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CBaseCommunications::BlockRead( unsigned int     addr,
                                       unsigned int     length,
                                       unsigned char    *data
                                     )
{
    RESULT  result = WMT_SUCCESS;
    size_t  dataIndex = 0;

    for ( size_t i = addr; i < addr + (length / 2); i++ )
    {
        unsigned int regval = 0;

        result = ReadRegister( i, &regval );
        if ( WMT_REG_NOT_PRESENT == result )
            result = WMT_SUCCESS;
        else if ( WMT_SUCCESS != result )
            goto done;

        // TODO: Can we work out the number of bits or assume 16 bit?
        data[dataIndex++] = ( (regval & 0xFF00) >> 8 );
        data[dataIndex++] = regval & 0xFF;
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: BlockWrite
///
/// @brief  Performs a block write on the device.
///
/// @param  addr    The address to write to.
/// @param  length  The length of data..
/// @param  data    The data to write.
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_NO_DEVICE   No device present.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CBaseCommunications::BlockWrite( unsigned int    addr,
                                        unsigned int    length,
                                        unsigned char   *data
                                      )
{
    RESULT  result = WMT_SUCCESS;
    size_t  dataIndex = 0;

    for ( size_t i = addr; i < addr + (length / 2); i++ )
    {
        unsigned int regval = 0;

        // TODO: Can we work out the number of bits or assume 16 bit?
        regval = ( data[dataIndex++] << 8 );
        regval += data[dataIndex++];

        result = WriteRegister( i, regval );
        if ( WMT_REG_NOT_PRESENT == result )
            result = WMT_SUCCESS;
        else if ( WMT_SUCCESS != result )
            goto done;
    }

done:
    return result;
}

///////////////////////////////// END OF FILE //////////////////////////////////