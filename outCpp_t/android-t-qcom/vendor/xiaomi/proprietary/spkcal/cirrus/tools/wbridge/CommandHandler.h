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
/// @file   CommandHandler.h
/// @brief  Parses the commands from the client.
///
/// @version \$Id: CommandHandler.h 16820 2015-01-09 13:47:56Z shackman $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
#ifndef __COMMANDHANDLER_H__
#define __COMMANDHANDLER_H__

//
// Include files
//
#include "DeviceCommunication.h"
#include <string>

//
// Definitions
//

//
// Forward declarations.
//
class CCommand;

////////////////////////////////////////////////////////////////////////////////
///
//  Class:  CCommandHandler
///
/// @brief  Parses the commands from the client.
///
////////////////////////////////////////////////////////////////////////////////
class CCommandHandler
{
public:
    // Constructors and destructors
    CCommandHandler( int argc = 0, char *argv[] = NULL );
    virtual ~CCommandHandler();

    RESULT HandleCommandString( std::string *buffer );

    RESULT GetDeviceID( std::string *buffer );
    RESULT GetErrorString( std::string *buffer, RESULT error );

private:
    RESULT HandleRead( const CCommand *pCommand, std::string *buffer );
    RESULT HandleWrite( const CCommand *pCommand, std::string *buffer );
    RESULT HandleInfo( const CCommand *pCommand, std::string *buffer );
    RESULT HandleHelp( const CCommand *pCommand, std::string *buffer );
    RESULT HandleDetect( const CCommand *pCommand, std::string *buffer );
    RESULT HandleDevice( const CCommand *pCommand, std::string *buffer );
    RESULT HandleRegisterMap( const CCommand *pCommand, std::string *buffer );
    RESULT HandleProtocolVersion( const CCommand *pCommand, std::string *buffer );
    RESULT HandleBlockRead( const CCommand *pCommand, std::string *buffer );
    RESULT HandleBlockWrite( const CCommand *pCommand, std::string *buffer );

    CDeviceCommunication    m_device;
    unsigned int            m_wisceVersion;
};

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Inline functions
////////////////////////////////////////////////////////////////////////////////



#endif  // __COMMANDHANDLER_H__
///////////////////////////////// END OF FILE //////////////////////////////////