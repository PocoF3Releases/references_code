/*****************************************************************************
Copyright (c) 2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/
#include "ImsConfigBackwardCompatibilityTest.h"

using ::vendor::qti::ims::configservice::V1_0::StatusCode;

UceCapabilityInfo ImsConfigBackwardCompatibilityTest::uceCapValues;

void ImsConfigBackwardCompatibilityTest::SetUp()
  {
    ALOGI("%s - Setup function",LOG_TAG);
    factory = IImsFactory::getService();
    IImsFactory::StatusCode status = IImsFactory::StatusCode::NOT_SUPPORTED;
    mListener = new mockConfigServiceListener_V1_0();
    if(factory != nullptr)
    {
      for(auto it : g_configValues)
      {
        std::cout << "Setup g_configValues [" << it.first << "] value[" << it.second << "]\n";
        ALOGI("Setup g_configValues[%s][%s]", it.first.c_str(), it.second.c_str() );
      }
      int testI = atoi(g_configValues[slot_id_key].c_str());
      ALOGI("Setup testI[%d]", testI);
      std::cout << "Setup testI[" << testI <<"]";
      factory->createConfigService(atoi(g_configValues[slot_id_key].c_str()),
                                    mListener,
                                   [&](IImsFactory::StatusCode _status, sp<IConfigService> config) {
                                   status = _status;
                                   pService = config;
                                   /* end Lamda Func*/ } );

    }
    userData++;
}

TEST_F(ImsConfigBackwardCompatibilityTest, ServiceAvailableTestWithFactory2_1) {
  IImsFactory::StatusCode status = IImsFactory::StatusCode::NOT_SUPPORTED;
  sp<::vendor::qti::ims::factory::V2_1::IImsFactory> factory = nullptr;
  sp<IConfigService> service = nullptr;

  factory = ::vendor::qti::ims::factory::V2_1::IImsFactory::getService();

  if(factory != nullptr)
  {
    factory->createConfigService(atoi(g_configValues[slot_id_key].c_str()),
              mListener,
              [&](IImsFactory::StatusCode _status, sp<IConfigService> config) {
                    status = _status;
                    service = config;
              /* end Lamda Func*/ } );
      if(service)
      ASSERT_NE(nullptr, service.get()) << "Could not get Service";
  }
  else
  {
    ALOGI("No service object received");
    //ASSERT_NE(nullptr, service.get()) << "Could not get Service";
  }
  ALOGI("Things worked as expected");
}

TEST_F(ImsConfigBackwardCompatibilityTest, ServiceAvailableTest) {
  IImsFactory::StatusCode status = IImsFactory::StatusCode::NOT_SUPPORTED;
  sp<IImsFactory> factory = nullptr;
  sp<IConfigService> service = nullptr;

  factory = IImsFactory::getService();

  if(factory != nullptr)
  {
    factory->createConfigService(atoi(g_configValues[slot_id_key].c_str()),
              mListener,
              [&](IImsFactory::StatusCode _status, sp<IConfigService> config) {
                    status = _status;
                    service = config;
              /* end Lamda Func*/ } );
      if(service)
      ASSERT_NE(nullptr, service.get()) << "Could not get Service";
  }
  else
  {
    ALOGI("No service object received");
    //ASSERT_NE(nullptr, service.get()) << "Could not get Service";
  }
  ALOGI("Things worked as expected");
}

TEST_F(ImsConfigBackwardCompatibilityTest, GetUceStatusTest) {

  uint32_t status = (uint32_t)RequestStatus::FAIL;
  if(pService != nullptr)
  {
    auto result = pService->getUceStatus([&](int32_t _status, UceCapabilityInfo _uceCapInfo) {
                                               status = _status;
                                               uceCapValues.isPresenceEnabled = _uceCapInfo.isPresenceEnabled;
                                               uceCapValues.isOptionsEnabled = _uceCapInfo.isOptionsEnabled;});

    EXPECT_EQ(RequestStatus::OK,(RequestStatus)status);
    //EXPECT_TRUE(uceCapValues.isPresenceEnabled);
    //EXPECT_TRUE(uceCapValues.isOptionsEnabled);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigBackwardCompatibilityTest, UpdateUceStatusTest) {

  uint32_t status = (uint32_t)RequestStatus::FAIL;
  UceCapabilityInfo uceCapInfo;

  SettingsData data;
  SettingsValues settingsValues;

  vector<KeyValuePairTypeBool> boolDataVec;
  vector<KeyValuePairTypeString> stringDataVec;
  vector<KeyValuePairTypeInt> intDataVec;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[0].key = (uint32_t)ImsServiceEnableConfigKeys::PRESENCE_ENABLED_KEY;
  boolDataVec[0].value = !(uceCapValues.isPresenceEnabled);

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[1].key = (uint32_t)ImsServiceEnableConfigKeys::OPTIONS_ENABLED_KEY;
  boolDataVec[1].value = !(uceCapValues.isOptionsEnabled);

  settingsValues.boolData = boolDataVec;
  settingsValues.stringData = stringDataVec;
  settingsValues.intData = intDataVec;
  data.settingsValues = settingsValues;
  data.settingsId = SettingsId::IMS_SERVICE_ENABLE_CONFIG;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue(data, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings);

    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res1.args->status);
    EXPECT_NE(uceCapValues.isOptionsEnabled,res1.args->uceCapData.isOptionsEnabled);
    EXPECT_NE(uceCapValues.isPresenceEnabled,res1.args->uceCapData.isPresenceEnabled);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigBackwardCompatibilityTest, RestoreUceStatusTest) {

  uint32_t status = (uint32_t)RequestStatus::FAIL;
  UceCapabilityInfo uceCapInfo;

  SettingsData data;
  SettingsValues settingsValues;

  vector<KeyValuePairTypeBool> boolDataVec;
  vector<KeyValuePairTypeString> stringDataVec;
  vector<KeyValuePairTypeInt> intDataVec;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[0].key = (uint32_t)ImsServiceEnableConfigKeys::PRESENCE_ENABLED_KEY;
  boolDataVec[0].value = uceCapValues.isPresenceEnabled;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[1].key = (uint32_t)ImsServiceEnableConfigKeys::OPTIONS_ENABLED_KEY;
  boolDataVec[1].value = uceCapValues.isOptionsEnabled;

  settingsValues.boolData = boolDataVec;
  settingsValues.stringData = stringDataVec;
  settingsValues.intData = intDataVec;
  data.settingsValues = settingsValues;
  data.settingsId = SettingsId::IMS_SERVICE_ENABLE_CONFIG;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue(data, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings);

    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res1.args->status);

    auto result1 = pService->getUceStatus([&](int32_t _status, UceCapabilityInfo _uceCapInfo) {
                                               status = _status;
                                               uceCapInfo.isPresenceEnabled = _uceCapInfo.isPresenceEnabled;
                                               uceCapInfo.isOptionsEnabled = _uceCapInfo.isOptionsEnabled;});

    EXPECT_EQ(RequestStatus::OK,(RequestStatus)status);

    EXPECT_EQ(uceCapValues.isOptionsEnabled, uceCapInfo.isOptionsEnabled);
    EXPECT_EQ(uceCapValues.isPresenceEnabled,uceCapInfo.isPresenceEnabled);

  }
  else
  {
     ALOGE("This functionality is not supported");
  }
}

TEST_F(ImsConfigBackwardCompatibilityTest, GetSettingsDataPresenceTest) {

  if(pService != nullptr)
  {
    auto result = pService->getSettingsValue(SettingsId::PRESENCE_CONFIG, userData);
    auto res = mListener->WaitForCallback(kCallbackOnGetSettingsResponse);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(SettingsId::PRESENCE_CONFIG, res.args->settingsData.settingsId);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigBackwardCompatibilityTest, GetSettingsDataStandaloneTest) {

  if(pService != nullptr)
  {
    auto result = pService->getSettingsValue(SettingsId::STANDALONE_MESSAGING_CONFIG, userData);
    auto res = mListener->WaitForCallback(kCallbackOnGetSettingsResponse);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(SettingsId::STANDALONE_MESSAGING_CONFIG, res.args->settingsData.settingsId);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigBackwardCompatibilityTest, GetSettingsDataUserAgentTest) {

  if(pService != nullptr)
  {
    auto result = pService->getSettingsValue(SettingsId::USER_AGENT_STRING, userData);
    auto res = mListener->WaitForCallback(kCallbackOnGetSettingsResponse);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(SettingsId::USER_AGENT_STRING, res.args->settingsData.settingsId);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigBackwardCompatibilityTest, GetSettingsDataImsServiceTest) {

  if(pService != nullptr)
  {
    auto result = pService->getSettingsValue(SettingsId::IMS_SERVICE_ENABLE_CONFIG, userData);
    auto res = mListener->WaitForCallback(kCallbackOnGetSettingsResponse);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(SettingsId::IMS_SERVICE_ENABLE_CONFIG, res.args->settingsData.settingsId);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}
