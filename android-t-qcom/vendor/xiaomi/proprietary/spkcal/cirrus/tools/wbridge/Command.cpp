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
/// @file   Command.cpp
/// @brief  Class for manipulating a command.
///
/// @version \$Id: Command.cpp 16820 2015-01-09 13:47:56Z shackman $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   used with dataString devices.
///
////////////////////////////////////////////////////////////////////////////////

//
// Include files
//
#include "Command.h"
#include "WISCEBridgeUtil.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
//
// We're compiling with Linux as well, so we can't use the secure versions
// of the CRTL functions.
//
#pragma warning( disable: 4996 )
#endif

//
// Global definitions
//

//
// Lengths for different parts of the command (includes space for '\0').
//
#define MAX_REGISTER_LENGTH     9
#define MAX_VALUE_LENGTH        9 // TODO: Increase when supporting block writes.
#define HEX_IDENTIFIER_LENGTH   2 // Length of the string "0x" (minus '\0' character).

//
// The position in the parameters list for various command parameters.
// The first string (index 0) is the command.
//
#define COMMAND_PARAMETER   0
#define FIRST_PARAMETER     COMMAND_PARAMETER + 1
#define SECOND_PARAMETER    FIRST_PARAMETER + 1

//
// Class dataString
//

//
// Function prototypes
//

///////////////////////////////////////////////////////////////////////////////
///
//  Class:  CCommand
///
/// @brief  Class for manipulating a command.
///
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
///
//  Function: CCommand
///
/// @brief  Constructor for CCommand.
///
////////////////////////////////////////////////////////////////////////////////
CCommand::CCommand()
{
    m_commandType = UnknownCommand;
    m_commandString = "";
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: CCommand
///
/// @brief  Constructor for CCommand.
///
/// (no parameters)
///
////////////////////////////////////////////////////////////////////////////////
CCommand::CCommand( std::string commandString )
{
    bool invalidDeviceID = false;

    //
    // Initialise defaults
    //
    m_hasSequenceNumber = false;
    m_hasDeviceID = false;
    m_sequenceNumber = 0;
    m_commandString = commandString;
    // Make our sequence number string and device id string the empty string
    m_sequenceNumberString[0] = '\0';
    m_deviceIDString[0] = '\0';

    // Skip any whitespace at the start
    size_t seqNoStart = FindFirstNotOf( commandString, " \t" );
    // Does it start with a sequence number?
    if ( NPOS != seqNoStart && '[' == commandString[seqNoStart] )
    {
        // Skip whitespace inside the sequence number
        seqNoStart = FindFirstNotOf( commandString, " \t", seqNoStart + 1 );

        // Is there a device ID?
        const char *pSeqNoEnd = strstr( commandString.c_str(), "]" );
        const char *pDeviceIDEnd = strstr( commandString.c_str(), ":" );
        std::string deviceID;
        if ( pSeqNoEnd && pDeviceIDEnd && pDeviceIDEnd < pSeqNoEnd && ':' != commandString[seqNoStart] )
        {
            // We have a Device, parse it out.
            size_t deviceIDEnd = FindFirstNotOf( commandString, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-#.", seqNoStart );
            if ( NPOS != deviceIDEnd )
            {
                deviceID = commandString.substr( seqNoStart, deviceIDEnd - 1 );
            }
            deviceIDEnd = FindFirstNotOf( commandString, " \t", deviceIDEnd );
            if ( NPOS != deviceIDEnd && ':' == commandString[deviceIDEnd] )
            {
                m_hasDeviceID = true;
                strncpy( m_deviceIDString, deviceID.c_str(), MAX_DEVICE_ID_LENGTH);
		m_deviceIDString[MAX_DEVICE_ID_LENGTH - 1] = '\0';
                seqNoStart = deviceIDEnd + 1;
            }
        }

        //
        // Handle if we have e.g. "[:<seqNo>]..." i.e. no actual ID but we have
        // the ':' separator. This is an invalid command (we support multiple
        // devices, but don't know which we've been asked to talk to, but we
        // should keep track of the sequence number anyway for reporting the
        // error.
        //
        if ( !m_hasDeviceID && ':' == commandString[seqNoStart] )
        {
            seqNoStart += 1;
            invalidDeviceID = true;
        }

        if ( '0' == commandString[seqNoStart] && 'x' == commandString[seqNoStart+1] )
            seqNoStart += 2;
        if ( NPOS != seqNoStart && IsHexDigit( commandString[seqNoStart] ) )
        {
            // Parse it
            std::string numberString = commandString.substr( seqNoStart );
            unsigned int sequenceNumber = strtoul( numberString.c_str(), NULL, 16 );

            // And check it's valid - it should finish with the ]
            size_t seqNoEnd = FindFirstNotOf( numberString, "0123456789abcdefABCDEF" );
            if ( NPOS != seqNoEnd )
                seqNoEnd = FindFirstNotOf( numberString, " \t", seqNoEnd );
            if ( NPOS != seqNoEnd && ']' == numberString[seqNoEnd] )
            {
                m_hasSequenceNumber = true;
                m_sequenceNumber = sequenceNumber;
                m_commandString = numberString.substr( seqNoEnd + 1 );
                snprintf( m_sequenceNumberString,
                          sizeof( m_sequenceNumberString ),
                          "[%X] ",
                          m_sequenceNumber
                        );
            }
        }
    }

    SplitString( m_commandString, " \t\n", &m_parameters, true );

    std::string command = "";
    RESULT result = GetParameter( COMMAND_PARAMETER, &command );
    if ( WMT_SUCCESS == result && !invalidDeviceID )
        m_commandType = ParseCommandType( command );
    else
        m_commandType = UnknownCommand;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ~CCommand
///
/// @brief  Destructor for CCommand.
///
////////////////////////////////////////////////////////////////////////////////
CCommand::~CCommand()
{
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetParameter
///
/// @brief  Gets the parameter string at the specified index.
///
/// @param  index       The index for the desired parameter.
/// @param  pParameter  Receives the parameter.
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   pParameter was null.
/// @retval WMT_OUT_OF_RANGE        Invalid index.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommand::GetParameter( size_t index, std::string *pParameter ) const
{
    RESULT result = WMT_SUCCESS;

    if ( !pParameter )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    if ( index < m_parameters.size() )
    {
        *pParameter = m_parameters[index];
    }
    else
    {
        result = WMT_OUT_OF_RANGE;
        goto done;
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ParseCommandType
///
/// @brief  Parses the command to get it's type.
///
/// @param  command The command to parse.
///
/// @return The type of the command, or UnknownCommand if unrecognised.
///
////////////////////////////////////////////////////////////////////////////////
CommandType CCommand::ParseCommandType( std::string command ) const
{
    CommandType commandType = UnknownCommand;
    const char  *charCommand = command.c_str();

    if ( 0 == strcasecmp( "read", charCommand ) ||
              0 == strcasecmp( "r", charCommand )
            )
    {
        commandType = ReadCommand;
    }
    else if ( 0 == strcasecmp( "write", charCommand ) ||
              0 == strcasecmp( "w", charCommand )
            )
    {
        commandType = WriteCommand;
    }
    else if ( 0 == strcasecmp( "blockwrite", charCommand ) ||
              0 == strcasecmp( "bw", charCommand )
            )
    {
        commandType = BlockWriteCommand;
    }
    else if ( 0 == strcasecmp( "blockread", charCommand ) ||
              0 == strcasecmp( "br", charCommand )
            )
    {
        commandType = BlockReadCommand;
    }
    else if ( 0 == strcasecmp( "info", charCommand ) )
    {
        commandType = InfoCommand;
    }
    else if ( 0 == strcasecmp( "detect", charCommand ) )
    {
        commandType = DetectCommand;
    }
    else if ( 0 == strcasecmp( "shutdown", charCommand ) )
    {
        commandType = ShutdownCommand;
    }
    else if ( 0 == strcasecmp( "help", charCommand ) )
    {
        commandType = HelpCommand;
    }
    else if ( 0 == strcasecmp( "getregmap", charCommand ) )
    {
        commandType = RegisterMapCommand;
    }
    else if ( 0 == strcasecmp( "device", charCommand ) )
    {
        commandType = DeviceCommand;
    }
    else if ( 0 == strcasecmp( "protocolversion", charCommand ) )
    {
        commandType = ProtocolVersionCommand;
    }

    return commandType;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetAddress
///
/// @brief  Gets the address from the command.
///
/// @param  pAddress    Receives the address.
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   pAddress was null.
/// @retval WMT_INVALID_COMMAND     The command didn't contain an address.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommand::GetAddress( unsigned int *pAddress ) const
{
    RESULT          result = WMT_SUCCESS;
    unsigned int    address = 0;
    std::string     parameter = "";

    if ( !pAddress )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    switch ( m_commandType )
    {
    case ReadCommand:
    case WriteCommand:
        result = GetParameter( FIRST_PARAMETER, &parameter );
        break;
    case BlockReadCommand:
    case BlockWriteCommand:
        {
            result = GetParameter( FIRST_PARAMETER, &parameter );
            if ( WMT_SUCCESS != result )
                goto done;

            //
            // For BR/BW the first parameter is of the following form:
            // [target\[region]:]address.
            //
            // Splitting on : will give us one or two strings depending
            // on whether a target/region part exists.
            //
            std::vector<std::string> parts;
            SplitString( parameter, ":", &parts );

            //
            // If we have more than one part to the parameter then set the
            // parameter to the last part. This will be the address.
            //
            if ( parts.size() > 1 )
                parameter = parts[parts.size() - 1];
        }
        break;
    default:
        result = WMT_INVALID_COMMAND;
        break;
    }

    if ( WMT_SUCCESS == result )
    {
        if ( !CheckHexValueLength( parameter.c_str(), MAX_REGISTER_LENGTH ) )
        {
            result = WMT_INVALID_COMMAND;
            goto done;
        }

        address = strtoul( parameter.c_str(), NULL, 16 );
    }
    else if ( WMT_OUT_OF_RANGE == result )
    {
        // The command didn't have sufficient parameters.
        result = WMT_INVALID_COMMAND;
        goto done;
    }

    *pAddress = address;

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetValue
///
/// @brief  Gets the value from the command.
///
/// @param  pValue  Receives the value.
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   pValue was null.
/// @retval WMT_INVALID_COMMAND     The command didn't contain the value.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommand::GetValue( unsigned int *pValue ) const
{
    RESULT          result = WMT_SUCCESS;
    unsigned int    value = 0;
    std::string     parameter = "";

    if ( !pValue )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    switch ( m_commandType )
    {
    case WriteCommand:
        result = GetParameter( SECOND_PARAMETER, &parameter );
        break;
    default:
        result = WMT_INVALID_COMMAND;
        break;
    }

    if ( WMT_SUCCESS == result )
    {
        if ( !CheckHexValueLength( parameter.c_str(), MAX_REGISTER_LENGTH ) )
        {
            result = WMT_INVALID_COMMAND;
            goto done;
        }

        value = strtoul( parameter.c_str(), NULL, 16 );
    }
    else if ( WMT_OUT_OF_RANGE == result )
    {
        // The command didn't have sufficient parameters.
        result = WMT_INVALID_COMMAND;
        goto done;
    }

    *pValue = value;

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetProtocolVersion
///
/// @brief  Gets the protocol version from the command.
///
/// @param  pVersion  Receives the protocol version.
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   pVersion was null.
/// @retval WMT_INVALID_COMMAND     The command didn't contain the protocol version.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommand::GetProtocolVersion( unsigned int *pVersion ) const
{
    RESULT          result = WMT_SUCCESS;
    unsigned int    version = 0;
    std::string     parameter = "";

    if ( !pVersion )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    switch ( m_commandType )
    {
    case ProtocolVersionCommand:
        result = GetParameter( FIRST_PARAMETER, &parameter );
        break;
    default:
        result = WMT_INVALID_COMMAND;
        break;
    }

    if ( WMT_SUCCESS == result )
    {
        version = strtoul( parameter.c_str(), NULL, 16 );
    }
    else if ( WMT_OUT_OF_RANGE == result )
    {
        // The command didn't have sufficient parameters.
        result = WMT_INVALID_COMMAND;
        goto done;
    }

    *pVersion = version;

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetDevice
///
/// @brief  Gets the device from the command.
///
/// @param  pDevice Receives the device.
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   pDevice was null.
/// @retval WMT_INVALID_COMMAND     The command didn't contain a device.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommand::GetDevice( std::string *pDevice ) const
{
    RESULT      result = WMT_SUCCESS;
    std::string device = "";
    std::string parameter = "";

    if ( !pDevice )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    switch ( m_commandType )
    {
    case DeviceCommand:
        result = GetParameter( FIRST_PARAMETER, &device );
        break;
    default:
        result = WMT_INVALID_COMMAND;
        break;
    }

    if ( WMT_OUT_OF_RANGE == result )
    {
        // The command didn't have sufficient parameters.
        result = WMT_INVALID_COMMAND;
        goto done;
    }

    *pDevice = device;

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: CheckHexValueLength
///
/// @brief  Check the string for a hex value is of valid length.
///
/// @param  value   The value to check.
/// @param  max     The maximum length (Including the '\0').
///
/// @retval true    Is valid.
/// @retval false   Is invalid.
///
////////////////////////////////////////////////////////////////////////////////
bool CCommand::CheckHexValueLength( const char *value, int max ) const
{
    bool retval = true;

    int length = strlen( value );
    if ( length >= max + HEX_IDENTIFIER_LENGTH )
    {
        retval = false;
    }
    else if ( length >= max )
    {
        if ( 0 != strncmp( value, "0x", HEX_IDENTIFIER_LENGTH ) )
            retval = false;
    }

    return retval;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetTarget
///
/// @brief  Gets the target from the command.
///
/// @param  pTarget    Receives the target.
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   pTarget was null.
/// @retval WMT_INVALID_COMMAND     The command didn't contain a target.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommand::GetTarget( std::string *pTarget ) const
{
    RESULT      result = WMT_SUCCESS;
    std::string target = "";
    std::string parameter = "";

    if ( !pTarget )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    switch ( m_commandType )
    {
    case BlockReadCommand:
    case BlockWriteCommand:
        {
            result = GetParameter( FIRST_PARAMETER, &parameter );
            if ( WMT_SUCCESS == result )
            {
                //
                // For BR/BW the first parameter is of the following form:
                // [target\[region]:]address.
                //
                // Splitting on : and \ will give us one two or three strings
                // depending on whether a target and a region part exists.
                //
                std::vector<std::string> parts;
                SplitString( parameter, ":\\", &parts, true );

                // If there is only one part it is the address and there is no target.
                if ( 1 == parts.size() )
                {
                    result = WMT_INVALID_COMMAND;
                    goto done;
                }

                // Otherwise the first part will be the target
                target = parts[0];
            }
        }
        break;
    default:
        result = WMT_INVALID_COMMAND;
        break;
    }

    if ( WMT_OUT_OF_RANGE == result )
    {
        // The command didn't have sufficient parameters.
        result = WMT_INVALID_COMMAND;
        goto done;
    }

    *pTarget = target;

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetRegion
///
/// @brief  Gets the region from the command.
///
/// @param  pRegion    Receives the region.
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   pRegion was null.
/// @retval WMT_INVALID_COMMAND     The command didn't contain a region.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommand::GetRegion( std::string *pRegion ) const
{
    RESULT          result = WMT_SUCCESS;
    std::string     parameter = "";

    if ( !pRegion )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    switch ( m_commandType )
    {
    case BlockReadCommand:
    case BlockWriteCommand:
        {
            result = GetParameter( FIRST_PARAMETER, &parameter );
            if ( WMT_SUCCESS != result )
                goto done;

            //
            // For BR/BW the first parameter is of the following form:
            // [target\[region]:]address.
            //
            // Splitting on : will give us up to three strings depending
            // on whether a target/region part exists.
            //
            std::vector<std::string> parts;
            SplitString( parameter, ":\\", &parts, true );

            //
            // When we have a region there will be three parts to the parameter.
            // The region is the middle part.
            //
            if ( 3 == parts.size() )
            {
                parameter = parts[1];
            }
            else
            {
                result = WMT_INVALID_COMMAND;
                goto done;
            }
        }
        break;
    default:
        result = WMT_INVALID_COMMAND;
        break;
    }

    if ( WMT_OUT_OF_RANGE == result )
    {
        // The command didn't have sufficient parameters.
        result = WMT_INVALID_COMMAND;
        goto done;
    }

    *pRegion = parameter;

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetLength
///
/// @brief  Gets the length from the command.
///
/// @param  pLength Receives the length.
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   pLength was null.
/// @retval WMT_INVALID_COMMAND     The command didn't contain a length.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommand::GetLength( unsigned int *pLength ) const
{
    RESULT          result = WMT_SUCCESS;
    unsigned int    length = 0;
    std::string     parameter = "";

    if ( !pLength )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    switch ( m_commandType )
    {
    case BlockReadCommand:
        result = GetParameter( SECOND_PARAMETER, &parameter );
        break;
    default:
        result = WMT_INVALID_COMMAND;
        break;
    }

    if ( WMT_SUCCESS == result )
    {
        length = strtoul( parameter.c_str(), NULL, 16 );
    }
    else if ( WMT_OUT_OF_RANGE == result )
    {
        // The command didn't have sufficient parameters.
        result = WMT_INVALID_COMMAND;
        goto done;
    }

    *pLength = length;

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetData
///
/// @brief  Gets the data from the command.
///
/// @note   You must delete the data once finished using it.
///
/// @param  ppData      Receives the data. You must delete when finished with it.
/// @param  pDataLength Receives the length of the data.
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   ppData or pDataLength was null.
/// @retval WMT_INVALID_COMMAND     The command didn't contain data.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CCommand::GetData( unsigned char **ppData,  size_t *pDataLength ) const
{
    RESULT          result = WMT_SUCCESS;
    size_t          length = 0;
    std::string     parameter = "";
    unsigned char   *pData = NULL;

    if ( !ppData || !pDataLength )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    switch ( m_commandType )
    {
    case BlockWriteCommand:
        result = GetParameter( SECOND_PARAMETER, &parameter );
        if ( WMT_SUCCESS != result )
        {
            if ( WMT_OUT_OF_RANGE == result )
                result = WMT_INVALID_COMMAND;

            goto done;
        }

        //
        // The data length should be a multiple of 2, otherwise we are missing
        // half a byte.
        //
        if ( 0 != parameter.size() % 2 )
        {
            result = WMT_INVALID_COMMAND;
            goto done;
        }

        //
        // Convert the ASCII formatted data to an array of bytes.
        //
        length = parameter.size() / 2;
        pData = new unsigned char[length];
        for ( size_t start = 0; start < parameter.size(); start += 2 )
        {
            std::string tempByte = parameter.substr( start, 2 );
            pData[start/2] = strtoul( tempByte.c_str(), NULL, 16 ) & 0xFF;
        }

        *ppData = pData;
        *pDataLength = length;
        break;
    default:
        result = WMT_INVALID_COMMAND;
        break;
    }

done:
    return result;
}

///////////////////////////////// END OF FILE //////////////////////////////////
