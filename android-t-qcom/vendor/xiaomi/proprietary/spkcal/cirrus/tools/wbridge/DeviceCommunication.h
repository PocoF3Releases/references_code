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
/// @file   DeviceCommunication.h
/// @brief  Functionality for talking to the device.
///
/// @version \$Id: DeviceCommunication.h 19714 2017-10-31 08:45:08Z stankic $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
#ifndef __DEVICECOMMUNICATION_H__
#define __DEVICECOMMUNICATION_H__

//
// Include files
//
#include "LinuxDriverCommunication.h"

#ifndef _WIN32
#include <limits.h>
#include <sys/types.h>
#include <string>
#endif

//
// Definitions
//

////////////////////////////////////////////////////////////////////////////////
///
//  Class:  CDeviceCommunication
///
/// @brief  Functionality for talking to the device.
///
////////////////////////////////////////////////////////////////////////////////
class CDeviceCommunication : public CLinuxDriverCommunication
{
public:
    // Constructors and destructors
    CDeviceCommunication( int argc, char *argv[] );
    virtual ~CDeviceCommunication();

    RESULT GetRegisterMap( CRegValuePairs &regvals );

#ifndef _WIN32
    //
    // We need to pull the base block read/write functions from the base class
    // into scope or they won't be found by the compiler.
    //
    using CLinuxDriverCommunication::BlockRead;
    using CLinuxDriverCommunication::BlockWrite;
#endif

private:
    RESULT GetDebugFsMountPoint();
    RESULT MountDebugFs();
    void SearchForCodecs();

    // For initialisation with a range file.
    RESULT InitialiseLayoutWithRanges( RegisterSection *pSection );
    // For initialisation with a registers/codec_reg file only.
    RESULT InitialiseLayoutFromRegisters( RegisterSection *pSection );
    // For QDSP initialisation.
    size_t ReadQDSPAlgoCount();
    RESULT ReadQDSPAlgoInfo( size_t algoInfoSize, void *algoInfo );

    // Normal codec access functionality.
    RESULT ReadCodecRegister( unsigned int regAddr, unsigned int *pValue );
    RESULT BlockReadCodec( unsigned int  addr, unsigned int length, unsigned char *data );
    RESULT WriteCodecRegister( unsigned int regAddr, unsigned int value );
    RESULT BlockWriteCodec( unsigned int addr, unsigned int length, unsigned char *data );

    // QDSP access functionality.
    RESULT ReadFromQDSPAlgorithm( unsigned int algoID,
                                  unsigned int offset,
                                  unsigned int *pValue
                                );
    RESULT BlockReadFromQDSPAlgorithm( unsigned int algoID,
                                       unsigned int offset,
                                       unsigned int numRegsToRead,
                                       unsigned int *pValues
                                     );
    RESULT WriteToQDSPAlgorithm( unsigned int algoID,
                                 unsigned int offset,
                                 unsigned int value
                               );
    RESULT BlockWriteToQDSPAlgorithm( unsigned int algoID,
                                      unsigned int offset,
                                      unsigned int numRegsToWrite,
                                      unsigned int *pValues
                                    );

//
// Additions for Linux version.
//
#ifndef _WIN32
    RESULT OpenCodecFile( const char *path, int *pFile );
    RESULT ReadFromOffset( const char   *path,
                           size_t       offset,
                           size_t       byteCount,
                           char         *buffer,
                           size_t       bufferSize,
                           size_t       *pBytesParsed = NULL
                         );
    RESULT ReadFromOffset( int      codecFile,
                           size_t   offset,
                           size_t   byteCount,
                           char     *buffer,
                           size_t   bufferSize,
                           size_t   *pBytesParsed = NULL
                         );
    RESULT WriteToCodecFile( int            codecFile,
                             unsigned int   regAddr,
                             unsigned int   value
                           );

    // FTW support.
    static int ftw_callback( const char         *fpath,
                             const struct stat  *sb,
                             int                typeflag
                           );
#endif
};

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Inline functions
////////////////////////////////////////////////////////////////////////////////



#endif  // __DEVICECOMMUNICATION_H__
///////////////////////////////// END OF FILE //////////////////////////////////
