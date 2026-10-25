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
/// @file   WMVersionUtil.h
/// @brief  Utilities for working with versions.
///
/// @version \$Id: WMVersionUtil.h 16820 2015-01-09 13:47:56Z shackman $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
#ifndef __WMVERSIONUTIL_H__
#define __WMVERSIONUTIL_H__

//
// Include files
//

//
// Definitions
//

/// Our version number type
typedef unsigned int WM_VERSION;

// Default version utils are for 32-bit versions.
#define MAKE_VERSION( major, minor, revision, internalRevision )    \
                                MAKE_32BIT_VERSION( major, minor, revision, internalRevision )
#define MAJOR_VERSION( v )      MAJOR_32BIT_VERSION( v )
#define MINOR_VERSION( v )      MINOR_32BIT_VERSION( v )
#define RELEASE_VERSION( v )    RELEASE_32BIT_VERSION( v )
#define INTERNAL_VERSION( v )   INTERNAL_32BIT_VERSION( v )
#define REVISION( v )           REVISION_32BIT( v )

/// Constructs a 32-bit version number out of its constituents
#define MAKE_32BIT_VERSION( major, minor, revision, internalRevision )   \
                          ( ( ( major            ) << 24 )   | \
                            ( ( minor            ) << 16 )   | \
                            ( ( revision         ) <<  8 )   | \
                            ( ( internalRevision ) <<  0 )     \
                          )
/// Extracts the major version out of a 32-bit version number
#define MAJOR_32BIT_VERSION( v ) ( ( ( v ) & 0xFF000000 ) >> 24 )
/// Extracts the minor version out of a 32-bit version number
#define MINOR_32BIT_VERSION( v ) ( ( ( v ) & 0x00FF0000 ) >> 16 )
/// Extracts the public release version out of a 32-bit version number
#define RELEASE_32BIT_VERSION( v ) ( ( ( v ) & 0x0000FF00 ) >> 8 )
/// Extracts the internal release version out of a 32-bit version number
#define INTERNAL_32BIT_VERSION( v ) ( ( v ) & 0x000000FF )
/// Extracts the release version out of a 32-bit version number
#define REVISION_32BIT( v ) ( ( v ) & 0x0000FFFF )

/// Constructs a 24-bit version number out of its constituents
#define MAKE_24BIT_VERSION( major, minor, patch )   \
                          ( ( ( ( major ) << 16 )   | \
                              ( ( minor ) <<  8 )   | \
                              ( ( patch ) <<  0 )     \
                            ) & 0x00FFFFFF            \
                          )
/// Extracts the major version out of a 24-bit version number
#define MAJOR_24BIT_VERSION( v ) ( ( ( v ) & 0xFF0000 ) >> 16 )
/// Extracts the minor version out of a 24-bit version number
#define MINOR_24BIT_VERSION( v ) ( ( ( v ) & 0x00FF00 ) >> 8 )
/// Extracts the patch version out of a 24-bit version number
#define PATCH_24BIT_VERSION( v ) ( ( ( v ) & 0x0000FF ) )

/// Constructs a 16-bit version number out of its constituents
#define MAKE_16BIT_VERSION( major, minor )   \
                          ( ( ( ( major ) <<  8 )   | \
                              ( ( minor ) <<  0 )     \
                            ) & 0x0000FFFF            \
                          )
/// Extracts the major version out of a 16-bit version number
#define MAJOR_16BIT_VERSION( v ) ( (unsigned int ) ( ( ( v ) & 0xFF00 ) >> 8 ) )
/// Extracts the minor version out of a 16-bit version number
#define MINOR_16BIT_VERSION( v ) ( (unsigned int ) ( ( v ) & 0x00FF ) )

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Inline functions
////////////////////////////////////////////////////////////////////////////////


#endif  // __WMVERSIONUTIL_H__
///////////////////////////////// END OF FILE //////////////////////////////////