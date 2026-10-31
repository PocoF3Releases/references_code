/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
 *  Copyright 2018-2019 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#pragma once
#include <pthread.h>
#include "config.h"
#include "uwb_target.h"
#include "uwb_hal_api.h"
#include "uwb_hal_int.h"
#include "hal_nxpuwb.h"
#include <utils/RefBase.h>

namespace vendor {
namespace xiaomi {
namespace uwb {
namespace V1_0 {
struct IUwb;
struct IUwbClientCallback;
}
}
}
}

class ThreadMutex {
 public:
  ThreadMutex();
  virtual ~ThreadMutex();
  void lock();
  void unlock();
  operator pthread_mutex_t*() { return &mMutex; }

 private:
  pthread_mutex_t mMutex;
};

class ThreadCondVar : public ThreadMutex {
 public:
  ThreadCondVar();
  virtual ~ThreadCondVar();
  void signal();
  void wait();
  operator pthread_cond_t*() { return &mCondVar; }
  operator pthread_mutex_t*() {
    return ThreadMutex::operator pthread_mutex_t*();
  }

 private:
  pthread_cond_t mCondVar;
};

class AutoThreadMutex {
 public:
  AutoThreadMutex(ThreadMutex& m);
  virtual ~AutoThreadMutex();
  operator ThreadMutex&() { return mm; }
  operator pthread_mutex_t*() { return (pthread_mutex_t*)mm; }

 private:
  ThreadMutex& mm;
};

class UwbAdaptation {
 public:
  virtual ~UwbAdaptation();
  void Initialize();
  void Finalize(bool graceExit);
  static UwbAdaptation& GetInstance();
  tHAL_UWB_ENTRY* GetHalEntryFuncs();
  tUWB_STATUS DumpFwDebugLog(bool enableFwDump,bool enableCirDump);
  void GetVendorConfigs(std::map<std::string, ConfigValue>& configMap);
  uwb_uci_IoctlInOutData_t* mCurrentIoctlData=NULL;
  static tUWB_STATUS CoreInitialization(bool recovery);

 private:
  UwbAdaptation();
  void signal();
  static UwbAdaptation* mpInstance;
  static ThreadMutex sLock;
  static ThreadMutex sIoctlLock;
  ThreadCondVar mCondVar;
  tHAL_UWB_ENTRY mHalEntryFuncs;  // function pointers for HAL entry points
  static android::sp<vendor::xiaomi::uwb::V1_0::IUwb> mHal; //vendor : sp
  static vendor::xiaomi::uwb::V1_0::IUwbClientCallback* mCallback;

  static tHAL_UWB_CBACK* mHalCallback;
  static tHAL_UWB_DATA_CBACK* mHalDataCallback;
  static ThreadCondVar mHalOpenCompletedEvent;
  static ThreadCondVar mHalCloseCompletedEvent;
  static ThreadCondVar mHalCoreResetCompletedEvent;
  static ThreadCondVar mHalCoreInitCompletedEvent;
  static ThreadCondVar mHalInitCompletedEvent;

  static uint32_t UWBA_TASK(uint32_t arg);
  static uint32_t Thread(uint32_t arg);
  void InitializeHalDeviceContext();
  static void HalDeviceContextCallback(uwb_event_t event,
                                       uwb_status_t event_status);
  static void HalDeviceContextDataCallback(uint16_t data_len, uint8_t* p_data);

  static void HalInitialize();
  static void HalTerminate();
  static void HalOpen(tHAL_UWB_CBACK* p_hal_cback,
                      tHAL_UWB_DATA_CBACK* p_data_cback);
  static void HalClose();
  static void HalWrite(uint16_t data_len, uint8_t* p_data);
  static tUWB_STATUS HalIoctl(long arg, void* p_data);

  static void HalDownloadFirmwareCallback(uwb_event_t event,
                                        uwb_status_t event_status);
  static void HalDownloadFirmwareDataCallback(uint16_t data_len,
                                              uint8_t* p_data);
};
