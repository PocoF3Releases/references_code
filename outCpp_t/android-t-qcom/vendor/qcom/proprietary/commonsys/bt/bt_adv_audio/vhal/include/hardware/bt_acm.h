/*
 * Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
*/
/******************************************************************************
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *      * Neither the name of The Linux Foundation nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#ifndef ANDROID_INCLUDE_BT_ACM_H
#define ANDROID_INCLUDE_BT_ACM_H

#include <vector>

#include <hardware/bluetooth.h>
#include <hardware/bt_av.h>
#include <hardware/bt_pacs_client.h>

__BEGIN_DECLS

#define BT_PROFILE_ACM_ID "bt_acm_proflie"
using bluetooth::bap::pacs::CodecConfig;
/* Bluetooth ACM connection states */
typedef enum {
  BTACM_CONNECTION_STATE_DISCONNECTED = 0,
  BTACM_CONNECTION_STATE_CONNECTING,
  BTACM_CONNECTION_STATE_CONNECTED,
  BTACM_CONNECTION_STATE_DISCONNECTING
} btacm_connection_state_t;

/* Bluetooth ACM datapath states */
typedef enum {
  BTACM_AUDIO_STATE_REMOTE_SUSPEND = 0,
  BTACM_AUDIO_STATE_STOPPED,
  BTACM_AUDIO_STATE_STARTED,
} btacm_audio_state_t;

/** Callback for connection state change.
 *  state will have one of the values from btacm_connection_state_t
 */
typedef void (*btacm_connection_state_callback)(const RawAddress& bd_addr,
                                                               btacm_connection_state_t state,
                                                               uint16_t contextType);

/** Callback for audiopath state change.
 *  state will have one of the values from btacm_audio_state_t
 */
typedef void (*btacm_audio_state_callback)(const RawAddress& bd_addr,
                                                        btacm_audio_state_t state,
                                                        uint16_t contextType);

/** Callback for audio configuration change.
 *  Used only for the ACM Initiator interface.
 */
typedef void (*btacm_audio_config_callback)(
    const RawAddress& bd_addr, CodecConfig codec_config,
    std::vector<CodecConfig> codecs_local_acmabilities,
    std::vector<CodecConfig> codecs_selectable_acmabilities,
    uint16_t contextType);

/** Callback for metadata change.
 *  state will have one of the values from btacm_audio_state_t
 */
typedef void (*btacm_metadata_callback)(const RawAddress& bd_addr,
                                                 uint16_t contextType);


/** Callback for Call Active Info.
 *  state will have one of the values from btacm_audio_state_t
 */
typedef bool (*btacm_callstate_callback)(void);

/** BT-ACM Initiator callback structure. */
typedef struct {
  /** set to sizeof(btacm_initiator_callbacks_t) */
  size_t size;
  btacm_connection_state_callback connection_state_cb;
  btacm_audio_state_callback audio_state_cb;
  btacm_audio_config_callback audio_config_cb;
  btacm_metadata_callback metadata_cb;
  btacm_callstate_callback is_call_active_cb;
} btacm_initiator_callbacks_t;

/** Represents the standard BT-ACM Initiator interface.
 */
typedef struct {
  /** set to sizeof(btacm_source_interface_t) */
  size_t size;
  /**
   * Register the BtAcm callbacks.
   */
  bt_status_t (*init)(
      btacm_initiator_callbacks_t* callbacks, int max_connected_audio_devices,
      const std::vector<CodecConfig>& codec_priorities);

  /** connect to headset */
  bt_status_t (*connect)(const RawAddress& bd_addr, uint16_t context_type,
                         uint16_t profile_type, uint16_t preferred_context);

  /** dis-connect from headset */
  bt_status_t (*disconnect)(const RawAddress& bd_addr, uint16_t context_type);

  /** sets the connected device as active */
  bt_status_t (*set_active_device)(const RawAddress& bd_addr,
                                   uint16_t context_type);

  /** start stream */
  bt_status_t (*start_stream)(const RawAddress& bd_addr, uint16_t context_type);

  /** stop stream */
  bt_status_t (*stop_stream)(const RawAddress& bd_addr, uint16_t context_type);

  /** configure the codecs settings preferences */
  bt_status_t (*config_codec)(
      const RawAddress& bd_addr,
      std::vector<CodecConfig> codec_preferences,
      uint16_t context_type, uint16_t preferred_context);

  /** configure the codec based on ID*/
  bt_status_t (*change_config_codec)(
      const RawAddress& bd_addr,
      char* Id);

  /** Closes the interface. */
  void (*cleanup)(void);

} btacm_initiator_interface_t;

__END_DECLS

#endif /* ANDROID_INCLUDE_BT_AV_H */
