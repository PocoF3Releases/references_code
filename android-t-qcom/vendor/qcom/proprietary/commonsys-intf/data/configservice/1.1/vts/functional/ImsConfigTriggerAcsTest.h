/*****************************************************************************
Copyright (c) 2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/
#ifndef IMS_CONFIG_TRIGGER_ACS_TEST_H
#define IMS_CONFIG_TRIGGER_ACS_H

#include "ImsConfigCommon.h"
#include "ImsConfigServiceGenericTest.h"

extern std::map<std::string, std::string> g_configValues;

class ImsConfigTriggerAcsTest : public ::testing::VtsHalHidlTargetTestBase, public ImsConfigVtsTest
{
public:
  virtual void SetUp() override
  {
    ALOGI("%s :ImsConfigTriggerAcsTest - Setup function",LOG_TAG);
    factory = IImsFactory::getService();
    IImsFactory::StatusCode status = IImsFactory::StatusCode::NOT_SUPPORTED;
    mListener = new mockConfigServiceListener();
    if(factory != nullptr)
    {
      for(auto it : g_configValues)
      {
        std::cout << "ImsConfigTriggerAcsTest : Setup g_configValues [" << it.first << "] value[" << it.second << "]\n";
        ALOGI("ImsConfigAppTokenTest : Setup g_configValues[%s][%s]", it.first.c_str(), it.second.c_str() );
      }
      int testI = atoi(g_configValues[slot_id_key].c_str());
      ALOGI("ImsConfigTriggerAcsTest : Setup testI[%d]", testI);
      std::cout << "ImsConfigTriggerAcsTest : Setup testI[" << testI <<"]";
      factory->createConfigService_1_1(atoi(g_configValues[slot_id_key].c_str()),
                                    mListener,
                                   [&](IImsFactory::StatusCode _status, sp<IConfigService> config) {
                                   status = _status;
                                   pService = config;
                                   /* end Lamda Func*/ } );

    }
    userData++;
  };
  sp<IImsFactory> factory = nullptr;
  sp<IConfigService> pService = nullptr;
  mockConfigServiceListener* mListener;
};
#endif
