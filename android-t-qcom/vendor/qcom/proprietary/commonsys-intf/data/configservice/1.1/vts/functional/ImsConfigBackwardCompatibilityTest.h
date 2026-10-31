/*****************************************************************************
Copyright (c) 2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/
#ifndef IMS_CONFIG_BACKWARD_COMPAT_TEST_H
#define IMS_CONFIG_BACKWARD_COMPAT_TEST_H

#define LOG_TAG "IMSConfig_VTS"

#include "ImsConfigCommon.h"
#include <vendor/qti/ims/factory/2.1/IImsFactory.h>
#include <vendor/qti/ims/configservice/1.0/IConfigService.h>

using ::vendor::qti::ims::factory::V2_0::IImsFactory;
using ::vendor::qti::ims::configservice::V1_0::IConfigService;
using ::vendor::qti::ims::configservice::V1_0::IConfigServiceListener;
using ::vendor::qti::ims::configservice::V1_0::ConfigData;
using ::vendor::qti::ims::configservice::V1_0::RequestStatus;
using ::vendor::qti::ims::configservice::V1_0::SettingsData;
using ::vendor::qti::ims::configservice::V1_0::SettingsId;
using ::vendor::qti::ims::configservice::V1_0::SettingsValues;
using ::vendor::qti::ims::configservice::V1_0::PresenceConfigKeys;
using ::vendor::qti::ims::configservice::V1_0::StandaloneMessagingConfigKeys;
using ::vendor::qti::ims::configservice::V1_0::UserAgentStringKeys;
using ::vendor::qti::ims::configservice::V1_0::ImsServiceEnableConfigKeys;
using ::vendor::qti::ims::configservice::V1_0::KeyValuePairTypeInt;
using ::vendor::qti::ims::configservice::V1_0::KeyValuePairTypeBool;
using ::vendor::qti::ims::configservice::V1_0::KeyValuePairTypeString;
using ::vendor::qti::ims::configservice::V1_0::UceCapabilityInfo;
using ::vendor::qti::ims::configservice::V1_0::TokenType;
using ::vendor::qti::ims::configservice::V1_0::TokenRequestReason;
using ::vendor::qti::ims::configservice::V1_0::StatusCode;
using ::vendor::qti::ims::configservice::V1_0::AutoConfigRequestType;
using ::vendor::qti::ims::configservice::V1_0::AutoConfig;
using ::vendor::qti::ims::configservice::V1_0::AutoConfigTriggerReason;
using ::vendor::qti::ims::configservice::V1_0::AutoConfigResponse;

extern std::map<std::string, std::string> g_configValues;

class mockConfigServiceListenerArgs_V1_0
{
public:
  RequestStatus status;
  int32_t requestId;
  uint32_t userData;
  TokenType tokenType;
  TokenRequestReason reqReason;
  SettingsData settingsData;
  UceCapabilityInfo uceCapData;
  AutoConfig acsData;
  bool isRcsEnabled;
  AutoConfigResponse acsResponse;
};

class mockConfigServiceListener_V1_0 : public IConfigServiceListener,
                              public ::testing::VtsHalHidlTargetCallbackBase<mockConfigServiceListenerArgs_V1_0>
{
public:
  mockConfigServiceListener_V1_0() {};
  ~mockConfigServiceListener_V1_0() {};

  Return<void> onCommandStatus(RequestStatus status, uint32_t userData)
  {
     ALOGI("%s - mockConfigServiceListener_V1_0: onCommandStatus received",LOG_TAG);
     mockConfigServiceListenerArgs_V1_0 args;
     args.status = status;
     args.userData = userData;
     NotifyFromCallback(kCallbackOnCommandStatus, args);
     return Void();
  }


  Return<void> onGetSettingsResponse(RequestStatus status, const SettingsData& cbdata, uint32_t userData)
  {
     ALOGI("%s - mockConfigServiceListener_V1_0: onGetSettingsResponse received",LOG_TAG);
     mockConfigServiceListenerArgs_V1_0 args;
     args.status = status;
     args.userData = userData;
     args.settingsData = cbdata;
     NotifyFromCallback(kCallbackOnGetSettingsResponse, args);
     return Void();
  }

  Return<void> onAutoConfigurationReceived(const AutoConfig& acsConfig)
  {
     ALOGI("%s - mockConfigServiceListener_V1_0: onAutoConfigurationReceived received",LOG_TAG);
     mockConfigServiceListenerArgs_V1_0 args;
     args.acsData = acsConfig;
     NotifyFromCallback(kCallbackOnAutoConfigurationReceived, args);
     return Void();
  }

  Return<void> onReconfigNeeded()
  {
     ALOGI("%s - mockConfigServiceListener_V1_0: onReconfigNeeded received",LOG_TAG);
     NotifyFromCallback(kCallbackOnReconfigNeeded);
     return Void();
  }

  Return<void> onTokenFetchRequest(int32_t requestId, TokenType tokenType, TokenRequestReason reqReason)
  {
     ALOGI("%s - mockConfigServiceListener_V1_0: onTokenFetchRequest received",LOG_TAG);
     mockConfigServiceListenerArgs_V1_0 args;
     args.requestId = requestId;
     args.tokenType = tokenType;
     args.reqReason = reqReason;
     NotifyFromCallback(kCallbackOnTokenFetchRequest, args);
     return Void();
  }

  Return<void> onUceStatusUpdate(const UceCapabilityInfo& capinfo)
  {
     ALOGI("%s - mockConfigServiceListener_V1_0: onUceStatusUpdate received",LOG_TAG);
     mockConfigServiceListenerArgs_V1_0 args;
     args.uceCapData = capinfo;
     NotifyFromCallback(kCallbackOnUceStatusUpdate, args);
     return Void();
  }

  Return<void> onRcsServiceStatusUpdate(bool isRcsEnabled)
  {
     ALOGI("%s - mockConfigServiceListener_V1_0: onRcsServiceStatusUpdate received",LOG_TAG);
     mockConfigServiceListenerArgs_V1_0 args;
     args.isRcsEnabled = isRcsEnabled;
     NotifyFromCallback(kCallbackOnRcsServiceStatusUpdate, args);
     return Void();
  }

  Return<void> onAutoConfigErrorSipResponse(const AutoConfigResponse& acsResponse)
  {
     ALOGI("%s - mockConfigServiceListener_V1_0: onAutoConfigErrorSipResponse received",LOG_TAG);
     mockConfigServiceListenerArgs_V1_0 args;
     args.acsResponse = acsResponse;
     NotifyFromCallback(kCallbackOnAutoConfigErrorSipResponse, args);
     return Void();
  }

  Return<void> onGetUpdatedSettings(const SettingsData& cbdata)
  {
     ALOGI("%s - mockConfigServiceListener_V1_0: onGetUpdatedSettings received",LOG_TAG);
     mockConfigServiceListenerArgs_V1_0 args;
     args.settingsData = cbdata;
     NotifyFromCallback(kCallbackOnGetUpdatedSettings, args);
     return Void();
  }

};

class ImsConfigBackwardCompatibilityTest : public ::testing::VtsHalHidlTargetTestBase
{
public:
  virtual void SetUp() override;
  sp<IImsFactory> factory = nullptr;
  sp<IConfigService> pService = nullptr;
  mockConfigServiceListener_V1_0* mListener;
  static UceCapabilityInfo uceCapValues;
};
#endif