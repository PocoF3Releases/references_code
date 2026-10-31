///////////////////////////////////////////////////////////////////////////////
///
// Copyright (c) 2007-2018 Cirrus Logic International (UK) Ltd.  All rights reserved.
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
///
/// @file   WISCEBridgeVersion.h
/// @brief  The version number for the WISCEBridge project.
///
/// @version $Id$
/// @version 20258 last modified 24-April-2018 11:37:57.
///
/// @note   WISCEBridgeVersion.h file is automatically generated from
///         Version.xml using VersionHeader.xsl.
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
///////////////////////////////////////////////////////////////////////////////
#ifndef __WISCEBRIDGEVERSION_H__
#define __WISCEBRIDGEVERSION_H__

//
// Include files
//
#ifndef RC_INVOKED
// The resource compiler can't cope with this.
#include <WMVersionUtil.h>
#endif // RC_INVOKED

//
// Definitions
//

/// Major version number - should only change for a breaking functionality upgrade
#define VERSION_MAJOR                   3
/// Minor version number - should change for a significant piece of new functionality
#define VERSION_MINOR                   10
/// Public release version number - changes every public release
#define VERSION_REVISION                0
/// Internal release version number - changes every release
#define VERSION_INTERNAL_REVISION       7
/// Build version - SVN revision
#define VERSION_BUILD                   20258

#if defined( WM_PRODUCTION_BUILD ) || defined( RC_INVOKED )
    /// Definitions for a full production build which will be released
    /// Special build string (ASCII)
#   define SPECIAL_BUILD                ""
    /// Special build string (Unicode)
#   define SPECIAL_BUILDW               L""
#else   // => !WM_PRODUCTION_BUILD
    /// Definitions for a development build
/// Special build string (ASCII)
#   define SPECIAL_BUILD                "Development build "
/// Special build string (Unicode)
#   define SPECIAL_BUILDW               L"Development build "
#endif  // WM_PRODUCTION_BUILD

/// Company name as a string (ASCII)
#define COMPANY_STRA                    "Cirrus Logic International (UK) Ltd"
/// Product name as a string (ASCII)
#define PRODUCT_STRA                    "WISCEBridge Interactive Setup and Configuration Environment"
/// Trademarks as a string (ASCII)
#define TRADEMARK_STRA                  "WISCEBridge"
/// Major version number as a string (ASCII)
#define VERSION_MAJOR_STR               "3"
/// Minor version number as a string (ASCII)
#define VERSION_MINOR_STR               "10"
/// Public release version number as a string (ASCII)
#define VERSION_REVISION_STR            "0"
/// Internal release version number as a string (ASCII)
#define VERSION_INTERNAL_REVISION_STR   "7"
/// Build version as a string (ASCII)
#define VERSION_BUILD_STR               "20258"
/// Copyright years string (ASCII)
#define COPYRIGHT_YEARS_STR             "2007-2018"
/// Copyright years string (ASCII)
#define COPYRIGHT_STRA                  "Copyright (c) " COPYRIGHT_YEARS_STR " " COMPANY_STRA

/// Company name as a string (Unicode)
#define COMPANY_STRW                    L"Cirrus Logic International (UK) Ltd"
/// Product name as a string (Unicode)
#define PRODUCT_STRW                    L"WISCEBridge Interactive Setup and Configuration Environment"
/// Trademarks as a string (Unicode)
#define TRADEMARK_STRW                  L"WISCEBridge"
/// Major version number as a string (Unicode)
#define VERSION_MAJOR_STRW              L"3"
/// Minor version number as a string (Unicode)
#define VERSION_MINOR_STRW              L"10"
/// Public release version number as a string (Unicode)
#define VERSION_REVISION_STRW           L"0"
/// Internal release version number as a string (Unicode)
#define VERSION_INTERNAL_REVISION_STRW  L"7"
/// Build version as a string (Unicode)
#define VERSION_BUILD_STRW              L"20258"
/// Copyright years string (Unicode)
#define COPYRIGHT_YEARS_STRW            L"2007-2018"
/// Copyright years string (Unicode)
#define COPYRIGHT_STRW                  L"Copyright (c) " COPYRIGHT_YEARS_STRW L" " COMPANY_STRW

/// Date and time of build
#define BUILD_DATE_STR                  "24-April-2018 11:37:57"
/// Date and time of build
#define BUILD_DATE_STRW                 L"24-April-2018 11:37:57"

//
// Definitions
//

/// The current version
#define VERSION         MAKE_VERSION( VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_INTERNAL_REVISION )

///
/// @{
/// Strings and definitions for the resource files.
///

/// The version as a string, e.g. 1.2.5.3 (ASCII)
#define VERSION_STRA            VERSION_MAJOR_STR "." VERSION_MINOR_STR "." VERSION_REVISION_STR "." VERSION_INTERNAL_REVISION_STR
/// The version as a string, e.g. 1.2.5.3 (Unicode)
#define VERSION_STRW            VERSION_MAJOR_STRW L"." VERSION_MINOR_STRW L"." VERSION_REVISION_STRW L"." VERSION_INTERNAL_REVISION_STRW

/// The long version as a string, e.g. "1.2.5.3.1877 special" (ASCII)
#define VERSION_LONG_STRA       VERSION_STRA "." VERSION_BUILD_STR " " SPECIAL_BUILD
/// The long version as a string, e.g. "1.2.5.3.1877 special" (Unicode)
#define VERSION_LONG_STRW       VERSION_STRW L"." VERSION_BUILD_STRW L" " SPECIAL_BUILDW

#ifdef UNICODE
/// The version as a string, e.g. 1.2.5.3 (TCHAR)
#   define VERSION_STR          VERSION_STRW
#   define VERSION_LONG_STR     VERSION_LONG_STRW
#   define COPYRIGHT_STR        COPYRIGHT_STRW
#   define COMPANY_STR          COMPANY_STRW
#   define PRODUCT_STR          PRODUCT_STRW
#   define TRADEMARK_STR        TRADEMARK_STRW
#else
/// The version as a string, e.g. 1.2.5.3 (TCHAR)
#   define VERSION_STR          VERSION_STRA
#   define VERSION_LONG_STR     VERSION_LONG_STRA
#   define COPYRIGHT_STR        COPYRIGHT_STRA
#   define COMPANY_STR          COMPANY_STRA
#   define PRODUCT_STR          PRODUCT_STRA
#   define TRADEMARK_STR        TRADEMARK_STRA
#endif

/// The version for the FILEVER section of the VERSIONINFO resource
#define FILEVER         VERSION_MAJOR,VERSION_MINOR,VERSION_REVISION,VERSION_INTERNAL_REVISION
/// The version for the PRODUCTVER section of the VERSIONINFO resource
#define PRODUCTVER      VERSION_MAJOR,VERSION_MINOR,VERSION_REVISION,VERSION_INTERNAL_REVISION
/// The COMPANY as a string
#define STRCOMPANY  COMPANY_STRA "\0"
/// The PRODUCT as a string
#define STRPRODUCTNAME  PRODUCT_STRA "\0"
/// The Trademarks as a string
#define STRTRADEMARKS   TRADEMARK_STRA "\0"
/// The FILEVER as a string
#define STRFILEVER      VERSION_STRA "\0"
/// The PRODUCTVER as a string
#define STRPRODUCTVER   VERSION_STRA "\0"
/// The build version as a string
#define STRBUILDVER     "Build " VERSION_BUILD_STR "\0"
/// Copyright years string
#define STRCOPYRIGHT    COPYRIGHT_STRA "\0"
/// Current year string
#define STRYEAR         "2018"
/// The special build version as a string
#define STRSPECIALBLD   SPECIAL_BUILD "\0"

/// @}

#endif // __WISCEBRIDGEVERSION_H__
//////////////////////////////// END OF FILE ///////////////////////////////////
