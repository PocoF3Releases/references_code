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

#include <string>
#include <vector>

#include "stack/include/bt_types.h"
#include <hardware/bt_bap_uclient.h>
#include <hardware/bt_pacs_client.h>
#include "btif_bap_codec_utils.h"

namespace bluetooth {
namespace bap {
namespace cis {

using bluetooth::bap::ucast::CISEstablishedEvtParamas;
using bluetooth::bap::ucast::CISConfig;
using bluetooth::bap::ucast::CIGConfig;
using bluetooth::bap::pacs::CodecConfig;

constexpr uint8_t  DIR_TO_AIR    = 0x1 << 0;
constexpr uint8_t  DIR_FROM_AIR  = 0x1 << 1;

typedef uint8_t sdu_interval_t[3];

enum class CisState {
  INVALID = 0,
  READY,
  DESTROYING,
  ESTABLISHING,
  ESTABLISHED
};

enum class CigState {
  INVALID = 0,
  IDLE,
  CREATING,
  CREATED,
  REMOVING
};

enum IsoHciStatus {
  ISO_HCI_SUCCESS = 0,
  ISO_HCI_FAILED,
  ISO_HCI_IN_PROGRESS
};

class CisInterfaceCallbacks {
 public:
  virtual ~CisInterfaceCallbacks() = default;

  /** Callback for connection state change */
  virtual void OnCigState(uint8_t cig_id, CigState state) = 0;

  virtual void OnCisState(uint8_t cig_id, uint8_t cis_id, uint8_t direction,
                          CISEstablishedEvtParamas cis_param, CisState state) = 0;
};

class CisInterface {
 public:
  virtual ~CisInterface() = default;

  static void Initialize(CisInterfaceCallbacks* callbacks);
  static void CleanUp();
  static CisInterface* Get();

  virtual CigState GetCigState(const uint8_t &cig_id);

  virtual CisState GetCisState(const uint8_t &cig_id, uint8_t cis_id);

  virtual uint8_t GetCisCount(const uint8_t &cig_id) = 0;

  virtual IsoHciStatus CreateCig(RawAddress client_peer_bda,
                         bool reconfig,
                         CIGConfig &cig_config,
                         std::vector<CISConfig> &cis_configs,
                         CodecConfig *codec_config,
                         uint16_t audio_context,
                         uint8_t stream_direction) = 0;

  virtual IsoHciStatus RemoveCig(RawAddress peer_bda,
                                 uint8_t cig_id) = 0;

  virtual IsoHciStatus CreateCis(uint8_t cig_id, std::vector<uint8_t> cis_ids,
                                 std::vector<RawAddress> peer_bda) = 0;

  virtual IsoHciStatus DisconnectCis(uint8_t cig_id, uint8_t cis_id,
                                     uint8_t direction) = 0;

  virtual IsoHciStatus SetupDataPath(uint8_t cig_id, uint8_t cis_id,
                                     uint8_t direction,
                                     uint8_t path_id) = 0;

  virtual IsoHciStatus RemoveDataPath(uint8_t cig_id, uint8_t cis_id,
                              uint8_t direction) = 0;

  virtual IsoHciStatus UpdateEncoderParams(uint8_t cig_id, uint8_t cis_id,
                                           std::vector<uint8_t> encoder_limit_params,
                                           uint8_t encoder_mode) = 0;

};

}  // namespace ucast
}  // namespace bap
}  // namespace bluetooth
