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
/// @file   Server.h
/// @brief  Contains main function and class for running WISCEBridge.
///
/// @version \$Id: Server.h 16820 2015-01-09 13:47:56Z shackman $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
#ifndef __SERVER_H__
#define __SERVER_H__

//
// Include files
// winsock2.h must be included first.
//
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#endif // endif _WIN32

#include "Result.h"
#include "CommandHandler.h"
#include <list>

//
// Definitions
//

//
// The initial length of the buffer to receive/send data.
//
#define INITIAL_BUFFER_LENGTH   1280

////////////////////////////////////////////////////////////////////////////////
///
//  Class: CBufferedCommand
///
/// @brief  Contains a command string.
///
////////////////////////////////////////////////////////////////////////////////
class CBufferedCommand
{
public:
    CBufferedCommand() { buffer = NULL; length = 0; complete = false; }

    char    *buffer;    ///< Buffer containing the command.
    size_t  length;     ///< Length of the command string.
    bool    complete;   ///< Whether it is a complete command or not.
};
typedef std::list<CBufferedCommand> CommandList;

////////////////////////////////////////////////////////////////////////////////
///
//  Class: CServer
///
/// @brief  Creates the server socket and handles connections.
///
////////////////////////////////////////////////////////////////////////////////
class CServer
{
public:
    CServer( int argc, char *argv[] );
    ~CServer();

    RESULT Run();

protected:
    RESULT SendData( const char *message )
        { return SendData( message, strlen( message ) ); }
    RESULT SendData( const char *message, size_t messageLength );
    RESULT SendWelcome();

    RESULT ReceiveCommand( CommandList &commands );

    RESULT HandleConnection();

private:
    int             m_serverSocket;     ///< Our socket.
    unsigned short  m_serverPort;       ///< Our port.
    int             m_clientSocket;     ///< Socket to the client.
    CCommandHandler m_commandHandler;   ///< Handles the commands.

//
// Functionality needed for windows systems.
//
#ifdef _WIN32
    bool                m_winSockActive;

    RESULT InitWSA();
#endif
};

#endif  // __SERVER_H__
///////////////////////////////// END OF FILE //////////////////////////////////