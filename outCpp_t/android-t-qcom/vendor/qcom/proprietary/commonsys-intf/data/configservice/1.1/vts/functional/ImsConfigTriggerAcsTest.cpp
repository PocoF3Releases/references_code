/*****************************************************************************
Copyright (c) 2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/
#include "ImsConfigTriggerAcsTest.h"

TEST_F(ImsConfigTriggerAcsTest, TriggerAcsRequestTest) {
  ALOGE("in vts TriggerAcsRequestTest");

  if(pService != nullptr)
  {
    ALOGE("calling TriggerAcsRequest");
    uint32_t result = pService->triggerAcsRequest(AutoConfigTriggerReason::AUTOCONFIG_INVALID_TOKEN, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1  = mListener->WaitForCallback(kCallbackOnAutoConfigurationReceived);
    EXPECT_EQ(RequestStatus::OK,(RequestStatus)result);
    EXPECT_EQ(RequestStatus::OK,(RequestStatus)res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_EQ(RequestStatus::OK,(RequestStatus)res1.args->status);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }
}

TEST_F(ImsConfigTriggerAcsTest, TriggerAcsReqForMaxRetry) {
  ALOGE("in vts TriggerAcsRequestTest");

  if(pService != nullptr)
  {
    ALOGE("calling TriggerAcsRequest");
    uint32_t result = pService->triggerAcsRequest(AutoConfigTriggerReason::AUTOCONFIG_INVALID_TOKEN, userData);
    auto res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    EXPECT_EQ(RequestStatus::OK,(RequestStatus)result);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_EQ(RequestStatus::RETRY_ATTEMPTS_MAXED_OUT,(RequestStatus)res.args->status);
    
    userData++;
    result = pService->triggerAcsRequest(AutoConfigTriggerReason::AUTOCONFIG_FACTORY_RESET, userData);
    res = mListener->WaitForCallback(kCallbackOnCommandStatus_1_1);
    auto res1  = mListener->WaitForCallback(kCallbackOnAutoConfigurationReceived);
    EXPECT_EQ(RequestStatus::OK,(RequestStatus)res.args->status);
    EXPECT_EQ(userData, res.args->userData);
    EXPECT_EQ(RequestStatus::OK,(RequestStatus)res1.args->status);
  }
  else
  {
     ALOGE("This functionality is not supported");
  }
}
