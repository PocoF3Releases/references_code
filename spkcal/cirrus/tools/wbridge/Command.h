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
/// @file   Command.h
/// @brief  Class for manipulating a command.
///
/// @version \$Id: Command.h 16820 2015-01-09 13:47:56Z shackman $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
#ifndef __COMMAND_H__
#define __COMMAND_H__

//
// Include files
//
#include "Result.h"
#include "StringUtil.h"
#include <string>

//
// Definitions
//

enum CommandType
{
    InfoCommand = 0,
    WriteCommand,
    ReadCommand,
    UnknownCommand,
    ShutdownCommand,
    HelpCommand,
    DetectCommand,
    DeviceCommand,
    RegisterMapCommand,
    ProtocolVersionCommand,
    BlockWriteCommand,
    BlockReadCommand
};

#define MAX_SEQUENCE_STRING_LENGTH  20
#define MAX_DEVICE_ID_LENGTH        32

///////////////////////////////////////////////////////////////////////////////
///
//  Class:  CCommand
///
/// @brief  Class for manipulating a command.
///
///////////////////////////////////////////////////////////////////////////////
class CCommand
{
public:
    CCommand();
    CCommand( std::string commandString );
    ~CCommand();

    typedef unsigned int SequenceNumber;

    CommandType     GetType() const { return m_commandType; }
    std::string     GetString() const { return m_commandString; }
    SequenceNumber  GetSequenceNumber() const { return m_sequenceNumber; }
    const char      *GetSequenceNumberString() const { return m_sequenceNumberString; }
    bool            HasSequenceNumber() const { return m_hasSequenceNumber; }
    const char      *GetDeviceIdentifierString() const { return m_deviceIDString; }
    bool            HasDeviceIdentifier() const { return m_hasDeviceID; }

    RESULT GetAddress( unsigned int *pAddress ) const;
    RESULT GetValue( unsigned int *pValue ) const;
    RESULT GetProtocolVersion( unsigned int *pVersion ) const;
    RESULT GetDevice( std::string *pDevice ) const;
    RESULT GetTarget( std::string *pTarget ) const;
    RESULT GetRegion( std::string *pRegion ) const;
    RESULT GetLength( unsigned int *pLength ) const;
    RESULT GetData( unsigned char **ppData, size_t *pDataLength ) const;

    bool CheckHexValueLength( const char *value, int max ) const;
    CommandType ParseCommandType( std::string command ) const;

private:
    RESULT GetParameter( size_t index, std::string *pParameter ) const;

    CommandType                 m_commandType;
    SequenceNumber              m_sequenceNumber;
    bool                        m_hasSequenceNumber;
    char                        m_sequenceNumberString[MAX_SEQUENCE_STRING_LENGTH];
    bool                        m_hasDeviceID;
    char                        m_deviceIDString[MAX_DEVICE_ID_LENGTH];
    std::vector<std::string>    m_parameters;
    std::string                 m_commandString;
};

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Inline functions
////////////////////////////////////////////////////////////////////////////////



#endif  // __COMMAND_H__
///////////////////////////////// END OF FILE //////////////////////////////////