/*
 * Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
*/
/******************************************************************************
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
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
 *
 ******************************************************************************/

#include <iostream>
#include <string.h>
#include <vector>
#include <stack>
#include <log/log.h>
#include <unordered_map>

#include "bt_types.h"
#include "bt_trace.h"

#include <libxml/parser.h>
#include "btif_bap_codec_utils.h"
#include "btif_vmcp.h"
#include "btif_api.h"
#include <bluetooth/uuid.h>
#include "bta_tmap_api.h"
#include <base/bind.h>
#include <base/callback.h>
#include "bta_closure_api.h"
#include <hardware/bluetooth.h>
#include "btif_common.h"
#include "btif_acm.h"

using namespace std;
using bluetooth::bap::pacs::CodecIndex;
using base::Bind;

unsigned long voice_codec_count, media_codec_count, qos_settings_count;

// holds the value of current profile being parsed from xml
uint8_t current_profile = 1;

CodecIndex current_codec;
uint8_t current_usecase = 1;
uint8_t current_qhs = 0;

Uuid tmas_uuid = Uuid::FromString("00001855-0000-1000-8000-00805F9B34FB");

// Maps so that multiple codec configs can be accommodated
std::unordered_map<CodecIndex, std::vector<codec_config>>vmcp_voice_codec;
std::unordered_map<CodecIndex, std::vector<codec_config>>vmcp_media_codec;
std::unordered_map<CodecIndex, std::vector<qos_config>>vmcp_qos_low_lat_voice;
std::unordered_map<CodecIndex, std::vector<qos_config>>vmcp_qos_low_lat_media;
std::unordered_map<CodecIndex, std::vector<qos_config>>vmcp_qos_high_rel_media;

std::unordered_map<CodecIndex, std::vector<codec_config>>bap_voice_codec;
std::unordered_map<CodecIndex, std::vector<codec_config>>bap_media_codec;
std::unordered_map<CodecIndex, std::vector<qos_config>>bap_qos_low_lat_voice;
std::unordered_map<CodecIndex, std::vector<qos_config>>bap_qos_low_lat_media;
std::unordered_map<CodecIndex, std::vector<qos_config>>bap_qos_high_rel_media;

std::unordered_map<CodecIndex, std::vector<codec_config>>gcp_voice_codec;
std::unordered_map<CodecIndex, std::vector<codec_config>>gcp_media_codec;
std::unordered_map<CodecIndex, std::vector<qos_config>>gcp_qos_low_lat_voice;
std::unordered_map<CodecIndex, std::vector<qos_config>>gcp_qos_low_lat_media;

std::unordered_map<CodecIndex, std::vector<codec_config>>wmcp_media_codec;
std::unordered_map<CodecIndex, std::vector<qos_config>>wmcp_qos_high_rel_media;

//Below are for gcp tx(for G+VBC) config.
std::unordered_map<CodecIndex, std::vector<codec_config>>gcp_tx_voice_codec;
std::unordered_map<CodecIndex, std::vector<codec_config>>gcp_tx_media_codec;
std::unordered_map<CodecIndex, std::vector<qos_config>>gcp_tx_qos_low_lat_voice;
std::unordered_map<CodecIndex, std::vector<qos_config>>gcp_tx_qos_low_lat_media;

//Below are for gcp rx config.
std::unordered_map<CodecIndex, std::vector<codec_config>>gcp_rx_voice_codec;
std::unordered_map<CodecIndex, std::vector<codec_config>>gcp_rx_media_codec;
std::unordered_map<CodecIndex, std::vector<qos_config>>gcp_rx_qos_low_lat_voice;
std::unordered_map<CodecIndex, std::vector<qos_config>>gcp_rx_qos_low_lat_media;

uint8_t get_pref_acm_profile(uint8_t profile, const RawAddress& bd_addr) {
  uint8_t pref_acm_profile = btif_acm_get_pref_profile_type(bd_addr);

  BTIF_TRACE_IMP("%s: profile: %d, prefAcmProfile: %d ",
                                  __func__, profile, pref_acm_profile);

  if (profile == BAP &&
      ((pref_acm_profile == 1) ||
       (pref_acm_profile == 2) ||
       (pref_acm_profile == 3))) {
    pref_acm_profile = TMAP;
  } else {
    pref_acm_profile = profile;
  }
  BTIF_TRACE_IMP("%s: pref_acm_profile: %d", __func__, pref_acm_profile);
  return pref_acm_profile;
}

vector<CodecConfig> get_all_codec_configs(uint8_t profile, uint8_t context,
                                          CodecIndex codec, const RawAddress& bd_addr) {
  vector<CodecConfig> ret_config;
  CodecConfig temp_config;
  vector<codec_config> *vptr = NULL;

  profile = get_pref_acm_profile(profile, bd_addr);

  if (profile == TMAP) {
    if (context == VOICE_CONTEXT) {
      vptr = &vmcp_voice_codec[codec];
    } else if(context == MEDIA_CONTEXT) {
      vptr = &vmcp_media_codec[codec];
    } else {
      // if no valid context is provided, use voice context
      vptr = &vmcp_voice_codec[codec];
    }
  } else if (profile == BAP) {
    if (context == VOICE_CONTEXT) {
      vptr = &bap_voice_codec[codec];
    } else if(context == MEDIA_CONTEXT) {
      vptr = &bap_media_codec[codec];
    } else {
      // if no valid context is provided, use voice context
      vptr = &bap_voice_codec[codec];
    }
  } else if (profile == GCP) {
    if (context == VOICE_CONTEXT) {
      vptr = &gcp_voice_codec[codec];
    } else if(context == MEDIA_CONTEXT) {
      vptr = &gcp_media_codec[codec];
    } else {
      // if no valid context is provided, use media context
      vptr = &gcp_media_codec[codec];
    }
  } else if (profile == WMCP) {
    if(context == MEDIA_CONTEXT) {
      vptr = &wmcp_media_codec[codec];
    } else {
      // if no valid context is provided, use media context
      vptr = &wmcp_media_codec[codec];
    }
  } else if (profile == GCP_RX) {
    if (context == VOICE_CONTEXT) {
      vptr = &gcp_rx_voice_codec[codec];
    } else if(context == MEDIA_CONTEXT) {
      vptr = &gcp_rx_media_codec[codec];
    } else {
      // if no valid context is provided, use media context
      vptr = &gcp_rx_media_codec[codec];
    }
  } else if (profile == GCP_TX) {
    if (context == VOICE_CONTEXT) {
      vptr = &gcp_tx_voice_codec[codec];
    } else if(context == MEDIA_CONTEXT) {
      vptr = &gcp_tx_media_codec[codec];
    } else {
      // if no valid context is provided, use media context
      vptr = &gcp_tx_media_codec[codec];
    }
  }

  if (!vptr){
     return { };
  }

  for (uint8_t i = 0; i < (uint8_t)vptr->size(); i++) {
    memset(&temp_config, 0, sizeof(CodecConfig));

    if (codec == CodecIndex::CODEC_INDEX_SOURCE_LC3)
      temp_config.codec_type = CodecIndex::CODEC_INDEX_SOURCE_LC3;
    else if (codec == CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE)
      temp_config.codec_type = CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE;

    switch (vptr->at(i).freq_in_hz) {
      case SAMPLE_RATE_8000:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_8000;
        break;
      case SAMPLE_RATE_16000:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_16000;
        break;
      case SAMPLE_RATE_24000:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_24000;
        break;
      case SAMPLE_RATE_32000:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_32000;
        break;
      case SAMPLE_RATE_44100:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_44100;
        break;
      case SAMPLE_RATE_48000:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_48000;
        break;
      case SAMPLE_RATE_96000:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_96000;
        break;
      default:
        break;
    }

    if (vptr->at(i).frame_dur_msecs == FRM_DURATION_7_5_MS)
      UpdateFrameDuration(&temp_config,
                          static_cast<uint8_t>(CodecFrameDuration::FRAME_DUR_7_5));
    else if (vptr->at(i).frame_dur_msecs == FRM_DURATION_10_MS)
      UpdateFrameDuration(&temp_config,
                          static_cast<uint8_t>(CodecFrameDuration::FRAME_DUR_10));

    UpdateOctsPerFrame(&temp_config, vptr->at(i).oct_per_codec_frm);
    UpdateTempVersionNumber(&temp_config, vptr->at(i).codec_version);
    ret_config.push_back(temp_config);
  }

  return ret_config;
}

vector<CodecConfig> get_preferred_codec_configs(uint8_t profile,
                                                uint8_t context, CodecIndex codec) {
  vector<CodecConfig> ret_config;
  CodecConfig temp_config;
  vector<codec_config> *vptr = NULL;

  if (profile == TMAP) {
    if (context == VOICE_CONTEXT) {
      vptr = &vmcp_voice_codec[codec];
    } else if(context == MEDIA_CONTEXT) {
      vptr = &vmcp_media_codec[codec];
    } else {
      // if no valid context is provided, use voice context
      vptr = &vmcp_voice_codec[codec];
    }
  } else if (profile == BAP) {
    if (context == VOICE_CONTEXT) {
      vptr = &bap_voice_codec[codec];
    } else if(context == MEDIA_CONTEXT) {
      vptr = &bap_media_codec[codec];
    } else {
      // if no valid context is provided, use voice context
      vptr = &bap_voice_codec[codec];
    }
  } else if (profile == GCP) {
    if (context == VOICE_CONTEXT) {
      vptr = &gcp_voice_codec[codec];
    } else if(context == MEDIA_CONTEXT) {
      vptr = &gcp_media_codec[codec];
    } else {
      // if no valid context is provided, use media context
      vptr = &gcp_media_codec[codec];
    }
  } else if (profile == WMCP) {
    if(context == MEDIA_CONTEXT) {
      vptr = &wmcp_media_codec[codec];
    } else {
      // if no valid context is provided, use media context
      vptr = &wmcp_media_codec[codec];
    }
  } else if (profile == GCP_RX) {
    if (context == VOICE_CONTEXT) {
      vptr = &gcp_rx_voice_codec[codec];
    } else if(context == MEDIA_CONTEXT) {
      vptr = &gcp_rx_media_codec[codec];
    } else {
      // if no valid context is provided, use media context
      vptr = &gcp_rx_media_codec[codec];
    }
  } else if (profile == GCP_TX) {
    if (context == VOICE_CONTEXT) {
      vptr = &gcp_tx_voice_codec[codec];
    } else if(context == MEDIA_CONTEXT) {
      vptr = &gcp_tx_media_codec[codec];
    } else {
      // if no valid context is provided, use media context
      vptr = &gcp_tx_media_codec[codec];
    }
  }

  if (!vptr) {
     return {};
  }

  for (uint8_t i = 0; i < (uint8_t)vptr->size(); i++) {
    if (vptr->at(i).mandatory == 1) {
      memset(&temp_config, 0, sizeof(CodecConfig));

    if (codec == CodecIndex::CODEC_INDEX_SOURCE_LC3)
      temp_config.codec_type = CodecIndex::CODEC_INDEX_SOURCE_LC3;
    else if (codec == CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE)
      temp_config.codec_type = CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE;

      switch (vptr->at(i).freq_in_hz) {
        case SAMPLE_RATE_8000:
          temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_8000;
          break;
        case SAMPLE_RATE_16000:
          temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_16000;
          break;
        case SAMPLE_RATE_24000:
          temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_24000;
          break;
        case SAMPLE_RATE_32000:
          temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_32000;
          break;
        case SAMPLE_RATE_44100:
          temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_44100;
          break;
        case SAMPLE_RATE_48000:
          temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_48000;
          break;
        case SAMPLE_RATE_96000:
          temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_96000;
          break;
        default:
          break;
      }

      if (vptr->at(i).frame_dur_msecs == FRM_DURATION_7_5_MS)
        UpdateFrameDuration(&temp_config,
                            static_cast<uint8_t>(CodecFrameDuration::FRAME_DUR_7_5));
      else if (vptr->at(i).frame_dur_msecs == FRM_DURATION_10_MS)
        UpdateFrameDuration(&temp_config,
                            static_cast<uint8_t>(CodecFrameDuration::FRAME_DUR_10));

      UpdateOctsPerFrame(&temp_config, vptr->at(i).oct_per_codec_frm);

      ret_config.push_back(temp_config);
    }
  }

  return ret_config;
}

vector<QoSConfig> get_all_qos_params(uint8_t profile,
                                     uint8_t context, CodecIndex codec) {
  vector<QoSConfig> ret_config;
  QoSConfig temp_config;
  vector<qos_config> *vptr = NULL;

  if (profile == TMAP) {
    if (context == VOICE_CONTEXT)
       vptr =  &vmcp_qos_low_lat_voice[codec];
    else if (context == MEDIA_LL_CONTEXT)
       vptr = &vmcp_qos_low_lat_media[codec];
    else if (context == MEDIA_HR_CONTEXT)
       vptr = &vmcp_qos_high_rel_media[codec];
  } else if (profile == BAP) {
    if (context == VOICE_CONTEXT)
       vptr =  &bap_qos_low_lat_voice[codec];
    else if (context == MEDIA_LL_CONTEXT)
       vptr = &bap_qos_low_lat_media[codec];
    else if (context == MEDIA_HR_CONTEXT)
       vptr = &bap_qos_high_rel_media[codec];
  } else if (profile == GCP) {
    if (context == VOICE_CONTEXT)
       vptr =  &gcp_qos_low_lat_voice[codec];
    else if (context == MEDIA_LL_CONTEXT)
       vptr = &gcp_qos_low_lat_media[codec];
  } else if (profile == WMCP) {
    if (context == MEDIA_HR_CONTEXT)
       vptr = &wmcp_qos_high_rel_media[codec];
  } else if (profile == GCP_RX) {
    if (context == VOICE_CONTEXT)
       vptr =  &gcp_rx_qos_low_lat_voice[codec];
    else if (context == MEDIA_LL_CONTEXT)
       vptr = &gcp_rx_qos_low_lat_media[codec];
  } else if (profile == GCP_TX) {
    if (context == VOICE_CONTEXT)
       vptr =  &gcp_tx_qos_low_lat_voice[codec];
    else if (context == MEDIA_LL_CONTEXT)
       vptr = &gcp_tx_qos_low_lat_media[codec];
  }

  if (!vptr) {
     return {};
  }

  for (uint8_t i = 0; i < (uint8_t)vptr->size(); i++) {
    memset(&temp_config, 0, sizeof(QoSConfig));

    switch (vptr->at(i).freq_in_hz) {
      case SAMPLE_RATE_8000:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_8000;
        break;
      case SAMPLE_RATE_16000:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_16000;
        break;
      case SAMPLE_RATE_24000:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_24000;
        break;
      case SAMPLE_RATE_32000:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_32000;
        break;
      case SAMPLE_RATE_44100:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_44100;
        break;
      case SAMPLE_RATE_48000:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_48000;
        break;
      case SAMPLE_RATE_96000:
        temp_config.sample_rate = CodecSampleRate::CODEC_SAMPLE_RATE_96000;
        break;
      default:
        break;
    }

    temp_config.sdu_int_micro_secs = vptr->at(i).sdu_int_micro_secs;
    temp_config.framing = vptr->at(i).framing;
    temp_config.max_sdu_size = vptr->at(i).max_sdu_size;
    temp_config.retrans_num = vptr->at(i).retrans_num;
    temp_config.max_trans_lat = vptr->at(i).max_trans_lat;
    temp_config.presentation_delay = vptr->at(i).presentation_delay;
    temp_config.mandatory = vptr->at(i).mandatory;

    ret_config.push_back(temp_config);
  }

  return ret_config;
}

vector<QoSConfig> get_qos_params_for_codec(uint8_t profile, uint8_t context,
                                           CodecSampleRate freq, uint8_t frame_dur,
                                           uint16_t octets, CodecIndex codec,
                                           uint8_t codec_version,
                                           const RawAddress& bd_addr) {
  vector<QoSConfig> ret_config;
  QoSConfig temp_config;
  vector<qos_config> *vptr = NULL;
  uint32_t frame_dur_micro_sec = 0;
  uint32_t local_freq = 0;

  if (frame_dur == static_cast<uint8_t>(CodecFrameDuration::FRAME_DUR_7_5))
    frame_dur_micro_sec = FRM_DURATION_7_5_MS * 1000;
  else if (frame_dur == static_cast<uint8_t>(CodecFrameDuration::FRAME_DUR_10))
    frame_dur_micro_sec = FRM_DURATION_10_MS * 1000;

  switch (freq) {
    case CodecSampleRate::CODEC_SAMPLE_RATE_8000:
      local_freq = SAMPLE_RATE_8000;
      break;
    case CodecSampleRate::CODEC_SAMPLE_RATE_16000:
      local_freq = SAMPLE_RATE_16000;
      break;
    case CodecSampleRate::CODEC_SAMPLE_RATE_24000:
      local_freq = SAMPLE_RATE_24000;
      break;
    case CodecSampleRate::CODEC_SAMPLE_RATE_32000:
      local_freq = SAMPLE_RATE_32000;
      break;
    case CodecSampleRate::CODEC_SAMPLE_RATE_44100:
      local_freq = SAMPLE_RATE_44100;
      break;
    case CodecSampleRate::CODEC_SAMPLE_RATE_48000:
      local_freq = SAMPLE_RATE_48000;
      break;
    case CodecSampleRate::CODEC_SAMPLE_RATE_96000:
      local_freq = SAMPLE_RATE_96000;
      break;
    default:
      break;
  }

  profile = get_pref_acm_profile(profile, bd_addr);

  if (profile == TMAP) {
    if (context == VOICE_CONTEXT)
       vptr =  &vmcp_qos_low_lat_voice[codec];
    else if (context == MEDIA_LL_CONTEXT)
       vptr = &vmcp_qos_low_lat_media[codec];
    else if (context == MEDIA_HR_CONTEXT)
       vptr = &vmcp_qos_high_rel_media[codec];
  } else if (profile == BAP) {
    if (context == VOICE_CONTEXT)
       vptr =  &bap_qos_low_lat_voice[codec];
    else if (context == MEDIA_LL_CONTEXT) {
       BTIF_TRACE_IMP("%s: filling BAP LL vptr", __func__);
       vptr = &bap_qos_low_lat_media[codec];
    } else if (context == MEDIA_HR_CONTEXT) {
       BTIF_TRACE_IMP("%s: filling BAP HR vptr", __func__);
       vptr = &bap_qos_high_rel_media[codec];
    }
  } else if (profile == GCP) {
    if (context == VOICE_CONTEXT)
       vptr =  &gcp_qos_low_lat_voice[codec];
    else if (context == MEDIA_LL_CONTEXT)
       vptr = &gcp_qos_low_lat_media[codec];
  } else if (profile == WMCP) {
    if (context == MEDIA_HR_CONTEXT) {
       BTIF_TRACE_IMP("%s: filling WMCP HR vptr", __func__);
       vptr = &wmcp_qos_high_rel_media[codec];
    }
  } else if (profile == GCP_RX) {
    if (context == VOICE_CONTEXT)
       vptr =  &gcp_rx_qos_low_lat_voice[codec];
    else if (context == MEDIA_LL_CONTEXT)
       vptr = &gcp_rx_qos_low_lat_media[codec];
  } else if (profile == GCP_TX) {
    if (context == VOICE_CONTEXT)
       vptr =  &gcp_tx_qos_low_lat_voice[codec];
    else if (context == MEDIA_LL_CONTEXT)
       vptr = &gcp_tx_qos_low_lat_media[codec];
  }

  if (!vptr) {
     return { };
  }

  BTIF_TRACE_IMP("%s: vptr size: %d", __func__, (uint8_t)vptr->size());
  BTIF_TRACE_IMP("%s: local_freq: %d, frame_dur_micro_sec: %d, octets: %d",
       __func__, local_freq, frame_dur_micro_sec, octets);

  for (uint8_t i = 0; i < (uint8_t)vptr->size(); i++) {
    BTIF_TRACE_IMP("%s: freq_in_hz: %d, sdu_int_micro_secs: %d, max_sdu_size: %d",
       __func__, vptr->at(i).freq_in_hz, vptr->at(i).sdu_int_micro_secs,
       vptr->at(i).max_sdu_size);
    BTIF_TRACE_IMP("%s: Local and vptr matched.", __func__);
    BTIF_TRACE_IMP("%s: check vptr->at(i).codec_version = %d",
       __func__, vptr->at(i).codec_version);
    BTIF_TRACE_IMP("%s: check codec_version = %d",
       __func__, codec_version);
    if (codec == CodecIndex::CODEC_INDEX_SOURCE_LC3 &&
        vptr->at(i).freq_in_hz == local_freq &&
        ((vptr->at(i).sdu_int_micro_secs % frame_dur_micro_sec) == 0) &&
        ((vptr->at(i).max_sdu_size % octets) == 0) &&
        (vptr->at(i).codec_version <= codec_version)) {
      BTIF_TRACE_IMP("%s: Local and vptr matched for LC3.", __func__);
      memset(&temp_config, 0, sizeof(QoSConfig));
      temp_config.sample_rate = freq;
      temp_config.sdu_int_micro_secs = vptr->at(i).sdu_int_micro_secs;
      temp_config.framing = vptr->at(i).framing;
      temp_config.max_sdu_size = vptr->at(i).max_sdu_size;
      temp_config.retrans_num = vptr->at(i).retrans_num;
      temp_config.max_trans_lat = vptr->at(i).max_trans_lat;
      temp_config.presentation_delay = vptr->at(i).presentation_delay;
      temp_config.mandatory = vptr->at(i).mandatory;
      temp_config.codec_version = vptr->at(i).codec_version;
      ret_config.push_back(temp_config);
      BTIF_TRACE_IMP("%s: Got a QoS match return config", __func__);
      return ret_config;
    } else if (codec == CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE &&
               vptr->at(i).freq_in_hz == local_freq) {
      BTIF_TRACE_IMP("%s: Local and vptr matched for AptX Adaptive LE.", __func__);
      memset(&temp_config, 0, sizeof(QoSConfig));
      temp_config.sample_rate = freq;
      temp_config.sdu_int_micro_secs = vptr->at(i).sdu_int_micro_secs;
      temp_config.framing = vptr->at(i).framing;
      temp_config.max_sdu_size = vptr->at(i).max_sdu_size;
      temp_config.retrans_num = vptr->at(i).retrans_num;
      temp_config.max_trans_lat = vptr->at(i).max_trans_lat;
      temp_config.presentation_delay = vptr->at(i).presentation_delay;
      temp_config.mandatory = vptr->at(i).mandatory;
      temp_config.codec_version = vptr->at(i).codec_version;
      ret_config.push_back(temp_config);
      BTIF_TRACE_IMP("%s: Got a QoS match return config", __func__);
      return ret_config;
    }
  }
  BTIF_TRACE_IMP("%s: No match for QoS config return empty", __func__);
  return ret_config;
}

bool is_leaf(xmlNode *node) {
  xmlNode *child = node->children;
  while(child)
  {
    if(child->type == XML_ELEMENT_NODE)
      return false;
    child = child->next;
  }
  return true;
}

void parseCodecConfigs(xmlNode *input_node, int context) {
  stack<xmlNode*> profile_node_stack;
  unsigned int TempCodecCount = 0;
  unsigned int TempFieldsCount = 0;
  xmlNode *FirstChild = xmlFirstElementChild(input_node);
  unsigned long CodecFields = xmlChildElementCount(FirstChild);
  codec_config temp_codec_config;
  memset(&temp_codec_config, 0, sizeof(codec_config));


  BTIF_TRACE_IMP("codec Fields count is %ld \n", CodecFields);
  for (xmlNode *node = input_node->children;
                node != NULL || !profile_node_stack.empty(); node = node->children) {
    if (node == NULL) {
      node = profile_node_stack.top();
      profile_node_stack.pop();
    }

    if(node) {
      if(node->type == XML_ELEMENT_NODE) {
        if((is_leaf(node))) {
          string content = (const char*)(xmlNodeGetContent(node));
          if (content[0] == '\0') {
              return;
          }

          if(!xmlStrcmp(node->name,(const xmlChar*)"SamplingFrequencyInHz")) {
            temp_codec_config.freq_in_hz = atoi(content.c_str());
            TempFieldsCount++;
          } else if(!xmlStrcmp(node->name, (const xmlChar*)"FrameDurationInMicroSecs")) {
            temp_codec_config.frame_dur_msecs = (float)atoi(content.c_str())/1000;
            TempFieldsCount++;
          } else if(!xmlStrcmp(node->name, (const xmlChar*)"OctetsPerCodecFrame")) {
            temp_codec_config.oct_per_codec_frm = atoi(content.c_str());
            TempFieldsCount++;
          } else if(!xmlStrcmp(node->name, (const xmlChar*)"Mandatory")) {
            temp_codec_config.mandatory = atoi(content.c_str());
            TempFieldsCount++;
          }
          else if(!xmlStrcmp(node->name, (const xmlChar*)"CodecVersion"))
          {
            temp_codec_config.codec_version = atoi(content.c_str());
            TempFieldsCount++;
          }
        }

        if(TempFieldsCount == CodecFields) {
          if (current_profile == TMAP) {
            if (context == VOICE_CONTEXT) {
              vmcp_voice_codec[current_codec].push_back(temp_codec_config);
            } else if (context == MEDIA_CONTEXT) {
              vmcp_media_codec[current_codec].push_back(temp_codec_config);
            }
          } else if (current_profile == BAP) {
            if (context == VOICE_CONTEXT) {
              bap_voice_codec[current_codec].push_back(temp_codec_config);
            } else if (context == MEDIA_CONTEXT) {
              bap_media_codec[current_codec].push_back(temp_codec_config);
            }
          } else if (current_profile == GCP) {
            if (context == VOICE_CONTEXT) {
              gcp_voice_codec[current_codec].push_back(temp_codec_config);
            } else if (context == MEDIA_CONTEXT) {
              gcp_media_codec[current_codec].push_back(temp_codec_config);
            }
          } else if (current_profile == WMCP) {
            if (context == MEDIA_CONTEXT) {
              BTIF_TRACE_IMP("%s: parsed codec config for wmcp", __func__);
              wmcp_media_codec[current_codec].push_back(temp_codec_config);
            }
          } else if (current_profile == GCP_RX) {
            if (context == VOICE_CONTEXT) {
              gcp_rx_voice_codec[current_codec].push_back(temp_codec_config);
            } else if (context == MEDIA_CONTEXT) {
              gcp_rx_media_codec[current_codec].push_back(temp_codec_config);
            }
          } else if (current_profile == GCP_TX) {
            if (context == VOICE_CONTEXT) {
              gcp_tx_voice_codec[current_codec].push_back(temp_codec_config);
            } else if (context == MEDIA_CONTEXT) {
              gcp_tx_media_codec[current_codec].push_back(temp_codec_config);
            }
          }

          TempFieldsCount = 0;
          TempCodecCount++;
        }
      }

      if(node->next != NULL)
      {
        profile_node_stack.push(node->next);
        node = node->next;
      }
    } // end of if (node)
  } // end of for

  if(context == VOICE_CONTEXT && TempCodecCount == voice_codec_count) {
    if (current_profile < GCP) {
      BTIF_TRACE_IMP("All %ld CG codecs are parsed successfully\n",
                                            voice_codec_count);
    } else {
      BTIF_TRACE_IMP("All %ld GAT Rx codecs are parsed successfully\n",
                                            voice_codec_count);
    }
  } else if(context == MEDIA_CONTEXT && TempCodecCount == media_codec_count) {
    if (current_profile < GCP) {
      BTIF_TRACE_IMP("All %ld UMS codecs are parsed successfully\n",
                                            media_codec_count);
    } else if (current_profile == GCP) {
      BTIF_TRACE_IMP("All %ld GAT Tx codecs are parsed successfully\n",
                                            media_codec_count);
    } else if (current_profile == WMCP) {
      BTIF_TRACE_IMP("All %ld WM Rx codecs are parsed successfully\n",
                                            media_codec_count);
    } else if (current_profile == GCP_RX) {
      BTIF_TRACE_IMP("All %ld GCP Rx codecs are parsed successfully\n",
                                            media_codec_count);
    } else if (current_profile == GCP_TX) {
      BTIF_TRACE_IMP("All %ld GCP Tx codecs are parsed successfully\n",
                                            media_codec_count);
    }
  }
}

void parseQoSConfigs(xmlNode *QoSInputNode, int context) {
  stack<xmlNode*> QoS_Stack;
  unsigned int TempQoSCodecCount = 0;
  unsigned int TempQoSFieldsCount = 0;
  xmlNode * FirstChild = xmlFirstElementChild(QoSInputNode);
  unsigned long QoSCodecFields = xmlChildElementCount(FirstChild);
  qos_config temp_qos_config ;
  memset(&temp_qos_config, 0, sizeof(qos_config));

  BTIF_TRACE_IMP("QoS Fields count %ld \n", QoSCodecFields);
  for (xmlNode *node = QoSInputNode->children;
                node != NULL || !QoS_Stack.empty(); node = node->children) {
    if (node == NULL) {
      node = QoS_Stack.top();
      QoS_Stack.pop();
    }

    if (node) {
      if (node->type == XML_ELEMENT_NODE) {
        if (is_leaf(node)) {
          string content = (const char*)(xmlNodeGetContent(node));
          if (content[0] == '\0') {
              return;
          }

          if(!xmlStrcmp(node->name,(const xmlChar*)"SamplingFrequencyInHz")) {
            temp_qos_config.freq_in_hz = atoi(content.c_str());
            TempQoSFieldsCount++;
          } else if(!xmlStrcmp(node->name, (const xmlChar*)"SDUIntervalInMicroSecs")) {
            temp_qos_config.sdu_int_micro_secs = atoi(content.c_str());
            TempQoSFieldsCount++;
          } else if(!xmlStrcmp(node->name, (const xmlChar*)"Framing")) {
            temp_qos_config.framing = atoi(content.c_str());
            TempQoSFieldsCount++;
          } else if(!xmlStrcmp(node->name, (const xmlChar*)"MaxSDUSize")) {
            temp_qos_config.max_sdu_size = atoi(content.c_str());
            TempQoSFieldsCount++;
          } else if(!xmlStrcmp(node->name, (const xmlChar*)"RetransmissionNumber")) {
            temp_qos_config.retrans_num = atoi(content.c_str());
            TempQoSFieldsCount++;
          } else if(!xmlStrcmp(node->name, (const xmlChar*)"MaxTransportLatency")) {
            temp_qos_config.max_trans_lat = atoi(content.c_str());
            TempQoSFieldsCount++;
          } else if(!xmlStrcmp(node->name, (const xmlChar*)"PresentationDelay")) {
            temp_qos_config.presentation_delay = atoi(content.c_str());
            TempQoSFieldsCount++;
          } else if(!xmlStrcmp(node->name, (const xmlChar*)"Mandatory")) {
            temp_qos_config.mandatory = atoi(content.c_str());
            TempQoSFieldsCount++;
          } else if(!xmlStrcmp(node->name, (const xmlChar*)"CodecVersion")) {
            temp_qos_config.codec_version = atoi(content.c_str());
            TempQoSFieldsCount++;
          }
        }

        if(TempQoSFieldsCount == QoSCodecFields) {
          if(current_profile == TMAP) {
            if (context == VOICE_CONTEXT) {
              vmcp_qos_low_lat_voice[current_codec].push_back(temp_qos_config);
            } else if (context == MEDIA_LL_CONTEXT) {
              vmcp_qos_low_lat_media[current_codec].push_back(temp_qos_config);
            } else if (context == MEDIA_HR_CONTEXT) {
              vmcp_qos_high_rel_media[current_codec].push_back(temp_qos_config);
            }
          } else if(current_profile == BAP) {
            if (context == VOICE_CONTEXT) {
              bap_qos_low_lat_voice[current_codec].push_back(temp_qos_config);
            } else if (context == MEDIA_LL_CONTEXT) {
              bap_qos_low_lat_media[current_codec].push_back(temp_qos_config);
            } else if (context == MEDIA_HR_CONTEXT) {
              bap_qos_high_rel_media[current_codec].push_back(temp_qos_config);
            }
          } else if(current_profile == GCP) {
            if (context == VOICE_CONTEXT) {
              gcp_qos_low_lat_voice[current_codec].push_back(temp_qos_config);
            } else if (context == MEDIA_LL_CONTEXT) {
              gcp_qos_low_lat_media[current_codec].push_back(temp_qos_config);
            }
          } else if(current_profile == WMCP) {
            if (context == MEDIA_HR_CONTEXT) {
              BTIF_TRACE_IMP("%s: parsed qos config for wmcp", __func__);
              wmcp_qos_high_rel_media[current_codec].push_back(temp_qos_config);
            }
          } else if(current_profile == GCP_RX) {
            if (context == VOICE_CONTEXT) {
              gcp_rx_qos_low_lat_voice[current_codec].push_back(temp_qos_config);
            } else if (context == MEDIA_LL_CONTEXT) {
              gcp_rx_qos_low_lat_media[current_codec].push_back(temp_qos_config);
            }
          } else if(current_profile == GCP_TX) {
            if (context == VOICE_CONTEXT) {
              gcp_tx_qos_low_lat_voice[current_codec].push_back(temp_qos_config);
            } else if (context == MEDIA_LL_CONTEXT) {
              gcp_tx_qos_low_lat_media[current_codec].push_back(temp_qos_config);
            }
          }

          TempQoSFieldsCount = 0;
          TempQoSCodecCount++;
        }
      }
      if(node->next != NULL) {
        QoS_Stack.push(node->next);
        node = node->next;
      }

    }
  }

  if(TempQoSCodecCount == qos_settings_count) {
    if(context == VOICE_CONTEXT) {
      if (current_profile < GCP) {
        BTIF_TRACE_IMP("All %ld CG Qos Config are parsed successfully\n",
                                                      qos_settings_count);
      } else {
        BTIF_TRACE_IMP("All %ld GAT Rx Qos Config are parsed successfully\n",
                                                      qos_settings_count);
      }
    } else if(context == MEDIA_CONTEXT) {
      if (current_profile < GCP) {
        BTIF_TRACE_IMP("All %ld UMS Qos Config are parsed successfully\n",
                                                      qos_settings_count);
      } else if (current_profile == GCP) {
        BTIF_TRACE_IMP("All %ld GAT Tx Qos Config are parsed successfully\n",
                                                      qos_settings_count);
      } else if (current_profile == WMCP) {
        BTIF_TRACE_IMP("All %ld WM Rx Qos Config are parsed successfully\n",
                                                      qos_settings_count);
      } else if (current_profile == GCP_RX) {
        BTIF_TRACE_IMP("All %ld GCP Rx Qos Config are parsed successfully\n",
                                                      qos_settings_count);
      } else if (current_profile == GCP_TX) {
        BTIF_TRACE_IMP("All %ld GCP Tx Qos Config are parsed successfully\n",
                                                      qos_settings_count);
      }
    }
  }
}

void parse_xml(xmlNode *inputNode) {
   stack<xmlNode*> S;
   for (xmlNode *node = inputNode;
                 node != NULL || !S.empty(); node = node->children) {
    if (node == NULL) {
       node = S.top();
       S.pop();
    }

    if (node) {
      if (node->type == XML_ELEMENT_NODE) {
        if (!(is_leaf(node))) {
          string content = (const char *) (xmlNodeGetContent (node));
          if (content[0] == '\0') {
              return;
          }

          if (!xmlStrcmp (node->name, (const xmlChar *) "TMAP")) {
             BTIF_TRACE_IMP("TMAP configs being parsed\n");
             current_profile = TMAP;
          }

          if (!xmlStrcmp (node->name, (const xmlChar *) "BAP")) {
             BTIF_TRACE_IMP("BAP configs being parsed\n");
             current_profile = BAP;
          }

          if (!xmlStrcmp (node->name, (const xmlChar *) "GCP")) {
             BTIF_TRACE_IMP("GCP configs being parsed\n");
             current_profile = GCP;
          }

          if (!xmlStrcmp (node->name, (const xmlChar *) "WMCP")) {
             BTIF_TRACE_IMP("WMCP configs being parsed\n");
             current_profile = WMCP;
          }

          if (!xmlStrcmp (node->name, (const xmlChar *) "GCP_RX")) {
             BTIF_TRACE_IMP("GCP_RX configs being parsed\n");
             current_profile = GCP_RX;
          }

          if (!xmlStrcmp (node->name, (const xmlChar *) "GCP_TX")) {
             BTIF_TRACE_IMP("GCP_TX configs being parsed\n");
             current_profile = GCP_TX;
          }

          if(!xmlStrcmp (node->name, (const xmlChar *) "LC3Q")) {
             BTIF_TRACE_IMP("AAR3 ->LC3Q configs being parsed\n");
             //current_codec = LC3Q;
             current_codec = CodecIndex::CODEC_INDEX_SOURCE_LC3;
             BTIF_TRACE_IMP("AAR3 -> current_codec = LC3Q");
          }

          if(!xmlStrcmp (node->name, (const xmlChar *) "AAR3")) {
             BTIF_TRACE_IMP("AAR3 -> AAR3 configs being parsed\n");
             current_codec = CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE;
             BTIF_TRACE_IMP("AAR3 -> current_codec = AAR3");
          }

          if (!xmlStrcmp (node->name, (const xmlChar *) "CodecCapabilitiesForVoice")) {
            voice_codec_count = xmlChildElementCount(node);
            if(current_codec != CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE) {
              parseCodecConfigs(node, VOICE_CONTEXT);
            } else{
              //Do nothing
            }
          } else if (!xmlStrcmp (node->name, (const xmlChar *) "CodecCapabilitiesForMedia")) {
            media_codec_count = xmlChildElementCount(node);
            parseCodecConfigs(node, MEDIA_CONTEXT);
          } else if (!xmlStrcmp (node->name, (const xmlChar *) "QosSettingsForLowLatencyVoice")) {
            qos_settings_count = xmlChildElementCount(node);
            if (current_codec != CodecIndex::CODEC_INDEX_SOURCE_APTX_ADAPTIVE_LE) {
              parseQoSConfigs(node, VOICE_CONTEXT);
            } else {
              //Do nothing
            }
          } else if (!xmlStrcmp (node->name, (const xmlChar *) "QosSettingsForLowLatencyMedia")) {
            qos_settings_count = xmlChildElementCount(node);
            parseQoSConfigs(node, MEDIA_LL_CONTEXT);
          } else if (!xmlStrcmp (node->name, (const xmlChar *) "QosSettingsForHighReliabilityMedia")) {
            qos_settings_count = xmlChildElementCount(node);
            parseQoSConfigs(node, MEDIA_HR_CONTEXT);
          }
        }
      }

      if(node->next != NULL) {
        S.push(node -> next);
      }
    }
   }
}

void print_codec_util(std::unordered_map<CodecIndex, std::vector<codec_config>>temp) {
  /*for (auto x : temp) {
    for(unsigned long i = 0; i < x.second.size(); i++) {
        BTIF_TRACE_IMP("x.second[i].codec_version = %f, freq_in_hz = %ld,"
                       " frame_dur_msecs = %f, oct_per_codec_frm = %ld,"
                       " mandatory = %ld, codec_version = %f \n",
                       x.second[i].codec_version, x.second[i].freq_in_hz,
                       x.second[i].frame_dur_msecs, x.second[i].oct_per_codec_frm,
                       x.second[i].mandatory,x.second[i].codec_version);
    }
  }*/
}

void print_qos_util(std::unordered_map<CodecIndex, std::vector<qos_config>>temp) {
  /*for (auto x : temp) {
    BTIF_TRACE_IMP("LC3Q/AAR3 = %ld \n", x.first);
    for(unsigned long i = 0; i < x.second.size(); i++) {
      BTIF_TRACE_IMP("AAR3 -> print_qos_util() x.second[i].x.second[i].codec_version = %f,"
                     " freq_in_hz = %ld , %ld , %ld, %ld \n", x.second[i].codec_version,
                     x.second[i].freq_in_hz, x.second[i].sdu_int_micro_secs,
                     x.second[i].framing,x.second[i].max_sdu_size);
      BTIF_TRACE_IMP("AAR3 -> print_qos_util() x.second[i].retrans_num = %ld,"
                     " %ld , %ld, %ld, %f \n", x.second[i].retrans_num,
                     x.second[i].max_trans_lat, x.second[i].presentation_delay,
                     x.second[i].mandatory,x.second[i].codec_version);
    }
  }*/
}
void print_parsed_leaudio_configs() {
    BTIF_TRACE_IMP("Printing vmcp_voice_codec \n");
    print_codec_util(vmcp_voice_codec);
    BTIF_TRACE_IMP("Printing vmcp_media_codec \n");
    print_codec_util(vmcp_media_codec);
    BTIF_TRACE_IMP("Printing vmcp_qos_low_lat_voice \n");
    print_qos_util(vmcp_qos_low_lat_voice);
    BTIF_TRACE_IMP("Printing vmcp_qos_low_lat_media \n");
    print_qos_util(vmcp_qos_low_lat_media);
    BTIF_TRACE_IMP("Printing vmcp_qos_high_rel_media \n");
    print_qos_util(vmcp_qos_high_rel_media);

    BTIF_TRACE_IMP("Printing bap_voice_codec \n");
    print_codec_util(bap_voice_codec);
    BTIF_TRACE_IMP("Printing bap_media_codec \n");
    print_codec_util(bap_media_codec);
    BTIF_TRACE_IMP("Printing bap_qos_low_lat_voice \n");
    print_qos_util(bap_qos_low_lat_voice);
    BTIF_TRACE_IMP("Printing bap_qos_low_lat_media \n");
    print_qos_util(bap_qos_low_lat_media);
    BTIF_TRACE_IMP("Printing bap_qos_high_rel_media \n");
    print_qos_util(bap_qos_high_rel_media);

    BTIF_TRACE_IMP("Printing gcp_voice_codec \n");
    print_codec_util(gcp_voice_codec);
    BTIF_TRACE_IMP("Printing gcp_media_codec \n");
    print_codec_util(gcp_media_codec);
    BTIF_TRACE_IMP("Printing gcp_qos_low_lat_voice \n");
    print_qos_util(gcp_qos_low_lat_voice);
    BTIF_TRACE_IMP("Printing gcp_qos_low_lat_media \n");
    print_qos_util(gcp_qos_low_lat_media);

    BTIF_TRACE_IMP("Printing wmcp_media_codec \n");
    print_codec_util(wmcp_media_codec);
    BTIF_TRACE_IMP("Printing wmcp_qos_high_rel_media \n");
    print_qos_util(wmcp_qos_high_rel_media);

    BTIF_TRACE_IMP("Printing gcp_rx_voice_codec \n");
    print_codec_util(gcp_rx_voice_codec);
    BTIF_TRACE_IMP("Printing gcp_rx_media_codec \n");
    print_codec_util(gcp_rx_media_codec);
    BTIF_TRACE_IMP("Printing gcp_rx_qos_low_lat_voice \n");
    print_qos_util(gcp_rx_qos_low_lat_voice);
    BTIF_TRACE_IMP("Printing gcp_rx_qos_low_lat_media \n");
    print_qos_util(gcp_rx_qos_low_lat_media);

    BTIF_TRACE_IMP("Printing gcp_tx_voice_codec \n");
    print_codec_util(gcp_tx_voice_codec);
    BTIF_TRACE_IMP("Printing gcp_tx_media_codec \n");
    print_codec_util(gcp_tx_media_codec);
    BTIF_TRACE_IMP("Printing gcp_tx_qos_low_lat_voice \n");
    print_qos_util(gcp_tx_qos_low_lat_voice);
    BTIF_TRACE_IMP("Printing gcp_tx_qos_low_lat_media \n");
    print_qos_util(gcp_tx_qos_low_lat_media);
}

void btif_vmcp_init() {
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;

  doc = xmlReadFile(LEAUDIO_CONFIG_PATH, NULL, 0);
  if (doc == NULL) {
    BTIF_TRACE_ERROR("Could not parse the XML file");
  }

  root_element = xmlDocGetRootElement(doc);
  parse_xml(root_element);
  print_parsed_leaudio_configs();
  xmlFreeDoc(doc);
  xmlCleanupParser();

  //Register Audio Gaming Service UUID (GCP) with Gattc
  btif_register_uuid_srvc_disc(bluetooth::Uuid::FromString("12994b7e-6d47-4215-8c9e-aae9a1095ba3"));

  //Register Wireless Microphone Configuration Service UUID (WMCP) with Gattc
  btif_register_uuid_srvc_disc(bluetooth::Uuid::FromString("2587db3c-ce70-4fc9-935f-777ab4188fd7"));

  //Register TMAP UUID (WMCP) with Gattc
  btif_register_uuid_srvc_disc(bluetooth::Uuid::FromString("00001855-0000-1000-8000-00805F9B34FB"));

  do_in_bta_thread(FROM_HERE,  Bind(&TmapServer::Initialize, tmas_uuid));
}

void btif_vmcp_cleanup() {
  BTIF_TRACE_IMP(": TMAP CleanUp is called");
  do_in_bta_thread(FROM_HERE,  Bind(&TmapServer::CleanUp));
}
