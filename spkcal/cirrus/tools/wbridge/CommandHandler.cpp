////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2011-2015, 2018 Cirrus Logic, Inc and
// Cirrus Logic International Semiconductor Ltd.  All rights reserved.
//
// This software as well as any related documentation is furnished under
// license and may only be used or copied in accordance with the terms of the
// license.  The information in this file is furnished for informational use
// only, is subject to change without notice, and should not be construed as
// a commitment by Cirrus Logic.  Cirrus Logic assumes no responsibility or
// liability for any errors or inaccuracies that may appear in this document
// or any software that may be provided in association with this document.
//
// Except as permitted by such license, no part of this document may be
// reproduced, stored in a retrieval system, or transmitted in any form or by
// any means without the express written consent of Cirrus Logic.
//
/// @file   CommandHandler.cpp
/// @brief  Parses the commands from the client.
///
//  @version \$Id: CommandHandler.cpp 20109 2018-03-21 15:00:48Z stankic $
//
//  @warning
//    This software is specifically written for Cirrus Logic devices.
//    It may not be used with other devices.
//
////////////////////////////////////////////////////////////////////////////////

//
// Include files
//
#include "CommandHandler.h"
#include "WISCEBridgeVersion.h"
#include "Command.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

//
// Global definitions
//

#ifdef _WIN32
//
// We're compiling with Linux as well, so we can't use the secure versions
// of the CRTL functions.
//
#pragma warning( disable: 4996 )
#endif

//
// Lengths for different parts of the command (includes space for '\0').
//
#define MAX_COMMAND_LENGTH      256

//
// Definitions for the INFO return string.
//
#define MAX_INFO_PART_LENGTH    64

//
// The version of the WISCEBridge protocol you are using.
//
// Version 1.0 includes 'END\n' for ending multi-line commands and the
// ProtocolVersion command.
//
// Version 1.1 includes BlockRead and BlockWrite.
//
// Version 1.2 includes sequence numbers.
//
// Version 1.3 updates how multiple devices are handled, include prefixing
// sequence numbers with destination devices.
//
#define PROTOCOL_VERSION    MAKE_16BIT_VERSION( 1, 3 )
#define DEFAULT_VERSION     MAKE_16BIT_VERSION( 0, 9 )

//
// Definitions of unknown device IDs
//
#define CHIP_ANY                0xFFFF      ///< Constant to say "don't care" when selecting device
#define CHIP_ANY_32BIT          0xFFFFFFFF  ///< As per above, but a 32-bit ID
#define CHIP_MIN_ID             0x1000      ///< Chip IDs must start at 1000

//
// Class data
//

//
// Function prototypes
//

///////////////////////////////////////////////////////////////////////////////
///
//  Class:  CCommandHandler
///
/// @brief  Parses the commands from the client.
///
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///
//  Constructor: CCommandHandler
///
/// @brief   Constructor for CCommandHandler
///
/// @param  argc    Argument count.
/// @param  argv    Arguments.
///
///////////////////////////////////////////////////////////////////////////////
CCommandHandler::CCommandHandler( int argc /* = 0 */, char *argv[] /* = NULL */ )
    : m_device( argc, argv )    // document that this is initialised
{
    m_wisceVersion = DEFAULT_VERSION;
}

///////////////////////////////////////////////////////////////////////////////
///
//  Destructor: ~CCommandHandler
///
/// @brief Destructor for CCommandHandler - cleans up when it is destroyed.
///
///////////////////////////////////////////////////////////////////////////////
CCommandHandler::~CCommandHandler()
{
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleCommandString
///
/// @brief  Handles the command received from the connection.
///
/// @param  buffer  The buffer holding the command (and to receive the return
///                 string).
///
/// @retval >0                  The size of the command to return to the client.
/// @retval WMT_SUCCESS         Success.
/// @retval WMT_CLOSE_SOCKET    The socket needs closing.
/// @retval WMT_STRING_ERR      There was an error manipulating a string.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::HandleCommandString( std::string *buffer )
{
    RESULT      result = WMT_SUCCESS;
    CCommand    command( *buffer );

    // Put the sequence number at the start of the string if relevant
    *buffer = command.GetSequenceNumberString();

    // Switch device if we need to.
    if ( command.HasDeviceIdentifier() )
    {
        std::string name = m_device.GetDeviceName();
        if ( command.GetDeviceIdentifierString() != name )
        {
            result = m_device.SetDevice( command.GetDeviceIdentifierString() );
            if ( WMT_NO_DEVICE == result )
            {
                result = GetErrorString( buffer, WMT_NO_DEVICE );
                if ( WMT_SUCCESS == result )
                    result = (RESULT) buffer->length();

                goto done;
            }
        }
    }

    // Now process the command
    switch ( command.GetType() )
    {
    case ReadCommand:
        result = HandleRead( &command, buffer );
        break;
    case InfoCommand:
        result = HandleInfo( &command, buffer );
        break;
    case WriteCommand:
        result = HandleWrite( &command, buffer );
        break;
    case ShutdownCommand:
        result = WMT_CLOSE_SOCKET;
        break;
    case HelpCommand:
        result = HandleHelp( &command, buffer );
        break;
    case DetectCommand:
        result = HandleDetect( &command, buffer );
        break;
    case DeviceCommand:
        result = HandleDevice( &command, buffer );
        break;
    case RegisterMapCommand:
        result = HandleRegisterMap( &command, buffer );
        break;
    case ProtocolVersionCommand:
        result = HandleProtocolVersion( &command, buffer );
        break;
    case BlockReadCommand:
        result = HandleBlockRead( &command, buffer );
        break;
    case BlockWriteCommand:
        result = HandleBlockWrite( &command, buffer );
        break;
    default:
        result = GetErrorString( buffer, WMT_INVALID_COMMAND );
        if ( WMT_SUCCESS == result )
            result = (RESULT) buffer->length();
        break;
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleRead
///
/// @brief  Handles a read command.
///
/// @param  pCommand        The read command.
/// @param  buffer          The buffer to put the return string in.  This
///                         should be cleaned out and already contain the
///                         sequence number if appropriate.
///
/// @return >0              The size of the command to return to the client.
/// @retval WMT_STRING_ERR  There was an error manipulating a string.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::HandleRead( const CCommand  *pCommand,
                                    std::string     *buffer
                                  )
{
    RESULT          result = WMT_SUCCESS;
    unsigned int    val;
    unsigned int    addr;
    char            readBuffer[PATH_MAX];

    assert( ReadCommand == pCommand->GetType() );
    if ( ReadCommand != pCommand->GetType() )
    {
        result = WMT_INVALID_COMMAND;
        goto error;
    }

    result = pCommand->GetAddress( &addr );
    if ( WMT_SUCCESS != result )
        goto error;

    result = m_device.ReadRegister( addr, &val );
    if ( WMT_SUCCESS != result )
        goto error;

    // Format our output string and set the buffer to this value.
    if ( WMT_STRING_ERR == snprintf( readBuffer, sizeof( readBuffer ), "%x\n", val ) )
        goto strerror;

    *buffer += readBuffer;

    return (RESULT) buffer->length();

error:
    result = GetErrorString( buffer, result );
    if ( WMT_STRING_ERR == result )
        goto strerror;

    return (RESULT) buffer->length();

strerror:
    return WMT_STRING_ERR;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleWrite
///
/// @brief  Handles a write command.
///
/// @param  pCommand        The write command.
/// @param  buffer          The buffer to put the return string in.  This
///                         should be cleaned out and already contain the
///                         sequence number if appropriate.
///
/// @retval >0              The size of the command to return to the client.
/// @retval WMT_STRING_ERR  There was an error manipulating a string.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::HandleWrite( const CCommand *pCommand,
                                     std::string    *buffer
                                   )
{
    RESULT          result = WMT_SUCCESS;
    unsigned int    val;
    unsigned int    addr;
    
    assert( WriteCommand == pCommand->GetType() );
    if ( WriteCommand != pCommand->GetType() )
    {
        result = WMT_INVALID_COMMAND;
        goto error;
    }

    result = pCommand->GetAddress( &addr );
    if ( WMT_SUCCESS != result )
        goto error;

    result = pCommand->GetValue( &val );
    if ( WMT_SUCCESS != result )
        goto error;

    result = m_device.WriteRegister( addr, val );
    if ( WMT_SUCCESS != result )
        goto error;

    *buffer += "Ok\n";

    return (RESULT) buffer->length();

error:
    result = GetErrorString( buffer, result );
    if ( WMT_STRING_ERR == result )
        goto strerror;

    return (RESULT) buffer->length();

strerror:
    return WMT_STRING_ERR;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleInfo
///
/// @brief  Handles an info command.
///
/// @param  pCommand        The info command.
/// @param  buffer          The buffer to put the return string in.  This
///                         should be cleaned out and already contain the
///                         sequence number if appropriate.
///
/// @retval >0              The size of the command to return to the client.
/// @retval WMT_STRING_ERR  There was an error manipulating a string.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::HandleInfo( const CCommand  *pCommand,
                                    std::string     *buffer
                                  )
{
    RESULT      result = WMT_SUCCESS;
    std::string device;
    char        osInfo[MAX_INFO_PART_LENGTH];
    char        fullInfo[PATH_MAX];

    assert( InfoCommand == pCommand->GetType() );
    if ( InfoCommand != pCommand->GetType() )
    {
        result = WMT_INVALID_COMMAND;
        goto error;
    }

    memset( osInfo, 0, MAX_INFO_PART_LENGTH );

    result = GetDeviceID( &device );
    if ( WMT_SUCCESS != result )
        goto error;

    result = m_device.GetOSInfo( osInfo, MAX_INFO_PART_LENGTH );
    if ( WMT_SUCCESS != result )
        goto error;

    if ( WMT_STRING_ERR == snprintf( fullInfo,
                                     sizeof( fullInfo ),
                                     "device=\"%s\" app=\"%s\" version=\"%s\" protocolversion=\"%x\" %s\n",
                                     device.c_str(),
                                     APPNAME,
                                     VERSION_STRA,
                                     PROTOCOL_VERSION,
                                     osInfo
                                   )
       )
    {
        goto strerror;
    }

    *buffer += fullInfo;

    return (RESULT) buffer->length();

error:
    result = GetErrorString( buffer, result );
    if ( WMT_STRING_ERR == result )
        goto strerror;

    return (RESULT) buffer->length();

strerror:
    return WMT_STRING_ERR;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleDetect
///
/// @brief  Handles the detect command.
///
/// @param  pCommand        The detect command.
/// @param  buffer          The buffer to put the return string in.  This
///                         should be cleaned out and already contain the
///                         sequence number if appropriate.
///
/// @retval >0              The size of the command to return to the client.
/// @retval WMT_STRING_ERR  There was an error manipulating a string.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::HandleDetect( const CCommand    *pCommand,
                                      std::string       *buffer
                                    )
{
    char                        enumeratedDevices[MAX_COMMAND_LENGTH];
    std::string                 currentDevice;
    RESULT                      result = WMT_SUCCESS;
    std::vector<std::string>    names;

    assert( DetectCommand == pCommand->GetType() );
    if ( DetectCommand != pCommand->GetType() )
    {
        result = WMT_INVALID_COMMAND;
        goto error;
    }

    result = m_device.EnumerateDevices( enumeratedDevices, MAX_COMMAND_LENGTH );
    if ( WMT_NO_DEVICE == result )
        goto error;

    *buffer += "Detect";

    // Store the current active device.
    currentDevice = m_device.GetDeviceName();

    SplitString( enumeratedDevices, " ", &names );
    for ( size_t i = 0; i < names.size(); i++ )
    {
        RESULT      localResult = WMT_SUCCESS;
        std::string id;

        localResult = m_device.SetDevice( names[i].c_str() );
        if ( WMT_NO_DEVICE == localResult )
            continue;

        localResult = GetDeviceID( &id );
        *buffer += " ";
        *buffer += names[i];
        *buffer += "=\"";
        if ( WMT_SUCCESS == localResult )
            *buffer += id;
        else
            *buffer += "unknown";
        *buffer += "\"";
    }
    *buffer += "\n";

    // Make sure we end on the device we started on.
    m_device.SetDevice( currentDevice.c_str() );

    return (RESULT) buffer->length();

error:
    result = GetErrorString( buffer, result );
    if ( WMT_STRING_ERR == result )
        goto strerror;

    return (RESULT) buffer->length();

strerror:
    return WMT_STRING_ERR;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleHelp
///
/// @brief  Handles the help command.
///
/// @param  pCommand        The help command.
/// @param  buffer          The buffer to put the return string in.  Note
///                         this overwrites any sequence number.
///
/// @retval >0              The size of the command to return to the client.
/// @retval WMT_STRING_ERR  There was an error manipulating a string.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::HandleHelp( const CCommand  *pCommand,
                                    std::string     *buffer
                                  )
{
    RESULT result = WMT_SUCCESS;

    assert( HelpCommand == pCommand->GetType() );
    if ( HelpCommand != pCommand->GetType() )
    {
        result = WMT_INVALID_COMMAND;
        goto error;
    }

    *buffer = "WISCEBridge Command List\n"
              "=========================\n"
              "info\t\t\t\tShow information about the target system.\n"
              "detect\t\t\t\tShow all the detected Cirrus devices.\n"
              "shutdown\t\t\tTerminate the connection to WISCEBridge.\n"
              "help\t\t\t\tDisplay this help.\n"
              "read <reg>\t\t\tRead the value of a register on the selected device (index in hex).\n"
              "write <reg> <value>\t\tUpdate the value to the given register on the selected device (index and value in hex).\n"
              "device <device name>\t\tSelect the device to operate on.\n"
              "getregmap\t\t\tReturn the entire register map in ASCII.\nEND\n"
              "protocolversion <version>\tQuery for the version of the protocol in use. Pass in the version supported by the client.\n"
              "\nFor the following commands the address takes the form [target[/region]:]<reg>\n\n"
              "blockread <address> <length>\tRead length bytes from the address\n"
              "blockwrite <address> <data>\tWrite data bytes to the address\n";

    if ( 0 == buffer->length() )
        goto strerror;

    return (RESULT) buffer->length();

error:
    result = GetErrorString( buffer, result );
    if ( WMT_STRING_ERR == result )
        goto strerror;

    return (RESULT) buffer->length();

strerror:
    return WMT_STRING_ERR;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleDevice
///
/// @brief  Handles the device command.
///
/// @param  pCommand        The device command.
/// @param  buffer          The buffer to put the return string in.  This
///                         should be cleaned out and already contain the
///                         sequence number if appropriate.
///
/// @retval >0              The size of the command to return to the client.
/// @retval WMT_STRING_ERR  There was an error manipulating a string.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::HandleDevice( const CCommand    *pCommand,
                                      std::string       *buffer
                                    )
{
    RESULT      result = WMT_SUCCESS;
    std::string device = "";

    assert( DeviceCommand == pCommand->GetType() );
    if ( DeviceCommand != pCommand->GetType() )
    {
        result = WMT_INVALID_COMMAND;
        goto error;
    }

    result = pCommand->GetDevice( &device );
    if ( WMT_SUCCESS != result )
        goto error;

    result = m_device.SetDevice( device.c_str() );
    if ( WMT_NO_DEVICE == result )
        goto error;

    *buffer += "Ok\n";

    return (RESULT) buffer->length();

error:
    result = GetErrorString( buffer, result );
    if ( WMT_STRING_ERR == result )
        goto strerror;

    return (RESULT) buffer->length();

strerror:
    return WMT_STRING_ERR;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleRegisterMap
///
/// @brief  Handles the getregmap command.
///
/// @param  pCommand        The getregmap command.
/// @param  buffer          The buffer to put the return string in.  This
///                         should be cleaned out and already contain the
///                         sequence number if appropriate.
///
/// @retval >0              The size of the command to return to the client.
/// @retval WMT_NO_DEVICE   No device present.
/// @retval WMT_STRING_ERR  There was an error manipulating a string.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::HandleRegisterMap( const CCommand   *pCommand,
                                           std::string      *buffer
                                         )
{
    CRegValuePairs  regvals;
    RESULT          result = WMT_SUCCESS;
    unsigned long   prev_reg, curr_reg;
    bool            flag = true;
    char            part[PATH_MAX];
    const char      *seqStr = pCommand->GetSequenceNumberString();

    assert( RegisterMapCommand == pCommand->GetType() );
    if ( RegisterMapCommand != pCommand->GetType() )
    {
        result = WMT_INVALID_COMMAND;
        goto error;
    }

    //
    // Get a vector of all the register -> value pairs available
    // in the currently active codec.
    //
    result = m_device.GetRegisterMap( regvals );
    if ( result != WMT_SUCCESS )
        goto error;

    if ( !regvals.size() )
        goto strerror;

    //
    // g++ complains of possible use of prev_reg while uninitialized.
    // Since this is not true, we initialise it here to make g++ happy.
    //
    prev_reg = 0;
    for ( CRegValuePairs::iterator it = regvals.begin();
          it != regvals.end();
          ++it
        )
    {
        //
        // We need NULL terminated register and value fields so make
        // a copy on the stack and NULL terminate them;
        //
        char reg[sizeof it->reg + 1] = { 0 };
        char value[sizeof it->value + 1] = { 0 };
        memcpy( reg, it->reg, sizeof it->reg );
        memcpy( value, it->value, sizeof it->value );
        curr_reg = strtoul( reg, NULL, 16 );

        //
        // Special case, first time through the vector. Required
        // for setting prev_reg to the correct register value.
        //
        if ( flag )
        {
            flag = false;
            if ( WMT_STRING_ERR == snprintf( part, sizeof( part ), "%s: %s", reg, value ) )
                goto strerror;

            *buffer += part;
            prev_reg = curr_reg;
            continue;
        }

        // We've got a gap in the register map.
        if ( prev_reg + 1 != curr_reg )
        {
            if ( WMT_STRING_ERR == snprintf( part, sizeof( part ), "\n%s%s: %s", seqStr, reg, value ) )
                goto strerror;
        }
        else
        {
            if ( WMT_STRING_ERR == snprintf( part, sizeof( part ), " %s", value ) )
                goto strerror;
        }

        *buffer += part;
        prev_reg = curr_reg;
    }

    *buffer += "\n";

    // Versions 1.0 and greater support 'END\n';
    if ( m_wisceVersion != DEFAULT_VERSION )
    {
        *buffer += seqStr;
        *buffer += "END\n";
    }

    return (RESULT) buffer->length();

error:
    result = GetErrorString( buffer, result );
    if ( WMT_STRING_ERR == result )
        goto strerror;

    return (RESULT) buffer->length();

strerror:
    return WMT_STRING_ERR;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleProtocolVersion
///
/// @brief  Handles the ProtocolVersion command.
///
/// @param  pCommand        The ProtocolVersion command.
/// @param  buffer          The buffer to put the return string in.  This
///                         should be cleaned out and already contain the
///                         sequence number if appropriate.
///
/// @retval >0              The size of the command to return to the client.
/// @retval WMT_STRING_ERR  Error manipulating strings.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::HandleProtocolVersion( const CCommand    *pCommand,
                                               std::string       *buffer
                                             )
{
    RESULT          result = WMT_SUCCESS;
    unsigned int    version;
    char            versionString[PATH_MAX];

    assert( ProtocolVersionCommand == pCommand->GetType() );
    if ( ProtocolVersionCommand != pCommand->GetType() )
    {
        result = WMT_INVALID_COMMAND;
        goto error;
    }

    result = pCommand->GetProtocolVersion( &version );
    if ( WMT_SUCCESS != result )
        goto error;

    m_wisceVersion = version;

    // Reply with our version.
    if ( WMT_STRING_ERR == snprintf( versionString,
                                     sizeof( versionString ),
                                     "ProtocolVersion %x\n",
                                     PROTOCOL_VERSION
                                   )
       )
    {
        result = WMT_STRING_ERR;
        goto error;
    }

    *buffer += versionString;

    return (RESULT) buffer->length();

error:
    result = GetErrorString( buffer, result );
    if ( WMT_STRING_ERR == result )
        goto strerror;

    return (RESULT) buffer->length();

strerror:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleBlockRead
///
/// @brief  Handles the block read command.
///
/// @param  pCommand        The BlockRead command.
/// @param  buffer          The buffer to put the return string in.  This
///                         should be cleaned out and already contain the
///                         sequence number if appropriate.
///
/// @retval >0              The size of the command to return to the client.
/// @retval WMT_STRING_ERR  Error manipulating strings.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::HandleBlockRead( const CCommand *pCommand,
                                         std::string    *buffer
                                       )
{
    RESULT          result = WMT_SUCCESS;
    unsigned int    addr;
    unsigned int    length;
    std::string     region = "";
    std::string     target = "";
    bool            hasTarget = false;
    bool            hasRegion = false;
    unsigned char   *data = NULL;
    char            byteBuffer[PATH_MAX];

    assert( BlockReadCommand == pCommand->GetType() );
    if ( BlockReadCommand != pCommand->GetType() )
    {
        result = WMT_INVALID_COMMAND;
        goto error;
    }

    result = pCommand->GetAddress( &addr );
    if ( WMT_SUCCESS != result )
        goto error;

    result = pCommand->GetTarget( &target );
    if ( WMT_SUCCESS == result )
    {
        hasTarget = true;
        result = pCommand->GetRegion( &region );
        if ( WMT_SUCCESS == result )
        {
            hasRegion = true;
        }
    }

    result = pCommand->GetLength( &length );
    if ( WMT_SUCCESS != result )
        goto error;

    // If length isn't a multiple of 2 then it's not valid.
    if ( 0 != length % 2 )
    {
        result = WMT_INVALID_COMMAND;
        goto error;
    }

    data = new unsigned char[length];

    // Call the appropriate version of BlockRead for our parameters.
    if ( hasRegion )
        result = m_device.BlockRead( target, region, addr, length, data );
    else if ( hasTarget )
        result = m_device.BlockRead( target, addr, length, data );
    else
        result = m_device.BlockRead( addr, length, data );

    if ( WMT_SUCCESS != result )
        goto error;

    // Format the data in ASCII to return to WISCE™.
    for ( size_t i = 0; i < length; i++ )
    {
        // We need to pad with 0s so each byte is two characters.
        if ( data[i] == 0 )
        {
            if ( WMT_STRING_ERR == snprintf( byteBuffer,
                                             sizeof( byteBuffer ),
                                             "00"
                                           )
               )
            {
                result = WMT_STRING_ERR;
                goto error;
            }
        }
        else
        {
            if ( data[i] < 0x10 )
            {
                if ( WMT_STRING_ERR == snprintf( byteBuffer,
                                                 sizeof( byteBuffer ),
                                                 "0%x",
                                                 data[i]
                                               )
                   )
                {
                    result = WMT_STRING_ERR;
                    goto error;
                }
            }
            else
            {
                if ( WMT_STRING_ERR == snprintf( byteBuffer,
                                                 sizeof( byteBuffer ),
                                                 "%x",
                                                 data[i]
                                               )
                   )
                {
                    result = WMT_STRING_ERR;
                    goto error;
                }
            }
        }
        
        *buffer += byteBuffer;
    }

    *buffer += "\n";

    delete data;
    return (RESULT) buffer->length();

error:
    if ( data )
    {
        delete[] data;
        data = NULL;
    }
    result = GetErrorString( buffer, result );
    if ( WMT_STRING_ERR == result )
        goto strerror;

    return (RESULT) buffer->length();

strerror:
    if ( data )
    {
        delete[] data;
        data = NULL;
    }

    return WMT_STRING_ERR;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleBlockWrite
///
/// @brief  Handles the block write command.
///
/// @param  pCommand        The BlockWrite command.
/// @param  buffer          The buffer to put the return string in.  This
///                         should be cleaned out and already contain the
///                         sequence number if appropriate.
///
/// @retval >0              The size of the command to return to the client.
/// @retval WMT_STRING_ERR  Error manipulating strings.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::HandleBlockWrite( const CCommand    *pCommand,
                                          std::string       *buffer
                                        )
{
    RESULT          result = WMT_SUCCESS;
    unsigned int    addr;
    size_t          length;
    std::string     region = "";
    unsigned char   *pData = NULL;
    std::string     target = "";
    bool            hasTarget = false;
    bool            hasRegion = false;

    assert( BlockWriteCommand == pCommand->GetType() );
    if ( BlockWriteCommand != pCommand->GetType() )
    {
        result = WMT_INVALID_COMMAND;
        goto error;
    }

    result = pCommand->GetAddress( &addr );
    if ( WMT_SUCCESS != result )
        goto error;

    result = pCommand->GetTarget( &target );
    if ( WMT_SUCCESS == result )
    {
        hasTarget = true;
        result = pCommand->GetRegion( &region );
        if ( WMT_SUCCESS == result )
        {
            hasRegion = true;
        }
    }

    result = pCommand->GetData( &pData, &length );
    if ( WMT_SUCCESS != result )
        goto error;

    // If length isn't a multiple of 2 then it's not valid.
    if ( 0 != length % 2 )
    {
        result = WMT_INVALID_COMMAND;
        goto error;
    }

    // Call the appropriate version of BlockRead for our parameters.
    if ( hasRegion )
        result = m_device.BlockWrite( target, region, addr, length, pData );
    else if ( hasTarget )
        result = m_device.BlockWrite( target, addr, length, pData );
    else
        result = m_device.BlockWrite( addr, length, pData );

    if ( WMT_SUCCESS != result )
        goto error;

    *buffer += "Ok\n";

    delete[] pData;
    return (RESULT) buffer->length();

error:
    if ( pData )
    {
        delete[] pData;
        pData = NULL;
    }
    result = GetErrorString( buffer, result );
    if ( WMT_STRING_ERR == result )
        goto strerror;

    return (RESULT) buffer->length();

strerror:
    if ( pData )
    {
        delete[] pData;
        pData = NULL;
    }

    return WMT_STRING_ERR;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetDeviceID
///
/// @brief  Gets the device's ID.
///
/// @param  buffer          The buffer to put the return string in.  This
///                         should be cleaned out and already contain the
///                         sequence number if appropriate.
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_STRING_ERR  Error manipulating strings.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::GetDeviceID( std::string *buffer )
{
    RESULT          result = WMT_SUCCESS;
    unsigned int    value = 0;
    const char      *deviceName = m_device.GetDeviceName();
    char            id[PATH_MAX];
    char            bufferStr[PATH_MAX];
    char            *bufferPointer = NULL;
    int             idRegisters[] = { 0x0, 0xF, 0x4000, 0xF000 };
    bool            found = false;

    // Get the device ID by reading from register 0.
    for ( size_t index = 0; index < 4; index++ )
    {
        result = m_device.ReadRegister( idRegisters[index], &value );
        if ( WMT_SUCCESS == result    &&
             0 != value               &&
             CHIP_ANY != value        &&
             CHIP_ANY_32BIT != value  &&
             value > CHIP_MIN_ID
           )
        {
            found = true;
           break;
        }
    }

    if ( WMT_SUCCESS == result && found )
    {
        if ( WMT_STRING_ERR == snprintf( id, sizeof( id ), "WM%4X", value ) )
        {
            if ( !deviceName )
            {
                result = WMT_STRING_ERR;
                goto done;
            }
            else
            {
                 // The buffer should already be cleared.
            }
        }

        *buffer += id;
    }
    else 
    {
        *buffer += "unknown";
    }

    // Ensure we don't include any line-breaks in the device name buffer.
    strncpy( bufferStr, buffer->c_str(), sizeof( bufferStr ) );
    bufferStr[sizeof( bufferStr ) - 1] = '\0';
    bufferPointer = strchr( bufferStr, '\n' );
    if ( bufferPointer )
        *bufferPointer = '\0';
    bufferPointer = strchr( bufferStr, '\r' );
    if ( bufferPointer )
        *bufferPointer = '\0';
    *buffer = bufferStr;

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetErrorString
///
/// @brief  Gets the string corresponding to the error.
///
/// @param  buffer          The buffer to put the return string in.  This
///                         should be cleaned out and already contain the
///                         sequence number if appropriate.
/// @param  error           The error to get the string for.
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_STRING_ERR  Error manipulating strings.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommandHandler::GetErrorString( std::string *buffer, RESULT error )
{
    RESULT  result = WMT_SUCCESS;
    char    string[PATH_MAX];

    if ( WMT_STRING_ERR == snprintf( string, sizeof( string ), "Error %x\n", ( error * -1 ) ) )
        result = WMT_STRING_ERR;
    else
        *buffer += string;

    return result;
}

///////////////////////////////// END OF FILE //////////////////////////////////
