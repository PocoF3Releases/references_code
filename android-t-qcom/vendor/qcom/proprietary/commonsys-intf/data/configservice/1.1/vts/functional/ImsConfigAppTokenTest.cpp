/*****************************************************************************
Copyright (c) 2021-2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/
#include "ImsConfigAppTokenTest.h"

TEST_F(ImsConfigAppTokenTest, AppTokenTest) {
  ALOGE("%s : ImsConfigAppTokenTest - in vts AppTokenTest", LOG_TAG);
  if(pService != nullptr) {
    //wait for modem triggers
    auto res = mListener->WaitForCallback(kCallbackOnTokenFetchRequest);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_NE(0, res.args->requestId);
    //update status
    auto res2 = pService->updateTokenFetchStatus(res.args->requestId, res.args->tokenType, StatusCode::SUCCESS, userData++ );
    //set App token
    pService->setAppToken(g_configValues[AppToken_key].c_str(), userData++);
    auto res1 = mListener->WaitForCallback(kCallbackOnCommandStatus);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res1.args->status);
   }
   else
   {
     ALOGE("ImsConfigAppTokenTest : This functionality is not supported");
   }
}

TEST_F(ImsConfigAppTokenTest, SendAppTokenTest) {
  ALOGE("%s :ImsConfigAppTokenTest - in vts SendAppTokenTest", LOG_TAG);
  if(pService != nullptr) {
    pService->setAppToken("someRandom Data", userData++);
    auto res1 = mListener->WaitForCallback(kCallbackOnCommandStatus);
    EXPECT_TRUE(res1.no_timeout);
    EXPECT_EQ(RequestStatus::OK, res1.args->status);
  }
  else
  {
    ALOGE("ImsConfigAppTokenTest : This functionality is not supported");
  }
}
