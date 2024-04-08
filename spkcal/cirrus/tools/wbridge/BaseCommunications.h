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
/// @file   BaseCommunications.h
/// @brief  Base communication class.
///
/// @version \$Id: BaseCommunications.h 16820 2015-01-09 13:47:56Z shackman $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
#ifndef __BASECOMMUNICATIONS_H__
#define __BASECOMMUNICATIONS_H__

//
// Include files
//
#include "WISCEBridgeUtil.h"
#ifndef _WIN32
#include <string>
#include <limits.h>
#include <sys/types.h>
#endif

//
// Definitions
//

////////////////////////////////////////////////////////////////////////////////
///
//  Class:  CBaseCommunications
///
/// @brief  Base communication class.
///
////////////////////////////////////////////////////////////////////////////////
class CBaseCommunications
{
public:
    CBaseCommunications();
    virtual ~CBaseCommunications();

    //
    // Implemented functions.
    //
    // These functions have base functionality already implemented. Override
    // these if you want specialised handling for them.
    //
    virtual RESULT GetOSInfo( char *buffer, size_t bufferLength );
    virtual RESULT BlockRead( unsigned int  addr,
                              unsigned int  length,
                              unsigned char *data
                            );
    virtual RESULT BlockWrite( unsigned int     addr,
                               unsigned int     length,
                               unsigned char    *data
                             );

    //
    // Mandatory functions.
    //
    // These are required for WISCE to be able to communicate with the device.
    //
    virtual RESULT ReadRegister( unsigned int /*regAddr*/, unsigned int * /*pValue*/ ) = 0;
    virtual RESULT WriteRegister( unsigned int /*regAddr*/, unsigned int /*value*/ ) = 0;

    virtual RESULT EnumerateDevices( char * /*buffer*/, size_t /*bufferLength*/ ) = 0;
    virtual RESULT GetRegisterMap( CRegValuePairs & /*regvals*/ ) = 0;
    virtual RESULT SetDevice( const char * /*deviceName*/ ) = 0;

    virtual const char *GetDeviceName() = 0;

    //
    // Optional functions
    //
    // Implement these if your WISCEBridge will be used to communicate with HiFi2
    // or ADSP cores.
    //
    virtual RESULT BlockRead( std::string   /*target*/,
                              std::string   /*region*/,
                              unsigned int  /*addr*/,
                              unsigned int  /*length*/,
                              unsigned char * /*data*/
                            )
        { return WMT_INVALID_COMMAND; }
    virtual RESULT BlockRead( std::string   /*target*/,
                              unsigned int  /*addr*/,
                              unsigned int  /*length*/,
                              unsigned char * /*data*/
                            )
        { return WMT_INVALID_COMMAND; }

    virtual RESULT BlockWrite( std::string      /*target*/,
                               std::string      /*region*/,
                               unsigned int     /*addr*/,
                               unsigned int     /*length*/,
                               unsigned char    * /*data*/
                             )
        { return WMT_INVALID_COMMAND; }
    virtual RESULT BlockWrite( std::string      /*target*/,
                               unsigned int     /*addr*/,
                               unsigned int     /*length*/,
                               unsigned char    * /*data*/
                             )
        { return WMT_INVALID_COMMAND; }
};

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Inline functions
////////////////////////////////////////////////////////////////////////////////



#endif  // __BASECOMMUNICATIONS_H__
///////////////////////////////// END OF FILE //////////////////////////////////