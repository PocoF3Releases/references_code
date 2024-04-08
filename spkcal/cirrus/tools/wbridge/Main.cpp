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
/// @file   Main.cpp
/// @brief  Argument parsing and various other initialisation checks.
///
/// @version \$Id: Main.cpp 18567 2016-08-08 14:32:32Z lbogdan $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////

//
// Include files
//
#include "Server.h"
#include "WISCEBridgeVersion.h"
#ifndef _WIN32
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <cstdio>
#endif
#include <cstdlib>

#ifndef _WIN32
#define LOCKFILE "wbridge.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
static int pid_fd;
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

#ifndef _WIN32
static void usage( const char *basename )
{
    fprintf( stderr,
             "%s Version %s\n%s\n",
             APPNAME,
             VERSION_STRA,
             COPYRIGHT_STRA
           );
    fprintf( stderr,
             "Usage: %s [--daemonize] [--help] [--port <port>] [[WMXXXX]|[--default <deviceName>]] [--max <hexRegAddr>] [-d <includeCodecPath>] [-x <excludeCodecPath>]\n",
             basename
           );
}

static int is_running( void )
{
    char buf[16];

    pid_fd = open( LOCKFILE, O_RDWR | O_CREAT, LOCKMODE );
    if ( pid_fd < 0 || flock( pid_fd, LOCK_EX | LOCK_NB ) < 0 ||
         ftruncate( pid_fd, 0 ) < 0
       )
    {
        goto out;
    }

    snprintf( buf, sizeof( buf ), "%jd\n", (intmax_t) getpid() );
    if ( write( pid_fd, buf, strlen( buf ) ) != (int) strlen( buf ) )
        goto out;
    return 0;

out:
    if ( pid_fd > -1 )
        TEMP_FAILURE_RETRY( close( pid_fd ) );
    pid_fd = -1;
    return 1;
}
#endif

////////////////////////////////////////////////////////////////////////////////
///
//  Function: main
///
/// @brief  The main function for the bridge.
///
/// @param  argc    Argument count.
/// @param  argv    Arguments.
///
/// @retval 0   Success.
/// @retval 1   Failure.
///
////////////////////////////////////////////////////////////////////////////////
int main( int argc, char *argv[] )
{
#ifndef _WIN32
    int         daemonize = 0;
    int         index = 0;
    const char  *basename;

    basename = argv[index];
    index++;
    while ( index < argc )
    {
        const char *arg = argv[index];
        if ( !strcmp( arg, "--daemonize" ))
        {
            daemonize = 1;
        }
        else if ( !strcmp( arg, "--help" ) )
        {
            usage( basename );
            exit( EXIT_SUCCESS );
        }
        else if ( !strcmp( arg, "--port" ) )
        {
            // Skip this and its value arg, we handle it in CServer.
            index++;
        }
        else if ( !strncasecmp( arg, "WM", 2 ) )
        {
            // Skip this, we handle it in CDeviceCommunication.
        }
        else if ( !strcasecmp( arg, "--default" ) )
        {
            // Skip this and its value arg, we handle it in CDeviceCommunication.
            index++;
        }
        else if ( !strcasecmp( arg, "--max" ) )
        {
            // Skip this and its value arg, we handle it in CDeviceCommunication.
            index++;
        }
        else if ( !strcasecmp( arg, "-d" ) )
        {
            // Skip this and its value arg, we handle it in CDeviceCommunication.
            index++;
        }
        else if ( !strcasecmp( arg, "-x" ) )
        {
            // Skip this and its value arg, we handle it in CDeviceCommunication.
            index++;
        }
        else
        {
            usage( basename );
            fprintf( stderr, "Unknown option: %s\n", arg );
            exit( EXIT_FAILURE );
        }
        index++;
    }

    if ( daemonize )
    {
        if ( is_running() )
        {
            fprintf( stderr, "Another wbridge process is already running\n" );
            exit( EXIT_FAILURE );
        }

        if ( daemon( 1, 0 ) < 0 )
        {
            perror( "daemon" );
            exit( EXIT_FAILURE );
        }
    }

    signal( SIGPIPE, SIG_IGN );

#endif

    //
    // Open our log file.  Note this doesn't return an error if it fails
    // since there's not much we can do about it.  No point aborting - we
    // may as well carry on.
    //
    OpenLogFile();

    RESULT result;

    //
    // Android doesn't support exceptions by default (supported in the NDK
    // toolchain r5+ with a particular C++ compiler flag; not supported by the
    // obsolete "arm-eabi-4.4.0" toolchain provided with the NDK for backwards
    // compatibility), so instead we use (nothrow) to ensure we get a NULL
    // CServer on failure.
    //
    // For more info, see:
    // http://stackoverflow.com/questions/6947038/do-the-c-operators-new-and-new-throw-stdbad-alloc-on-android
    //
    // We can't use (nothrow) generally, however, because it only applies to
    // throws made by new, not anything thrown by CServer. As such, we need to
    // use a try/catch for our non-Linux/Android implementations.
    //
    // For more info, see:
    // http://stackoverflow.com/questions/1877743/will-using-new-stdnothrow-mask-exceptions-thrown-from-a-constructor
    //
#ifdef _WIN32
    try
    {
        CServer app( argc, argv );

        result = app.Run();
    }
    catch ( std::bad_alloc )
    {
        ErrorMessage( "Failed to allocate memory. Try closing a few programs and restart WISCEBridge.\n" );
        result = WMT_NO_DEVICE;
    }
#else
    CServer *pApp = new (std::nothrow) CServer( argc, argv );
    if ( !pApp )
    {
        ErrorMessage( "Failed to allocate memory. Try closing a few programs and restart WISCEBridge.\n" );
        result = WMT_NO_DEVICE;
    }

    result = pApp->Run();

    if ( pApp )
    {
        free( pApp );
        pApp = NULL;
    }
#endif

    //
    // Close our log file again.
    //
    CloseLogFile();

    if ( WMT_SUCCESS == result )
        exit( EXIT_SUCCESS );
    else
        exit( EXIT_FAILURE );
}

///////////////////////////////// END OF FILE //////////////////////////////////