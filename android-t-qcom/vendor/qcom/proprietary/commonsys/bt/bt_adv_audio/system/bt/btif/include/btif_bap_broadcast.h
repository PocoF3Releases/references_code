/*
 * Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
*/
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*******************************************************************************
 *
 *  Filename:      btif_bap_broadcast.h
 *
 *  Description:   Main API header file for all BTIF BAP Broadcast functions
 *                 accessed from internal stack.
 *
 ******************************************************************************/

#ifndef BTIF_BAP_BROADCAST_H
#define BTIF_BAP_BROADCAST_H

#include "bta_av_api.h"
#include "btif_common.h"
#include "btif_sm.h"


/*******************************************************************************
 *  Type definitions for callback functions
 ******************************************************************************/

typedef enum {
  /* Reuse BTA_AV_XXX_EVT - No need to redefine them here */
  BTIF_BAP_BROADCAST_ENABLE_EVT,
  BTIF_BAP_BROADCAST_DISABLE_EVT,
  BTIF_BAP_BROADCAST_START_STREAM_REQ_EVT,
  BTIF_BAP_BROADCAST_STOP_STREAM_REQ_EVT,
  BTIF_BAP_BROADCAST_SUSPEND_STREAM_REQ_EVT,
  BTIF_BAP_BROADCAST_SOURCE_CONFIG_REQ_EVT,
  BTIF_BAP_BROADCAST_CLEANUP_REQ_EVT,
  BTIF_BAP_BROADCAST_SET_ACTIVE_REQ_EVT,
  BTIF_BAP_BROADCAST_REMOVE_ACTIVE_REQ_EVT,
  BTIF_BAP_BROADCAST_SETUP_ISO_DATAPATH_EVT,
  BTIF_BAP_BROADCAST_REMOVE_ISO_DATAPATH_EVT,
  BTIF_BAP_BROADCAST_GENERATE_ENC_KEY_EVT,
  BTIF_BAP_BROADCAST_SET_BROADCAST_CODE_EVT,
  BTIF_BAP_BROADCAST_BISES_SETUP_EVT,
  BTIF_BAP_BROADCAST_BISES_REMOVE_EVT,
  BTIF_BAP_BROADCAST_BIG_SETUP_EVT,
  BTIF_BAP_BROADCAST_BIG_REMOVED_EVT,
  BTIF_BAP_BROADCAST_SETUP_NEXT_BIS_EVENT,
  BTIF_BAP_BROADCAST_PROCESS_HIDL_REQ_EVT,
} btif_bap_broadcast_sm_event_t;

enum {
  BTBAP_CODEC_CHANNEL_MODE_JOINT_STEREO = 0x01 << 2,
  BTBAP_CODEC_CHANNEL_MODE_DUAL_MONO = 0x1 << 3
};
/*******************************************************************************
 *  BTIF AV API
 ******************************************************************************/
bool btif_bap_broadcast_is_active();

uint16_t btif_bap_broadcast_get_sample_rate(uint8_t direction);
uint8_t btif_bap_broadcast_get_ch_mode(uint8_t direction);
uint16_t btif_bap_broadcast_get_framelength(uint8_t direction);
uint32_t btif_bap_broadcast_get_mtu(uint32_t bitrate, uint8_t direction);
uint32_t btif_bap_broadcast_get_bitrate(uint8_t direction);
uint8_t btif_bap_broadcast_get_ch_count();
bool btif_bap_broadcast_is_simulcast_enabled();
/*******************************************************************************
 *
 * Function         btif_dispatch_sm_event
 *
 * Description      Send event to AV statemachine
 *
 * Returns          None
 *
 ******************************************************************************/

/* used to pass events to AV statemachine from other tasks */
void btif_bap_ba_dispatch_sm_event(btif_bap_broadcast_sm_event_t event, void *p_data, int len);


#endif /* BTIF_BAP_BROADCAST_H */

