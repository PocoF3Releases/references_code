/*
*
* Copyright 2019-2020 NXP.
*
* NXP Confidential. This software is owned or controlled by NXP and may only be
* used strictly in accordance with the applicable license terms. By expressly
* accepting such terms or by downloading,installing, activating and/or otherwise
* using the software, you are agreeing that you have read,and that you agree to
* comply with and are bound by, such license terms. If you do not agree to be
* bound by the applicable license terms, then you may not retain, install, activate
* or otherwise use the software.
*
*/
#define LOG_TAG "vendor.xiaomi.uwb@1.0-service"

#include <vendor/xiaomi/uwb/1.0/IUwb.h>

#include <hidl/LegacySupport.h>

#include "Uwb.h"

using vendor::xiaomi::uwb::V1_0::IUwb;
using vendor::xiaomi::uwb::implementation::Uwb;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;

int main() {
      int res;
      android::sp<IUwb> uwb_service = new Uwb();
      ALOGE("simp main");
      configureRpcThreadpool(1, true /*callerWillJoin*/);

      if (uwb_service != nullptr) {
          res = uwb_service->registerAsService();
          if(res != 0)
            ALOGE("Can't register instance of uwb Hardware, nullptr");
      } else {
          ALOGE("Can't create instance of Uwb Hardware, nullptr");
       }

      joinRpcThreadpool();

      return 0; // should never get here
}
