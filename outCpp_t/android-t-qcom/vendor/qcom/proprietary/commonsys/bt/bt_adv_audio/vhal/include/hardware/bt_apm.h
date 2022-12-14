/*
 * Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
*/
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
       * Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
         copyright notice, this list of conditions and the following
         disclaimer in the documentation and/or other materials provided
         with the distribution.
       * Neither the name of The Linux Foundation nor the names of its
         contributors may be used to endorse or promote products derived
         from this software without specific prior written permission.
 *
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
 **************************************************************************/

#ifndef ANDROID_INCLUDE_BT_APM_H
#define ANDROID_INCLUDE_BT_APM_H

#define BT_APM_MODULE_ID "apm"

#include <vector>

#include <hardware/bluetooth.h>

__BEGIN_DECLS

/* Bluetooth APM Audio Types */
typedef enum {
    BTAPM_VOICE_AUDIO = 0,
    BTAPM_MEDIA_AUDIO,
    BTAPM_BROADCAST_AUDIO
} btapm_audio_type_t;

void call_active_profile_info(const RawAddress& bd_addr, uint16_t audio_type);
int get_active_profile(const RawAddress& bd_addr, uint16_t audio_type);
typedef int (*btapm_active_profile_callback)(const RawAddress& bd_addr, uint16_t audio_type);


typedef struct {
        size_t          size;
        btapm_active_profile_callback active_profile_cb;
}btapm_initiator_callbacks_t;



/** APM interface
 */
typedef struct {

    /** set to sizeof(bt_apm_interface_t) */
    size_t          size;
    /**
     * Register the BtAv callbacks.
     */
    bt_status_t (*init)(btapm_initiator_callbacks_t* callbacks);

    /** updates new active device to stack */
    bt_status_t (*active_device_change)(const RawAddress& bd_addr, uint16_t profile, uint16_t audio_type);

    /** send content control id to stack */
    bt_status_t (*set_content_control_id)(uint16_t content_control_id, uint16_t audio_type);

    /** find if qc lea enabled */
    bool (*is_qclea_enabled)();

    /** find if Aosp Hal Enabled */
    bool (*is_aospLea_enabled)();

    /** Closes the interface. */
    void  (*cleanup)( void );


}bt_apm_interface_t;

__END_DECLS

#endif /* ANDROID_INCLUDE_BT_APM_H */

