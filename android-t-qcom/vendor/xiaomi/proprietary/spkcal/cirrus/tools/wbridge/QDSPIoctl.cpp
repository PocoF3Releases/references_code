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
/// @file   QDSPIoctl.cpp
/// @brief  Functionality for talking to the QDSP device via IOCTL calls.
///
/// @version \$Id: QDSPIoctl.cpp 18539 2016-08-01 10:46:15Z lbogdan $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////

//
// Include files
//
#include "QDSPIoctl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>

#ifndef _WIN32
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "msm-cirrus-spk-pr.h"

#ifdef _WIN32
//
// We're compiling with Linux as well, so we can't use the secure versions
// of the CRTL functions.
//
#pragma warning( disable: 4996 )
#endif

#ifndef _WIN32

////////////////////////////////////////////////////////////////////////////////
///
//  Function: fw_ioctl
///
/// @brief  Makes an IOCTL call to the QDSP driver file.
///
/// @param  fd       The driver file handle.
/// @param  ioctl_t  The IOCTL type e.g. get or set.
/// @param  module   The module ID the call is intended for e.g. TX or RX.
/// @param  param    The parameter ID the call is intended for e.g. ALGO COUNT.
/// @param  datal    The length of the payload data passed in.
/// @param  data     The payload data to be sent or received to/from the device.
///
/// @return An int representing the operation result < 0 indicates an error.
///
////////////////////////////////////////////////////////////////////////////////
int CQDSPIoctl::fw_ioctl( int       fd,
                          uint64_t  ioctl_t,
                          uint32_t  module,
                          uint32_t  param,
                          uint32_t  datal,
                          void      *data
                        )
{
    struct crus_gb_ioctl_header info;
    int ec;

    info.module_id = module;
    info.param_id = param;
    info.data_length = datal;
    info.data = data;

    ec = ioctl(fd, ioctl_t, &info);
    if (ec < 0)
        fprintf(stderr, "%s: error: %d\n", __func__, ec);
    return ec;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: get
///
/// @brief  Makes a GET IOCTL call to the QDSP driver file.
///
/// @param  fd       The driver file handle.
/// @param  module   The module ID the call is intended for e.g. TX or RX.
/// @param  param    The parameter ID the call is intended for e.g. ALGO COUNT.
/// @param  datal    The length of the payload data passed in.
/// @param  data     The payload data to be sent or received to/from the device.
///
/// @return An int representing the operation result < 0 indicates an error.
///
////////////////////////////////////////////////////////////////////////////////
int CQDSPIoctl::get( int fd, uint32_t module, uint32_t param, uint32_t datal, void *data )
{
    return fw_ioctl( fd, CRUS_GB_IOCTL_GET, module, param, datal, data );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: set
///
/// @brief  Makes a SET IOCTL call to the QDSP driver file.
///
/// @param  fd       The driver file handle.
/// @param  module   The module ID the call is intended for e.g. TX or RX.
/// @param  param    The parameter ID the call is intended for e.g. ALGO COUNT.
/// @param  datal    The length of the payload data passed in.
/// @param  data     The payload data to be sent or received to/from the device.
///
/// @return An int representing the operation result < 0 indicates an error.
///
////////////////////////////////////////////////////////////////////////////////
int CQDSPIoctl::set( int fd, uint32_t module, uint32_t param, uint32_t datal, void *data )
{
    return fw_ioctl( fd, CRUS_GB_IOCTL_SET, module, param, datal, data );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: get_tx_alg_count
///
/// @brief  Makes a get algorithm count call to the QDSP driver file on the TX
///         module.
///
/// @param  fd       The driver file handle.
/// @param  datal    The length of the payload data passed in.
/// @param  data     The payload data to be sent or received to/from the device.
///
/// @return An int representing the operation result < 0 indicates an error.
///
////////////////////////////////////////////////////////////////////////////////
int CQDSPIoctl::get_tx_alg_count( int fd, uint32_t datal, void *data )
{
    return get( fd, CRUS_MODULE_ID_TX, CRUS_PARAM_ALGORITHM_COUNT, datal, data );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: get_tx_alg_info
///
/// @brief  Makes a get algorithm info call to the QDSP driver file on the TX
///         module. This should return an interleaved array of Algo ID and 
///         algo revision for each algo in the algo count.
///
/// @param  fd       The driver file handle.
/// @param  datal    The length of the payload data passed in.
/// @param  data     The payload data to be sent or received to/from the device.
///
/// @return An int representing the operation result < 0 indicates an error.
///
////////////////////////////////////////////////////////////////////////////////
int CQDSPIoctl::get_tx_alg_info( int fd, uint32_t datal, void *data )
{
    return get( fd, CRUS_MODULE_ID_TX, CRUS_PARAM_ALGORITHM_INFO, datal, data );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: get_tx_register_data
///
/// @brief  Makes a get register data call to the QDSP driver file on the TX
///         module. This is essentially a Read register operation.
///
/// @param  fd       The driver file handle.
/// @param  datal    The length of the payload data passed in.
/// @param  data     The payload data to be sent or received to/from the device.
///
/// @return An int representing the operation result < 0 indicates an error.
///
////////////////////////////////////////////////////////////////////////////////
int CQDSPIoctl::get_tx_register_data( int fd, uint32_t datal, void *data )
{
    return get( fd, CRUS_MODULE_ID_TX, CRUS_PARAM_REGISTER_DATA, datal, data );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: get_rx_alg_count
///
/// @brief  Makes a get algorithm count call to the QDSP driver file on the RX
///         module.
///
/// @param  fd       The driver file handle.
/// @param  datal    The length of the payload data passed in.
/// @param  data     The payload data to be sent or received to/from the device.
///
/// @return An int representing the operation result < 0 indicates an error.
///
////////////////////////////////////////////////////////////////////////////////
int CQDSPIoctl::get_rx_alg_count( int fd, uint32_t datal, void *data )
{
    return get( fd, CRUS_MODULE_ID_RX, CRUS_PARAM_ALGORITHM_COUNT, datal, data );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: get_rx_alg_info
///
/// @brief  Makes a get algorithm info call to the QDSP driver file on the RX
///         module. This should return an interleaved array of Algo ID and 
///         algo revision for each algo in the algo count.
///
/// @param  fd       The driver file handle.
/// @param  datal    The length of the payload data passed in.
/// @param  data     The payload data to be sent or received to/from the device.
///
/// @return An int representing the operation result < 0 indicates an error.
///
////////////////////////////////////////////////////////////////////////////////
int CQDSPIoctl::get_rx_alg_info( int fd, uint32_t datal, void *data )
{
    return get( fd, CRUS_MODULE_ID_RX, CRUS_PARAM_ALGORITHM_INFO, datal, data );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: get_rx_register_data
///
/// @brief  Makes a get register data call to the QDSP driver file on the RX
///         module. This is essentially a Read register operation.
///
/// @param  fd       The driver file handle.
/// @param  datal    The length of the payload data passed in.
/// @param  data     The payload data to be sent or received to/from the device.
///
/// @return An int representing the operation result < 0 indicates an error.
///
////////////////////////////////////////////////////////////////////////////////
int CQDSPIoctl::get_rx_register_data( int fd, uint32_t datal, void *data )
{
    return get( fd, CRUS_MODULE_ID_RX, CRUS_PARAM_REGISTER_DATA, datal, data );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: set_rx_register_data
///
/// @brief  Makes a set register data call to the QDSP driver file on the RX
///         module. This is essentially a Write register operation.
///
/// @param  fd       The driver file handle.
/// @param  datal    The length of the payload data passed in.
/// @param  data     The payload data to be sent or received to/from the device.
///
/// @return An int representing the operation result < 0 indicates an error.
///
////////////////////////////////////////////////////////////////////////////////
int CQDSPIoctl::set_rx_register_data( int fd, uint32_t datal, void *data )
{
    return set( fd, CRUS_MODULE_ID_RX, CRUS_PARAM_REGISTER_DATA, datal, data );
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: set_rx_register_select
///
/// @brief  Makes a set register data call to the QDSP driver file on the RX
///         module. This is essentially a register select operation this needs
///         to be called before a set_rx_register_data to tell the driver which
///         register to return the data for in the get_rx_register_data call.
///
/// @param  fd       The driver file handle.
/// @param  datal    The length of the payload data passed in.
/// @param  data     The payload data to be sent or received to/from the device.
///
/// @return An int representing the operation result < 0 indicates an error.
///
////////////////////////////////////////////////////////////////////////////////
int CQDSPIoctl::set_rx_register_select( int fd, uint32_t datal, void *data )
{
    return set( fd, CRUS_MODULE_ID_RX, CRUS_PARAM_REGISTER_SELECT, datal, data );
}

#endif
///////////////////////////////// END OF FILE //////////////////////////////////