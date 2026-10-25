/**************************************************************************************
 * Copyright (c) 2019-2020 XiaoMi Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - XiaoMi Technologies, Inc.
 *
 * Description:
 *   MotorModule.cpp
 *
 * Date: 22-02-2019 (zhunengjin@xiaomi.com)
 **************************************************************************************/

#include "MotorModule.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "MotorModule"

namespace android {
namespace vendor {
namespace xiaomi {
namespace hardware {
namespace motor {
namespace V1_0 {
namespace helper {


MotorModule::MotorModule():deviceCallback({deviceNotify})
{
   motorBinLoad();
   if(hwMothods != nullptr)
   {
      hwMothods->initialize();
      hwMothods->setCallback(this, &deviceCallback);
   }
}

MotorModule::~MotorModule()
{
   if(hwMothods != nullptr)
   {
      hwMothods->release();
      hwMothods = NULL;
   }
}

void MotorModule::initialize()
{
   if(hwMothods != nullptr)
   {
      hwMothods->enable();
   }
}


void MotorModule::release()
{
   if(hwMothods != nullptr)
   {
      hwMothods->disable();
   }
}


void MotorModule::pop(int32_t cookie)
{
   if(hwMothods != nullptr)
   {
      cookies_id =cookie;
      hwMothods->popUp();
   }
}


void MotorModule::retract(int32_t cookie)
{
   if(hwMothods != nullptr)
   {
      cookies_id =cookie;
      hwMothods->retract();
   }
}

void MotorModule::calibration()
{
    if(hwMothods != nullptr)
    {
        hwMothods->calibration();
    }

}

uint32_t MotorModule::getMotorStatus()
{
    if(hwMothods != nullptr)
     {
      return (uint32_t)hwMothods->getStatus();
     }

     return 0;

}

void MotorModule::falldown()
{
    if(hwMothods != nullptr)
    {
      hwMothods->falldown();
    }

}


void MotorModule::setCallback(const sp<IMotorCallback> callback)
{
   if (callback != nullptr)
   {
      mMotorCallback = callback;
   }
}

void MotorModule::motorBinLoad(void)
{
    void *(*module_lib_open)(void) = NULL;

    mothodHadle = dlopen(BIN_LIB_PATH, RTLD_NOW);
    if (mothodHadle == nullptr) {
        ALOGE("Load %s module library failed, error = %s\n", BIN_LIB_PATH, dlerror());
    } else {
        *(void **)&module_lib_open = dlsym(mothodHadle, "hardware_module_open");
        if (!module_lib_open) {
            ALOGE("Open %s module library fail, error = %s\n", BIN_LIB_PATH, dlerror());
        } else {
            hwMothods = (motor_mothods_t *)module_lib_open();
            if (hwMothods == nullptr) {
                ALOGE("Get %s sub module function fail, error = %s, %p\n", BIN_LIB_PATH, dlerror(), hwMothods);
            } else {
                ALOGE("Get %s sub module function success, func_ptr = %p\n", BIN_LIB_PATH, hwMothods);
            }
        }
    }
}

void MotorModule::deviceNotify(void *callback, motor_notify_type_t notice)
{
   struct MotorEvent event;
   MotorModule *cb = const_cast<MotorModule *>(static_cast<const MotorModule *>(callback));

   if (cb == nullptr)
   {
      ALOGE("Invalid callback");
      return;
   }

   if((NOTIFY_INVALID_EVENT == notice) || (NOTIFY_MAX_EVENT == notice))
   {
      ALOGE("Motor notify error, notice = %d", notice);
   }

   ALOGE("notice = %d", notice);

#if 0
   switch(notice){
      case NOTIFY_INVALID_EVENT:
      {
         ALOGE("notice = NOTIFY_INVALID_EVENT");
         break;
      }
      case NOTIFY_TOP_SUCCESS_EVENT:
      {
         ALOGE("notice = NOTIFY_TOP_SUCCESS_EVENT");
         break;
      }
      case NOTIFY_TOP_ERROR_EVENT:
      {
         ALOGE("notice = NOTIFY_TOP_ERROR_EVENT");
         break;
      }
      case NOTIFY_BOTTOM_SUCCESS_EVENT:
      {
         ALOGE("notice = NOTIFY_BOTTOM_SUCCESS_EVENT");
         break;
      }
      case NOTIFY_BOTTOM_ERROR_EVENT:
      {
         ALOGE("notice = NOTIFY_BOTTOM_ERROR_EVENT");
         break;
      }
      case NOTIFY_PRESS_DOWN_EVENT:
      {
         ALOGE("notice = NOTIFY_PRESS_DOWN_EVENT");
         break;
      }
      case NOTIFY_DROP_EVENT:
      {
         ALOGE("notice = NOTIFY_DROP_EVENT");
         break;
      }
      case NOTIFY_CALI_SUCCESS_EVENT:
      {
         ALOGE("notice = NOTIFY_CALI_SUCCESS_EVENT");
         break;
      }
      case NOTIFY_CALI_ERROR_EVENT:
      {
         ALOGE("notice = NOTIFY_CALI_ERROR_EVENT");
         break;
      }
   }
#endif

   event.value = (uint32_t)notice;
   event.cookie = cb->cookies_id;
   if(cb->mMotorCallback != nullptr)
   {
      cb->mMotorCallback->onNotify(event);
      cb->cookies_id = -1;
   }
}


} /* namespace helper */
} /* namespace V1_0 */
} /* namespace motor */
} /* namespace hardware */
} /* namespace xiaomi */
} /* namespace vendor */
}

