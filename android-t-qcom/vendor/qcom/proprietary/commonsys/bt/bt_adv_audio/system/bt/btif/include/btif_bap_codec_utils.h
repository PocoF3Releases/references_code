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

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <hardware/bt_pacs_client.h>
#include <hardware/bt_bap_uclient.h>
#include "bt_types.h"

using bluetooth::bap::pacs::CodecConfig;
using bluetooth::bap::ucast::QosConfig;

bool UpdateCapaSupFrameDurations(CodecConfig *config , uint8_t sup_frame);

bool UpdateCapaMaxSupLc3Frames(CodecConfig *config,
                                uint8_t max_sup_lc3_frames);


bool UpdateCapaPreferredContexts(CodecConfig *config, uint16_t contexts);


bool UpdateCapaSupOctsPerFrame(CodecConfig *config,
                                          uint32_t octs_per_frame);

bool UpdateCapaVendorMetaDataLc3QPref(CodecConfig *config, bool lc3q_pref);

bool UpdateCapaVendorMetaDataCodecEncoderVer(CodecConfig *config, uint8_t encoder_version);

bool UpdateCapaVendorMetaDataCodecDecoderVer(CodecConfig *config, uint8_t decoder_version);

bool UpdateCapaMinSupFrameDur(CodecConfig *config, uint8_t min_sup_frame_dur);

bool UpdateCapaFeatureMap(CodecConfig *config, uint8_t feature_map);

uint8_t GetCapaVendorMetaDataCodecEncoderVer(CodecConfig *config);

uint8_t GetCapaVendorMetaDataCodecDecoderVer(CodecConfig *config);

uint8_t GetCapaMinSupFrameDur(CodecConfig *config);

uint8_t GetCapaFeatureMap(CodecConfig *config);

uint8_t GetCapaSupFrameDurations(CodecConfig *config);

uint8_t GetCapaMaxSupLc3Frames(CodecConfig *config);

uint16_t GetCapaPreferredContexts(CodecConfig *config);

uint32_t GetCapaSupOctsPerFrame(CodecConfig *config);

bool GetCapaVendorMetaDataLc3QPref(CodecConfig *config);

// configurations
bool UpdateFrameDuration(CodecConfig *config , uint8_t frame_dur);

bool UpdateLc3BlocksPerSdu(CodecConfig *config,
                                uint8_t lc3_blocks_per_sdu) ;

bool UpdateOctsPerFrame(CodecConfig *config , uint16_t octs_per_frame);

bool UpdateLc3QPreference(CodecConfig *config , bool lc3q_pref);

bool UpdateVendorMetaDataLc3QPref(CodecConfig *config, bool lc3q_pref);

bool UpdateVendorMetaDataCodecEncoderVer(CodecConfig *config, uint8_t lc3q_ver);

bool UpdateVendorMetaDataCodecDecoderVer(CodecConfig *config, uint8_t lc3q_ver);

bool UpdatePreferredAudioContext(CodecConfig *config ,
                                    uint16_t pref_audio_context);

uint8_t GetFrameDuration(CodecConfig *config);

uint8_t GetLc3BlocksPerSdu(CodecConfig *config);

uint16_t GetOctsPerFrame(CodecConfig *config);

uint8_t GetLc3QPreference(CodecConfig *config);

uint8_t GetVendorMetaDataLc3QPref(CodecConfig *config);

uint8_t GetVendorMetaDataCodecEncoderVer(CodecConfig *config);

uint8_t GetVendorMetaDataCodecDecoderVer(CodecConfig *config);

uint16_t GetPreferredAudioContext(CodecConfig *config);

bool IsCodecConfigEqual(CodecConfig *src_config, CodecConfig *dst_config);

bool IsQosConfigEqual(QosConfig *src_config, QosConfig *dst_config);

uint8_t GetCodecType(CodecConfig *config);

uint8_t GetTempVersionNumber(CodecConfig *config);

bool UpdateTempVersionNumber(CodecConfig *config, uint8_t codec_version);
