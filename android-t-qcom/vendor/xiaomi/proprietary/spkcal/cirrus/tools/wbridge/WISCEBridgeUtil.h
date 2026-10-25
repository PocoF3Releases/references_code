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
/// @file   WISCEBridgeUtil.h
/// @brief  Util definitions for WISCEBridge.
///
/// @version \$Id: WISCEBridgeUtil.h 19717 2017-10-31 11:21:28Z stankic $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
#ifndef __WISCEBRIDGEUTIL_H__
#define __WISCEBRIDGEUTIL_H__

// Comment out to disable printing of debug messages.
#ifndef DEBUG
#define DEBUG
#endif

//
// Include files
//
#include <stdlib.h>
#include "Result.h"
#include <vector>
#include <string>

#ifdef _WIN32
#include "Windows.h"
#else
#include <stdarg.h>
#endif

//
// Definitions
//

#define APPNAME                 "WISCEBridge"

// Maximum length of an OS info string.
#define MAX_OS_STRING           100

// Length of a string for containing a register address or its value.
#define REGVALUE_PAIR_LENGTH    9

#ifndef PATH_MAX
// Maximum length of a codec_reg path string.
#define PATH_MAX                512
#endif

//
// To allow compilation for Windows redefine several functions to the ISO
// standard equivalents.
//
#ifdef _WIN32
#define snprintf    _snprintf
#define strcasecmp  _stricmp
#define fileno      _fileno
#define read        _read
#endif

////////////////////////////////////////////////////////////////////////////////
///
//  Enum:  DeviceState
///
/// @brief  An indicator for whether communication with the device is ready.
///
////////////////////////////////////////////////////////////////////////////////
enum DeviceState
{
    NoDevice, DeviceReady
};

////////////////////////////////////////////////////////////////////////////////
///
//  Class:  CCodecPathPair
///
/// @brief  Matches a codec to the path to its codec_reg file.
///
////////////////////////////////////////////////////////////////////////////////
class CCodecPathPair
{
public:
    char    name[256];
    char    path[PATH_MAX];
    int     registerBytes;
    int     dspIndex;
};

////////////////////////////////////////////////////////////////////////////////
///
//  Class:  CRegValuePair
///
/// @brief  Contains a pair of strings representing a register's address and value.
///
////////////////////////////////////////////////////////////////////////////////
class CRegValuePair
{
public:
    char reg[REGVALUE_PAIR_LENGTH];
    char value[REGVALUE_PAIR_LENGTH];
};

// A collection of CRegValuePair objects.
typedef std::vector<CRegValuePair> CRegValuePairs;

//
// Definitions to use for register layout information.
//

/// The maximum register address.
#define MAX_REGISTER            0xFFFFFFFF

/// The initial capacity to use for a RegisterLayout.
#define INITIAL_CAPACITY        1024

/// Information about the line a register may be found at in a codec file.
struct RegisterLineInfo
{
    unsigned int registerAddress;   // The register address the info is for
    unsigned int offset;            // Where the register line starts in the file
    unsigned int lineLength;        // How big the register line is
    unsigned int lineNumber;        // The number of the line in the file
};

/// A vector for register line information in a codec file.
typedef std::vector<RegisterLineInfo> RegisterLayout;

// Collection of RegisterLayouts for a device with multiple registers files.
struct RegisterSection
{
    RegisterLayout  layout;
    char            regPath[PATH_MAX];
    int             bytesPerRegister;
    int             addressIncrement;
    unsigned int    algorithmID; // ID of the algorithm associated with this RegisterSection; if 0, no associated algorithm.
};
typedef std::vector<RegisterSection> RegisterSections;

//
// Sorts RegisterSection such that those starting at a lower address come first.
//
struct SectionSort
{
    bool operator() ( RegisterSection const &a, RegisterSection const &b ) const
    {
        return a.layout[0].registerAddress < b.layout[0].registerAddress;
    }
};

////////////////////////////////////////////////////////////////////////////////
///
//  struct:  CodecRegisters
///
/// @brief  A codec and it's cached register information.
///
////////////////////////////////////////////////////////////////////////////////
struct CodecRegisters
{
    char                name[256];
    RegisterSections    registerSections;
    bool                initialised;
};

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////

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
void OpenLogFile();

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
void CloseLogFile();

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
void Log( const char *format, ... );

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
void LogM( const char *message );

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
void InfoMessage( const char *format, ... );

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
void DebugMessage( const char *format, ... );

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
void ErrorMessage( const char *format, ... );

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
void PrintError( const char *message );

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
void SocketErrorMessage( const char *format, ... );

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
RESULT GetLogFolder( char *buffer, int buflen );

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
RESULT ParseWMCodec( const char *path, char *buffer, size_t bufferSize );

////////////////////////////////////////////////////////////////////////////////
///
//  Function: StoreCodec
///
/// @brief  Stores the codec in the given collection unless it is a duplicate or to be excluded.
///
/// @param  pCodecPaths      The collection of codec paths to be updated.
/// @param  codec            The name of the codec to add.
/// @param  path             The path of the codec to add.
/// @param  pExcludeDevices  The collection of codec names to exclude.
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
                 );

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
RESULT GetPathIdentifier( const char *path, char *buffer, size_t bufferLength );

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
                     );

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
/// @param  maxRegisterIndex    The maximum register address to use for the
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
                            unsigned int    maxRegisterIndex = MAX_REGISTER
                          );

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
                            unsigned int    maxRegisterIndex = MAX_REGISTER
                          );

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
                          );

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
int GetRegisterPathBytes( const char *path );

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
int GetDSPIndexFromPath( const char *path );

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
                                    );

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
const RegisterLineInfo *FindInSections( const RegisterSections  *pRegisterSections,
                                        unsigned int            regAddr
                                      );

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
                                  );

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
                             );

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
                                );

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
                        );

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
RESULT GrowBuffer( char **ppBuffer, size_t *pBufferSize );

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
char *stristr( char *haystack, const char *needle );
const char *stristr( const char *haystack, const char *needle );

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ByteSwap32bit
///
/// @brief  Performs a byte-swap for the given 32-bit value.
///
/// @param  val     The value to byte-swap.
///
/// @retval The byte-swapped value.
///
////////////////////////////////////////////////////////////////////////////////
unsigned int ByteSwap32bit( unsigned int val );

////////////////////////////////////////////////////////////////////////////////
// Inline functions
////////////////////////////////////////////////////////////////////////////////



#endif  // __WISCEBRIDGEUTIL_H__
///////////////////////////////// END OF FILE //////////////////////////////////