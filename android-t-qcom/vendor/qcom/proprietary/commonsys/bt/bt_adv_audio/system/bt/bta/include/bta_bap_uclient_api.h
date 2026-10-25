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
#include "connected_iso_api.h"
#include <hardware/bt_bap_uclient.h>
#include "bta_ascs_client_api.h"
#include "bta/bap/uclient_alarm.h"

namespace bluetooth {
namespace bap {
namespace ucast {

using bluetooth::bap::pacs::PacsClientCallbacks;
using bluetooth::bap::ascs::AscsClientCallbacks;
using bluetooth::bap::cis::CisInterfaceCallbacks;
using bluetooth::bap::ucast::StreamConnect;
using bluetooth::bap::ucast::StreamType;
using bluetooth::bap::alarm::BapAlarmCallbacks;

class UcastClient : public PacsClientCallbacks,
                    public AscsClientCallbacks,
                    public CisInterfaceCallbacks,
                    public BapAlarmCallbacks {
 public:
  virtual ~UcastClient() = default;

  static void Initialize(UcastClientCallbacks* callbacks);
  static void CleanUp();
  static UcastClient* Get();

  // APIs exposed to upper layer
  virtual void Connect(std::vector<RawAddress> address, bool is_direct,
                       std::vector<StreamConnect> streams) = 0;
  virtual void Disconnect(const RawAddress& address,
                       std::vector<StreamType> streams) = 0;
  virtual void Start(std::vector<RawAddress> address,
                     std::vector<StreamType> streams,
                     bool is_mono_mic) = 0;
  virtual void Stop(const RawAddress& address,
                    std::vector<StreamType> streams) = 0;
  virtual void Reconfigure(const RawAddress& address,
                           std::vector<StreamReconfig> streams) = 0;
  virtual void UpdateStream(const RawAddress& address,
                           std::vector<StreamUpdate> update_streams) = 0;
};

}  // namespace ucast
}  // namespace bap
}  // namespace bluetooth
