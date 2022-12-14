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
/// @file   LinuxDriverCommunication.cpp
/// @brief  Abstract class for communication with Cirrus Linux driver devices.
///
/// This covers shared functionality between the Linux/AndroidBridge and
/// WISCEBridge™ implementations which both ultimately communicate via Cirrus'
/// Linux drivers for Cirrus devices.
///
/// @version \$Id: LinuxDriverCommunication.cpp 18652 2016-08-30 14:46:26Z lbogdan $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////

//
// Include files
//
#include "LinuxDriverCommunication.h"
#include <cstring>
#include <cstdio>
#include <algorithm>

#ifdef _WIN32
//
// We have to build on Linux as well, so we can't use the secure versions
// of the CRTL functions.
//
#pragma warning( disable: 4996 )
#else
#include <ctype.h>
#include <sys/types.h>
#endif

//
// Global definitions
//

//
// Class data
//
std::vector<CCodecPathPair>  CLinuxDriverCommunication::m_codecPaths;
std::vector<std::string>     CLinuxDriverCommunication::m_excludeDevices;
std::vector<std::string>     CLinuxDriverCommunication::m_includeDevices;

//
// Function prototypes
//

////////////////////////////////////////////////////////////////////////////////
///
//  Class:  CLinuxDriverCommunication
///
/// @brief  Abstract class for communication with Cirrus Linux driver devices.
///
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
///
//  Function: PreArgsInit
///
/// @brief  Default initialisation steps for before any given args are handled.
///
/// (no parameters)
///
/// @return void
///
////////////////////////////////////////////////////////////////////////////////
void CLinuxDriverCommunication::PreArgsInit()
{
    m_maxRegisterIndex = MAX_REGISTER;
    m_maxLineNumber = MAX_REGISTER;

    m_isRegistersFile = false;

    m_deviceName = NULL;

    m_excludeDevices.clear();
    m_includeDevices.clear();
    
    memset( m_defaultCodec, 0, PATH_MAX );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleDefaultCodecArg
///
/// @brief  Handles the <deviceName> part of the --default <deviceName> command line argument.
///
/// @note  Legacy: handles the WM* command line argument, where WM* is the <deviceName>.
///
/// @param  arg     The <deviceName> arg to handle.
///
/// @return void
///
////////////////////////////////////////////////////////////////////////////////
void CLinuxDriverCommunication::HandleDefaultCodecArg( const char *arg )
{
    snprintf( m_defaultCodec, PATH_MAX, "%s", arg );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleMaxRegAddressArgValue
///
/// @brief  Handles the <addr> part of the --max <addr> command line argument.
///
/// @param  arg     The <addr> arg to handle.
///
/// @return void
///
////////////////////////////////////////////////////////////////////////////////
void CLinuxDriverCommunication::HandleMaxRegAddressArgValue( const char *arg )
{
    m_maxRegisterIndex = strtoul( arg, NULL, 16 );
    InfoMessage( "Max register address set to 0x%X.\n", m_maxRegisterIndex );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleDeviceInclude
///
/// @brief  Handles the <device> part of the -d <device> command line argument.
///
/// @param  arg     The <device> arg to handle.
///
/// @return void
///
////////////////////////////////////////////////////////////////////////////////
void CLinuxDriverCommunication::HandleDeviceInclude( const char *arg )
{
    m_includeDevices.push_back( std::string( arg ) );
    InfoMessage( "Adding \"%s\" to the included list of codecs.\n", arg );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleDeviceExclude
///
/// @brief  Handles the <device> part of the -x <device> command line argument.
///
/// @param  arg     The <device> arg to handle.
///
/// @return void
///
////////////////////////////////////////////////////////////////////////////////
void CLinuxDriverCommunication::HandleDeviceExclude( const char *arg )
{
    m_excludeDevices.push_back( std::string( arg ) );
    InfoMessage( "Adding \"%s\" to the excluded list of codecs.\n", arg );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: Initialise
///
/// @brief  Default initialisation steps.
///
/// @note This base initialisation should always be called once any arg handling
///       is complete.
///
/// (no parameters)
///
/// @return void
///
////////////////////////////////////////////////////////////////////////////////
void CLinuxDriverCommunication::Initialise()
{
    // Find any codecs.
    m_deviceState = NoDevice;
    FindCodecs();
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: FindCodecs
///
/// @brief  Finds the paths of all Cirrus codec files for the Linux driver.
///
/// (no parameters)
///
/// @retval WMT_SUCCESS     Device found.
/// @retval WMT_NO_DEVICE   No devices found.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::FindCodecs()
{
    RESULT result = WMT_NO_DEVICE;

    // Locate and initialise all codec files.
    InitialiseCodecPaths();

    // Bail out if we did not find any codecs.
    if ( 0 == m_codecPaths.size() )
    {
        ErrorMessage( "Could not find any Cirrus codecs!\n" );
        goto done;
    }

    // If we have a default codec try and use that.
    if ( strlen( m_defaultCodec ) > 0 )
    {
        result = SetDevice( m_defaultCodec );
    }

    //
    // If we haven't set a device yet (haven't filled in m_deviceName) ask
    // the user what device to use (unless we have only one).
    //
    if ( !m_deviceName )
    {
        size_t deviceIndex = 0;

        //
        // If we have found more than one codec ask the user which one they want
        // to use.
        //
        if ( m_codecRegisters.size() > 1 )
            deviceIndex = QueryForCodec();

        if ( (size_t) WMT_NO_DEVICE == deviceIndex )
            goto done;

        // Select the codec
        result = SetDevice( m_codecRegisters[deviceIndex].name );
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: InitialiseCodecPaths
///
/// @brief  Locates all codec files and fills m_codecPaths with them.
///
/// (no parameters)
///
/// @return void
///
////////////////////////////////////////////////////////////////////////////////
void CLinuxDriverCommunication::InitialiseCodecPaths()
{
    RESULT localResult = WMT_SUCCESS;

    // Clear any existing codecs.
    m_codecPaths.clear();
    m_codecRegisters.clear();

    // Do any comms preparation required.
    localResult = PrepareComms();
    if ( WMT_SUCCESS != localResult )
        goto done;

    // Check that the debugfs filesystem is mounted.
    if ( WMT_SUCCESS != GetDebugFsMountPoint() )
        if ( WMT_SUCCESS != MountDebugFs() )
            goto done;

    // Now see if we can find any codecs.
    InfoMessage( "Searching for codecs...\n" );
    SearchForCodecs();
    InfoMessage( "...done.\n" );

done:
    if ( !m_codecPaths.size() )
    {
        ErrorMessage( "Unable to locate device register files.\n" );
    }
    else
    {
        CreateRegisterCaches();

        //
        // Initialise the different codecs. We do this now so a detect command,
        // or a switch to any of the available devices, will not cause a
        // slow-down for the user once WISCEBridge™ has started waiting for a
        // connection.
        // If there's more than one codec to choose from, reset m_deviceName
        // afterwards - we still want to either use the default codec given as
        // a command arg, or otherwise query the user for which device they
        // want to use.
        //
        InitialiseRegisterLayouts();
        if ( m_codecRegisters.size() > 1 )
            m_deviceName = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: CreateRegisterCaches
///
/// @brief  Creates the empty register layout caches for the devices.
///
/// (no parameters)
///
/// @return void.
///
////////////////////////////////////////////////////////////////////////////////
void CLinuxDriverCommunication::CreateRegisterCaches()
{
    for ( std::vector<CCodecPathPair>::iterator codecIterator = m_codecPaths.begin();
          codecIterator != m_codecPaths.end();
          ++codecIterator
        )
    {
        CodecRegisters *pExistingRegisters = GetCodecRegisters( codecIterator->name );

        RegisterSection section;
        strncpy( section.regPath, codecIterator->path, sizeof( section.regPath ) );
        section.regPath[sizeof( section.regPath ) - 1] = '\0';

        if ( pExistingRegisters )
        {
            pExistingRegisters->registerSections.push_back( section );
        }
        else
        {
            CodecRegisters registers;

            strncpy( registers.name, codecIterator->name, sizeof( registers.name ) );
            registers.name[sizeof( registers.name ) - 1] = '\0';
            registers.initialised = false;

            registers.registerSections.push_back( section );

            m_codecRegisters.push_back( registers );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetCodecRegisters
///
/// @brief  Gets the cached register layout for the codec.
///
/// @param  deviceName The codec to get the cached registers for.
///
/// @return The cached register layout, or a null pointer if none found.
///
////////////////////////////////////////////////////////////////////////////////
CodecRegisters *CLinuxDriverCommunication::GetCodecRegisters( const char *deviceName )
{
    CodecRegisters *pRegisters = NULL;

    for ( size_t i = 0; i < m_codecRegisters.size(); i++ )
    {
        if ( 0 == strcasecmp( deviceName, m_codecRegisters[i].name ) )
        {
            pRegisters = &m_codecRegisters[i];
            break;
        }
    }

    return pRegisters;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: InitialiseRegisterLayouts
///
/// @brief  Initialises the register layout information for the devices.
///
/// (no parameters)
///
/// @return void.
///
////////////////////////////////////////////////////////////////////////////////
void CLinuxDriverCommunication::InitialiseRegisterLayouts()
{
    for ( std::vector<CCodecPathPair>::iterator codecIterator = m_codecPaths.begin();
          codecIterator != m_codecPaths.end();
          ++codecIterator
        )
    {
        // Setting the device will take care of initialising its layout.
        SetDevice( codecIterator->name );
    }
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: InitialiseRegisterLayout
///
/// @brief  Initialises the register layout information for the selected device.
///
/// (no parameters)
///
/// @retval WMT_SUCCESS     Register layout initialised successfully.
/// @retval WMT_NO_DEVICE   Register layout initialisation failed - no device.
/// @retval WMT_READ_FAILED Register layout initialisation failed - no device data.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::InitialiseRegisterLayout()
{
    RESULT          result = WMT_NO_DEVICE;
    CodecRegisters  *pRegisters = NULL;

    // We must have a device to have a codec file.
    if ( NoDevice == m_deviceState )
    {
        goto done;
    }

    // Reset our layout map.
    m_pRegisterSections = NULL;

    //
    // Get the registers for this codec, these should never return NULL as
    // they're created alongside the codecs.
    //
    pRegisters = GetCodecRegisters( m_deviceName );
    if ( !pRegisters )
        goto done;
    m_pRegisterSections = &( pRegisters->registerSections );
    if ( !m_pRegisterSections )
        goto done;

    // If they're already initialised we've nothing to do.
    if ( pRegisters->initialised )
    {
        result = WMT_SUCCESS;
        goto done;
    }

    InfoMessage( "Initialising register layout...\n" );

    //
    // If this is the QDSP device, we actually want to do QDSP initialisation.
    //
    if ( IsQDSP() )
    {
        //
        // Call QDSP initialisation and goto done regardless of result- we
        // don't want any other kind of initialisation attempted for QDSP.
        //
        result = InitialiseQDSP( m_pRegisterSections );

        // If successful, ensure any QDSP sections are in order.
        if ( WMT_SUCCESS == result )
        {
            // Sort our sections so they are in order.
            std::sort( m_pRegisterSections->begin(), m_pRegisterSections->end(), SectionSort() );
        }

        goto done;
    }

    for ( RegisterSections::iterator sectionIterator = m_pRegisterSections->begin();
          sectionIterator != m_pRegisterSections->end();
          ++sectionIterator
        )
    {
        RegisterSection *pSection = &( *sectionIterator );

        //
        // Try to initialise with the register ranges file.
        // If successful, we're done. Otherwise, we need to continue and fill in
        // the layout map by reading the whole codec file.
        //
        result = InitialiseLayoutWithRanges( pSection );
        if ( WMT_SUCCESS == result )
            continue;

        //
        // No range file present - address increment must default to 1, as we
        // have no way of determining the true increment without the range file.
        //
        pSection->addressIncrement = 1;
        Log( "Unable to detect address increment without using range file: defaulting to address increment of %d.\n",
             pSection->addressIncrement
           );

        //
        // If we get here, we must try to initialise the section using only the
        // registers/codec_reg file; this will typically only occur for our
        // older devices, for which there is no range file available.
        //
        result = InitialiseLayoutFromRegisters( pSection );
        if ( WMT_SUCCESS != result )
            goto done;
    }

    // Sort our sections so they are in order.
    std::sort( m_pRegisterSections->begin(), m_pRegisterSections->end(), SectionSort() );

    // We were successful.
    result = WMT_SUCCESS;

done:
    // If we've done our initialisation print out what we found.
    if ( pRegisters && !pRegisters->initialised )
    {
        pRegisters->initialised = true;
        for ( RegisterSections::iterator sectionIterator = m_pRegisterSections->begin();
              sectionIterator != m_pRegisterSections->end();
              ++sectionIterator
            )
        {
            if ( !sectionIterator->layout.empty() )
            {
                DebugMessage( "Found %d registers (last register: R%Xh at offset %d).\n",
                              sectionIterator->layout.size(),
                              sectionIterator->layout.rbegin()->registerAddress,
                              sectionIterator->layout.rbegin()->offset
                            );
            }
            else
            {
                DebugMessage( "No registers found.\n" );
            }
        }

        InfoMessage( "...done.\n" );
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: InitialiseQDSP
///
/// @brief  Handles initialisation of the QDSP driver.
///
/// @param  pRegisterSections       The register sections to initialise.
///
/// @retval WMT_SUCCESS             QDSP initialised successfully.
/// @retval WMT_NO_DEVICE           No device present.
/// @retval WMT_INVALID_PARAMETER   The given sections are not for the QDSP driver.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::InitialiseQDSP( RegisterSections *pRegisterSections )
{
    RESULT          result = WMT_SUCCESS;
    RegisterSection *pSection = NULL;

    // We must have a device to have a codec file.
    if ( NoDevice == m_deviceState )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }
    // This must be initialisation for our QDSP driver.
    if ( !IsQDSP() || !pRegisterSections )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }
    // And we should only have one section just now, for the QDSP_DRIVER_FILE.
    pSection = &( *pRegisterSections->begin() );
    if ( 1 != pRegisterSections->size() || 0 != strcasecmp( pSection->regPath, QDSP_DRIVER_FILE ) )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    Log( "Doing QDSP initialisation for \"%s\"\n", QDSP_DRIVER_FILE );

    //
    // Set up the XM Header info so we know what algorithms are there.
    // This will set up the register section for the spoofed XM header
    // as well as the register sections for any algorithms.
    //
    result = InitialiseQDSPXMHeader( pRegisterSections, pSection, AUTO_DETECT );
    if ( WMT_SUCCESS != result )
        goto done;

done:
    if ( WMT_SUCCESS != result )
        Log( "Failed to initialise QDSP.\n" );

    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: InitialiseQDSPXMHeader
///
/// @brief  Handles initialisation of the XM header for the QDSP driver.
///
/// @param  pRegisterSections   The register sections for the device.
/// @param  pXMHeaderSection    The register section for the XM Header.
/// @param  algos               The types of algorithms to spoof.
///
/// @retval WMT_SUCCESS             XM header initialised successfully.
/// @retval WMT_NO_DEVICE           No device present.
/// @retval WMT_INVALID_PARAMETER   The given sections are not for the QDSP driver.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::InitialiseQDSPXMHeader( RegisterSections  *pRegisterSections,
                                                          RegisterSection   *pXMHeaderSection,
                                                          int               algos
                                                        )
{
    RESULT                      result = WMT_SUCCESS;
    char                        tmpbuf[PATH_MAX];
    size_t                      totalOffset = 0;
    size_t                      lineNumber = 0;
    size_t                      lineLength = QDSP_DATA_SIZE_BITS;
    size_t                      addressIncrement = 1;
    std::vector<unsigned int>   readAddresses;
    size_t                      numAlgos = 0;
    size_t                      algoIDIndex = DSP_FIRST_ALGO_ID_IDX;
    size_t                      algoXMOffsetIndex = DSP_FIRST_ALGO_XM_BASE_IDX;
    std::vector<unsigned int>   xmCounts;

    // We must have a device to have a codec file.
    if ( NoDevice == m_deviceState )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }
    // This must be QDSP and params should be provided.
    if ( !IsQDSP() || !pXMHeaderSection )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    Log( "Initialising QDSP XM header...\n" );

    //
    // Set up the register section for our spoofed XM header registers.
    //
    pXMHeaderSection->algorithmID = 0;
    pXMHeaderSection->bytesPerRegister = ( QDSP_DATA_SIZE_BITS + 7 ) / 8;
    pXMHeaderSection->addressIncrement = addressIncrement;

    //
    // Set up our spoofed XM header registers.
    //

    // Set up our static header parts...
    m_QDSPXMHeader.push_back( 0x00020000 ); // DATA_HEADER_COREID
    m_QDSPXMHeader.push_back( 0x00000501 ); // DATA_HEADER_COREREVISION
    m_QDSPXMHeader.push_back( 0x00A40066 ); // DATA_HEADER_FIRMWAREID
    m_QDSPXMHeader.push_back( 0x00000001 ); // DATA_HEADER_FIRMWAREREVISION
    m_QDSPXMHeader.push_back( XM_FIRMWARE_OFFSET ); // DATA_HEADER_FIRMWAREZMBASE
    m_QDSPXMHeader.push_back( XM_FIRMWARE_OFFSET ); // DATA_HEADER_FIRMWAREXMBASE
    m_QDSPXMHeader.push_back( XM_FIRMWARE_OFFSET ); // DATA_HEADER_FIRMWAREYMBASE

    // Then set up the algorithms as appropriate.
    if ( AUTO_DETECT == algos )
    {
        //
        // Auto-detection of algorithms from the driver.
        //
        InfoMessage( "Auto-detecting algorithms from the driver...\n" );

        // The count of algorithms.
        unsigned int algoCount = ReadQDSPAlgoCount();
        m_QDSPXMHeader.push_back( algoCount ); // DATA_HEADER_FIRMWAREALGORITHMCOUNT

        // The infos for the algorithms.
        int             messageSize = sizeof( QDSPAlgoInfo ) * algoCount;
        QDSPAlgoInfo    *pAlgoInfo = NULL;
        pAlgoInfo = (QDSPAlgoInfo *) malloc( messageSize );
        if ( NULL == pAlgoInfo )
        {
            DebugMessage( "InitialiseQDSPXMHeader: Failed to allocate memory.\n" );
            result = WMT_RESOURCE_FAIL;
            goto cleanup;
        }

        if ( WMT_SUCCESS == ReadQDSPAlgoInfo( messageSize, pAlgoInfo ) )
        {
            int offset = XM_INITIAL_ALGO_OFFSET;
            for ( unsigned int algo = 0; algo < algoCount; algo++ )
            {
                QDSPAlgoInfo algoInfo = pAlgoInfo[algo];
                m_QDSPXMHeader.push_back( algoInfo.ID );
                m_QDSPXMHeader.push_back( algoInfo.revision );

                //
                // QDSP has no ZM or YM memory so just return the same for all
                //
                m_QDSPXMHeader.push_back( offset ); // ALGORITHM ZMBASE
                m_QDSPXMHeader.push_back( offset ); // ALGORITHM XMBASE
                m_QDSPXMHeader.push_back( offset ); // ALGORITHM YMBASE

                // Save the count for later initialisation of the algorithm RegisterSection for the detected algorithm.
                DebugMessage( "InitialiseQDSPXMHeader: Detected algorithm: ID: 0x%X Revision: 0x%X Register Count: %u (Offset: 0x%X) \n",
                              algoInfo.ID,
                              algoInfo.revision,
                              algoInfo.registerCount,
                              offset
                            );
                xmCounts.push_back( algoInfo.registerCount );

                //
                // Make sure we round up to the nearest 0x100 so there is a bit of breathing room.
                //
                offset += ( ( algoInfo.registerCount / 0x100 ) + 1 ) * 0x100;
            }
        }
cleanup:
        if ( NULL != pAlgoInfo )
            free( pAlgoInfo );
    }
    else
    {
        //
        // One of our hard-coded lists of available algorithms:
        //
        if ( COMP == algos )
        {
            //
            // The single COMP algorithm firmware
            //
            InfoMessage( "Using COMP algorithm...\n" );
            m_QDSPXMHeader.push_back( 0x00000001 ); // DATA_HEADER_FIRMWAREALGORITHMCOUNT

            m_QDSPXMHeader.push_back( 0x00002009 ); // ALGORITHM_HEADER1_ID
            m_QDSPXMHeader.push_back( 0x00010100 ); // ALGORITHM_HEADER1_REVISION
            m_QDSPXMHeader.push_back( 0x00000200 ); // ALGORITHM_HEADER1_ZMBASE
            m_QDSPXMHeader.push_back( 0x00000200 ); // ALGORITHM_HEADER1_XMBASE
            m_QDSPXMHeader.push_back( 0x00000200 ); // ALGORITHM_HEADER1_YMBASE
        }
        else if ( QDSP_SPEAKER_PROTECTION == algos )
        {
            //
            // The 4 algorithm speaker protection firmware
            //
            InfoMessage( "Using QDSP_SPEAKER_PROTECTION algorithms...\n" );
            m_QDSPXMHeader.push_back( 0x00000004 ); // DATA_HEADER_FIRMWAREALGORITHMCOUNT

            m_QDSPXMHeader.push_back( 0x00002010 ); // ALGORITHM_HEADER1_ID
            m_QDSPXMHeader.push_back( 0x00410000 ); // ALGORITHM_HEADER1_REVISION
            m_QDSPXMHeader.push_back( 0x00000200 ); // ALGORITHM_HEADER1_ZMBASE
            m_QDSPXMHeader.push_back( 0x00000200 ); // ALGORITHM_HEADER1_XMBASE
            m_QDSPXMHeader.push_back( 0x00000200 ); // ALGORITHM_HEADER1_YMBASE

            m_QDSPXMHeader.push_back( 0x00002011 ); // ALGORITHM_HEADER2_ID
            m_QDSPXMHeader.push_back( 0x00420100 ); // ALGORITHM_HEADER2_REVISION
            m_QDSPXMHeader.push_back( 0x00000500 ); // ALGORITHM_HEADER2_ZMBASE
            m_QDSPXMHeader.push_back( 0x00000500 ); // ALGORITHM_HEADER2_XMBASE
            m_QDSPXMHeader.push_back( 0x00000500 ); // ALGORITHM_HEADER2_YMBASE

            m_QDSPXMHeader.push_back( 0x00002012 ); // ALGORITHM_HEADER3_ID
            m_QDSPXMHeader.push_back( 0x00410202 ); // ALGORITHM_HEADER3_REVISION
            m_QDSPXMHeader.push_back( 0x00000800 ); // ALGORITHM_HEADER3_ZMBASE
            m_QDSPXMHeader.push_back( 0x00000800 ); // ALGORITHM_HEADER3_XMBASE
            m_QDSPXMHeader.push_back( 0x00000800 ); // ALGORITHM_HEADER3_YMBASE

            m_QDSPXMHeader.push_back( 0x00002013 ); // ALGORITHM_HEADER3_ID
            m_QDSPXMHeader.push_back( 0x00000002 ); // ALGORITHM_HEADER3_REVISION
            m_QDSPXMHeader.push_back( 0x00000F00 ); // ALGORITHM_HEADER3_ZMBASE
            m_QDSPXMHeader.push_back( 0x00000F00 ); // ALGORITHM_HEADER3_XMBASE
            m_QDSPXMHeader.push_back( 0x00000F00 ); // ALGORITHM_HEADER3_YMBASE
        }
        else
        {
            // Unknown
            InfoMessage( "Using no algorithms...\n" );
            m_QDSPXMHeader.push_back( 0x00000000 ); // DATA_HEADER_FIRMWAREALGORITHMCOUNT
        }
    }

    m_QDSPXMHeader.push_back( DSP_ALGORITHM_LIST_TERMINATOR );

    //
    // And set up our spoofed registers for the XM header, mocking
    // a line from a range file, which should be in the format:
    // <startAddr>-<endAddr>.
    //
    snprintf( tmpbuf,
              sizeof( tmpbuf ),
              "%x-%x",
              XM_START_REGISTER,
              ( XM_START_REGISTER + (unsigned int) m_QDSPXMHeader.size() )
            );

    Log( "Spoofing DSP data header registers for QDSP, range: \"%s\"\n", tmpbuf );
    result = HandleRangeFileLine( tmpbuf,
                                  pXMHeaderSection,
                                  &totalOffset,
                                  lineLength,
                                  &lineNumber,
                                  &addressIncrement,
                                  &readAddresses
                                );

    //
    // Now set up the algorithm RegisterSections for any algorithms.
    // Note: we must do this after the XM header section set-up is completed,
    // or we seem to get a segfault.
    //
    numAlgos = m_QDSPXMHeader[DSP_HEADER_ALGORITHM_COUNT_IDX];
    DebugMessage( "Initialising QDSP algorithm sections for %u algorithms.\n", numAlgos );
    for ( size_t i = 0; i < numAlgos; i++ )
    {
        unsigned int algoID = m_QDSPXMHeader[algoIDIndex];
        unsigned int xmOffset = m_QDSPXMHeader[algoXMOffsetIndex];
        unsigned int xmCount = 0;

        if ( i < xmCounts.size() )
        {
            // Auto-detected algorithms - we have explicit counts.
            xmCount = xmCounts[i];
        }
        else if ( i + 1 < numAlgos )
        {
            xmCount = m_QDSPXMHeader[algoXMOffsetIndex + DSP_ALGO_HEADER_NUM_ELEMENTS] - m_QDSPXMHeader[algoXMOffsetIndex];
        }
        else
        {
            xmCount = XM_LAST_REGISTER - ( XM_START_REGISTER + xmOffset );
        }

        result = InitialiseQDSPAlgorithmSection( pRegisterSections,
                                                 pXMHeaderSection,
                                                 algoID,
                                                 xmOffset,
                                                 xmCount
                                               );
        if ( WMT_SUCCESS != result )
            goto done;

        // Increment our indexes so they point to the next algorithm's info.
        algoIDIndex += DSP_ALGO_HEADER_NUM_ELEMENTS;
        algoXMOffsetIndex += DSP_ALGO_HEADER_NUM_ELEMENTS;
    }

    Log( "...done.\n" );

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: InitialiseQDSPAlgorithmSection
///
/// @brief  Handles initialisation of an algorithm section for the QDSP driver.
///
/// @param  pRegisterSections   The register sections for the device.
/// @param  pXMHeaderSection    The register section for the XM Header.
/// @param  algorithmID         The ID of the algorithm this section is for.
/// @param  xmOffset            The algorithm's base offset in XM memory.
/// @param  xmCount             The count of registers in XM for the algorithm.
///
/// @retval WMT_SUCCESS             Algorithm section initialised successfully.
/// @retval WMT_NO_DEVICE           No device present.
/// @retval WMT_INVALID_PARAMETER   The given sections are not for the QDSP driver.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::InitialiseQDSPAlgorithmSection( RegisterSections   *pRegisterSections,
                                                                  RegisterSection    *pXMHeaderSection,
                                                                  unsigned int       algorithmID,
                                                                  unsigned int       xmOffset,
                                                                  unsigned int       xmCount
                                                                )
{
    RESULT                      result = WMT_SUCCESS;
    RegisterSection             algorithmSection;
    unsigned int                xmStartAddress = XM_START_REGISTER + xmOffset;
    unsigned int                xmEndAddress = xmStartAddress + xmCount - 1;
    char                        tmpbuf[PATH_MAX];
    size_t                      totalOffset = 0;
    size_t                      lineNumber = 0;
    size_t                      lineLength = QDSP_DATA_SIZE_BITS;
    size_t                      addressIncrement = 1;
    std::vector<unsigned int>   readAddresses;

    // We must have a device to have a codec file.
    if ( NoDevice == m_deviceState )
    {
        result = WMT_NO_DEVICE;
        goto done;
    }
    // This must be QDSP and params should be provided/valid.
    if ( !IsQDSP() || !pRegisterSections || !pXMHeaderSection ||
         0 == algorithmID || m_QDSPXMHeader.size() > xmOffset || 0 == xmCount
       )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    Log( "Initialising QDSP register section for algorithm 0x%X...", algorithmID );

    //
    // Set up the register section for our spoofed algorithm registers.
    // Basic information will be the same as that of the XM Header section;
    // only the registers and algorithm ID differ.
    //
    algorithmSection.algorithmID = algorithmID;
    strncpy( algorithmSection.regPath, pXMHeaderSection->regPath, sizeof( algorithmSection.regPath ) );
    algorithmSection.regPath[sizeof( algorithmSection.regPath ) - 1] = '\0';
    algorithmSection.bytesPerRegister = pXMHeaderSection->bytesPerRegister;
    algorithmSection.addressIncrement = pXMHeaderSection->addressIncrement;

    //
    // And set up our spoofed registers for the algorithm, mocking
    // a line from a range file, which should be in the format:
    // <startAddr>-<endAddr>.
    //
    snprintf( tmpbuf, sizeof( tmpbuf ), "%x-%x", xmStartAddress, xmEndAddress );

    Log( "Spoofing algorithm registers for QDSP algorithm 0x%X, range: \"%s\"\n", algorithmID, tmpbuf );
    result = HandleRangeFileLine( tmpbuf,
                                  &algorithmSection,
                                  &totalOffset,
                                  lineLength,
                                  &lineNumber,
                                  &addressIncrement,
                                  &readAddresses
                                );

    // Add the section to the collection of sections.
    pRegisterSections->push_back( algorithmSection );

    Log( "...done.\n" );

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: GetRangeFilePath
///
/// @brief  Gets the range file path for the given register section.
///
/// @note   This only retrieves what the file path for the range file would
///         be. It does NOT check if the range file is actually present.
///
/// @param  pSection        The section to get the range file path for.
/// @param  rangeFile       Buffer to receive the range file path.
/// @param  bufferLength    Length of the rangeFile buffer.
///
/// @retval WMT_SUCCESS     Found path for range file.
/// @retval WMT_NO_DATA     No range file available to use.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::GetRangeFilePath( RegisterSection *pSection,
                                                    char            *rangeFile,
                                                    size_t          bufferLength
                                                  )
{
    RESULT  result = WMT_SUCCESS;
    char    *p;

    //
    // If we are using a registers codec, there is potentially a range file - it
    // will be under the same directory if one exists. Check if we have a
    // 'registers' codec file, and if so, get the range file path by replacing
    // the 'registers' file name with 'range' in the codec file path.
    //

    // Get a local copy of the path.
    strncpy( rangeFile, pSection->regPath, bufferLength );
    rangeFile[bufferLength - 1] = '\0';

    //
    // Check the codec file name - we need a 'registers' codec file for there to
    // possibly be a range file.
    //
    p = strrchr( rangeFile, '/' );
    if ( !p )
    {
        result = WMT_NO_DATA;
        goto done;
    }
    p++;
    if ( !*p )
    {
        result = WMT_NO_DATA;
        goto done;
    }
    if ( 0 != strcasecmp( p, "registers" ) )
    {
        result = WMT_NO_DATA;
        goto done;
    }

    //
    // We potentially have a range file now - it should be under the same
    // directory. Note: We know this fits, as 'registers' is a longer string.
    //
    strcpy( p, "range" );

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleRegisterLineForRanges
///
/// @brief  Handles register line(s) for section initialisation with ranges.
///
/// Handles register lines as required for section initialisation:
///  - The first register line will give us the register line length and bytes
///    per register count.
///  - Potentially all lines handled will be used towards calculating the address
///    increment. If the increment is 1, only the first two lines need to be
///    looked at, otherwise we store the read addresses and the increment will
///    need to be calculated later (i.e. in conjunction with reading the range
///    file).
///    Note: this does make the assumption that the amount of register lines
///    handled are sufficient to have encountered the first contiguous block of
///    registers for the registers section being initialised.
///
/// @param  pLine               The register line to handle.
/// @param  lineLength          The length of the given register line.
/// @param  pSection            The register section being initialised.
/// @param  pLineLength         Receives the constant line length to use for the
///                             section's registers file.
/// @param  pBytesPerRegister   Receives the bytesPerRegister setting to use
///                             for the section's registers.
/// @param  pAddressIncrement   Receives the address increment to use for the
///                             section's registers.
/// @param  pReadAddresses      The collection of read addresses to be updated.
///
/// @retval WMT_SUCCESS             Address increment detected; all
///                                 initialisation now complete.
/// @retval WMT_INVALID_PARAMETER   Given parameter(s) invalid.
/// @retval WMT_NO_DATA             Basic initialisation complete, but address
///                                 increment not yet detected; either more
///                                 register lines need to be handled and/or
///                                 this must be calculated in conjunction with
///                                 the range file.
/// @retval WMT_REG_NOT_PRESENT     Given line was not a register line.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::HandleRegisterLineForRanges( const char                *pLine,
                                                               size_t                    lineLength,
                                                               RegisterSection           *pSection,
                                                               size_t                    *pLineLength,
                                                               size_t                    *pBytesPerRegister,
                                                               size_t                    *pAddressIncrement,
                                                               std::vector<unsigned int> *pReadAddresses
                                                             )
{
    RESULT          result = WMT_REG_NOT_PRESENT;
    unsigned int    regAddr;
    char            colonStr[2];
    int             fieldCount;
    
    // We need all of our parameters to handle the register file line correctly.
    if ( !pLine || 0 == lineLength || !pSection || !pLineLength ||
         !pBytesPerRegister || !pAddressIncrement || !pReadAddresses
       )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    // If we have the correct count from the scan, we have a register line.
    fieldCount = sscanf( pLine, "%x%1[:]", &regAddr, colonStr );
    if ( 2 == fieldCount )
    {
        result = WMT_NO_DATA;

        //
        // If we haven't worked out our register line length and bytes per
        // register yet, do so.
        //
        if ( 0 == *pLineLength )
        {
            // Get our constant line length. We add + 1 for the '\n'.
            *pLineLength = lineLength + 1;

            //
            // If we don't yet have our bytesPerRegister setting (we
            // shouldn't), initialise bytesPerRegister.
            //
            if ( 0 == *pBytesPerRegister )
            {
                GetBytesPerRegister( pLine, pBytesPerRegister );
                if ( 8 == *pBytesPerRegister )
                {
                    Log( "64-bit (8-byte) values not yet supported. Treating as 32-bit (4-byte) values." );
                    *pBytesPerRegister = 4;
                }
                pSection->bytesPerRegister = *pBytesPerRegister;
            }
        }

        //
        // Get the information for working out the address increment. If we
        // encounter an address increment of 1, the caller can stop processing
        // lines early.
        // Otherwise, the address increment differs, and the caller needs to
        // process what register addresses it can from its read, so it can
        // ensure it has sufficient addresses to work out the increment once
        // it knows the contiguous blocks from looking at the range file.
        //
        if ( !pReadAddresses->empty() )
        {
            if ( pReadAddresses->back() + 1 == regAddr )
            {
                *pAddressIncrement = 1;
                pSection->addressIncrement = *pAddressIncrement;
                Log( "Detected address increment of %d.\n", *pAddressIncrement );
                result = WMT_SUCCESS;
            }
        }
        pReadAddresses->push_back( regAddr );
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleRangeFileLine
///
/// @brief  Handles range file line(s) for section initialisation.
///
/// @param  pLine               The range line to handle.
/// @param  pSection            The register section being initialised.
/// @param  pTotalOffset        The byte offset where the register line starts
///                             in the file for the first register in the range;
///                             updated by lineLength for all addresses handled
///                             for the range.
/// @param  lineLength          The constant line length within the registers file.
/// @param  pLineNumber         The number of the register line in the file for
///                             the first register in the range; incremented for
///                             all addresses handled for the range.
/// @param  pAddressIncrement   Receives the address increment to use for the
///                             section's registers, if not already set.
/// @param  pReadAddresses      The collection of read addresses to use.
///
/// @retval WMT_SUCCESS             Success.
/// @retval WMT_INVALID_PARAMETER   Given parameter(s) invalid.
/// @retval WMT_READ_FAILED         Failed to parse range from given line.
/// @retval WMT_OUT_OF_RANGE        We've passed the maximum register we care
///                                 about; ignore the rest.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::HandleRangeFileLine( const char                *pLine,
                                                       RegisterSection           *pSection,
                                                       size_t                    *pTotalOffset,
                                                       size_t                    lineLength,
                                                       size_t                    *pLineNumber,
                                                       size_t                    *pAddressIncrement,
                                                       std::vector<unsigned int> *pReadAddresses
                                                     )
{
    RESULT          result = WMT_SUCCESS;
    unsigned int    startAddr, endAddr, increment;
    int             fieldCount;

    // We need all of our parameters to handle the range file line correctly.
    if ( !pLine || !pSection || !pTotalOffset || 0 == lineLength ||
         !pLineNumber || !pAddressIncrement || !pReadAddresses
       )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    //
    // A line in the range file should be in the format:
    //
    // <startAddr>-<endAddr>
    //
    // Where:
    // <startAddr> and <endAddr> are hex values representing the address at
    //                           the start and end of the range, inclusive.
    //
    fieldCount = sscanf( pLine, "%x-%x", &startAddr, &endAddr );
    if ( 2 != fieldCount )
    {
        Log( "Failed to parse range from \"%s\"\n", pLine );
        result = WMT_READ_FAILED;
        goto done;
    }

    //
    // If we haven't yet calculated the address increment, do so now if we
    // have our first contiguous block.
    //
    if ( 0 == *pAddressIncrement && endAddr > startAddr )
    {
        unsigned int firstAddr = MAX_REGISTER;

        for ( size_t index = 0; index < pReadAddresses->size(); index++ )
        {
            unsigned int address = (*pReadAddresses)[index];

            //
            // If we have the first address, this should be the next address
            // within the range.
            //
            if ( MAX_REGISTER != firstAddr )
            {
                // Double-check that the second address is valid and within the range.
                if ( address > firstAddr && address <= endAddr )
                {
                    *pAddressIncrement = address - firstAddr;
                    Log( "Detected address increment of %d.\n", *pAddressIncrement );
                }
                else
                {
                    *pAddressIncrement = 1;
                    Log( "Unable to detect address increment: consecutively read addresses R%Xh and R%Xh don't fit within expected range R%Xh-R%Xh\n",
                         firstAddr,
                         address,
                         startAddr,
                         endAddr
                       );
                    Log( "Defaulting to address increment of %d.\n", *pAddressIncrement );
                }
                pSection->addressIncrement = *pAddressIncrement;
                break;
            }

            //
            // Use the first address we encounter within the range (which
            // should be the startAddr itself anyway) to calculate the
            // address increment.
            //
            if ( address >= startAddr )
                firstAddr = address;
        }
    }

    //
    // Add the range to our layout, startAddr and endAddr inclusive.
    // If we encounter a sparse register before a contiguous block, we won't
    // have calculated an address increment yet - in this case, use an
    // increment of 1 so the loop terminates.
    //
    increment = ( 0 == *pAddressIncrement ) ? 1 : *pAddressIncrement;
    for ( unsigned int regAddr = startAddr; regAddr <= endAddr; regAddr += increment )
    {
        RESULT localResult = AddRegisterToLayout( &pSection->layout,
                                                  regAddr,
                                                  *pTotalOffset,
                                                  lineLength,
                                                  *pLineNumber,
                                                  m_maxRegisterIndex
                                                );
        if ( WMT_OUT_OF_RANGE == localResult )
        {
            // We've passed the maximum register we care about; ignore the rest.
            m_maxLineNumber = *pLineNumber--;
            result = WMT_OUT_OF_RANGE;
            goto done;
        }

        // Now update the offset so far.
        *pTotalOffset += lineLength;
        (*pLineNumber)++;
    }

    result = WMT_SUCCESS;

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: EnsureAddressIncrementSet
///
/// @brief  Ensures the address increment is set for the section.
///
/// @param  pSection            The register section being initialised.
/// @param  addressIncrement    The address increment extracted for the
///                             section's registers.
/// @param  pReadAddresses      The collection of read addresses used.
///
/// @return void
///
////////////////////////////////////////////////////////////////////////////////
void CLinuxDriverCommunication::EnsureAddressIncrementSet( RegisterSection           *pSection,
                                                           size_t                    addressIncrement,
                                                           std::vector<unsigned int> *pReadAddresses
                                                         )
{
    //
    // If we haven't yet set the addressIncrement, default to 1, but make sure
    // we log it - this means we need to read more from the registers file,
    // namely so that we're within the first contiguous block of registers, to
    // work out the actual address increment.
    //
    if ( 0 == addressIncrement )
    {
        pSection->addressIncrement = 1;

        if ( pReadAddresses && !pReadAddresses->empty() )
        {
            if ( MAX_REGISTER != m_maxRegisterIndex && m_maxRegisterIndex < pReadAddresses->back() )
            {
                Log( "Unable to detect address increment: hit user-specified max address (R%Xh) before the first range of contiguous registers.\n",
                     m_maxRegisterIndex
                   );
            }
            else
            {
                Log( "Unable to detect address increment: read addresses R%Xh to R%Xh do not hit the first range of contiguous registers.\n",
                     pReadAddresses->front(),
                     pReadAddresses->back()
                   );
            }
        }
        else
        {
            Log( "Unable to detect address increment: no register addresses read.\n" );
        }

        Log( "Defaulting to address increment of %d.\n", pSection->addressIncrement );
    }
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleRegisterFileLine
///
/// @brief  Handles register file line(s) for section initialisation.
///
/// @param  pLine               The register line to handle.
/// @param  pSection            The register section being initialised.
/// @param  pTotalOffset        The byte offset where the register line starts
///                             in the file; updated by lineLength after handling.
/// @param  lineLength          The constant line length within the registers file.
/// @param  pLineNumber         The number of the register line in the file;
///                             incremented after handling.
/// @param  pBytesPerRegister   Receives the bytesPerRegister setting used
///                             for the section's registers, if not already set.
///
/// @retval WMT_SUCCESS             Success.
/// @retval WMT_INVALID_PARAMETER   Given parameter(s) invalid.
/// @retval WMT_OUT_OF_RANGE        We've passed the maximum register we care
///                                 about; ignore the rest.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::HandleRegisterFileLine( const char                *pLine,
                                                          RegisterSection           *pSection,
                                                          size_t                    *pTotalOffset,
                                                          size_t                    lineLength,
                                                          size_t                    *pLineNumber,
                                                          size_t                    *pBytesPerRegister
                                                        )
{
    RESULT result = WMT_SUCCESS;
    RESULT localResult;
    
    // We need all of our parameters to handle the register file line correctly.
    if ( !pLine || !pSection || !pTotalOffset || 0 == lineLength ||
         !pLineNumber || !pBytesPerRegister
       )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    localResult = AddRegisterToLayout( &pSection->layout,
                                       pLine,
                                       *pTotalOffset,
                                       lineLength,
                                       *pLineNumber,
                                       m_maxRegisterIndex
                                     );
    if ( WMT_OUT_OF_RANGE == localResult )
    {
        // We've passed the maximum register we care about; ignore the rest.
        m_maxLineNumber = *pLineNumber--;
        result = WMT_OUT_OF_RANGE;
        goto done;
    }

    //
    // If we don't yet have our bytesPerRegister setting and we've
    // successfully added a register, initialise bytesPerRegister with
    // the same line.
    //
    if ( 0 == *pBytesPerRegister && WMT_SUCCESS == localResult )
    {
        GetBytesPerRegister( pLine, pBytesPerRegister );
        if ( 8 == *pBytesPerRegister )
        {
            Log( "64-bit (8-byte) values not yet supported. Treating as 32-bit (4-byte) values." );
            *pBytesPerRegister = 4;
        }
        pSection->bytesPerRegister = *pBytesPerRegister;
    }

    // Now update the length so far.
    *pTotalOffset += lineLength;
    (*pLineNumber)++;

done:
    return result;
}

///////////////////////////////////////////////////////////////////////////////
///
//  Function: QueryForCodec
///
/// @brief  Queries the user for the codec to use.
///
/// (no parameters)
///
/// @return The index of the codec selected by the user or WMT_NO_DEVICE if no
//          selection made.
///
///////////////////////////////////////////////////////////////////////////////
size_t CLinuxDriverCommunication::QueryForCodec()
{
    size_t  codecIndex = 0;
    char    input[256];
    int     index = 0;
    bool    done = false;

    if ( 0 == m_codecPaths.size() )
        goto done;

    // Query the user for which codec to use.
    InfoMessage( "\nDetected codecs:\n" );

    for ( std::vector<CodecRegisters>::iterator codecIterator = m_codecRegisters.begin();
          codecIterator != m_codecRegisters.end();
          ++codecIterator
        )
    {
        InfoMessage( "%d: %s\n", index, codecIterator->name );
        index++;
    }

    InfoMessage( "Type 'quit' to exit codec selection.\n" );
    InfoMessage( "Enter the index of the codec you would like to use (e.g. 2)\n" );

    while ( !done )
    {
        bool    codecName = false;
        char    codecString[256];
        size_t  inputValue = 0;

        InfoMessage( "> " );
        if ( !fgets( input, sizeof ( input ), stdin ) )
            Log( "No input for codec to use.\n" );
        else
            Log( "User entered '%s'.\n", input );

        //
        // Convert the input to uppercase and remove and whitespace from the
        // beginning and end.
        //
        char *pInput = input, *pEnd = &input[strlen( input ) - 1];

        while( isspace( *pInput ) )
            ++pInput;

        while( isspace( *pEnd ) && pEnd > pInput )
            --pEnd;

        *(pEnd + 1) = '\0';

#ifdef _WIN32
        strupr( pInput );
#endif
        //
        // Now see if we can work out what codec they want.
        //
        if ( 0 == strcasecmp( pInput, "QUIT" ) )
        {
            // Cancel codec selection.
            codecIndex = (size_t) WMT_NO_DEVICE;
            break;
        }
#ifdef _WIN32
        else if ( 0 == strncmp( pInput, "WM", 2 ) )
#else
        else if ( 0 == strncasecmp( pInput, "WM", 2 ) )
#endif
        {
            // The user entered a codec name.
            snprintf( codecString, 256, "%s", pInput );
            codecName = true;
        }
        else
        {
            // Let's try and convert it to a number.
            char *pStopChar = NULL;
            inputValue = strtoul( pInput, &pStopChar, 10 );
            if ( pStopChar == pInput )
            {
                // This wasn't a number.
                InfoMessage( "Invalid selection (%s), please try again.\n", pInput );
                continue;
            }

            //
            // If the number is greater than the number of codecs it might be
            // the device ID minus the 'WM'.
            //
            if ( inputValue >= m_codecRegisters.size() )
            {
                snprintf( codecString, 256, "WM%s", pInput );
                codecName = true;
            }
        }

        // Either see if we have the named codec or use the index we were given.
        if ( codecName )
        {
            // See if we can find a codec with the same name.
            RESULT result = FindCodecIndex( codecString );
            if ( WMT_NO_DEVICE == result )
            {
                InfoMessage( "%s matches no available codecs, please try again.\n", codecString );
                continue;
            }
            else
            {
                codecIndex = (size_t) result;
            }
        }
        else
        {
            codecIndex = inputValue;
        }

        // If we get here we found the codec.
        InfoMessage( "Selected %s codec.\n", m_codecRegisters[codecIndex].name );
        done = true;
    }

done:
    return codecIndex;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: FindCodecIndex
///
/// @brief  Finds the index of the given codec.
///
/// @param  codecName   The name of the codec.
///
/// @retval >= 0            The index of the device in m_codecRegisters.
/// @retval WMT_NO_DEVICE   No device found with the given name.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::FindCodecIndex( const char *codecName )
{
    RESULT result = WMT_NO_DEVICE;

    for ( size_t index = 0; index < m_codecRegisters.size(); index++ )
    {
        if ( 0 == strcmp( m_codecRegisters[index].name, codecName ) )
        {
            result = (RESULT) index;
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: EnumerateDevices
///
/// @brief  Enumerates the available devices into a space separated list.
///
/// @param  buffer          Buffer to receive the list of devices.
/// @param  bufferLength    Length of the buffer.
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_NO_DEVICE   There are no devices present.
/// @retval WMT_TRUNCATED   The list is truncated - buffer too small.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::EnumerateDevices( char *buffer, size_t bufferLength )
{
    RESULT  result = WMT_SUCCESS;
    char    *pBufferPos = buffer;
    size_t  currentCount = 0;

    if ( !m_codecPaths.size() || NoDevice == m_deviceState )
        goto done;

    for ( std::vector<CodecRegisters>::iterator codecIterator = m_codecRegisters.begin();
          codecIterator != m_codecRegisters.end();
          ++codecIterator
        )
    {
        size_t codecLength = strlen( codecIterator->name );

        //
        // We are going to be adding a space and the codec string, and we need a
        // spot for the terminating null after it. If that doesn't fit, the list
        // is truncated.
        //
        if ( currentCount + codecLength + 2 > bufferLength )
        {
            result = WMT_TRUNCATED;
            goto done;
        }

        // If this isn't the first one, add a space before the codec name.
        if ( codecIterator != m_codecRegisters.begin() )
        {
            *pBufferPos++ = ' ';
            currentCount++;
        }

        // Add the codec name.
        memcpy( pBufferPos, codecIterator->name, codecLength );
        currentCount += codecLength;

        // Increment insertion pointer.
        pBufferPos += codecLength;
    }

done:
    // Add terminating null.
    *pBufferPos = '\0';

    if ( 0 == strlen( buffer ) )
        result = WMT_NO_DEVICE;

    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: SetDevice
///
/// @brief  Sets the device to the one represented by the given name.
///
/// @param  deviceName  The device to switch to.
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_NO_DEVICE   There is no device with the given name.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::SetDevice( const char *deviceName )
{
    RESULT  result = WMT_NO_DEVICE;

    if ( 0 == m_codecPaths.size() )
        goto done;

    for ( std::vector<CCodecPathPair>::iterator codecIterator = m_codecPaths.begin();
          codecIterator != m_codecPaths.end();
          ++codecIterator
        )
    {
        if ( 0 == strcasecmp( deviceName, codecIterator->name ) )
        {
            // Switch over to the newly selected device.
            m_deviceName = codecIterator->name;
            m_deviceState = DeviceReady;
            if ( strstr( codecIterator->path, "registers" ) )
                m_isRegistersFile = true;
            else
                m_isRegistersFile = false;
            result = WMT_SUCCESS;
            break;
        }
    }

    if ( WMT_SUCCESS == result )
    {
        InfoMessage( "Selected codec: %s\n", m_deviceName );
        if ( WMT_SUCCESS == InitialiseRegisterLayout() )
            m_deviceState = DeviceReady;
        else
            m_deviceState = NoDevice;
    }
    else
    {
        InfoMessage( "Couldn't find codec for %s.\n", deviceName );
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: HandleGetRegisterMapLine
///
/// @brief  Handles register file line(s) for GetRegisterMap processing.
///
/// @param  pLine           The register line to handle.
/// @param  pLineNumber     The number of the register line in the file;
///                         incremented after handling.
/// @param  regvals         The collection of register-value pairs to update.
///
/// @retval WMT_SUCCESS             Success.
/// @retval WMT_INVALID_PARAMETER   Given parameter(s) invalid.
/// @retval WMT_NO_DATA             Given line was malformed (i.e. non-register).
///                                 This may happen at the start of the register
///                                 map (e.g. "WM8994 Registers" line at the
///                                 start of the codec on the Nexus S) or for
///                                 the last entry of the exposed register map.
/// @retval WMT_OUT_OF_RANGE        We've passed the maximum register we care
///                                 about; ignore the rest.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::HandleGetRegisterMapLine( const char      *pLine,
                                                            size_t          *pLineNumber,
                                                            CRegValuePairs  &regvals
                                                          )
{
    RESULT          result = WMT_SUCCESS;
    CRegValuePair   regvalpair;
    const char      *pBuffer;
    char            *endCharPointer;

    // We need all of our parameters to handle the register file line correctly.
    if ( !pLine || !pLineNumber )
    {
        result = WMT_INVALID_PARAMETER;
        goto done;
    }

    if ( MAX_REGISTER != m_maxLineNumber && *pLineNumber >= m_maxLineNumber )
    {
        // We've passed the maximum register we care about; ignore the rest.
        result = WMT_OUT_OF_RANGE;
        goto done;
    }

    //
    // This may happen at the start of the register map (e.g. "WM8994
    // Registers" line at the start of the codec on the Nexus S) or for the
    // last entry of the exposed register map.
    // We only report this to the logs before continuing.
    //
    if ( !strchr( pLine, ':' ) )
    {
        Log( "Malformed (non-register) debugfs entry at line %d: \"%s\"\n",
             *pLineNumber,
             pLine
           );
        ( *pLineNumber )++;
        result = WMT_NO_DATA;
        goto done;
    }

    memset( &regvalpair, 0, sizeof( regvalpair ) );

    // Parse the register field.
    memccpy( regvalpair.reg, pLine, ':', sizeof( regvalpair.reg ) );
    endCharPointer = strchr( regvalpair.reg, ':' );
    if ( endCharPointer )
        *endCharPointer = '\0';
    pBuffer = strchr( pLine, ':' ) + 1;

    // Eat all blanks (usually just one).
    while ( isblank( *pBuffer ) )
        ++pBuffer;

    // Parse the value field.
    memccpy( regvalpair.value, pBuffer, '\n', sizeof( regvalpair.value ) );
    endCharPointer = strchr( regvalpair.value, '\n' );
    if ( endCharPointer )
        *endCharPointer = '\0';
    regvals.push_back( regvalpair );

    // Move to the next line.
    ( *pLineNumber )++;

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ReadRegister
///
/// @brief  Reads the given register.
///
/// @param  regAddr The register to read.
/// @param  pValue  Receives the value read.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_REG_NOT_PRESENT Register is not present on device.
/// @retval WMT_READ_FAILED     Unable to read the register. Either the device
///                             is in low power mode or the codec file is in an
///                             unrecognised format.
/// @retval WMT_COMMS_ERR       There was a comm failure.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::ReadRegister( unsigned int regAddr, unsigned int *pValue )
{
    if ( IsQDSP() )
    {
        return ReadQDSPRegister( regAddr, pValue );
    }
    else
    {
        return ReadCodecRegister( regAddr, pValue );
    }
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: BlockRead
///
/// @brief  Performs a block read on the device.
///
/// @param  addr    The address to read from.
/// @param  length  The number of bytes to read.
/// @param  data    Receives the data.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_RESOURCE_FAIL   Failed to allocate read buffer.
/// @retval WMT_COMMS_ERR       Failed to read the codec file.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::BlockRead( unsigned int    addr,
                                             unsigned int    length,
                                             unsigned char   *data
                                           )
{
    if ( IsQDSP() )
    {
        return BlockReadQDSPInChunks( addr, length, data );
    }
    else
    {
        return BlockReadCodec( addr, length, data );
    }
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: WriteRegister
///
/// @brief  Writes the value to the given register.
///
/// @param  regAddr The register to write to.
/// @param  value   The value to write.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_REG_NOT_PRESENT Register is not present on device.
/// @retval WMT_COMMS_ERR       Error writing to codec file.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::WriteRegister( unsigned int regAddr, unsigned int value )
{
    if ( IsQDSP() )
    {
        return WriteQDSPRegister( regAddr, value );
    }
    else
    {
        return WriteCodecRegister( regAddr, value );
    }
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: BlockWrite
///
/// @brief  Performs a block write on the device.
///
/// @param  addr    The address to write to.
/// @param  length  The length of data.
/// @param  data    The data to write.
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_NO_DEVICE   No device present.
/// @retval WMT_COMMS_ERR   Error writing to codec file.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::BlockWrite( unsigned int   addr,
                                              unsigned int   length,
                                              unsigned char  *data
                                            )
{
    if ( IsQDSP() )
    {
        return BlockWriteQDSPInChunks( addr, length, data );
    }
    else
    {
        return BlockWriteCodec( addr, length, data );
    }
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: ReadQDSPRegister
///
/// @brief  Reads the given address from the QDSP after mapping the register
///         address to the algo, bank and offset.
///
/// @param  regAddr The register to read.
/// @param  pValue  Receives the value read.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_REG_NOT_PRESENT Register is not present on device.
/// @retval WMT_READ_FAILED     Unable to read the register. Either the device
///                             is in low power mode or the codec file is in an
///                             unrecognised format.
/// @retval WMT_COMMS_ERR       There was a comm failure.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::ReadQDSPRegister( unsigned int regAddr,
                                                    unsigned int *pValue
                                                  )
{
    RESULT                  result = WMT_SUCCESS;
    unsigned int            algoID = 0;
    unsigned int            offset = 0;
    RegisterLineInfo        lineInfo;
    const RegisterSection   *pSection = FindSection( m_pRegisterSections, regAddr, &lineInfo );

    if ( NULL == pSection )
    {
        // QDSP doesn't know about this address; just return zero.
        DebugMessage( "ReadQDSPRegister: UNKNOWN Register, could not find register section\n" );

        *pValue = 0;
        goto done;
    }
    else if ( 0 == pSection->algorithmID )
    {
        // This is a DSP Header register.
        DebugMessage( "ReadQDSPRegister: DSP Header register\n" );

        *pValue = m_QDSPXMHeader[lineInfo.lineNumber];
        goto done;
    }
    else
    {
        //
        // This is a QDSP algorithm register - translate our register address.
        // Note: nothing uses bank at the moment as we only have a single bank.
        //
        algoID = pSection->algorithmID;
        offset = lineInfo.lineNumber;

        DebugMessage( "ReadQDSPRegister: Algorithm ID: 0x%X, Offset: %u\n", algoID, offset );

        result = ReadFromQDSPAlgorithm( algoID, offset, pValue );
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: BlockReadQDSPInChunks
///
/// @brief  Performs a chunked block read on the device.
///
/// @param  addr    The address to read from.
/// @param  length  The number of bytes to read.
/// @param  data    Receives the data.
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_NO_DEVICE   No device present.
/// @retval WMT_COMM_ERR    There was a comm failure.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::BlockReadQDSPInChunks( unsigned int  addr,
                                                         unsigned int  length,
                                                         unsigned char *data
                                                       )
{
    RESULT  result = WMT_SUCCESS;
    int     remaining_len = length / sizeof( unsigned int );

    while ( remaining_len > 0 )
    {
        // TODO Should we break out early if result is fail?
        if ( remaining_len > MAX_REG_PER_OPERATION )
        {
            result = BlockReadQDSP( addr, MAX_REG_PER_OPERATION, (unsigned int *) data );

            addr += MAX_REG_PER_OPERATION;
            remaining_len -= MAX_REG_PER_OPERATION;
            data += MAX_REG_PER_OPERATION * sizeof( unsigned int );
        }
        else
        {
            result = BlockReadQDSP( addr, remaining_len, (unsigned int *) data );
            remaining_len = 0;
        }
    };

    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: BlockReadQDSP
///
/// @brief  Reads a number of registers from the given address from the QDSP 
///         after mapping the register address to the algo, bank and offset.
///
/// @param  regAddr         The register to read.
/// @param  numRegsToRead   The number of registers to read.
/// @param  pValues          Receives the values read.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_REG_NOT_PRESENT Register is not present on device.
/// @retval WMT_READ_FAILED     Unable to read the register. Either the device
///                             is in low power mode or the codec file is in an
///                             unrecognised format.
/// @retval WMT_COMMS_ERR       There was a comm failure.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::BlockReadQDSP( unsigned int regAddr,
                                                 unsigned int numRegsToRead,
                                                 unsigned int *pValues
                                               )
{
    RESULT                  result = WMT_SUCCESS;
    unsigned int            algoID = 0;
    unsigned int            offset = 0;
    RegisterLineInfo        lineInfo;
    const RegisterSection   *pSection = FindSection( m_pRegisterSections, regAddr, &lineInfo );

    if ( NULL == pSection )
    {
        // QDSP doesn't know about this address; just return zeros.
        DebugMessage( "BlockReadQDSP: UNKNOWN Register, could not find register section\n" );

        for ( size_t i = 0; i < numRegsToRead; i++ )
        {
            *pValues = 0;
            pValues++;
        }

        goto done;
    }
    else if ( 0 == pSection->algorithmID )
    {
        // This is a DSP Header register.
        DebugMessage( "BlockReadQDSP: DSP Header register\n" );

        for ( size_t i = 0; i < numRegsToRead; i++ )
        {
            //
            // Someone may try to read more registers than are in the section.
            // Read the ones that we can read and fill the rest of the values
            // with zeros.
            //
            if ( lineInfo.lineNumber + i < m_QDSPXMHeader.size() )
            {
                *pValues = ByteSwap32bit( m_QDSPXMHeader[lineInfo.lineNumber + i] );
            }
            else
            {
                *pValues = 0;
            }
            pValues++;
        }

        goto done;
    }
    else
    {
        //
        // This is a QDSP algorithm register - translate our register address.
        // Note: nothing uses bank at the moment as we only have a single bank.
        //
        algoID = pSection->algorithmID;
        offset = lineInfo.lineNumber;

        DebugMessage( "BlockReadQDSP: Algorithm ID: 0x%X, Offset: %u\n", algoID, offset );

        result = BlockReadFromQDSPAlgorithm( algoID, offset, numRegsToRead, pValues );
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: WriteQDSPRegister
///
/// @brief  Writes the value to the QDSP after mapping the register address
///         to the algo, bank and offset.
///
/// @param  regAddr The register to write to.
/// @param  value   The value to write.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_REG_NOT_PRESENT Register is not present on device.
/// @retval WMT_COMMS_ERR       Error writing to DSP file.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::WriteQDSPRegister( unsigned int regAddr,
                                                     unsigned int value
                                                   )
{
    RESULT                  result = WMT_SUCCESS;
    unsigned int            algoID = 0;
    unsigned int            offset = 0;
    RegisterLineInfo        lineInfo;
    const RegisterSection   *pSection = FindSection( m_pRegisterSections, regAddr, &lineInfo );

    if ( NULL == pSection )
    {
        // QDSP doesn't know about this address; nothing to do.
        DebugMessage( "WriteQDSPRegister: UNKNOWN Register, could not find register section\n" );
        goto done;
    }
    else if ( 0 == pSection->algorithmID )
    {
        // This is a DSP Header register - we don't want to change these spoofed values.
        DebugMessage( "WriteQDSPRegister: DSP Header register; ignoring write\n" );
        goto done;
    }
    else
    {
        //
        // This is a QDSP algorithm register - translate our register address.
        // Note: nothing uses bank at the moment as we only have a single bank.
        //
        algoID = pSection->algorithmID;
        offset = lineInfo.lineNumber;

        DebugMessage( "WriteQDSPRegister: Algorithm ID: 0x%X, Offset: %u\n", algoID, offset );

        result = WriteToQDSPAlgorithm( algoID, offset, value );
    }

done:
    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: BlockWriteQDSPInChunks
///
/// @brief  Performs a chunked block write on the device.
///
/// @param  addr    The address to write to.
/// @param  length  The number of bytes to write.
/// @param  data    The data to write.
///
/// @retval WMT_SUCCESS     Succeeded.
/// @retval WMT_NO_DEVICE   No device present.
/// @retval WMT_COMM_ERR    There was a comm failure.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::BlockWriteQDSPInChunks( unsigned int  addr,
                                                          unsigned int  length,
                                                          unsigned char *data
                                                        )
{
    RESULT  result = WMT_SUCCESS;
    int     remaining_len = length / sizeof( unsigned int );

    while ( remaining_len > 0 )
    {
        // TODO Should we break out early if result is fail?
        if ( remaining_len > MAX_REG_PER_OPERATION )
        {
            result = BlockWriteQDSP( addr, MAX_REG_PER_OPERATION, (unsigned int *) data );

            addr += MAX_REG_PER_OPERATION;
            remaining_len -= MAX_REG_PER_OPERATION;
            data += MAX_REG_PER_OPERATION * sizeof( unsigned int );
        }
        else
        {
            result = BlockWriteQDSP( addr, remaining_len, (unsigned int *) data );
            remaining_len = 0;
        }
    };

    return result;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: BlockWriteQDSP
///
/// @brief  Writes the value to the QDSP after mapping the register address
///         to the algo, bank and offset.
///
/// @param  regAddr         The register to write to.
/// @param  numRegsToWrite  The number of registers to write.
/// @param  pValues           The values to write.
///
/// @retval WMT_SUCCESS         Succeeded.
/// @retval WMT_NO_DEVICE       No device present.
/// @retval WMT_REG_NOT_PRESENT Register is not present on device.
/// @retval WMT_COMMS_ERR       Error writing to DSP file.
///
////////////////////////////////////////////////////////////////////////////////
RESULT CLinuxDriverCommunication::BlockWriteQDSP( unsigned int regAddr,
                                                  unsigned int numRegsToWrite,
                                                  unsigned int *pValues
                                                )
{
    RESULT                  result = WMT_SUCCESS;
    unsigned int            algoID = 0;
    unsigned int            offset = 0;
    RegisterLineInfo        lineInfo;
    const RegisterSection   *pSection = FindSection( m_pRegisterSections, regAddr, &lineInfo );

    if ( NULL == pSection )
    {
        // QDSP doesn't know about this address; nothing to do.
        DebugMessage( "BlockWriteQDSP: UNKNOWN Register, could not find register section\n" );
        goto done;
    }
    else if ( 0 == pSection->algorithmID )
    {
        // This is a DSP Header register - we don't want to change these spoofed values.
        DebugMessage( "BlockWriteQDSP: DSP Header register; ignoring write\n" );
        goto done;
    }
    else
    {
        //
        // This is a QDSP algorithm register - translate our register address.
        // Note: nothing uses bank at the moment as we only have a single bank.
        //
        algoID = pSection->algorithmID;
        offset = lineInfo.lineNumber;

        DebugMessage( "BlockWriteQDSP: Algorithm ID: 0x%X, Offset: %u\n", algoID, offset );

        result = BlockWriteToQDSPAlgorithm( algoID, offset, numRegsToWrite, pValues );
    }

done:
    return result;
}

///////////////////////////////// END OF FILE //////////////////////////////////
