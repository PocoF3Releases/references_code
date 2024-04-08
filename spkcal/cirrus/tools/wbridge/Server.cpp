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
/// @file   Server.cpp
/// @brief  Contains main function and class for running WISCEBridge.
///
/// @version \$Id: Server.cpp 18539 2016-08-01 10:46:15Z lbogdan $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////

//
// Networking code is based off of Beej's Guide to Network Programming.
// http://beej.us/guide/bgnet/output/html/multipage/index.html
//

//
// Include files.
//
#include "Server.h"
#include "WISCEBridgeUtil.h"
#include "WISCEBridgeVersion.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <sstream>

//
// Global definitions
//

//
// To allow compilation for Windows redefine _snprintf as snprintf.
//
#ifdef _WIN32
//
// We have a few cases of while(1), which triggers "conditional expression
// is constant" warnings.
//
#pragma warning( disable: 4127 )

//
// We have to build on Linux as well, so we can't use the secure versions
// of the CRTL functions.
//
#pragma warning( disable: 4996 )
#endif

//
// We need to define INVALID_SOCKET as -1 for *nix.
//
#ifndef _WIN32
#define INVALID_SOCKET -1
#endif

// Ports < PORT_RESERVED are reserved for privileged processes (e.g. root).
#ifdef _WIN32
#define PORT_RESERVED       IPPORT_RESERVED
#else
#define PORT_RESERVED       1024
#endif // endif _WIN32

#define DEFAULTPORT         22349   // The port users will be connecting to by default.
#define MAX_PORTNAME_LENGTH 10      // The maximum length for a string representing a port.
#define BACKLOG             1       // How many pending connections queue will hold.
#define COMMAND_END_CHAR    '\n'    // The character denoting the end of a command.

// The message to send when we don't have a failure code.
#define GENERAL_FAILURE_MESSAGE "Error 63\n"

///////////////////////////////////////////////////////////////////////////////
///
//  Class:  Server
///
/// @brief  Creates the server socket and handles connections.
///
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
///
//  Constructor: CServer
///
/// @brief  Constructor for CServer
///
/// @param  argc    Argument count.
/// @param  argv    Arguments.
///
////////////////////////////////////////////////////////////////////////////////
CServer::CServer( int argc, char *argv[] )
    : m_commandHandler( argc, argv )    // document that this is initialised
{
    int index = 1;  // arg0 should be the basename; we don't care about it here.

    m_serverSocket = WMT_SOCKET_ERR;
    m_clientSocket = WMT_SOCKET_ERR;
    m_serverPort = DEFAULTPORT;

    // If the user has specified a port, use it.
    while ( index < argc )
    {
        const char *arg = argv[index];

        if ( 0 == strcmp( arg, "--port" ))
        {
            // The user has specified a port; its value should be the next arg.
            index++;
            if ( index < argc )
            {
                arg = argv[index];

                // Let's try and convert it to a number.
                char            *pStopChar = NULL;
                unsigned long   port = strtoul( arg, &pStopChar, 10 );
                if ( pStopChar == arg )
                {
                    // Couldn't parse the number.
                    InfoMessage( "Couldn't parse port \"%s\".\nUsing default port %u.\n",
                                 arg,
                                 m_serverPort
                               );
                }
                else
                {
                    //
                    // We have a number.
                    // Never use port zero - it is reserved - and the port
                    // must be an unsigned short value.
                    //
                    if ( 0 == port )
                    {
                        InfoMessage( "Reserved port (%u).\nUsing default port %u.\n",
                                     port,
                                     m_serverPort
                                   );
                    }
                    else if( port > USHRT_MAX )
                    {
                        InfoMessage( "Invalid port (%u).\nUsing default port %u.\n",
                                     port,
                                     m_serverPort
                                   );
                    }
                    else
                    {
                        m_serverPort = (unsigned short) port;

                        //
                        // If the user is trying to use a reserved port, warn
                        // them, but don't block them.
                        //
                        if ( m_serverPort < PORT_RESERVED )
                        {
                            InfoMessage( "WARNING: Ports less than %d are reserved for privileged processes (e.g. root).\n",
                                         PORT_RESERVED
                                       );
                        }

                        InfoMessage( "Using port %u.\n", m_serverPort );
                    }
                }
            }
            else
            {
                // Incorrect usage of --port.
                InfoMessage( "No port specified.\nUsage: --port <port>\nUsing default port %u.\n",
                             arg,
                             m_serverPort
                           );
            }
        }

        index++;
    }

#ifdef WIN32
    m_winSockActive = false;
#endif
}

////////////////////////////////////////////////////////////////////////////////
///
//  Destructor: ~CServer
///
/// @brief  Cleans up CServer.
///
////////////////////////////////////////////////////////////////////////////////
CServer::~CServer()
{
#ifdef WIN32
    if ( m_winSockActive )
    {
        //
        // Must cleanup WinSock.
        //
        WSACleanup();
        m_winSockActive = false;
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: Run
///
/// @brief  Sets up the socket and handles incoming connections.
///
/// (no parameters)
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_SOCKET_ERR  Failed.
/// @retval WMT_WINSOCK_ERR WinSock 2.2 not supported or failed to initialise.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CServer::Run()
{
    RESULT          result = WMT_SUCCESS;
    struct addrinfo hints, *pMyAddr;
    char            portName[MAX_PORTNAME_LENGTH];
    int             status;

#ifdef WIN32
    //
    // Must call WSAStartup first.
    //
    result = InitWSA();
    if ( WMT_SUCCESS != result )
    {
        SocketErrorMessage( "accept failed" );
        goto error;
    }
#endif

    // Load up address structs with getaddrinfo():
    memset( &hints, 0, sizeof( hints ) );
    hints.ai_family = AF_INET;          // Use IPv4 (allowing IPv6 with AF_UNSPEC breaks this for Win7)
    hints.ai_socktype = SOCK_STREAM;    // Stream socket.
    hints.ai_flags = AI_PASSIVE;        // Fill in my IP for me

    snprintf( portName, MAX_PORTNAME_LENGTH, "%d", m_serverPort );

    status = getaddrinfo( NULL, portName, &hints, &pMyAddr );
    if ( WMT_SUCCESS != status )
    {
        result = WMT_SOCKET_ERR;
        ErrorMessage( "getaddrinfo error: %s\n", gai_strerror( status ) );
        goto error;
    }

    // Make a socket, bind it, and listen on it:
    m_serverSocket = socket( pMyAddr->ai_family, pMyAddr->ai_socktype, pMyAddr->ai_protocol );
    if ( WMT_SOCKET_ERR == m_serverSocket )
    {
        result = WMT_SOCKET_ERR;
        SocketErrorMessage( "couldn't create socket" );
        goto error;
    }

    // Make sure we don't get hung up with a socket already in use:
#ifdef _WIN32
    {
        char yes = 1;
        result = (RESULT) setsockopt( m_serverSocket,
                                      SOL_SOCKET,
                                      SO_REUSEADDR,
                                      &yes,
                                      sizeof( char )
                                    );
    }
#else
    {
        int yes = 1;
        result = (RESULT) setsockopt( m_serverSocket,
                                      SOL_SOCKET,
                                      SO_REUSEADDR,
                                      &yes,
                                      sizeof( int )
                                    );
    }
#endif
    if ( WMT_SOCKET_ERR == result )
    {
        SocketErrorMessage( "setsockopt failed" );
        goto error;
    }

    result = (RESULT) bind( m_serverSocket, pMyAddr->ai_addr, pMyAddr->ai_addrlen );
    if ( WMT_SOCKET_ERR == result )
    {
        SocketErrorMessage( "bind failed" );
        goto error;
    }

    result = (RESULT) listen( m_serverSocket, BACKLOG );
    if ( WMT_SOCKET_ERR == result )
    {
        SocketErrorMessage( "listen failed" );
        goto error;
    }

    // Now accept incoming connections:
    while ( 1 )
    {
        DebugMessage( "Waiting for connection on port %u.\n", m_serverPort );

        m_clientSocket = accept( m_serverSocket, NULL, NULL );
        if ( INVALID_SOCKET == m_clientSocket )
        {
            result = WMT_SOCKET_ERR;
            SocketErrorMessage( "accept failed" );
            goto error;
        }

        DebugMessage( "Got connection.\n" );

        result = HandleConnection();
#ifndef _WIN32
        TEMP_FAILURE_RETRY( close( m_clientSocket ) );
#endif
        if ( WMT_SOCKET_CLOSED != result )
        {
            if ( WMT_CLOSE_SOCKET == result )
            {
#ifdef _WIN32
                closesocket( m_serverSocket );
#else
                TEMP_FAILURE_RETRY( close( m_serverSocket ) );
#endif
                goto done;
            }
        }
    }

done:
    if ( pMyAddr )
        freeaddrinfo( pMyAddr );
    return WMT_SUCCESS;

error:
    if ( pMyAddr )
        freeaddrinfo( pMyAddr );
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleConnection
///
/// @brief  Handles the connection.
///
/// @param  m_clientSocket    The client's socket descriptor to use.
///
/// @retval WMT_SOCKET_CLOSED   Success.
/// @retval WMT_CLOSE_SOCKET    The socket needs to be closed.
/// @retval WMT_SOCKET_ERR      Failure.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CServer::HandleConnection()
{
    RESULT      result = WMT_SUCCESS;
    std::string buffer;
    int         status;
    int         commandLength;
    CommandList commands;

    result = SendWelcome();
    if ( WMT_SOCKET_ERR == result )
        goto error;
    else if ( WMT_SOCKET_CLOSED == result )
        goto done;

    while ( 1 )
    {
        status = ReceiveCommand( commands );
        if ( WMT_SOCKET_ERR == status )
        {
#ifdef _WIN32
            int error = WSAGetLastError();
            if ( WSAECONNRESET == error )
            {
                InfoMessage( "Connection Lost\n" );
                result = WMT_SOCKET_CLOSED;
                goto done;
            }
#endif
            result = WMT_SOCKET_ERR;
            goto error;
        }
        else if ( WMT_SOCKET_CLOSED == status )
        {
            result = WMT_SOCKET_CLOSED;
            goto done;
        }

        // We should have received a command.
        assert( commands.size() > 0 );

        for ( CommandList::iterator it = commands.begin(); it != commands.end(); )
        {
            // Ignore incomplete commands. They will be processed later.
            if ( !it->complete )
            {
                ++it;
                continue;
            }

            buffer = it->buffer;
            DebugMessage( "CMD: %s", buffer.c_str() );

            // Our buffer length should equal our command string length.
            assert( buffer.length() == it->length );

            commandLength = m_commandHandler.HandleCommandString( &buffer );
            if ( WMT_CLOSE_SOCKET == commandLength )
            {
                result = WMT_CLOSE_SOCKET;
                goto error;
            }
            else if ( WMT_STRING_ERR == commandLength )
            {
                result = SendData( GENERAL_FAILURE_MESSAGE );
            }
            else if ( commandLength > 0 )
            {
                result = SendData( buffer.c_str(), commandLength );
                if ( WMT_SOCKET_ERR == result )
                    goto error;
                else if ( WMT_SOCKET_CLOSED == result )
                    goto done;
            }

            // This command has been handled, remove it and cleanup the memory.
            delete [] it->buffer;
            it = commands.erase( it );
        }
    }

done:
    return WMT_SOCKET_CLOSED;

error:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: SendData
///
/// @brief  Sends a message over the socket.
///
/// @param  message         The message to send.
/// @param  messageLength   The length of the message.
///
/// @retval >0                  Length of data sent.
/// @retval WMT_SOCKET_CLOSED   The socket is now closed.
/// @retval WMT_SOCKET_ERR      Failed.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CServer::SendData( const char *message, size_t messageLength )
{
    size_t  sentData = 0;
    int     status;
    int     dataRemaining = messageLength;
    int     flags = 0;

#ifndef _WIN32
    flags = MSG_NOSIGNAL;
#endif

    DebugMessage( "SND: %s", message );

    while ( sentData < messageLength )
    {
        status = send( m_clientSocket, &message[sentData], dataRemaining, flags );
        if ( WMT_SOCKET_ERR == status || WMT_SOCKET_CLOSED == status )
        {
            SocketErrorMessage( "send failed" );
            goto done;
        }

        sentData += status;

        if ( sentData < messageLength )
            dataRemaining = messageLength - sentData;
    }

done:
    return (RESULT) sentData;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ReceiveCommand
///
/// @brief  Receives data from the socket.
///
/// @param  commands    List of commands to be handled.
///
/// @retval >0                  Length of data received.
/// @retval WMT_SOCKET_CLOSED   The socket needs to be closed.
/// @retval WMT_SOCKET_ERR      Failed.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CServer::ReceiveCommand( CommandList &commands )
{
    int                 receivedData = 0;
    bool                commandReceived = false;
    size_t              bufferLength = INITIAL_BUFFER_LENGTH;
    int                 bufferRemaining = bufferLength;
    char                *pEnd = NULL;
    ptrdiff_t           length;
    int                 status = 0;
    char                *buffer = new char[INITIAL_BUFFER_LENGTH + 1];
    CBufferedCommand    command;
    char                *pStr;

    // Ensure the buffer is null-terminated.
    memset( buffer, 0, bufferLength );

    //
    // If there is an incomplete command to be processed copy it into the front
    // of the receive buffer.
    //
    if ( commands.size() )
    {
        assert( !commands.front().complete );

        memcpy( buffer, commands.front().buffer, commands.front().length );
        receivedData += commands.front().length;
        bufferRemaining -= commands.front().length;
        delete [] commands.front().buffer;
        commands.pop_front();

        assert( !commands.size() );
    }

    while ( !commandReceived )
    {
        status = recv( m_clientSocket,
                       &buffer[receivedData],
                       bufferRemaining,
                       0
                     );
        if ( WMT_SOCKET_ERR == status || WMT_SOCKET_CLOSED == status )
        {
#ifndef WIN32
            if ( EINTR == errno )
                continue;
#endif
            if ( WMT_SOCKET_ERR == status )
                SocketErrorMessage( "recv failed" );
            goto done;
        }

        //
        // Status contains the number of bytes read in by recv. Add the number of
        // received bytes to our counter, remove them from the remaining buffer
        // size.
        //
        receivedData += status;
        bufferRemaining -= status;

        // Bail out if we have at least 1 full command.
        pEnd = strchr( buffer, COMMAND_END_CHAR );
        if ( pEnd != NULL )
            commandReceived = true;

        // Is the buffer full?
        if ( !commandReceived && 0 == bufferRemaining )
        {
            // Buffer has filled up, we need to make it bigger.
            status = GrowBuffer( &buffer, &bufferLength );
            bufferRemaining = bufferLength - receivedData;
        }
    }

    //
    // Parse the received command buffer into a linked list of commands
    // to be handled later on.
    //
    pStr = buffer;
    do
    {
        if ( pEnd )
        {
            command.complete = true;
            // Work out the position of pEnd in the string.
            length = (ptrdiff_t) ( pEnd - pStr );
            assert( length >= 1 );
        }
        else
        {
            length = receivedData - (ptrdiff_t)( pStr - buffer ) - 1;
            if ( length <= 0 )
                break;

            command.complete = false;
        }

        //
        // Length is the length from the start of the string to the '\n'.
        // We want to include the '\n' with the rest of the command so add 1.
        //
        length++;

        //
        // Copy the command from the receive buffer into the command buffer.
        //
        command.buffer = new char[length + 1]; // +1 for the '\0'.
        memcpy( command.buffer, pStr, length );
        command.length = length;
        // Make sure it ends with \0.
        command.buffer[command.length] = '\0';

        // If we have \r\n get rid of the \r.
        if ( command.buffer[command.length - 2] == '\r' )
        {
            command.buffer[command.length - 2] = '\n';
            command.buffer[command.length - 1] = '\0';
            command.length --;
        }

#ifdef DEBUG
        Log( "[RCV: %s]", command.buffer );
#endif
        commands.push_back( command );

        // Move pStr to the end of the command we've just finished with.
        pStr += length;
        if ( pStr + 1 >= buffer + INITIAL_BUFFER_LENGTH )
            break;

        ++pStr;

        pEnd = strchr( pStr, COMMAND_END_CHAR );
    }
    while ( 1 );

    status = receivedData;

done:
    delete [] buffer;

    return (RESULT) status;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: SendWelcome
///
/// @brief  Sends the welcome message to the client.
///
/// (no parameters)
///
/// @retval >0                  Length of data sent.
/// @retval WMT_SOCKET_CLOSED   The socket is now closed.
/// @retval WMT_SOCKET_ERR      Failed.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CServer::SendWelcome()
{
    RESULT      result = WMT_SUCCESS;
    int         sentData = 0;
    char        buffer[INITIAL_BUFFER_LENGTH];
    int         bufferLength = INITIAL_BUFFER_LENGTH;
    std::string deviceBuffer;

    memset( buffer, 0, bufferLength );

    result = m_commandHandler.GetDeviceID( &deviceBuffer );
    if ( WMT_STRING_ERR == result )
        goto error;

    result = (RESULT) snprintf( buffer,
                                bufferLength,
                                "%s Version %s\nCopyright (c) %s Cirrus Logic\n%s\n",
                                APPNAME,
                                VERSION_STRA,
                                STRYEAR,
                                deviceBuffer.c_str()
                              );
    if ( WMT_STRING_ERR == result )
    {
        ErrorMessage( "Couldn't generate welcome string\n" );
        goto error;
    }

    sentData = SendData( buffer, strlen( buffer ) );
    if ( WMT_SOCKET_ERR == result )
    {
        result = WMT_SOCKET_ERR;
        goto error;
    }

    return (RESULT) sentData;

error:
    return result;
}

#ifdef WIN32
////////////////////////////////////////////////////////////////////////////////
///
//  Function: InitWSA
///
/// @brief  Initialises the WinSock library.
///
/// (no parameters)
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_WINSOCK_ERR Couldn't find usable WinSock DLL.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CServer::InitWSA()
{
    RESULT  result = WMT_SUCCESS;
    WORD    wVersionRequested;
    WSADATA wsaData;
    int     err;

    // We want WinSock version 2.2
    wVersionRequested = MAKEWORD( 2, 2 );

    err = WSAStartup( wVersionRequested, &wsaData );
    if ( WMT_SUCCESS != err )
    {
        result = WMT_WINSOCK_ERR;
        SocketErrorMessage( "WSAStartup failed" );
        goto done;
    }

    // Confirm we have version 2.2.
    if ( LOBYTE( wsaData.wVersion ) != 2 ||
         HIBYTE( wsaData.wVersion ) != 2
       )
    {
        WSACleanup();
        ErrorMessage( "Expected Winsock version 2.2; got %d.%d",
                      HIBYTE( wsaData.wVersion ),
                      LOBYTE( wsaData.wVersion )
                    );
        result = WMT_WINSOCK_ERR;
        goto done;
    }

    //
    // Set m_winSockActive to true so we know it can be cleaned up in the
    // destructor.
    //
    m_winSockActive = true;

done:
    return result;
}
#endif //WIN32

///////////////////////////////// END OF FILE //////////////////////////////////
