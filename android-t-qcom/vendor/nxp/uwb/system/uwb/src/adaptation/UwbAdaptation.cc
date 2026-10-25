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
#include <vendor/xiaomi/uwb/1.0/IUwb.h>
#include <base/command_line.h>
#include <cutils/properties.h>
#include <hwbinder/ProcessState.h>

#include <pthread.h>
#include "UwbAdaptation.h"

#include "uwa_api.h"
#include "uwb_int.h"
#include "uwb_target.h"
#include "uwb_hal_int.h"
#include "uwb_config.h"
#include "uci_log.h"

using android::OK;
using android::sp;
using android::status_t;

using android::hardware::ProcessState;
using android::hardware::Return;
using android::hardware::Void;
using vendor::xiaomi::uwb::V1_0::IUwb;
using IUwbV1_0 = vendor::xiaomi::uwb::V1_0::IUwb;
using UwbVendorConfigV1_0 = vendor::xiaomi::uwb::V1_0::UwbConfig;
using vendor::xiaomi::uwb::V1_0::IUwbClientCallback;
using android::hardware::hidl_vec;

extern bool uwb_debug_enabled;

bool uwb_debug_enabled = false;
bool IsdebugLogEnabled = false;

extern void phUwb_GKI_shutdown();

UwbAdaptation* UwbAdaptation::mpInstance = NULL;
ThreadMutex UwbAdaptation::sLock;
ThreadMutex UwbAdaptation::sIoctlLock;
android::Mutex sIoctlMutex;

tHAL_UWB_CBACK* UwbAdaptation::mHalCallback = NULL;
tHAL_UWB_DATA_CBACK* UwbAdaptation::mHalDataCallback = NULL;
ThreadCondVar UwbAdaptation::mHalOpenCompletedEvent;
ThreadCondVar UwbAdaptation::mHalCloseCompletedEvent;

ThreadCondVar UwbAdaptation::mHalCoreResetCompletedEvent;
ThreadCondVar UwbAdaptation::mHalCoreInitCompletedEvent;
ThreadCondVar UwbAdaptation::mHalInitCompletedEvent;
sp<IUwb> UwbAdaptation::mHal;
IUwbClientCallback* UwbAdaptation::mCallback;


namespace {
void initializeGlobalDebugEnabledFlag() {
  uwb_debug_enabled = true;

  UCI_TRACE_I("%s: Debug log is enabled =%u", __func__, uwb_debug_enabled);
}
}  // namespace

class UwbClientCallback : public IUwbClientCallback {
 public:
  UwbClientCallback(tHAL_UWB_CBACK* eventCallback,
                    tHAL_UWB_DATA_CBACK dataCallback) {
    mEventCallback = eventCallback;
    mDataCallback = dataCallback;
  };
  virtual ~UwbClientCallback() = default;

  Return<void> sendEvent(
      ::vendor::xiaomi::uwb::V1_0::UwbEvent event,
      ::vendor::xiaomi::uwb::V1_0::UwbStatus event_status) override {
    mEventCallback((uint8_t)event, (uint16_t)event_status);
    return Void();
  };
  Return<void> sendData(
      const ::vendor::xiaomi::uwb::V1_0::UwbData& data) override {
    ::vendor::xiaomi::uwb::V1_0::UwbData copy = data;
    if(copy != NULL){
      mDataCallback(copy.size(), &copy[0]);
    }
    return Void();
  };

 private:
  tHAL_UWB_CBACK* mEventCallback;
  tHAL_UWB_DATA_CBACK* mDataCallback;
};

/*******************************************************************************
**
** Function:    UwbAdaptation::UwbAdaptation()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
UwbAdaptation::UwbAdaptation() {
  memset(&mHalEntryFuncs, 0, sizeof(mHalEntryFuncs));
}

/*******************************************************************************
**
** Function:    UwbAdaptation::~UwbAdaptation()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
UwbAdaptation::~UwbAdaptation() { mpInstance = NULL; }

/*******************************************************************************
**
** Function:    UwbAdaptation::GetInstance()
**
** Description: access class singleton
**
** Returns:     pointer to the singleton object
**
*******************************************************************************/
UwbAdaptation& UwbAdaptation::GetInstance() {
  AutoThreadMutex a(sLock);

  if (!mpInstance) mpInstance = new UwbAdaptation;
  CHECK(mpInstance);
  return *mpInstance;
}

void UwbAdaptation::GetVendorConfigs(std::map<std::string, ConfigValue>& configMap) {
  const char* func = "UwbAdaptation::GetVendorConfigs";
  UNUSED(func);
  UCI_TRACE_I("%s: enter", func);
  UwbVendorConfigV1_0 configValue;
  uint16_t antennaPairSelectionConfig5;
  if (mHal) {
    mHal->getConfig([&configValue](UwbVendorConfigV1_0 config) {configValue = config;});
  }

  if (mHal) {
    configMap.emplace(NAME_ANTENNA_PAIR_SELECTION_CONFIG1,
                        ConfigValue(configValue.antennaPairSelectionConfig1));

    configMap.emplace(NAME_ANTENNA_PAIR_SELECTION_CONFIG2,
                        ConfigValue(configValue.antennaPairSelectionConfig2));

    configMap.emplace(NAME_ANTENNA_PAIR_SELECTION_CONFIG3,
                        ConfigValue(configValue.antennaPairSelectionConfig3));

    configMap.emplace(NAME_ANTENNA_PAIR_SELECTION_CONFIG4,
                        ConfigValue(configValue.antennaPairSelectionConfig4));

    if(configValue.uwbXtalConfig.size() != 0) {
      antennaPairSelectionConfig5 = (configValue.uwbXtalConfig[0] | (uint16_t)configValue.uwbXtalConfig[1] << 8);
      configMap.emplace(NAME_ANTENNA_PAIR_SELECTION_CONFIG5,
                        ConfigValue(antennaPairSelectionConfig5));   // TODO need to change this with configValue.antennaPairSelectionConfig5
    }
  }
}

/*******************************************************************************
**
** Function:    UwbAdaptation::Initialize()
**
** Description: class initializer
**
** Returns:     none
**
*******************************************************************************/
void UwbAdaptation::Initialize() {
  const char* func = "UwbAdaptation::Initialize";
  UNUSED(func);
 // uint8_t mConfig;
  UCI_TRACE_I("%s: enter", func);
  initializeGlobalDebugEnabledFlag();
  phUwb_GKI_init();
  phUwb_GKI_enable();
  phUwb_GKI_create_task((TASKPTR)UWBA_TASK, BTU_TASK, (int8_t*)"UWBA_TASK", 0, 0,
                  (pthread_cond_t*)NULL, NULL);
  {
    AutoThreadMutex guard(mCondVar);
    phUwb_GKI_create_task((TASKPTR)Thread, MMI_TASK, (int8_t*)"UWBA_THREAD", 0, 0,
                    (pthread_cond_t*)NULL, NULL);
    mCondVar.wait();
  }

 // mConfig =UwbConfig::getUnsigned(NAME_UWB_FW_LOG_THREAD_ID, 0x00);
 // IsdebugLogEnabled = (mConfig == 0x00)? false:true;
 // DLOG_IF(INFO, uwb_debug_enabled) << StringPrintf(
  //    "%s: NAME_UWB_FW_LOG_THREAD_ID: %x", __func__, IsdebugLogEnabled);

  mHalCallback = NULL;
  memset(&mHalEntryFuncs, 0, sizeof(mHalEntryFuncs));
  InitializeHalDeviceContext();
  UCI_TRACE_I("%s: exit", func);
}

/*******************************************************************************
**
** Function:    UwbAdaptation::Finalize(bool exitStatus)
**
** Description: class finalizer
**
** Returns:     none
**
*******************************************************************************/
void UwbAdaptation::Finalize(bool graceExit) {
  const char* func = "UwbAdaptation::Finalize";
  UNUSED(func);
  AutoThreadMutex a(sLock);

  UCI_TRACE_I("%s: enter, graceful: %d", func, graceExit);
  phUwb_GKI_shutdown();

  memset(&mHalEntryFuncs, 0, sizeof(mHalEntryFuncs));
  if(graceExit){
    UwbConfig::clear();
  }

  UCI_TRACE_I("%s: exit", func);
  delete this;
}

/*******************************************************************************
**
** Function:    UwbAdaptation::signal()
**
** Description: signal the CondVar to release the thread that is waiting
**
** Returns:     none
**
*******************************************************************************/
void UwbAdaptation::signal() { mCondVar.signal(); }

/*******************************************************************************
**
** Function:    UwbAdaptation::UWBA_TASK()
**
** Description: UWBA_TASK runs the GKI main task
**
** Returns:     none
**
*******************************************************************************/
uint32_t UwbAdaptation::UWBA_TASK(__attribute__((unused)) uint32_t arg) {
  const char* func = "UwbAdaptation::UWBA_TASK";
  UNUSED(func);
  UCI_TRACE_I("%s: enter", func);
  phUwb_GKI_run(0);
  UCI_TRACE_I("%s: exit", func);
  return 0;
}

/*******************************************************************************
**
** Function:    UwbAdaptation::Thread()
**
** Description: Creates work threads
**
** Returns:     none
**
*******************************************************************************/
uint32_t UwbAdaptation::Thread(__attribute__((unused)) uint32_t arg) {
  const char* func = "UwbAdaptation::Thread";
  UNUSED(func);
  UCI_TRACE_I("%s: enter", func);

  {
    ThreadCondVar CondVar;
    AutoThreadMutex guard(CondVar);
    phUwb_GKI_create_task((TASKPTR)uwb_task, UWB_TASK, (int8_t*)"UWB_TASK", 0, 0,
                    (pthread_cond_t*)CondVar, (pthread_mutex_t*)CondVar);
    CondVar.wait();
  }

  UwbAdaptation::GetInstance().signal();

  phUwb_GKI_exit_task(phUwb_GKI_get_taskid());
  UCI_TRACE_I("%s: exit", func);
  return 0;
}

/*******************************************************************************
**
** Function:    UwbAdaptation::GetHalEntryFuncs()
**
** Description: Get the set of HAL entry points.
**
** Returns:     Functions pointers for HAL entry points.
**
*******************************************************************************/
tHAL_UWB_ENTRY* UwbAdaptation::GetHalEntryFuncs() { return &mHalEntryFuncs; }

/*******************************************************************************
**
** Function:    UwbAdaptation::InitializeHalDeviceContext
**
** Description: Ask the generic Android HAL to find the Broadcom-specific HAL.
**
** Returns:     None.
**
*******************************************************************************/
void UwbAdaptation::InitializeHalDeviceContext() {
  const char* func = "UwbAdaptation::InitializeHalDeviceContext";
  UNUSED(func);
  UCI_TRACE_I("%s: enter", func);

  mHalEntryFuncs.open = HalOpen;
  mHalEntryFuncs.close = HalClose;
  mHalEntryFuncs.write = HalWrite;
  mHalEntryFuncs.ioctl = HalIoctl;
  mHalEntryFuncs.CoreInitialization = CoreInitialization;

  mHal = IUwb::getService();
  LOG_FATAL_IF(mHal == nullptr, "Failed to retrieve the UWB HAL!");
  UCI_TRACE_I("%s: IUwb::getService() returned %p (%s)", func,
                            mHal.get(),
                            (mHal->isRemote() ? "remote" : "local"));
}

/*******************************************************************************
**
** Function:    UwbAdaptation::HalOpen
**
** Description: Turn on controller, download firmware.
**
** Returns:     None.
**
*******************************************************************************/
void UwbAdaptation::HalOpen(tHAL_UWB_CBACK* p_hal_cback,
                            tHAL_UWB_DATA_CBACK* p_data_cback) {
  const char* func = "UwbAdaptation::HalOpen";
  UNUSED(func);
  UCI_TRACE_I("%s", func);
  mCallback = new UwbClientCallback(p_hal_cback, p_data_cback);
  if (mHal != nullptr) {
    mHal->open(mCallback);
  } else{
    UCI_TRACE_E("%s mHal is NULL", func);
  }
}
/*******************************************************************************
**
** Function:    UwbAdaptation::HalClose
**
** Description: Turn off controller.
**
** Returns:     None.
**
*******************************************************************************/
void UwbAdaptation::HalClose() {
  const char* func = "UwbAdaptation::HalClose";
  UNUSED(func);
//  int retval;
  UCI_TRACE_I("%s HalClose Enter", func);
  if(mHal != nullptr)
    mHal->close();
  UCI_TRACE_E("%s: status", func);
}

/*******************************************************************************
**
** Function:    UwbAdaptation::HalDeviceContextCallback
**
** Description: Translate generic Android HAL's callback into Broadcom-specific
**              callback function.
**
** Returns:     None.
**
*******************************************************************************/
void UwbAdaptation::HalDeviceContextCallback(uwb_event_t event,
                                             uwb_status_t event_status) {
  const char* func = "UwbAdaptation::HalDeviceContextCallback";
  UNUSED(func);
  UCI_TRACE_I("%s: event=%u", func, event);
  if (mHalCallback) mHalCallback(event, (uint16_t)event_status);
}

/*******************************************************************************
**
** Function:    UwbAdaptation::HalDeviceContextDataCallback
**
** Description: Translate generic Android HAL's callback into Broadcom-specific
**              callback function.
**
** Returns:     None.
**
*******************************************************************************/
void UwbAdaptation::HalDeviceContextDataCallback(uint16_t data_len,
                                                 uint8_t* p_data) {
  const char* func = "UwbAdaptation::HalDeviceContextDataCallback";
  UNUSED(func);
  UCI_TRACE_I("%s: len=%u", func, data_len);
  if (mHalDataCallback) mHalDataCallback(data_len, p_data);
}

/*******************************************************************************
**
** Function:    UwbAdaptation::HalWrite
**
** Description: Write UCI message to the controller.
**
** Returns:     None.
**
*******************************************************************************/
void UwbAdaptation::HalWrite(__attribute__((unused)) uint16_t data_len,
                             __attribute__((unused)) uint8_t* p_data) {
  const char* func = "UwbAdaptation::HalWrite";
  UNUSED(func);
  UCI_TRACE_I("%s: Enter", func);
  ::vendor::xiaomi::uwb::V1_0::UwbData data;
  if(p_data == NULL){
    UCI_TRACE_E("p_data is null");
    return;
  }
  data.setToExternal(p_data, data_len);
  if(mHal != nullptr){
    mHal->write(data);
  } else{
    UCI_TRACE_E("mHal is NULL");
  }
}

/*******************************************************************************
**
** Function:    IoctlCallback
**
** Description: Callback from HAL stub for IOCTL api invoked.
**              Output data for IOCTL is sent as argument
**
** Returns:     None.
**
*******************************************************************************/
void IoctlCallback(::vendor::xiaomi::uwb::V1_0::UwbData outputData) {
  const char* func = "IoctlCallback";
  UNUSED(func);
  uwb_uci_ExtnOutputData_t* pOutData =
      (uwb_uci_ExtnOutputData_t*)&outputData[0];
  UCI_TRACE_I("%s Ioctl Type=%llu", func, (unsigned long long)pOutData->ioctlType);
  UwbAdaptation* pAdaptation = (UwbAdaptation*)pOutData->context;
  /*Output Data from stub->Proxy is copied back to output data
   * This data will be sent back to libuwb*/
  if(pAdaptation->mCurrentIoctlData != NULL) {
    memcpy(&pAdaptation->mCurrentIoctlData->out, &outputData[0],
         sizeof(uwb_uci_ExtnOutputData_t));
  }
}

/*******************************************************************************
**
** Function:    UwbAdaptation::HalIoctl
**
** Description: Calls ioctl to the Uwb driver.
**              If called with a arg value of 0x01 than wired access requested,
**              status of the requst would be updated to p_data.
**              If called with a arg value of 0x00 than wired access will be
**              released, status of the requst would be updated to p_data.
**              If called with a arg value of 0x02 than current p61 state would
*be
**              updated to p_data.
**
** Returns:     -1 or 0.
**
*******************************************************************************/
tUWB_STATUS UwbAdaptation::HalIoctl(__attribute__((unused)) long arg,
                            __attribute__((unused)) void* p_data) {
  const char* func = "UwbAdaptation::HalIoctl";
  UNUSED(func);
  ::vendor::xiaomi::uwb::V1_0::UwbData data;
  sIoctlMutex.lock();
  uwb_uci_IoctlInOutData_t* pInpOutData = (uwb_uci_IoctlInOutData_t*)p_data;
  UCI_TRACE_I("%s arg=%ld", func, arg);
  pInpOutData->inp.context = &UwbAdaptation::GetInstance();
  UwbAdaptation::GetInstance().mCurrentIoctlData = pInpOutData;
  data.setToExternal((uint8_t*)pInpOutData, sizeof(uwb_uci_IoctlInOutData_t));

  if(mHal != nullptr)
      mHal->ioctl(arg, data, IoctlCallback);
  UCI_TRACE_I("%s Ioctl Completed for Type=%llu", func, (unsigned long long)pInpOutData->out.ioctlType);
  sIoctlMutex.unlock();
  return (pInpOutData->out.result);
}

 /*******************************************************************************
 **
 ** Function:    UwbAdaptation::CoreInitialization
 **
 ** Description: Performs UWB CoreInitialization.
 **
 ** Returns:     UwbStatus::OK on success and UwbStatus::FAILED on error.
 **
 *******************************************************************************/
tUWB_STATUS UwbAdaptation::CoreInitialization(bool recovery) {
  const char* func = "UwbAdaptation::CoreInitialization";
  UNUSED(func);
  uint8_t stat = UWB_STATUS_FAILED;
  UCI_TRACE_I("%s: enter", func);
  uwb_uci_IoctlInOutData_t inpOutData;
  inpOutData.inp.data.recovery = recovery;
  if (mHal) {
    stat = uwb_cb.p_hal->ioctl(HAL_UWB_IOCTL_CORE_INIT, (void*)&inpOutData);
  } else {
    UCI_TRACE_E("mHal is NULL");
  }
  return stat;
}

/*******************************************************************************
**
** Function:    UwbAdaptation::DumpFwDebugLog
**
** Description: set Fw debug configuration.
**
** Returns:     None.
**
*******************************************************************************/
tUWB_STATUS UwbAdaptation::DumpFwDebugLog(bool enableFwDebugLogs,bool enableCirDebugLogs) {
  const char* func = "UwbAdaptation::DumpFwDebugLog";
  UNUSED(func);
  ::vendor::xiaomi::uwb::V1_0::UwbData data;
  uwb_uci_IoctlInOutData_t inpOutData;
  if(enableFwDebugLogs && enableCirDebugLogs){
    inpOutData.inp.data.fwLogType = DUMP_FW_DEBUG_CIR_LOG;
  } else if(enableFwDebugLogs){
    inpOutData.inp.data.fwLogType = DUMP_FW_DEBUG_LOG;
  } else if(enableCirDebugLogs){
    inpOutData.inp.data.fwLogType = DUMP_FW_CIR_LOG;
  } else{
    inpOutData.inp.data.fwLogType = DUMP_FW_DISABLE_ALL_LOG;
  }
  uint8_t stat;
  UCI_TRACE_I("%s ", func);
  stat = uwb_cb.p_hal->ioctl(HAL_UWB_IOCTL_DUMP_FW_LOG, (void*)&inpOutData);
  UCI_TRACE_I("%s Ioctl Completed", func);
  return stat;
}

/*******************************************************************************
**
** Function:    UwbAdaptation::HalDownloadFirmwareCallback
**
** Description: Receive events from the HAL.
**
** Returns:     None.
**
*******************************************************************************/
void UwbAdaptation::HalDownloadFirmwareCallback(uwb_event_t event,
                                                __attribute__((unused))
                                                uwb_status_t event_status) {
  const char* func = "UwbAdaptation::HalDownloadFirmwareCallback";
  UNUSED(func);
  UCI_TRACE_I("%s: event=0x%X", func, event);
  switch (event) {
    case HAL_UWB_OPEN_CPLT_EVT: {
      UCI_TRACE_I("%s: HAL_UWB_OPEN_CPLT_EVT", func);
      mHalOpenCompletedEvent.signal();
      break;
    }
    case HAL_UWB_CLOSE_CPLT_EVT: {
      UCI_TRACE_I("%s: HAL_UWB_CLOSE_CPLT_EVT", func);
      mHalCloseCompletedEvent.signal();
      break;
    }
  }
}

/*******************************************************************************
**
** Function:    UwbAdaptation::HalDownloadFirmwareDataCallback
**
** Description: Receive data events from the HAL.
**
** Returns:     None.
**
*******************************************************************************/
void UwbAdaptation::HalDownloadFirmwareDataCallback(__attribute__((unused))
                                                    uint16_t data_len,
                                                    __attribute__((unused))
                                                    uint8_t* p_data) {}

/*******************************************************************************
**
** Function:    ThreadMutex::ThreadMutex()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
ThreadMutex::ThreadMutex() {
  pthread_mutexattr_t mutexAttr;

  pthread_mutexattr_init(&mutexAttr);
  pthread_mutex_init(&mMutex, &mutexAttr);
  pthread_mutexattr_destroy(&mutexAttr);
}

/*******************************************************************************
**
** Function:    ThreadMutex::~ThreadMutex()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
ThreadMutex::~ThreadMutex() { pthread_mutex_destroy(&mMutex); }

/*******************************************************************************
**
** Function:    ThreadMutex::lock()
**
** Description: lock kthe mutex
**
** Returns:     none
**
*******************************************************************************/
void ThreadMutex::lock() { pthread_mutex_lock(&mMutex); }

/*******************************************************************************
**
** Function:    ThreadMutex::unblock()
**
** Description: unlock the mutex
**
** Returns:     none
**
*******************************************************************************/
void ThreadMutex::unlock() { pthread_mutex_unlock(&mMutex); }

/*******************************************************************************
**
** Function:    ThreadCondVar::ThreadCondVar()
**
** Description: class constructor
**
** Returns:     none
**
*******************************************************************************/
ThreadCondVar::ThreadCondVar() {
  pthread_condattr_t CondAttr;

  pthread_condattr_init(&CondAttr);
  pthread_cond_init(&mCondVar, &CondAttr);

  pthread_condattr_destroy(&CondAttr);
}

/*******************************************************************************
**
** Function:    ThreadCondVar::~ThreadCondVar()
**
** Description: class destructor
**
** Returns:     none
**
*******************************************************************************/
ThreadCondVar::~ThreadCondVar() { pthread_cond_destroy(&mCondVar); }

/*******************************************************************************
**
** Function:    ThreadCondVar::wait()
**
** Description: wait on the mCondVar
**
** Returns:     none
**
*******************************************************************************/
void ThreadCondVar::wait() {
  pthread_cond_wait(&mCondVar, *this);
  pthread_mutex_unlock(*this);
}

/*******************************************************************************
**
** Function:    ThreadCondVar::signal()
**
** Description: signal the mCondVar
**
** Returns:     none
**
*******************************************************************************/
void ThreadCondVar::signal() {
  AutoThreadMutex a(*this);
  pthread_cond_signal(&mCondVar);
}

/*******************************************************************************
**
** Function:    AutoThreadMutex::AutoThreadMutex()
**
** Description: class constructor, automatically lock the mutex
**
** Returns:     none
**
*******************************************************************************/
AutoThreadMutex::AutoThreadMutex(ThreadMutex& m) : mm(m) { mm.lock(); }

/*******************************************************************************
**
** Function:    AutoThreadMutex::~AutoThreadMutex()
**
** Description: class destructor, automatically unlock the mutex
**
** Returns:     none
**
*******************************************************************************/
AutoThreadMutex::~AutoThreadMutex() { mm.unlock(); }
