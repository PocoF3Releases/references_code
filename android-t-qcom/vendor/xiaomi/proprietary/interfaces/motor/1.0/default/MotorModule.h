/**************************************************************************************
 * Copyright (c) 2019-2020 XiaoMi Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - XiaoMi Technologies, Inc.
 *
 * Description:
 *   MotorModule.h
 *
 * Date: 22-02-2019 (zhunengjin@xiaomi.com)
 **************************************************************************************/

#ifndef MOTORMODULE_H
#define MOTORMODULE_H

#include <utils/RefBase.h>
#include <utils/Log.h>
#include <dlfcn.h>
#include <hardware/motor_common.h>
#include <vendor/xiaomi/hardware/motor/1.0/IMotorCallback.h>
namespace android {
namespace vendor {
namespace xiaomi {
namespace hardware {
namespace motor {
namespace V1_0 {
namespace helper {

using ::android::sp;
using ::vendor::xiaomi::hardware::motor::V1_0::MotorEvent;
using ::vendor::xiaomi::hardware::motor::V1_0::IMotorCallback;

#define BIN_LIB_PATH "/vendor/lib64/mi.motor.daemon.so"

class MotorModule : public RefBase {

public:
   MotorModule();
   ~MotorModule();

   void initialize();
   void release();
   void pop(int32_t cookie);
   void retract(int32_t cookie);
   void setCallback(sp<IMotorCallback> callback);
   uint32_t getMotorStatus();
   void calibration();
   void falldown();
   sp<IMotorCallback> mMotorCallback;

private:
   void motorBinLoad();
   static void deviceNotify(void *callback, motor_notify_type_t notice);

   void *mothodHadle;
   motor_mothods_t *hwMothods;

   motor_module_callback_t deviceCallback;

   int32_t cookies_id;

}; /* MotorModule */


} /* namespace helper */
} /* namespace V1_0 */
} /* namespace motor */
} /* namespace hardware */
} /* namespace xiaomi */
} /* namespace vendor */
}

#endif /* MOTORMODULE_H */
