/*
 * Copyright (c) 2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
*/
/******************************************************************************
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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

#ifndef ANDROID_INCLUDE_BT_MCP_H
#define ANDROID_INCLUDE_BT_MCP_H



#include <hardware/bluetooth.h>
#include <vector>

#define BT_PROFILE_MCP_ID "mcs_server"

namespace bluetooth {
namespace mcp_server {

class McpServerCallbacks {
  public:
  virtual ~McpServerCallbacks() = default;
  virtual void OnConnectionStateChange(int status, const RawAddress& bd_addr) = 0;
  virtual void MediaControlPointChangeReq(uint8_t state,  const RawAddress& bd_addr) = 0;
  virtual void TrackPositionChangeReq(int32_t position) = 0;
  virtual void PlayingOrderChangeReq(uint32_t order) = 0;
};


class McpServerInterface {
  public:
    virtual ~McpServerInterface() = default;
    virtual void Init(McpServerCallbacks* callbacks, Uuid uuid) = 0;
    virtual void MediaState(uint8_t state) = 0;
    virtual void MediaPlayerName(uint8_t *name) = 0;
    virtual void MediaControlPointOpcodeSupported(uint32_t feature) = 0;
    virtual void MediaControlPoint(uint8_t value, uint8_t status) = 0;
    virtual void TrackChanged(bool status) = 0;
    virtual void TrackTitle(uint8_t* title) = 0;
    virtual void TrackPosition(int32_t position) = 0;
    virtual void TrackDuration(int32_t duration) = 0;
    virtual void PlayingOrderSupported(uint16_t order) = 0;
    virtual void PlayingOrder(uint8_t value) = 0;
    virtual void ContentControlId(uint8_t ccid) = 0;
    virtual void SetActiveDevice(const RawAddress& address, int set_id, int profile) = 0;
    virtual void DisconnectMcp(const RawAddress& address) = 0;
    virtual void BondStateChange(const RawAddress& address, int state) = 0;
    virtual void Cleanup(void) = 0;
};

}
}
#endif /* ANDROID_INCLUDE_BT_MCP_H */
