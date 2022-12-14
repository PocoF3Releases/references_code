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

#include "bta_closure_api.h"
#include "bta_bap_uclient_api.h"
#include "btif_common.h"
#include "btif_storage.h"
#include "osi/include/thread.h"

// Capabilities to be stored on CodecConfig structure :
// Supported_Sampling_Frequencies  ( codec config sampling rate )
// Audio_Channel_Counts ( codec config channel mode )
// Supported_Frame_Durations ( 1st byte of codec specific 1 ) 
// Max_Supported_LC3_Frames_Per_SDU  ( 2nd byte of codec specific 1)
// Preferred_Audio_Contexts ( 3&4 bytes of codec specific 1 )
// Supported_Octets_Per_Codec_Frame ( first 4 bytes codec specific 2 )
/* Vendor_Specific Metadata ( first 4 bytes of codec specific 3)
 *    1st byte conveys LC3Q support, //CHECK?
 *    2nd byte conveys LC3Q version
 */

// Configurations to be stored in CodecConfig structure :
// Sampling_Frequency  ( codec config sampling rate )
// Audio_Channel_Allocation ( codec config channel mode )
// Frame_Duration  ( 5th bye of codec specific 1 ) 
// LC3_Blocks_Per_SDU ( 6th byte of codec specific 1 )
// Preferred_Audio_Contexts ( 7&8 bytes of codec specific 1 )
// Octets_Per_Codec_Frame ( 2 bytes  )  ( 5&6th bytes of codec specific 2 )
// LC3Q preferred (1 byte)  ( 7th byte of codec specific 2 )
/* Vendor_Specific Metadata ( first 4 bytes of codec specific 3)
 *    1st byte conveys LC3Q support,
 *    2nd byte conveys LC3Q version
 */

// Vendor specific meta data for AptX Adaptive R3 to be stored in
// first 4 Bytes of codec specific 3, 5-10 octets are RFU as of now
// 2nd byte conveys codec encoder version number,
// 3rd byte conveys codec decoder version number,
// 4th byte conveys minimum supported frame duration
// 5th byte conveys feature map of which only first bit is used currently, others are RFU right now
// 1st byte is not used for AptX Adaptive R3 because trying to keep up with LC3Q format

#include <base/logging.h>
#include <hardware/bluetooth.h>
#include <hardware/bt_pacs_client.h>
#include <hardware/bt_bap_uclient.h>

using bluetooth::bap::pacs::CodecConfig;
using bluetooth::bap::pacs::CodecChannelMode;
using bluetooth::bap::ucast::QosConfig;
using bluetooth::bap::ucast::CISConfig;
using bluetooth::bap::ucast::CIGConfig;
using bluetooth::bap::pacs::CodecIndex;

constexpr uint8_t  CAPA_SUP_FRAME_DUR_INDEX           = 0x00; // CS1
constexpr uint8_t  CAPA_MAX_SUP_LC3_FRAMES_INDEX      = 0x01; // CS1
constexpr uint8_t  CAPA_PREF_AUDIO_CONT_INDEX         = 0x02; // CS1
constexpr uint8_t  CAPA_SUP_OCTS_PER_FRAME_INDEX      = 0x00; // CS2

constexpr uint8_t  CAPA_VENDOR_METADATA_LC3Q_PREF_INDEX = 0x00; // CS3
constexpr uint8_t  CAPA_CODEC_ENCODER_VER_INDEX  = 0x01; // CS3
constexpr uint8_t  CAPA_CODEC_DECODER_VER_INDEX  = 0x02; // CS3

// constexpr uint8_t  CAPA_MAX_ALLOWABLE_RAMP_INDEX      = 0x01; //CS3
constexpr uint8_t  CAPA_MIN_SUP_FRAME_DUR_INDEX       = 0x03; //CS3
constexpr uint8_t  CAPA_FEATURE_MAP_INDEX             = 0x04; //Only first bit used, others are RFU right now

constexpr uint8_t  TEMP_CODEC_VER_INDEX       = 0x07;

constexpr uint8_t  CONFIG_FRAME_DUR_INDEX           = 0x04; // CS1
constexpr uint8_t  CONFIG_LC3_BLOCKS_INDEX          = 0x05; // CS1
constexpr uint8_t  CONFIG_PREF_AUDIO_CONT_INDEX     = 0x06; // CS1
constexpr uint8_t  CONFIG_OCTS_PER_FRAME_INDEX      = 0x04; // CS2
constexpr uint8_t  CONFIG_LC3Q_PREF_INDEX           = 0x06; // CS2
constexpr uint8_t  CONFIG_VENDOR_METADATA_LC3Q_PREF_INDEX    = 0x00; // CS3
constexpr uint8_t  CONFIG_VENDOR_METADATA_ENCODER_VER_INDEX  = 0x01; // CS3
constexpr uint8_t  CONFIG_VENDOR_METADATA_DECODER_VER_INDEX  = 0x02; // CS3

// capabilities
bool UpdateCapaSupFrameDurations(CodecConfig *config , uint8_t sup_frame) {
  config->codec_specific_1 &= ~(0xFF << (CAPA_SUP_FRAME_DUR_INDEX * 8));
  config->codec_specific_1 |= sup_frame << (CAPA_SUP_FRAME_DUR_INDEX * 8);
  return true;
}

bool UpdateCapaMaxSupLc3Frames(CodecConfig *config,
                                uint8_t max_sup_lc3_frames) {
  config->codec_specific_1 &= ~(0xFF << (CAPA_MAX_SUP_LC3_FRAMES_INDEX * 8));
  config->codec_specific_1 |= max_sup_lc3_frames <<
                               (CAPA_MAX_SUP_LC3_FRAMES_INDEX * 8);
  return true;
}

bool UpdateCapaPreferredContexts(CodecConfig *config, uint16_t contexts) {
  config->codec_specific_1 &= ~(0xFFFF << (CAPA_PREF_AUDIO_CONT_INDEX * 8));
  config->codec_specific_1 |= contexts << (CAPA_PREF_AUDIO_CONT_INDEX * 8);
  return true;
}

bool UpdateCapaSupOctsPerFrame(CodecConfig *config,
                              uint32_t octs_per_frame) {
  config->codec_specific_2 &= ~(0xFFFFFFFF <<
                          (CAPA_SUP_OCTS_PER_FRAME_INDEX * 8));
  config->codec_specific_2 |= octs_per_frame <<
                              (CAPA_SUP_OCTS_PER_FRAME_INDEX * 8);
  return true;
}

bool UpdateCapaVendorMetaDataLc3QPref(CodecConfig *config, bool lc3q_pref) {
  config->codec_specific_3 &= ~(0xFF << (CAPA_VENDOR_METADATA_LC3Q_PREF_INDEX * 8));
  config->codec_specific_3 |= lc3q_pref << (CAPA_VENDOR_METADATA_LC3Q_PREF_INDEX * 8);
  return true;
}

bool UpdateCapaVendorMetaDataCodecEncoderVer(CodecConfig *config, uint8_t encoder_version) {
  config->codec_specific_3 &= ~(0xFF << (CAPA_CODEC_ENCODER_VER_INDEX * 8));
  config->codec_specific_3 |= encoder_version << (CAPA_CODEC_ENCODER_VER_INDEX * 8);
  // BTIF_TRACE_IMP(" UpdateCapaCodecEncoderVer version number = %ld", (config->codec_specific_3 >>(8*CAPA_CODEC_ENCODER_VER_INDEX)) & 0xff);
  return true;
}

bool UpdateCapaVendorMetaDataCodecDecoderVer(CodecConfig *config, uint8_t decoder_version) {
  // BTIF_TRACE_IMP(" UpdateCapaCodecDecoderVer decoder_version = %ld",decoder_version);
  config->codec_specific_3 &= ~(0xFF << (CAPA_CODEC_DECODER_VER_INDEX * 8));
  config->codec_specific_3 |= decoder_version << (CAPA_CODEC_DECODER_VER_INDEX * 8);
  return true;
}

bool UpdateCapaMinSupFrameDur(CodecConfig *config, uint8_t min_sup_frame_dur) {
  // BTIF_TRACE_IMP(" UpdateMinSupFrameDur min_sup_frame_dur = %ld",min_sup_frame_dur);
  config->codec_specific_3 &= ~(0xFF << (CAPA_MIN_SUP_FRAME_DUR_INDEX * 8));
  config->codec_specific_3 |= min_sup_frame_dur << (CAPA_MIN_SUP_FRAME_DUR_INDEX * 8);
  return true;
}

bool UpdateCapaFeatureMap(CodecConfig *config, uint8_t feature_map) { //check
  // BTIF_TRACE_IMP(" UpdateFeatureMap feature_map = %ld",feature_map);
  uint64_t value = 0xFF;
  config->codec_specific_3 &= ~(value << (CAPA_FEATURE_MAP_INDEX * 8));
  config->codec_specific_3 |= static_cast<uint64_t>(feature_map) << (CAPA_FEATURE_MAP_INDEX * 8);
  return true;
}

uint8_t GetCapaVendorMetaDataCodecEncoderVer(CodecConfig *config) {
  return (config->codec_specific_3 >>
                (8*CAPA_CODEC_ENCODER_VER_INDEX)) & 0xff;
}

uint8_t GetCapaVendorMetaDataCodecDecoderVer(CodecConfig *config) {
  return (config->codec_specific_3 >>
                (8*CAPA_CODEC_DECODER_VER_INDEX)) & 0xff;
}

uint8_t GetCapaMinSupFrameDur(CodecConfig *config) {
  return (config->codec_specific_3 >>
                (8*CAPA_MIN_SUP_FRAME_DUR_INDEX)) & 0xff;
}

uint8_t GetCapaFeatureMap(CodecConfig *config) {
  return (config->codec_specific_3 >>
                (8*CAPA_FEATURE_MAP_INDEX)) & 0xff;
}

uint8_t GetCapaSupFrameDurations(CodecConfig *config) {
  return (config->codec_specific_1 >> (8*CAPA_SUP_FRAME_DUR_INDEX)) & 0xff;
}

uint8_t GetCapaMaxSupLc3Frames(CodecConfig *config) {
  if (((config->codec_specific_1 >>
                (8*CAPA_MAX_SUP_LC3_FRAMES_INDEX)) & 0xff) == 0x0) {
    uint8_t max_chnl_count = 0;
    LOG(ERROR) << __func__
               << ": Max Sup LC3 frames is 0, deriving based on chnl count";
    if(static_cast<uint16_t> (config->channel_mode) &
       static_cast<uint16_t> (CodecChannelMode::CODEC_CHANNEL_MODE_STEREO)) {
      max_chnl_count = 2;
    } else if(static_cast<uint16_t> (config->channel_mode) &
           static_cast<uint16_t> (CodecChannelMode::CODEC_CHANNEL_MODE_MONO)) {
      max_chnl_count = 1;
    }
    return max_chnl_count;
  } else {
    return (config->codec_specific_1 >>
               (8*CAPA_MAX_SUP_LC3_FRAMES_INDEX)) & 0xff;
  }
}

uint16_t GetCapaPreferredContexts(CodecConfig *config) {
  return (config->codec_specific_1 >>
                (8*CAPA_PREF_AUDIO_CONT_INDEX)) & 0xffff;
}

uint32_t GetCapaSupOctsPerFrame(CodecConfig *config) {
  return (config->codec_specific_2 >>
                (8*CAPA_SUP_OCTS_PER_FRAME_INDEX)) & 0xffffffff;
}

bool GetCapaVendorMetaDataLc3QPref(CodecConfig *config) {
  if (((config->codec_specific_3 >>
        (8*CAPA_VENDOR_METADATA_LC3Q_PREF_INDEX)) & 0xff) == 0x0) {
    return false;
  } else
    return true;
}

// Configurations
bool UpdateFrameDuration(CodecConfig *config , uint8_t frame_dur) {
  uint64_t value = 0xFF;
  config->codec_specific_1 &= ~(value << (CONFIG_FRAME_DUR_INDEX * 8));
  config->codec_specific_1 |=  static_cast<uint64_t>(frame_dur)  <<
                               (CONFIG_FRAME_DUR_INDEX * 8);
  return true;
}

bool UpdateLc3BlocksPerSdu(CodecConfig *config, uint8_t lc3_blocks_per_sdu) {
  uint64_t value = 0xFF;
  config->codec_specific_1 &= ~(value << (CONFIG_LC3_BLOCKS_INDEX * 8));
  config->codec_specific_1 |=  static_cast<uint64_t>(lc3_blocks_per_sdu) <<
                               (CONFIG_LC3_BLOCKS_INDEX * 8);
  return true;
}

bool UpdatePreferredAudioContext(CodecConfig *config ,
                                    uint16_t pref_audio_context) {
  uint64_t value = 0xFFFF;
  config->codec_specific_1 &= ~(value << (CONFIG_PREF_AUDIO_CONT_INDEX * 8));
  config->codec_specific_1 |=  static_cast<uint64_t>(pref_audio_context) <<
                               (CONFIG_PREF_AUDIO_CONT_INDEX * 8);
  return true;
}

bool UpdateOctsPerFrame(CodecConfig *config , uint16_t octs_per_frame) {
  uint64_t value = 0xFFFF;
  config->codec_specific_2 &= ~(value << (CONFIG_OCTS_PER_FRAME_INDEX * 8));
  config->codec_specific_2 |=
                    static_cast<uint64_t>(octs_per_frame) <<
                    (CONFIG_OCTS_PER_FRAME_INDEX * 8);
  return true;
}

bool UpdateLc3QPreference(CodecConfig *config , bool lc3q_pref) {
  uint64_t value = 0xFF;
  config->codec_specific_2 &= ~(value << (CONFIG_LC3Q_PREF_INDEX * 8));
  config->codec_specific_2 |=
                    static_cast<uint64_t>(lc3q_pref) <<
                    (CONFIG_LC3Q_PREF_INDEX * 8);
  LOG(WARNING) << __func__
               << ": lc3q_pref cs2: " << loghex(config->codec_specific_2);
  return true;
}

bool UpdateVendorMetaDataLc3QPref(CodecConfig *config, bool lc3q_pref) {
  uint64_t value = 0xFF;
  config->codec_specific_3 &= ~(value << (CONFIG_VENDOR_METADATA_LC3Q_PREF_INDEX * 8));
  config->codec_specific_3 |= static_cast<uint64_t>(lc3q_pref) <<
                                (CONFIG_VENDOR_METADATA_LC3Q_PREF_INDEX * 8);
  return true;
}

bool UpdateVendorMetaDataCodecEncoderVer(CodecConfig *config, uint8_t lc3q_ver) {
  uint64_t value = 0xFF;
  config->codec_specific_3 &= ~(value << (CONFIG_VENDOR_METADATA_ENCODER_VER_INDEX * 8));
  config->codec_specific_3 |= static_cast<uint64_t>(lc3q_ver) <<
                                  (CONFIG_VENDOR_METADATA_ENCODER_VER_INDEX * 8);
  // BTIF_TRACE_IMP(" UpdateVendorMetaDataLc3QEncoderVer encoder version number = %ld",(config->codec_specific_3 >>(8*CAPA_CODEC_ENCODER_VER_INDEX)) & 0xff);
  return true;
}

bool UpdateVendorMetaDataCodecDecoderVer(CodecConfig *config, uint8_t lc3q_ver) {
  uint64_t value = 0xFF;
  config->codec_specific_3 &= ~(value << (CONFIG_VENDOR_METADATA_DECODER_VER_INDEX * 8));
  config->codec_specific_3 |= static_cast<uint64_t>(lc3q_ver) <<
                                  (CONFIG_VENDOR_METADATA_DECODER_VER_INDEX * 8);
  // BTIF_TRACE_IMP(" UpdateVendorMetaDataLc3QDecoderVer decoder version number = %ld",(config->codec_specific_3 >>(8*CAPA_CODEC_DECODER_VER_INDEX)) & 0xff);
  return true;
}

uint8_t GetFrameDuration(CodecConfig *config) {
  return (config->codec_specific_1 >> (8*CONFIG_FRAME_DUR_INDEX)) & 0xff;
}

uint8_t GetLc3BlocksPerSdu(CodecConfig *config) {
  return (config->codec_specific_1 >> (8*CONFIG_LC3_BLOCKS_INDEX)) & 0xff;
}

uint16_t GetPreferredAudioContext(CodecConfig *config) {
  return (config->codec_specific_1 >>
                    (8*CONFIG_PREF_AUDIO_CONT_INDEX)) & 0xffff;
}

uint16_t GetOctsPerFrame(CodecConfig *config) {
  return (config->codec_specific_2 >> (8*CONFIG_OCTS_PER_FRAME_INDEX)) & 0xffff;
}

uint8_t GetLc3QPreference(CodecConfig *config) {
  LOG(WARNING) << __func__ << ": lc3q_pref cs2: "
      << loghex((config->codec_specific_2 >> (8*CONFIG_LC3Q_PREF_INDEX)) & 0xff);
  return (config->codec_specific_2 >>
                (8*CONFIG_LC3Q_PREF_INDEX)) & 0xff;
}

uint8_t GetVendorMetaDataLc3QPref(CodecConfig *config) {
  return (config->codec_specific_3 >>
                (8*CONFIG_VENDOR_METADATA_LC3Q_PREF_INDEX)) & 0xff;
}

uint8_t GetVendorMetaDataCodecEncoderVer(CodecConfig *config) {
  return (config->codec_specific_3 >>
                (8*CONFIG_VENDOR_METADATA_ENCODER_VER_INDEX)) & 0xff;
}

uint8_t GetVendorMetaDataCodecDecoderVer(CodecConfig *config) {
  return (config->codec_specific_3 >>
                (8*CONFIG_VENDOR_METADATA_DECODER_VER_INDEX)) & 0xff;
}

bool IsCodecConfigEqual(CodecConfig *src_config, CodecConfig *dst_config) {

  if(src_config == nullptr || dst_config == nullptr) {
    return false;
  }

  if (src_config->codec_type == CodecIndex::CODEC_INDEX_SOURCE_LC3)
    LOG(INFO) << __func__ << ": src_config's codec_type = CodecIndex::CODEC_INDEX_SOURCE_LC3";
  else if(src_config->codec_type == CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE)
    LOG(INFO) << __func__ << ": src_config's codec_type = CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE";

  if (dst_config->codec_type == CodecIndex::CODEC_INDEX_SOURCE_LC3)
    LOG(INFO) << __func__ << ": dst_config's codec_type = CodecIndex::CODEC_INDEX_SOURCE_LC3";
  else if(dst_config->codec_type == CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE)
    LOG(INFO) << __func__ << ": dst_config's codec_type = CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE";

  // first check if passed codec configs are configurations or
  // capabilities using first byte of codec specific 1
  // Dbt - 1st Byte of CS1 is frame durations

  bool is_src_capability = src_config->codec_specific_1 & 0XFF;
  bool is_dst_capability = dst_config->codec_specific_1 & 0XFF;

  // check the codec type
  if(src_config->codec_type != dst_config->codec_type) {
    LOG(ERROR) << __func__ << ": No match for codec type ";
    return false;
  }

  // check sample rate
  if(src_config->sample_rate != dst_config->sample_rate) {
    LOG(ERROR) << __func__ << ": No match for sample rate";
    return false;
  }

  // check channel mode
  if(!(static_cast<int>(src_config->channel_mode) &
       static_cast<int>(dst_config->channel_mode))) {
    LOG(ERROR) << __func__ << ": No match for channel mode ";
    return false;
  }

  LOG(WARNING) << __func__
               << ": is_src_capability: " << loghex(is_src_capability)
               << ", is_dst_capability: " << loghex(is_dst_capability);

  if(is_src_capability && is_dst_capability) {
    if(src_config->codec_specific_1 != dst_config->codec_specific_1 ||
       src_config->codec_specific_2 != dst_config->codec_specific_2 ||
       src_config->codec_specific_3 != dst_config->codec_specific_3) {
      LOG(WARNING) << __func__ << ": No match for CS params. ";
      return false;
    }
  } else if (!is_src_capability && !is_dst_capability) {
    LOG(INFO) << __func__ << ": Comparison for both configs ";

    if (src_config->codec_type == CodecIndex::CODEC_INDEX_SOURCE_LC3) {
      uint8_t src_frame_dur = GetFrameDuration(src_config);
      uint8_t src_lc3_blocks_per_sdu = GetLc3BlocksPerSdu(src_config);
      uint16_t src_octs_per_frame = GetOctsPerFrame(src_config);
      uint8_t src_lc3q_pref = GetLc3QPreference(src_config);
      uint16_t src_pref_audio_context = GetPreferredAudioContext(src_config);

      uint8_t dst_frame_dur = GetFrameDuration(dst_config);
      uint8_t dst_lc3_blocks_per_sdu = GetLc3BlocksPerSdu(dst_config);
      uint16_t dst_octs_per_frame = GetOctsPerFrame(dst_config);
      uint8_t dst_lc3q_pref = GetLc3QPreference(dst_config);
      uint16_t dst_pref_audio_context = GetPreferredAudioContext(dst_config);

      if(src_frame_dur != dst_frame_dur) {
        LOG(ERROR) << __func__ << ": Frame Dur not match with existing config ";
        return false;
      }

      if(src_lc3_blocks_per_sdu != dst_lc3_blocks_per_sdu) {
        LOG(ERROR) << __func__ << ": Lc3 blocks not match with existing config ";
        return false;
      }

      if(src_octs_per_frame != dst_octs_per_frame) {
        LOG(ERROR) << __func__ << ": Octs per frame not match with existing config ";
        return false;
      }

      if(src_lc3q_pref != dst_lc3q_pref) {
        LOG(ERROR) << __func__ << ": Lc3Q pref not match with existing config ";
        return false;
      }

      if (!(src_pref_audio_context & dst_pref_audio_context)) {
        LOG(ERROR) << __func__ << ": pref_audio_context not match with existing config ";
        return false;
      }
    } else if (src_config->codec_type == CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE) {
      LOG(INFO) << __func__ << ": Codec type is AptX Adaptive LE ";
      return true;
    }


  } else if(is_src_capability || is_dst_capability) {
    CodecConfig *capa_config = is_src_capability ? src_config:dst_config;
    CodecConfig *oth_config = is_src_capability ? dst_config:src_config;

    if (capa_config->codec_type == CodecIndex::CODEC_INDEX_SOURCE_LC3) {
      uint8_t capa_sup_frame_dur = GetCapaSupFrameDurations(capa_config);
      uint8_t capa_max_sup_lc3_frames = GetCapaMaxSupLc3Frames(capa_config);
      uint16_t capa_min_sup_octs = GetCapaSupOctsPerFrame(capa_config) & 0xFFFF;
      uint16_t capa_max_sup_octs = (GetCapaSupOctsPerFrame(capa_config)
                               & 0xFFFF0000) >> 16;
      bool capa_lc3q_pref = GetCapaVendorMetaDataLc3QPref(capa_config);
      uint16_t capa_pref_audio_context = GetCapaPreferredContexts(capa_config);

      uint8_t frame_dur = GetFrameDuration(oth_config);
      uint8_t lc3_blocks_per_sdu = GetLc3BlocksPerSdu(oth_config);
      uint16_t octs_per_frame = GetOctsPerFrame(oth_config);
      uint8_t lc3q_pref = GetLc3QPreference(oth_config);
      uint16_t dst_pref_audio_context = GetPreferredAudioContext(oth_config);

      LOG(WARNING) << __func__
                   << ": capa_sup_frame_dur: " << loghex(capa_sup_frame_dur)
                   << ", frame_dur: " << loghex(frame_dur);

      LOG(WARNING) << __func__
                   << ": capa_lc3q_pref: " << capa_lc3q_pref
                   << ", lc3q_pref: " << loghex(lc3q_pref);

      LOG(WARNING) << __func__
                   << ": capa_pref_audio_context: " << capa_pref_audio_context
                   << ", dst_pref_audio_context: " << dst_pref_audio_context;

      LOG(WARNING) << __func__
                   << ": capa_max_sup_lc3_frames: " << loghex(capa_max_sup_lc3_frames)
                   << ", lc3_blocks_per_sdu: " << loghex(lc3_blocks_per_sdu)
                   << ", channel_mode: "
                   << loghex(static_cast<uint8_t> (oth_config->channel_mode));

      LOG(WARNING) << __func__
                   << ": octs_per_frame: " << octs_per_frame
                   << ", capa_min_sup_octs: " << capa_min_sup_octs
                   << ", capa_max_sup_octs: " << capa_max_sup_octs;

      if (!(capa_sup_frame_dur & (0x01 << frame_dur))) {
        LOG(ERROR) << __func__ << ": No match for frame duration ";
        return false;
      }

      if(capa_max_sup_lc3_frames && lc3_blocks_per_sdu) {
        if(capa_max_sup_lc3_frames < lc3_blocks_per_sdu *
                        static_cast<uint8_t> (oth_config->channel_mode)) {
          LOG(ERROR) << __func__ << ": blocks per sdu exceeds the capacity ";
          return false;
        }
      }

      if (octs_per_frame < capa_min_sup_octs ||
          octs_per_frame > capa_max_sup_octs) {
        LOG(ERROR) << __func__ << ": octs per frame not in limits ";
        return false;
      }

      if (!(capa_pref_audio_context & dst_pref_audio_context)) {
        LOG(ERROR) << __func__ << ": No Match for Audio context";
        return false;
      }
    } else if (src_config->codec_type == CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE) {
      LOG(INFO) << __func__ << ": Codec type is AptX Adaptive LE ";
      return true;
    }

  }
  return true;
}

bool IsQosConfigEqual(QosConfig *src_config, QosConfig *dst_config) {
  bool is_params_same = true;
  CIGConfig * src_cig_cfg = &src_config->cig_config;
  CIGConfig * dst_cig_cfg = &dst_config->cig_config;

  std::vector<CISConfig> *src_cis_cfgs = &src_config->cis_configs;
  std::vector<CISConfig> *dst_cis_cfgs = &dst_config->cis_configs;

  if((src_cis_cfgs->size() != dst_cis_cfgs->size())) {
    LOG(WARNING) << __func__  << ": Count is different ";
    return false;
  }

  if(src_cig_cfg->cig_id != dst_cig_cfg->cig_id ||
     src_cig_cfg->cis_count != dst_cig_cfg->cis_count ||
     src_cig_cfg->packing !=  dst_cig_cfg->packing ||
     src_cig_cfg->framing != dst_cig_cfg->framing ||
     src_cig_cfg->max_tport_latency_m_to_s !=
                       dst_cig_cfg->max_tport_latency_m_to_s ||
     src_cig_cfg->max_tport_latency_s_to_m !=
                     dst_cig_cfg->max_tport_latency_s_to_m ||
     src_cig_cfg->sdu_interval_m_to_s[0] !=
                     dst_cig_cfg->sdu_interval_m_to_s[0] ||
     src_cig_cfg->sdu_interval_m_to_s[1] !=
                     dst_cig_cfg->sdu_interval_m_to_s[1] ||
     src_cig_cfg->sdu_interval_m_to_s[2] !=
                     dst_cig_cfg->sdu_interval_m_to_s[2] ||
     src_cig_cfg->sdu_interval_s_to_m[0] !=
                     dst_cig_cfg->sdu_interval_s_to_m[0] ||
     src_cig_cfg->sdu_interval_s_to_m[1] !=
                     dst_cig_cfg->sdu_interval_s_to_m[1] ||
     src_cig_cfg->sdu_interval_s_to_m[2] !=
                       dst_cig_cfg->sdu_interval_s_to_m[2]) {
    LOG(WARNING) << __func__  << " cig params are different ";
    return false;
  }

  for(uint8_t i = 0; i < src_cis_cfgs->size() ; i++) {
    CISConfig src_cis = src_cis_cfgs->at(i);
    CISConfig dst_cis = dst_cis_cfgs->at(i);
    if(src_cis.cis_id  ==  dst_cis.cis_id &&
       src_cis.max_sdu_m_to_s == dst_cis.max_sdu_m_to_s &&
       src_cis.max_sdu_s_to_m == dst_cis.max_sdu_s_to_m &&
       src_cis.phy_m_to_s == dst_cis.phy_m_to_s  &&
       src_cis.phy_s_to_m == dst_cis.phy_s_to_m  &&
       src_cis.rtn_m_to_s == dst_cis.rtn_m_to_s  &&
       src_cis.rtn_s_to_m == dst_cis.rtn_s_to_m) {
    } else {
      is_params_same = false;
      break;
    }
  }
  LOG(WARNING) << __func__  << ": is_params_same : "
                            << loghex(is_params_same);
  return is_params_same;
}

uint8_t GetCodecType(CodecConfig *config) {
  return static_cast<uint8_t>(config->codec_type);
}

// Uses 8th Byte of cs4
bool UpdateTempVersionNumber(CodecConfig *config, uint8_t codec_version) {
  uint64_t value = 0xFF;
  // BTIF_TRACE_IMP(" UpdateTempVersionNumber codec_version = %ld",codec_version);
  config->codec_specific_4 &= ~(value << (TEMP_CODEC_VER_INDEX * 8));
  config->codec_specific_4 |= static_cast<uint64_t>(codec_version) << (TEMP_CODEC_VER_INDEX * 8);
  return true;
}

uint8_t GetTempVersionNumber(CodecConfig *config) {
  return (config->codec_specific_4 >>
                (8*TEMP_CODEC_VER_INDEX)) & 0xff;
}
