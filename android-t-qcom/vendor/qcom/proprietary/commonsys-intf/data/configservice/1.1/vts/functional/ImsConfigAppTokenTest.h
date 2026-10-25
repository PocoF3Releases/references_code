/*****************************************************************************
Copyright (c) 2021-2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/
#ifndef IMS_CONFIG_APPTOKEN_TEST_H
#define IMS_CONFIG_APPTOKEN_TEST_H

#include "ImsConfigCommon.h"
#include "ImsConfigServiceGenericTest.h"

extern std::map<std::string, std::string> g_configValues;

class ImsConfigAppTokenTest : public ::testing::VtsHalHidlTargetTestBase, public ImsConfigVtsTest
{
public:
  virtual void SetUp() override
  {
    ALOGI("%s :ImsConfigAppTokenTest - Setup function",LOG_TAG);
    factory = IImsFactory::getService();
    IImsFactory::StatusCode status = IImsFactory::StatusCode::NOT_SUPPORTED;
    mListener = new mockConfigServiceListener();
    if(factory != nullptr)
    {
      for(auto it : g_configValues)
      {
        std::cout << "ImsConfigAppTokenTest : Setup g_configValues [" << it.first << "] value[" << it.second << "]\n";
        ALOGI("ImsConfigAppTokenTest : Setup g_configValues[%s][%s]", it.first.c_str(), it.second.c_str() );
      }
      int testI = atoi(g_configValues[slot_id_key].c_str());
      ALOGI("ImsConfigAppTokenTest : Setup testI[%d]", testI);
      std::cout << "ImsConfigAppTokenTest : Setup testI[" << testI <<"]";
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
