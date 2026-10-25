////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2016 Cirrus Logic International (UK) Ltd.  All rights reserved.
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
/// @file   LinuxDriverCommunication.h
/// @brief  Abstract class for communication with Cirrus Linux driver devices.
///
/// This covers shared functionality between the Linux/AndroidBridge and
/// WISCEBridge™ implementations which both ultimately communicate via Cirrus'
/// Linux drivers for Cirrus devices.
///
/// @version \$Id: LinuxDriverCommunication.h 18567 2016-08-08 14:32:32Z lbogdan $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
#ifndef __LINUXDRIVERCOMMUNICATION_H__
#define __LINUXDRIVERCOMMUNICATION_H__

//
// Include files
//
#include "BaseCommunications.h"
#include "QDSPInfo.h"

//
// Definitions
//
#ifdef _WIN32
//
// We have to build on Linux as well, so we can't use the secure versions
// of the CRTL functions.
//
#pragma warning( disable: 4996 )
#endif

////////////////////////////////////////////////////////////////////////////////
///
//  Class:  CLinuxDriverCommunication
///
/// @brief  Abstract class for communication with Cirrus Linux driver devices.
///
////////////////////////////////////////////////////////////////////////////////
class CLinuxDriverCommunication : public CBaseCommunications
{
public:
    RESULT EnumerateDevices( char *buffer, size_t bufferLength );
    RESULT SetDevice( const char *deviceName );

    const char *GetDeviceName() { return m_deviceName; }

    bool IsQDSP() { return IsQDSPDevice( GetDeviceName() ); }

    // Base access functions - call through as appropriate for normal codec or QDSP access.
    RESULT ReadRegister( unsigned int regAddr, unsigned int *pValue );
    virtual RESULT BlockRead( unsigned int  addr,
                              unsigned int  length,
                              unsigned char *data
                            );
    RESULT WriteRegister( unsigned int regAddr, unsigned int value );
    virtual RESULT BlockWrite( unsigned int     addr,
                               unsigned int     length,
                               unsigned char    *data
                             );

    //
    // We need to pull the base block read/write functions from the base class
    // into scope or they won't be found by the compiler.
    //
    using CBaseCommunications::BlockRead;
    using CBaseCommunications::BlockWrite;

protected:
    //
    // Generic initialisation.
    //
    void PreArgsInit();
    void HandleDefaultCodecArg( const char *arg );
    void HandleMaxRegAddressArgValue( const char *arg );
    void HandleDeviceInclude( const char *arg );
    void HandleDeviceExclude( const char *arg );

    void Initialise();

    //
    // Codec initialisation.
    //
    virtual RESULT PrepareComms() { return WMT_SUCCESS; }
    virtual RESULT GetDebugFsMountPoint() = 0;
    virtual RESULT MountDebugFs() = 0;
    virtual void SearchForCodecs() = 0;

    //
    // Register layout initialisation.
    //

    // For initialisation with a range file.
    virtual RESULT InitialiseLayoutWithRanges( RegisterSection *pSection ) = 0;
    RESULT GetRangeFilePath( RegisterSection *pSection,
                             char            *rangeFile,
                             size_t          bufferLength
                           );
    RESULT HandleRegisterLineForRanges( const char                *pLine,
                                        size_t                    lineLength,
                                        RegisterSection           *pSection,
                                        size_t                    *pLineLength,
                                        size_t                    *pBytesPerRegister,
                                        size_t                    *pAddressIncrement,
                                        std::vector<unsigned int> *pReadAddresses
                                      );
    RESULT HandleRangeFileLine( const char                *pLine,
                                RegisterSection           *pSection,
                                size_t                    *pTotalOffset,
                                size_t                    lineLength,
                                size_t                    *pLineNumber,
                                size_t                    *pAddressIncrement,
                                std::vector<unsigned int> *pReadAddresses
                              );
    void EnsureAddressIncrementSet( RegisterSection           *pSection,
                                    size_t                    addressIncrement,
                                    std::vector<unsigned int> *pReadAddresses
                                  );

    // For initialisation with a registers/codec_reg file only.
    virtual RESULT InitialiseLayoutFromRegisters( RegisterSection *pSection ) = 0;
    RESULT HandleRegisterFileLine( const char                *pLine,
                                   RegisterSection           *pSection,
                                   size_t                    *pTotalOffset,
                                   size_t                    lineLength,
                                   size_t                    *pLineNumber,
                                   size_t                    *pBytesPerRegister
                                 );

    // For QDSP initialisation, to be overridden.
    virtual size_t ReadQDSPAlgoCount() = 0;
    virtual RESULT ReadQDSPAlgoInfo( size_t algoCount, void *algoInfo ) = 0;

    //
    // Other shared functionality.
    //
    RESULT HandleGetRegisterMapLine( const char     *pLine,
                                     size_t         *pLineNumber,
                                     CRegValuePairs &regvals
                                   );

    // Normal codec access functionality, to be overridden.
    virtual RESULT ReadCodecRegister( unsigned int regAddr, unsigned int *pValue ) = 0;
    virtual RESULT BlockReadCodec( unsigned int  addr,
                                   unsigned int length,
                                   unsigned char *data
                                 ) = 0;
    virtual RESULT WriteCodecRegister( unsigned int regAddr, unsigned int value ) = 0;
    virtual RESULT BlockWriteCodec( unsigned int addr,
                                    unsigned int length,
                                    unsigned char *data
                                  ) = 0;

    // QDSP access functionality, to be overridden.
    virtual RESULT ReadFromQDSPAlgorithm( unsigned int algoID,
                                          unsigned int offset,
                                          unsigned int *pValue
                                        ) = 0;
    virtual RESULT BlockReadFromQDSPAlgorithm( unsigned int algoID,
                                               unsigned int offset,
                                               unsigned int numRegsToRead,
                                               unsigned int *pValues
                                             ) = 0;
    virtual RESULT WriteToQDSPAlgorithm( unsigned int algoID,
                                         unsigned int offset,
                                         unsigned int value
                                       ) = 0;
    virtual RESULT BlockWriteToQDSPAlgorithm( unsigned int algoID,
                                              unsigned int offset,
                                              unsigned int numRegsToWrite,
                                              unsigned int *pValues
                                            ) = 0;

    char                                m_debugfsMountPoint[PATH_MAX];  //< The path to where debugfs is mounted;
    static std::vector<CCodecPathPair>  m_codecPaths;                   //< The paths to the various codecs.
    static std::vector<std::string>     m_excludeDevices;               //< List of device names to exclude from storing.
    static std::vector<std::string>     m_includeDevices;               //< List of device names to include from storing (excludes others).
    DeviceState                         m_deviceState;                  //< Whether the selected codec is ready.
    RegisterSections                    *m_pRegisterSections;           //< Stores information on the different register sections available for the selected codec.
    bool                                m_isRegistersFile;              //< True if the selected codec is a registers file as opposed to a codec_reg file.
private:
    //
    // Generic initialisation.
    //
    RESULT FindCodecs();
    size_t QueryForCodec();
    RESULT FindCodecIndex( const char *codecName );

    //
    // Codec initialisation.
    //
    void InitialiseCodecPaths();

    //
    // Register layout initialisation.
    //
    void CreateRegisterCaches();
    CodecRegisters *GetCodecRegisters( const char *deviceName );

    void InitialiseRegisterLayouts();
    RESULT InitialiseRegisterLayout();

    // For QDSP initialisation.
    RESULT InitialiseQDSP( RegisterSections *pRegisterSections );
    RESULT InitialiseQDSPXMHeader( RegisterSections *pRegisterSections,
                                   RegisterSection  *pXMHeaderSection,
                                   int              algos
                                 );
    RESULT InitialiseQDSPAlgorithmSection( RegisterSections *pRegisterSections,
                                           RegisterSection  *pXMHeaderSection,
                                           unsigned int     algorithmID,
                                           unsigned int     xmOffset,
                                           unsigned int     xmCount
                                         );

    // For QDSP communication.
    RESULT ReadQDSPRegister( unsigned int regAddr, unsigned int *pValue );
    RESULT BlockReadQDSPInChunks( unsigned int  addr,
                                  unsigned int  length,
                                  unsigned char *data
                                );
    RESULT BlockReadQDSP( unsigned int regAddr,
                          unsigned int numRegsToRead,
                          unsigned int *pValues
                        );
    RESULT WriteQDSPRegister( unsigned int regAddr, unsigned int value );
    RESULT BlockWriteQDSPInChunks( unsigned int  addr,
                                   unsigned int  length,
                                   unsigned char *data
                                 );
    RESULT BlockWriteQDSP( unsigned int regAddr,
                           unsigned int numRegsToWrite,
                           unsigned int *pValues
                         );

    char                        m_defaultCodec[PATH_MAX];   //< The default codec to use.
    std::vector<CodecRegisters> m_codecRegisters;           //< The cached register information for the various codecs.
    unsigned int                m_maxRegisterIndex;         //< The maximum register address we care about.
    unsigned int                m_maxLineNumber;            //< The maximum line number in the codec file we care about.
    const char                  *m_deviceName;              //< The name of the codec currently in use.
    std::vector<unsigned int>   m_QDSPXMHeader;             //< The XM Header for the QDSP device, if there is one.
};

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Inline functions
////////////////////////////////////////////////////////////////////////////////



#endif  // __LINUXDRIVERCOMMUNICATION_H__
///////////////////////////////// END OF FILE //////////////////////////////////
