/*****************************************************************************
Copyright (c) 2020-2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/
#ifndef IMS_CONFIG_SERVICE_GENERIC_TEST_H
#define IMS_CONFIG_SERVICE_GENERIC_TEST_H

#include <vendor/qti/ims/factory/2.1/IImsFactory.h>
#include <vendor/qti/ims/configservice/1.1/IConfigService.h>
#include "ImsConfigCommon.h"


using ::vendor::qti::ims::factory::V2_1::IImsFactory;
using ::vendor::qti::ims::configservice::V1_1::IConfigService;
using ::vendor::qti::ims::configservice::V1_1::IConfigServiceListener;
using ::vendor::qti::ims::configservice::V1_0::ConfigData;
using ::vendor::qti::ims::configservice::V1_1::RequestStatus;
using ::vendor::qti::ims::configservice::V1_1::SettingsData;
using ::vendor::qti::ims::configservice::V1_0::SettingsId;
using ::vendor::qti::ims::configservice::V1_1::SettingsValues;
using ::vendor::qti::ims::configservice::V1_0::PresenceConfigKeys;
using ::vendor::qti::ims::configservice::V1_1::StandaloneMessagingConfigKeys;
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

class mockConfigServiceListenerArgs
{
public:
  RequestStatus status;
  ::vendor::qti::ims::configservice::V1_0::RequestStatus statusV0;
  int32_t requestId;
  uint32_t userData;
  TokenType tokenType;
  TokenRequestReason reqReason;
  SettingsData settingsData;
  ::vendor::qti::ims::configservice::V1_0::SettingsData settingsDataV0;
  UceCapabilityInfo uceCapData;
  AutoConfig acsData;
  bool isRcsEnabled;
  AutoConfigResponse acsResponse;
};

class mockConfigServiceListener : public IConfigServiceListener,
                              public ::testing::VtsHalHidlTargetCallbackBase<mockConfigServiceListenerArgs>
{
public:
  mockConfigServiceListener() {};
  ~mockConfigServiceListener() {};

  Return<void> onCommandStatus(::vendor::qti::ims::configservice::V1_0::RequestStatus status, uint32_t userData)
  {
     ALOGI("%s - mockConfigServiceListener: onCommandStatus received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.statusV0 = status;
     args.userData = userData;
     NotifyFromCallback(kCallbackOnCommandStatus, args);
     return Void();
  }

  Return<void> onCommandStatus_1_1(RequestStatus status, uint32_t userData)
  {
     ALOGI("%s - mockConfigServiceListener: onCommandStatus_1_1 received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.status = status;
     args.userData = userData;
     NotifyFromCallback(kCallbackOnCommandStatus_1_1, args);
     return Void();  }

  Return<void> onGetSettingsResponse(::vendor::qti::ims::configservice::V1_0::RequestStatus status, const ::vendor::qti::ims::configservice::V1_0::SettingsData& cbdata, uint32_t userData)
  {
     ALOGI("%s - mockConfigServiceListener: onGetSettingsResponse received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.statusV0 = status;
     args.userData = userData;
     args.settingsDataV0 = cbdata;
     NotifyFromCallback(kCallbackOnGetSettingsResponse, args);
     return Void();
  }

  Return<void> onGetSettingsResponse_1_1(::vendor::qti::ims::configservice::V1_1::RequestStatus status,
                                     const ::vendor::qti::ims::configservice::V1_1::SettingsData& cbdata, uint32_t userData)
  {
     ALOGI("%s - mockConfigServiceListener: onGetSettingsResponse_1_1 received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.status = status;
     args.userData = userData;
     args.settingsData = cbdata;
     NotifyFromCallback(kCallbackOnGetSettingsResponse_1_1, args);
     return Void();
  }

  Return<void> onAutoConfigurationReceived(const AutoConfig& acsConfig)
  {
     ALOGI("%s - mockConfigServiceListener: onAutoConfigurationReceived received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.acsData = acsConfig;
     NotifyFromCallback(kCallbackOnAutoConfigurationReceived, args);
     return Void();
  }

  Return<void> onReconfigNeeded()
  {
     ALOGI("%s - mockConfigServiceListener: onReconfigNeeded received",LOG_TAG);
     NotifyFromCallback(kCallbackOnReconfigNeeded);
     return Void();
  }

  Return<void> onTokenFetchRequest(int32_t requestId, TokenType tokenType, TokenRequestReason reqReason)
  {
     ALOGI("%s - mockConfigServiceListener: onTokenFetchRequest received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.requestId = requestId;
     args.tokenType = tokenType;
     args.reqReason = reqReason;
     NotifyFromCallback(kCallbackOnTokenFetchRequest, args);
     return Void();
  }

  Return<void> onUceStatusUpdate(const UceCapabilityInfo& capinfo)
  {
     ALOGI("%s - mockConfigServiceListener: onUceStatusUpdate received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.uceCapData = capinfo;
     NotifyFromCallback(kCallbackOnUceStatusUpdate, args);
     return Void();
  }

  Return<void> onRcsServiceStatusUpdate(bool isRcsEnabled)
  {
     ALOGI("%s - mockConfigServiceListener: onRcsServiceStatusUpdate received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.isRcsEnabled = isRcsEnabled;
     NotifyFromCallback(kCallbackOnRcsServiceStatusUpdate, args);
     return Void();
  }

  Return<void> onAutoConfigErrorSipResponse(const AutoConfigResponse& acsResponse)
  {
     ALOGI("%s - mockConfigServiceListener: onAutoConfigErrorSipResponse received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.acsResponse = acsResponse;
     NotifyFromCallback(kCallbackOnAutoConfigErrorSipResponse, args);
     return Void();
  }

  Return<void> onGetUpdatedSettings(const ::vendor::qti::ims::configservice::V1_0::SettingsData& cbdata)
  {
     ALOGI("%s - mockConfigServiceListener: onGetUpdatedSettings received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.settingsDataV0 = cbdata;
     NotifyFromCallback(kCallbackOnGetUpdatedSettings, args);
     return Void();
  }

  Return<void> onGetUpdatedSettings_1_1(const ::vendor::qti::ims::configservice::V1_1::SettingsData& cbdata)
  {
     ALOGI("%s - mockConfigServiceListener: onGetUpdatedSettings_1_1 received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.settingsData = cbdata;
     NotifyFromCallback(kCallbackOnGetUpdatedSettings_1_1, args);
     return Void();
  }
};

class ImsConfigVtsTest
{
public:
  ImsConfigVtsTest() {};
  ~ImsConfigVtsTest() {};
  static SettingsData presenceSettings;
  static SettingsData standAloneMsgSettings;
  static SettingsData userAgentStringSettings;
  static SettingsData imsServiceEnableConfig;
  static UceCapabilityInfo uceCapDataDefault;
};

class ImsConfigServiceGenericTest : public ::testing::VtsHalHidlTargetTestBase, public ImsConfigVtsTest
{
public:
  virtual void SetUp() override;
  sp<IImsFactory> factory = nullptr;
  sp<IConfigService> pService = nullptr;
  mockConfigServiceListener* mListener;
};

#endif
