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

#include "uclient_alarm.h"
#include "bt_trace.h"
#define LOG_TAG "uclient_alarm"

namespace bluetooth {
namespace bap {
namespace alarm {

class BapAlarmImpl;
BapAlarmImpl *instance;

static void alarm_handler(void* data);

class BapAlarmImpl : public BapAlarm {
  public:
    BapAlarmImpl(BapAlarmCallbacks* callback):
       callbacks(callback)  { }

    ~BapAlarmImpl() override = default;

    void CleanUp () { }

    alarm_t* Create(const char* name) {
      return alarm_new(name);
    }

    void Delete(alarm_t* alarm) {
      alarm_free(alarm);
    }

    void Start(alarm_t* alarm, period_ms_t interval_ms,
                              void* data) {
      alarm_set_on_mloop(alarm, interval_ms, alarm_handler, data);
    }

    void Stop(alarm_t* alarm) {
      alarm_cancel(alarm);
    }

    bool IsScheduled(const alarm_t* alarm) {
      return alarm_is_scheduled(alarm);
    }

    void Timeout(void* data) {
      if (callbacks)
        callbacks->OnTimeout(data); // Call uclient_main
    }

  private:
    BapAlarmCallbacks *callbacks;
};

void BapAlarm::Initialize(
                   BapAlarmCallbacks* callbacks) {
  if (instance) {
    LOG(ERROR) << "Already initialized!";
  } else {
    instance = new BapAlarmImpl(callbacks);
  }
}

void BapAlarm::CleanUp() {
  BapAlarmImpl* ptr = instance;
  instance = nullptr;
  ptr->CleanUp();
  delete ptr;
}

BapAlarm* BapAlarm::Get() {
  return instance;
}

static void alarm_handler(void* data) {
  if (instance)
    instance->Timeout(data);
}

} //alarm
} //bap
} //bluetooth
