/*****************************************************************************
Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/
#define LOG_TAG "IMSConfig_VTS"
#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetCallbackBase.h>
#include <log/log.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <zlib.h>
#include <vendor/qti/ims/factory/2.0/IImsFactory.h>
#include <vendor/qti/ims/configservice/1.0/IConfigService.h>

#define GZIP_WINDOWSIZE 15
#define GZIP_CFACTOR 9
#define GZIP_BSIZE 8096
#define GZIP_OUTPUTBUFFSIZE 32768

using ::android::sp;
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

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

constexpr char kCallbackOnCommandStatus[] = "onCommandStatus";
constexpr char kCallbackOnReconfigNeeded[] = "onReconfigNeeded";
constexpr char kCallbackOnUceStatusUpdate[] = "onUceStatusUpdate";
constexpr char kCallbackOnTokenFetchRequest[] = "onTokenFetchRequest";
constexpr char kCallbackOnGetUpdatedSettings[] = "onGetUpdatedSettings";
constexpr char kCallbackOnGetSettingsResponse[] = "onGetSettingsResponse";
constexpr char kCallbackOnRcsServiceStatusUpdate[] = "onRcsServiceStatusUpdate";
constexpr char kCallbackOnAutoConfigurationReceived[] = "onAutoConfigurationReceived";
constexpr char kCallbackOnAutoConfigErrorSipResponse[] = "onAutoConfigErrorSipResponse";

constexpr char slot_id_key[] = "slot_id";
constexpr char isCompressed_key[] = "isCompressed";
constexpr char xmlFileName_key[] = "xmlFile";
constexpr char AppToken_key[] = "app_token";
constexpr char PROPERTIES_FILE[] = "/data/properties.txt";
void loadProperties();

class mockConfigServiceListenerArgs
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

class mockConfigServiceListener : public IConfigServiceListener,
                              public ::testing::VtsHalHidlTargetCallbackBase<mockConfigServiceListenerArgs>
{
public:
  mockConfigServiceListener() {};
  ~mockConfigServiceListener() {};

  Return<void> onCommandStatus(RequestStatus status, uint32_t userData)
  {
     ALOGI("%s - mockConfigServiceListener: onCommandStatus received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.status = status;
     args.userData = userData;
     NotifyFromCallback(kCallbackOnCommandStatus, args);
     return Void();
  }

  Return<void> onGetSettingsResponse(RequestStatus status, const SettingsData& cbdata, uint32_t userData)
  {
     ALOGI("%s - mockConfigServiceListener: onGetSettingsResponse received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.status = status;
     args.userData = userData;
     args.settingsData = cbdata;
     NotifyFromCallback(kCallbackOnGetSettingsResponse, args);
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

  Return<void> onGetUpdatedSettings(const SettingsData& cbdata)
  {
     ALOGI("%s - mockConfigServiceListener: onGetUpdatedSettings received",LOG_TAG);
     mockConfigServiceListenerArgs args;
     args.settingsData = cbdata;
     NotifyFromCallback(kCallbackOnGetUpdatedSettings, args);
     return Void();
  }

};

class ImsConfigVtsTest
{
public:
  ImsConfigVtsTest() {};
  ~ImsConfigVtsTest() {};
  static std::map<std::string, std::string> g_configValues;

};

class ImsConfigServiceGenericTest : public ::testing::VtsHalHidlTargetTestBase, public ImsConfigVtsTest
{
public:
  virtual void SetUp() override;
  uint32_t userData = 1001;
  sp<IImsFactory> factory = nullptr;
  sp<IConfigService> pService = nullptr;
  mockConfigServiceListener* mListener;
};
