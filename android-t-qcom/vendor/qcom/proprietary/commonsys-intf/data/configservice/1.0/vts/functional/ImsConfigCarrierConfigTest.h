/*****************************************************************************
Copyright (c) 2021 Qualcomm Technologies, Inc.
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
#include "ImsConfigServiceGenericTest.h"



class ImsConfigCarrierConfigTest : public ::testing::VtsHalHidlTargetTestBase, public ImsConfigVtsTest
{
public:
  virtual void SetUp() override
  {
    ALOGI("%s - Setup function",LOG_TAG);
    factory = IImsFactory::getService();
    IImsFactory::StatusCode status = IImsFactory::StatusCode::NOT_SUPPORTED;
    mListener = new mockConfigServiceListener();
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

  };
  std::string compressFileData(bool compressData);
  uint32_t userData = 1001;
  sp<IImsFactory> factory = nullptr;
  sp<IConfigService> pService = nullptr;
  mockConfigServiceListener* mListener;
};
