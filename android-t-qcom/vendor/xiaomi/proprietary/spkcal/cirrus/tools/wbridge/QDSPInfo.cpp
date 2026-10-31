////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2016 Cirrus Logic International (UK) Ltd.  All rights reserved.
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
/// @file   QDSPInfo.cpp
/// @brief  QDSP-specific information.
///
/// @version \$Id: QDSPInfo.cpp 18495 2016-07-04 14:16:15Z lbogdan $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////

//
// Include files
//
#include "QDSPInfo.h"
#include "WISCEBridgeUtil.h"
#include <cstring>

//
// Definitions
//

////////////////////////////////////////////////////////////////////////////////
///
//  Function: IsQDSPDevice
///
/// @brief  Checks whether the given device is the QDSP device.
///
/// @param  deviceName  Device name to check.
///
/// @return True if the device is the QDSP device; false otherwise.
///
////////////////////////////////////////////////////////////////////////////////
bool IsQDSPDevice( const char *deviceName )
{
    if ( 0 == strcasecmp( deviceName, QDSP_NAME ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

///////////////////////////////// END OF FILE //////////////////////////////////