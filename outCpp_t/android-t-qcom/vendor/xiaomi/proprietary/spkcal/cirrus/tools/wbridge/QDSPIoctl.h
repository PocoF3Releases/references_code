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
/// @file   QDSPIoctl.h
/// @brief  Functionality for talking to the QDSP device via IOCTL calls.
///
/// @version \$Id: QDSPIoctl.h 18495 2016-07-04 14:16:15Z lbogdan $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
#ifndef __QDSPIOCTL_H__
#define __QDSPIOCTL_H__

//
// Include files
//
#include <stdint.h>
#ifndef _WIN32
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "msm-cirrus-spk-pr.h"

//
// Definitions
//

////////////////////////////////////////////////////////////////////////////////
///
//  Class:  CQDSPIoctl
///
/// @brief  Wrapper class to handle QDSP IOCTL calls.
///
////////////////////////////////////////////////////////////////////////////////
class CQDSPIoctl
{
public:
    // Constructors and destructors
    CQDSPIoctl();

    // Static utility methods for calling into the driver
    static int fw_ioctl( int        fd,
                         uint64_t   ioctl_t,
                         uint32_t   module,
                         uint32_t   param,
                         uint32_t   datal,
                         void       *data
                       );

    static int get( int fd, uint32_t module, uint32_t param, uint32_t datal, void *data );
   
    static int set( int fd, uint32_t module, uint32_t param, uint32_t datal, void *data );

    static int get_tx_alg_count( int fd, uint32_t datal, void *data );

    static int get_tx_alg_info( int fd, uint32_t datal, void *data );

    static int get_tx_register_data( int fd, uint32_t datal, void *data );
    
    static int get_rx_alg_count( int fd, uint32_t datal, void *data ); 

    static int get_rx_alg_info( int fd, uint32_t datal, void *data );

    static int get_rx_register_data( int fd, uint32_t datal, void *data );

    static int set_rx_register_data( int fd, uint32_t datal, void *data );

    static int set_rx_register_select( int fd, uint32_t datal, void *data );
};
#endif // __QDSPIOCTL_H__
///////////////////////////////// END OF FILE //////////////////////////////////