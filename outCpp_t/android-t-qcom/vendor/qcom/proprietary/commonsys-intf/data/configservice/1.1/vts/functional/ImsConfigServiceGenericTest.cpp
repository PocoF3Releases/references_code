/*****************************************************************************
Copyright (c) 2020-2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/
#include "ImsConfigServiceGenericTest.h"

using ::vendor::qti::ims::configservice::V1_0::StatusCode;

SettingsData ImsConfigVtsTest::presenceSettings;
SettingsData ImsConfigVtsTest::standAloneMsgSettings;
SettingsData ImsConfigVtsTest::userAgentStringSettings;
SettingsData ImsConfigVtsTest::imsServiceEnableConfig;
UceCapabilityInfo ImsConfigVtsTest::uceCapDataDefault;

void ImsConfigServiceGenericTest::SetUp()
  {
    ALOGI("%s :ImsConfigServiceGenericTest - Setup function",LOG_TAG);
    factory = IImsFactory::getService();
    IImsFactory::StatusCode status = IImsFactory::StatusCode::NOT_SUPPORTED;
    mListener = new mockConfigServiceListener();
    if(factory != nullptr)
    {
      for(auto it : g_configValues)
      {
        std::cout << "ImsConfigServiceGenericTest : Setup g_configValues [" << it.first << "] value[" << it.second << "]\n";
        ALOGI("ImsConfigServiceGenericTest : Setup g_configValues[%s][%s]", it.first.c_str(), it.second.c_str() );
      }
      int testI = atoi(g_configValues[slot_id_key].c_str());
      ALOGI("ImsConfigServiceGenericTest : Setup testI[%d]", testI);
      std::cout << "ImsConfigServiceGenericTest : Setup testI[" << testI <<"]";
      factory->createConfigService_1_1(atoi(g_configValues[slot_id_key].c_str()),
                                    mListener,
                                   [&](IImsFactory::StatusCode _status, sp<IConfigService> config) {
                                   status = _status;
                                   pService = config;
                                   /* end Lamda Func*/ } );

    }
    userData++;
}

TEST_F(ImsConfigServiceGenericTest, ServiceAvailableTest) {
  IImsFactory::StatusCode status = IImsFactory::StatusCode::NOT_SUPPORTED;
  sp<IImsFactory> factory = nullptr;
  sp<IConfigService> service = nullptr;

  factory = IImsFactory::getService();

  if(factory != nullptr)
  {
    factory->createConfigService_1_1(atoi(g_configValues[slot_id_key].c_str()),
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


TEST_F(ImsConfigServiceGenericTest, GetUceStatusTest) {

  uint32_t status = (uint32_t)RequestStatus::FAIL;
  if(pService != nullptr)
  {
    auto result = pService->getUceStatus([&](int32_t _status, UceCapabilityInfo _uceCapInfo) {
                                               status = _status;
                                               uceCapDataDefault.isPresenceEnabled = _uceCapInfo.isPresenceEnabled;
                                               uceCapDataDefault.isOptionsEnabled = _uceCapInfo.isOptionsEnabled;});

    EXPECT_EQ(RequestStatus::OK,(RequestStatus)status);
    EXPECT_TRUE(uceCapDataDefault.isPresenceEnabled);
    EXPECT_TRUE(uceCapDataDefault.isOptionsEnabled);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, UpdateUceStatusTest) {

  uint32_t status = (uint32_t)RequestStatus::FAIL;
  UceCapabilityInfo uceCapInfo;

  SettingsData data;
  SettingsValues settingsValues;

  vector<KeyValuePairTypeBool> boolDataVec;
  vector<KeyValuePairTypeString> stringDataVec;
  vector<KeyValuePairTypeInt> intDataVec;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[0].key = (uint32_t)ImsServiceEnableConfigKeys::PRESENCE_ENABLED_KEY;
  boolDataVec[0].value = !(uceCapDataDefault.isPresenceEnabled);

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[1].key = (uint32_t)ImsServiceEnableConfigKeys::OPTIONS_ENABLED_KEY;
  boolDataVec[1].value = !(uceCapDataDefault.isOptionsEnabled);

  settingsValues.boolData = boolDataVec;
  settingsValues.stringData = stringDataVec;
  settingsValues.intData = intDataVec;
  data.settingsValues = settingsValues;
  data.settingsId = SettingsId::IMS_SERVICE_ENABLE_CONFIG;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue_1_1(data, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings_1_1);

    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res1.args->status);
    EXPECT_NE(uceCapDataDefault.isOptionsEnabled,res1.args->uceCapData.isOptionsEnabled);
    EXPECT_NE(uceCapDataDefault.isPresenceEnabled,res1.args->uceCapData.isPresenceEnabled);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, RestoreUceStatusTest) {

  uint32_t status = (uint32_t)RequestStatus::FAIL;
  UceCapabilityInfo uceCapInfo;

  SettingsData data;
  SettingsValues settingsValues;

  vector<KeyValuePairTypeBool> boolDataVec;
  vector<KeyValuePairTypeString> stringDataVec;
  vector<KeyValuePairTypeInt> intDataVec;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[0].key = (uint32_t)ImsServiceEnableConfigKeys::PRESENCE_ENABLED_KEY;
  boolDataVec[0].value = uceCapDataDefault.isPresenceEnabled;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[1].key = (uint32_t)ImsServiceEnableConfigKeys::OPTIONS_ENABLED_KEY;
  boolDataVec[1].value = uceCapDataDefault.isOptionsEnabled;

  settingsValues.boolData = boolDataVec;
  settingsValues.stringData = stringDataVec;
  settingsValues.intData = intDataVec;
  data.settingsValues = settingsValues;
  data.settingsId = SettingsId::IMS_SERVICE_ENABLE_CONFIG;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue_1_1(data, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings_1_1);

    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res1.args->status);

    auto result1 = pService->getUceStatus([&](int32_t _status, UceCapabilityInfo _uceCapInfo) {
                                               status = _status;
                                               uceCapInfo.isPresenceEnabled = _uceCapInfo.isPresenceEnabled;
                                               uceCapInfo.isOptionsEnabled = _uceCapInfo.isOptionsEnabled;});

    EXPECT_EQ(RequestStatus::OK,(RequestStatus)status);

    EXPECT_EQ(uceCapDataDefault.isOptionsEnabled, uceCapInfo.isOptionsEnabled);
    EXPECT_EQ(uceCapDataDefault.isPresenceEnabled,uceCapInfo.isPresenceEnabled);

  }
  else
  {
     ALOGE("This functionality is not supported");
  }
}

TEST_F(ImsConfigServiceGenericTest, GetSettingsDataPresenceTest) {

  if(pService != nullptr)
  {
    auto result = pService->getSettingsValue(SettingsId::PRESENCE_CONFIG, userData);
    auto res = mListener->WaitForCallback(kCallbackOnGetSettingsResponse_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(SettingsId::PRESENCE_CONFIG, res.args->settingsData.settingsId);
    EXPECT_EQ(userData, res.args->userData);
    presenceSettings = res.args->settingsData;
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, GetSettingsDataStandaloneTest) {

  if(pService != nullptr)
  {
    auto result = pService->getSettingsValue(SettingsId::STANDALONE_MESSAGING_CONFIG, userData);
    auto res = mListener->WaitForCallback(kCallbackOnGetSettingsResponse_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(SettingsId::STANDALONE_MESSAGING_CONFIG, res.args->settingsData.settingsId);
    EXPECT_EQ(userData, res.args->userData);
    standAloneMsgSettings = res.args->settingsData;
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, GetSettingsDataUserAgentTest) {

  if(pService != nullptr)
  {
    auto result = pService->getSettingsValue(SettingsId::USER_AGENT_STRING, userData);
    auto res = mListener->WaitForCallback(kCallbackOnGetSettingsResponse_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(SettingsId::USER_AGENT_STRING, res.args->settingsData.settingsId);
    EXPECT_EQ(userData, res.args->userData);
    userAgentStringSettings = res.args->settingsData;
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, GetSettingsDataImsServiceTest) {

  if(pService != nullptr)
  {
    auto result = pService->getSettingsValue(SettingsId::IMS_SERVICE_ENABLE_CONFIG, userData);
    auto res = mListener->WaitForCallback(kCallbackOnGetSettingsResponse_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(SettingsId::IMS_SERVICE_ENABLE_CONFIG, res.args->settingsData.settingsId);
    EXPECT_EQ(userData, res.args->userData);
    imsServiceEnableConfig = res.args->settingsData;
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, RegisterForImsServiceIndications) {
  ALOGE("in vts register for indications");

  if(pService != nullptr)
  {
    ALOGE("calling RegisterForIndications");
    uint32_t result = pService->registerForSettingsChange(userData);
   // auto res = mListener->WaitForCallback(kCallbackonUpdateRcsSettingsResponse);
   // EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK,(RequestStatus)result);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, SetSettingsDataPresenceConfigTest) {
  ALOGE("in vts SetSettingsDataPresenceConfigTest rcs settings");
  SettingsData data;
  SettingsValues settingsValues;

  vector<KeyValuePairTypeInt> intDataVec;
  vector<KeyValuePairTypeBool> boolDataVec;
  vector<KeyValuePairTypeString> stringDataVec;

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[0].key = (uint32_t)PresenceConfigKeys::PUBLISH_TIMER_KEY;
  intDataVec[0].value = 100;

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[1].key = (uint32_t)PresenceConfigKeys::AVAILABILITY_CACHE_EXPIRY_KEY;
  intDataVec[1].value = 50;

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[2].key = (uint32_t)PresenceConfigKeys::PUBLISH_EXTENDED_TIMER_KEY;
  intDataVec[2].value = 50;

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[3].key = (uint32_t)PresenceConfigKeys::PUBLISH_SRC_THROTTLE_TIMER_KEY;
  intDataVec[3].value = 50;

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[4].key = (uint32_t)PresenceConfigKeys::PUBLISH_ERROR_RECOVERY_TIMER_KEY;
  intDataVec[4].value = 50;

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[5].key = (uint32_t)PresenceConfigKeys::LIST_SUBSCRIPTION_EXPIRY_KEY;
  intDataVec[5].value = 50;

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[6].key = (uint32_t)PresenceConfigKeys::CAPABILITES_CACHE_EXPIRY_KEY;
  intDataVec[6].value = 50;

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[7].key = (uint32_t)PresenceConfigKeys::CAPABILITY_POLL_INTERVAL_KEY;
  intDataVec[7].value = 50;

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[8].key = (uint32_t)PresenceConfigKeys::MAX_ENTIES_IN_LIST_SUBSCRIBE_KEY;
  intDataVec[8].value = 50;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[0].key = (uint32_t)PresenceConfigKeys::CAPABILITY_DISCOVERY_ENABLED_KEY;
  boolDataVec[0].value = false;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[1].key = (uint32_t)PresenceConfigKeys::GZIP_ENABLED_KEY;
  boolDataVec[1].value = true;

  stringDataVec.push_back(KeyValuePairTypeString());
  stringDataVec[0].key = (uint32_t)PresenceConfigKeys::USER_AGENT_KEY;
  stringDataVec[0].value = "Presence_config";

  settingsValues.intData = intDataVec;
  settingsValues.boolData = boolDataVec;
  settingsValues.stringData = stringDataVec;
  data.settingsValues = settingsValues;
  data.settingsId = SettingsId::PRESENCE_CONFIG;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue_1_1(data, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(SettingsId::PRESENCE_CONFIG, res1.args->settingsData.settingsId);  }
  else
  {
     ALOGE("This functionality is not supported");

  }

}

TEST_F(ImsConfigServiceGenericTest, SetSettingsDataStandaloneMessagingTest) {
  ALOGE("in vts SetSettingsDataStandaloneMessagingTest rcs settings");
  SettingsData data;
  SettingsValues settingsValues;

  vector<KeyValuePairTypeInt> intDataVec;
  vector<KeyValuePairTypeString> stringDataVec;
  //intDataVec.resize(2);
  settingsValues.stringData.resize(13);

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[0].key = (uint32_t)StandaloneMessagingConfigKeys::DEFAULT_SMS_APP_KEY;
  intDataVec[0].value = 100;

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[1].key = (uint32_t)StandaloneMessagingConfigKeys::DEFAULT_VVM_APP_KEY;
  intDataVec[1].value = 50;

  //stringDataVec.push_back(KeyValuePairTypeString());
  settingsValues.stringData[0].key = (uint32_t)StandaloneMessagingConfigKeys::AUTO_CONFIG_USER_AGENT_KEY;
  settingsValues.stringData[0].value = "AUTO_CONFIG_USER_AGENT_KEY";

 // stringDataVec.push_back(KeyValuePairTypeString());
  settingsValues.stringData[1].key = (uint32_t)StandaloneMessagingConfigKeys::XDM_CLIENT_USER_AGENT_KEY;
  settingsValues.stringData[1].value = "XDM_CLIENT_USER_AGENT_KEY";

 // stringDataVec.push_back(KeyValuePairTypeString());
  settingsValues.stringData[2].key = (uint32_t)StandaloneMessagingConfigKeys::CLIENT_VENDOR_KEY;
  settingsValues.stringData[2].value = "CLIENT_VENDOR";

 // stringDataVec.push_back(KeyValuePairTypeString());
  settingsValues.stringData[3].key = (uint32_t)StandaloneMessagingConfigKeys::CLIENT_VERSION_KEY;
  settingsValues.stringData[3].value = "CLIENT_VERSION";

//stringDataVec.push_back(KeyValuePairTypeString());
  settingsValues.stringData[4].key = (uint32_t)StandaloneMessagingConfigKeys::TERMINAL_VENDOR_KEY;
  settingsValues.stringData[4].value = "TERMINAL_VENDOR";

//stringDataVec.push_back(KeyValuePairTypeString());
  settingsValues.stringData[5].key = (uint32_t)StandaloneMessagingConfigKeys::TERMINAL_MODEL_KEY;
  settingsValues.stringData[5].value = "TERMINAL_MODEL";

//  stringDataVec.push_back(KeyValuePairTypeString());
  settingsValues.stringData[6].key = (uint32_t)StandaloneMessagingConfigKeys::TERMINAL_SW_VERSION_KEY;
  settingsValues.stringData[6].value = "TERMINAL_SW";

//  stringDataVec.push_back(KeyValuePairTypeString());
  settingsValues.stringData[7].key = (uint32_t)StandaloneMessagingConfigKeys::RCS_VERSION_KEY;
  settingsValues.stringData[7].value = "RCS_VERSION";

//  stringDataVec.push_back(KeyValuePairTypeString());
  settingsValues.stringData[8].key = (uint32_t)StandaloneMessagingConfigKeys::PROVISIONING_VERSION_KEY;
  settingsValues.stringData[8].value = "PROVISIONING";

 // stringDataVec.push_back(KeyValuePairTypeString());
  settingsValues.stringData[9].key = (uint32_t)StandaloneMessagingConfigKeys::FRIENDLY_DEVICE_NAME_KEY;
  settingsValues.stringData[9].value = "FRIENDLY_DEV";

//  stringDataVec.push_back(KeyValuePairTypeString());
  settingsValues.stringData[10].key = (uint32_t)StandaloneMessagingConfigKeys::RCS_PROFILE_KEY;
  settingsValues.stringData[10].value = "RCS_PROFILE";

//  stringDataVec.push_back(KeyValuePairTypeString());
  settingsValues.stringData[11].key = (uint32_t)StandaloneMessagingConfigKeys::BOT_VERSION_KEY;
  settingsValues.stringData[11].value = "BOT_VERSION_KEY";

  settingsValues.stringData[12].key = (uint32_t)StandaloneMessagingConfigKeys::APP_ID_KEY;
  settingsValues.stringData[12].value = "ap2002";
  
  settingsValues.intData = intDataVec;
  //settingsValues.stringData = stringDataVec;
  data.settingsValues = settingsValues;
  data.settingsId = SettingsId::STANDALONE_MESSAGING_CONFIG;


  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue_1_1(data, userData);
    //sleep(5);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(SettingsId::STANDALONE_MESSAGING_CONFIG, res1.args->settingsData.settingsId);  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}


TEST_F(ImsConfigServiceGenericTest, SetSettingsDataUserAgentTestEmptyValTest) {
  ALOGE("in vts SetSettingsDataUserAgentTestEmptyValTest rcs settings");
  SettingsData data;
  SettingsValues settingsValues;

  data.settingsValues = settingsValues;
  data.settingsId = SettingsId::USER_AGENT_STRING;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue_1_1(data, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::FAIL, res.args->status);
    EXPECT_EQ(userData, res.args->userData);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, SetSettingsDataUserAgentTest) {
  ALOGE("in vts SetSettingsDataUserAgentTest rcs settings");
  SettingsData data;
  SettingsValues settingsValues;

  vector<KeyValuePairTypeString> stringDataVec;

  stringDataVec.push_back(KeyValuePairTypeString());
  stringDataVec[0].key = (uint32_t)UserAgentStringKeys::IMS_USER_AGENT_KEY;
  stringDataVec[0].value = "user";

  data.settingsValues = settingsValues;
  settingsValues.stringData = stringDataVec;
  data.settingsValues = settingsValues;
  data.settingsId = SettingsId::USER_AGENT_STRING;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue_1_1(data, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(SettingsId::USER_AGENT_STRING, res1.args->settingsData.settingsId);  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, SetSettingsDataImsServiceEnableTest) {
  ALOGE("in vts set rcs settings.. sleeping for sometime");
  //sleep(15);
  SettingsData data;
  SettingsValues settingsValues;

  //vector<KeyValuePairTypeInt> intDataVec;
  vector<KeyValuePairTypeBool> boolDataVec;
  vector<KeyValuePairTypeString> stringDataVec;
  vector<KeyValuePairTypeInt> intDataVec;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[0].key = (uint32_t)ImsServiceEnableConfigKeys::VIDEOTELEPHONY_ENABLED_KEY;
  boolDataVec[0].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[1].key = (uint32_t)ImsServiceEnableConfigKeys::WIFI_CALLING_ENABLED_KEY;
  boolDataVec[1].value = 1;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[2].key = (uint32_t)ImsServiceEnableConfigKeys::VOLTE_ENABLED_KEY;
  boolDataVec[2].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[3].key = (uint32_t)ImsServiceEnableConfigKeys::MOBILE_DATA_ENABLED_KEY;
  boolDataVec[3].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[4].key = (uint32_t)ImsServiceEnableConfigKeys::WIFI_CALLING_IN_ROAMING_ENABLED_KEY;
  boolDataVec[4].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[5].key = (uint32_t)ImsServiceEnableConfigKeys::IMS_SERVICE_ENABLED_KEY;
  boolDataVec[5].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[6].key = (uint32_t)ImsServiceEnableConfigKeys::UT_ENABLED_KEY;
  boolDataVec[6].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[7].key = (uint32_t)ImsServiceEnableConfigKeys::SMS_ENABLED_KEY;
  boolDataVec[7].value = 1;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[8].key = (uint32_t)ImsServiceEnableConfigKeys::DAN_ENABLED_KEY;
  boolDataVec[8].value = 1;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[9].key = (uint32_t)ImsServiceEnableConfigKeys::USSD_ENABLED_KEY;
  boolDataVec[9].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[10].key = (uint32_t)ImsServiceEnableConfigKeys::MWI_ENABLED_KEY;
  boolDataVec[10].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[11].key = (uint32_t)ImsServiceEnableConfigKeys::PRESENCE_ENABLED_KEY;
  boolDataVec[11].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[12].key = (uint32_t)ImsServiceEnableConfigKeys::AUTOCONFIG_ENABLED_KEY;
  boolDataVec[12].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[13].key = (uint32_t)ImsServiceEnableConfigKeys::XDM_CLIENT_ENABLED_KEY;
  boolDataVec[13].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[14].key = (uint32_t)ImsServiceEnableConfigKeys::RCS_MESSAGING_ENABLED_KEY;
  boolDataVec[14].value = 1;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[15].key = (uint32_t)ImsServiceEnableConfigKeys::CALL_MODE_PREF_ROAM_ENABLED_KEY;
  boolDataVec[15].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[16].key = (uint32_t)ImsServiceEnableConfigKeys::RTT_ENABLED_KEY;
  boolDataVec[16].value = 0;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[17].key = (uint32_t)ImsServiceEnableConfigKeys::CARRIER_CONFIG_ENABLED_KEY;
  boolDataVec[17].value = 1;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[18].key = (uint32_t)ImsServiceEnableConfigKeys::OPTIONS_ENABLED_KEY;
  boolDataVec[18].value = 1;

  boolDataVec.push_back(KeyValuePairTypeBool());
  boolDataVec[19].key = (uint32_t)ImsServiceEnableConfigKeys::CALL_COMPOSER_ENABLED_KEY;
  boolDataVec[19].value = 1;

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[0].key = (uint32_t)ImsServiceEnableConfigKeys::CALL_MODE_PREFERENCE_KEY;
  intDataVec[0].value = 2;

  intDataVec.push_back(KeyValuePairTypeInt());
  intDataVec[1].key = (uint32_t)ImsServiceEnableConfigKeys::SERVICE_MASK_BY_NETWORK_ENABLED_KEY;
  intDataVec[1].value = 1;

  stringDataVec.push_back(KeyValuePairTypeString());
  stringDataVec[0].key = (uint32_t)ImsServiceEnableConfigKeys::WIFI_PROVISIONING_ID_KEY;
  stringDataVec[0].value = "abcd";

  //settingsValues.intData = intDataVec;
  settingsValues.boolData = boolDataVec;
  settingsValues.stringData = stringDataVec;
  settingsValues.intData = intDataVec;
  data.settingsValues = settingsValues;
  data.settingsId = SettingsId::IMS_SERVICE_ENABLE_CONFIG;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue_1_1(data, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(SettingsId::IMS_SERVICE_ENABLE_CONFIG, res1.args->settingsData.settingsId);  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, RestorePresenceConfigTest) {
  ALOGE("in vts SetSettingsDataPresenceConfigTest rcs settings");
  SettingsData data;

  data = presenceSettings;
  data.settingsId = SettingsId::PRESENCE_CONFIG;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue_1_1(data, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(SettingsId::PRESENCE_CONFIG, res1.args->settingsData.settingsId);  }
  else
  {
     ALOGE("This functionality is not supported");

  }

}

TEST_F(ImsConfigServiceGenericTest, RestoreStandaloneMessagingTest) {
  ALOGE("in vts SetSettingsDataStandaloneMessagingTest rcs settings");
  SettingsData data;

  //settingsValues.stringData = stringDataVec;
  data = standAloneMsgSettings;
  data.settingsId = SettingsId::STANDALONE_MESSAGING_CONFIG;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue_1_1(data, userData);
    //sleep(5);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(SettingsId::STANDALONE_MESSAGING_CONFIG, res1.args->settingsData.settingsId);  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}


TEST_F(ImsConfigServiceGenericTest, RestoreUserAgentTest) {
  ALOGE("in vts SetSettingsDataUserAgentTest rcs settings");
  SettingsData data;
  data =  userAgentStringSettings;
  data.settingsId = SettingsId::USER_AGENT_STRING;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue_1_1(data, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(SettingsId::USER_AGENT_STRING, res1.args->settingsData.settingsId);  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, RestoreImsServiceEnableTest) {
  ALOGE("in vts set rcs settings.. sleeping for sometime");
  //sleep(15);
  SettingsData data;
  data = imsServiceEnableConfig;
  data.settingsId = SettingsId::IMS_SERVICE_ENABLE_CONFIG;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    auto result = pService->setSettingsValue_1_1(data, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(SettingsId::IMS_SERVICE_ENABLE_CONFIG, res1.args->settingsData.settingsId);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}


TEST_F(ImsConfigServiceGenericTest, GetSettingsDataPresenceTest1) {

  if(pService != nullptr)
  {
    auto result = pService->getSettingsValue(SettingsId::PRESENCE_CONFIG, userData);
    auto res = mListener->WaitForCallback(kCallbackOnGetSettingsResponse_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(SettingsId::PRESENCE_CONFIG, res.args->settingsData.settingsId);
    EXPECT_EQ(userData, res.args->userData);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, GetSettingsDataStandaloneTest1) {

  if(pService != nullptr)
  {
    auto result = pService->getSettingsValue(SettingsId::STANDALONE_MESSAGING_CONFIG, userData);
    auto res = mListener->WaitForCallback(kCallbackOnGetSettingsResponse_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(SettingsId::STANDALONE_MESSAGING_CONFIG, res.args->settingsData.settingsId);
    EXPECT_EQ(userData, res.args->userData);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, GetSettingsDataUserAgentTest1) {

  if(pService != nullptr)
  {
    auto result = pService->getSettingsValue(SettingsId::USER_AGENT_STRING, userData);
    auto res = mListener->WaitForCallback(kCallbackOnGetSettingsResponse_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(SettingsId::USER_AGENT_STRING, res.args->settingsData.settingsId);
    EXPECT_EQ(userData, res.args->userData);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}
TEST_F(ImsConfigServiceGenericTest, GetSettingsDataImsServiceTest1) {

  if(pService != nullptr)
  {
    auto result = pService->getSettingsValue(SettingsId::IMS_SERVICE_ENABLE_CONFIG, userData);
    auto res = mListener->WaitForCallback(kCallbackOnGetSettingsResponse_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(SettingsId::IMS_SERVICE_ENABLE_CONFIG, res.args->settingsData.settingsId);
    EXPECT_EQ(userData, res.args->userData);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, DeRegisterForIndications) {
  ALOGE("in vts DeRegisterForIndications");

  if(pService != nullptr)
  {
    ALOGE("calling DeRegisterForIndications");
    uint32_t result = pService->deregisterForSettingsChange(userData);
    EXPECT_EQ(RequestStatus::OK,(RequestStatus)result);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}

TEST_F(ImsConfigServiceGenericTest, SetAppIdTest) {
  ALOGE("in vts SetSettingsDataStandaloneMessagingTest rcs settings");
  SettingsData data;
  SettingsValues settingsValues;
  settingsValues.stringData.resize(1);

  settingsValues.stringData[0].key = (uint32_t)StandaloneMessagingConfigKeys::APP_ID_KEY;
  settingsValues.stringData[0].value = userProvidedAppId;
  
  data.settingsValues = settingsValues;
  data.settingsId = SettingsId::STANDALONE_MESSAGING_CONFIG;

  if(pService != nullptr)
  {
    ALOGE("calling set rcs settings");
    uint32_t ret = pService->registerForSettingsChange(userData);
    EXPECT_EQ(RequestStatus::OK,(RequestStatus)ret);
    auto result = pService->setSettingsValue_1_1(data, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1 = mListener->WaitForCallback(kCallbackOnGetUpdatedSettings_1_1);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(SettingsId::STANDALONE_MESSAGING_CONFIG, res1.args->settingsData.settingsId);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }

}
