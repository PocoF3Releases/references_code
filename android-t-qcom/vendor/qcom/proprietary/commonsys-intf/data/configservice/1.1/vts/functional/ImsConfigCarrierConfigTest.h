/*****************************************************************************
Copyright (c) 2021-2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/
#ifndef IMS_CONFIG_CARRIER_TEST_H
#define IMS_CONFIG_CARRIER_TEST_H

#include <vendor/qti/ims/factory/2.0/IImsFactory.h>
#include <vendor/qti/ims/configservice/1.0/IConfigService.h>
#include "ImsConfigServiceGenericTest.h"
#include "ImsConfigCommon.h"

extern std::map<std::string, std::string> g_configValues;

class ImsConfigCarrierConfigTest : public ::testing::VtsHalHidlTargetTestBase, public ImsConfigVtsTest
{
public:
  virtual void SetUp() override
  {
    ALOGI("%s : ImsConfigCarrierConfigTest - Setup function",LOG_TAG);
    factory = IImsFactory::getService();
    IImsFactory::StatusCode status = IImsFactory::StatusCode::NOT_SUPPORTED;
    mListener = new mockConfigServiceListener();
    if(factory != nullptr)
    {
      for(auto it : g_configValues)
      {
        std::cout << "ImsConfigCarrierConfigTest : Setup g_configValues [" << it.first << "] value[" << it.second << "]\n";
        ALOGI("ImsConfigCarrierConfigTest : Setup g_configValues[%s][%s]", it.first.c_str(), it.second.c_str() );
      }
      int testI = atoi(g_configValues[slot_id_key].c_str());
      ALOGI("ImsConfigCarrierConfigTest : Setup testI[%d]", testI);
      std::cout << "ImsConfigCarrierConfigTest : Setup testI[" << testI <<"]";
      factory->createConfigService_1_1(atoi(g_configValues[slot_id_key].c_str()),
                                    mListener,
                                   [&](IImsFactory::StatusCode _status, sp<IConfigService> config) {
                                   status = _status;
                                   pService = config;
                                   /* end Lamda Func*/ } );

    }
    userData++;
  };
  std::string compressFileData(bool compressData);
  sp<IImsFactory> factory = nullptr;
  sp<IConfigService> pService = nullptr;
  mockConfigServiceListener* mListener;
};
#endif
