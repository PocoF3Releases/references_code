/* Copyright (c) 2016 Cirrus Logic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/
#ifndef _UAPI_MSM_CIRRUS_SPK_PR_H
#define _UAPI_MSM_CIRRUS_SPK_PR_H

#ifndef _WIN32
#include <linux/types.h>
#include <linux/ioctl.h>
#else
#include <stdint.h>
#endif

//
// Literals for IOCTL calls
//
#define CRUS_MODULE_ID_RX               0x00000001
#define CRUS_MODULE_ID_TX               0x00000002

#define CRUS_PARAM_RX_INIT_DEFAULT      0x00A1AF02
#define CRUS_PARAM_RX_DIAG              0x00A1AF03
#define CRUS_PARAM_RX_INIT_EXT          0x00A1AF05
#define CRUS_PARAM_RX_EXC_FETCH         0x00A1AF06
#define CRUS_PARAM_RX_TEMP_FETCH        0x00A1AF07
#define CRUS_PARAM_RX_SET_TEMP_CAL      0x00A1AF08

#define CRUS_PARAM_TX_DIAG              0x00A1BF03
#define CRUS_PARAM_TX_FO_FETCH          0x00A1BF05
#define CRUS_PARAM_TX_TEMP_CAL_FETCH    0x00A1BF06
#define CRUS_PARAM_TX_INIT_EXT          0x00A1BF08

#define CRUS_PARAM_REGISTER_DATA        0x00A1AF80
#define CRUS_PARAM_REGISTER_SELECT      0x00A1AF81
#define CRUS_PARAM_ALGORITHM_COUNT      0x00A1AF82
#define CRUS_PARAM_ALGORITHM_INFO       0x00A1AF83

#define CRUS_AFE_PARAM_ID_ENABLE        0x00010203

#define CRUS_GB_FF_PARAMS               0x00ABCDEF
#define CRUS_GB_FB_PARAMS               0x00ABCDEE

#define SPK_PROT_IOCTL_MAGIC            'a'

//
// Get and Set IOCTL definitions
//
#define CRUS_GB_IOCTL_GET   _IOWR( SPK_PROT_IOCTL_MAGIC, 219, \
        struct crus_gb_ioctl_header )
#define CRUS_GB_IOCTL_SET   _IOWR( SPK_PROT_IOCTL_MAGIC, 220, \
        struct crus_gb_ioctl_header )

//
// A structure for composing QDSP messages.
//
struct crus_gb_ioctl_header
{
    uint32_t    module_id;
    uint32_t    param_id;
    uint32_t    data_length;
    void        *data;
};

//
// A structure for composing QDSP messages.
//
struct QDSP_message
{
    //
    // The algo_count below is the number of algorithms this
    // command applies to i.e. it's possible to read/write to
    // multiple algos with the same command, apparently!
    //
    uint32_t    algo_count;
    uint32_t    algorithm_id;
    uint32_t    bank_offset;
    uint32_t    data_length;
    uint32_t    data[1];
};

#endif // _UAPI_MSM_CIRRUS_SPK_PR_H
///////////////////////////////// END OF FILE //////////////////////////////////