////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2011-2017 Cirrus Logic International (UK) Ltd.  All rights reserved.
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
/// @file   WISCEBridgeUtil.cpp
/// @brief  Utility functions for WISCEBridge.
///
/// @version \$Id: WISCEBridgeUtil.cpp 19717 2017-10-31 11:21:28Z stankic $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
//#pragma managed( push, off )

//
// Include files.
//
#include "WISCEBridgeUtil.h"
#include "WISCEBridgeVersion.h"
#include "QDSPInfo.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <time.h>
#include <cctype>
#include "CommandHandler.h"
#include "StringUtil.h"

#ifndef _WIN32
#include <sys/time.h>
#endif

//
// Global definitions
//

#ifdef _WIN32
#include <shlobj.h>     // For SHGetFolderPathAndSubdir

//
// We have to build on Linux as well, so we can't use the secure versions
// of the CRTL functions.
//
//#pragma warning( disable: 4996 )
#endif

// The maximum length of a debug message.
#define MAX_DEBUG_MESSAGE       4096

/// How we format a time in logs
#define LOG_TIME_FORMAT "%02d-%02d-%04d %02d:%02d:%02d.%03d "

//
// Our log file.
//
static FILE *sg_logFile = NULL;
#define LOG_FILE_NAME           APPNAME ".log"
#define DEFAULT_APP_SUBFOLDER   "Wolfson Evaluation Software"

// Maximum length of a bit width number.
#define MAX_BITWIDTH_LENGTH 4

// Maximum length of a DSP index number
#define MAX_DSP_INDEX_LENGTH 2

// Driver file name defines
#define DSP_SUFFIX "-dsp"

#ifdef WIN32
#   define FOLDER_SEP   "\\"
#else
#   define FOLDER_SEP   "/"
#endif

//
// Function prototypes.
//
#ifdef WIN32
static RESULT GetAppDataFolder( char *buffer, int buflen );
static RESULT GetModuleFolder( char *buffer, int buflen );
#endif // WIN32

////////////////////////////////////////////////////////////////////////////////
///
//  Function: OpenLogFile
///
/// @brief  Opens the log file.
///
/// (no parameters)
///
/// @return void.  Note this doesn't actually return an error if it fails.
///
////////////////////////////////////////////////////////////////////////////////
void OpenLogFile()
{
    char    buffer[MAX_DEBUG_MESSAGE];
    size_t  buflen = sizeof( buffer ) / sizeof( buffer[0] );
    GetLogFolder( buffer, buflen );
    const char *fileName = FOLDER_SEP LOG_FILE_NAME;
    char *logFileName = (char *) malloc( strlen( buffer ) + strlen( fileName ) );
    sprintf( logFileName, "%s%s", buffer, fileName );

#ifdef _WIN32
    //
    // Open the log file with sharing on WIN32 so WISCE crash dump can access it
    // if something goes wrong.
    //
    sg_logFile = _fsopen( logFileName, "wt", _SH_DENYNO );  // write, text
#else
    sg_logFile = fopen( logFileName, "wt" );    // write, text
#endif
    //
    // Now that we potentially have a log file set up, display a startup message
    // with the WISCEBridge version and copyright, then display the appropriate
    // log file startup message.
    //
    InfoMessage( "%s Version %s\nCopyright (c) %s Cirrus Logic\n",
                 APPNAME,
                 VERSION_STRA,
                 STRYEAR
               );

    if ( sg_logFile )
    {
        Log( "*** %s log file started ***\n\n", APPNAME );
        InfoMessage( "Log file saved to %s\n", logFileName );
    }
    else
    {
        fprintf( stderr,
                 "Couldn't open log file %s: %s\n",
                 LOG_FILE_NAME,
                 strerror( errno )
               );
    }
    if ( logFileName )
        free( logFileName );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: CloseLogFile
///
/// @brief  Closes the log file again.
///
/// (no parameters)
///
/// @return void.
///
////////////////////////////////////////////////////////////////////////////////
void CloseLogFile()
{
    if ( sg_logFile )
    {
        Log( "\n*** %s log file closed ***\n", APPNAME );
        fclose( sg_logFile );
        sg_logFile = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: Log
///
/// @brief  Prints a message to the log file.
///
/// @param  format  The message to print.
/// @param  ...     Printf args.
///
/// @return void.
///
////////////////////////////////////////////////////////////////////////////////
void Log( const char *format, ... )
{
    va_list args;
    char    message[MAX_DEBUG_MESSAGE];
    memset( message, 0, MAX_DEBUG_MESSAGE );

    va_start( args, format );
    vsnprintf( message, MAX_DEBUG_MESSAGE, format, args );
    va_end( args );

    LogM( message );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: LogM
///
/// @brief  Prints a message to the log file.
///
/// @param  message The message to print.
///
/// @return void.
///
////////////////////////////////////////////////////////////////////////////////
void LogM( const char *message )
{
    if ( sg_logFile )
    {
        char logMessage[MAX_DEBUG_MESSAGE];
        memset( logMessage, 0, MAX_DEBUG_MESSAGE );

#ifdef _WIN32
        SYSTEMTIME  sysNow;
        GetLocalTime( &sysNow );

        // Our log message includes a timestamp.
        snprintf( logMessage,
                  sizeof( logMessage ),
                  LOG_TIME_FORMAT "* %s",
                  sysNow.wDay,
                  sysNow.wMonth,
                  sysNow.wYear,
                  sysNow.wHour,
                  sysNow.wMinute,
                  sysNow.wSecond,
                  sysNow.wMilliseconds,
                  message
                );

#else
        // Output a timestamp to precede the log message.
        timeval tp;
        time_t  timeNow;
        tm      *timeInfo;

        gettimeofday( &tp, 0 );
        timeNow = tp.tv_sec;
        timeInfo = localtime( &timeNow );

        snprintf( logMessage,
                  sizeof( logMessage ),
                  LOG_TIME_FORMAT "* %s",
                  timeInfo->tm_mday,
                  1 + timeInfo->tm_mon,      // Month given as 0-11
                  1900 + timeInfo->tm_year,  // Year given as years since 1900
                  timeInfo->tm_hour,
                  timeInfo->tm_min,
                  timeInfo->tm_sec,
                  (int) tp.tv_usec/1000,
                  message
               );
#endif

        // Output our log message to the log file.
        fputs( logMessage, sg_logFile );
        fflush( sg_logFile );
    }
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: InfoMessage
///
/// @brief  Outputs an informational message.
///
/// @param  format  The message to print.
/// @param  ...     Printf args.
///
/// @return void.
///
////////////////////////////////////////////////////////////////////////////////
void InfoMessage( const char *format, ... )
{
    va_list args;
    char    message[MAX_DEBUG_MESSAGE];
    memset( message, 0, MAX_DEBUG_MESSAGE );

    va_start( args, format );
    vsnprintf( message, MAX_DEBUG_MESSAGE, format, args );
    va_end( args );

    printf( "%s", message );
    LogM( message );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: DebugMessage
///
/// @brief  Prints a debug message to the screen.
///
/// Only prints the debug message if DEBUG is defined.
///
/// @param  format  The message to print.
/// @param  ...     Printf args.
///
/// @return void.
///
////////////////////////////////////////////////////////////////////////////////
void DebugMessage( const char *format, ... )
{
    va_list args;
    char    message[MAX_DEBUG_MESSAGE];
    memset( message, 0, MAX_DEBUG_MESSAGE );

    va_start( args, format );
    vsnprintf( message, MAX_DEBUG_MESSAGE, format, args );
    va_end( args );

#ifdef DEBUG
    printf( "%s", message );
#endif
    LogM( message );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ErrorMessage
///
/// @brief  Prints an error message to the screen.
///
/// @param  format  The message to print.
/// @param  ...     Printf args.
///
/// @return void.
///
////////////////////////////////////////////////////////////////////////////////
void ErrorMessage( const char *format, ... )
{
    va_list args;
    char    message[MAX_DEBUG_MESSAGE];
    memset( message, 0, MAX_DEBUG_MESSAGE );

    va_start( args, format );
    vsnprintf( message, MAX_DEBUG_MESSAGE, format, args );
    va_end( args );

    fprintf( stderr, "%s", message );
    LogM( message );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: PrintError
///
/// @brief  Local equivalent of perror which also logs the error.
///
/// @param  message The message to print, appended with the error string.
///
/// @return void.
///
////////////////////////////////////////////////////////////////////////////////
void PrintError( const char *message )
{
    ErrorMessage( "%s: %s\n", message, strerror( errno ) );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: SocketErrorMessage
///
/// @brief  Prints an error message to the screen, appending the socket error.
///
/// Since the output appends the socket error, the format string should not
/// finish in a carriage return.
///
/// @param  format  The message to print.
/// @param  ...     Printf args.
///
/// @return void.
///
////////////////////////////////////////////////////////////////////////////////
void SocketErrorMessage( const char *format, ... )
{
    char    buffer[MAX_DEBUG_MESSAGE];
    va_list args;
    int     socketError;

    va_start( args, format );
    memset( buffer, 0, MAX_DEBUG_MESSAGE );
    vsnprintf( buffer, MAX_DEBUG_MESSAGE, format, args );

#ifdef WIN32
    socketError = WSAGetLastError();
#else
    socketError = errno;
#endif

    ErrorMessage( "%s: %d\n", buffer, socketError );
}


////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetLogFolder
///
/// @brief  Returns the folder which should be used for log file output.
///
/// This will typically be the user's AppData folder on Windows.
///
/// @param  buffer [out]    The buffer to receive the folder.
/// @param  buflen          The length of the buffer in characters.
///
/// @retval WMR_SUCCESS         Succeeded
/// @retval WMR_TRUNCATED       Buffer too small
/// @retval WMR_FILE_NOT_FOUND  Couldn't work out the folder
/// @retval WMR_FAILURE         Unexpected error
///
////////////////////////////////////////////////////////////////////////////////
RESULT GetLogFolder( char *buffer, int buflen )
{
    RESULT   result = WMT_SUCCESS;

#ifdef WIN32

    //
    // Try the Appdata folder first.
    //
    result = GetAppDataFolder( buffer, buflen );
    if ( WMT_SUCCESS != result )
    {
        //
        // Couldn't get local AppData folder - just use the module folder instead.
        //
        result = GetModuleFolder( buffer, buflen );
    }
#else
    snprintf( buffer, buflen, "./"  );
#endif

    return result;
}

#ifdef WIN32
////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetAppDataFolder
///
/// @brief  Returns the folder which should be used for application data.
///
/// This will typically be the user's AppData folder on Windows.
///
/// @param  buffer [out]    The buffer to receive the folder.
/// @param  buflen          The length of the buffer in characters.
///
/// @retval WMR_SUCCESS     Succeeded
/// @retval WMR_TRUNCATED   Buffer too small
/// @retval WMR_FAILURE     Unexpected error
///
////////////////////////////////////////////////////////////////////////////////
static RESULT GetAppDataFolder( char *buffer, int buflen )
{
    RESULT      result;
    HRESULT     hr;
    char        folderName[MAX_PATH] = { 0 };
    errno_t     err;
    const char  *subdir = DEFAULT_APP_SUBFOLDER;

    hr = SHGetFolderPathAndSubDirA( NULL,                // hWnd
                                    CSIDL_APPDATA | CSIDL_FLAG_CREATE,
                                                         // csidl - create
                                    NULL,                // hToken (user)
                                    SHGFP_TYPE_CURRENT,  // get current path
                                    subdir,              // subdir
                                    folderName           // [out] receives folder
                                  );
    if ( SUCCEEDED( hr ) )
    {
        err = strncpy_s( buffer, buflen, folderName, _TRUNCATE );
        if ( err )
        {
            result = WMT_STRING_ERR;

            goto done;
        }

        result = WMT_SUCCESS;
    }
    else
    {
        result = WMT_STRING_ERR;
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetModuleFolder
///
/// @brief  Returns the folder where this app is located.
///
/// @param  buffer [out]    The buffer to receive the folder.
/// @param  buflen          The length of the buffer in characters.
///
/// @retval WMR_SUCCESS         Succeeded
/// @retval WMR_TRUNCATED       Buffer too small
/// @retval WMR_FILE_NOT_FOUND  Couldn't work out the folder
/// @retval WMR_FAILURE         Unexpected error
///
////////////////////////////////////////////////////////////////////////////////
static RESULT GetModuleFolder( char *buffer, int buflen )
{
    RESULT      result = WMT_SUCCESS;
    char        path[MAX_PATH];
    int         pathlen = sizeof( path ) / sizeof( path[0] );

    memset( path, 0, sizeof( path ) );

    if ( GetModuleFileNameA( NULL, path, pathlen ) > 0 )
    {
        char *lastDirSep = strrchr( path, '\\' );
        if ( lastDirSep )
            *lastDirSep = '\0';

        errno_t err = strncpy_s( buffer, buflen, path, _TRUNCATE );
        if ( err )
        {
            result = WMT_STRING_ERR;
        }
    }
    else
    {
        result = WMT_NO_DEVICE;
    }

    return result;
}
#endif  // WIN32

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ParseWMCodec
///
/// @brief  Parses the codec name for a WM*/codec_reg codec path.
///
/// This handles retrieving the codec name for a 'codec_reg' file, which should
/// be present in the filepath. The name should be given as WM* where we read
/// the name as 'WM' up to the next non-alpha-numeric character. For example,
/// we parse the codec name 'WM8994' from
/// /sys/kernel/debug/asoc/WM8994/codec_reg.
///
/// The codec name for a 'registers' file is not necessarily found in the
/// filepath, and must be handled specific to the device communications by
/// reading a 'name' file which should be present in the same directory and
/// will contain the name. For example, for
/// /sys/kernel/debug/regmap/spi0.1/registers we read from
/// /sys/kernel/debug/regmap/spi0.1/name to get the codec name 'arizona'.
///
/// @param  path        The codec path to parse.
/// @param  buffer      Receives the name of the codec from the path.
/// @param  bufferSize  The size of the buffer.
///
/// @retval WMT_SUCCESS             Successfully parsed the codec name.
/// @retval WMT_TRUNCATED           Buffer too small - the name has been truncated.
/// @retval WMT_INVALID_PARAMETER   Path or buffer was NULL.
/// @retval WMT_NO_DEVICE           Not a WM device codec.
///
////////////////////////////////////////////////////////////////////////////////
RESULT ParseWMCodec( const char *path, char *buffer, size_t bufferSize )
{
    RESULT      result = WMT_NO_DEVICE;
    const char  *cPtr = path;
    bool        found = false;

    if ( !path || !buffer )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    // Look for WM - a path that contains the codec file should also contain WM.
    do
    {
        if ( 'w' == cPtr[0] || 'W' == cPtr[0] )
        {
            if ( 'm' == cPtr[1] || 'M' == cPtr[1] )
            {
                // Note: we want WM as part of the name, so don't skip over it.
                found = true;
                break;
            }
        }
        cPtr++;
    } while ( '\0' != *cPtr );

    //
    // Pull out the device name from the path. If we've found WM, we should now
    // be looking at WM*/CODEC_REG - we want the codec name, which is from WM to
    // the character before the next non-alpha-numeric character e.g. WM8994
    // from WM8994/CODEC_REG.
    //
    if ( found )
    {
        size_t index = 0;
        result = WMT_SUCCESS;

        while ( index < bufferSize && IsAlphanumeric( *cPtr ) )
            buffer[index++] = *cPtr++;

        // Ensure we null-terminate.
        if ( bufferSize == index )
        {
            result = WMT_TRUNCATED;
            index = bufferSize - 1;
        }
        buffer[index] = '\0';
    }
    else
    {
        DebugMessage( "Ignoring %s. Not a Cirrus codec!\n", path );
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: StoreCodec
///
/// @brief  Stores the codec in the given collection unless it is a duplicate or to be excluded.
///
/// @param  pCodecPaths      The collection of codec paths to be updated.
/// @param  codec            The name of the codec to add.
/// @param  path             The path of the codec to add.
/// @param  pExcludeDevices  The collection of codec names to exclude
///                          (will override includes).
/// @param  pIncludeDevices  The collection of codec names to include.
///
/// @retval WMT_SUCCESS             Successfully stored the codec.
/// @retval WMT_INVALID_PARAMETER   One or more of the parameters was NULL.
/// @retval WMT_DUPLICATED          A codec by the same name was already in the
///                                 collection; the collection hasn't been changed.
/// @retval WMT_EXCLUDED            The codec matches a specifier on the exclusion
///                                 list; the collection hasn't been changed.
///
////////////////////////////////////////////////////////////////////////////////
RESULT StoreCodec( std::vector<CCodecPathPair>      *pCodecPaths,
                   char                             *codec,
                   const char                       *path,
                   const std::vector<std::string>   *pExcludeDevices,
                   const std::vector<std::string>   *pIncludeDevices
                 )
{
    RESULT          result = WMT_SUCCESS;
    CCodecPathPair  cpath;
    char            *p;
    int             width = 0;
    int             dspIndex = 0;
    char            identifier[PATH_MAX];
    std::string     nameWithIdentifier = "";

    if ( !pCodecPaths || !codec || !path )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    // Ensure we don't include any line-breaks in the codec name.
    p = strchr( codec, '\n' );
    if ( p )
        *p = '\0';
    p = strchr( codec, '\r' );
    if ( p )
        *p = '\0';

    //
    // Check whether the codec is on the included or excluded lists
    // - reject or continue accordingly.
    //
    // If we have excluded devices, and one of them matches the filename,
    // ignore this file.
    //
    if ( NULL != pExcludeDevices && pExcludeDevices->size() > 0 )
    {
        for ( size_t i = 0; i < pExcludeDevices->size(); i++ )
        {
            if ( 0 != stristr( codec, ( *pExcludeDevices )[i].c_str() ) )
            {
                // It matched. Exclude it.
                InfoMessage( "Ignoring %s codec because it matches %s\n",
                             codec,
                             ( *pExcludeDevices )[i].c_str()
                           );

                result = WMT_EXCLUDED;
                goto done;
            }
        }
    }

    //
    // If we have included devices, ignore this file unless one of them
    // matches the filename.
    //
    if ( NULL != pIncludeDevices && pIncludeDevices->size() > 0 )
    {
        bool matched = false;

        for ( size_t i = 0; i < pIncludeDevices->size(); i++ )
        {
            if ( 0 != stristr( codec, ( *pIncludeDevices )[i].c_str() ) )
            {
                matched = true;
                break;
            }
        }

        if ( !matched )
        {
            // Nothing matched. Exclude it.
            InfoMessage( "Ignoring %s codec because it didn't match\n",
                         codec
                       );

            result = WMT_EXCLUDED;
            goto done;
        }
    }

    // See if we can get a width from the path.
    width = GetRegisterPathBytes( path );

    // See if we can get a DSP index from the path.
    dspIndex = GetDSPIndexFromPath( path );

    //
    // Get the unique name for the codec. This is only for the newer
    // drivers not under the asoc folder, and not the QDSP driver.
    //
    nameWithIdentifier += codec;
    if ( !strstr( path, "asoc" ) && !strstr( path, QDSP_DRIVER_FILE ) )
    {
        GetPathIdentifier( path, identifier, PATH_MAX );
        nameWithIdentifier += "_";
        nameWithIdentifier += identifier;
    }

    // Check we don't already have a codec with the same name.
    for ( size_t i = 0; i < pCodecPaths->size(); i++ )
    {
        if ( 0 == strcmp( ( *pCodecPaths )[i].name, nameWithIdentifier.c_str() ) &&
             width == (*pCodecPaths)[i].registerBytes &&
             dspIndex == ( *pCodecPaths )[i].dspIndex
           )
        {
            result = WMT_DUPLICATED;
            break;
        }
    }

    if ( WMT_SUCCESS == result )
    {
        //
        // Fill the CCodecPathPair and add it to our vector of codecs.
        //
        snprintf( cpath.name, sizeof( cpath.name ), "%s", nameWithIdentifier.c_str() );
        snprintf( cpath.path, sizeof( cpath.path ), "%s", path );
        cpath.registerBytes = width;
        cpath.dspIndex = dspIndex;
        pCodecPaths->push_back( cpath );

        InfoMessage( "Found %s codec: %s\n", nameWithIdentifier.c_str(), path );
    }
    else
    {
        InfoMessage( "%s is a duplicate codec, ignoring %s\n", nameWithIdentifier.c_str(), path );
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetPathIdentifier
///
/// @brief  Pulls the path identifier out of the given path.
///
/// @param  path            Path to pull the identifier out of.
/// @param  buffer          Buffer to receive identifier.
/// @param  bufferLength    Length of the buffer.
///
/// @retval WMT_SUCCESS             Successfully retreived the identifier.
/// @retval WMT_INVALID_PARAMETER   One or more of the parameters was NULL.
/// @retval WMT_TRUNCATED           Buffer was too short.
///
////////////////////////////////////////////////////////////////////////////////
RESULT GetPathIdentifier( const char *path, char *buffer, size_t bufferLength )
{
    RESULT  result = WMT_SUCCESS;
    char    *copy = NULL;
    char    *pos = NULL;
    char    *first = NULL, *last = NULL;
    int     length = 0;

    // Make sure we have our inputs.
    if ( !path || !buffer || 0 == bufferLength )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    // Copy the path to work with.
    copy = strdup( path );

    pos = strstr( copy, "/" );

    // Move through the path to get the last folder.
    while ( pos )
    {
        first = last;
        last = ++pos;
        pos = strstr( last, "/" );
    }

    // If the path doesn't have two '/' characters we can't extract the identifier.
    if ( !first || !last || first == last )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    // Make sure the identifier ends before the last /.
    *( --last ) = '\0';

    //
    // If there is an underscore in the identifier, we want to ignore everything
    // past it as it will just be the bit count of the registers.
    //
    pos = strstr( first, "_" );
    if ( pos )
        *pos = '\0';
    //
    // If there is a DSP suffix in the identifier, we want to ignore everything
    // past it.
    //
    pos = strstr( first, DSP_SUFFIX );
    if ( pos )
        *pos = '\0';

    //
    // If the buffer isn't long enough, return WMT_TRUNCATED, we still want to
    // fill it though.
    //
    if ( bufferLength <= strlen( first ) )
        result = WMT_TRUNCATED;

    length = snprintf( buffer, bufferLength, "%s", first );
    // If truncated the '\0' will not have been added.
    if ( length < 0 || length == (int) bufferLength )
        buffer[bufferLength - 1] = '\0';

done:
    if ( copy )
    {
        delete copy;
        copy = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ParseReadValue
///
/// @brief  Sets the value read from the given register line.
///
/// @param  regAddr         The register that was read.
/// @param  registerLine    Buffer containing the the register line read.
/// @param  pValue          Receives the value read.
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   Register line buffer was NULL.
/// @retval WMT_READ_FAILED         Unable to parse the register line. Either
///                                 the device is in low power mode or the line
///                                 is in an unrecognised format.
///
////////////////////////////////////////////////////////////////////////////////
RESULT ParseReadValue( unsigned int regAddr,
                       const char   *registerLine,
                       unsigned int *pValue
                     )
{
    RESULT          result = WMT_SUCCESS;
    unsigned int    localRegAddr = 0;
    unsigned int    localValue = 0;
    const char      *pChar = NULL;

    // We need a buffer to do anything.
    if ( !registerLine )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    //
    // If the line begins with a command prompt, we need to cut it off.
    //
    pChar = strchr( registerLine, '#' );
    if ( pChar )
        pChar++;
    else
        pChar = registerLine;

    //
    // Try to parse the register and its value from the register line. If this
    // fails, there are two possibilities:
    // - The driver will read XXXX for registers which must be read from the
    //   device if the device is powered down. This is normal and expected
    //   behaviour when we are in low power mode.
    // - The codec file is in an unrecognised format (or corrupted).
    // In either case, WISCEBridge should return an error to the effect that
    // the register can't be read.
    //
    if ( 2 != sscanf( pChar, "%x: %x", &localRegAddr, &localValue ) )
    {
        char tmpBuffer[PATH_MAX];
        char *pLineEnd;

        // Get the line as-is for the log message.
        memccpy( tmpBuffer, pChar, 0, sizeof( tmpBuffer ) );
        pLineEnd = strchr( tmpBuffer, '\n' );
        if ( pLineEnd )
            *pLineEnd = '\0';

        Log( "Unable to read R%Xh from \"%s\"\n", regAddr, tmpBuffer );
        result = WMT_READ_FAILED;
        goto done;
    }

#ifdef DEBUG
    if ( localRegAddr != regAddr )
    {
        Log( "Error: register line for R%Xh, received R%Xh\n",
             regAddr,
             localRegAddr
           );
    }
#endif

    if ( pValue )
        *pValue = localValue;

    Log( "R%Xh = %Xh\n", regAddr, *pValue );

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: AddRegisterToLayout
///
/// @brief  Adds the register from the line to the given register layout.
///
/// @param  pRegisterLayout     The register layout to be updated.
/// @param  registerLine        Buffer containing the the register line read.
/// @param  totalOffset         Byte offset where the register line starts in
///                             the file.
/// @param  lineLength          The length of the register line (including the
///                             line break).
/// @param  lineNumber          The number of the line in the file.
/// @param  maxRegister         The maximum register address to use for the
///                             layout (optional).
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   Register layout or register line buffer was NULL.
/// @retval WMT_OUT_OF_RANGE        Register exceeds the maximum register index.
/// @retval WMT_DUPLICATED          Register already exists in the map; ignored.
/// @retval WMT_REG_NOT_PRESENT     Unable to parse a register address.
///
////////////////////////////////////////////////////////////////////////////////
RESULT AddRegisterToLayout( RegisterLayout  *pRegisterLayout,
                            const char      *registerLine,
                            size_t          totalOffset,
                            size_t          lineLength,
                            size_t          lineNumber,
                            unsigned int    maxRegisterIndex /* = MAX_REGISTER */
                          )
{
    RESULT          result = WMT_SUCCESS;
    int             fieldCount;
    unsigned int    regAddr = 0;
    char            colonStr[2];

    if ( !pRegisterLayout || !registerLine )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    //
    // If we have the correct count from the scan, we have a register line.
    // Store its offset and length in the register layout.
    //
    fieldCount = sscanf( registerLine, "%x%1[:]", &regAddr, colonStr );
    if ( 2 == fieldCount )
    {
        result = AddRegisterToLayout( pRegisterLayout,
                                      regAddr,
                                      totalOffset,
                                      lineLength,
                                      lineNumber,
                                      maxRegisterIndex
                                    );
    }
    else
    {
        // Unable to parse a register address - not a register line.
        result = WMT_REG_NOT_PRESENT;
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: AddRegisterToLayout
///
/// @brief  Adds the register's line info to the given register layout.
///
/// @param  pRegisterLayout     The register layout to be updated.
/// @param  regAddr             The register associated with the line info.
/// @param  totalOffset         Byte offset where the register line starts in
///                             the file.
/// @param  lineLength          The length of the register line (including the
///                             line break).
/// @param  lineNumber          The number of the line in the file.
/// @param  maxRegister         The maximum register address to use for the
///                             layout (optional).
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   Register layout was NULL.
/// @retval WMT_OUT_OF_RANGE        Register exceeds the maximum register index.
/// @retval WMT_DUPLICATED          Register already exists in the map; ignored.
///
////////////////////////////////////////////////////////////////////////////////
RESULT AddRegisterToLayout( RegisterLayout  *pRegisterLayout,
                            unsigned int    regAddr,
                            size_t          totalOffset,
                            size_t          lineLength,
                            size_t          lineNumber,
                            unsigned int    maxRegisterIndex /* = MAX_REGISTER */
                          )
{
    RESULT                  result = WMT_SUCCESS;
    const RegisterLineInfo  *pLineInfo = NULL;

    if ( !pRegisterLayout )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    // If we've passed the maximum register we care about, we're done.
    if ( MAX_REGISTER != maxRegisterIndex && regAddr > maxRegisterIndex )
    {
        result = WMT_OUT_OF_RANGE;
        goto done;
    }

    // If the RegisterLayout is not yet at the initial capacity, set it.
    if ( pRegisterLayout->capacity() < INITIAL_CAPACITY )
    {
        DebugMessage( "Setting initial capacity to %d.\n", INITIAL_CAPACITY );
        pRegisterLayout->reserve( INITIAL_CAPACITY );
    }

    // Add the register line to our layout if we don't already have it.
    pLineInfo = FindInLayout( pRegisterLayout, regAddr );
    if ( !pLineInfo )
    {
        RegisterLineInfo lineInfo = { regAddr,      // The register address for the line.
                                      (unsigned int)totalOffset,  // Offset: the length so far.
                                      (unsigned int)lineLength,   // Length; the given length should already include any line breaks.
                                      (unsigned int)lineNumber    // Line number.
                                    };
        pRegisterLayout->push_back( lineInfo );
    }
    else
    {
        result = WMT_DUPLICATED;
        DebugMessage( "Duplicate entry for R%Xh at offset %d; keeping offset %d\n",
                      regAddr,
                      totalOffset,
                      pLineInfo->offset
                    );
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetBytesPerRegister
///
/// @brief  Determines the bytes per register from the given register line.
///
/// Note: we work up to 64-bit (8-byte) values, but we handle them as 64-bit
/// (8-byte), 32-bit (4-byte), 16-bit (2-byte), and 8-bit (1-byte) values, as
/// that's how we deal with them via WISCE.
///
/// @param  registerLine        Buffer containing the the register line to use.
/// @param  pBytesPerRegister   Receives the bytes per register.
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   Register line buffer was NULL.
/// @retval WMT_OUT_OF_RANGE        Value was larger than 64-bits (8 bytes).
/// @retval WMT_REG_NOT_PRESENT     Unable to process the register line. The
///                                 line is in an unrecognised format.
///
////////////////////////////////////////////////////////////////////////////////
RESULT GetBytesPerRegister( const char  *registerLine,
                            size_t      *pBytesPerRegister
                          )
{
    RESULT          result = WMT_SUCCESS;
    const char      *cPtr;
    unsigned int    regAddr;
    char            colonStr[2];
    size_t          digits = 0;
    unsigned int    bytesPerRegister = 0;

    // We need a register line to do anything.
    if ( !registerLine )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    //
    // If this is a register line, we can work out the count of bytes per
    // register from its value part.
    //
    if ( 2 != sscanf( registerLine, "%x%1[:]", &regAddr, colonStr ) )
    {
        Log( "Unable to work out bytes per register from \"%s\"\n", registerLine );
        result = WMT_REG_NOT_PRESENT;
        goto done;
    }

    //
    // Get the value part of the register line and eat all blanks (usually just one).
    //
    cPtr = strchr( registerLine, ':' );
    cPtr++; // We know we have a ':' from the sscanf.
    while ( ' ' == *cPtr || '\t' == *cPtr )
        cPtr++;

    //
    // Work out the count of digits in the value; allow X 'digits' for a
    // possible low-power-mode value.
    //
    while( IsHexDigit( *cPtr ) || 'X' == *cPtr || 'x' == *cPtr )
    {
        cPtr++;
        digits++;
    }

    //
    // The value part of a register line in a codec file is always going to
    // have count of digits that's a multiple of 2 characters (i.e. padded to
    // represent full bytes, as 2 characters represent a byte), whether in
    // low-power mode or not. Still, make sure we round up.
    //
    bytesPerRegister = ( digits + 1 ) / 2;

    Log( "Detected %d bytes per register from register line \"%s\"\n",
         bytesPerRegister,
         registerLine
       );

    //
    // We work up to 64-bit (8-byte) values, but we handle them as 64-bit
    // (8-byte), 32-bit (4-byte), 16-bit (2-byte), and 8-bit (1-byte) values,
    // as that's how we deal with them via WISCE.
    //
    if ( bytesPerRegister > 8 )
    {
        Log( "Values larger than 64-bits (8-bytes) are not supported. Treating as 64-bit (8-byte) values." );
        result = WMT_OUT_OF_RANGE;
        bytesPerRegister = 8;
    }
    else if ( bytesPerRegister < 8 && bytesPerRegister > 4 )
    {
        Log( "Treating as 64-bit (8-byte) values." );
        bytesPerRegister = 8;
    }
    else if ( 3 == bytesPerRegister )
    {
        Log( "Treating as 32-bit (4-byte) values." );
        bytesPerRegister = 4;
    }

    if ( pBytesPerRegister )
        *pBytesPerRegister = bytesPerRegister;

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetRegisterPathBytes
///
/// @brief  Gets the number of bytes from the register path.
///
/// @param  path The path to get the register byte width from.
///
/// @retval -1      Path contained no bit width value.
/// @retval other   The byte width of the registers.
///
////////////////////////////////////////////////////////////////////////////////
int GetRegisterPathBytes( const char *path )
{
    const char *ptr = path;
    int bytes = -1;

    ptr = strstr( ptr, "_" );
    while ( ptr )
    {
        const char *startPointer = ptr + 1;
        const char *endPointer = strstr( ptr, "bit" );
        if ( endPointer && endPointer > startPointer )
        {
            char subString[MAX_BITWIDTH_LENGTH];
            size_t index = 0;

            while ( index < MAX_BITWIDTH_LENGTH &&
                    IsDigit( *startPointer ) &&
                    startPointer < endPointer
                  )
            {
                subString[index++] = *startPointer++;
            }

            // Ensure we haven't an empty buffer or maxed out the buffer.
            if ( 0 == index || MAX_BITWIDTH_LENGTH == index )
            {
                goto done;
            }

            subString[index] = '\0';

            // Now convert to an integer and we're done.
            bytes = (atoi( subString ) + 7) / 8;
            break;
        }

        ptr = strstr( startPointer, "_" );
    }

done:
    return bytes;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetDSPIndexFromPath
///
/// @brief  Gets the DSP index from the register path.
///
/// @param  path The path to get the DSP index from.
///
/// @retval -1      Path contained no DSP index value.
/// @retval other   The index of DSP extracted from register path.
///
////////////////////////////////////////////////////////////////////////////////
int GetDSPIndexFromPath( const char *path )
{
    const char *ptr = path;
    int dspIndex = -1;

    ptr = strstr( ptr, DSP_SUFFIX );
    while ( ptr )
    {
        // Start pointer should point on first character after DSP_SUFFIX
        const char *startPointer = ptr + strlen( DSP_SUFFIX );
        // DSP index should follow directory separator
        const char *endPointer = strstr( ptr, "/" );
        if ( endPointer && endPointer > startPointer )
        {
            char subString[MAX_DSP_INDEX_LENGTH];
            size_t index = 0;

            while ( index < MAX_DSP_INDEX_LENGTH &&
                    IsDigit( *startPointer ) &&
                    startPointer < endPointer
                  )
            {
                subString[index++] = *startPointer++;
            }

            // Ensure we haven't an empty buffer or maxed out the buffer.
            if ( 0 == index || MAX_DSP_INDEX_LENGTH == index )
            {
                goto done;
            }

            subString[index] = '\0';

            // Now convert to an integer and we're done.
            dspIndex = atoi( subString );
            break;
        }

        ptr = strstr( startPointer, DSP_SUFFIX );
    }

done:
    return dspIndex;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: FindInLayout
///
/// @brief  Performs a binary search for the register in the layout.
///
/// @param  pRegisterLayout The register layout to search.
/// @param  regAddr         The register address to search for.
///
/// @return The register line info, if found; otherwise, returns NULL.
///
////////////////////////////////////////////////////////////////////////////////
const RegisterLineInfo *FindInLayout( const RegisterLayout  *pRegisterLayout,
                                      unsigned int          regAddr
                                    )
{
    const RegisterLineInfo  *pLineInfo = NULL;
    size_t                  minIndex = 0;
    size_t                  maxIndex = pRegisterLayout->size() - 1;
    int                     layoutIndex = -1;

    if ( !pRegisterLayout || pRegisterLayout->empty() )
        goto done;

    // Make checking for register 0 super-quick, since we know we'll do that.
    if ( (*pRegisterLayout)[minIndex].registerAddress == regAddr )
    {
        layoutIndex = minIndex;
        goto done;
    }

    // If the register to check for is beyond the last register, we're done.
    if ( (*pRegisterLayout)[maxIndex].registerAddress < regAddr )
    {
        goto done;
    }

    // Perform an iterative binary search for the index.
    while ( maxIndex >= minIndex )
    {
        // Calculate the midpoint.
        size_t midIndex = minIndex + ( ( maxIndex - minIndex ) / 2 );

        //
        // Determine where the register would fall relative to our midpoint,
        // and adjust our search range appropriately.
        //
        if ( (*pRegisterLayout)[midIndex].registerAddress < regAddr )
        {
            // Register would come later in the layout.
            minIndex = midIndex + 1;
        }
        else if ( (*pRegisterLayout)[midIndex].registerAddress > regAddr )
        {
            // Register would come earlier in the layout.
            if ( midIndex == 0 )
                break;
            maxIndex = midIndex - 1;
        }
        else
        {
            // Register has been found.
            layoutIndex = midIndex;
            break;
        }
    }

done:
    // If we have an index, we were successful.
    if ( layoutIndex >= 0 )
        pLineInfo = &(*pRegisterLayout)[layoutIndex];

    return pLineInfo;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: FindInSections
///
/// @brief  Finds the register info in the sections.
///
/// @param  pRegisterSections   The register sections to search.
/// @param  regAddr             The register address to search for.
///
/// @return The register line info, if found; otherwise, returns NULL.
///
////////////////////////////////////////////////////////////////////////////////
const RegisterLineInfo *FindInSections( const RegisterSections    *pRegisterSections,
                                        unsigned int              regAddr
                                      )
{
    const RegisterLineInfo  *pLineInfo = NULL;

    if ( !pRegisterSections || pRegisterSections->empty() )
        goto done;

    for ( RegisterSections::const_iterator sectionIterator = pRegisterSections->begin();
          sectionIterator != pRegisterSections->end();
          ++sectionIterator
        )
    {
        pLineInfo = FindInLayout( &sectionIterator->layout, regAddr );
        if ( pLineInfo )
            goto done;
    }

done:
    return pLineInfo;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: FindSection
///
/// @brief  Finds the section containing the given register address.
///
/// @param  pRegisterSections   The register sections to search.
/// @param  regAddr             The register address to search for.
/// @param  pRegisterLineInfo   Optional; receives the RegisterLineInfo, if found.
///
/// @return The register section, if found; otherwise, returns NULL.
///
////////////////////////////////////////////////////////////////////////////////
const RegisterSection *FindSection( const RegisterSections    *pRegisterSections,
                                    unsigned int              regAddr,
                                    RegisterLineInfo          *pRegisterLineInfo
                                  )
{
    const RegisterSection   *pSection = NULL;
    const RegisterLineInfo  *pLineInfo = NULL;

    if ( !pRegisterSections || pRegisterSections->empty() )
    {
        goto done;
    }

    for ( RegisterSections::const_iterator sectionIterator = pRegisterSections->begin();
          sectionIterator != pRegisterSections->end();
          ++sectionIterator
        )
    {
        pLineInfo = FindInLayout( &sectionIterator->layout, regAddr );
        if ( pLineInfo )
        {
            pSection = &(*sectionIterator);
            goto done;
        }
    }

done:
    if ( pLineInfo && pRegisterLineInfo )
        *pRegisterLineInfo = *pLineInfo;

    return pSection;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetRegisterBytes
///
/// @brief  Gets the number of bytes for a given register.
///
/// @param  pRegisterSections   The register sections to find the register in.
/// @param  regAddr             The register address to get the bytes for.
///
/// @return The number of bytes for the register.
///
////////////////////////////////////////////////////////////////////////////////
unsigned int GetRegisterBytes( const RegisterSections   *pRegisterSections,
                               unsigned int             regAddr
                             )
{
    unsigned int bytes = 0;

    if ( !pRegisterSections || pRegisterSections->empty() )
        goto done;

    for ( RegisterSections::const_iterator sectionIterator = pRegisterSections->begin();
          sectionIterator != pRegisterSections->end();
          ++sectionIterator
        )
    {
        if ( sectionIterator->layout.empty() )
            continue;

        unsigned int minAddress = sectionIterator->layout[0].registerAddress;

        if ( regAddr >= minAddress )
            bytes = sectionIterator->bytesPerRegister;
        else
            goto done;
    }

done:
    return bytes;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetAddressIncrement
///
/// @brief  Gets the address increment for a given register's section.
///
/// @param  pRegisterSections   The register sections to find the register in.
/// @param  regAddr             The register address whose section to check.
///
/// @return The address increment for the register's section.
///
////////////////////////////////////////////////////////////////////////////////
unsigned int GetAddressIncrement( const RegisterSections    *pRegisterSections,
                                  unsigned int              regAddr
                                )
{
    unsigned int addressIncrement = 0;

    if ( !pRegisterSections || pRegisterSections->empty() )
        goto done;

    for ( RegisterSections::const_iterator sectionIterator = pRegisterSections->begin();
          sectionIterator != pRegisterSections->end();
          ++sectionIterator
        )
    {
        if ( sectionIterator->layout.empty() )
            continue;

        unsigned int minAddress = sectionIterator->layout[0].registerAddress;

        if ( regAddr >= minAddress )
            addressIncrement = sectionIterator->addressIncrement;
        else
            goto done;
    }

done:
    return addressIncrement;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetCodecPath
///
/// @brief  Gets the file path for a given register.
///
/// @param  pRegisterSections   The register sections to search.
/// @param  regAddr             The register address to get the path for.
///
/// @return The path for the file containing the register.
///
////////////////////////////////////////////////////////////////////////////////
const char *GetCodecPath( const RegisterSections   *pRegisterSections,
                          unsigned int             regAddr
                        )
{
    const char *path = NULL;

    if ( !pRegisterSections || pRegisterSections->empty() )
        goto done;

    for ( RegisterSections::const_iterator sectionIterator = pRegisterSections->begin();
          sectionIterator != pRegisterSections->end();
          ++sectionIterator
        )
    {
        if ( sectionIterator->layout.empty() )
            continue;

        unsigned int minAddress = sectionIterator->layout[0].registerAddress;

        if ( regAddr >= minAddress )
            path = sectionIterator->regPath;
        else
            goto done;
    }

done:
    return path;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GrowBuffer
///
/// @brief  Grows the given buffer, doubling it in size and retaining any content.
///
/// @param  ppBuffer     Pointer to the buffer to grow. Receives the new buffer.
/// @param  pBufferSize  Pointer to the buffer size. Receives the new size.
///
/// @retval WMT_SUCCESS             Succeeded.
/// @retval WMT_INVALID_PARAMETER   Buffer is null or of invalid size.
/// @retval WMT_RESOURCE_FAIL       Failed to grow buffer.
///
////////////////////////////////////////////////////////////////////////////////
RESULT GrowBuffer( char **ppBuffer, size_t *pBufferSize )
{
    RESULT  result = WMT_SUCCESS;
    char    *oldBuffer = NULL;
    char    *newBuffer = NULL;
    size_t  oldBufferSize = 0;
    size_t  newBufferSize = 0;

    //
    // If we don't have an old buffer, its size, or any actual content, there's
    // nothing we can do.
    //
    if ( !ppBuffer || !pBufferSize || !*ppBuffer || 0 == *pBufferSize )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    // Get the old buffer. We'll need it to copy its contents to the new one.
    oldBuffer = *ppBuffer;
    oldBufferSize = *pBufferSize;

    // Create the new buffer, doubling the old buffer's size.
    newBufferSize = oldBufferSize * 2;
    newBuffer = new char[newBufferSize];
    if ( !newBuffer )
    {
        result = WMT_RESOURCE_FAIL;
        goto done;
    }

    // Ensure the new buffer is null-terminated.
    memset( newBuffer, 0, newBufferSize );

    //
    // Successful - copy the contents of the old buffer into the new one and
    // set the new buffer to be returned.
    //
    memcpy( newBuffer, oldBuffer, oldBufferSize );
    *ppBuffer = newBuffer;
    *pBufferSize = newBufferSize;

    // Free the old buffer - it has been replaced.
    delete [] oldBuffer;

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: stristr
///
/// @brief  case-insensitive version of strstr
///
/// @param  haystack    String to search within
/// @param  needle      String to look for
///
/// @retval The location of first matching substring, or NULL
///
////////////////////////////////////////////////////////////////////////////////
char *stristr( char *haystack, const char *needle )
{
    char        *p1 = haystack ;
    const char  *p2 = needle ;
    char        *retval;

    if ( '\0' == *p2 )
        retval = haystack;
    else retval = NULL;

    while ( *p1 != '\0' && *p2 != '\0' )
    {
        if ( tolower( *p1 ) == tolower( *p2 ) )
        {
            //
            // A match.  If this is the first, then save where it started.
            //
            if ( !retval )
            {
                retval = p1;
            }

            p2++;
        }
        else
        {
            //
            // Not a match.  Start looking again.
            //
            p2 = needle;
            if ( tolower( *p1 ) == tolower( *p2 ) )
            {
                //
                // If this character matches, we're good to go again.
                //
                retval = p1;
                p2++ ;
            }
            else
            {
                //
                // We haven't found anything yet...
                //
                retval = NULL;
            }
        }

        p1++ ;
    }

    //
    // If we've run out of p1, but we've still got some p2 left, it's not a match.
    //
    if ( '\0' != *p2 )
        retval = NULL;

    return retval;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: stristr
///
/// @brief  case-insensitive version of strstr
///
/// This is a version for a const input.
///
/// @param  haystack    String to search within
/// @param  needle      String to look for
///
/// @retval The location of first matching substring, or NULL
///
////////////////////////////////////////////////////////////////////////////////
const char *stristr( const char *haystack, const char *needle )
{
    return stristr( (char *) haystack, needle );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ByteSwap32bit
///
/// @brief  Swaps the byte ordering for the given 32-bit value.
///
/// @param  val     The value to byte-swap.
///
/// @retval The byte-swapped value.
///
////////////////////////////////////////////////////////////////////////////////
unsigned int ByteSwap32bit( unsigned int val )
{
    val = ( ( val << 8 ) & 0xFF00FF00 ) | ( ( val >> 8 ) & 0xFF00FF );
    return ( val << 16 ) | ( ( val >> 16 ) & 0xFFFF );
}

//#pragma managed( pop )
///////////////////////////////// END OF FILE //////////////////////////////////
