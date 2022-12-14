////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2011-2016 Cirrus Logic International (UK) Ltd.  All rights reserved.
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
/// @file   DeviceCommunication.cpp
/// @brief  Functionality for talking to the device.
///
/// @version \$Id: DeviceCommunication.cpp 18716 2016-09-12 14:41:42Z lbogdan $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////

//
// Include files
//
#include "DeviceCommunication.h"
#include "QDSPIoctl.h"
#include <cstring>
#include <cstdio>

#ifdef _WIN32
//
// We have to build on Linux as well, so we can't use the secure versions
// of the CRTL functions.
//
#pragma warning( disable: 4996 )
#else
#include <sys/mount.h>
#include <ftw.h>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <mntent.h>
#include <sys/utsname.h>
#include <cassert>
#include <sys/resource.h>
#endif

//
// Global definitions
//

// The errno we get when debugfs is already mounted.
#define ERROR_ALREADY_MOUNTED   16

//
// Class data
//

//
// Function prototypes
//

///////////////////////////////////////////////////////////////////////////////
///
//  Class:  CDeviceCommunication
///
/// @brief  Functionality for talking to the device.
///
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//  Constructor: CDeviceCommunication
///
/// @brief   Constructor for CDeviceCommunication
///
/// @param  argc    Argument count.
/// @param  argv    Arguments.
///
///////////////////////////////////////////////////////////////////////////////
CDeviceCommunication::CDeviceCommunication( int argc, char *argv[] )
{
#ifdef _WIN32
    // Do any initialisation here.
    UNREFERENCED_PARAMETER( argc );
    UNREFERENCED_PARAMETER( argv );
#else
    int     index = 1;  // arg0 should be the basename; we don't care about it here.

    PreArgsInit();

    // Was there a device name passed on the command line?
    DebugMessage( "arg count %d\n", argc );
    while ( index < argc )
    {
        const char *arg = argv[index];
        DebugMessage( "arg %d %s\n", index, arg );

        if ( 0 == strncasecmp( arg, "WM", 2 ) )
        {
            HandleDefaultCodecArg( arg );
        }
        else if ( 0 == strcasecmp( arg, "--default" ) )
        {
            index++;
            if ( index < argc )
            {
                arg = argv[index];
                HandleDefaultCodecArg( arg );
            }
        }
        else if ( 0 == strcasecmp( arg, "--max" ) )
        {
            // This is the maximum register index to use.
            index++;
            if ( index < argc )
            {
                arg = argv[index];
                HandleMaxRegAddressArgValue( arg );
            }
        }
        else if (0 == strcasecmp( arg, "-d" ))
        {
            index++;
            if (index < argc)
            {
                arg = argv[index];
                HandleDeviceInclude( arg );
            }
        }
        else if ( 0 == strcasecmp( arg, "-x" ) )
        {
            index++;
            if ( index < argc )
            {
                arg = argv[index];
                HandleDeviceExclude( arg );
            }
        }

        index++;
    }

    // Do base initialisation.
    Initialise();
#endif
}

///////////////////////////////////////////////////////////////////////////////
///
//  Destructor: ~CDeviceCommunication
///
/// @brief Destructor for CDeviceCommunication - cleans up when it is destroyed.
///
///////////////////////////////////////////////////////////////////////////////
CDeviceCommunication::~CDeviceCommunication()
{
    // Do any cleanup here.
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ReadCodecRegister
///
/// @brief  Reads the given register.
///
/// @param  regAddr The register to read.
/// @param  pValue  Receives the value read.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_REG_NOT_PRESENT Register is not present on device.
/// @retval WMT_READ_FAILED     Unable to read the register. Either the device
///                             is in low power mode or the codec file is in an
///                             unrecognised format.
/// @retval WMT_COMMS_ERR       There was a comm failure.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::ReadCodecRegister( unsigned int regAddr, unsigned int *pValue )
{
    RESULT result = WMT_SUCCESS;

#ifdef _WIN32
    UNREFERENCED_PARAMETER( regAddr );
    UNREFERENCED_PARAMETER( pValue );

    result = WMT_NO_DEVICE;
    goto done;
#else
    char                    registerString[32];
    char                    readBuffer[PATH_MAX];
    char                    *valuePos;
    const RegisterLineInfo  *pLineInfo = NULL;
    const char              *path = NULL;

    if ( NoDevice == m_deviceState )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }

    pLineInfo = FindInSections( m_pRegisterSections, regAddr );
    if ( !pLineInfo )
    {
        Log( "R%Xh not present\n", regAddr );
        result = WMT_REG_NOT_PRESENT;
        goto done;
    }

    // Read enough of the codec file for the register line.
    path = GetCodecPath( m_pRegisterSections, regAddr );

    result = ReadFromOffset( path,
                             pLineInfo->offset,
                             pLineInfo->lineLength,
                             readBuffer,
                             sizeof( readBuffer )
                           );
    if ( WMT_SUCCESS != result )
        goto done;

    // Locate the required register in the buffer.
    snprintf( registerString, sizeof( registerString ), "%x:", regAddr );
    valuePos = strstr( readBuffer, registerString );
    if ( !valuePos )
    {
        Log( "R%Xh line not found in buffer as expected\n", regAddr );
        result = WMT_REG_NOT_PRESENT;
        goto done;
    }

    // Return it by value to the caller
    result = ParseReadValue( regAddr, valuePos, pValue );

#endif

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: BlockReadCodec
///
/// @brief  Performs a block read on the device.
///
/// @param  addr    The address to read from.
/// @param  length  The number of bytes to read.
/// @param  data    Receives the data.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_RESOURCE_FAIL   Failed to allocate read buffer.
/// @retval WMT_COMMS_ERR       Failed to read the codec file.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::BlockReadCodec( unsigned int    addr,
                                             unsigned int    length,
                                             unsigned char   *data
                                           )
{
    RESULT  result = WMT_SUCCESS;
    char                    *readBuffer = NULL;

#ifdef _WIN32
    UNREFERENCED_PARAMETER( addr );
    UNREFERENCED_PARAMETER( length );
    UNREFERENCED_PARAMETER( data );

    result = WMT_NO_DEVICE;
    goto done;
#else
    size_t                  dataIndex = 0;
    const RegisterLineInfo  *pLineInfo = NULL;
    unsigned int            startOffset = MAX_REGISTER;
    unsigned int            bytesToRead = 0;
    unsigned int            bufferSize;
    unsigned int            bytesPerRegister = 0;
    unsigned int            addressIncrement = 0;
    const char              *path = NULL;

    if ( NoDevice == m_deviceState )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }

    bytesPerRegister = GetRegisterBytes( m_pRegisterSections, addr );
    addressIncrement = GetAddressIncrement( m_pRegisterSections, addr );
    path = GetCodecPath( m_pRegisterSections, addr );

    //
    // If we don't have a path or know bytes per register or the address
    // increment, nothing to do - we effectively have no device/registers to
    // talk to sensibly.
    //
    if ( NULL == path || 0 == bytesPerRegister || 0 == addressIncrement )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }

    //
    // Work out how much we need to read, and from what offset, from the codec.
    //
    for ( size_t regAddr = addr;
          regAddr < addr + ( ( length / bytesPerRegister ) * addressIncrement );
          regAddr += addressIncrement
        )
    {
        pLineInfo = FindInSections( m_pRegisterSections, regAddr );
        if ( pLineInfo )
        {
            //
            // This register exists - add its register line length to the total
            // bytes to read.
            //
            bytesToRead += pLineInfo->lineLength;

            //
            // If we don't have a startOffset yet, this is the first register
            // that we need that is present in the file - set the start offset
            // accordingly.
            //
            if ( MAX_REGISTER == startOffset )
                startOffset = pLineInfo->offset;
        }
    }

    //
    // Read from the codec. If there's nothing to read (no registers actually
    // present in the codec), we still need to fill the given data buffer, but
    // we don't actually need to read from the codec.
    //
    if ( 0 != bytesToRead && MAX_REGISTER != startOffset )
    {
        //
        // Our base size is bytesToRead. Add one so we include space for
        // the string terminator. We add one again (assuming the count was an
        // accumulation of line lengths for the file) so we have enough space
        // for kernel read requirements to actually get the last line;
        // essentially, it needs an extra +1 byteCount to force the recognition
        // of aligned access by pushing it to the next block offset (the kernel
        // read implementation requires this in order for it to return the last
        // line correctly/at all).
        //
        bufferSize = bytesToRead + 2;
        readBuffer = (char *) malloc( bufferSize );
        if ( !readBuffer )
        {
            result = WMT_RESOURCE_FAIL;
            PrintError( "Allocating memory failed for block read buffer" );
            goto done;
        }

        result = ReadFromOffset( path, startOffset, bytesToRead, readBuffer, bufferSize );
        if ( WMT_SUCCESS != result )
            goto done;
    }

    //
    // Now we run through the readBuffer to fill in the given data buffer. Any
    // registers that aren't in the layout will ignore the readBuffer.
    //
    for ( size_t regAddr = addr;
          regAddr < addr + ( ( length / bytesPerRegister ) * addressIncrement );
          regAddr += addressIncrement
        )
    {
        unsigned int    regval = 0;
        char            registerString[32], *valuePos;

        pLineInfo = FindInSections( m_pRegisterSections, regAddr );
        if ( !pLineInfo || !readBuffer )
        {
            Log( "R%Xh not present\n", regAddr );
            result = WMT_REG_NOT_PRESENT;
        }
        else
        {
            // Locate the required register in the readBuffer.
            snprintf( registerString, sizeof( registerString ), "%x:", (unsigned int) regAddr );
            valuePos = strstr( readBuffer, registerString );
            if ( !valuePos )
            {
                Log( "R%Xh line not found in buffer as expected\n", regAddr );
                result = WMT_REG_NOT_PRESENT;
            }
            else
            {
                result = ParseReadValue( regAddr, valuePos, &regval );
            }
        }

        //
        // We want to continue processing even after we encounter a not-present
        // register or a register that can't be read, as there might still be
        // useful data in the block.
        //
        if ( WMT_REG_NOT_PRESENT == result || WMT_READ_FAILED == result )
            result = WMT_SUCCESS;
        else if ( WMT_SUCCESS != result )
            goto done;

        // We currently support 32-bit, 16-bit, and 8-bit values.
        switch ( bytesPerRegister )
        {
            case 8:
                Log( "64-bit (8-byte) values not yet supported." );
                break;
            case 4:
                data[dataIndex++] = ( regval >> 24 ) & 0xFF;
                data[dataIndex++] = ( regval >> 16 ) & 0xFF;
                // Fall through for the remaining bytes.
            case 2:
                data[dataIndex++] = ( regval >> 8 ) & 0xFF;
                // Fall through for the remaining byte.
            case 1:
                data[dataIndex++] = regval & 0xFF;
                break;
        } // End: switch ( bytesPerRegister )
    }

#endif

done:
    // Clean-up the read buffer.
    if ( readBuffer )
    {
        free( readBuffer );
        readBuffer = NULL;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: WriteCodecRegister
///
/// @brief  Writes the value to the given register.
///
/// @param  regAddr The register to write to.
/// @param  value   The value to write.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_REG_NOT_PRESENT Register is not present on device.
/// @retval WMT_COMMS_ERR       Error writing to codec file.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::WriteCodecRegister( unsigned int regAddr, unsigned int value )
{
    RESULT result = WMT_SUCCESS;

#ifdef _WIN32
    UNREFERENCED_PARAMETER( regAddr );
    UNREFERENCED_PARAMETER( value );

    result = WMT_NO_DEVICE;
    goto done;
#else
    int         file;
    const char  *path = NULL;

    if ( NoDevice == m_deviceState )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }

    // Open the codec_reg file.
    path = GetCodecPath( m_pRegisterSections, regAddr );
    result = OpenCodecFile( path, &file );
    if ( WMT_SUCCESS != result )
        goto done;

    result = WriteToCodecFile( file, regAddr, value );
    TEMP_FAILURE_RETRY( close( file ) );
#endif

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: BlockWriteCodec
///
/// @brief  Performs a block write on the device.
///
/// @param  addr    The address to write to.
/// @param  length  The length of data.
/// @param  data    The data to write.
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_NO_DEVICE   No device present.
/// @retval WMT_COMMS_ERR   Error writing to codec file.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::BlockWriteCodec( unsigned int   addr,
                                              unsigned int   length,
                                              unsigned char  *data
                                            )
{
    RESULT  result = WMT_SUCCESS;

#ifdef _WIN32
    UNREFERENCED_PARAMETER( addr );
    UNREFERENCED_PARAMETER( length );
    UNREFERENCED_PARAMETER( data );

    result = WMT_NO_DEVICE;
    goto done;
#else
    size_t          dataIndex = 0;
    int             file;
    unsigned int    bytesPerRegister = 0;
    unsigned int    addressIncrement = 0;
    const char      *path = NULL;

    if ( NoDevice == m_deviceState )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }

    bytesPerRegister = GetRegisterBytes( m_pRegisterSections, addr );
    addressIncrement = GetAddressIncrement( m_pRegisterSections, addr );
    path = GetCodecPath( m_pRegisterSections, addr );

    //
    // If we don't have a path or know bytes per register or the address
    // increment, nothing to do - we effectively have no device/registers to
    // talk to sensibly.
    //
    if ( NULL == path || 0 == bytesPerRegister || 0 == addressIncrement )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }

    // Open the codec file.
    result = OpenCodecFile( path, &file );
    if ( WMT_SUCCESS != result )
        goto done;

    for ( size_t regAddr = addr;
          regAddr < addr + ( ( length / bytesPerRegister ) * addressIncrement );
          regAddr += addressIncrement
        )
    {
        unsigned int regval = 0;

        // We currently support 32-bit, 16-bit, and 8-bit values.
        switch ( bytesPerRegister )
        {
            case 8:
                Log( "64-bit (8-byte) values not yet supported." );
                break;
            case 4:
                regval += ( data[dataIndex++] << 24 );
                regval += ( data[dataIndex++] << 16 );
                // Fall through for the remaining bytes.
            case 2:
                regval += ( data[dataIndex++] << 8 );
                // Fall through for the remaining byte.
            case 1:
                regval += data[dataIndex++];
                break;
        } // End: switch ( bytesPerRegister )

        result = WriteToCodecFile( file, regAddr, regval );
        if ( WMT_REG_NOT_PRESENT == result )
        {
            result = WMT_SUCCESS;
        }
        else if ( WMT_SUCCESS != result )
        {
            TEMP_FAILURE_RETRY( close( file ) );
            goto done;
        }
    }

    TEMP_FAILURE_RETRY( close( file ) );
#endif

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ReadQDSPAlgoCount
///
/// @brief  Reads the algorithm count from the QDSP driver.
///
/// @returns The number of algorithms on the DSP.
///
////////////////////////////////////////////////////////////////////////////////
size_t CDeviceCommunication::ReadQDSPAlgoCount()
{
    uint32_t count = 0;
#ifdef _WIN32
    goto done;
#else
    int file;

    //
    // Open the device file
    //
    RESULT result = OpenCodecFile( QDSP_DRIVER_FILE, &file );
    if ( WMT_SUCCESS != result )
        goto done;

    //
    // Read our algo count
    //
    CQDSPIoctl::get_rx_alg_count( file, sizeof(count), &count );
    DebugMessage( "ReadQDSPAlgoCount: count = %d\n", count );

    close( file );
#endif

done:
    return count;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ReadQDSPAlgoInfo
///
/// @brief  Reads the info for all algorithms from the QDSP driver.
///
/// @param  algoInfoSize    The size of the algoInfo buffer.
/// @param  algoInfo        The buffer to store the info returned from the driver.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_COMMS_ERR       Error returned from QDSP IOCTL call.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::ReadQDSPAlgoInfo( size_t algoInfoSize, void *algoInfo )
{
    RESULT  result = WMT_SUCCESS;

#ifdef _WIN32
    UNREFERENCED_PARAMETER( algoInfoSize );
    UNREFERENCED_PARAMETER( algoInfo );

    result = WMT_NO_DEVICE;
    goto done;
#else
    int file;

    //
    // Open the device file
    //
    result = OpenCodecFile( QDSP_DRIVER_FILE, &file );
    if ( WMT_SUCCESS != result )
        goto done;

    //
    // Read our algo info
    //
    DebugMessage( "ReadQDSPAlgoInfo: Read algo info. QDSPAlgoInfo buffer size: %u\n", algoInfoSize );
    if ( CQDSPIoctl::get_rx_alg_info( file, algoInfoSize, algoInfo ) < 0 )
    {
        result = WMT_COMMS_ERR;
        goto cleanup;
    }
    DebugMessage( "ReadQDSPAlgoInfo: success\n" );

cleanup:
    close( file );
#endif

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ReadFromQDSPAlgorithm
///
/// @brief  Reads from the offset for the given algorithm.
///
/// Note: in future, bank information may be necessary as well, but for the
/// moment there is only one bank, and said bank is always zero, so there's
/// no actual use of it for accessing the QDSP algorithm just now.
///
/// @param  algoID  The ID of the algorithm to access.
/// @param  offset  The offset within the algorithm to access.
/// @param  pValue  Receives the value read.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_RESOURCE_FAIL   Failed to allocate memory for the QDSP message.
/// @retval WMT_COMMS_ERR       Error returned from QDSP IOCTL call.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::ReadFromQDSPAlgorithm( unsigned int algoID,
                                                    unsigned int offset,
                                                    unsigned int *pValue
                                                  )
{
    RESULT  result = WMT_SUCCESS;

#ifdef _WIN32
    UNREFERENCED_PARAMETER( algoID );
    UNREFERENCED_PARAMETER( offset );
    UNREFERENCED_PARAMETER( pValue );

    result = WMT_NO_DEVICE;
    goto done;
#else
    int                 file;
    int                 message_size = 0;
    struct QDSP_message *pQDSPMessage = NULL;

    //
    // Open the device file
    //
    result = OpenCodecFile( QDSP_DRIVER_FILE, &file );
    if ( WMT_SUCCESS != result )
        goto done;

    //
    // Make IOCTL calls etc.
    //
    message_size = sizeof( struct QDSP_message );
    pQDSPMessage = ( struct QDSP_message * )malloc( message_size );
    if ( NULL == pQDSPMessage )
    {
        DebugMessage( "ReadFromQDSPAlgorithm: Failed to allocate memory.\n" );
        result = WMT_RESOURCE_FAIL;
        goto cleanup;
    }

    //
    // We need to set our register details to read.
    //
    pQDSPMessage->algo_count = 1;
    pQDSPMessage->algorithm_id = algoID;
    pQDSPMessage->bank_offset = offset;
    pQDSPMessage->data_length = 1;       // number of words we want to read.

    DebugMessage( "ReadFromQDSPAlgorithm: Set Register to read. Message Size: %u\n", message_size );
    if ( CQDSPIoctl::set_rx_register_select( file, message_size, pQDSPMessage ) < 0 )
    {
        result = WMT_COMMS_ERR;
        goto cleanup;
    }

    //
    // Now do the read
    //
    DebugMessage( "ReadFromQDSPAlgorithm: Read the register.\n" );
    if ( CQDSPIoctl::get_rx_register_data( file, message_size, pQDSPMessage ) < 0 )
    {
        result = WMT_COMMS_ERR;
        goto cleanup;
    }


    //
    // Copy our value back to the calling function.
    //
    DebugMessage( "ReadFromQDSPAlgorithm: Read the value: %u.\n", pQDSPMessage->data[0] );
    *pValue = pQDSPMessage->data[0];

cleanup:
    if ( NULL != pQDSPMessage )
        free( pQDSPMessage );
    close( file );
#endif

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: BlockReadFromQDSPAlgorithm
///
/// @brief  Reads a number of registers from the offset for the given algorithm.
///
/// Note: in future, bank information may be necessary as well, but for the
/// moment there is only one bank, and said bank is always zero, so there's
/// no actual use of it for accessing the QDSP algorithm just now.
///
/// @param  algoID          The ID of the algorithm to access.
/// @param  offset          The offset within the algorithm to access.
/// @param  numRegsToRead   The number of registers to read.
/// @param  pValues         Receives the values read.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_RESOURCE_FAIL   Failed to allocate memory for the QDSP message.
/// @retval WMT_COMMS_ERR       Error returned from QDSP IOCTL call.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::BlockReadFromQDSPAlgorithm( unsigned int algoID,
                                                         unsigned int offset,
                                                         unsigned int numRegsToRead,
                                                         unsigned int *pValues
                                                       )
{
    RESULT  result = WMT_SUCCESS;

#ifdef _WIN32
    UNREFERENCED_PARAMETER( algoID );
    UNREFERENCED_PARAMETER( offset );
    UNREFERENCED_PARAMETER( numRegsToRead );
    UNREFERENCED_PARAMETER( pValues );

    result = WMT_NO_DEVICE;
    goto done;
#else
    int                 file;
    int                 message_size = 0;
    struct QDSP_message *pQDSPMessage = NULL;

    //
    // Open the device file
    //
    result = OpenCodecFile( QDSP_DRIVER_FILE, &file );
    if ( WMT_SUCCESS != result )
        goto done;

    //
    // Make IOCTL calls etc.
    //
    message_size = sizeof( struct QDSP_message ) + ( ( numRegsToRead - 1 ) * sizeof( uint32_t ) );
    pQDSPMessage = ( struct QDSP_message * )malloc( message_size );
    if ( NULL == pQDSPMessage )
    {
        DebugMessage( "BlockReadFromQDSPAlgorithm: Failed to allocate memory.\n" );
        result = WMT_RESOURCE_FAIL;
        goto cleanup;
    }

    //
    // We need to set our register details to read.
    //
    pQDSPMessage->algo_count = 1;
    pQDSPMessage->algorithm_id = algoID;
    pQDSPMessage->bank_offset = offset;
    pQDSPMessage->data_length = numRegsToRead;       // number of words we want to read.

    DebugMessage( "BlockReadFromQDSPAlgorithm: Set Registers to read. Message Size: %u\n", message_size );
    if ( CQDSPIoctl::set_rx_register_select( file, message_size, pQDSPMessage ) < 0 )
    {
        result = WMT_COMMS_ERR;
        goto cleanup;
    }

    //
    // Now do the read
    //
    DebugMessage( "BlockReadFromQDSPAlgorithm: Read the registers.\n" );
    if ( CQDSPIoctl::get_rx_register_data( file, message_size, pQDSPMessage ) < 0 )
    {
        result = WMT_COMMS_ERR;
        goto cleanup;
    }

    //
    // Copy our value back to the calling function.
    //
    for ( size_t i = 0; i < numRegsToRead; i++ )
    {
        DebugMessage( "BlockReadFromQDSPAlgorithm: Read the value[%u]: %u.\n", i, pQDSPMessage->data[i] );
        *pValues = ByteSwap32bit( pQDSPMessage->data[i] );
        pValues++;
    }

cleanup:
    if ( NULL != pQDSPMessage )
        free( pQDSPMessage );
    close( file );
#endif

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: WriteToQDSPAlgorithm
///
/// @brief  Writes the value to the offset for the given algorithm.
///
/// Note: in future, bank information may be necessary as well, but for the
/// moment there is only one bank, and said bank is always zero, so there's
/// no actual use of it for accessing the QDSP algorithm just now.
///
/// @param  algoID  The ID of the algorithm to access.
/// @param  offset  The offset within the algorithm to access.
/// @param  value   The value to write.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_RESOURCE_FAIL   Failed to allocate memory for the QDSP message.
/// @retval WMT_COMMS_ERR       Error returned from QDSP IOCTL call.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::WriteToQDSPAlgorithm( unsigned int algoID,
                                                   unsigned int offset,
                                                   unsigned int value
                                                 )
{
    RESULT  result = WMT_SUCCESS;

#ifdef _WIN32
    UNREFERENCED_PARAMETER( algoID );
    UNREFERENCED_PARAMETER( offset );
    UNREFERENCED_PARAMETER( value );

    result = WMT_NO_DEVICE;
    goto done;
#else
    int                 file;
    int                 message_size = 0;
    struct QDSP_message *pQDSPMessage = NULL;

    //
    // Open the device file
    //
    result = OpenCodecFile( QDSP_DRIVER_FILE, &file );
    if ( WMT_SUCCESS != result )
        goto done;

    //
    // Make IOCTL calls etc.
    //
    message_size = sizeof( struct QDSP_message );
    pQDSPMessage = (struct QDSP_message *)malloc( message_size );
    if ( NULL == pQDSPMessage )
    {
        DebugMessage( "WriteToQDSPAlgorithm: Failed to allocate memory.\n" );
        result = WMT_RESOURCE_FAIL;
        goto cleanup;
    }

    //
    // We need to set our register details to write.
    //
    pQDSPMessage->algo_count = 1;
    pQDSPMessage->algorithm_id = algoID;
    pQDSPMessage->bank_offset = offset;
    pQDSPMessage->data_length = 1;
    pQDSPMessage->data[0] = value;

    DebugMessage( "WriteToQDSPAlgorithm: Write the register.\n" );
    if ( CQDSPIoctl::set_rx_register_data( file, message_size, pQDSPMessage ) < 0 )
    {
        result = WMT_COMMS_ERR;
        goto cleanup;
    }

cleanup:
    if ( NULL != pQDSPMessage )
        free( pQDSPMessage );
    close( file );
#endif

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: BlockWriteToQDSPAlgorithm
///
/// @brief  Writes the values to the offset for the given algorithm.
///
/// Note: in future, bank information may be necessary as well, but for the
/// moment there is only one bank, and said bank is always zero, so there's
/// no actual use of it for accessing the QDSP algorithm just now.
///
/// @param  algoID          The ID of the algorithm to access.
/// @param  offset          The offset within the algorithm to access.
/// @param  numRegsToWrite  The number of registers to write.
/// @param  pValues         The values to write.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_RESOURCE_FAIL   Failed to allocate memory for the QDSP message.
/// @retval WMT_COMMS_ERR       Error returned from QDSP IOCTL call.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::BlockWriteToQDSPAlgorithm( unsigned int algoID,
                                                        unsigned int offset,
                                                        unsigned int numRegsToWrite,
                                                        unsigned int *pValues
                                                      )
{
    RESULT  result = WMT_SUCCESS;

#ifdef _WIN32
    UNREFERENCED_PARAMETER( algoID );
    UNREFERENCED_PARAMETER( offset );
    UNREFERENCED_PARAMETER( numRegsToWrite );
    UNREFERENCED_PARAMETER( pValues );

    result = WMT_NO_DEVICE;
    goto done;
#else
    int                 file;
    int                 message_size = 0;
    struct QDSP_message *pQDSPMessage = NULL;

    //
    // Open the device file
    //
    result = OpenCodecFile( QDSP_DRIVER_FILE, &file );
    if ( WMT_SUCCESS != result )
        goto done;

    //
    // Make IOCTL calls etc.
    //
    message_size = sizeof( struct QDSP_message ) + ( sizeof( uint32_t ) * numRegsToWrite );
    pQDSPMessage = ( struct QDSP_message * )malloc( message_size );
    if ( NULL == pQDSPMessage )
    {
        DebugMessage( "BlockWriteToQDSPAlgorithm: Failed to allocate memory.\n" );
        result = WMT_RESOURCE_FAIL;
        goto cleanup;
    }

    //
    // We need to set our register details to write.
    //
    pQDSPMessage->algo_count = 1;
    pQDSPMessage->algorithm_id = algoID;
    pQDSPMessage->bank_offset = offset;
    pQDSPMessage->data_length = numRegsToWrite;

    for ( size_t i = 0; i < numRegsToWrite; i++ )
    {
        pQDSPMessage->data[i] = ByteSwap32bit( *pValues );
        pValues++;
    }

    DebugMessage( "BlockWriteToQDSPAlgorithm: Write the registers.\n" );
    if ( CQDSPIoctl::set_rx_register_data( file, message_size, pQDSPMessage ) < 0 )
    {
        result = WMT_COMMS_ERR;
        goto cleanup;
    }

cleanup:
    if ( NULL != pQDSPMessage )
        free( pQDSPMessage );
    close( file );
#endif

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetDebugFsMountPoint
///
/// @brief  Finds the debugfs mount point.
///
/// (no parameters)
///
/// @retval WMT_SUCCESS     Found the mount point.
/// @retval WMT_NO_DEVICE   Couldn't find mount point.
/// @retval WMT_TRUNCATED   Mount point path was truncated.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::GetDebugFsMountPoint()
{
#ifdef _WIN32
    return WMT_NO_DEVICE;
#else
    RESULT          result = WMT_NO_DEVICE;
    FILE            *pMountFile;
    struct mntent   *entry;

    // Attempt to get a list of mount points.
    pMountFile = fopen( "/etc/mtab", "r" );
    if ( pMountFile )
        goto out;
    pMountFile = fopen( "/proc/mounts", "r" );
    if ( pMountFile )
        goto out;

    // Attempt to mount procfs and see what we can do.
    if ( mount( "none", "/proc", "procfs", 0, NULL ) < 0 )
    {
        PrintError( "Mounting procfs failed" );
        goto done;
    }
    pMountFile = fopen( "/proc/mounts", "r" );
    if ( !pMountFile )
    {
        Log( "Can't get a list of mount points.\n" );
        goto done;
    }

out:
    // Ensure that debugfs is mounted.
    while ( ( entry = getmntent( pMountFile ) ) )
    {
        if ( !strcmp( "debugfs", entry->mnt_fsname ) ||
             strstr( entry->mnt_fsname, "debug" )
           )
        {
            result = WMT_SUCCESS;

            if ( strlen( entry->mnt_dir ) >= sizeof( m_debugfsMountPoint ) )
            {
                //
                // We wont be able to fit the whole path in m_debugfsMountPoint.
                // This shouldn't happen though.
                //
                result = WMT_TRUNCATED;
                break;
            }

            // Get the path of where debugfs is mounted.
            Log( "\nFound the debugfs path! %s \n", entry->mnt_dir );
            strncpy( m_debugfsMountPoint, entry->mnt_dir, sizeof( m_debugfsMountPoint ) );
            m_debugfsMountPoint[sizeof( m_debugfsMountPoint ) - 1] = '\0';

            break;
        }
    }
    fclose( pMountFile );

done:
    if ( WMT_SUCCESS != result )
    {
        Log( "Debugfs is not mounted.\n" );
        *m_debugfsMountPoint = '\0';
    }

    return result;
#endif
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: MountDebugFs
///
/// @brief  Attempts to mount the debugfs filesystem.
///
/// (no parameters)
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_NO_DEVICE   Failed to mount DebugFs.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::MountDebugFs()
{
#ifdef _WIN32
    return WMT_NO_DEVICE;
#else
    RESULT  result = WMT_SUCCESS;
    int     retval = 0;

    Log( "Attempting to mount debugfs at /sys/kernel/debug...\n" );
    retval = mount( "none", "/sys/kernel/debug", "debugfs", 0, NULL );
    if ( retval < 0 && ERROR_ALREADY_MOUNTED != errno )
    {
        Log( "Failed to mount debugfs: mount returned %d\n", retval );
        PrintError( "Mounting debugfs failed." );
        result = WMT_NO_DEVICE;
        goto done;
    }

    Log( "Successfully mounted debugfs!\n" );
    snprintf( m_debugfsMountPoint, sizeof( m_debugfsMountPoint ), "/sys/kernel/debug" );

done:
    return result;
#endif
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: SearchForCodecs
///
/// @brief  Locates all codec files and fills m_codecPaths with them.
///
/// (no parameters)
///
/// @return void
///
////////////////////////////////////////////////////////////////////////////////
void CDeviceCommunication::SearchForCodecs()
{
#ifdef _WIN32
    return;
#else
    char searchPath[PATH_MAX];

    //
    // First check for our QDSP driver.
    //
    if ( ftw( QDSP_DRIVER_FILE, ftw_callback, 64 ) < 0 )
    {
        PrintError( "ftw failed" );
        //
        // Don't goto done in this case - if we don't have the QDSP_DRIVER_FILE,
        // we still have the m_debugfsMountPoint locations to try.
        //
    }

    //
    // Then see if we can find any codecs.
    //

    // First try the location for registers files.
    snprintf( searchPath, PATH_MAX, "%s/%s", m_debugfsMountPoint, "regmap" );
    if ( ftw( searchPath, ftw_callback, 64 ) < 0 )
    {
        PrintError( "ftw failed" );
        goto done;
    }

    // If we still don't have any, try the location for codec_reg files.
    if ( !m_codecPaths.size() )
    {
        snprintf( searchPath, PATH_MAX, "%s/%s", m_debugfsMountPoint, "asoc" );
        if ( ftw( searchPath, ftw_callback, 64 ) < 0 )
        {
            PrintError( "ftw failed" );
            goto done;
        }
    }

done:
    return;
#endif
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: InitialiseLayoutWithRanges
///
/// @brief  Attempts to initialise the register layout with the register ranges file.
///
/// @param  pSection    The section to initialise.
///
/// @retval WMT_SUCCESS     Register layout initialised successfully.
/// @retval WMT_NO_DEVICE   No device present.
/// @retval WMT_NO_DATA     No range file available to use.
/// @retval WMT_READ_FAILED Unable to parse a line from the range file.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::InitialiseLayoutWithRanges( RegisterSection *pSection )
{
#ifdef _WIN32
    UNREFERENCED_PARAMETER( pSection );

    return WMT_NO_DEVICE;
#else
    RESULT                      result = WMT_SUCCESS;
    FILE                        *pFile;
    char                        rangeFile[PATH_MAX];
    char                        lineBuffer[PATH_MAX];
    char                        *pLine, *pNextLine;
    size_t                      totalOffset = 0;
    size_t                      lineNumber = 0;
    size_t                      lineLength = 0;
    size_t                      bytesPerRegister = 0;
    size_t                      addressIncrement = 0;
    std::vector<unsigned int>   readAddresses;

    // We must have a device to have a codec file.
    if ( NoDevice == m_deviceState )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }

    // Get the path of the range file.
    result = GetRangeFilePath( pSection, rangeFile, sizeof( rangeFile ) );
    if ( WMT_SUCCESS != result )
        goto done;

    // Open the range file.
    pFile = fopen( rangeFile, "r+" );
    if ( !pFile )
    {
        PrintError( "Failed to open range file" );
        result = WMT_NO_DATA;
        goto done;
    }

    //
    // If we get here, we have a range file. We should read enough from the
    // codec file now so we have our initialisation information. Read a small
    // amount into our buffer instead of using fgets, it will save time for
    // accessing only the first few lines out of the file (PATH_MAX should
    // be large enough to get us at least a couple of lines from the codec file).
    //
    Log( "Getting info from first register lines.\n" );
    result = ReadFromOffset( pSection->regPath,
                             0,
                             PATH_MAX - 2, // See @note for ReadFromOffset: byteCount + 2 must be <= bufferSize.
                             lineBuffer,
                             sizeof( lineBuffer )
                           );
    if ( WMT_SUCCESS != result )
    {
        if ( WMT_TRUNCATED == result )
        {
            // Make a note of the truncation, but continue anyway.
            Log( "Attempting to read from truncated codec file data.\n" );
            result = WMT_SUCCESS;
        }
        else
        {
            Log( "Failed to read from codec file.\n" );
            fclose( pFile );
            goto done;
        }
    }

    //
    // Get the information we need from the register lines.
    //
    pLine = lineBuffer;
    while ( pLine )
    {
        pNextLine = strchr( pLine, '\n' );
        if ( pNextLine )
        {
            *pNextLine = '\0';
            pNextLine++;
        }

        if ( WMT_SUCCESS == HandleRegisterLineForRanges( pLine,
                                                         strlen( pLine ),
                                                         pSection,
                                                         &lineLength,
                                                         &bytesPerRegister,
                                                         &addressIncrement,
                                                         &readAddresses
                                                       )
           )
        {
            break;
        }

        // Now get the next line.
        pLine = pNextLine;
    }

    Log( "Using range file \"%s\"\n", rangeFile );

    //
    // Go through each line from the range file.
    //
    while ( fgets( lineBuffer, sizeof( lineBuffer ), pFile ) )
    {
        result = HandleRangeFileLine( lineBuffer,
                                      pSection,
                                      &totalOffset,
                                      lineLength,
                                      &lineNumber,
                                      &addressIncrement,
                                      &readAddresses
                                    );
        if ( WMT_OUT_OF_RANGE == result )
        {
            // Hit max address - skip any further range file lines.
            result = WMT_SUCCESS;
            break;
        }
        else if ( WMT_SUCCESS != result )
        {
            fclose( pFile );
            goto done;
        }
    }

    fclose( pFile );

    EnsureAddressIncrementSet( pSection, addressIncrement, &readAddresses );

done:
    if ( WMT_SUCCESS != result )
        Log( "Unable to use range file for initialisation.\n" );

    return result;
#endif
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: InitialiseLayoutFromRegisters
///
/// @brief  Attempts to initialise the register layout from the registers/codec_reg file.
///
/// @param  pSection    The section to initialise.
///
/// @retval WMT_SUCCESS     Register layout initialised successfully.
/// @retval WMT_NO_DEVICE   Register layout initialisation failed - no device.
/// @retval WMT_READ_FAILED Register layout initialisation failed - no device data.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::InitialiseLayoutFromRegisters( RegisterSection *pSection )
{
#ifdef _WIN32
    UNREFERENCED_PARAMETER( pSection );

    return WMT_NO_DEVICE;
#else
    RESULT          result = WMT_NO_DEVICE;
    FILE            *pFile;
    char            lineBuffer[PATH_MAX];
    size_t          totalLength = 0;
    size_t          lineNumber = 0;
    size_t          bytesPerRegister = 0;

    // We must have a device to have a codec file.
    if ( NoDevice == m_deviceState )
    {
        goto done;
    }

    // Open the codec file.
    pFile = fopen( pSection->regPath, "r+" );
    if ( !pFile )
    {
        ErrorMessage( "Failed to open codec file: %s.\nReason: %s\n", pSection->regPath, strerror( errno ) );
        result = WMT_READ_FAILED;
        goto done;
    }

    //
    // Read lines from the file until we reach the end of the file (or maximum
    // register), processing the lines as we go.
    //
    lineNumber = 0;
    totalLength = 0;
    while ( fgets( lineBuffer, sizeof( lineBuffer ), pFile ) )
    {
        RESULT localResult = HandleRegisterFileLine( lineBuffer,
                                                     pSection,
                                                     &totalLength,
                                                     strlen( lineBuffer ), // Note: the line from fgets will include the '\n' character, if any.
                                                     &lineNumber,
                                                     &bytesPerRegister
                                                   );
        if ( WMT_OUT_OF_RANGE == localResult )
        {
            // We've passed the maximum register we care about; ignore the rest.
            break;
        }
    }

    result = WMT_SUCCESS;
    fclose( pFile );

done:
    return result;
#endif
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetRegisterMap
///
/// @brief  Returns in a vector all the register -> value pairs.
///
/// @param  regvals  Receives the register value pairs.
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_NO_DEVICE   Reading register map failed - no device present.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::GetRegisterMap( CRegValuePairs &regvals )
{
#ifdef _WIN32
    UNREFERENCED_PARAMETER( regvals );

    return WMT_NO_DEVICE;
#else
    RESULT          result = WMT_SUCCESS;
    FILE            *pFile;
    char            lineBuffer[PATH_MAX];
    size_t          lineNumber = 0;
    const char      *path = NULL;

    if ( NoDevice == m_deviceState )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }

    // Open the codec file.
    path = GetCodecPath( m_pRegisterSections, 0 );
    pFile = fopen( path, "r+" );
    if ( !pFile )
    {
        PrintError( "Failed to open codec file" );
        result = WMT_NO_DEVICE;
        goto done;
    }

    //
    // Read lines from the file until we reach the end of the file (or maximum
    // register), processing the lines as we go. We return by reference a vector
    // that contains the register -> value pairs for all available registers.
    //
    while ( fgets( lineBuffer, sizeof( lineBuffer ), pFile ) )
    {
        if ( WMT_OUT_OF_RANGE == HandleGetRegisterMapLine( lineBuffer, &lineNumber, regvals ) )
            break;
    }

    Log( "Found %d register-value pairs (last pair: %s: %s).\n",
         regvals.size(),
         regvals.back().reg,
         regvals.back().value
       );
    fclose( pFile );

    result = WMT_SUCCESS;

done:
    return result;
#endif
}

#ifndef _WIN32
////////////////////////////////////////////////////////////////////////////////
///
//  Function: ftw_callback
///
/// @brief  Locates all loaded Cirrus codecs.
///
/// This function is called for every file and directory under the debugfs
/// mount point.  Its purpose is to locate all the Cirrus codecs currently
/// loaded on the system and load them onto a vector.
///
/// @param  fpath       Filepath.
/// @param  sb          Information about the file in the filepath.
/// @param  typeflag    The file type information
///
/// @return WMT_SUCCESS Success.
///
////////////////////////////////////////////////////////////////////////////////
int CDeviceCommunication::ftw_callback( const char          *fpath,
                                        const struct stat   *sb,
                                        int                 typeflag
                                      )
{
    char                        *codec, *p;
    char                        tmpbuf[PATH_MAX];
    int                         fd;
    ssize_t                     ret;
    size_t                      bytes_read;
    ssize_t                     bytes_left;
    const char                  *qdsp_file_p;
    const char                  *codec_reg_p;
    const char                  *registers_p;
    bool                        matchedInclude = false;
    std::vector<std::string>    *pIncludeDevices = &m_includeDevices;
    std::vector<std::string>    *pExcludeDevices = &m_excludeDevices;

    if (sb == NULL)
    {
        InfoMessage( "Ignoring, no error here \n");
    }
    //
    // We only care about files.
    //
    if ( typeflag == FTW_F )
    {
        qdsp_file_p = strstr( fpath, QDSP_DRIVER_FILE );
        codec_reg_p = strstr( fpath, "codec_reg" );
        registers_p = strstr( fpath, "registers" );
        if ( qdsp_file_p || codec_reg_p || registers_p )
        {
            snprintf( tmpbuf, sizeof( tmpbuf ), "%s", fpath );

            //
            // ## TODO:
            // Refactor this and the equivalent in WISCEBridge/DeviceCommunication.cpp
            // to pull out the common code (which is most of it).
            //

            //
            // Check whether the codec filename is on the included or excluded lists
            // - reject or continue accordingly.
            //
            // If we have excluded devices, and one of them matches the filename,
            // ignore this file.
            //
            if ( m_excludeDevices.size() > 0 )
            {
                for ( size_t i = 0; i < m_excludeDevices.size(); i++ )
                {
                    if ( 0 != strstr( tmpbuf, m_excludeDevices[i].c_str() ) )
                    {
                        // It matched. Exclude it.
                        InfoMessage( "Ignoring %s codec because it matches %s\n",
                                     tmpbuf,
                                     m_excludeDevices[i].c_str()
                                   );
                        goto done;
                    }
                }
            }

            //
            // If we have included devices, ignore this file unless one of them
            // matches the filename.
            //
            if ( m_includeDevices.size() > 0 )
            {
                for ( size_t i = 0; i < m_includeDevices.size(); i++ )
                {
                    if ( 0 != strstr( tmpbuf, m_includeDevices[i].c_str() ) )
                    {
                        matchedInclude = true;
                        break;
                    }
                }

                if ( !matchedInclude )
                {
                    // Nothing matched. Exclude it.
                    InfoMessage( "Ignoring %s codec because it didn't match\n",
                                 tmpbuf
                               );
                    goto done;
                }
            }

            if ( qdsp_file_p )
            {
                snprintf( tmpbuf, sizeof( tmpbuf ), "%s", QDSP_NAME );
                codec = tmpbuf;
            }
            else if ( registers_p )
            {
                //
                // For regmap we need to read the file `name' under the
                // same directory as the file `registers'.
                //
                // If some other debugfs dir has the `registers' file then
                // ignore it.
                //
                if ( !strstr( tmpbuf, "regmap" ) )
                    goto done;

                p = strrchr( tmpbuf, '/' );
                if ( !p )
                    goto done;
                p++;
                if ( !*p )
                    goto done;

                //
                // We know this fits because we're just replacing 'registers'
                // with the shorter 'name' filename.
                //
                strcpy( p, "name" );

                // Open the name file and read its contents.
                fd = open( tmpbuf, O_RDONLY );
                if ( fd < 0 )
                {
                    // Ignore this error, skip to the next file.
                    goto done;
                }

                memset( tmpbuf, 0, sizeof( tmpbuf ) );
                bytes_left = sizeof( tmpbuf );
                bytes_read = 0;
                do
                {
                    ret = read( fd, tmpbuf + bytes_read, bytes_left );
                    if ( ret < 0 )
                    {
                        // Ignore this error, skip to the next file.
                        TEMP_FAILURE_RETRY( close( fd ) );
                        goto done;
                    }
                    else if ( !ret )
                    {
                        break;
                    }

                    bytes_read += ret;
                    bytes_left -= ret;
                } while ( bytes_left > 0 );

                if ( !bytes_read )
                {
                    // Got EOF too early, ignore and skip to the next file.
                    TEMP_FAILURE_RETRY( close( fd ) );
                    goto done;
                }

                tmpbuf[sizeof( tmpbuf ) - 1] = '\0';
                codec = tmpbuf;
                TEMP_FAILURE_RETRY( close( fd ) );

                // Just for cs35l41 amp
                if(strncmp(codec, "cs35l41", strlen("cs35l41")))
                    goto done;

                InfoMessage( "ftw_callback :Just for %s ONLY\n", codec);
            }
            else
            {
                RESULT result = ParseWMCodec( fpath, &*tmpbuf, sizeof( tmpbuf ) );
                if ( WMT_SUCCESS != result )
                    goto done;

                codec = tmpbuf;
            }

            //
            // If we've already successfully checked it should be included, don't check again.
            //
            if ( matchedInclude )
                pIncludeDevices = NULL;

            StoreCodec( &m_codecPaths, codec, fpath, pExcludeDevices, pIncludeDevices );
        }
    }

done:
    return WMT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: OpenCodecFile
///
/// @brief  Opens the codec file with read/write access.
///
/// @param  path    Path to the codec file to open.
/// @param  pFile   Receives the file descriptor for the codec file.
///
/// @retval WMT_SUCCESS             Successfully opened the codec file.
/// @retval WMT_NO_DEVICE           No device present.
/// @retval WMT_INVALID_PARAMETER   pFile was NULL.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::OpenCodecFile( const char *path, int *pFile )
{
    RESULT result = WMT_SUCCESS;

    if ( NoDevice == m_deviceState )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }

    if ( !pFile )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    // Open the codec_reg file.
    *pFile = open( path, O_RDWR );
    if ( *pFile < 0 )
    {
        Log( "Failed to open codec file: %s.\nReason: %s\n", path, strerror( errno ) );
        result = WMT_NO_DEVICE;
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ReadFromOffset
///
/// @brief  Reads from the codec file at the given offset into the given buffer.
///
/// @note   Data will be truncated if (byteCount + 2) > bufferSize. The +2
///         covers:
///          - Adding one so we include space for the string terminator.
///          - Adding one again (assuming the count was an accumulation of line
///            lengths for the file) to actually get the last line; essentially,
///            it forces the recognition of aligned access by pushing it to the
///            next block offset (the kernel read implementation requires this
///            in order for it to return the last line correctly/at all).
///
/// @param  path            Path to the codec file to read from.
/// @param  offset          Byte offset to start reading from.
/// @param  byteCount       The count of bytes to read.
/// @param  buffer          The buffer to read into.
/// @param  bufferSize      The size of the buffer.
/// @param  pBytesParsed    Receives the number of bytes parsed (optional).
///
/// @retval WMT_SUCCESS             Successfully read from the codec file.
/// @retval WMT_TRUNCATED           Successfully read from the codec file, but
///                                 the given buffer was not large enough for
///                                 the requested count of bytes to read - data
///                                 has been truncated.
/// @retval WMT_NO_DEVICE           No device present.
/// @retval WMT_INVALID_PARAMETER   Buffer was NULL.
/// @retval WMT_COMMS_ERR           Failed to read from the codec file.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::ReadFromOffset( const char *path,
                                             size_t     offset,
                                             size_t     byteCount,
                                             char       *buffer,
                                             size_t     bufferSize,
                                             size_t     *pBytesParsed /* = NULL */
                                           )
{
    RESULT  result = WMT_SUCCESS;
    int     file;

    if ( NoDevice == m_deviceState )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }

    // Open the codec_reg file.
    result = OpenCodecFile( path, &file );
    if ( WMT_SUCCESS != result )
        goto done;

    // Read from the given offset in the file.
    result = ReadFromOffset( file,
                             offset,
                             byteCount,
                             buffer,
                             bufferSize,
                             pBytesParsed
                           );

    // Close the file now we're done with it.
    TEMP_FAILURE_RETRY( close( file ) );

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ReadFromOffset
///
/// @brief  Reads from the codec file at the given offset into the given buffer.
///
/// @note   Data will be truncated if (byteCount + 2) > bufferSize. The +2
///         covers:
///          - Adding one so we include space for the string terminator.
///          - Adding one again (assuming the count was an accumulation of line
///            lengths for the file) to actually get the last line; essentially,
///            it forces the recognition of aligned access by pushing it to the
///            next block offset (the kernel read implementation requires this
///            in order for it to return the last line correctly/at all).
///
/// @param  codecFile       File descriptor for the codec file to read from.
/// @param  offset          Byte offset to start reading from.
/// @param  byteCount       The count of bytes to read.
/// @param  buffer          The buffer to read into.
/// @param  bufferSize      The size of the buffer.
/// @param  pBytesParsed    Receives the number of bytes parsed (optional).
///
/// @retval WMT_SUCCESS             Successfully read from the codec file.
/// @retval WMT_TRUNCATED           Successfully read from the codec file, but
///                                 the given buffer was not large enough for
///                                 the requested count of bytes to read - data
///                                 has been truncated.
/// @retval WMT_NO_DEVICE           No device present.
/// @retval WMT_INVALID_PARAMETER   Buffer was NULL.
/// @retval WMT_COMMS_ERR           Failed to read from the codec file.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::ReadFromOffset( int    codecFile,
                                             size_t offset,
                                             size_t byteCount,
                                             char   *buffer,
                                             size_t bufferSize,
                                             size_t *pBytesParsed /* = NULL */
                                           )
{
    RESULT  result = WMT_SUCCESS;
    int     bytesRead;
    size_t  bytesParsed = 0;
    size_t  bytesRemaining;
    off_t   offsetLocation = 0;

    if ( codecFile < 0 )
    {
        result = WMT_NO_DEVICE;
        Log( "Invalid codec file descriptor; unable to read.\n" );
        goto done;
    }

    if ( !buffer )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    // Seek to the location in the file.
    offsetLocation = lseek( codecFile, offset, SEEK_SET );
    if ( -1 == offsetLocation )
    {
        PrintError( "Failed to lseek" );
        result = WMT_COMMS_ERR;
        goto done;
    }

    //
    // Read byteCount bytes into the buffer. Add one so we include space for
    // the string terminator. We add one again (assuming the count was an
    // accumulation of line lengths for the file) to actually get the last line;
    // essentially, it forces the recognition of aligned access by pushing it
    // to the next block offset (the kernel read implementation requires this
    // in order for it to return the last line correctly/at all).
    //
    bytesRemaining = byteCount + 2;

    // If bytesRemaining won't fit in the buffer, we truncate.
    if ( bytesRemaining > bufferSize )
    {
        bytesRemaining = bufferSize - 1;
        result = WMT_TRUNCATED;
        DebugMessage( "ReadFromOffset truncated: requested %d bytes; using %d bytes.\n",
                      byteCount,
                      bytesRemaining
                    );
    }

    do
    {
        bytesRead = read( codecFile, buffer + bytesParsed, bytesRemaining );
        if ( 0 == bytesRead )
            break;

        if ( bytesRead < 0 )
        {
            if ( EINTR == errno )
                continue;

            PrintError( "Reading from codec failed" );
            result = WMT_COMMS_ERR;
            goto done;
        }

        bytesParsed += bytesRead;
        bytesRemaining -= bytesRead;
    }
    while ( bytesRemaining > 0 );

    // Ensure the buffer is line-terminated where it should be.
    if ( bufferSize > bytesParsed )
    {
        buffer[bytesParsed] = '\0';
    }

done:
    if ( pBytesParsed )
        *pBytesParsed = bytesParsed;

    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: WriteToCodecFile
///
/// @brief  Writes the value for the given register to the codec file.
///
/// @note   This does not handle the opening or closing of the file - just
///         writing to it.
///
/// @param  codecFile   File descriptor for the codec file to write to.
/// @param  regAddr     The register to write to.
/// @param  value       The value to write.
///
/// @retval WMT_SUCCESS     Successfully wrote to the codec file.
/// @retval WMT_NO_DEVICE   Invalid codec file descriptor.
/// @retval WMT_COMMS_ERR   Error writing to codec file.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CDeviceCommunication::WriteToCodecFile( int          codecFile,
                                               unsigned int regAddr,
                                               unsigned int value
                                             )
{
    RESULT  result = WMT_SUCCESS;
    char    request[64];
    ssize_t status;

    if ( codecFile < 0 )
    {
        result = WMT_NO_DEVICE;
        Log( "Invalid codec file descriptor; unable to write.\n" );
        goto done;
    }

    // Prepare the request.
    snprintf( request, sizeof( request ), "%x %x\n", regAddr, value );

    // Write out the request.
    status = write( codecFile, request, strlen( request ) );
    if ( status != (ssize_t) strlen( request ) )
    {
        result = WMT_COMMS_ERR;
        if ( -1 == status )
        {
            PrintError( "Error writing to codec file" );
        }
        else
        {
            DebugMessage( "Error writing to codec file: wrote %d bytes, expected to write %d bytes\n",
                          status,
                          strlen( request )
                        );
        }
        goto done;
    }

done:
    return result;
}

#endif

///////////////////////////////// END OF FILE //////////////////////////////////